// log.h — logging ke serial monitor.
// Kenapa bukan Serial.printf: di core 2.0.17, Serial (HWCDC) di port USB native
// kadang salah deteksi "tidak tersambung" (deteksi SOF gagal, terlihat di macOS)
// dan semua output dibuang. log_printf menulis langsung ke konsol, selalu keluar.
#pragma once

#include <esp32-hal-log.h>

#define LOG(...) log_printf(__VA_ARGS__)
