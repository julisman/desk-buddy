#!/bin/bash
# panduan.html -> Desk-Buddy-Panduan-Rakit.pdf (Chrome headless)
set -euo pipefail
cd "$(dirname "$0")"
python3 build_guide.py
"/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" --headless=new --disable-gpu \
  --no-pdf-header-footer --virtual-time-budget=8000 \
  --print-to-pdf="$PWD/Desk-Buddy-Panduan-Rakit.pdf" "file://$PWD/panduan.html" 2>/dev/null
echo "OK: $PWD/Desk-Buddy-Panduan-Rakit.pdf"
