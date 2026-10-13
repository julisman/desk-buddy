// focus.h — Pomodoro timer: fokus -> istirahat -> fokus ...
#pragma once

#include <stdint.h>

enum class FocusPhase { Work, Break };
enum class FocusState { Idle, Running, Paused };
enum class FocusEvent { None, WorkDone, BreakDone };

void focusInit();
void focusClick();          // start / pause / resume
void focusTurn(int delta);  // saat Idle/Paused: ubah durasi (menit)
FocusEvent focusUpdate(uint32_t now);

FocusPhase focusPhase();
FocusState focusState();
bool focusRunning();
uint32_t focusRemainingMs();
float focusFraction();      // sisa waktu 0..1
int focusCompleted();       // jumlah sesi fokus selesai sejak boot
