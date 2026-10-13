#!/bin/bash
# Pasang / lepas integrasi Claude Code -> Desk Buddy (mode CLAUDE) di laptop ini.
#
#   tools/install-claude-hooks.sh             pasang
#   tools/install-claude-hooks.sh --uninstall lepas
#
# Yang diubah (backup *.bak-deskbuddy dibuat sekali sebelum perubahan pertama):
#   ~/.claude/settings.json      hooks UserPromptSubmit, PostToolUse, Notification, Stop, SessionEnd
#   script status line kamu      satu baris yang meneruskan JSON status ke Desk Buddy
# Berlaku untuk semua sesi Claude Code. Hooks aktif di sesi baru.
set -euo pipefail

SCRIPT="$(cd "$(dirname "$0")" && pwd)/claude-desk-buddy.sh"
SETTINGS="$HOME/.claude/settings.json"
MARK="# Desk Buddy:"
MODE="${1:-install}"

command -v jq >/dev/null || { echo "butuh jq: brew install jq" >&2; exit 1; }
[ -f "$SETTINGS" ] || echo '{}' > "$SETTINGS"
[ -f "$SETTINGS.bak-deskbuddy" ] || cp "$SETTINGS" "$SETTINGS.bak-deskbuddy"

# Script status line yang sekarang dipakai (kalau bentuknya "bash <file>")
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
echo "hooks: $([ "$MODE" = --uninstall ] && echo dilepas || echo terpasang) di $SETTINGS"

if [ -n "$statusline_file" ] && [ -f "$statusline_file" ]; then
  [ -f "$statusline_file.bak-deskbuddy" ] || cp "$statusline_file" "$statusline_file.bak-deskbuddy"
  python3 - "$statusline_file" "$SCRIPT" "$MODE" "$MARK" <<'EOF2'
import sys
path, script, mode, mark = sys.argv[1:]
lines = open(path).read().split("\n")
# Buang blok lama: baris penanda + baris sesudahnya
out, skip = [], 0
for l in lines:
    if skip: skip -= 1; continue
    if l.startswith(mark): skip = 1; continue
    out.append(l)
msg = "dilepas dari"
if mode != "--uninstall":
    try:
        i = out.index("input=$(cat)")
    except ValueError:
        print(f"status line: tidak ketemu 'input=$(cat)' di {path}. Tambahkan manual setelah JSON dibaca:")
        print(f'  echo "$input" | bash {script} status')
        sys.exit(0)
    out[i+1:i+1] = [f"{mark} send model / ctx % / usage limits to the desk display (in the background)",
                    f'echo "$input" | bash {script} status']
    msg = "terpasang di"
open(path, "w").write("\n".join(out))
print(f"status line: {msg} {path}")
EOF2
else
  echo "status line: belum ada script status line (statusLine.command di $SETTINGS)."
  echo "  Buat script yang membaca stdin ke \$input, lalu tambahkan:"
  echo "  echo \"\$input\" | bash $SCRIPT status"
fi
