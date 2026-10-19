#!/usr/bin/env python3
"""Generate the Desk Buddy build guide in two languages from one layout.

    guide.html    English
    panduan.html  Indonesian

Usage:
    python3 docs/guide/build_guide.py      # HTML only
    bash docs/guide/make-pdf.sh            # HTML + both PDFs (headless Chrome)

The layout lives in render(); every piece of language-specific text lives in
the EN / ID dicts below. Device screen and setup-portal text is quoted in
English in both languages because the firmware UI is English.
URLs below are the public repo / installer; update them when publishing.
"""
import html, pathlib

REPO_URL = "https://github.com/julisman/desk-buddy"
INSTALL_URL = "https://julisman.github.io/desk-buddy/"
VERSION = "1.0.1"
HERE = pathlib.Path(__file__).parent

# ---------- Wiring (shared by both languages) ----------
ESP_PINS = ["3V3", "4", "5", "6", "7", "15", "9", "10", "11", "12", "13", "14", "G"]
C = {"pwr": "#d93025", "gnd": "#3c4043", "lcd": "#1a73e8", "enc": "#188038", "bz": "#b06000", "touch": "#a142f4"}


def modules(t):
    """Modules and their pins: (name, [(module pin label, ESP32 pin, colour key)])."""
    return [
        ("LCD 1.69\" ST7789", [("BLK", "9", "lcd"), ("CS", "10", "lcd"), ("DC", "11", "lcd"), ("RES", "12", "lcd"),
                               ("SDA", "13", "lcd"), ("SCL", "14", "lcd"), ("VCC", "3V3", "pwr"), ("GND", "G", "gnd")]),
        ("Knob KY-040", [("CLK", "4", "enc"), ("DT", "5", "enc"), ("SW", "6", "enc"), ("+", "3V3", "pwr"), ("GND", "G", "gnd")]),
        ("Buzzer KY-006", [("S", "15", "bz"), (t["buzzer_plus"], "3V3", "pwr"), ("−", "G", "gnd")]),
        ("Touch pad", [(t["touch_wire"], "7", "touch")]),
    ]


def wiring_svg(t):
    mods = modules(t)
    W, row = 760, 30
    esp_x, esp_y = 40, 50
    esp_pin_y = {p: esp_y + 26 + i * row for i, p in enumerate(ESP_PINS)}
    esp_h = len(ESP_PINS) * row + 30
    mod_h = 30 + sum(len(p) * row + 34 + 16 for _, p in mods)
    out = [f'<svg viewBox="0 0 {W} {max(esp_h + esp_y, mod_h) + 10}" xmlns="http://www.w3.org/2000/svg" font-family="IBM Plex Mono, monospace">']
    out.append(f'<rect x="{esp_x}" y="{esp_y}" width="120" height="{esp_h}" rx="10" fill="#202124"/>')
    out.append(f'<text x="{esp_x + 60}" y="{esp_y - 14}" text-anchor="middle" font-size="13" fill="#5f6368">{t["esp_header"]}</text>')
    for p, y in esp_pin_y.items():
        out.append(f'<text x="{esp_x + 16}" y="{y + 4}" font-size="13" fill="#fff">{"GPIO " + p if p.isdigit() else p}</text>')
    lines, boxes = [], []
    my, mx, lane = 30, 470, 0
    for name, pins in mods:
        h = len(pins) * row + 34
        boxes.append(f'<rect x="{mx}" y="{my}" width="250" height="{h}" rx="10" fill="#f1f3f4" stroke="#dadce0"/>')
        boxes.append(f'<text x="{mx + 14}" y="{my + 22}" font-size="13" font-weight="600" fill="#202124">{html.escape(name)}</text>')
        for i, (label, esp, kind) in enumerate(pins):
            y = my + 44 + i * row
            boxes.append(f'<circle cx="{mx}" cy="{y}" r="5" fill="{C[kind]}"/>')
            boxes.append(f'<text x="{mx + 14}" y="{y + 4}" font-size="12" fill="#202124">{html.escape(label)}</text>')
            ey = esp_pin_y[esp]
            x1, x2 = esp_x + 120, mx
            bend = 200 + (lane % 22) * 11
            lane += 1
            lines.append(f'<path d="M{x1} {ey} H{bend} V{y} H{x2}" fill="none" stroke="{C[kind]}" stroke-width="2.2" stroke-opacity=".85"/>')
        my += h + 16
    out += lines + boxes
    for p, y in esp_pin_y.items():
        out.append(f'<circle cx="{esp_x + 120}" cy="{y}" r="5" fill="#fbbc04"/>')
    out.append("</svg>")
    return "\n".join(out)


