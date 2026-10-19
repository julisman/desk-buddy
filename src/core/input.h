// input.h — KY-040 rotary encoder + button (push knob).
#pragma once

#include <stdint.h>

struct InputEvents {
  int turns = 0;          // turn clicks since last poll (+ right, - left)
  bool click = false;     // short press
  bool longPress = false; // held >= LONG_PRESS_MS (once, while still held)
  bool touching = false;  // touch pad is being touched
  bool touchStart = false;// touch started this frame

  bool any() const { return turns != 0 || click || longPress || touchStart; }
};

void inputInit();
InputEvents inputPoll();  // call every frame
uint32_t touchRaw();      // last raw touch value (for calibrating via the log)
uint32_t touchBaseline();
