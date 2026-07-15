# 📌 Wiring Guide & Sensor Calibration

Panduan lengkap sambungan kabel dan kalibrasi sensor untuk **Smart Plant Monitor v2.0**.
Complete wiring and sensor calibration guide for **Smart Plant Monitor v2.0**.

---

## 🗺️ Pin Mapping

### DHT11 (Temperature & Humidity Sensor)
| Pin DHT11 | Pin NodeMCU | Keterangan |
|---|---|---|
| VCC | 3V3 | Power 3.3V |
| DATA | D4 | Data signal |
| GND | G (GND) | Ground |

> Urutan pin fisik DHT11 dari atas: **VCC - DATA - GND**

---

### LCD 16x2 + I2C Module (alamat 0x27)
| Pin LCD I2C | Pin NodeMCU | Keterangan |
|---|---|---|
| GND | G (GND) | Ground |
| VCC | **VU** | Power 5V dari USB — WAJIB VU, bukan 3V3 |
| SDA | D5 | I2C Data |
| SCL | D6 | I2C Clock |

> ⚠️ **Penting:** LCD I2C butuh tegangan **5V** agar kontras optimal. Gunakan pin **VU** (bukan 3V3 atau VIN). Jika menggunakan 3V3, teks hanya terlihat sebagai bayangan samar.

> Urutan pin fisik LCD I2C dari kiri: **GND - VCC - SDA - SCL**

---

### Relay Module 1 Channel (SRD-05VDC-SL-C)
| Pin Relay | Pin NodeMCU | Keterangan |
|---|---|---|
| DC+ | **VU** | Power 5V dari USB |
| DC- | G (GND) | Ground |
| IN | D7 | Control signal dari NodeMCU |

> ℹ️ Sisi relay **NO, COM, NC** digunakan untuk menyambungkan perangkat yang dikontrol. Pada project ini relay digunakan sebagai **sinyal fisik** saat jadwal siram tiba (aktif 5 detik), bukan untuk mengontrol pompa atau perangkat listrik lainnya.

---

### Soil Moisture Sensor (FC-28)
| Pin Sensor | Pin NodeMCU | Keterangan |
|---|---|---|
| VCC | 3V3 | Power 3.3V |
| GND | G (GND) | Ground |
| A0 | A0 | Analog output — nilai kelembaban |
| D0 | *(tidak dipakai)* | Digital output, tidak digunakan |

> Urutan pin fisik Soil Moisture dari kiri: **AO - DO - GND - VCC**

---

## 📍 Posisi Pin di Board NodeMCU v3 Lolin

```
      ┌──────────────────────────────────────────────────┐
ATAS  │ VIN  G  RST  EN  3V  G  SK  SO  SC  S1  S2  S3  VU  G  A0 │
      │  •   •   •   •   •  •   •   •   •   •   •   •   •  •   •  │
      │              [ NodeMCU v3 Lolin ]                           │
      │  •   •   •   •   •  •   •   •   •   •   •   •   •  •   •  │
BAWAH │ 3V   G  TX  RX  D8  D7  D6  D5  G  3V  D4  D3  D2  D1  D0 │
      └──────────────────────────────────────────────────┘
                              ↑ USB / Micro-B
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
| A0 | Sisi atas | Soil Moisture analog output |

> ⚠️ **Jangan gunakan pin S1/S2/S3/SC/SO/SK** — pin-pin ini terhubung ke flash memory internal ESP8266 dan dapat menyebabkan crash/restart jika digunakan sebagai GPIO umum.

> ⚠️ **Pin VIN ≠ VU** — VIN adalah input eksternal (untuk power dari adaptor luar), bukan output 5V dari USB. Gunakan **VU** untuk mendapatkan 5V dari USB.

---

## ⚡ Catatan Power / Power Notes

| Pin | Tegangan | Digunakan untuk |
|---|---|---|
| 3V3 | 3.3V | DHT11, Soil Moisture Sensor |
| VU | 5V (dari USB) | LCD I2C, Relay Module |
| G (GND) | 0V | Semua komponen |

---

## 🔬 Kalibrasi Soil Moisture Sensor

Sensor kelembaban tanah menghasilkan nilai **analog (raw)** antara 0-1024. Nilai ini dikonversi ke persen (0-100%) menggunakan data kalibrasi dari pengukuran nyata.

### Hasil Kalibrasi

| Kondisi | Nilai Raw | Persentase |
|---|---|---|
| Sensor di udara / sangat kering | **1024** | 0% |
| Sensor terendam air penuh | **351** | 100% |

### Formula konversi di kode:
```cpp
kelembabanTanahPersen = map(kelembabanTanahRaw, 1024, 351, 0, 100);
kelembabanTanahPersen = constrain(kelembabanTanahPersen, 0, 100);
```

### Cara kalibrasi ulang jika diperlukan:
1. Upload sketch `test_sketches/TestSoilMoisture/TestSoilMoisture.ino`
2. Buka Serial Monitor (baud 115200)
3. Catat nilai raw saat sensor di udara → nilai kering
4. Celupkan probe ke air, catat nilai raw → nilai basah
5. Update di kode utama `smart_plant_monitor.ino`:
```cpp
kelembabanTanahPersen = map(kelembabanTanahRaw, NILAI_KERING, NILAI_BASAH, 0, 100);
```

---

## 🧪 Test Sketches

Sketch individual untuk testing tiap komponen sebelum integrasi:

| File | Fungsi |
|---|---|
| `test_sketches/TestDHT11/TestDHT11.ino` | Test baca suhu & kelembaban DHT11 via Serial Monitor |
| `test_sketches/TestLCD_D5D6/TestLCD_D5D6.ino` | Test tampilan LCD dengan pin SDA=D5, SCL=D6 |
| `test_sketches/TestSoilMoisture/TestSoilMoisture.ino` | Test baca nilai raw soil moisture sensor |
| `test_sketches/TestRelay/TestRelay.ino` | Test relay ON/OFF siklus 2 detik |
