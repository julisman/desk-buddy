// board.h — hardware shared by all apps: pins, display, input, buzzer, WiFi.
// Display (TFT) pins live in platformio.ini. App-specific constants live in the app's folder.
#pragma once

// ---------- WiFi / lokasi ----------
// Normally WiFi, time zone and weather location are set via the setup page (hotspot
// "DeskBuddy-XXXX", see net.cpp) and stored in flash. secrets.h (not committed)
// is optional for developers: it provides defaults if setup was never done.
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
#define SETUP_AP_PREFIX       "DeskBuddy-"  // + 4 hex digits from the MAC
#define SETUP_FALLBACK_MS     (45UL * 1000)  // saved WiFi fails to connect for 45 s -> open setup hotspot
#define SETUP_RETRY_MS        (3UL * 60 * 1000)  // fallback hotspot with no visitors for 3 min -> restart, try again
#define NTP_SERVER_1          "pool.ntp.org"
#define NTP_SERVER_2          "time.google.com"
#define WEATHER_REFRESH_MS    (15UL * 60 * 1000)
#define WEATHER_RETRY_MS      (60UL * 1000)

// ---------- Pin (ESP32-S3 DevKitC N16R8) ----------
// Avoid: 0/3/45/46 (strapping), 19/20 (USB), 26-37 (flash + octal PSRAM),
// 43/44 (UART0), 48 (onboard RGB LED).
// LCD BLK,CS,DC,RES,SDA,SCL = GPIO 9,10,11,12,13,14. VCC to 3V3 (not 5V, see README).
#define PIN_TFT_BL       9     // BLK on the display module (PWM for dimming)
#define PIN_ENC_CLK      4
#define PIN_ENC_DT       5
#define PIN_ENC_SW       6
#define PIN_BUZZER       15
// Touch pad (S3 built-in touch sensor, GPIO 1-14). A wire / foil / copper tape is enough.
#define PIN_TOUCH        7

// ---------- Display ----------
#define SCREEN_W         240
#define SCREEN_H         280
#define SCREEN_ROTATION  0     // 0-3, rotate if the display is mounted sideways
#define FRAME_MS         33    // ~30 fps

// LEDC: channel n uses timer (n/2). Backlight & buzzer must use different timers,
// otherwise changing the buzzer tone also changes the backlight frequency.
#define BL_PWM_CH        0
#define BL_PWM_FREQ      5000
#define BL_BRIGHT        255
#define BL_DIM           12    // brightness while asleep

// ---------- Encoder (KY-040) ----------
#define ENC_STEPS_PER_DETENT 4   // if 1 knob click reads as 2 steps, change to 2
#define ENC_REVERSE      0       // set to 1 if the turn direction is reversed
#define LONG_PRESS_MS    600

// ---------- Touch ("petting") ----------
// Baseline is measured at boot (don't touch it while booting). Touched when the value rises
// above baseline x TOUCH_ON_RATIO, released when it drops below baseline x TOUCH_OFF_RATIO.
#define TOUCH_ON_RATIO   1.15f
#define TOUCH_OFF_RATIO  1.08f
#define TOUCH_RELEASE_MS 200   // must stay below threshold this long to count as released
#define TOUCH_BASE_TRACK 64    // larger = baseline follows drift more slowly

// ---------- Buzzer ----------
// Passive buzzer can play tones; active buzzer only plays one tone (on/off).
#define BUZZER_ENABLED   1      // 0 = fully muted (debug reset loops while the buzzer sounds)
#define BUZZER_DUTY      16     // 0..255 (128 = 50%, loudest). KY-006 without a resistor causes resets at 128
#define BUZZER_PASSIVE   1
#define BUZZER_ACTIVE_LOW 0      // some modules sound when the pin is LOW
#define BUZZER_PWM_CH    2

// ---------- Colors (RGB565) ----------
#define COLOR_BG         0x0000
#define COLOR_EYE        0xF800  // red
#define COLOR_BLUSH      0xFD20  // orange (contrasts with red eyes)
#define COLOR_HEART      0xF8B2  // pink
#define COLOR_TEXT       0xFFFF
#define COLOR_MUTED      0x7BEF
