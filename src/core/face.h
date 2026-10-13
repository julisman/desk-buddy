// face.h — mata animasi prosedural (gaya robot: dua rounded-rect).
#pragma once

#include <TFT_eSPI.h>

enum class Mood { Neutral, Happy, Sleepy, Focused, Surprised };

void faceInit();
void faceSetMood(Mood mood);                    // aman dipanggil tiap frame
void faceLook(float x, float y, uint32_t holdMs); // x,y -1..1; tahan arah ini selama holdMs
void faceUpdate(uint32_t now);                  // animasi: blink, lirik, transisi mood

// Gambar wajah di sprite. scale 1.0 = ukuran penuh (sekitar 170x90 px).
void faceDraw(TFT_eSprite &spr, int cx, int cy, float scale);
