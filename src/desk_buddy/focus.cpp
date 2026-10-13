// focus.cpp — Pomodoro. Fokus selesai -> istirahat otomatis jalan.
// Istirahat selesai -> balik ke fokus, menunggu tombol ditekan.
#include "focus.h"

#include <Arduino.h>

#include "app_config.h"
#include "log.h"

static FocusPhase phase = FocusPhase::Work;
static FocusState state = FocusState::Idle;
static int workMin = FOCUS_DEFAULT_MIN;
static int breakMin = BREAK_DEFAULT_MIN;
static uint32_t remainingMs = 0;  // valid saat Idle/Paused
static uint32_t endMs = 0;        // valid saat Running
static int completed = 0;

static uint32_t phaseTotalMs() {
  return (phase == FocusPhase::Work ? workMin : breakMin) * 60UL * 1000;
}

static void resetPhase(FocusPhase p) {
  phase = p;
  state = FocusState::Idle;
  remainingMs = phaseTotalMs();
}

static void start() {
  state = FocusState::Running;
  endMs = millis() + remainingMs;
  LOG("[focus] %s start, %lu s\n", phase == FocusPhase::Work ? "work" : "break", remainingMs / 1000);
}

void focusInit() { resetPhase(FocusPhase::Work); }

void focusClick() {
  switch (state) {
    case FocusState::Idle:
    case FocusState::Paused:
      start();
      break;
    case FocusState::Running:
      remainingMs = endMs - millis();
      state = FocusState::Paused;
      LOG("[focus] pause, sisa %lu s\n", remainingMs / 1000);
      break;
  }
}

void focusTurn(int delta) {
  if (state == FocusState::Running) return;
  int &minutes = phase == FocusPhase::Work ? workMin : breakMin;
  int maxMin = phase == FocusPhase::Work ? FOCUS_MAX_MIN : BREAK_MAX_MIN;
  minutes = constrain(minutes + delta, 1, maxMin);
  resetPhase(phase);  // putar saat pause = set ulang durasi
}

FocusEvent focusUpdate(uint32_t now) {
  if (state != FocusState::Running || (int32_t)(now - endMs) < 0) return FocusEvent::None;

  if (phase == FocusPhase::Work) {
    completed++;
    LOG("[focus] work selesai (%d sesi), mulai break\n", completed);
    resetPhase(FocusPhase::Break);
    start();
    return FocusEvent::WorkDone;
  }
  LOG("[focus] break selesai\n");
  resetPhase(FocusPhase::Work);
  return FocusEvent::BreakDone;
}

FocusPhase focusPhase() { return phase; }
FocusState focusState() { return state; }
bool focusRunning() { return state == FocusState::Running; }
int focusCompleted() { return completed; }

uint32_t focusRemainingMs() {
  if (state != FocusState::Running) return remainingMs;
  int32_t left = endMs - millis();
  return left > 0 ? left : 0;
}

float focusFraction() { return (float)focusRemainingMs() / phaseTotalMs(); }
