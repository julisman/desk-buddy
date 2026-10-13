// Desk Buddy — main
//
// Mode (tahan knob untuk ganti):  FACE -> FOCUS -> CLOCK -> CLAUDE
//   FACE  : putar = elus (senang), tekan = boop (kaget)
//   FOCUS : putar = set menit, tekan = start/pause
//   CLOCK : jam + cuaca, tekan = boop
//   CLAUDE: status Claude Code dari laptop (context, limit sesi/minggu, working/butuh kamu),
//           putar = pindah sesi (kalau > 1), tekan = diamkan alarm "butuh kamu"
// Idle lama (lebih cepat di malam hari) -> tidur + layar redup. Sentuh knob -> bangun.
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <esp_system.h>
#include <time.h>

#include "app_config.h"
#include "buzzer.h"
#include "claude_link.h"
#include "display.h"
#include "face.h"
#include "focus.h"
#include "input.h"
#include "log.h"
#include "net.h"

enum class Mode { Face, Focus, Clock, Claude, Count };
static const char *MODE_NAMES[] = {"FACE", "FOCUS", "CLOCK", "CLAUDE"};

static TFT_eSprite &canvas = displayCanvas();

static Mode mode = Mode::Face;
static bool asleep = false;
static uint32_t lastInputMs = 0;
static uint32_t lastFrameMs = 0;
static uint32_t toastUntil = 0;

// Alarm "Claude butuh kamu": jumlah bunyi yang sudah dimainkan untuk permintaan sekarang
static uint8_t claudeAlertCount = 0;
static uint32_t claudeAlertNextMs = 0;

static Mood tempMood = Mood::Neutral;
static uint32_t tempMoodUntil = 0;

// Hati yang melayang saat dielus (touch pad)
struct Heart {
  int x;
  uint32_t born;
  bool alive;
};
static Heart hearts[8];
static uint32_t lastHeartMs = 0;
static const uint32_t HEART_LIFE_MS = 1500;

// Layout FOCUS/CLOCK didesain untuk tinggi 240; OY menengahkan di layar lebih tinggi (280)
static const int OY = (SCREEN_H - 240) / 2;

// ---------- Helpers ----------

static void setTempMood(Mood m, uint32_t ms) {
  tempMood = m;
  tempMoodUntil = millis() + ms;
}

static bool isNight() {
  if (!netTimeValid()) return false;
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  return t.tm_hour >= NIGHT_START_HOUR || t.tm_hour < NIGHT_END_HOUR;
}

static void setMode(Mode m) {
  mode = m;
  toastUntil = millis() + 1000;
  LOG("[mode] %s\n", MODE_NAMES[(int)m]);
}

static void wake() {
  asleep = false;
  setTempMood(Mood::Surprised, 600);
  buzzerPlay(SND_WAKE);
  LOG("[sleep] bangun\n");
}

static void goToSleep() {
  asleep = true;
  mode = Mode::Face;
  LOG("[sleep] tidur\n");
}

// ---------- Input per mode ----------

static void handleInput(const InputEvents &ev) {
  if (ev.longPress) {
    setMode((Mode)(((int)mode + 1) % (int)Mode::Count));
    buzzerPlay(SND_MODE);
    return;
  }

  switch (mode) {
    case Mode::Face:
    case Mode::Clock:
    case Mode::Claude:
      if (ev.click && mode == Mode::Claude && claudeAlertCount > 0 && claudeAlertCount < CLAUDE_ALERT_MAX) {
        claudeAlertCount = CLAUDE_ALERT_MAX;  // diamkan alarm, status tetap tampil
        buzzerPlay(SND_MODE);
        break;
      }
      if (ev.turns && mode == Mode::Claude && claudeState().count > 1) {
        claudeSelect(ev.turns > 0 ? 1 : -1);
        buzzerPlay(SND_TICK);
        break;
      }
      if (ev.turns) {
        // "Dielus": senang + lirik ke arah putaran
        setTempMood(Mood::Happy, 1500);
        faceLook(ev.turns > 0 ? 0.8f : -0.8f, -0.2f, 900);
        buzzerPlay(SND_TICK);
      }
      if (ev.click) {
        setTempMood(Mood::Surprised, 700);
        buzzerPlay(SND_BOOP);
      }
      break;

    case Mode::Focus:
      if (ev.turns) {
        focusTurn(ev.turns);
        buzzerPlay(SND_TICK);
      }
      if (ev.click) {
        focusClick();
        buzzerPlay(focusRunning() ? SND_START : SND_PAUSE);
      }
      break;

    case Mode::Count: break;
  }
}

