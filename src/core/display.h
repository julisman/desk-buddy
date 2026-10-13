// display.h — ST7789 + frame buffer penuh di PSRAM + backlight PWM.
#pragma once

#include <TFT_eSPI.h>

// Backlight, init TFT, tes warna merah-hijau-biru, alokasi frame buffer.
void displayInit();

// Frame buffer 240x280 (gambar di sini, lalu displayPush) -> tanpa flicker
TFT_eSprite &displayCanvas();
void displayPush();

// Panggil tiap frame: backlight fade halus ke level 0..255
void displayFadeTo(uint8_t level);