def table(rows, head, cls=""):
    th = "".join(f"<th>{h}</th>" for h in head)
    trs = "".join("<tr>" + "".join(f"<td>{c}</td>" for c in r) + "</tr>" for r in rows)
    return f"<table class=\"{cls}\"><thead><tr>{th}</tr></thead><tbody>{trs}</tbody></table>"


def pin_rows(t):
    rows = []
    for name, pins in modules(t):
        for label, esp, kind in pins:
            target = "GPIO " + esp if esp.isdigit() else (t["to_3v3"] if esp == "3V3" else t["to_gnd"])
            rows.append((f'<span class="dot" style="background:{C[kind]}"></span>{html.escape(name)}', html.escape(label), target))
    return rows


def ui(s):
    """Quote a device / portal UI string (always English)."""
    return f'<span class="mono ui">{html.escape(s)}</span>'


# Exact English UI strings shown by the firmware.
UI_CLOCK_NOWIFI = ui("WiFi setup (see below)")
UI_BANNER = ui("WiFi setup: join DeskBuddy-XXXX") + " / " + ui("then open 192.168.4.1")
UI_RESET = ui("WiFi reset")
UI_SAVED = ui("Saved")
F_NAME, F_PASS, F_TZ, F_WX, F_BTN = (ui(s) for s in ("WiFi name", "Password", "Time zone", "Weather location", "Save & connect"))