// ---------- Drawing ----------

static void drawZzz(uint32_t now) {
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(COLOR_MUTED);
  for (int i = 0; i < 3; i++) {
    // Tiap "z" melayang naik lalu muncul lagi dari bawah
    float t = fmodf(now / 2400.0f + i / 3.0f, 1.0f);
    int x = 170 + i * 14 + (int)(sinf(t * 6.28f) * 4);
    int y = OY + 80 - (int)(t * 50);
    canvas.drawString("z", x, y, i == 2 ? 4 : 2);
  }
}

static void spawnHeart(uint32_t now) {
  for (Heart &h : hearts) {
    if (h.alive) continue;
    h = {SCREEN_W / 2 + (int)random(-70, 71), now, true};
    return;
  }
}

static void drawHeart(int x, int y, int r, uint16_t color) {
  canvas.fillCircle(x - r, y, r, color);
  canvas.fillCircle(x + r, y, r, color);
  canvas.fillTriangle(x - 2 * r, y + r / 3, x + 2 * r, y + r / 3, x, y + 2 * r + r / 2, color);
}

static void drawHearts(uint32_t now) {
  for (Heart &h : hearts) {
    if (!h.alive) continue;
    uint32_t age = now - h.born;
    if (age >= HEART_LIFE_MS) {
      h.alive = false;
      continue;
    }
    float t = (float)age / HEART_LIFE_MS;
    int y = SCREEN_H / 2 - 60 - (int)(t * 70);
    int x = h.x + (int)(sinf(t * 9.0f) * 6);
    int r = (int)(7 * (1 - t * 0.6f));
    drawHeart(x, y, r, COLOR_HEART);
  }
}

static void drawFaceMode(uint32_t now) {
  faceDraw(canvas, SCREEN_W / 2, SCREEN_H / 2, 1.0f);
  if (asleep) drawZzz(now);
  drawHearts(now);
}

// Arc TFT_eSPI: 0 derajat = jam 6, searah jarum jam. Mulai dari jam 12 = 180.
static void drawRing(float fraction, uint16_t color) {
  const int cx = SCREEN_W / 2, cy = SCREEN_H / 2, r = 117, ir = 109;
  canvas.drawSmoothArc(cx, cy, r, ir, 0, 360, COLOR_TRACK, COLOR_BG, false);
  if (fraction <= 0.002f) return;
  uint32_t end = 180 + (uint32_t)(fraction * 360);
  if (end <= 360) {
    canvas.drawSmoothArc(cx, cy, r, ir, 180, end, color, COLOR_BG, true);
  } else {
    canvas.drawSmoothArc(cx, cy, r, ir, 180, 360, color, COLOR_BG, false);
    canvas.drawSmoothArc(cx, cy, r, ir, 0, end - 360, color, COLOR_BG, true);
  }
}

static void drawFocusMode() {
  bool work = focusPhase() == FocusPhase::Work;
  uint16_t color = work ? COLOR_FOCUS : COLOR_BREAK;
  drawRing(focusFraction(), color);

  faceDraw(canvas, SCREEN_W / 2, OY + 72, 0.5f);

  uint32_t secs = (focusRemainingMs() + 999) / 1000;
  char buf[8];
  snprintf(buf, sizeof(buf), "%02lu:%02lu", secs / 60, secs % 60);
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(COLOR_TEXT);
  canvas.drawString(buf, SCREEN_W / 2, OY + 135, 7);

  const char *label;
  switch (focusState()) {
    case FocusState::Idle: label = work ? "FOCUS - press to start" : "BREAK - press to start"; break;
    case FocusState::Paused: label = "PAUSED"; break;
    default: label = work ? "FOCUS" : "BREAK"; break;
  }
  canvas.setTextColor(color);
  canvas.drawString(label, SCREEN_W / 2, OY + 180, 2);

  // Satu titik per sesi fokus yang sudah selesai (maks 8 ditampilkan)
  int n = min(focusCompleted(), 8);
  int x0 = SCREEN_W / 2 - (n - 1) * 7;
  for (int i = 0; i < n; i++) canvas.fillSmoothCircle(x0 + i * 14, OY + 204, 4, COLOR_FOCUS, COLOR_BG);
}

