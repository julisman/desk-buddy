// app_config.h — Desk Buddy app constants. Hardware is in core/board.h.
#pragma once

#include "board.h"

#define APP_VERSION          "1.0.1"

#define HEART_INTERVAL_MS    350   // interval between new hearts while petted

#define COLOR_TRACK          0x2104
#define COLOR_FOCUS          0xFD20  // orange
#define COLOR_BREAK          0x07E8  // green

// ---------- Behaviour ----------
#define IDLE_SLEEP_MS        (5UL * 60 * 1000)  // daytime: sleep after 5 min idle
#define NIGHT_IDLE_SLEEP_MS  (60UL * 1000)      // night: 1 min
#define NIGHT_START_HOUR     23
#define NIGHT_END_HOUR       7

#define FOCUS_DEFAULT_MIN    25
#define BREAK_DEFAULT_MIN    5
#define FOCUS_MAX_MIN        90
#define BREAK_MAX_MIN        30

// ---------- Claude Code status (CLAUDE mode, see claude_link.h) ----------
#define CLAUDE_CHIRP_MIN_MS     20000   // "done" sound only if Claude worked >= 20 s
#define CLAUDE_STALE_MS         (10UL * 60 * 1000)  // "working" with no update for 10 min = treat as stale
#define CLAUDE_ALERT_REPEAT_MS  30000   // repeat the "needs you" sound every 30 s...
#define CLAUDE_ALERT_MAX        3       // ...at most 3 times (press knob = silence)
#define CLAUDE_SESSION_TIMEOUT_MS (2UL * 60 * 60 * 1000)  // sessions with no update for 2 h are removed

#define COLOR_CLAUDE         0xDBAA  // coral
#define COLOR_OK             0x07E8  // green  (< 70%)
#define COLOR_WARN           0xFE60  // yellow (70-89%)
#define COLOR_BAD            0xF800  // red    (>= 90%)
