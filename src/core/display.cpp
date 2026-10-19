// display.cpp — see display.h
#include "display.h"

#include <Arduino.h>

#include "board.h"
#include "log.h"

// Local static: safe to use from global initializers in other files
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
  // Quick color test: if the screen doesn't flash red-green-blue, SPI data isn't arriving
  // (check the SDA/SCL/CS/DC/RES wires and the LCD header solder joints).
  const uint16_t testColors[] = {TFT_RED, TFT_GREEN, TFT_BLUE};
  ledcWrite(BL_PWM_CH, BL_BRIGHT);
  for (uint16_t c : testColors) {
    tft().fillScreen(c);
    delay(300);
  }
  tft().fillScreen(COLOR_BG);
  brightness = BL_BRIGHT;
  LOG("[boot] display red-green-blue test done\n");

  TFT_eSprite &canvas = displayCanvas();
  canvas.setColorDepth(16);
  if (!canvas.createSprite(SCREEN_W, SCREEN_H)) {
    LOG("[boot] ERROR: failed to allocate frame buffer (PSRAM?)\n");
  }
}

void displayPush() { displayCanvas().pushSprite(0, 0); }

void displayFadeTo(uint8_t level) {
  brightness += (level - brightness) * 0.08f;  // smooth fade
  ledcWrite(BL_PWM_CH, (uint32_t)brightness);
}
