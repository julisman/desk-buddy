// input.cpp — encoder is read via interrupts (quadrature decoder),
// button is polled every frame (33 ms is enough debounce).
#include "input.h"

#include <Arduino.h>
#include <soc/gpio_struct.h>

#include "board.h"

static portMUX_TYPE encMux = portMUX_INITIALIZER_UNLOCKED;
static volatile int32_t encSteps = 0;
static volatile uint8_t encState = 0;

// Quadrature transition table: index = (old state << 2) | new state.
// Invalid transitions (bounce) = 0.
static const int8_t QUAD_TABLE[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

static void IRAM_ATTR encoderIsr() {
  // Read the register directly (IRAM-safe, works even when flash cache is off)
  uint32_t in = GPIO.in;
  uint8_t ab = (((in >> PIN_ENC_CLK) & 1) << 1) | ((in >> PIN_ENC_DT) & 1);
  encState = ((encState << 2) | ab) & 0x0F;
  portENTER_CRITICAL_ISR(&encMux);
  encSteps += QUAD_TABLE[encState];
  portEXIT_CRITICAL_ISR(&encMux);
}

// Touch: on ESP32-S3 touchRead() goes UP when touched
static uint32_t touchBase = 0;
static uint32_t touchLast = 0;
static bool touched = false;
static uint32_t touchLowSinceMs = 0;  // since when the value is below the release threshold

static bool btnDown = false;
static bool longFired = false;
static uint32_t btnDownMs = 0;

void inputInit() {
  // KY-040 already has 10k pull-ups on CLK/DT (to the + pin), SW doesn't.
  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  encState = (digitalRead(PIN_ENC_CLK) << 1) | digitalRead(PIN_ENC_DT);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), encoderIsr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_DT), encoderIsr, CHANGE);

  // Calibrate the touch baseline (average of 16 samples)
  uint64_t sum = 0;
  for (int i = 0; i < 16; i++) {
    sum += touchRead(PIN_TOUCH);
    delay(5);
  }
  touchBase = sum / 16;
}

uint32_t touchRaw() { return touchLast; }
uint32_t touchBaseline() { return touchBase; }

InputEvents inputPoll() {
  InputEvents ev;

  // Encoder: take steps that add up to whole detents, keep the remainder
  portENTER_CRITICAL(&encMux);
  int32_t detents = encSteps / ENC_STEPS_PER_DETENT;
  encSteps -= detents * ENC_STEPS_PER_DETENT;
  portEXIT_CRITICAL(&encMux);
  ev.turns = ENC_REVERSE ? -detents : detents;

  // Button (active LOW)
  bool pressed = digitalRead(PIN_ENC_SW) == LOW;
  uint32_t now = millis();
  if (pressed && !btnDown) {
    btnDown = true;
    longFired = false;
    btnDownMs = now;
  } else if (pressed && btnDown && !longFired && now - btnDownMs >= LONG_PRESS_MS) {
    longFired = true;
    ev.longPress = true;
  } else if (!pressed && btnDown) {
    btnDown = false;
    if (!longFired) ev.click = true;
  }

  // Touch: hysteresis + delayed release. A finger on bare metal often loses contact
  // briefly; only counts as released after staying below threshold for TOUCH_RELEASE_MS.
  touchLast = touchRead(PIN_TOUCH);
  bool high = touchLast > touchBase * TOUCH_ON_RATIO;
  bool low = touchLast < touchBase * TOUCH_OFF_RATIO;
  if (!touched) {
    if (high) {
      touched = true;
      ev.touchStart = true;
      touchLowSinceMs = 0;
    } else if (low) {
      // Baseline slowly tracks temperature/humidity while untouched
      touchBase = (touchBase * (TOUCH_BASE_TRACK - 1) + touchLast) / TOUCH_BASE_TRACK;
    }
  } else if (low) {
    if (touchLowSinceMs == 0) touchLowSinceMs = now ? now : 1;
    else if (now - touchLowSinceMs >= TOUCH_RELEASE_MS) touched = false;
  } else {
    touchLowSinceMs = 0;
  }
  ev.touching = touched;

  return ev;
}
