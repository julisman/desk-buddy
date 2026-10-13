// buzzertest.cpp — tes buzzer di GPIO 15, berulang:
//   1) pin HIGH 1 detik  -> active buzzer bunyi "beep" (passive cuma "klik")
//   2) diam 1 detik
//   3) nada 1 kHz 1 detik -> passive buzzer bunyi "tuut"
//   4) diam 1 detik
// Flash: pio run -e buzzertest -t upload
#include <Arduino.h>
#include <esp32-hal-log.h>

static const int PIN_BUZZER = 15;
static const int CH = 2;

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
}

void loop() {
  log_printf("[buzzertest] 1) HIGH (active buzzer)\n");
  digitalWrite(PIN_BUZZER, HIGH);
  delay(1000);
  digitalWrite(PIN_BUZZER, LOW);
  delay(1000);

  log_printf("[buzzertest] 2) nada 1 kHz (passive buzzer)\n");
  ledcSetup(CH, 1000, 8);
  ledcAttachPin(PIN_BUZZER, CH);
  ledcWriteTone(CH, 1000);
  delay(1000);
  ledcWriteTone(CH, 0);
  ledcDetachPin(PIN_BUZZER);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  delay(1000);
}
