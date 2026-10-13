#!/usr/bin/env python3
"""Generate panduan.html (Indonesian build guide) -> print to PDF with Chrome.

    python3 docs/guide/build_guide.py && docs/guide/make-pdf.sh
URLs below are the public repo / installer; update when publishing.
"""
import html, pathlib

REPO_URL = "https://github.com/julisman/desk-buddy"
INSTALL_URL = "https://julisman.github.io/desk-buddy/"
VERSION = "1.0.0"
OUT = pathlib.Path(__file__).with_name("panduan.html")

# ---------- Wiring diagram (SVG) ----------
ESP_PINS = ["3V3", "4", "5", "6", "7", "15", "9", "10", "11", "12", "13", "14", "G"]
C = {"pwr": "#d93025", "gnd": "#3c4043", "lcd": "#1a73e8", "enc": "#188038", "bz": "#b06000", "touch": "#a142f4"}
MODULES = [
    ("LCD 1.69\" ST7789", [("BLK", "9", "lcd"), ("CS", "10", "lcd"), ("DC", "11", "lcd"), ("RES", "12", "lcd"),
                           ("SDA", "13", "lcd"), ("SCL", "14", "lcd"), ("VCC", "3V3", "pwr"), ("GND", "G", "gnd")]),
    ("Knob KY-040", [("CLK", "4", "enc"), ("DT", "5", "enc"), ("SW", "6", "enc"), ("+", "3V3", "pwr"), ("GND", "G", "gnd")]),
    ("Buzzer KY-006", [("S", "15", "bz"), ("+ (tengah)", "3V3", "pwr"), ("−", "G", "gnd")]),
    ("Touch pad", [("kabel / copper tape", "7", "touch")]),
]

def wiring_svg():
    W, row = 760, 30
    esp_x, esp_y = 40, 50
    esp_pin_y = {p: esp_y + 26 + i * row for i, p in enumerate(ESP_PINS)}
    esp_h = len(ESP_PINS) * row + 30
    mod_h = 30 + sum(len(p) * row + 34 + 16 for _, p in MODULES)
    out = [f'<svg viewBox="0 0 {W} {max(esp_h + esp_y, mod_h) + 10}" xmlns="http://www.w3.org/2000/svg" font-family="IBM Plex Mono, monospace">']
    out.append(f'<rect x="{esp_x}" y="{esp_y}" width="120" height="{esp_h}" rx="10" fill="#202124"/>')
    out.append(f'<text x="{esp_x + 60}" y="{esp_y - 14}" text-anchor="middle" font-size="13" fill="#5f6368">ESP32-S3 (header kiri)</text>')
    for p, y in esp_pin_y.items():
        out.append(f'<text x="{esp_x + 16}" y="{y + 4}" font-size="13" fill="#fff">{"GPIO " + p if p.isdigit() else p}</text>')
    lines, boxes = [], []
    my = 30
    mx = 470
    lane = 0
    for name, pins in MODULES:
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

BOM = [
    ("ESP32-S3 N16R8 DevKitC (2 port USB-C)", "esp32 s3 n16r8", "Rp85–110rb", "Harus N16R8 (flash 16MB, PSRAM 8MB)"),
    ("LCD 1.69\" ST7789 240x280 SPI", "lcd 1.69 st7789", "Rp40–55rb", "Yang ada pin BLK"),
    ("Rotary encoder KY-040", "ky-040 rotary encoder", "Rp6–10rb", "Knob putar + tekan"),
    ("Buzzer pasif KY-006", "ky-006 buzzer pasif", "Rp4–7rb", "Modul 3 pin: S, +, −"),
    ("Breadboard 400 lubang", "breadboard 400", "Rp8–12rb", "Atau mini breadboard"),
    ("Kabel jumper female-male (20 pcs)", "kabel jumper female male", "Rp8–12rb", "Sinyal ke pin ESP32"),
    ("Kabel jumper male-male (20 pcs)", "kabel jumper male male", "Rp8–12rb", "Ke jalur power breadboard"),
    ("Kabel USB-C data", "kabel usb c data", "Rp15–25rb", "Bukan kabel cas saja"),
    ("Copper tape (opsional)", "copper tape", "Rp10–15rb", "Pad sentuh untuk dielus"),
]

