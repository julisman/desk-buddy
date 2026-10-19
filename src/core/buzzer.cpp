// buzzer.cpp — passive buzzer uses LEDC tone, active buzzer uses on/off.
#include "buzzer.h"

#include <Arduino.h>

#include "board.h"

const Note SND_BOOT[]       = {{1047, 70}, {1319, 70}, {1568, 110}, {0, 0}};
const Note SND_TICK[]       = {{2400, 6}, {0, 0}};
const Note SND_BOOP[]       = {{880, 50}, {1320, 70}, {0, 0}};
const Note SND_MODE[]       = {{1568, 35}, {0, 0}};
const Note SND_START[]      = {{1047, 60}, {1568, 90}, {0, 0}};
const Note SND_PAUSE[]      = {{1568, 60}, {1047, 90}, {0, 0}};
const Note SND_FOCUS_DONE[] = {{1568, 120}, {0, 60}, {1568, 120}, {0, 60}, {2093, 260}, {0, 0}};
const Note SND_BREAK_DONE[] = {{1047, 150}, {1319, 150}, {1568, 150}, {2093, 300}, {0, 0}};
const Note SND_WAKE[]       = {{660, 60}, {990, 80}, {0, 0}};
const Note SND_ALERT[]      = {{1760, 80}, {0, 60}, {1760, 80}, {0, 60}, {2349, 160}, {0, 0}};
const Note SND_CHIRP[]      = {{1319, 50}, {1760, 50}, {2637, 90}, {0, 0}};
const Note SND_PURR[]       = {{523, 40}, {0, 40}, {659, 60}, {0, 0}};

static const Note *current = nullptr;
static uint32_t noteEndMs = 0;

static void toneOn(uint16_t freq) {
#if BUZZER_PASSIVE
  ledcWriteTone(BUZZER_PWM_CH, freq);   // set frequency (50% duty)
  ledcWrite(BUZZER_PWM_CH, BUZZER_DUTY);  // lower duty: smaller peak current
#else
  (void)freq;
  digitalWrite(PIN_BUZZER, BUZZER_ACTIVE_LOW ? LOW : HIGH);
#endif
}

static void toneOff() {
#if BUZZER_PASSIVE
  ledcWriteTone(BUZZER_PWM_CH, 0);
#endif
  // Make sure the pin is at its "silent" level (matters for active-low modules)
#if BUZZER_PASSIVE && BUZZER_ACTIVE_LOW
  ledcWrite(BUZZER_PWM_CH, 255);
#elif !BUZZER_PASSIVE
  digitalWrite(PIN_BUZZER, BUZZER_ACTIVE_LOW ? HIGH : LOW);
#endif
}

static void startNote() {
  if (current->freq) toneOn(current->freq);
  else toneOff();
  noteEndMs = millis() + current->ms;
}

void buzzerInit() {
  if (!BUZZER_ENABLED) return;  // pin left as input (high-Z)
#if BUZZER_PASSIVE
  ledcSetup(BUZZER_PWM_CH, 2000, 8);
  ledcAttachPin(PIN_BUZZER, BUZZER_PWM_CH);
#else
  pinMode(PIN_BUZZER, OUTPUT);
#endif
  toneOff();
}

void buzzerPlay(const Note *melody) {
  if (!BUZZER_ENABLED) return;
  current = melody;
  startNote();
}

void buzzerUpdate() {
  if (!current || (int32_t)(millis() - noteEndMs) < 0) return;
  current++;
  if (current->ms == 0) {
    toneOff();
    current = nullptr;
    return;
  }
  startNote();
}
