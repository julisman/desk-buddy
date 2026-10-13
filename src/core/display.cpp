// display.cpp — lihat display.h
#include "display.h"

#include <Arduino.h>

#include "board.h"
#include "log.h"

// Static lokal: aman dipakai dari initializer global di file lain
static TFT_eSPI &tft() {
  static TFT_eSPI t;
  return t;
}

TFT_eSprite &displayCanvas() {
  static TFT_eSprite s(&tft());
  return s;
}

static float brightness = 0;

void displayInit() {
  ledcSetup(BL_PWM_CH, BL_PWM_FREQ, 8);
  ledcAttachPin(PIN_TFT_BL, BL_PWM_CH);
  ledcWrite(BL_PWM_CH, 0);

  tft().init();
  tft().setRotation(SCREEN_ROTATION);
  // Tes warna singkat: kalau layar tidak berkedip merah-hijau-biru, data SPI tidak sampai
  // (cek kabel SDA/SCL/CS/DC/RES dan solderan header LCD).
  const uint16_t testColors[] = {TFT_RED, TFT_GREEN, TFT_BLUE};
  ledcWrite(BL_PWM_CH, BL_BRIGHT);
  for (uint16_t c : testColors) {
    tft().fillScreen(c);
    delay(300);
  }
  tft().fillScreen(COLOR_BG);
  brightness = BL_BRIGHT;
  LOG("[boot] display test merah-hijau-biru selesai\n");

  TFT_eSprite &canvas = displayCanvas();
  canvas.setColorDepth(16);
  if (!canvas.createSprite(SCREEN_W, SCREEN_H)) {
    LOG("[boot] ERROR: gagal alokasi frame buffer (PSRAM?)\n");
  }
}

void displayPush() { displayCanvas().pushSprite(0, 0); }

void displayFadeTo(uint8_t level) {
  brightness += (level - brightness) * 0.08f;  // fade halus
  ledcWrite(BL_PWM_CH, (uint32_t)brightness);
}
