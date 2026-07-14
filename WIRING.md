# 📌 Wiring Guide & Sensor Calibration

Panduan lengkap sambungan kabel dan kalibrasi sensor untuk **Smart Climate Control System**.
Complete wiring and sensor calibration guide for the **Smart Climate Control System**.

---

## 🗺️ Pin Mapping

### DHT11 (Temperature & Humidity Sensor)
| Pin DHT11 | Pin NodeMCU | Keterangan |
|---|---|---|
| VCC | 3V3 | Power 3.3V |
| DATA | D4 | Data signal |
| GND | G (GND) | Ground |

### LCD 16x2 + I2C Module
| Pin LCD I2C | Pin NodeMCU | Keterangan |
|---|---|---|
| GND | G (GND) | Ground |
| VCC | **VU** | Power 5V dari USB — WAJIB VU, bukan 3V3 |
| SDA | D5 | I2C Data |
| SCL | D6 | I2C Clock |

> ⚠️ **Penting:** LCD I2C butuh tegangan 5V agar kontras optimal. Gunakan pin **VU** (bukan 3V3 atau VIN).
> ⚠️ **Important:** The LCD I2C requires 5V for optimal contrast. Use the **VU** pin (not 3V3 or VIN).

### Relay Module 1 Channel (SRD-05VDC-SL-C)
| Pin Relay | Pin NodeMCU | Keterangan |
|---|---|---|
| DC+ | **VU** | Power 5V dari USB |
| DC- | G (GND) | Ground |
| IN | D7 | Control signal dari NodeMCU |

> ℹ️ Pin **NO, COM, NC** di sisi relay digunakan untuk menyambungkan perangkat yang dikontrol (misalnya kipas). Untuk testing logic saja, pin ini bisa dibiarkan kosong.

### Soil Moisture Sensor
| Pin Sensor | Pin NodeMCU | Keterangan |
|---|---|---|
| VCC | 3V3 | Power 3.3V |
| GND | G (GND) | Ground |
| A0 | A0 | Analog output — nilai kelembaban |
| D0 | *(tidak dipakai)* | Digital output, tidak digunakan |

---

## 📍 Posisi Pin di Board NodeMCU v3 Lolin

```
         ┌─────────────────────────────────────────────┐
SISI     │ VIN  G  RST  EN  3V   G  SK  SO  SC  S1  S2  S3  VU   G  A0 │
ATAS ──▶ │  •   •   •   •   •   •   •   •   •   •   •   •   •   •   •  │
         │                   [NodeMCU v3 Lolin]                          │
SISI     │  •   •   •   •   •   •   •   •   •   •   •   •   •   •   •  │
BAWAH──▶ │ 3V   G  TX  RX  D8  D7  D6  D5   G  3V  D4  D3  D2  D1  D0 │
         └─────────────────────────────────────────────┘
                                        ↑USB
```

**Pin yang dipakai di project ini:**

| Pin | Lokasi | Digunakan untuk |
|---|---|---|
| D4 | Sisi bawah | DHT11 DATA |
| D5 | Sisi bawah | LCD SDA |
| D6 | Sisi bawah | LCD SCL |
| D7 | Sisi bawah | Relay IN |
| G (GND) | Sisi bawah | GND semua komponen |
| 3V3 | Sisi bawah | VCC DHT11 + Soil Moisture |
| VU | Sisi atas | VCC LCD + Relay (5V dari USB) |
| A0 | Sisi atas | Soil Moisture A0 |

> ⚠️ **Jangan pakai pin S1/S2/S3/SC/SO/SK** — pin-pin ini terhubung ke flash memory internal ESP8266 dan dapat menyebabkan crash/restart bila digunakan sebagai GPIO umum.

---

## 🔬 Kalibrasi Soil Moisture Sensor

Sensor kelembaban tanah menghasilkan nilai **analog (raw)** antara 0-1024. Nilai ini perlu dikonversi ke persen (0-100%) menggunakan data kalibrasi dari sensor aslinya.

### Hasil Kalibrasi (sensor ini)

| Kondisi | Nilai Raw | Persentase |
|---|---|---|
| Sensor di udara / sangat kering | **1024** | 0% |
| Sensor terendam air penuh | **351** | 100% |

### Cara kalibrasi ulang jika diperlukan:
1. Upload sketch `TestSoilMoisture.ino`
2. Buka Serial Monitor (baud 115200)
3. Catat nilai raw saat sensor di udara (angka tertinggi = nilai kering)
4. Celupkan probe ke air, catat nilai raw saat terendam (angka terendah = nilai basah)
5. Update nilai di kode utama:

```cpp
kelembabanTanahPersen = map(kelembabanTanahRaw, NILAI_KERING, NILAI_BASAH, 0, 100);
```

Contoh dengan data kalibrasi di atas:
```cpp
kelembabanTanahPersen = map(kelembabanTanahRaw, 1024, 351, 0, 100);
```

---

## ⚡ Catatan Power / Power Notes

| Sumber | Tegangan | Digunakan untuk |
|---|---|---|
| 3V3 | 3.3V | DHT11, Soil Moisture Sensor |
| VU | 5V (dari USB) | LCD I2C, Relay Module |
| G (GND) | 0V | Semua komponen |

> NodeMCU v3 Lolin pin **VU** menyalurkan tegangan 5V langsung dari USB power, berbeda dengan pin **VIN** yang merupakan input eksternal (butuh tegangan lebih tinggi untuk diregulasi). Pastikan NodeMCU mendapat power dari USB saat menggunakan pin VU sebagai sumber.

---

## 🧪 Test Sketches

Tersedia sketch individual untuk testing tiap komponen sebelum integrasi:

| File | Fungsi |
|---|---|
| `TestDHT11.ino` | Test baca suhu & kelembaban DHT11 via Serial Monitor |
| `TestLCD_D5D6.ino` | Test tampilan LCD dengan pin D5/D6 |
| `TestSoilMoisture.ino` | Test baca nilai raw soil moisture sensor |
| `TestRelay.ino` | Test relay ON/OFF siklus 2 detik |
