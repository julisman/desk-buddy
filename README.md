# Desk Buddy

Teman kecil di meja: wajah animasi yang berkedip, melirik, senang kalau "dielus" (knob diputar atau pad sentuh), plus Pomodoro focus timer, jam + cuaca, dan monitor Claude Code.

**Status: Phase 1 selesai (v1.0.0).** Wajah + elus, FOCUS, CLOCK, CLAUDE (multi-sesi), setup WiFi lewat hotspot, web installer. Phase 2: baterai LiPo + charger, suara (mic + speaker, AI lewat laptop).

- **Install tanpa coding:** https://julisman.github.io/desk-buddy/ (web installer di `docs/`, ESP Web Tools), lihat [Rilis](#rilis-web-installer).
- **Panduan rakit (PDF, Bahasa Indonesia):** `docs/guide/Desk-Buddy-Panduan-Rakit.pdf` (komponen, wiring, install, cara pakai, troubleshooting).

Hardware: ESP32-S3-N16R8 DevKitC, display ST7789V3 1.69" 240x280 (SPI), rotary encoder KY-040, modul buzzer, touch pad.

## Struktur

```
src/core/        kode hardware bersama: board.h (pin & hardware), display, input (knob + touch), buzzer, face, net (WiFi/NTP/cuaca/setup)
src/desk_buddy/  app Desk Buddy: mode FACE/FOCUS/CLOCK/CLAUDE, focus timer, claude_link, app_config.h
tools/           script laptop: Claude Code hooks, rilis firmware
docs/            web installer (GitHub Pages) + panduan rakit PDF
```

Perbaikan driver (touch, buzzer, display) cukup di `core/`. Konstanta khusus app di `app_config.h`, bukan di `board.h`. App lain untuk hardware yang sama bisa ditambah di `src/<app>/` dengan env di file `*.app.ini` (otomatis dibaca `platformio.ini`).

## Mode

Tahan knob (≥ 0.6 detik) untuk ganti mode: **FACE → FOCUS → CLOCK → CLAUDE**. Elus pad sentuh (GPIO 7) kapan saja: wajah senang, hati melayang, bunyi purr.

| Mode | Putar knob | Tekan knob |
| --- | --- | --- |
| FACE | elus → wajah senang, melirik ke arah putaran | boop → kaget |
| FOCUS | set durasi (menit) saat belum jalan / pause | start / pause |
| CLOCK | elus | boop |
| CLAUDE | pindah sesi (kalau > 1) | diamkan alarm "butuh kamu" |

