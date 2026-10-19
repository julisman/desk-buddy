# Desk Buddy

A little desk companion: an animated face that blinks, glances around, and gets happy when you "pet" it (turn the knob or touch the pad), plus a Pomodoro focus timer, clock + weather, and a Claude Code monitor.

**Status: Phase 1 done (v1.0.1).** Face + petting, FOCUS, CLOCK, CLAUDE (multi-session), WiFi setup via hotspot, web installer. Phase 2: LiPo battery + charger, voice (mic + speaker, AI via the laptop).

- **Install without coding:** https://julisman.github.io/desk-buddy/ (web installer in `docs/`, ESP Web Tools), see [Release](#release-web-installer).
- **Build guide (PDF):** `docs/guide/Desk-Buddy-Build-Guide.pdf` (parts, wiring, install, usage, troubleshooting).
- **Panduan rakit (Bahasa Indonesia):** `docs/guide/Desk-Buddy-Panduan-Rakit.pdf`

Hardware: ESP32-S3-N16R8 DevKitC, ST7789V3 1.69" 240x280 display (SPI), KY-040 rotary encoder, buzzer module, touch pad.

## Layout

```
src/core/        shared hardware code: board.h (pins & hardware), display, input (knob + touch), buzzer, face, net (WiFi/NTP/weather/setup)
src/desk_buddy/  Desk Buddy app: FACE/FOCUS/CLOCK/CLAUDE modes, focus timer, claude_link, app_config.h
tools/           laptop scripts: Claude Code hooks, firmware release
docs/            web installer (GitHub Pages) + PDF build guide
```

Driver fixes (touch, buzzer, display) go in `core/` only. App-specific constants go in `app_config.h`, not in `board.h`. Other apps for the same hardware can be added under `src/<app>/` with their env in a `*.app.ini` file (read automatically by `platformio.ini`).

## Modes

Hold the knob (≥ 0.6 s) to switch modes: **FACE → FOCUS → CLOCK → CLAUDE**. Stroke the touch pad (GPIO 7) any time: happy face, floating hearts, purr sound.

| Mode | Turn knob | Press knob |
| --- | --- | --- |
| FACE | pet → happy face, glances in the turn direction | boop → startled |
| FOCUS | set duration (minutes) while stopped / paused | start / pause |
| CLOCK | pet | boop |
| CLAUDE | switch session (if > 1) | silence the "needs you" alarm |

