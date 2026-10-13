#!/bin/bash
# Bikin file firmware untuk web installer (docs/):
#   1. build env desk-buddy-release (tanpa secrets.h)
#   2. gabung bootloader + partisi + app jadi satu .bin (flash di offset 0)
#   3. tulis docs/manifest.json untuk ESP Web Tools
#
#   tools/make-release.sh
# Versi diambil dari APP_VERSION di src/desk_buddy/app_config.h.
set -euo pipefail
cd "$(dirname "$0")/.."

PIO="${PIO:-$HOME/.platformio/penv/bin/pio}"
PY="$HOME/.platformio/penv/bin/python"
ESPTOOL="$HOME/.platformio/packages/tool-esptoolpy/esptool.py"
BOOT_APP0="$HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"
BUILD=.pio/build/desk-buddy-release

VERSION=$(sed -nE 's/^#define APP_VERSION +"([^"]+)".*/\1/p' src/desk_buddy/app_config.h)
[ -n "$VERSION" ] || { echo "APP_VERSION tidak ketemu" >&2; exit 1; }

"$PIO" run -e desk-buddy-release

# Jaga-jaga: file publik tidak boleh berisi WiFi developer
if [ -f src/secrets.h ]; then
  ssid=$(sed -nE 's/^#define WIFI_SSID +"([^"]+)".*/\1/p' src/secrets.h)
  if [ -n "$ssid" ] && grep -aq "$ssid" "$BUILD/firmware.bin"; then
    echo "STOP: firmware berisi SSID dari secrets.h" >&2
    exit 1
  fi
fi

mkdir -p docs/firmware
OUT="docs/firmware/deskbuddy-$VERSION.bin"
"$PY" "$ESPTOOL" --chip esp32s3 merge_bin -o "$OUT" \
  --flash_mode keep --flash_freq keep --flash_size keep \
  0x0 "$BUILD/bootloader.bin" \
  0x8000 "$BUILD/partitions.bin" \
  0xe000 "$BOOT_APP0" \
  0x10000 "$BUILD/firmware.bin"

cat > docs/manifest.json <<EOF
{
  "name": "Desk Buddy",
  "version": "$VERSION",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32-S3",
      "parts": [{ "path": "firmware/deskbuddy-$VERSION.bin", "offset": 0 }]
    }
  ]
}
EOF

# File versi lama tidak dipakai lagi oleh manifest
find docs/firmware -name 'deskbuddy-*.bin' ! -name "deskbuddy-$VERSION.bin" -delete
echo "OK: $OUT ($(du -h "$OUT" | cut -f1)), docs/manifest.json -> v$VERSION"