static void drawClockMode() {
  canvas.setTextDatum(MC_DATUM);
  const int cx = SCREEN_W / 2;

  if (!netTimeValid()) {
    canvas.setTextColor(COLOR_TEXT);
    canvas.drawString("--:--", cx, OY + 95, 7);
    const char *msg = "Setup WiFi (lihat bawah)";
    if (!netSetupActive() && netStatus() == NetStatus::Connecting) msg = "Connecting WiFi...";
    else if (netStatus() == NetStatus::SyncingTime) msg = "Syncing time...";
    canvas.setTextColor(COLOR_MUTED);
    canvas.drawString(msg, cx, OY + 150, 2);
    faceDraw(canvas, cx, OY + 200, 0.3f);
    return;
  }

  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  char buf[24];

  // Titik dua kedip tiap detik
  snprintf(buf, sizeof(buf), t.tm_sec % 2 ? "%02d:%02d" : "%02d.%02d", t.tm_hour, t.tm_min);
  canvas.setTextColor(COLOR_TEXT);
  canvas.drawString(buf, cx, OY + 78, 7);

  strftime(buf, sizeof(buf), "%a, %d %b", &t);
  canvas.setTextColor(COLOR_MUTED);
  canvas.drawString(buf, cx, OY + 124, 4);

  Weather w = netWeather();
  if (w.valid) {
    snprintf(buf, sizeof(buf), "%d", (int)roundf(w.tempC));
    canvas.setTextColor(COLOR_TEXT);
    canvas.setTextDatum(MR_DATUM);
    canvas.drawString(buf, cx + 2, OY + 160, 4);
    canvas.drawCircle(cx + 7, OY + 152, 3, COLOR_TEXT);  // simbol derajat (font tidak punya)
    canvas.setTextDatum(ML_DATUM);
    canvas.drawString("C", cx + 12, OY + 160, 4);
    canvas.setTextDatum(MC_DATUM);
    canvas.setTextColor(COLOR_MUTED);
    canvas.drawString(weatherText(w.code), cx, OY + 186, 2);
  }

  faceDraw(canvas, cx, OY + 214, 0.25f);
}

// ---------- CLAUDE mode ----------

static uint16_t pctColor(float pct) {
  return pct >= 90 ? COLOR_BAD : pct >= 70 ? COLOR_WARN : COLOR_OK;
}

// "resets 08:10" (sesi) atau "resets Wed 14 Oct 15:00" (mingguan); kosong kalau jam belum sync
static void formatReset(char *buf, size_t size, uint32_t epoch, bool withDate) {
  buf[0] = 0;
  if (!epoch || !netTimeValid()) return;
  time_t t = epoch;
  struct tm tm;
  localtime_r(&t, &tm);
  char when[24];
  strftime(when, sizeof(when), withDate ? "%a %d %b %H:%M" : "%H:%M", &tm);
  snprintf(buf, size, "resets %s", when);
}

// Label + persen di atas, bar di tengah, keterangan kecil di bawah. Tinggi total 38 px.
static void drawUsageBar(int y, const char *label, float pct, const char *sub) {
  const int x = 20, w = SCREEN_W - 40;
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(COLOR_MUTED);
  canvas.drawString(label, x, y, 2);

  char buf[8] = "--";
  if (pct >= 0) snprintf(buf, sizeof(buf), "%d%%", (int)roundf(pct));
  canvas.setTextDatum(TR_DATUM);
  canvas.setTextColor(pct >= 0 ? pctColor(pct) : COLOR_MUTED);
  canvas.drawString(buf, x + w, y, 2);

  canvas.fillSmoothRoundRect(x, y + 18, w, 8, 4, COLOR_TRACK, COLOR_BG);
  if (pct > 0) {
    int fill = max(8, (int)(w * min(pct, 100.0f) / 100));
    canvas.fillSmoothRoundRect(x, y + 18, fill, 8, 4, pctColor(pct), COLOR_BG);
  }
  if (sub && *sub) {
    canvas.setTextDatum(TL_DATUM);
    canvas.setTextColor(COLOR_MUTED);
    canvas.drawString(sub, x, y + 30, 1);
  }
}