TROUBLE = [
    ("Layar menyala tapi abu-abu / putih polos", "Kabel data LCD (SDA, SCL, CS, DC, RES) tidak nempel. Tancap ulang, miringkan sedikit header LCD di breadboard, tahan pakai selotip, tekan RST. Solder header LCD = solusi permanen."),
    ("Layar mati total", "VCC LCD harus ke 3V3, bukan 5V (pin 5V tidak ada listrik saat colok USB native)."),
    ("Board restart terus saat bunyi", "Cek kabel buzzer: S ke GPIO 15, + ke 3V3, − ke GND. Firmware sudah memakai volume kecil supaya aman."),
    ("Port tidak muncul saat install", "Ganti kabel USB-C (banyak yang cuma untuk cas). Coba port USB yang satunya. Atau tahan BOOT, tekan RST, lepas BOOT."),
    ("Knob kebalik / loncat 2", "Tukar kabel CLK dan DT. Kalau 1 klik = 2 langkah, beri tahu kami (setting firmware)."),
    ("Jam / cuaca tidak muncul", "WiFi belum tersambung. Tahan knob sambil menyalakan untuk setup ulang WiFi."),
    ("Elus tidak terasa", "Jangan sentuh pad saat Desk Buddy menyala (kalibrasi). Pakai kabel pendek dan pad lebih lebar (copper tape 2x2 cm)."),
]

def table(rows, head):
    th = "".join(f"<th>{h}</th>" for h in head)
    trs = "".join("<tr>" + "".join(f"<td>{c}</td>" for c in r) + "</tr>" for r in rows)
    return f"<table><thead><tr>{th}</tr></thead><tbody>{trs}</tbody></table>"

def pin_rows():
    rows = []
    for name, pins in MODULES:
        for label, esp, kind in pins:
            target = "GPIO " + esp if esp.isdigit() else ("3V3 (jalur + breadboard)" if esp == "3V3" else "GND (jalur − breadboard)")
            rows.append((f'<span class="dot" style="background:{C[kind]}"></span>{html.escape(name)}', html.escape(label), target))
    return rows

