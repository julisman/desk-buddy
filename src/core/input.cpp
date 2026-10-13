// input.cpp — encoder dibaca lewat interrupt (decoder quadrature),
// tombol di-poll tiap frame (33 ms sudah cukup sebagai debounce).
#include "input.h"

#include <Arduino.h>
#include <soc/gpio_struct.h>

#include "board.h"

static portMUX_TYPE encMux = portMUX_INITIALIZER_UNLOCKED;
static volatile int32_t encSteps = 0;
static volatile uint8_t encState = 0;

// Tabel transisi quadrature: index = (state lama << 2) | state baru.
// Transisi tidak valid (bounce) = 0.
static const int8_t QUAD_TABLE[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

static void IRAM_ATTR encoderIsr() {
  // Baca register langsung (IRAM-safe, aman walau cache flash sedang off)
  uint32_t in = GPIO.in;
  uint8_t ab = (((in >> PIN_ENC_CLK) & 1) << 1) | ((in >> PIN_ENC_DT) & 1);
  encState = ((encState << 2) | ab) & 0x0F;
  portENTER_CRITICAL_ISR(&encMux);
  encSteps += QUAD_TABLE[encState];
  portEXIT_CRITICAL_ISR(&encMux);
}

// Touch: di ESP32-S3 nilai touchRead() NAIK saat disentuh
static uint32_t touchBase = 0;
static uint32_t touchLast = 0;
static bool touched = false;
static uint32_t touchLowSinceMs = 0;  // sejak kapan nilai di bawah ambang lepas

static bool btnDown = false;
static bool longFired = false;
static uint32_t btnDownMs = 0;

void inputInit() {
  // KY-040 sudah punya pull-up 10k di CLK/DT (ke pin +), SW tidak.
  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  encState = (digitalRead(PIN_ENC_CLK) << 1) | digitalRead(PIN_ENC_DT);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), encoderIsr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_DT), encoderIsr, CHANGE);

  // Kalibrasi baseline touch (rata-rata 16 sampel)
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

  // Encoder: ambil langkah yang sudah genap 1 detent, sisanya disimpan
  portENTER_CRITICAL(&encMux);
  int32_t detents = encSteps / ENC_STEPS_PER_DETENT;
  encSteps -= detents * ENC_STEPS_PER_DETENT;
  portEXIT_CRITICAL(&encMux);
  ev.turns = ENC_REVERSE ? -detents : detents;

  // Tombol (aktif LOW)
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

  // Touch: hysteresis + tunda lepas. Jari di ujung logam sering putus kontak
  // sesaat; baru dianggap lepas kalau di bawah ambang selama TOUCH_RELEASE_MS.
  touchLast = touchRead(PIN_TOUCH);
  bool high = touchLast > touchBase * TOUCH_ON_RATIO;
  bool low = touchLast < touchBase * TOUCH_OFF_RATIO;
  if (!touched) {
    if (high) {
      touched = true;
      ev.touchStart = true;
      touchLowSinceMs = 0;
    } else if (low) {
      // Baseline ikut pelan-pelan mengikuti suhu/kelembapan saat tidak disentuh
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
