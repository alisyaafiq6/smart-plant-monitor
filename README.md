# 🌱 Smart Climate Control System

> Sistem otomasi iklim berbasis NodeMCU v3 Lolin dengan web dashboard untuk monitoring dan kontrol kipas secara otomatis maupun manual.
>
> An IoT-based climate automation system using NodeMCU v3 Lolin, featuring a web dashboard for real-time monitoring and automatic/manual fan control.

---

## 📸 Demo

> *(Tambahkan foto/video sistem di sini / Add photos or videos of the system here)*

---

## 🇮🇩 Deskripsi

Sistem ini dirancang untuk memonitor kondisi lingkungan (suhu, kelembaban udara, kelembaban tanah) dan mengontrol kipas/exhaust fan secara otomatis berdasarkan threshold suhu yang bisa diatur. NodeMCU bertindak sebagai WiFi Access Point sekaligus web server, sehingga dashboard bisa diakses dari browser tanpa perlu koneksi internet.

## 🇬🇧 Description

This system monitors environmental conditions (temperature, air humidity, soil moisture) and automatically controls a fan/exhaust based on an adjustable temperature threshold. The NodeMCU acts as both a WiFi Access Point and a web server, making the dashboard accessible from any browser without an internet connection.

---

## ✨ Fitur / Features

| Fitur (ID) | Feature (EN) |
|---|---|
| Monitoring suhu & kelembaban udara real-time | Real-time temperature & air humidity monitoring |
| Monitoring kelembaban tanah | Soil moisture monitoring |
| Kontrol kipas otomatis berbasis suhu | Automatic fan control based on temperature |
| Threshold suhu bisa diatur dari dashboard | Adjustable temperature threshold via dashboard |
| Mode Manual: kontrol kipas langsung dari web | Manual mode: direct fan control from web |
| Tampilan data lokal di LCD 16x2 | Local data display on LCD 16x2 |
| Web dashboard tanpa internet (WiFi AP) | Internet-free web dashboard (WiFi AP mode) |
| Auto-refresh dashboard setiap 2 detik | Dashboard auto-refresh every 2 seconds |

---

## 🔧 Hardware

| Komponen / Component | Jumlah / Qty |
|---|---|
| NodeMCU v3 Lolin (ESP8266 / CH340C) | 1 |
| DHT11 Temperature & Humidity Sensor | 1 |
| LCD 16x2 + I2C Module (alamat 0x27) | 1 |
| Relay Module 1 Channel (SRD-05VDC-SL-C) | 1 |
| Soil Moisture Sensor (FC-28 / analog) | 1 |
| Breadboard | 1 |
| Kabel Jumper Male-to-Female / Jumper Wires | secukupnya |

---

## 📌 Wiring / Pin Mapping

Lihat file terpisah: **[WIRING.md](WIRING.md)**

---

## 💻 Software

### Library yang dibutuhkan / Required Libraries
Install via Arduino IDE → Tools → Manage Libraries:

| Library | Author |
|---|---|
| `LiquidCrystal I2C` | Frank de Brabander |
| `DHT sensor library` | Adafruit |
| `Adafruit Unified Sensor` | Adafruit *(auto-install saat install DHT)* |
| `ESP8266WiFi` | ESP8266 Community *(include dalam board package)* |
| `ESP8266WebServer` | ESP8266 Community *(include dalam board package)* |

### Board Setup
1. Tambahkan URL board ESP8266 di Arduino IDE:
   `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
2. Install board: **ESP8266 Community**
3. Pilih board: **NodeMCU 1.0 (ESP-12E Module)**

---

## 🚀 Cara Pakai / How to Use

### Upload Kode / Upload Code
1. Clone atau download repository ini
2. Buka `greenhouse_climate_control.ino` di Arduino IDE
3. Pastikan board dan port sudah benar
4. Upload ke NodeMCU (Ctrl+U)

### Akses Dashboard / Access Dashboard
1. Hubungkan HP/laptop ke WiFi **"GreenhouseAP"**
   - Password: `greenhouse123`
2. Buka browser, ketik: `192.168.4.1`
3. Dashboard akan muncul secara otomatis

### Cara Baca LCD / Reading the LCD
LCD bergantian menampilkan 2 halaman setiap 3 detik:

**Halaman 1:**
```
Suhu: 29.8°C
Udara: 70%
```

**Halaman 2:**
```
Tanah: 45%
Kipas:ON [A]
```

Keterangan:
- `[A]` = Mode Auto (kipas dikontrol otomatis berdasar suhu)
- `[M]` = Mode Manual (kipas dikontrol dari dashboard)

---

## ⚙️ Konfigurasi / Configuration

Ubah nilai-nilai ini di bagian atas kode sesuai kebutuhan:

```cpp
// WiFi Access Point
const char* ap_ssid = "GreenhouseAP";    // Nama WiFi
const char* ap_password = "greenhouse123"; // Password WiFi

