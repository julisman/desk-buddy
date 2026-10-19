// focus.cpp — Pomodoro. Focus done -> break starts automatically.
// Break done -> back to focus, waiting for a button press.
#include "focus.h"

#include <Arduino.h>

#include "app_config.h"
#include "log.h"

static FocusPhase phase = FocusPhase::Work;
static FocusState state = FocusState::Idle;
static int workMin = FOCUS_DEFAULT_MIN;
static int breakMin = BREAK_DEFAULT_MIN;
static uint32_t remainingMs = 0;  // valid when Idle/Paused
static uint32_t endMs = 0;        // valid when Running
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
      LOG("[focus] pause, %lu s left\n", remainingMs / 1000);
      break;
  }
}

void focusTurn(int delta) {
  if (state == FocusState::Running) return;
  int &minutes = phase == FocusPhase::Work ? workMin : breakMin;
  int maxMin = phase == FocusPhase::Work ? FOCUS_MAX_MIN : BREAK_MAX_MIN;
  minutes = constrain(minutes + delta, 1, maxMin);
  resetPhase(phase);  // turning while paused = reset duration
}

FocusEvent focusUpdate(uint32_t now) {
  if (state != FocusState::Running || (int32_t)(now - endMs) < 0) return FocusEvent::None;

  if (phase == FocusPhase::Work) {
    completed++;
    LOG("[focus] work done (%d sessions), starting break\n", completed);
    resetPhase(FocusPhase::Break);
    start();
    return FocusEvent::WorkDone;
  }
  LOG("[focus] break done\n");
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
