#!/bin/bash
# Build both guides and print them to PDF with headless Chrome:
#   guide.html   -> Desk-Buddy-Build-Guide.pdf   (English)
#   panduan.html -> Desk-Buddy-Panduan-Rakit.pdf (Indonesian)
set -euo pipefail
cd "$(dirname "$0")"
python3 build_guide.py
CHROME="/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"

print_pdf() {  # $1 = input html, $2 = output pdf
  "$CHROME" --headless=new --disable-gpu \
    --no-pdf-header-footer --virtual-time-budget=8000 \
    --print-to-pdf="$PWD/$2" "file://$PWD/$1" 2>/dev/null
  echo "OK: $PWD/$2"
}

print_pdf guide.html Desk-Buddy-Build-Guide.pdf
print_pdf panduan.html Desk-Buddy-Panduan-Rakit.pdf
