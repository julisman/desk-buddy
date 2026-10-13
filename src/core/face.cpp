// face.cpp — semua ekspresi dibuat dari satu bentuk mata (rounded rect) lalu
// "dipotong" dengan warna background:
//   lid   = kelopak atas turun (ngantuk)
//   slant = sudut dalam atas dipotong miring (fokus/serius)
//   happy = bagian bawah dipotong elips -> mata melengkung ^ ^
// Parameter bergerak halus (lerp) ke target tiap frame, jadi transisi mood mulus.
#include "face.h"

#include <Arduino.h>

#include "board.h"

struct FaceParams {
  float open = 1;   // tinggi mata relatif (blink pakai faktor terpisah)
  float lookX = 0;  // -1..1
  float lookY = 0;
  float lid = 0;    // 0..1 bagian atas yang tertutup
  float slant = 0;  // 0..1
  float happy = 0;  // 0..1
  float size = 1;   // skala mata (kaget = lebih besar)
};

static FaceParams cur, target;
static Mood mood = Mood::Neutral;

static const uint32_t BLINK_MS = 150;
static uint32_t blinkStartMs = 0;
static uint32_t nextBlinkMs = 0;
static bool doubleBlink = false;
static uint32_t nextLookMs = 0;
static uint32_t lookHoldUntil = 0;
static float blinkOpen = 1;

static const float BASE_EYE_W = 62;
static const float BASE_EYE_H = 78;
static const float BASE_GAP = 34;
static const float LOOK_RANGE_X = 22;
static const float LOOK_RANGE_Y = 16;

static float lerp(float a, float b, float k) { return a + (b - a) * k; }

void faceInit() {
  randomSeed(esp_random());
  nextBlinkMs = millis() + 1500;
  nextLookMs = millis() + 2000;
}

void faceSetMood(Mood m) {
  if (m == mood) return;
  mood = m;
  target.lid = target.slant = target.happy = 0;
  target.open = target.size = 1;
  switch (m) {
    case Mood::Neutral: break;
    case Mood::Happy: target.happy = 1; break;
    case Mood::Sleepy: target.lid = 0.72f; target.lookX = 0; target.lookY = 0.3f; break;
    case Mood::Focused: target.slant = 0.6f; target.lid = 0.12f; target.lookY = 0.25f; target.lookX = 0; break;
    case Mood::Surprised: target.size = 1.18f; target.open = 1.1f; target.lookX = target.lookY = 0; break;
  }
}

void faceLook(float x, float y, uint32_t holdMs) {
  target.lookX = constrain(x, -1.0f, 1.0f);
  target.lookY = constrain(y, -1.0f, 1.0f);
  lookHoldUntil = millis() + holdMs;
}

void faceUpdate(uint32_t now) {
  // Blink otomatis (tidak saat tidur — mata sudah hampir tertutup)
  if (mood != Mood::Sleepy && (int32_t)(now - nextBlinkMs) >= 0) {
    blinkStartMs = now;
    if (doubleBlink) {
      doubleBlink = false;
      nextBlinkMs = now + random(2500, 6000);
    } else {
      doubleBlink = random(100) < 20;  // 20% kedip dua kali
      nextBlinkMs = now + (doubleBlink ? 260 : random(2500, 6000));
    }
  }
  blinkOpen = 1;
  if (blinkStartMs && now - blinkStartMs < BLINK_MS) {
    float t = (float)(now - blinkStartMs) / BLINK_MS;
    blinkOpen = 1 - sinf(t * PI);  // tutup lalu buka
  }

  // Lirik ke sana-sini saat santai
  if (mood == Mood::Neutral && (int32_t)(now - lookHoldUntil) >= 0 &&
      (int32_t)(now - nextLookMs) >= 0) {
    bool center = random(100) < 35;
    target.lookX = center ? 0 : random(-100, 101) / 100.0f;
    target.lookY = center ? 0 : random(-60, 61) / 100.0f;
    nextLookMs = now + random(1200, 4000);
  }

  // Tidur: kelopak naik-turun pelan seperti napas
  if (mood == Mood::Sleepy) target.lid = 0.74f + 0.06f * sinf(now * 0.0018f);

  const float k = 0.22f, kLook = 0.32f;
  cur.open = lerp(cur.open, target.open, k);
  cur.lid = lerp(cur.lid, target.lid, k);
  cur.slant = lerp(cur.slant, target.slant, k);
  cur.happy = lerp(cur.happy, target.happy, k);
  cur.size = lerp(cur.size, target.size, k);
  cur.lookX = lerp(cur.lookX, target.lookX, kLook);
  cur.lookY = lerp(cur.lookY, target.lookY, kLook);
}

static void drawEye(TFT_eSprite &spr, int side, int cx, int cy, float scale) {
  float w = BASE_EYE_W * scale * cur.size;
  float h = BASE_EYE_H * scale * cur.size * cur.open * blinkOpen;
  if (h < 4 * scale) h = 4 * scale;
  float gap = BASE_GAP * scale;

  float ecx = cx + side * (gap / 2 + w / 2) + cur.lookX * LOOK_RANGE_X * scale;
  float ecy = cy + cur.lookY * LOOK_RANGE_Y * scale;
  // Mata yang searah lirikan sedikit lebih besar -> efek 3D sederhana
  float persp = 1 + 0.08f * cur.lookX * side;
  w *= persp;
  h *= persp;

  int x = (int)(ecx - w / 2), y = (int)(ecy - h / 2);
  int iw = (int)w, ih = (int)h;
  int r = (int)(min(w, h) * 0.28f);
  spr.fillSmoothRoundRect(x, y, iw, ih, r, COLOR_EYE, COLOR_BG);

  if (cur.lid > 0.01f) {
    spr.fillRect(x - 2, y - 2, iw + 4, (int)(h * cur.lid) + 2, COLOR_BG);
  }
  if (cur.slant > 0.01f) {
    int innerX = side < 0 ? x + iw + 2 : x - 2;
    int outerX = side < 0 ? x - 2 : x + iw + 2;
    spr.fillTriangle(outerX, y - 2, innerX, y - 2, innerX, y + (int)(h * 0.55f * cur.slant), COLOR_BG);
  }
  if (cur.happy > 0.01f) {
    spr.fillEllipse((int)ecx, y + (int)(h * 1.12f), (int)(w * 0.82f), (int)(h * 0.72f * cur.happy), COLOR_BG);
    if (cur.happy > 0.5f) {
      int bx = (int)(ecx + side * w * 0.35f);
      int by = (int)(ecy + h * 0.55f);
      int br = (int)(9 * scale);
      spr.fillSmoothCircle(bx, by, br, COLOR_BLUSH, COLOR_BG);
    }
  }
}

void faceDraw(TFT_eSprite &spr, int cx, int cy, float scale) {
  drawEye(spr, -1, cx, cy, scale);
  drawEye(spr, 1, cx, cy, scale);
}
