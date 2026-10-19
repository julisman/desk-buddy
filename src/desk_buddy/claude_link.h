// claude_link.h — Claude Code status from the laptop, sent over WiFi.
//
// The laptop (Claude Code status line + hooks, see tools/claude-desk-buddy.sh) POSTs compact
// JSON to http://deskbuddy.local/claude. It only holds numbers & status, never prompts.
//   {"type":"status","session":"…","project":"ESP32","model":"Opus 5.5","ctx":24,
//    "session_pct":13,"session_resets":1791900600,"week_pct":74,"week_resets":1791968400}
//   {"type":"hook","session":"…","project":"ESP32","event":"Stop","mode":"auto",
//    "notification":"permission_prompt","tool":"Bash"}
//
// Multiple concurrent Claude Code sessions are tracked separately (max CLAUDE_MAX_SESSIONS).
// Session/week limits belong to the account, so they're shared by all sessions (latest value wins).
#pragma once

#include <stdint.h>

enum class ClaudeActivity { None, Working, NeedsYou, Done, Idle };

#define CLAUDE_MAX_SESSIONS 4

// View for the screen: selected session + account limits + summary of all sessions
struct ClaudeState {
  bool valid = false;        // received data since boot
  int count = 0;             // number of active sessions
  int index = -1;            // position of the selected session (0..count-1), -1 = none
  ClaudeActivity activities[CLAUDE_MAX_SESSIONS] = {};  // status of each session, in position order

  // Selected session
  char project[24] = "";
  char model[24] = "";
  char mode[16] = "";        // permission mode: auto, plan, default, ...
  char tool[24] = "";        // tool asking for permission (when NeedsYou)
  float ctxPct = -1;         // -1 = unknown
  ClaudeActivity activity = ClaudeActivity::None;
  uint32_t updatedMs = 0;    // millis() of the last data from this session

  // Account limits
  float sessionPct = -1;
  float weekPct = -1;
  uint32_t sessionResets = 0;  // epoch seconds, 0 = unknown
  uint32_t weekResets = 0;
};

// One-shot events for loop() (sounds, mode switch). Read + cleared by claudeTakeEvents().
enum : uint8_t {
  CLAUDE_EV_NEEDS_YOU = 1 << 0,
  CLAUDE_EV_DONE = 1 << 1,       // done, >= CLAUDE_CHIRP_MIN_MS since the prompt
  CLAUDE_EV_WORKING = 1 << 2,
  CLAUDE_EV_ACTIVITY = 1 << 3,   // any hook at all (to keep the screen awake)
};

// Start the HTTP server on port 80. Call once after WiFi connects.
void claudeLinkStart();
bool claudeLinkStarted();

ClaudeState claudeState();
uint8_t claudeTakeEvents();

// Select another session (turn knob): delta +1 / -1, wraps around
void claudeSelect(int delta);