- **Focus:** default 25 min focus → 5 min break starts automatically → back to focus (press to start again). Orange dots at the bottom = number of completed focus sessions. While focus is running, the face looks serious too. The timer keeps running in other modes; when it finishes, the screen switches to FOCUS and the buzzer sounds.
- **Sleep:** untouched for 5 minutes (23:00–07:00: 1 minute) → sleepy eyes, "zzz", screen dims. Touch the knob to wake it (the first touch only wakes it). It doesn't sleep while the focus timer is running.
- **Clock:** needs WiFi (NTP + weather from [Open-Meteo](https://open-meteo.com), no API key). Without WiFi the clock shows "WiFi setup (see below)"; FACE and FOCUS still work.

## CLAUDE mode: Claude Code status

Desk Buddy shows Claude Code status from your laptop: model, context %, session (5-hour) and weekly limits + reset times, and work status (WORKING / NEEDS YOU / DONE).

Several sessions at once (max 4): each dot at the top of the screen = one session, colored by status (coral working, red needs you, green done, grey idle). Turn the knob to switch sessions. The screen jumps automatically to the session you just sent a prompt to, or the one asking for permission. Session/weekly limits belong to the account, so they're the same for all sessions. A session disappears when it's closed (SessionEnd) or after 2 hours without updates. In CLAUDE mode, every update from Claude wakes the screen and resets the sleep timer. When Claude asks for permission: switches to CLAUDE mode, sounds an alarm (max 3x, press the knob to silence), red frame blinks. A long task (>= 20 s) finishes: a "chirp" + happy face.

### How it works

```
Laptop                                              Desk Buddy (ESP32)
Claude Code                                         HTTP server in firmware
 ├─ status line ─┐                                  (src/desk_buddy/claude_link.cpp)
 └─ hooks ───────┴─► tools/claude-desk-buddy.sh ──WiFi──► POST http://deskbuddy.local/claude
                     (filter: numbers & status only)        -> CLAUDE mode screen
```

- **The server lives on Desk Buddy** and starts automatically once WiFi connects. There is **no server to run on the laptop**: Claude Code itself calls the script on every update.
- The laptop and Desk Buddy must be on the **same WiFi**. Desk Buddy announces itself as `deskbuddy.local` (mDNS), so its IP can change.
- Only these are sent: session id, project folder name, model name, percentages, reset times, event name, permission mode, tool name. Prompts, Claude's replies, file contents and full paths are **not** sent.
- Desk Buddy off / on a different WiFi: requests time out quietly in the background; Claude Code is not affected.

### Laptop setup (once)

Needs `jq` (`brew install jq`) and a Claude Code status line script that reads stdin into `input=$(cat)`.

```sh
tools/install-claude-hooks.sh              # install hooks + 1 line in the status line script
tools/install-claude-hooks.sh --uninstall  # remove again (backup: ~/.claude/*.bak-deskbuddy)
```

What gets changed: `~/.claude/settings.json` (async hooks UserPromptSubmit, PostToolUse, Notification, Stop, SessionEnd) and your status line script. Applies to all sessions; hooks take effect in **new sessions** (status line numbers start working right away).

### Testing & debugging

- Without Claude Code: `tools/claude-desk-buddy.sh demo` (status -> working -> needs you -> done).
- See the data Desk Buddy is holding: `curl -4 http://deskbuddy.local/claude`.
- Different address: `DESK_BUDDY_URL=http://192.168.x.x/claude` (env var for the script).
- `curl -4` is required: without it macOS tries IPv6 first for `.local` and waits ~5 seconds.

## Wiring

### Display

| LCD pin | ESP32-S3 |
| --- | --- |
| BLK | GPIO 9 |
| CS | GPIO 10 |
| DC | GPIO 11 |
| RES | GPIO 12 |
| SDA | GPIO 13 |
| SCL | GPIO 14 |
| VCC | **3V3** (via the breadboard + rail) |
| GND | G |

> **Don't use the 5V pin** when the board is plugged into the native USB port (the one used for upload/logs).
> On this board the 5V pin only gets power from the native USB port if the **IN-OUT** pad is soldered.
> It's open by default, so 5V = 0 V and the LCD stays completely dead.

This board is too wide for a breadboard (the pins land in columns a and j, leaving no free holes),
so the ESP32 stays off the breadboard: the modules plug into the breadboard, with female-to-male wires
from the module rows to the ESP32 pins.

### Encoder & buzzer

| Module | Module pin | Connect to |
| --- | --- | --- |
| KY-040 encoder | GND | breadboard − rail |
| | + | breadboard + rail (3V3, not 5V: the module's pull-ups would put 5V on the GPIOs) |
| | SW | GPIO 6 |
| | DT | GPIO 5 |
| | CLK | GPIO 4 |
| Buzzer | S | GPIO 15 |
| | + (middle) | breadboard + rail |
| | − | breadboard − rail |

The KY-006 buzzer (passive, no resistor) is driven at a low duty (`BUZZER_DUTY 16`). At 50% duty its peak current keeps resetting the board (reset reason 1). Want it louder: put a 100–220 Ω resistor in series between GPIO 15 and S, then raise `BUZZER_DUTY`.

### Touch pad ("petting")

Uses the ESP32-S3's built-in touch sensor, no module. Connect one wire to **GPIO 7**; the other end
becomes the touch pad (for testing: touch the metal tip of the male wire; later: aluminium foil / copper tape inside the case).
While touched, Desk Buddy is happy, looks up and shows hearts. **Don't touch the pad during boot** (the baseline is calibrated at boot).
The `[status] ... touch=value/baseline` log helps tune `TOUCH_ON_RATIO` in `src/core/board.h`.

### Power (shared via the breadboard power rails)

The board only has two 3V3 pins, but the LCD, encoder and buzzer all need 3V3. So:

- ESP32 **3V3** (top pin) → red **+** rail, ESP32 **G** (bottom pin) → blue **−** rail (female-to-male wires).
- Each module's VCC/+ → + rail, each module's GND/− → − rail (male-to-male wires).

The full diagram is on the wiring page (artifact "Desk Buddy LCD Wiring").

Display pins are defined in `platformio.ini` (TFT_eSPI needs build flags), other pins in `src/core/board.h`.

## WiFi setup

WiFi, time zone (WIB/WITA/WIT) and weather location are set from your phone and stored in flash:

1. No WiFi saved yet: Desk Buddy opens a `DeskBuddy-XXXX` hotspot and shows "WiFi setup: join DeskBuddy-XXXX, then open 192.168.4.1" on screen.
2. Connect your phone to that hotspot; the setup page opens by itself (or open `192.168.4.1`).
3. Pick the **WiFi name**, enter the **Password**, set **Time zone** and **Weather location**, then **Save & connect**. Desk Buddy restarts and connects.

- **Change WiFi:** hold the knob while powering on until "WiFi reset" appears.
- **Saved WiFi fails for 45 seconds** (wrong password / moved house): the setup hotspot opens by itself. If nobody opens it within 3 minutes, it restarts and tries the saved WiFi again.
- **Developer:** `src/secrets.h` (from `secrets.h.example`) provides defaults when setup has never been done, so you don't have to redo setup after every flash. The `desk-buddy-release` build ignores it.

## Build & flash (developer)

```sh
cp src/secrets.h.example src/secrets.h   # optional: default WiFi for development
pio run -e desk-buddy -t upload
pio device monitor
```

Plug USB-C into the **native USB** port (shows up as `/dev/cu.usbmodem*`). If upload fails: hold BOOT, press RST, release BOOT, upload again.

> On this Mac PlatformIO lives at `~/.platformio/penv/bin/pio` (see the robot car README about Homebrew Python).

## Release (web installer)

```sh
# 1. bump APP_VERSION in src/desk_buddy/app_config.h
tools/make-release.sh        # build desk-buddy-release (without secrets.h) -> docs/firmware/deskbuddy-<version>.bin + docs/manifest.json
docs/guide/make-pdf.sh       # PDF guide (repo/installer URLs are in docs/guide/build_guide.py)
```

`docs/` is a static installer page (ESP Web Tools: desktop Chrome/Edge, Web Serial), hosted on GitHub Pages from the `docs/` folder: https://julisman.github.io/desk-buddy/ (PDF: `/guide/Desk-Buddy-Build-Guide.pdf`, Indonesian: `/guide/Desk-Buddy-Panduan-Rakit.pdf`). The release script refuses to continue if the SSID from `secrets.h` ends up in the firmware.

The public repo (github.com/julisman/desk-buddy) is filled from this repo without the private app:
```sh
tools/publish-public.sh "v1.0.1: ..."   # rsync to ../desk-buddy-public (without private files & secrets.h) + checks + commit
cd ../desk-buddy-public && git push
```

Normal log:
```
=== Desk Buddy v1.0.1 ===
[boot] display red-green-blue test done
[boot] ready (reset reason 1)
[wifi] connecting to "wifi-name"
[wifi] connected, IP 192.168.x.x
[mdns] http://deskbuddy.local
[claude] server ready: POST http://deskbuddy.local/claude
[weather] 29.4 C, Partly cloudy
```
(`spiAttachMISO(): HSPI Does not have default pins` at boot is harmless, ignore it.)

## Test after wiring

1. **Display:** after boot the face appears (cyan eyes), blinking and glancing around on its own. The screen fades in and the buzzer plays 3 rising notes.
2. **Encoder:** turn the knob → happy eyes + tick sound. One click of rotation should be one step in FOCUS (25 → 26).
3. **Button:** press → startled face. Hold → mode changes, the mode label appears at the top.
4. **Focus:** set 1 minute, press start, wait → finish melody, happy face, 5 min break starts.
5. **Clock:** time + date + temperature appear (after WiFi connects, ~5–10 seconds).
6. **Sleep:** leave it for 5 minutes → sleepy eyes + zzz + dim screen. Turn the knob → it wakes up.

## Tuning

| Symptom | Change |
| --- | --- |
| Inverted colors (white background, red eyes) | add `-DTFT_INVERSION_OFF=1` (or `_ON`) in `platformio.ini` |
| Red/blue swapped | `TFT_RGB_ORDER` in `platformio.ini` (this panel: `TFT_BGR`) |
| Image shifted / cut off | check the display size: `TFT_HEIGHT` (platformio.ini) and `SCREEN_H` (core/board.h), currently 280 |
| Display rotated | `SCREEN_ROTATION` in `core/board.h` |
| 1 knob click = 2 steps, or 2 clicks needed for 1 step | `ENC_STEPS_PER_DETENT` (4 ↔ 2) |
| Turn direction reversed | `ENC_REVERSE 1` |
| Buzzer only "clicks" or plays a single tone | it's an active buzzer: `BUZZER_PASSIVE 0` |
| Buzzer sounds constantly when idle | active-low module: `BUZZER_ACTIVE_LOW 1` |
| Time zone | `TZ_INFO` in `core/board.h` (WIB-7 / WITA-8 / WIT-9) |

## Display debugging

- At boot the screen flashes red → green → blue. If it doesn't, SPI data isn't arriving: check the SDA/SCL/CS/DC/RES wires and the pin header contact in the LCD holes.
- `pio run -e lcdtest -t upload`: continuous color test using a different library (Adafruit ST7789), to make sure the problem isn't in TFT_eSPI.
- `pio run -e pintest -t upload`: GPIO 9–14 blink together. Move the data wires to the BLK hole one by one; if the backlight blinks, that wire is good.

## Technical notes

- 240x280 frame buffer (134 KB) in PSRAM, redrawn every frame then pushed at once → no flicker, ~28 fps.
- WiFi/NTP/weather run in their own task on core 0, so HTTP requests don't stutter the animation.
- Logging uses `LOG()` (see `src/core/log.h`), not `Serial.printf`: on core 2.0.17, `Serial` on the native USB port can wrongly detect "not connected" and drop all output.

## License

[CC BY-NC 4.0](LICENSE): free to use, share and modify for **non-commercial** purposes with attribution. For commercial use (for example selling kits or finished units), contact the author. Third-party libraries (TFT_eSPI, ArduinoJson, ESP Web Tools) use their own licenses.
