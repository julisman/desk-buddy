// buzzertest.cpp — buzzer test on GPIO 15, repeating:
//   1) pin HIGH for 1 s   -> active buzzer goes "beep" (passive only "clicks")
//   2) silent for 1 s
//   3) 1 kHz tone for 1 s -> passive buzzer goes "toot"
//   4) silent for 1 s
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

  log_printf("[buzzertest] 2) 1 kHz tone (passive buzzer)\n");
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
