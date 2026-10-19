#!/bin/bash
# Install / remove the Claude Code -> Desk Buddy (CLAUDE mode) integration on this laptop.
#
#   tools/install-claude-hooks.sh             install
#   tools/install-claude-hooks.sh --uninstall remove
#
# What gets changed (a *.bak-deskbuddy backup is made once, before the first change):
#   ~/.claude/settings.json      hooks UserPromptSubmit, PostToolUse, Notification, Stop, SessionEnd
#   your status line script      one line that forwards the status JSON to Desk Buddy
# Applies to all Claude Code sessions. Hooks take effect in new sessions.
set -euo pipefail

SCRIPT="$(cd "$(dirname "$0")" && pwd)/claude-desk-buddy.sh"
SETTINGS="$HOME/.claude/settings.json"
MARK="# Desk Buddy:"
MODE="${1:-install}"

command -v jq >/dev/null || { echo "needs jq: brew install jq" >&2; exit 1; }
[ -f "$SETTINGS" ] || echo '{}' > "$SETTINGS"
[ -f "$SETTINGS.bak-deskbuddy" ] || cp "$SETTINGS" "$SETTINGS.bak-deskbuddy"

# The status line script currently in use (if it has the form "bash <file>")
statusline_file=$(jq -r '.statusLine.command // ""' "$SETTINGS" | sed -E 's/^(bash|sh) +//; s|^~|'"$HOME"'|')

python3 - "$SETTINGS" "$SCRIPT" "$MODE" <<'EOF'
import json, sys
path, script, mode = sys.argv[1:]
d = json.load(open(path))
cmd = f"bash {script} hook"
events = ["UserPromptSubmit", "PostToolUse", "Notification", "Stop", "SessionEnd"]
hooks = d.setdefault("hooks", {})
for e in events:
    groups = [g for g in hooks.get(e, [])
              if not any("claude-desk-buddy.sh" in h.get("command", "") for h in g.get("hooks", []))]
    if mode != "--uninstall":
        groups.append({"hooks": [{"type": "command", "command": cmd, "async": True}]})
    if groups: hooks[e] = groups
    else: hooks.pop(e, None)
if not hooks: d.pop("hooks")
json.dump(d, open(path, "w"), indent=2, ensure_ascii=False)
open(path, "a").write("\n")
EOF
echo "hooks: $([ "$MODE" = --uninstall ] && echo removed || echo installed) in $SETTINGS"

if [ -n "$statusline_file" ] && [ -f "$statusline_file" ]; then
  [ -f "$statusline_file.bak-deskbuddy" ] || cp "$statusline_file" "$statusline_file.bak-deskbuddy"
  python3 - "$statusline_file" "$SCRIPT" "$MODE" "$MARK" <<'EOF2'
import sys
path, script, mode, mark = sys.argv[1:]
lines = open(path).read().split("\n")
# Drop the old block: marker line + the line after it
out, skip = [], 0
for l in lines:
    if skip: skip -= 1; continue
    if l.startswith(mark): skip = 1; continue
    out.append(l)
msg = "removed from"
if mode != "--uninstall":
    try:
        i = out.index("input=$(cat)")
    except ValueError:
        print(f"status line: 'input=$(cat)' not found in {path}. Add this manually after the JSON is read:")
        print(f'  echo "$input" | bash {script} status')
        sys.exit(0)
    out[i+1:i+1] = [f"{mark} send model / ctx % / usage limits to the desk display (in the background)",
                    f'echo "$input" | bash {script} status']
    msg = "installed in"
open(path, "w").write("\n".join(out))
print(f"status line: {msg} {path}")
EOF2
else
  echo "status line: no status line script yet (statusLine.command in $SETTINGS)."
  echo "  Create a script that reads stdin into \$input, then add:"
  echo "  echo \"\$input\" | bash $SCRIPT status"
fi
