// pintest.cpp — wiring test: GPIO 9-14 blink together every 0.5 s.
// Move the LCD end of each data wire to the BLK hole one at a time:
// if the backlight blinks, that wire + its ESP32 pin are correct.
// Flash: pio run -e pintest -t upload
#include <Arduino.h>
#include <esp32-hal-log.h>

static const int PINS[] = {9, 10, 11, 12, 13, 14};

void setup() {
  for (int p : PINS) pinMode(p, OUTPUT);
  log_printf("[pintest] GPIO 9-14 blinking\n");
}

void loop() {
  static bool on = false;
  on = !on;
  for (int p : PINS) digitalWrite(p, on);
  delay(500);
}