- **Focus:** default 25 menit fokus → istirahat 5 menit otomatis jalan → balik ke fokus (tekan untuk mulai lagi). Titik oranye di bawah = jumlah sesi fokus selesai. Selama fokus jalan, wajah ikut serius. Timer tetap jalan di mode lain; begitu selesai, layar pindah ke FOCUS dan buzzer bunyi.
- **Tidur:** tidak disentuh 5 menit (jam 23:00–07:00: 1 menit) → mata ngantuk, "zzz", layar redup. Sentuh knob untuk membangunkan (sentuhan pertama cuma membangunkan). Tidak tidur selama focus timer jalan.
- **Clock:** butuh WiFi (NTP + cuaca dari [Open-Meteo](https://open-meteo.com), tanpa API key). Tanpa WiFi, FACE dan FOCUS tetap jalan.

## Mode CLAUDE: status Claude Code

Desk Buddy menampilkan status Claude Code dari laptop: model, context %, limit sesi (5 jam) dan mingguan + jam reset, dan status kerja (WORKING / NEEDS YOU / DONE).

Beberapa sesi sekaligus (maks 4): titik di atas layar = satu sesi, warnanya status (coral working, merah butuh kamu, hijau done, abu idle). Putar knob untuk pindah sesi. Layar otomatis pindah ke sesi yang baru kamu kirimi prompt atau yang minta izin. Limit sesi/minggu milik akun, jadi sama untuk semua sesi. Sesi hilang saat ditutup (SessionEnd) atau 2 jam tanpa kabar. Di mode CLAUDE, tiap kabar dari Claude membangunkan layar dan me-reset timer tidur. Saat Claude minta izin: pindah ke mode CLAUDE, bunyi alarm (maks 3x, tekan knob = diam), bingkai merah berkedip. Tugas panjang (>= 20 detik) selesai: bunyi "chirp" + wajah senang.

### Cara kerja

```
Laptop                                              Desk Buddy (ESP32)
Claude Code                                         HTTP server di firmware
 ├─ status line ─┐                                  (src/desk_buddy/claude_link.cpp)
 └─ hooks ───────┴─► tools/claude-desk-buddy.sh ──WiFi──► POST http://deskbuddy.local/claude
                     (filter: angka & status saja)          -> layar mode CLAUDE
```

- **Server-nya ada di Desk Buddy**, jalan otomatis begitu WiFi connect. Di laptop **tidak ada server yang perlu dijalankan**: Claude Code sendiri yang memanggil script tiap ada update.
- Laptop dan Desk Buddy harus di **WiFi yang sama**. Desk Buddy mengumumkan diri sebagai `deskbuddy.local` (mDNS), jadi IP boleh berubah.
- Yang dikirim cuma: session id, nama folder project, nama model, persen, jam reset, nama event, permission mode, nama tool. Prompt, balasan Claude, isi file, dan path lengkap **tidak** dikirim.
- Desk Buddy mati / beda WiFi: kiriman timeout diam-diam di background, Claude Code tidak terpengaruh.

### Setup di laptop (sekali)

Butuh `jq` (`brew install jq`) dan script status line Claude Code yang membaca stdin ke `input=$(cat)`.

```sh
tools/install-claude-hooks.sh              # pasang hooks + 1 baris di script status line
tools/install-claude-hooks.sh --uninstall  # lepas lagi (backup: ~/.claude/*.bak-deskbuddy)
```

Yang diubah: `~/.claude/settings.json` (hooks async UserPromptSubmit, PostToolUse, Notification, Stop, SessionEnd) dan script status line kamu. Berlaku untuk semua sesi; hooks aktif di **sesi baru** (angka dari status line langsung jalan).

### Tes & debug

- Tanpa Claude Code: `tools/claude-desk-buddy.sh demo` (status -> working -> needs you -> done).
- Lihat data yang dipegang Desk Buddy: `curl -4 http://deskbuddy.local/claude`.
- Alamat lain: `DESK_BUDDY_URL=http://192.168.x.x/claude` (env var untuk script).
- `curl -4` wajib: tanpa itu macOS coba IPv6 dulu untuk `.local` dan nunggu ~5 detik.

## Wiring

### Display

| Pin LCD | ESP32-S3 |
| --- | --- |
| BLK | GPIO 9 |
| CS | GPIO 10 |
| DC | GPIO 11 |
| RES | GPIO 12 |
| SDA | GPIO 13 |
| SCL | GPIO 14 |
| VCC | **3V3** (lewat jalur + breadboard) |
| GND | G |

> **Jangan pakai pin 5V** kalau board dicolok lewat port USB native (yang dipakai untuk upload/log).
> Di board ini pin 5V cuma dapat listrik dari port USB native kalau pad **IN-OUT** disolder.
> Defaultnya terbuka, jadi 5V = 0 V dan LCD mati total.

Board ini terlalu lebar untuk breadboard (pin masuk di kolom a dan j, tidak ada lubang sisa),
jadi ESP32 dicabut dari breadboard: modul ditancap di breadboard, kabel female-to-male
dari baris modul ke pin ESP32.

### Encoder & buzzer

| Modul | Pin modul | Sambung ke |
| --- | --- | --- |
| Encoder KY-040 | GND | jalur − breadboard |
| | + | jalur + breadboard (3V3, bukan 5V: pull-up modul akan memberi 5V ke GPIO) |
| | SW | GPIO 6 |
| | DT | GPIO 5 |
| | CLK | GPIO 4 |
| Buzzer | S | GPIO 15 |
| | + (tengah) | jalur + breadboard |
| | − | jalur − breadboard |

Buzzer KY-006 (pasif, tanpa resistor) dibunyikan dengan duty kecil (`BUZZER_DUTY 16`). Di duty 50% arus puncaknya bikin board reset terus (reset reason 1). Mau lebih keras: pasang resistor 100–220 Ω seri antara GPIO 15 dan S, lalu naikkan `BUZZER_DUTY`.

### Touch pad ("elus-elus")

Pakai sensor sentuh bawaan ESP32-S3, tanpa modul. Sambungkan satu kabel ke **GPIO 7**; ujung satunya
jadi pad sentuh (untuk tes: sentuh logam ujung kabel male; nanti: aluminium foil / copper tape di dalam casing).
Selama disentuh, Desk Buddy senang, melihat ke atas dan keluar hati. **Jangan sentuh pad saat boot** (baseline dikalibrasi waktu boot).
Log `[status] ... touch=nilai/baseline` membantu menyetel `TOUCH_ON_RATIO` di `src/core/board.h`.

### Listrik (dibagi lewat jalur power breadboard)

Board cuma punya dua pin 3V3, padahal LCD, encoder dan buzzer semua butuh 3V3. Jadi:

- ESP32 **3V3** (pin paling atas) → jalur merah **+**, ESP32 **G** (pin paling bawah) → jalur biru **−** (kabel female-to-male).
- VCC/+ tiap modul → jalur +, GND/− tiap modul → jalur − (kabel male-to-male).

Diagram lengkap ada di halaman wiring (artifact "Desk Buddy LCD Wiring").

Pin display didefinisikan di `platformio.ini` (TFT_eSPI butuh build flag), pin lain di `src/core/board.h`.

## Setup WiFi

WiFi, zona waktu (WIB/WITA/WIT), dan lokasi cuaca diisi lewat HP, disimpan di flash:

1. Belum ada WiFi tersimpan: Desk Buddy membuka hotspot `DeskBuddy-XXXX` (nama tampil di bawah layar).
2. Sambung HP ke hotspot itu; halaman setup terbuka sendiri (atau buka `192.168.4.1`).
3. Pilih WiFi, isi password, Simpan. Desk Buddy restart dan tersambung.

- **Ganti WiFi:** tahan knob sambil menyalakan sampai muncul "WiFi direset".
- **WiFi tersimpan gagal 45 detik** (password salah / pindah rumah): hotspot setup terbuka sendiri. Kalau 3 menit tidak ada yang buka, restart dan coba WiFi tersimpan lagi.
- **Developer:** `src/secrets.h` (dari `secrets.h.example`) jadi nilai default kalau belum pernah setup, supaya tidak perlu setup ulang tiap flash. Build `desk-buddy-release` mengabaikannya.

## Build & flash (developer)

```sh
cp src/secrets.h.example src/secrets.h   # opsional: WiFi default untuk development
pio run -e desk-buddy -t upload
pio device monitor
```

Colok USB-C ke port **native USB** (kebaca sebagai `/dev/cu.usbmodem*`). Kalau upload gagal: tahan BOOT, tekan RST, lepas BOOT, upload lagi.

> PlatformIO di Mac ini ada di `~/.platformio/penv/bin/pio` (lihat README robot car soal Homebrew Python).

## Rilis (web installer)

```sh
# 1. naikkan APP_VERSION di src/desk_buddy/app_config.h
tools/make-release.sh        # build desk-buddy-release (tanpa secrets.h) -> web/firmware/deskbuddy-<versi>.bin + web/manifest.json
docs/guide/make-pdf.sh       # panduan PDF (URL repo/installer ada di docs/guide/build_guide.py)
```

`docs/` adalah halaman installer statis (ESP Web Tools: Chrome/Edge desktop, Web Serial), di-host GitHub Pages dari folder `docs/`: https://julisman.github.io/desk-buddy/ (PDF: `/guide/Desk-Buddy-Panduan-Rakit.pdf`). Script rilis menolak kalau SSID dari `secrets.h` ikut masuk ke firmware.

Repo publik (github.com/julisman/desk-buddy) diisi dari repo ini tanpa app privat:
```sh
tools/publish-public.sh "v1.0.1: ..."   # rsync ke ../desk-buddy-public (tanpa file privat & secrets.h) + cek + commit
cd ../desk-buddy-public && git push
```

Log yang normal:
```
=== Desk Buddy v1.0.0 ===
[boot] display test merah-hijau-biru selesai
[boot] ready (reset reason 1)
[wifi] connect ke "nama-wifi"
[wifi] connected, IP 192.168.x.x
[mdns] http://deskbuddy.local
[claude] server siap: POST http://deskbuddy.local/claude
[weather] 29.4 C, Partly cloudy
```
(`spiAttachMISO(): HSPI Does not have default pins` saat boot aman, abaikan.)

## Test setelah wiring

1. **Display:** setelah boot, wajah muncul (mata cyan), berkedip dan melirik sendiri. Layar fade-in, buzzer bunyi 3 nada naik.
2. **Encoder:** putar knob → mata senang + bunyi tick. Satu klik putar harus terasa satu langkah di FOCUS (25 → 26).
3. **Tombol:** tekan → wajah kaget. Tahan → ganti mode, label mode muncul di atas.
4. **Focus:** set 1 menit, tekan start, tunggu → melodi selesai, wajah senang, break 5 menit mulai.
5. **Clock:** jam + tanggal + suhu muncul (setelah WiFi connect, ~5–10 detik).
6. **Tidur:** biarkan 5 menit → mata ngantuk + zzz + layar redup. Putar knob → bangun.

## Tuning

| Gejala | Ubah |
| --- | --- |
| Warna kebalik (background putih, mata merah) | tambah `-DTFT_INVERSION_OFF=1` (atau `_ON`) di `platformio.ini` |
| Merah/biru tertukar | `TFT_RGB_ORDER` di `platformio.ini` (panel ini: `TFT_BGR`) |
| Gambar geser / terpotong | cek ukuran display: `TFT_HEIGHT` (platformio.ini) dan `SCREEN_H` (core/board.h), sekarang 280 |
| Display miring | `SCREEN_ROTATION` di `core/board.h` |
| 1 klik knob = 2 langkah, atau harus 2 klik untuk 1 langkah | `ENC_STEPS_PER_DETENT` (4 ↔ 2) |
| Arah putar kebalik | `ENC_REVERSE 1` |
| Buzzer cuma "klik" atau bunyi satu nada saja | itu active buzzer: `BUZZER_PASSIVE 0` |
| Buzzer bunyi terus saat diam | modul active-low: `BUZZER_ACTIVE_LOW 1` |
| Zona waktu | `TZ_INFO` di `core/board.h` (WIB-7 / WITA-8 / WIT-9) |

## Debug display

- Saat boot layar berkedip merah → hijau → biru. Kalau tidak, data SPI tidak sampai: cek kabel SDA/SCL/CS/DC/RES dan kontak pin header di lubang LCD.
- `pio run -e lcdtest -t upload`: tes warna terus-menerus pakai library lain (Adafruit ST7789), untuk memastikan masalahnya bukan di TFT_eSPI.
- `pio run -e pintest -t upload`: GPIO 9–14 kedip bareng. Pindahkan kabel data ke lubang BLK satu per satu; kalau backlight kedip, kabel itu benar.

## Catatan teknis

- Frame buffer 240x280 (134 KB) di PSRAM, digambar ulang tiap frame lalu di-push sekaligus → tanpa flicker, ~28 fps.
- WiFi/NTP/cuaca jalan di task sendiri di core 0, jadi request HTTP tidak bikin animasi patah.
- Log pakai `LOG()` (lihat `src/core/log.h`), bukan `Serial.printf`: di core 2.0.17 `Serial` di port USB native bisa salah deteksi "tidak tersambung" dan membuang semua output.

## Lisensi

[CC BY-NC 4.0](LICENSE): bebas dipakai, dibagikan, dan dimodifikasi untuk keperluan **non-komersial** dengan mencantumkan sumber. Untuk penggunaan komersial (misalnya menjual kit atau unit jadi), hubungi pembuat. Library pihak ketiga (TFT_eSPI, ArduinoJson, ESP Web Tools) memakai lisensinya masing-masing.