# ---------- Per-language text ----------
EN = {
    "lang": "en", "out": "guide.html", "title": "Desk Buddy Build Guide",
    "buzzer_plus": "+ (middle)", "touch_wire": "wire / copper tape", "esp_header": "ESP32-S3 (left header)",
    "to_3v3": "3V3 (breadboard + rail)", "to_gnd": "GND (breadboard − rail)",
    "kicker": f"Build guide · v{VERSION}",
    "sub": "A small desk companion: a face that blinks and loves to be petted, a focus timer, clock &amp; weather, and a Claude Code monitor.",
    "hero_alt": "Desk Buddy showing Claude Code status",
    "feat": [("Living face", "Blinks, glances around, gets happy when you turn the knob or pet the pad."),
             ("Focus timer", "25/5 minute Pomodoro; the face turns serious while you focus."),
             ("Clock &amp; weather", "Synced over the internet, with weather for your location."),
             ("Claude Code monitor", "Context, session &amp; weekly limits, alarm when Claude needs permission.")],
    "meta": f"Firmware &amp; code: {REPO_URL} · Installer: {INSTALL_URL}<br>License CC BY-NC 4.0: free for non-commercial use with attribution.",
    "meet_h": "Meet Desk Buddy",
    "meet_p": "A 1.69\" screen with a lively robot face. Hold the knob to switch modes.",
    "gallery": ["Claude Code is working: context, session and weekly limits, reset times.",
                "The session is waiting for the next prompt. Bar colours: green is safe, yellow is getting high, red is almost used up.",
                "Time, date and weather for your location.",
                "Pomodoro with a progress ring; the face turns serious."],
    "parts_h": "Parts list",
    "parts_p": "Approximate Shopee prices (October 2026). Search for the keywords directly on Shopee.",
    "bom_head": ["Part", "Keyword", "Price", "Notes"],
    "bom": [
        ("ESP32-S3 N16R8 DevKitC (2 USB-C ports)", "esp32 s3 n16r8", "Rp85–110k", "Must be N16R8 (16MB flash, 8MB PSRAM)"),
        ("LCD 1.69\" ST7789 240x280 SPI", "lcd 1.69 st7789", "Rp40–55k", "One with a BLK pin"),
        ("Rotary encoder KY-040", "ky-040 rotary encoder", "Rp6–10k", "Turn + press knob"),
        ("Passive buzzer KY-006", "ky-006 buzzer pasif", "Rp4–7k", "3-pin module: S, +, −"),
        ("Breadboard, 400 holes", "breadboard 400", "Rp8–12k", "Or a mini breadboard"),
        ("Female-male jumper wires (20 pcs)", "kabel jumper female male", "Rp8–12k", "Signals to the ESP32 pins"),
        ("Male-male jumper wires (20 pcs)", "kabel jumper male male", "Rp8–12k", "To the breadboard power rails"),
        ("USB-C data cable", "kabel usb c data", "Rp15–25k", "Not a charge-only cable"),
        ("Copper tape (optional)", "copper tape", "Rp10–15k", "Touch pad for petting"),
    ],
    "total": ("Estimated total", "Rp185–260k"),
    "tools_h": "Tools",
    "tools_p": "A laptop/PC with Google Chrome or Microsoft Edge (to install the firmware). No soldering needed for this breadboard version. Optional: solder the LCD header so the contacts last.",
    "board_note": "<b>Important:</b> the board must be an <b>ESP32-S3 N16R8</b>. Other variants (N8, N8R2, plain ESP32) have different memory and this firmware will not run on them.",
    "wiring_h": "Wiring",
    "wiring_p": "All signals go to the left header of the ESP32-S3. Power is shared through the breadboard rails: pin <b>3V3</b> to the <b>+</b> rail, pin <b>G</b> to the <b>−</b> rail.",
    "conn_h": "Connection table",
    "conn_head": ["Module", "Module pin", "Connect to"],
    "no5v": "<b>Do not use the 5V pin.</b> When plugged into the native USB port, the 5V pin has no power. All modules use 3V3.",
    "steps_h": "Assembly steps",
    "steps": [
        "Unplug USB. Push the LCD, knob and buzzer into the breadboard, each in its own rows.",
        "Male-male wires: ESP32 pin <b>3V3</b> to the <b>+</b> rail, pin <b>G</b> to the <b>−</b> rail. Then each module's VCC/+ to the + rail and GND/− to the − rail.",
        "Female-male wires: female end on the ESP32 pin, male end in the module's pin row, as in the table.",
        "Touch pad: one wire to GPIO 7. Leave the other end free, or stick on 2x2 cm of copper tape (it works through a plastic case).",
        "Double-check: no wire on the 5V pin, and the + and − rails are not swapped.",
    ],
    "lcd_note": "LCD without soldering: the LCD header pins often make poor contact. Insert it slightly tilted and hold it with tape so the pins press against the sides of the holes.",
    "inst_h": "Install the firmware",
    "inst_p": "No software to install. On your laptop, open (Chrome or Edge):",
    "inst": [
        "Plug the ESP32-S3 into your laptop with a USB-C <b>data</b> cable, using the port labelled <b>USB</b> (not COM).",
        "Click <b>Install Desk Buddy</b>, choose the port that appears (usually \"USB JTAG/serial debug unit\"), then <b>Install</b>. Wait about 1 minute.",
        "Press the <b>RST</b> button on the board. The screen flashes red-green-blue, then the face appears.",
    ],
    "wifi_h": "Connect to WiFi",
    "wifi": [
        f"The bottom of the screen shows the setup banner {UI_BANNER}, where <span class=\"mono\">XXXX</span> is your device's code, e.g. <span class=\"mono\">DeskBuddy-1A2B</span>.",
        "On your phone, open the WiFi settings and join that hotspot. The setup page opens by itself; if it doesn't, open <span class=\"mono\">192.168.4.1</span> in the browser.",
        f"Fill in {F_NAME} (your home WiFi), {F_PASS}, {F_TZ} (e.g. WIB/WITA/WIT) and {F_WX} (your city), then tap {F_BTN}. The page shows {UI_SAVED}, Desk Buddy restarts and connects.",
    ],
    "wifi_note": f"Changing WiFi / moving house: hold the knob while powering on Desk Buddy until {UI_RESET} appears. If the saved WiFi fails to connect, the setup hotspot also opens by itself.",
    "src_p": f"Want to change the code? The full source is at <span class=\"mono\">{REPO_URL}</span> (PlatformIO).",
    "use_h": "How to use",
    "use_p": "<b>Hold the knob</b> (about 1 second) to change mode. <b>Pet</b> the touch pad any time: the face gets happy, hearts pop out, and it \"purrs\".",
    "modes": [
        ("FACE", "Turn = petting (happy, glancing). Press = boop (surprised)."),
        ("FOCUS", "Turn = set minutes. Press = start / pause. Focus for 25 minutes, then a 5-minute break starts automatically."),
        ("CLOCK", f"Time, date and weather for your location. Before WiFi is set up it shows {UI_CLOCK_NOWIFI}."),
        ("CLAUDE", "Claude Code status from your laptop. Turn = switch session. Press = silence the alarm."),
    ],
    "sleep_p": "Untouched for 5 minutes (at night, 23:00–07:00: 1 minute), Desk Buddy goes to sleep and dims the screen. Touch the knob to wake it.",
    "claude_h": "CLAUDE mode (for Claude Code users)",
    "claude_p": "Desk Buddy shows the model, context %, session (5-hour) and weekly limits with their reset times, and the status: <b>WORKING</b>, <b>NEEDS YOU</b> (alarm + red frame when Claude asks for permission), <b>DONE</b>. It can track up to 4 sessions at once.",
    "claude": [
        "Laptop and Desk Buddy on the same WiFi. Requires <span class=\"mono\">jq</span> (<span class=\"mono\">brew install jq</span>).",
        f"Download the source code from {REPO_URL}, then run <span class=\"mono\">tools/install-claude-hooks.sh</span>. To remove it: <span class=\"mono\">--uninstall</span>.",
        "Start a new Claude Code session. To try it without Claude: <span class=\"mono\">tools/claude-desk-buddy.sh demo</span>.",
    ],
    "privacy_p": "Only numbers and status are sent to Desk Buddy. Your prompts and Claude's replies never leave your laptop.",
    "trouble_h": "Troubleshooting",
    "trouble_head": ["Symptom", "Fix"],
    "trouble": [
        ("Screen lights up but is plain grey / white", "The LCD data wires (SDA, SCL, CS, DC, RES) aren't making contact. Re-seat them, tilt the LCD header slightly in the breadboard, hold it with tape, press RST. Soldering the LCD header is the permanent fix."),
        ("Screen completely dead", "LCD VCC must go to 3V3, not 5V (the 5V pin has no power when plugged into native USB)."),
        ("Board keeps restarting when it beeps", "Check the buzzer wiring: S to GPIO 15, + to 3V3, − to GND. The firmware already uses a low volume to be safe."),
        ("Port doesn't appear when installing", "Swap the USB-C cable (many are charge-only). Try the other USB port. Or hold BOOT, press RST, release BOOT."),
        ("Knob reversed / jumps by 2", "Swap the CLK and DT wires. If 1 click = 2 steps, let us know (firmware setting)."),
        ("No clock / weather", f"WiFi isn't connected yet (clock mode shows {UI_CLOCK_NOWIFI}). Hold the knob while powering on to redo WiFi setup."),
        ("Petting isn't detected", "Don't touch the pad while Desk Buddy is powering on (calibration). Use a short wire and a wider pad (2x2 cm copper tape)."),
    ],
    "next_h": "Next",
    "next_p": "<b>Phase 2:</b> LiPo battery + Type-C charger (take it anywhere, battery icon) and voice: talk to Desk Buddy through a mic and speaker, answered by an AI running on your laptop.",
}

