// net.h — WiFi + NTP + cuaca (Open-Meteo), jalan di task terpisah (core 0)
// supaya request HTTP tidak bikin animasi patah-patah.
//
// Belum ada WiFi tersimpan (atau WiFi tersimpan gagal connect): buka hotspot
// "DeskBuddy-XXXX" dengan halaman setup di http://192.168.4.1 (WiFi, zona waktu, lokasi).
#pragma once

#include <stdint.h>

enum class NetStatus { Setup, Connecting, SyncingTime, Ready };

struct Weather {
  bool valid = false;
  float tempC = 0;
  int code = 0;  // WMO weather code
};

void netInit();
NetStatus netStatus();
bool netTimeValid();
inline bool netConnected() { return netStatus() == NetStatus::SyncingTime || netStatus() == NetStatus::Ready; }
Weather netWeather();
const char *weatherText(int code);

// Hotspot setup sedang terbuka? (bisa bareng Connecting kalau WiFi tersimpan gagal)
bool netSetupActive();
const char *netSetupApName();  // "DeskBuddy-1A2B"

// Hapus WiFi tersimpan (tahan knob saat dinyalakan). Berlaku setelah restart.
void netForgetWifi();