page = f"""<!doctype html><html lang="id"><head><meta charset="utf-8"><title>Desk Buddy — Panduan Rakit</title>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Fraunces:opsz,wght@9..144,600;9..144,800&family=IBM+Plex+Sans:wght@400;600&family=IBM+Plex+Mono:wght@500&display=swap">
<style>
@page {{ size: A4; margin: 16mm 16mm 18mm; }}
:root {{ --ink:#1d1f22; --muted:#5f6368; --accent:#e8193c; --line:#e3e3df; --soft:#f6f5f2; }}
* {{ box-sizing: border-box; }}
body {{ margin:0; color:var(--ink); font: 10.5pt/1.55 "IBM Plex Sans", system-ui, sans-serif; }}
h1,h2,h3 {{ font-family: Fraunces, Georgia, serif; line-height:1.15; margin:0; }}
h2 {{ font-size: 20pt; margin: 0 0 4mm; }}
h3 {{ font-size: 12.5pt; margin: 5mm 0 2mm; }}
p {{ margin: 0 0 3mm; }}
section {{ break-before: page; }}
.cover {{ height: 255mm; display:flex; flex-direction:column; justify-content:space-between; break-before:auto; }}
.cover .kicker {{ font-family:"IBM Plex Mono", monospace; color:var(--accent); letter-spacing:.12em; font-size:9pt; text-transform:uppercase; }}
.cover h1 {{ font-size: 54pt; font-weight:800; margin: 4mm 0; }}
.cover .sub {{ font-size: 14pt; color: var(--muted); max-width: 120mm; }}
.face {{ width: 120mm; height: 70mm; background:#0b0b0c; border-radius: 12mm; display:flex; align-items:center; justify-content:center; gap: 14mm; margin: 10mm 0; }}
.eye {{ width: 22mm; height: 30mm; background: var(--accent); border-radius: 7mm; }}
.feat {{ display:grid; grid-template-columns: 1fr 1fr; gap: 3mm 8mm; }}
.feat div {{ border-top: 2px solid var(--ink); padding-top: 2mm; }}
.feat b {{ display:block; font-family: Fraunces, serif; font-size: 12pt; }}
.meta {{ font-family:"IBM Plex Mono", monospace; font-size: 8.5pt; color: var(--muted); }}
table {{ width:100%; border-collapse: collapse; margin: 2mm 0 4mm; font-size: 9.5pt; }}
th {{ text-align:left; font-size: 7.5pt; letter-spacing:.08em; text-transform:uppercase; color:var(--muted); border-bottom: 1.5px solid var(--ink); padding: 1.5mm 2mm; }}
td {{ border-bottom: 1px solid var(--line); padding: 1.6mm 2mm; vertical-align: top; }}
tr {{ break-inside: avoid; }}
code, .mono {{ font-family: "IBM Plex Mono", monospace; font-size: 9pt; background: var(--soft); padding: 0 1.2mm; border-radius: 1mm; }}
.dot {{ display:inline-block; width:2.6mm; height:2.6mm; border-radius:50%; margin-right:2mm; vertical-align: 0; }}
.note {{ background: var(--soft); border-left: 3px solid var(--accent); padding: 3mm 4mm; margin: 3mm 0; break-inside: avoid; }}
ol {{ padding-left: 5mm; margin: 0 0 3mm; }} li {{ margin-bottom: 1.6mm; }}
.diagram svg {{ width: 100%; height: auto; }}
.modes {{ display:grid; grid-template-columns: 1fr 1fr; gap: 4mm; }}
.mode {{ border: 1px solid var(--line); border-radius: 3mm; padding: 3mm 4mm; break-inside: avoid; }}
.mode b {{ font-family: Fraunces, serif; font-size: 12pt; }}
.total td {{ font-weight: 600; border-bottom: 0; }}
</style></head><body>

<div class="cover">
  <div>
    <div class="kicker">Panduan rakit · v{VERSION}</div>
    <h1>Desk Buddy</h1>
    <p class="sub">Teman kecil di meja: wajah yang berkedip dan senang dielus, focus timer, jam &amp; cuaca, dan monitor Claude Code.</p>
    <div class="face"><div class="eye"></div><div class="eye"></div></div>
  </div>
  <div class="feat">
    <div><b>Wajah hidup</b>Berkedip, melirik, senang saat knob diputar atau pad dielus.</div>
    <div><b>Focus timer</b>Pomodoro 25/5 menit, wajah ikut serius saat fokus.</div>
    <div><b>Jam &amp; cuaca</b>Sinkron lewat internet, cuaca lokasi kamu.</div>
    <div><b>Monitor Claude Code</b>Context, limit sesi &amp; mingguan, alarm saat Claude butuh izin.</div>
  </div>
  <p class="meta">Firmware &amp; kode: {REPO_URL} · Installer: {INSTALL_URL}<br>Lisensi CC BY-NC 4.0: bebas untuk non-komersial dengan menyebut sumber.</p>
</div>

<section>
  <h2>Daftar komponen</h2>
  <p>Harga perkiraan Shopee (Oktober 2026). Kata kunci bisa langsung dicari di Shopee.</p>
  {table([(a, f'<span class="mono">{b}</span>', c, d) for a, b, c, d in BOM], ["Komponen", "Kata kunci", "Harga", "Catatan"])}
  <table><tbody><tr class="total"><td>Total perkiraan</td><td style="text-align:right">Rp185–260rb</td></tr></tbody></table>
  <h3>Alat</h3>
  <p>Laptop/PC dengan Google Chrome atau Microsoft Edge (untuk install firmware). Tidak perlu solder untuk versi breadboard ini. Opsional: solder untuk header LCD supaya kontaknya awet.</p>
  <div class="note"><b>Penting:</b> board harus <b>ESP32-S3 N16R8</b>. Varian lain (N8, N8R2, ESP32 biasa) memakai memori berbeda dan firmware ini tidak akan jalan.</div>
</section>

<section>
  <h2>Wiring</h2>
  <p>Semua sinyal ke header kiri ESP32-S3. Listrik dibagi lewat jalur breadboard: pin <b>3V3</b> ke jalur <b>+</b>, pin <b>G</b> ke jalur <b>−</b>.</p>
  <div class="diagram">{wiring_svg()}</div>
</section>

<section>
  <h2>Tabel sambungan</h2>
  {table(pin_rows(), ["Modul", "Pin modul", "Sambung ke"])}
  <div class="note"><b>Jangan pakai pin 5V.</b> Saat dicolok ke port USB native, pin 5V tidak ada listriknya. Semua modul pakai 3V3.</div>
  <h3>Langkah rakit</h3>
  <ol>
    <li>Cabut USB. Tancap LCD, knob, dan buzzer di breadboard, masing-masing di baris sendiri.</li>
    <li>Kabel male-male: pin <b>3V3</b> ESP32 ke jalur <b>+</b>, pin <b>G</b> ke jalur <b>−</b>. Lalu VCC/+ tiap modul ke jalur +, GND/− ke jalur −.</li>
    <li>Kabel female-male: ujung female ke pin ESP32, ujung male ke baris pin modul, sesuai tabel.</li>
    <li>Touch pad: satu kabel ke GPIO 7. Ujung satunya dibiarkan bebas, atau tempel copper tape 2x2 cm (bisa dari balik casing plastik).</li>
    <li>Cek sekali lagi: tidak ada kabel ke pin 5V, jalur + dan − tidak tertukar.</li>
  </ol>
  <div class="note">LCD tanpa solder: pin header LCD sering kurang nempel. Tancap sedikit miring dan tahan pakai selotip supaya pin menekan dinding lubang.</div>
</section>

<section>
  <h2>Install firmware</h2>
  <p>Tanpa install software. Buka di laptop (Chrome atau Edge):</p>
  <p style="font-size:14pt"><b class="mono" style="font-size:12pt">{INSTALL_URL}</b></p>
  <ol>
    <li>Colok ESP32-S3 ke laptop pakai kabel USB-C <b>data</b>, di port <b>USB</b> (bukan COM).</li>
    <li>Klik <b>Install Desk Buddy</b>, pilih port yang muncul (biasanya "USB JTAG/serial debug unit"), lalu <b>Install</b>. Tunggu sekitar 1 menit.</li>
    <li>Tekan tombol <b>RST</b> di board. Layar berkedip merah-hijau-biru lalu muncul wajah.</li>
  </ol>
  <h3>Sambungkan ke WiFi</h3>
  <ol>
    <li>Di bawah layar muncul nama hotspot, misalnya <span class="mono">DeskBuddy-1A2B</span>.</li>
    <li>Di HP, buka pengaturan WiFi dan sambung ke hotspot itu. Halaman setup terbuka sendiri; kalau tidak, buka <span class="mono">192.168.4.1</span>.</li>
    <li>Pilih WiFi rumah, isi password, pilih zona waktu (WIB/WITA/WIT), lalu <b>Simpan</b>. Desk Buddy restart dan tersambung.</li>
  </ol>
  <div class="note">Ganti WiFi / pindah rumah: tahan knob sambil menyalakan Desk Buddy sampai muncul "WiFi direset". Kalau WiFi tersimpan gagal tersambung, hotspot setup juga terbuka sendiri.</div>
  <p>Mau ubah kode? Source code lengkap ada di <span class="mono">{REPO_URL}</span> (PlatformIO).</p>
</section>

<section>
  <h2>Cara pakai</h2>
  <p><b>Tahan knob</b> (± 1 detik) untuk ganti mode. <b>Elus</b> pad sentuh kapan saja: wajah senang, keluar hati, bunyi "purr".</p>
  <div class="modes">
    <div class="mode"><b>FACE</b><br>Putar = dielus (senang, melirik). Tekan = boop (kaget).</div>
    <div class="mode"><b>FOCUS</b><br>Putar = atur menit. Tekan = mulai / jeda. Fokus 25 menit lalu istirahat 5 menit otomatis.</div>
    <div class="mode"><b>CLOCK</b><br>Jam, tanggal, dan cuaca lokasi kamu.</div>
    <div class="mode"><b>CLAUDE</b><br>Status Claude Code dari laptop. Putar = pindah sesi. Tekan = diamkan alarm.</div>
  </div>
  <p style="margin-top:4mm">Tidak disentuh 5 menit (malam 23:00–07:00: 1 menit): Desk Buddy tidur dan layar redup. Sentuh knob untuk membangunkan.</p>

  <h3>Mode CLAUDE (untuk pengguna Claude Code)</h3>
  <p>Desk Buddy menampilkan model, context %, limit sesi (5 jam) dan mingguan beserta jam reset, dan status: <b>WORKING</b>, <b>NEEDS YOU</b> (alarm + bingkai merah saat Claude minta izin), <b>DONE</b>. Bisa memantau sampai 4 sesi sekaligus.</p>
  <ol>
    <li>Laptop dan Desk Buddy di WiFi yang sama. Butuh <span class="mono">jq</span> (<span class="mono">brew install jq</span>).</li>
    <li>Download source code dari {REPO_URL}, lalu jalankan <span class="mono">tools/install-claude-hooks.sh</span>. Lepas lagi: <span class="mono">--uninstall</span>.</li>
    <li>Buka sesi Claude Code baru. Coba tanpa Claude: <span class="mono">tools/claude-desk-buddy.sh demo</span>.</li>
  </ol>
  <p>Yang dikirim ke Desk Buddy cuma angka dan status. Prompt dan balasan Claude tidak pernah keluar dari laptop.</p>
</section>

<section>
  <h2>Kalau ada masalah</h2>
  {table(TROUBLE, ["Gejala", "Solusi"])}
  <h3>Berikutnya</h3>
  <p><b>Phase 2:</b> baterai LiPo + charger Type-C (bisa dibawa ke mana-mana, ikon baterai) dan suara: ngobrol dengan Desk Buddy lewat mic dan speaker, dijawab AI dari laptop kamu.</p>
  <p class="meta" style="margin-top:8mm">Desk Buddy v{VERSION} · {REPO_URL} · CC BY-NC 4.0</p>
</section>
</body></html>"""

OUT.write_text(page, encoding="utf-8")
print(f"wrote {OUT}")
