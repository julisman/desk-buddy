// net.cpp — WiFi STA, hotspot setup, sync jam via NTP, cuaca dari Open-Meteo (tanpa API key).
#include "net.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <time.h>

#include <mutex>

#include "board.h"
#include "log.h"

static std::mutex netMutex;
static Weather weather;
static volatile NetStatus status = NetStatus::Connecting;

static const time_t VALID_EPOCH = 1700000000;  // waktu sebelum ini = belum sync

// Setting yang diisi lewat halaman setup (NVS "net"), default dari board.h / secrets.h
struct NetConfig {
  String ssid, pass, tz;
  float lat, lon;
};
static NetConfig cfg;

static void loadConfig() {
  Preferences p;
  p.begin("net", false);  // read-write: buat namespace kalau belum ada (read-only = log error)
  cfg.ssid = p.getString("ssid", WIFI_SSID);
  cfg.pass = p.getString("pass", WIFI_PASSWORD);
  cfg.tz = p.getString("tz", TZ_INFO);
  cfg.lat = p.getFloat("lat", WEATHER_LAT);
  cfg.lon = p.getFloat("lon", WEATHER_LON);
  p.end();
}

void netForgetWifi() {
  Preferences p;
  p.begin("net", false);
  p.clear();
  p.end();
  LOG("[wifi] setting WiFi dihapus\n");
}

bool netTimeValid() { return time(nullptr) > VALID_EPOCH; }

// ---------- Hotspot setup ----------

static DNSServer dns;
static WebServer *portal = nullptr;
static char apName[24] = "";
static String scanOptions;  // <option> hasil scan WiFi, dibuat sebelum hotspot nyala

bool netSetupActive() { return portal != nullptr; }
const char *netSetupApName() { return apName; }

static String htmlEscape(const String &s) {
  String out;
  for (char c : s) {
    if (c == '&') out += "&amp;";
    else if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else if (c == '"') out += "&quot;";
    else out += c;
  }
  return out;
}

static void scanNetworks() {
  int n = WiFi.scanNetworks();
  scanOptions = "";
  for (int i = 0; i < n && i < 15; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.isEmpty() || scanOptions.indexOf("\"" + htmlEscape(ssid) + "\"") >= 0) continue;
    scanOptions += "<option value=\"" + htmlEscape(ssid) + "\">";
  }
  WiFi.scanDelete();
}

