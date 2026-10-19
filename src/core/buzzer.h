// buzzer.h — non-blocking melody player.
#pragma once

#include <stdint.h>

struct Note {
  uint16_t freq;  // Hz, 0 = rest
  uint16_t ms;    // duration, 0 = end of melody
};

// Built-in melodies
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
extern const Note SND_ALERT[];  // needs attention (e.g. Claude Code asks for permission)
extern const Note SND_CHIRP[];  // task done

void buzzerInit();
void buzzerPlay(const Note *melody);  // replaces the melody currently playing
void buzzerUpdate();                   // call as often as possible from loop()
