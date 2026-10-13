// buzzer.h — pemutar melodi non-blocking.
#pragma once

#include <stdint.h>

struct Note {
  uint16_t freq;  // Hz, 0 = diam
  uint16_t ms;    // durasi, 0 = akhir melodi
};

// Melodi bawaan
extern const Note SND_BOOT[];
extern const Note SND_TICK[];
extern const Note SND_BOOP[];
extern const Note SND_MODE[];
extern const Note SND_START[];
extern const Note SND_PAUSE[];
extern const Note SND_FOCUS_DONE[];
extern const Note SND_BREAK_DONE[];
extern const Note SND_WAKE[];
extern const Note SND_PURR[];
extern const Note SND_ALERT[];  // butuh perhatian (mis. Claude Code minta izin)
extern const Note SND_CHIRP[];  // tugas selesai

void buzzerInit();
void buzzerPlay(const Note *melody);  // ganti melodi yang sedang main
void buzzerUpdate();                   // panggil sesering mungkin dari loop()
