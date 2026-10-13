// board.h — hardware bersama semua app: pin, display, input, buzzer, WiFi.
// Pin display (TFT) ada di platformio.ini. Konstanta khusus app ada di folder app-nya.
#pragma once

// ---------- WiFi / lokasi ----------
// Normalnya WiFi, zona waktu, dan lokasi cuaca diisi lewat halaman setup (hotspot
// "DeskBuddy-XXXX", lihat net.cpp) dan disimpan di flash. secrets.h (tidak di-commit)
// opsional untuk developer: jadi nilai default kalau belum pernah setup.
#if __has_include("secrets.h") && !defined(NO_SECRETS)
#include "secrets.h"
#endif
#ifndef WIFI_SSID
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#endif
#ifndef WEATHER_LAT
#define WEATHER_LAT  -6.2088    // default: Jakarta
#define WEATHER_LON  106.8456
#endif

#define NET_HOSTNAME          "deskbuddy"  // http://deskbuddy.local
#define TZ_INFO               "WIB-7"   // default POSIX TZ. WITA: "WITA-8", WIT: "WIT-9"
#define SETUP_AP_PREFIX       "DeskBuddy-"  // + 4 digit hex dari MAC
#define SETUP_FALLBACK_MS     (45UL * 1000)  // WiFi tersimpan gagal connect 45 detik -> buka hotspot setup
#define SETUP_RETRY_MS        (3UL * 60 * 1000)  // hotspot fallback tanpa pengunjung 3 menit -> restart, coba lagi
#define NTP_SERVER_1          "pool.ntp.org"
#define NTP_SERVER_2          "time.google.com"
#define WEATHER_REFRESH_MS    (15UL * 60 * 1000)
#define WEATHER_RETRY_MS      (60UL * 1000)

// ---------- Pin (ESP32-S3 DevKitC N16R8) ----------
// Hindari: 0/3/45/46 (strapping), 19/20 (USB), 26-37 (flash + octal PSRAM),
// 43/44 (UART0), 48 (LED RGB onboard).
// LCD BLK,CS,DC,RES,SDA,SCL = GPIO 9,10,11,12,13,14. VCC ke 3V3 (bukan 5V, lihat README).
#define PIN_TFT_BL       9     // BLK di modul display (PWM untuk dimming)
#define PIN_ENC_CLK      4
#define PIN_ENC_DT       5
#define PIN_ENC_SW       6
#define PIN_BUZZER       15
// Touch pad (sensor sentuh bawaan S3, GPIO 1-14). Cukup kabel / foil / copper tape.
#define PIN_TOUCH        7

// ---------- Display ----------
#define SCREEN_W         240
#define SCREEN_H         280
#define SCREEN_ROTATION  0     // 0-3, putar kalau display dipasang miring
#define FRAME_MS         33    // ~30 fps

// LEDC: channel n pakai timer (n/2). Backlight & buzzer harus beda timer,
// kalau tidak, ganti nada buzzer ikut mengubah frekuensi backlight.
#define BL_PWM_CH        0
#define BL_PWM_FREQ      5000
#define BL_BRIGHT        255
#define BL_DIM           12    // brightness saat tidur

// ---------- Encoder (KY-040) ----------
#define ENC_STEPS_PER_DETENT 4   // kalau 1 klik knob kebaca 2 langkah, ganti ke 2
#define ENC_REVERSE      0       // set 1 kalau arah putar kebalik
#define LONG_PRESS_MS    600

// ---------- Touch ("elus-elus") ----------
// Baseline diukur saat boot (jangan disentuh waktu boot). Disentuh kalau nilai naik
// di atas baseline x TOUCH_ON_RATIO, lepas kalau turun di bawah baseline x TOUCH_OFF_RATIO.
#define TOUCH_ON_RATIO   1.15f
#define TOUCH_OFF_RATIO  1.08f
#define TOUCH_RELEASE_MS 200   // harus di bawah ambang selama ini baru dianggap lepas
#define TOUCH_BASE_TRACK 64    // makin besar, baseline makin lambat mengikuti drift

// ---------- Buzzer ----------
// Passive buzzer bisa main nada; active buzzer cuma bunyi satu nada (on/off).
#define BUZZER_ENABLED   1      // 0 = bisu total (debug reset loop saat buzzer bunyi)
#define BUZZER_DUTY      16     // 0..255 (128 = 50%, paling keras). KY-006 tanpa resistor bikin reset di 128
#define BUZZER_PASSIVE   1
#define BUZZER_ACTIVE_LOW 0      // beberapa modul bunyi saat pin LOW
#define BUZZER_PWM_CH    2

// ---------- Warna (RGB565) ----------
#define COLOR_BG         0x0000
#define COLOR_EYE        0xF800  // merah
#define COLOR_BLUSH      0xFD20  // oranye (kontras dengan mata merah)
#define COLOR_HEART      0xF8B2  // pink
#define COLOR_TEXT       0xFFFF
#define COLOR_MUTED      0x7BEF