static uint16_t activityColor(ClaudeActivity a) {
  switch (a) {
    case ClaudeActivity::Working: return COLOR_CLAUDE;
    case ClaudeActivity::NeedsYou: return COLOR_BAD;
    case ClaudeActivity::Done: return COLOR_OK;
    default: return COLOR_MUTED;
  }
}

static void drawClaudeMode(uint32_t now) {
  const int cx = SCREEN_W / 2;
  ClaudeState cs = claudeState();
  faceDraw(canvas, cx, 52, 0.4f);

  // Satu titik per sesi Claude Code, warna = status; yang tampil lebih besar + cincin
  if (cs.count > 1) {
    const int gap = 18, y = 14;
    int x0 = cx - (cs.count - 1) * gap / 2;
    for (int i = 0; i < cs.count; i++) {
      bool sel = i == cs.index;
      canvas.fillSmoothCircle(x0 + i * gap, y, sel ? 5 : 3, activityColor(cs.activities[i]), COLOR_BG);
      if (sel) canvas.drawCircle(x0 + i * gap, y, 7, COLOR_TEXT);
    }
  }

  const char *label = "NO SESSION";
  uint16_t color = COLOR_MUTED;
  char info[48];
  info[0] = 0;
  bool flashOn = (now / 400) % 2 == 0;

  if (!cs.valid) {
    label = "WAITING";
    snprintf(info, sizeof(info), netConnected() ? "for Claude Code -> %s.local" : "Connecting WiFi...",
             NET_HOSTNAME);
  } else {
    switch (cs.activity) {
      case ClaudeActivity::Working: {
        static const char *dots[] = {"WORKING", "WORKING.", "WORKING..", "WORKING..."};
        label = dots[(now / 400) % 4];
        color = COLOR_CLAUDE;
        break;
      }
      case ClaudeActivity::NeedsYou:
        label = "NEEDS YOU";
        color = flashOn ? COLOR_BAD : COLOR_TEXT;
        break;
      case ClaudeActivity::Done: label = "DONE"; color = COLOR_OK; break;
      case ClaudeActivity::Idle: label = "IDLE"; break;
      case ClaudeActivity::None: break;
    }
    if (cs.activity == ClaudeActivity::NeedsYou && cs.tool[0]) {
      snprintf(info, sizeof(info), "%s: approve %s", cs.project, cs.tool);
    } else {
      uint32_t ageMin = (now - cs.updatedMs) / 60000;
      if (ageMin >= 5) snprintf(info, sizeof(info), "%s | %s | %lum ago", cs.project, cs.model, (unsigned long)ageMin);
      else snprintf(info, sizeof(info), "%s | %s", cs.project, cs.model);
    }
  }

  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(color);
  canvas.drawString(label, cx, 104, 4);
  canvas.setTextColor(COLOR_MUTED);
  canvas.drawString(info, cx, 132, 2);

  char sub[40] = "";
  if (cs.mode[0]) snprintf(sub, sizeof(sub), "%s mode", cs.mode);
  drawUsageBar(152, "Context", cs.ctxPct, sub);
  formatReset(sub, sizeof(sub), cs.sessionResets, false);
  drawUsageBar(192, "Session", cs.sessionPct, sub);
  formatReset(sub, sizeof(sub), cs.weekResets, true);
  drawUsageBar(232, "Week", cs.weekPct, sub);

  // Bingkai berkedip saat Claude menunggu izin
  if (cs.activity == ClaudeActivity::NeedsYou && flashOn) {
    for (int i = 0; i < 3; i++) canvas.drawRoundRect(i, i, SCREEN_W - 2 * i, SCREEN_H - 2 * i, 18, COLOR_BAD);
  }
}