ID = {
    "lang": "id", "out": "panduan.html", "title": "Desk Buddy — Panduan Rakit",
    "buzzer_plus": "+ (tengah)", "touch_wire": "kabel / copper tape", "esp_header": "ESP32-S3 (header kiri)",
    "to_3v3": "3V3 (jalur + breadboard)", "to_gnd": "GND (jalur − breadboard)",
    "kicker": f"Panduan rakit · v{VERSION}",
    "sub": "Teman kecil di meja: wajah yang berkedip dan senang dielus, focus timer, jam &amp; cuaca, dan monitor Claude Code.",
    "hero_alt": "Desk Buddy menampilkan status Claude Code",
    "feat": [("Wajah hidup", "Berkedip, melirik, senang saat knob diputar atau pad dielus."),
             ("Focus timer", "Pomodoro 25/5 menit, wajah ikut serius saat fokus."),
             ("Jam &amp; cuaca", "Sinkron lewat internet, cuaca lokasi kamu."),
             ("Monitor Claude Code", "Context, limit sesi &amp; mingguan, alarm saat Claude butuh izin.")],
    "meta": f"Firmware &amp; kode: {REPO_URL} · Installer: {INSTALL_URL}<br>Lisensi CC BY-NC 4.0: bebas untuk non-komersial dengan menyebut sumber.",
    "meet_h": "Kenalan dengan Desk Buddy",
    "meet_p": "Layar 1.69\" dengan wajah robot yang hidup. Tahan knob untuk pindah mode.",
    "gallery": ["Claude Code sedang bekerja: context, limit sesi dan mingguan, jam reset.",
                "Sesi menunggu prompt berikutnya. Warna bar: hijau aman, kuning mulai tinggi, merah hampir habis.",
                "Jam, tanggal, dan cuaca lokasi kamu.",
                "Pomodoro dengan cincin progres; wajah ikut serius."],
    "parts_h": "Daftar komponen",
    "parts_p": "Harga perkiraan Shopee (Oktober 2026). Kata kunci bisa langsung dicari di Shopee.",
    "bom_head": ["Komponen", "Kata kunci", "Harga", "Catatan"],
    "bom": [
        ("ESP32-S3 N16R8 DevKitC (2 port USB-C)", "esp32 s3 n16r8", "Rp85–110rb", "Harus N16R8 (flash 16MB, PSRAM 8MB)"),
        ("LCD 1.69\" ST7789 240x280 SPI", "lcd 1.69 st7789", "Rp40–55rb", "Yang ada pin BLK"),
        ("Rotary encoder KY-040", "ky-040 rotary encoder", "Rp6–10rb", "Knob putar + tekan"),
        ("Buzzer pasif KY-006", "ky-006 buzzer pasif", "Rp4–7rb", "Modul 3 pin: S, +, −"),
        ("Breadboard 400 lubang", "breadboard 400", "Rp8–12rb", "Atau mini breadboard"),
        ("Kabel jumper female-male (20 pcs)", "kabel jumper female male", "Rp8–12rb", "Sinyal ke pin ESP32"),
        ("Kabel jumper male-male (20 pcs)", "kabel jumper male male", "Rp8–12rb", "Ke jalur power breadboard"),
        ("Kabel USB-C data", "kabel usb c data", "Rp15–25rb", "Bukan kabel cas saja"),
        ("Copper tape (opsional)", "copper tape", "Rp10–15rb", "Pad sentuh untuk dielus"),
    ],
    "total": ("Total perkiraan", "Rp185–260rb"),
    "tools_h": "Alat",
    "tools_p": "Laptop/PC dengan Google Chrome atau Microsoft Edge (untuk install firmware). Tidak perlu solder untuk versi breadboard ini. Opsional: solder untuk header LCD supaya kontaknya awet.",
    "board_note": "<b>Penting:</b> board harus <b>ESP32-S3 N16R8</b>. Varian lain (N8, N8R2, ESP32 biasa) memakai memori berbeda dan firmware ini tidak akan jalan.",
    "wiring_h": "Wiring",
    "wiring_p": "Semua sinyal ke header kiri ESP32-S3. Listrik dibagi lewat jalur breadboard: pin <b>3V3</b> ke jalur <b>+</b>, pin <b>G</b> ke jalur <b>−</b>.",
    "conn_h": "Tabel sambungan",
    "conn_head": ["Modul", "Pin modul", "Sambung ke"],
    "no5v": "<b>Jangan pakai pin 5V.</b> Saat dicolok ke port USB native, pin 5V tidak ada listriknya. Semua modul pakai 3V3.",
    "steps_h": "Langkah rakit",
    "steps": [
        "Cabut USB. Tancap LCD, knob, dan buzzer di breadboard, masing-masing di baris sendiri.",
        "Kabel male-male: pin <b>3V3</b> ESP32 ke jalur <b>+</b>, pin <b>G</b> ke jalur <b>−</b>. Lalu VCC/+ tiap modul ke jalur +, GND/− ke jalur −.",
        "Kabel female-male: ujung female ke pin ESP32, ujung male ke baris pin modul, sesuai tabel.",
        "Touch pad: satu kabel ke GPIO 7. Ujung satunya dibiarkan bebas, atau tempel copper tape 2x2 cm (bisa dari balik casing plastik).",
        "Cek sekali lagi: tidak ada kabel ke pin 5V, jalur + dan − tidak tertukar.",
    ],
    "lcd_note": "LCD tanpa solder: pin header LCD sering kurang nempel. Tancap sedikit miring dan tahan pakai selotip supaya pin menekan dinding lubang.",
    "inst_h": "Install firmware",
    "inst_p": "Tanpa install software. Buka di laptop (Chrome atau Edge):",
    "inst": [
        "Colok ESP32-S3 ke laptop pakai kabel USB-C <b>data</b>, di port <b>USB</b> (bukan COM).",
        "Klik <b>Install Desk Buddy</b>, pilih port yang muncul (biasanya \"USB JTAG/serial debug unit\"), lalu <b>Install</b>. Tunggu sekitar 1 menit.",
        "Tekan tombol <b>RST</b> di board. Layar berkedip merah-hijau-biru lalu muncul wajah.",
    ],
    "wifi_h": "Sambungkan ke WiFi",
    "wifi": [
        f"Di bawah layar muncul banner setup {UI_BANNER}. <span class=\"mono\">XXXX</span> adalah kode perangkat kamu, misalnya <span class=\"mono\">DeskBuddy-1A2B</span>.",
        "Di HP, buka pengaturan WiFi dan sambung ke hotspot itu. Halaman setup terbuka sendiri; kalau tidak, buka <span class=\"mono\">192.168.4.1</span> di browser.",
        f"Isi {F_NAME} (nama WiFi rumah), {F_PASS}, {F_TZ} (zona waktu, misalnya WIB/WITA/WIT) dan {F_WX} (kota kamu, untuk cuaca), lalu tekan {F_BTN}. Halaman menampilkan {UI_SAVED} (tersimpan), Desk Buddy restart dan tersambung.",
    ],
    "wifi_note": f"Ganti WiFi / pindah rumah: tahan knob sambil menyalakan Desk Buddy sampai muncul {UI_RESET}. Kalau WiFi tersimpan gagal tersambung, hotspot setup juga terbuka sendiri.",
    "src_p": f"Mau ubah kode? Source code lengkap ada di <span class=\"mono\">{REPO_URL}</span> (PlatformIO).",
    "use_h": "Cara pakai",
    "use_p": "<b>Tahan knob</b> (± 1 detik) untuk ganti mode. <b>Elus</b> pad sentuh kapan saja: wajah senang, keluar hati, bunyi \"purr\".",
    "modes": [
        ("FACE", "Putar = dielus (senang, melirik). Tekan = boop (kaget)."),
        ("FOCUS", "Putar = atur menit. Tekan = mulai / jeda. Fokus 25 menit lalu istirahat 5 menit otomatis."),
        ("CLOCK", f"Jam, tanggal, dan cuaca lokasi kamu. Sebelum WiFi di-setup, muncul {UI_CLOCK_NOWIFI}."),
        ("CLAUDE", "Status Claude Code dari laptop. Putar = pindah sesi. Tekan = diamkan alarm."),
    ],
    "sleep_p": "Tidak disentuh 5 menit (malam 23:00–07:00: 1 menit): Desk Buddy tidur dan layar redup. Sentuh knob untuk membangunkan.",
    "claude_h": "Mode CLAUDE (untuk pengguna Claude Code)",
    "claude_p": "Desk Buddy menampilkan model, context %, limit sesi (5 jam) dan mingguan beserta jam reset, dan status: <b>WORKING</b>, <b>NEEDS YOU</b> (alarm + bingkai merah saat Claude minta izin), <b>DONE</b>. Bisa memantau sampai 4 sesi sekaligus.",
    "claude": [
        "Laptop dan Desk Buddy di WiFi yang sama. Butuh <span class=\"mono\">jq</span> (<span class=\"mono\">brew install jq</span>).",
        f"Download source code dari {REPO_URL}, lalu jalankan <span class=\"mono\">tools/install-claude-hooks.sh</span>. Lepas lagi: <span class=\"mono\">--uninstall</span>.",
        "Buka sesi Claude Code baru. Coba tanpa Claude: <span class=\"mono\">tools/claude-desk-buddy.sh demo</span>.",
    ],
    "privacy_p": "Yang dikirim ke Desk Buddy cuma angka dan status. Prompt dan balasan Claude tidak pernah keluar dari laptop.",
    "trouble_h": "Kalau ada masalah",
    "trouble_head": ["Gejala", "Solusi"],
    "trouble": [
        ("Layar menyala tapi abu-abu / putih polos", "Kabel data LCD (SDA, SCL, CS, DC, RES) tidak nempel. Tancap ulang, miringkan sedikit header LCD di breadboard, tahan pakai selotip, tekan RST. Solder header LCD = solusi permanen."),
        ("Layar mati total", "VCC LCD harus ke 3V3, bukan 5V (pin 5V tidak ada listrik saat colok USB native)."),
        ("Board restart terus saat bunyi", "Cek kabel buzzer: S ke GPIO 15, + ke 3V3, − ke GND. Firmware sudah memakai volume kecil supaya aman."),
        ("Port tidak muncul saat install", "Ganti kabel USB-C (banyak yang cuma untuk cas). Coba port USB yang satunya. Atau tahan BOOT, tekan RST, lepas BOOT."),
        ("Knob kebalik / loncat 2", "Tukar kabel CLK dan DT. Kalau 1 klik = 2 langkah, beri tahu kami (setting firmware)."),
        ("Jam / cuaca tidak muncul", f"WiFi belum tersambung (mode jam menampilkan {UI_CLOCK_NOWIFI}). Tahan knob sambil menyalakan untuk setup ulang WiFi."),
        ("Elus tidak terasa", "Jangan sentuh pad saat Desk Buddy menyala (kalibrasi). Pakai kabel pendek dan pad lebih lebar (copper tape 2x2 cm)."),
    ],
    "next_h": "Berikutnya",
    "next_p": "<b>Phase 2:</b> baterai LiPo + charger Type-C (bisa dibawa ke mana-mana, ikon baterai) dan suara: ngobrol dengan Desk Buddy lewat mic dan speaker, dijawab AI dari laptop kamu.",
}

