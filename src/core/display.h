// display.h — ST7789 + full frame buffer in PSRAM + PWM backlight.
#pragma once

#include <TFT_eSPI.h>

// Backlight, TFT init, red-green-blue color test, frame buffer allocation.
void displayInit();

// 240x280 frame buffer (draw here, then displayPush) -> no flicker
TFT_eSprite &displayCanvas();
void displayPush();

// Call every frame: backlight fades smoothly to level 0..255
void displayFadeTo(uint8_t level);