static const char PAGE_HEAD[] PROGMEM = R"(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Desk Buddy</title>
<style>body{font-family:system-ui,sans-serif;background:#111;color:#eee;margin:0;padding:24px;max-width:420px}
h1{font-size:22px;margin:0 0 4px}p{color:#aaa;margin:0 0 20px}label{display:block;margin:14px 0 6px;font-size:14px}
input,select{width:100%;box-sizing:border-box;padding:12px;border-radius:10px;border:1px solid #444;background:#222;color:#eee;font-size:16px}
button{margin-top:22px;width:100%;padding:14px;border:0;border-radius:10px;background:#f04;color:#fff;font-size:17px;font-weight:600}
.row{display:flex;gap:10px}.row>div{flex:1}small{color:#888}</style></head><body>)";

static void handleRoot() {
  String page = FPSTR(PAGE_HEAD);
  page += "<h1>Desk Buddy</h1><p>Sambungkan Desk Buddy ke WiFi rumah kamu.</p>"
          "<form method=post action=/save>"
          "<label for=s>Nama WiFi</label><input id=s name=ssid list=nets required value=\"" +
          htmlEscape(cfg.ssid) + "\"><datalist id=nets>" + scanOptions +
          "</datalist>"
          "<label for=p>Password</label><input id=p name=pass type=password>"
          "<label for=t>Zona waktu</label><select id=t name=tz>";
  const char *zones[][2] = {{"WIB-7", "WIB (UTC+7)"}, {"WITA-8", "WITA (UTC+8)"}, {"WIT-9", "WIT (UTC+9)"}};
  for (auto &z : zones) {
    page += String("<option value=") + z[0] + (cfg.tz == z[0] ? " selected>" : ">") + z[1] + "</option>";
  }
  page += "</select><label>Lokasi cuaca <small>(latitude, longitude; default Jakarta)</small></label>"
          "<div class=row><div><input name=lat inputmode=decimal value=\"" + String(cfg.lat, 4) +
          "\"></div><div><input name=lon inputmode=decimal value=\"" + String(cfg.lon, 4) +
          "\"></div></div><button>Simpan &amp; sambungkan</button></form></body></html>";
  portal->send(200, "text/html", page);
}

static void handleSave() {
  String ssid = portal->arg("ssid");
  ssid.trim();
  if (ssid.isEmpty()) {
    portal->send(400, "text/plain", "Nama WiFi wajib diisi");
    return;
  }
  Preferences p;
  p.begin("net", false);
  p.putString("ssid", ssid);
  p.putString("pass", portal->arg("pass"));
  String tz = portal->arg("tz");
  if (tz == "WIB-7" || tz == "WITA-8" || tz == "WIT-9") p.putString("tz", tz);
  float lat = portal->arg("lat").toFloat(), lon = portal->arg("lon").toFloat();
  if (lat >= -90 && lat <= 90 && lon >= -180 && lon <= 180 && (lat != 0 || lon != 0)) {
    p.putFloat("lat", lat);
    p.putFloat("lon", lon);
  }
  p.end();
  String page = FPSTR(PAGE_HEAD);
  page += "<h1>Tersimpan</h1><p>Desk Buddy restart dan menyambung ke <b>" + htmlEscape(ssid) +
          "</b>. Hotspot ini akan hilang; sambungkan HP kembali ke WiFi biasa.</p></body></html>";
  portal->send(200, "text/html", page);
  LOG("[setup] WiFi \"%s\" tersimpan, restart\n", ssid.c_str());
  delay(1500);
  ESP.restart();
}

static uint32_t portalStartMs = 0;

static void startPortal() {
  if (portal) return;
  WiFi.disconnect();     // berhenti mencoba WiFi tersimpan: retry STA lompat-lompat channel
  WiFi.mode(WIFI_STA);   // dan bikin HP yang tersambung ke hotspot putus
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(apName, sizeof(apName), "%s%02X%02X", SETUP_AP_PREFIX, mac[4], mac[5]);

  scanNetworks();  // scan dulu: selama AP nyala scan bisa mengganggu klien
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apName);
  portalStartMs = millis();
  dns.start(53, "*", WiFi.softAPIP());  // semua domain -> halaman setup (captive portal)

  portal = new WebServer(80);
  portal->on("/", HTTP_GET, handleRoot);
  portal->on("/save", HTTP_POST, handleSave);
  portal->onNotFound([] {  // HP cek "internet?" ke URL lain -> arahkan ke setup
    portal->sendHeader("Location", "http://192.168.4.1/");
    portal->send(302, "text/plain", "");
  });
  portal->begin();
  LOG("[setup] hotspot \"%s\" -> http://192.168.4.1\n", apName);
}

static void stopPortal() {
  if (!portal) return;
  portal->stop();
  delete portal;
  portal = nullptr;
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  LOG("[setup] hotspot ditutup\n");
}

// ---------- Cuaca ----------

static bool fetchWeather() {
  char url[200];
  snprintf(url, sizeof(url),
           "http://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,weather_code",
           (double)cfg.lat, (double)cfg.lon);

  HTTPClient http;
  http.setTimeout(8000);
  http.begin(url);
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    LOG("[weather] HTTP %d\n", code);
    http.end();
    return false;
  }
  String body = http.getString();
  http.end();

  JsonDocument doc;
  if (deserializeJson(doc, body)) {
    LOG("[weather] JSON tidak valid\n");
    return false;
  }
  Weather w;
  w.valid = true;
  w.tempC = doc["current"]["temperature_2m"] | 0.0f;
  w.code = doc["current"]["weather_code"] | 0;
  {
    std::lock_guard<std::mutex> lock(netMutex);
    weather = w;
  }
  LOG("[weather] %.1f C, %s\n", w.tempC, weatherText(w.code));
  return true;
}

// ---------- Task ----------

static void netTask(void *) {
  WiFi.setHostname(NET_HOSTNAME);
  configTzTime(cfg.tz.c_str(), NTP_SERVER_1, NTP_SERVER_2);

  if (cfg.ssid.isEmpty()) {
    status = NetStatus::Setup;
    startPortal();
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg.ssid.c_str(), cfg.pass.c_str());
    LOG("[wifi] connect ke \"%s\"\n", cfg.ssid.c_str());
  }

  bool wasConnected = false;
  bool everConnected = false;
  uint32_t startMs = millis();
  uint32_t nextWeatherMs = 0;

  for (;;) {
    if (portal) {
      dns.processNextRequest();
      portal->handleClient();
    }

    // Hotspot karena WiFi tersimpan gagal, tapi tidak ada yang buka: restart & coba lagi
    // (mungkin router cuma mati sebentar)
    if (portal && !cfg.ssid.isEmpty() && WiFi.softAPgetStationNum() == 0 &&
        millis() - portalStartMs > SETUP_RETRY_MS) {
      LOG("[setup] tidak ada yang setup, restart untuk coba WiFi lagi\n");
      ESP.restart();
    }

    bool connected = !portal && !cfg.ssid.isEmpty() && WiFi.status() == WL_CONNECTED;
    if (connected != wasConnected) {
      wasConnected = connected;
      if (connected) {
        everConnected = true;
        stopPortal();
        LOG("[wifi] connected, IP %s\n", WiFi.localIP().toString().c_str());
        static bool mdnsStarted = false;
        if (!mdnsStarted && MDNS.begin(NET_HOSTNAME)) {
          MDNS.addService("http", "tcp", 80);
          mdnsStarted = true;
          LOG("[mdns] http://%s.local\n", NET_HOSTNAME);
        }
      } else {
        LOG("[wifi] terputus, reconnect otomatis\n");
      }
    }

    // WiFi tersimpan tidak pernah berhasil (password salah / pindah rumah): buka setup
    if (!connected && !everConnected && !portal && !cfg.ssid.isEmpty() &&
        millis() - startMs > SETUP_FALLBACK_MS) {
      LOG("[wifi] gagal connect ke \"%s\", buka hotspot setup\n", cfg.ssid.c_str());
      startPortal();
    }

    if (cfg.ssid.isEmpty()) status = NetStatus::Setup;
    else if (!connected) status = NetStatus::Connecting;
    else if (!netTimeValid()) status = NetStatus::SyncingTime;
    else status = NetStatus::Ready;

    if (connected && (int32_t)(millis() - nextWeatherMs) >= 0) {
      nextWeatherMs = millis() + (fetchWeather() ? WEATHER_REFRESH_MS : WEATHER_RETRY_MS);
    }
    vTaskDelay(pdMS_TO_TICKS(portal ? 10 : 1000));
  }
}

void netInit() {
  loadConfig();
  // Core 0 (bareng WiFi stack); loop() + animasi jalan di core 1
  xTaskCreatePinnedToCore(netTask, "net", 8192, nullptr, 1, nullptr, 0);
}

NetStatus netStatus() { return status; }

Weather netWeather() {
  std::lock_guard<std::mutex> lock(netMutex);
  return weather;
}

const char *weatherText(int code) {
  if (code == 0) return "Clear";
  if (code == 1) return "Mostly clear";
  if (code == 2) return "Partly cloudy";
  if (code == 3) return "Cloudy";
  if (code == 45 || code == 48) return "Fog";
  if (code >= 51 && code <= 57) return "Drizzle";
  if (code >= 61 && code <= 67) return "Rain";
  if (code >= 71 && code <= 77) return "Snow";
  if (code >= 80 && code <= 82) return "Showers";
  if (code >= 95) return "Thunderstorm";
  return "?";
}
