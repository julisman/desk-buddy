// claude_link.h — status Claude Code dari laptop, dikirim lewat WiFi.
//
// Laptop (status line + hooks Claude Code, lihat tools/claude-desk-buddy.sh) POST JSON
// ringkas ke http://deskbuddy.local/claude. Isinya cuma angka & status, bukan prompt.
//   {"type":"status","session":"…","project":"ESP32","model":"Opus 5.5","ctx":24,
//    "session_pct":13,"session_resets":1791900600,"week_pct":74,"week_resets":1791968400}
//   {"type":"hook","session":"…","project":"ESP32","event":"Stop","mode":"auto",
//    "notification":"permission_prompt","tool":"Bash"}
//
// Beberapa sesi Claude Code sekaligus dilacak terpisah (maks CLAUDE_MAX_SESSIONS).
// Limit sesi/minggu milik akun, jadi sama untuk semua sesi (nilai terbaru dipakai).
#pragma once

#include <stdint.h>

enum class ClaudeActivity { None, Working, NeedsYou, Done, Idle };

#define CLAUDE_MAX_SESSIONS 4

// Tampilan untuk layar: sesi yang dipilih + limit akun + ringkasan semua sesi
struct ClaudeState {
  bool valid = false;        // pernah terima data sejak boot
  int count = 0;             // jumlah sesi aktif
  int index = -1;            // posisi sesi yang dipilih (0..count-1), -1 = tidak ada
  ClaudeActivity activities[CLAUDE_MAX_SESSIONS] = {};  // status tiap sesi, urut posisi

  // Sesi yang dipilih
  char project[24] = "";
  char model[24] = "";
  char mode[16] = "";        // permission mode: auto, plan, default, ...
  char tool[24] = "";        // tool yang minta izin (kalau NeedsYou)
  float ctxPct = -1;         // -1 = belum tahu
  ClaudeActivity activity = ClaudeActivity::None;
  uint32_t updatedMs = 0;    // millis() data terakhir dari sesi ini

  // Limit akun
  float sessionPct = -1;
  float weekPct = -1;
  uint32_t sessionResets = 0;  // epoch detik, 0 = belum tahu
  uint32_t weekResets = 0;
};

// Event satu kali untuk loop() (bunyi, ganti mode). Diambil + dihapus oleh claudeTakeEvents().
enum : uint8_t {
  CLAUDE_EV_NEEDS_YOU = 1 << 0,
  CLAUDE_EV_DONE = 1 << 1,       // selesai, >= CLAUDE_CHIRP_MIN_MS sejak prompt
  CLAUDE_EV_WORKING = 1 << 2,
  CLAUDE_EV_ACTIVITY = 1 << 3,   // ada hook apa pun (untuk menjaga layar tetap bangun)
};

// Start HTTP server port 80. Panggil sekali setelah WiFi connect.
void claudeLinkStart();
bool claudeLinkStarted();

ClaudeState claudeState();
uint8_t claudeTakeEvents();

// Pilih sesi lain (putar knob): delta +1 / -1, berputar
void claudeSelect(int delta);
