#!/bin/bash
# Salin Desk Buddy ke repo publik (github.com/julisman/desk-buddy) TANPA app privat, lalu commit.
# Repo ini (lengkap, termasuk src/pet/) tetap privat/lokal; repo publik punya history sendiri.
#
#   tools/publish-public.sh "pesan commit"
#   cd ../desk-buddy-public && git push          # push manual setelah dicek
#
# Folder repo publik: ../desk-buddy-public (ubah dengan PUBLIC_DIR=...).
set -euo pipefail
SRC="$(cd "$(dirname "$0")/.." && pwd)"
DST="${PUBLIC_DIR:-$SRC/../desk-buddy-public}"
MSG="${1:?pesan commit wajib, contoh: tools/publish-public.sh \"v1.0.1: perbaikan touch\"}"

[ -d "$DST/.git" ] || { echo "repo publik tidak ada di $DST (git clone dulu)" >&2; exit 1; }

# Yang TIDAK boleh ke publik: app privat, secrets, build output
rsync -a --delete \
  --exclude '.git/' --exclude '.pio/' --exclude '.vscode/' --exclude '.DS_Store' \
  --exclude 'src/secrets.h' --exclude 'src/pet/' --exclude '*.app.ini' \
  "$SRC/" "$DST/"

cd "$DST"
fail() { echo "STOP: $1" >&2; exit 1; }
[ ! -e src/secrets.h ] || fail "src/secrets.h ikut tersalin"
[ ! -e src/pet ] || fail "src/pet ikut tersalin"
if [ -f "$SRC/src/secrets.h" ]; then
  for key in WIFI_SSID WIFI_PASSWORD; do
    val=$(sed -nE "s/^#define $key +\"([^\"]+)\".*/\1/p" "$SRC/src/secrets.h")
    [ -z "$val" ] || ! grep -rqaF --exclude-dir=.git -e "$val" . || fail "$key dari secrets.h ada di repo publik"
  done
fi
# (script ini sendiri menyebut src/pet di daftar exclude, jadi dilewati)
! grep -rqE --exclude-dir=.git --exclude=publish-public.sh 'pet_config|src/pet|env:pet' . || fail "masih ada referensi app pet"

git add -A
if git diff --cached --quiet; then
  echo "tidak ada perubahan"
else
  git commit -q -m "$MSG"
  echo "commit: $(git log --oneline -1)"
  echo "cek lalu push: cd $DST && git push"
fi
