// face.h — procedurally animated eyes (robot style: two rounded rects).
#pragma once

#include <TFT_eSPI.h>

enum class Mood { Neutral, Happy, Sleepy, Focused, Surprised };

void faceInit();
void faceSetMood(Mood mood);                    // safe to call every frame
void faceLook(float x, float y, uint32_t holdMs); // x,y -1..1; hold this direction for holdMs
void faceUpdate(uint32_t now);                  // animation: blink, glances, mood transitions

// Draw the face into a sprite. scale 1.0 = full size (about 170x90 px).
void faceDraw(TFT_eSprite &spr, int cx, int cy, float scale);