CSS = """
@page { size: A4; margin: 16mm 16mm 18mm; }
:root { --ink:#1d1f22; --muted:#5f6368; --accent:#e8193c; --line:#e3e3df; --soft:#f6f5f2; }
* { box-sizing: border-box; }
body { margin:0; color:var(--ink); font: 10.5pt/1.55 "IBM Plex Sans", system-ui, sans-serif; }
h1,h2,h3 { font-family: Fraunces, Georgia, serif; line-height:1.15; margin:0; }
h2 { font-size: 20pt; margin: 0 0 4mm; }
h3 { font-size: 12.5pt; margin: 5mm 0 2mm; }
p { margin: 0 0 3mm; }
section { break-before: page; }
.cover { height: 255mm; display:flex; flex-direction:column; justify-content:space-between; break-before:auto; }
.cover .kicker { font-family:"IBM Plex Mono", monospace; color:var(--accent); letter-spacing:.12em; font-size:9pt; text-transform:uppercase; }
.cover h1 { font-size: 54pt; font-weight:800; margin: 4mm 0; }
.cover .sub { font-size: 14pt; color: var(--muted); max-width: 120mm; }
.hero { display:block; height: 118mm; border-radius: 6mm; margin: 6mm 0 0; }
.gallery { display:grid; grid-template-columns: 1fr 1fr; gap: 6mm 6mm; }
.gallery figure { margin:0; break-inside: avoid; }
.gallery img { width:100%; height: 92mm; object-fit: cover; border-radius: 4mm; display:block; }
.gallery figcaption { font-size: 9pt; color: var(--muted); margin-top: 1.5mm; }
.gallery figcaption b { color: var(--ink); font-family: "IBM Plex Mono", monospace; font-size: 8.5pt; letter-spacing: .06em; }
.feat { display:grid; grid-template-columns: 1fr 1fr; gap: 3mm 8mm; }
.feat div { border-top: 2px solid var(--ink); padding-top: 2mm; }
.feat b { display:block; font-family: Fraunces, serif; font-size: 12pt; }
.meta { font-family:"IBM Plex Mono", monospace; font-size: 8.5pt; color: var(--muted); }
table { width:100%; border-collapse: collapse; margin: 2mm 0 4mm; font-size: 9.5pt; }
th { text-align:left; font-size: 7.5pt; letter-spacing:.08em; text-transform:uppercase; color:var(--muted); border-bottom: 1.5px solid var(--ink); padding: 1.5mm 2mm; }
td { border-bottom: 1px solid var(--line); padding: 1.6mm 2mm; vertical-align: top; }
tr { break-inside: avoid; }
code, .mono { font-family: "IBM Plex Mono", monospace; font-size: 9pt; background: var(--soft); padding: 0 1.2mm; border-radius: 1mm; }
.dot { display:inline-block; width:2.6mm; height:2.6mm; border-radius:50%; margin-right:2mm; vertical-align: 0; }
.note { background: var(--soft); border-left: 3px solid var(--accent); padding: 3mm 4mm; margin: 3mm 0; break-inside: avoid; }
ol { padding-left: 5mm; margin: 0 0 3mm; } li { margin-bottom: 1.6mm; }
.diagram svg { width: 100%; height: auto; }
.modes { display:grid; grid-template-columns: 1fr 1fr; gap: 4mm; }
.mode { border: 1px solid var(--line); border-radius: 3mm; padding: 3mm 4mm; break-inside: avoid; }
.mode b { font-family: Fraunces, serif; font-size: 12pt; }
.ui { white-space: nowrap; }
.compact td { padding: 1mm 2mm; font-size: 9pt; }
.total td { font-weight: 600; border-bottom: 0; }
"""