// Banner bawah selama hotspot setup terbuka (semua mode)
static void drawSetupBanner() {
  if (!netSetupActive()) return;
  canvas.fillRect(0, SCREEN_H - 58, SCREEN_W, 58, COLOR_TRACK);
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(COLOR_MUTED);
  canvas.drawString("Setup WiFi: sambung HP ke", SCREEN_W / 2, SCREEN_H - 46, 2);
  canvas.setTextColor(COLOR_TEXT);
  canvas.drawString(netSetupApName(), SCREEN_W / 2, SCREEN_H - 28, 2);
  canvas.setTextColor(COLOR_MUTED);
  canvas.drawString("lalu buka 192.168.4.1", SCREEN_W / 2, SCREEN_H - 11, 1);
}

static void drawToast(uint32_t now) {
  if ((int32_t)(toastUntil - now) <= 0) return;
  const char *name = MODE_NAMES[(int)mode];
  canvas.fillSmoothRoundRect(80, 6, 80, 24, 12, COLOR_TRACK, COLOR_BG);
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(COLOR_TEXT);
  canvas.drawString(name, SCREEN_W / 2, 18, 2);
}

static void render(uint32_t now) {
  canvas.fillSprite(COLOR_BG);
  switch (mode) {
    case Mode::Face: drawFaceMode(now); break;
    case Mode::Focus: drawFocusMode(); break;
    case Mode::Clock: drawClockMode(); break;
    case Mode::Claude: drawClaudeMode(now); break;
    case Mode::Count: break;
  }
  drawSetupBanner();
  drawToast(now);
  displayPush();
}

// Statistik touch sejak boot (supaya hasil tes tetap kelihatan di log berikutnya)
static uint32_t touchCount = 0, touchLongestMs = 0, touchStartMs = 0;

// Log status berkala: bukti board hidup + fps render aktual
static void logStatus(uint32_t now) {
  static uint32_t frames = 0, windowStart = 0;
  frames++;
  if (now - windowStart < 5000) return;
  LOG("[status] mode=%s%s fps=%.1f heap=%uKB psram=%uKB touch=%u/%u sentuhan=%u terlama=%ums\n",
      MODE_NAMES[(int)mode], asleep ? " (tidur)" : "", frames * 1000.0f / (now - windowStart),
      ESP.getFreeHeap() / 1024, ESP.getFreePsram() / 1024, touchRaw(), touchBaseline(),
      touchCount, touchLongestMs);
  frames = 0;
  windowStart = now;
}

// ---------- Setup / loop ----------

void setup() {
  Serial.begin(115200);
  delay(1500);  // USB native re-enumerate setelah reset; kasih waktu monitor connect
  LOG("\n=== Desk Buddy v%s ===\n", APP_VERSION);
  LOG("[boot] PSRAM %u KB, flash %u MB\n", ESP.getPsramSize() / 1024, ESP.getFlashChipSize() >> 20);

  displayInit();

  inputInit();
  buzzerInit();

  // Tahan knob saat dinyalakan = lupakan WiFi -> hotspot setup terbuka lagi
  if (digitalRead(PIN_ENC_SW) == LOW) {
    delay(1500);
    if (digitalRead(PIN_ENC_SW) == LOW) {
      netForgetWifi();
      TFT_eSprite &c = displayCanvas();
      c.fillSprite(COLOR_BG);
      c.setTextDatum(MC_DATUM);
      c.setTextColor(COLOR_TEXT);
      c.drawString("WiFi direset", SCREEN_W / 2, SCREEN_H / 2, 4);
      displayPush();
      buzzerPlay(SND_MODE);
      while (digitalRead(PIN_ENC_SW) == LOW) buzzerUpdate();  // tunggu knob dilepas
    }
  }
  faceInit();
  focusInit();
  netInit();

  buzzerPlay(SND_BOOT);
  setTempMood(Mood::Happy, 1500);
  lastInputMs = millis();
  // Alasan reset terakhir: 1=power-on 3=software 4=panic 5..7=watchdog 9=brownout
  LOG("[boot] ready (reset reason %d)\n", (int)esp_reset_reason());
}

