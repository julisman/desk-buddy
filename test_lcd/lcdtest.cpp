// lcdtest.cpp — tes display terpisah pakai Adafruit ST7789 (bukan TFT_eSPI).
// Layar ganti warna terus: merah, hijau, biru, putih (1 detik). Pin sama dengan firmware utama.
// Flash: pio run -e lcdtest -t upload
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <esp32-hal-log.h>

static const int PIN_BL = 9, PIN_CS = 10, PIN_DC = 11, PIN_RST = 12, PIN_MOSI = 13, PIN_SCLK = 14;

static SPIClass spi(FSPI);
static Adafruit_ST7789 tft(&spi, PIN_CS, PIN_DC, PIN_RST);

void setup() {
  delay(1500);
  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, HIGH);
  spi.begin(PIN_SCLK, -1, PIN_MOSI, PIN_CS);
  tft.init(240, 280);
  tft.setSPISpeed(10000000);  // 10 MHz, sangat konservatif
  log_printf("[lcdtest] init selesai, mulai siklus warna\n");
}

void loop() {
  static const uint16_t colors[] = {ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE, ST77XX_WHITE};
  static const char *names[] = {"merah", "hijau", "biru", "putih"};
  for (int i = 0; i < 4; i++) {
    tft.fillScreen(colors[i]);
    log_printf("[lcdtest] %s\n", names[i]);
    delay(1000);
  }
}