GALLERY_IMGS = [("claude-working.jpg", "CLAUDE · WORKING"), ("claude-idle.jpg", "CLAUDE · IDLE"),
                ("clock.jpg", "CLOCK"), ("focus.jpg", "FOCUS")]


def ol(items):
    return "<ol>" + "".join(f"<li>{i}</li>" for i in items) + "</ol>"


def render(t):
    """Build the full HTML page for one language dict."""
    feat = "".join(f"<div><b>{h}</b>{d}</div>" for h, d in t["feat"])
    gallery = "".join(f'<figure><img src="img/{img}" alt=""><figcaption><b>{label}</b><br>{cap}</figcaption></figure>'
                      for (img, label), cap in zip(GALLERY_IMGS, t["gallery"]))
    bom = table([(a, f'<span class="mono">{b}</span>', c, d) for a, b, c, d in t["bom"]], t["bom_head"])
    modes = "".join(f'<div class="mode"><b>{n}</b><br>{d}</div>' for n, d in t["modes"])
    return f"""<!doctype html><html lang="{t['lang']}"><head><meta charset="utf-8"><title>{t['title']}</title>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Fraunces:opsz,wght@9..144,600;9..144,800&family=IBM+Plex+Sans:wght@400;600&family=IBM+Plex+Mono:wght@500&display=swap">
<style>{CSS}</style></head><body>

<div class="cover">
  <div>
    <div class="kicker">{t['kicker']}</div>
    <h1>Desk Buddy</h1>
    <p class="sub">{t['sub']}</p>
    <img class="hero" src="img/claude-working.jpg" alt="{t['hero_alt']}">
  </div>
  <div class="feat">{feat}</div>
  <p class="meta">{t['meta']}</p>
</div>

<section>
  <h2>{t['meet_h']}</h2>
  <p>{t['meet_p']}</p>
  <div class="gallery">{gallery}</div>
</section>

<section>
  <h2>{t['parts_h']}</h2>
  <p>{t['parts_p']}</p>
  {bom}
  <table><tbody><tr class="total"><td>{t['total'][0]}</td><td style="text-align:right">{t['total'][1]}</td></tr></tbody></table>
  <h3>{t['tools_h']}</h3>
  <p>{t['tools_p']}</p>
  <div class="note">{t['board_note']}</div>
</section>

<section>
  <h2>{t['wiring_h']}</h2>
  <p>{t['wiring_p']}</p>
  <div class="diagram">{wiring_svg(t)}</div>
</section>

<section>
  <h2>{t['conn_h']}</h2>
  {table(pin_rows(t), t['conn_head'], 'compact')}
  <div class="note">{t['no5v']}</div>
  <h3>{t['steps_h']}</h3>
  {ol(t['steps'])}
  <div class="note">{t['lcd_note']}</div>
</section>

<section>
  <h2>{t['inst_h']}</h2>
  <p>{t['inst_p']}</p>
  <p style="font-size:14pt"><b class="mono" style="font-size:12pt">{INSTALL_URL}</b></p>
  {ol(t['inst'])}
  <h3>{t['wifi_h']}</h3>
  {ol(t['wifi'])}
  <div class="note">{t['wifi_note']}</div>
  <p>{t['src_p']}</p>
</section>

<section>
  <h2>{t['use_h']}</h2>
  <p>{t['use_p']}</p>
  <div class="modes">{modes}</div>
  <p style="margin-top:4mm">{t['sleep_p']}</p>

  <h3>{t['claude_h']}</h3>
  <p>{t['claude_p']}</p>
  {ol(t['claude'])}
  <p>{t['privacy_p']}</p>
</section>

<section>
  <h2>{t['trouble_h']}</h2>
  {table(t['trouble'], t['trouble_head'])}
  <h3>{t['next_h']}</h3>
  <p>{t['next_p']}</p>
  <p class="meta" style="margin-top:8mm">Desk Buddy v{VERSION} · {REPO_URL} · CC BY-NC 4.0</p>
</section>
</body></html>"""


if __name__ == "__main__":
    for t in (EN, ID):
        out = HERE / t["out"]
        out.write_text(render(t), encoding="utf-8")
        print(f"wrote {out}")