void loop() {
  buzzerUpdate();
  uint32_t now = millis();
  if (now - lastFrameMs < FRAME_MS) {
    delay(1);
    return;
  }
  lastFrameMs = now;

  InputEvents ev = inputPoll();
  if (ev.any()) {
    LOG("[input] turns=%d%s%s%s\n", ev.turns, ev.click ? " click" : "", ev.longPress ? " long" : "",
        ev.touchStart ? " touch" : "");
    lastInputMs = now;
    if (asleep) wake();  // sentuhan pertama cuma membangunkan
    else handleInput(ev);
  }

  // Dielus: selama touch pad disentuh, Desk Buddy senang, melihat ke atas, keluar hati
  static bool wasTouching = false;
  if (ev.touching && !asleep) {
    lastInputMs = now;
    setTempMood(Mood::Happy, 700);
    faceLook(0, -0.5f, 700);
    if (ev.touchStart) buzzerPlay(SND_PURR);
    if (mode == Mode::Face && now - lastHeartMs >= HEART_INTERVAL_MS) {
      spawnHeart(now);
      lastHeartMs = now;
    }
  }
  if (ev.touchStart) {
    touchStartMs = now;
    touchCount++;
  }
  if (wasTouching && !ev.touching) {
    uint32_t held = now - touchStartMs;
    if (held > touchLongestMs) touchLongestMs = held;
    LOG("[touch] lepas setelah %ums\n", held);
  }
  wasTouching = ev.touching;

  FocusEvent fe = focusUpdate(now);
  if (fe != FocusEvent::None) {
    if (asleep) asleep = false;
    mode = Mode::Focus;
    lastInputMs = now;
    setTempMood(Mood::Happy, 4000);
    buzzerPlay(fe == FocusEvent::WorkDone ? SND_FOCUS_DONE : SND_BREAK_DONE);
  }

  // Claude Code: server nyala begitu WiFi connect, lalu reaksi ke event dari laptop
  if (!claudeLinkStarted() && netConnected()) claudeLinkStart();
  uint8_t claudeEvents = claudeTakeEvents();
  ClaudeState cs = claudeState();
  bool needsYou = cs.activity == ClaudeActivity::NeedsYou;
  bool claudeBusy = cs.activity == ClaudeActivity::Working && now - cs.updatedMs < CLAUDE_STALE_MS;
  if (claudeEvents & CLAUDE_EV_NEEDS_YOU) {
    asleep = false;  // bangun tanpa bunyi "wake", langsung alarm
    if (mode != Mode::Claude) setMode(Mode::Claude);
    lastInputMs = now;
    buzzerPlay(SND_ALERT);
    claudeAlertCount = 1;
    claudeAlertNextMs = now + CLAUDE_ALERT_REPEAT_MS;
  } else if (needsYou && claudeAlertCount > 0 && claudeAlertCount < CLAUDE_ALERT_MAX &&
             (int32_t)(now - claudeAlertNextMs) >= 0) {
    buzzerPlay(SND_ALERT);
    claudeAlertCount++;
    claudeAlertNextMs = now + CLAUDE_ALERT_REPEAT_MS;
  }
  if (!needsYou) claudeAlertCount = 0;
  // Di mode CLAUDE, kabar dari Claude = aktivitas: bangun pelan (tanpa bunyi) + reset timer tidur,
  // supaya status DONE tetap kelihatan
  if ((claudeEvents & CLAUDE_EV_ACTIVITY) && mode == Mode::Claude) {
    asleep = false;
    lastInputMs = now;
  }
  if ((claudeEvents & CLAUDE_EV_DONE) && !asleep) {
    setTempMood(Mood::Happy, 2500);
    buzzerPlay(SND_CHIRP);
  }

  uint32_t idleLimit = isNight() ? NIGHT_IDLE_SLEEP_MS : IDLE_SLEEP_MS;
  bool stayAwake = focusRunning() || needsYou || (mode == Mode::Claude && claudeBusy);
  if (!asleep && !stayAwake && now - lastInputMs > idleLimit) goToSleep();

  Mood m = asleep ? Mood::Sleepy : (focusRunning() || claudeBusy) ? Mood::Focused : Mood::Neutral;
  if (needsYou && !asleep) m = Mood::Surprised;
  if ((int32_t)(tempMoodUntil - now) > 0) m = tempMood;
  faceSetMood(m);
  faceUpdate(now);

  displayFadeTo(asleep ? BL_DIM : BL_BRIGHT);
  render(now);
  logStatus(now);
}
