// net.h — WiFi + NTP + weather (Open-Meteo), runs in a separate task (core 0)
// so HTTP requests don't make the animation stutter.
//
// No saved WiFi yet (or saved WiFi fails to connect): open hotspot
// "DeskBuddy-XXXX" with a setup page at http://192.168.4.1 (WiFi, time zone, location).
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

// Is the setup hotspot open? (can overlap with Connecting if saved WiFi fails)
bool netSetupActive();
const char *netSetupApName();  // "DeskBuddy-1A2B"

// Forget saved WiFi (hold the knob at power-on). Takes effect after restart.
void netForgetWifi();