// Threshold default
float thresholdSuhu = 30.0; // Suhu (°C) buat nyalain kipas otomatis
```

---

## 🔩 Kalibrasi Sensor Tanah / Soil Sensor Calibration

Lihat file terpisah: **[WIRING.md](WIRING.md)**

---

## 📁 Struktur File / File Structure

```
smart-climate-control/
├── greenhouse_climate_control.ino  # Kode utama / Main code
├── README.md                       # Dokumentasi ini / This documentation
├── WIRING.md                       # Panduan wiring & kalibrasi / Wiring & calibration guide
└── test_sketches/                  # Sketch testing per komponen / Per-component test sketches
    ├── TestDHT11.ino
    ├── TestLCD_D5D6.ino
    ├── TestSoilMoisture.ino
    └── TestRelay.ino
```

---

## 🛠️ Troubleshooting

| Masalah / Problem | Solusi / Solution |
|---|---|
| LCD blank / tidak ada tulisan | Cek apakah VCC LCD tersambung ke pin **VU** (bukan 3V3). Putar trimpot kontras pelan-pelan |
| LCD tidak terdeteksi (I2C scanner kosong) | Cek wiring SDA/SCL, pastikan SDA→D5 dan SCL→D6 tidak tertukar |
| DHT11 baca `nan` | Cek kabel DATA ke D4, pastikan VCC di 3V3 |
| Relay tidak klik | Cek VCC relay ke VU (butuh 5V), pastikan IN→D7 |
| Port COM tidak muncul | Install driver CH340C |
| Arduino IDE hang di logo | Jalankan sebagai Administrator (Run as Administrator) |
| WiFi "GreenhouseAP" tidak muncul | Cek Serial Monitor di baud 115200, pastikan muncul "AP IP address: 192.168.4.1" |

---

## 📝 Catatan Pengembangan / Development Notes

- Board ESP8266 pin **S1/S2/S3/SC/SO/SK** tidak aman digunakan sebagai GPIO umum karena terhubung ke flash memory internal
- Pin **VU** di NodeMCU v3 Lolin menghasilkan 5V langsung dari USB — lebih stabil untuk LCD dan Relay dibanding VIN
- Hysteresis 2°C diterapkan pada kontrol kipas untuk mencegah relay on/off berulang saat suhu di sekitar threshold

---

## 🏁 Status Project / Project Status

**🇮🇩** Project ini berstatus **FINAL — SELESAI**. Tidak ada penambahan fitur, upgrade, atau perubahan apapun yang direncanakan pada repository ini. Semua yang ada di sini adalah hasil akhir dari prototype pertama.

Jika di masa depan ada pengembangan lebih lanjut, penelitian ulang, atau versi yang lebih baik — hal tersebut akan dilakukan sebagai **project baru yang terpisah**, dengan repository baru, komponen baru, dan build dari awal. Repository ini akan tetap dipertahankan sebagai arsip prototype pertama apa adanya.

**🇬🇧** This project is **FINAL — COMPLETE**. No new features, upgrades, or changes are planned for this repository. Everything here represents the final result of the first prototype.

If future development, further research, or an improved version is pursued — it will be done as a **separate new project**, with a new repository, new components, and a fresh build from scratch. This repository will be preserved as an archive of the first prototype as-is.

---

## 👤 Author

> *(Tambahkan nama dan info kontakmu di sini / Add your name and contact info here)*

---

## 📄 License

MIT License — bebas digunakan dan dimodifikasi dengan tetap mencantumkan kredit.
MIT License — free to use and modify with proper attribution.
