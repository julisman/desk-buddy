// focus.h — Pomodoro timer: focus -> break -> focus ...
#pragma once

#include <stdint.h>

enum class FocusPhase { Work, Break };
enum class FocusState { Idle, Running, Paused };
enum class FocusEvent { None, WorkDone, BreakDone };

void focusInit();
void focusClick();          // start / pause / resume
void focusTurn(int delta);  // when Idle/Paused: change duration (minutes)
FocusEvent focusUpdate(uint32_t now);

FocusPhase focusPhase();
FocusState focusState();
bool focusRunning();
uint32_t focusRemainingMs();
float focusFraction();      // time remaining 0..1
int focusCompleted();       // focus sessions completed since boot
