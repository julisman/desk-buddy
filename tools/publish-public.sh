#!/bin/bash
# Copy Desk Buddy to the public repo (github.com/julisman/desk-buddy) WITHOUT the private app, then commit.
# This repo (complete, including src/pet/) stays private/local; the public repo has its own history.
#
#   tools/publish-public.sh "commit message"
#   cd ../desk-buddy-public && git push          # push manually after checking
#
# Public repo folder: ../desk-buddy-public (change with PUBLIC_DIR=...).
set -euo pipefail
SRC="$(cd "$(dirname "$0")/.." && pwd)"
DST="${PUBLIC_DIR:-$SRC/../desk-buddy-public}"
MSG="${1:?commit message required, e.g.: tools/publish-public.sh \"v1.0.1: touch fix\"}"

[ -d "$DST/.git" ] || { echo "public repo not found at $DST (git clone it first)" >&2; exit 1; }

# What must NOT go public: private app, secrets, build output
rsync -a --delete \
  --exclude '.git' --exclude '.pio/' --exclude '.vscode/' --exclude '.DS_Store' \
  --exclude 'src/secrets.h' --exclude 'src/pet/' --exclude '*.app.ini' \
  "$SRC/" "$DST/"

cd "$DST"
fail() { echo "STOP: $1" >&2; exit 1; }
[ ! -e src/secrets.h ] || fail "src/secrets.h was copied"
[ ! -e src/pet ] || fail "src/pet was copied"
if [ -f "$SRC/src/secrets.h" ]; then
  for key in WIFI_SSID WIFI_PASSWORD; do
    val=$(sed -nE "s/^#define $key +\"([^\"]+)\".*/\1/p" "$SRC/src/secrets.h")
    [ -z "$val" ] || ! grep -rqaF --exclude-dir=.git -e "$val" . || fail "$key from secrets.h is in the public repo"
  done
fi
# (this script itself names src/pet in its exclude list, so it is skipped)
! grep -rqE --exclude-dir=.git --exclude=publish-public.sh 'pet_config|src/pet|env:pet' . || fail "references to the pet app remain"

git add -A
if git diff --cached --quiet; then
  echo "no changes"
else
  git commit -q -m "$MSG"
  echo "commit: $(git log --oneline -1)"
  echo "check, then push: cd $DST && git push"
fi
