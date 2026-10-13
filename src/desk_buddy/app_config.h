// app_config.h — konstanta khusus app Desk Buddy. Hardware ada di core/board.h.
#pragma once

#include "board.h"

#define APP_VERSION          "1.0.0"

#define HEART_INTERVAL_MS    350   // jeda hati baru saat dielus

#define COLOR_TRACK          0x2104
#define COLOR_FOCUS          0xFD20  // oranye
#define COLOR_BREAK          0x07E8  // hijau

// ---------- Behaviour ----------
#define IDLE_SLEEP_MS        (5UL * 60 * 1000)  // siang: tidur setelah 5 menit idle
#define NIGHT_IDLE_SLEEP_MS  (60UL * 1000)      // malam: 1 menit
#define NIGHT_START_HOUR     23
#define NIGHT_END_HOUR       7

#define FOCUS_DEFAULT_MIN    25
#define BREAK_DEFAULT_MIN    5
#define FOCUS_MAX_MIN        90
#define BREAK_MAX_MIN        30

// ---------- Claude Code status (mode CLAUDE, lihat claude_link.h) ----------
#define CLAUDE_CHIRP_MIN_MS     20000   // bunyi "selesai" cuma kalau Claude kerja >= 20 detik
#define CLAUDE_STALE_MS         (10UL * 60 * 1000)  // "working" tanpa kabar 10 menit = anggap basi
#define CLAUDE_ALERT_REPEAT_MS  30000   // ulangi bunyi "butuh kamu" tiap 30 detik...
#define CLAUDE_ALERT_MAX        3       // ...maksimal 3 kali (tekan knob = diam)
#define CLAUDE_SESSION_TIMEOUT_MS (2UL * 60 * 60 * 1000)  // sesi tanpa kabar 2 jam dihapus

#define COLOR_CLAUDE         0xDBAA  // coral
#define COLOR_OK             0x07E8  // hijau  (< 70%)
#define COLOR_WARN           0xFE60  // kuning (70-89%)
#define COLOR_BAD            0xF800  // merah  (>= 90%)
