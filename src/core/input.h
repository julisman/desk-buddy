// input.h — KY-040 rotary encoder + tombol (push knob).
#pragma once

#include <stdint.h>

struct InputEvents {
  int turns = 0;          // jumlah klik putar sejak poll terakhir (+ kanan, - kiri)
  bool click = false;     // tekan singkat
  bool longPress = false; // tahan >= LONG_PRESS_MS (sekali, saat masih ditahan)
  bool touching = false;  // touch pad sedang disentuh
  bool touchStart = false;// baru mulai disentuh di frame ini

  bool any() const { return turns != 0 || click || longPress || touchStart; }
};

void inputInit();
InputEvents inputPoll();  // panggil tiap frame
uint32_t touchRaw();      // nilai mentah touch terakhir (untuk kalibrasi lewat log)
uint32_t touchBaseline();
