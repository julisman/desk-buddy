#!/bin/bash
# Kirim status Claude Code ke Desk Buddy (mode CLAUDE).
#
# Yang dikirim cuma angka & status (model, context %, limit sesi/minggu, nama folder,
# jenis event). Prompt, balasan Claude, isi tool, dan path lengkap TIDAK dikirim.
#
#   status line : echo "$input" | claude-desk-buddy.sh status
#   hook        : claude-desk-buddy.sh hook          (JSON hook dari stdin)
#   tes         : claude-desk-buddy.sh demo          (data palsu, lihat layar)
#
# Alamat bisa diganti: DESK_BUDDY_URL=http://192.168.1.50/claude
URL="${DESK_BUDDY_URL:-http://deskbuddy.local/claude}"

send() {
  # Background + timeout pendek: Claude Code tidak pernah menunggu Desk Buddy.
  # -4: tanpa ini macOS coba IPv6 dulu untuk .local dan nunggu ~5 detik.
  curl -4 -s -m 2 -H 'Content-Type: application/json' --data "$1" "$URL" >/dev/null 2>&1 &
}

case "$1" in
  status)
    body=$(jq -c '{
        type: "status",
        session: .session_id,
        project: ((.workspace.current_dir // .cwd // "") | split("/") | last),
        model: .model.display_name,
        ctx: .context_window.used_percentage,
        session_pct: .rate_limits.five_hour.used_percentage,
        session_resets: .rate_limits.five_hour.resets_at,
        week_pct: .rate_limits.seven_day.used_percentage,
        week_resets: .rate_limits.seven_day.resets_at
      } | with_entries(select(.value != null))' 2>/dev/null)
    [ -n "$body" ] && send "$body"
    ;;
  hook)
    body=$(jq -c '{
        type: "hook",
        session: .session_id,
        project: ((.cwd // "") | split("/") | last),
        event: .hook_event_name,
        mode: .permission_mode,
        notification: .notification_type,
        tool: .tool_name
      } | with_entries(select(.value != null))' 2>/dev/null)
    [ -n "$body" ] && send "$body"
    ;;
  demo)
    now=$(date +%s)
    step() { echo "-> $2"; curl -4 -s -m 3 -H 'Content-Type: application/json' --data "$1" "$URL"; echo; sleep "$3"; }
    step "{\"type\":\"status\",\"session\":\"demo\",\"project\":\"ESP32\",\"model\":\"Opus 5.5\",\"ctx\":24,\"session_pct\":13,\"session_resets\":$((now+7200)),\"week_pct\":74,\"week_resets\":$((now+200000))}" "status (ctx 24%, session 13%, week 74%)" 2
    step '{"type":"hook","session":"demo","project":"ESP32","event":"UserPromptSubmit","mode":"auto"}' "working" 5
    step '{"type":"hook","session":"demo","project":"ESP32","event":"Notification","notification":"permission_prompt","tool":"Bash","mode":"default"}' "needs you (alarm)" 6
    step '{"type":"hook","session":"demo","project":"ESP32","event":"PostToolUse","tool":"Bash","mode":"default"}' "approved, working again" 3
    step "{\"type\":\"status\",\"session\":\"demo\",\"project\":\"ESP32\",\"model\":\"Opus 5.5\",\"ctx\":92,\"session_pct\":81,\"week_pct\":95}" "status (high usage: yellow/red bars)" 2
    step '{"type":"hook","session":"demo","project":"ESP32","event":"Stop","mode":"auto"}' "done" 0
    ;;
  *)
    echo "usage: $0 status|hook|demo" >&2
    exit 2
    ;;
esac
