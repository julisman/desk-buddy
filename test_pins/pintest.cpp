// pintest.cpp — tes kabel: GPIO 9-14 kedip bareng tiap 0.5 detik.
// Pindahkan ujung LCD dari tiap kabel data ke lubang BLK satu per satu:
// kalau backlight kedip, kabel + pin ESP32-nya benar.
// Flash: pio run -e pintest -t upload
#include <Arduino.h>
#include <esp32-hal-log.h>

static const int PINS[] = {9, 10, 11, 12, 13, 14};

void setup() {
  for (int p : PINS) pinMode(p, OUTPUT);
  log_printf("[pintest] GPIO 9-14 kedip\n");
}

void loop() {
  static bool on = false;
  on = !on;
  for (int p : PINS) digitalWrite(p, on);
  delay(500);
}
