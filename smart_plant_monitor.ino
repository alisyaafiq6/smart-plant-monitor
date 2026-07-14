/*
 * SMART PLANT MONITOR v2.0
 * NodeMCU v3 Lolin
 *
 * Komponen:
 * - DHT11        : sensor suhu & kelembaban udara (pin D4)
 * - Soil Moisture: monitoring kelembaban tanah (pin A0)
 * - LCD I2C 16x2 : tampilan lokal (SDA=D5, SCL=D6, VCC=VU)
 * - Relay 1ch    : sinyal fisik saat jadwal siram tiba (pin D7, VCC=VU)
 *
 * WiFi : WiFiManager (setup lewat browser, fleksibel pindah lokasi)
 * NTP  : sync waktu otomatis dari internet
 * Akses: http://<IP yang didapat dari router>
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>

// ===== PIN =====
#define DHTPIN    D4
#define DHTTYPE   DHT11
#define SOIL_PIN  A0
#define RELAY_PIN D7

// ===== OBJEK =====
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
ESP8266WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000); // UTC+7 (WIB)

// ===== VARIABEL SENSOR =====
float suhu = 0;
float kelembabanUdara = 0;
int kelembabanTanahRaw = 0;
int kelembabanTanahPersen = 0;

// ===== VARIABEL SIRAM =====
// Mode: "sensor" atau "jadwal"
String modeSiram = "sensor";
int thresholdTanah = 40; // persen — kalau tanah < 40%, rekomendasikan siram
bool relayAktif = false;
unsigned long relayMulai = 0;
const long relayDurasi = 5000; // relay nyala 5 detik sebagai sinyal

// ===== VARIABEL JADWAL =====
// Maksimal 5 jadwal
const int MAX_JADWAL = 5;
int jadwalJam[MAX_JADWAL]   = {-1, -1, -1, -1, -1};
int jadwalMenit[MAX_JADWAL] = {-1, -1, -1, -1, -1};
bool jadwalSudahTrigger[MAX_JADWAL] = {false, false, false, false, false};

// ===== VARIABEL STATUS =====
String statusSiram = "Mengecek...";
bool wifiTerhubung = false;
String jamSekarang = "--:--";

// ===== TIMER =====
unsigned long lastSensorRead  = 0;
unsigned long lastLcdUpdate   = 0;
unsigned long lastNTPUpdate   = 0;
const long sensorInterval = 2000;
const long lcdInterval    = 3000;
const long ntpInterval    = 30000;
int lcdPage = 0;

// ===== EEPROM untuk simpan jadwal =====
#define EEPROM_SIZE 64
void simpanJadwal() {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < MAX_JADWAL; i++) {
    EEPROM.write(i * 2,     jadwalJam[i]   + 1); // +1 biar -1 jadi 0
    EEPROM.write(i * 2 + 1, jadwalMenit[i] + 1);
  }
  EEPROM.commit();
  EEPROM.end();
}

void muatJadwal() {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < MAX_JADWAL; i++) {
    jadwalJam[i]   = (int)EEPROM.read(i * 2)     - 1;
    jadwalMenit[i] = (int)EEPROM.read(i * 2 + 1) - 1;
  }
  EEPROM.end();
}

void setup() {
  Serial.begin(115200);

  // Inisialisasi relay (default OFF)
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // Inisialisasi sensor
  dht.begin();

  // Inisialisasi LCD
  Wire.begin(D5, D6);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Plant Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Connecting...");

  // Muat jadwal dari EEPROM
  muatJadwal();

  // WiFiManager
  WiFiManager wifiManager;
  wifiManager.setAPCallback([](WiFiManager* wm) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Setup WiFi:");
    lcd.setCursor(0, 1);
    lcd.print("PlantSetup");
  });

  if (!wifiManager.autoConnect("PlantSetup")) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Gagal konek!");
    lcd.setCursor(0, 1);
    lcd.print("Restart...");
    delay(3000);
    ESP.restart();
  }

  // WiFi berhasil
  wifiTerhubung = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi OK!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);

  // NTP
  timeClient.begin();
  timeClient.update();

  // Web server routes
  server.on("/",          handleRoot);
  server.on("/data",      handleData);
  server.on("/mode",      handleMode);
  server.on("/threshold", handleThreshold);
  server.on("/jadwal",    handleJadwal);
  server.on("/hapusjadwal", handleHapusJadwal);
  server.on("/resetwifi", handleResetWifi);
  server.begin();

  lcd.clear();
  Serial.println("Sistem siap!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();

  unsigned long now = millis();

  // Baca sensor
  if (now - lastSensorRead >= sensorInterval) {
    lastSensorRead = now;
    bacaSensor();
  }

  // Update NTP
  if (now - lastNTPUpdate >= ntpInterval) {
    lastNTPUpdate = now;
    timeClient.update();
  }

  // Update waktu & cek jadwal
  jamSekarang = timeClient.getFormattedTime().substring(0, 5); // "HH:MM"
  int jamNow   = timeClient.getHours();
  int menitNow = timeClient.getMinutes();
  cekJadwal(jamNow, menitNow);

  // Update status siram
  updateStatusSiram();

  // Kontrol relay (matiin setelah durasi)
  if (relayAktif && (now - relayMulai >= relayDurasi)) {
    relayAktif = false;
    digitalWrite(RELAY_PIN, LOW);
  }

  // Update LCD
  if (now - lastLcdUpdate >= lcdInterval) {
    lastLcdUpdate = now;
    updateLCD();
  }
}

void bacaSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h) && !isnan(t)) {
    suhu = t;
    kelembabanUdara = h;
  }
  kelembabanTanahRaw = analogRead(SOIL_PIN);
  kelembabanTanahPersen = map(kelembabanTanahRaw, 1024, 351, 0, 100);
  kelembabanTanahPersen = constrain(kelembabanTanahPersen, 0, 100);
}

void updateStatusSiram() {
  if (modeSiram == "sensor") {
    if (kelembabanTanahPersen < thresholdTanah) {
      statusSiram = "Siram Sekarang!";
    } else {
      statusSiram = "Jangan Siram";
    }
  }
  // Kalau mode jadwal, statusSiram diupdate di cekJadwal()
}

void cekJadwal(int jam, int menit) {
  if (modeSiram != "jadwal") return;

  bool adaJadwalAktif = false;
  for (int i = 0; i < MAX_JADWAL; i++) {
    if (jadwalJam[i] < 0) continue;

    if (jam == jadwalJam[i] && menit == jadwalMenit[i]) {
      adaJadwalAktif = true;
      if (!jadwalSudahTrigger[i]) {
        jadwalSudahTrigger[i] = true;
        statusSiram = "Waktunya Siram!";
        // Nyalakan relay sebagai sinyal fisik
        relayAktif = true;
        relayMulai = millis();
        digitalWrite(RELAY_PIN, HIGH);
      }
    } else {
      jadwalSudahTrigger[i] = false; // reset biar bisa trigger lagi besok
    }
  }

  if (!adaJadwalAktif && modeSiram == "jadwal") {
    statusSiram = "Jangan Siram";
  }
}

void updateLCD() {
  lcd.clear();
  if (lcdPage == 0) {
    // Halaman 1: Suhu & Kelembaban Udara
    lcd.setCursor(0, 0);
    lcd.print("Suhu: ");
    lcd.print(suhu, 1);
    lcd.print((char)223);
    lcd.print("C");
    lcd.setCursor(0, 1);
    lcd.print("Udara: ");
    lcd.print(kelembabanUdara, 0);
    lcd.print("%");
  } else if (lcdPage == 1) {
    // Halaman 2: Kelembaban Tanah & Waktu
    lcd.setCursor(0, 0);
    lcd.print("Tanah: ");
    lcd.print(kelembabanTanahPersen);
    lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("Jam: ");
    lcd.print(jamSekarang);
  } else {
    // Halaman 3: Status Siram
    lcd.setCursor(0, 0);
    lcd.print("Status:");
    lcd.setCursor(0, 1);
    // Potong teks kalau lebih dari 16 karakter
    String s = statusSiram;
    if (s.length() > 16) s = s.substring(0, 16);
    lcd.print(s);
  }
  lcdPage = (lcdPage + 1) % 3;
}

// ===== WEB SERVER HANDLERS =====

void handleRoot() {
  server.send(200, "text/html", getHTML());
}

void handleData() {
  // Susun daftar jadwal sebagai JSON array
  String jadwalJSON = "[";
  for (int i = 0; i < MAX_JADWAL; i++) {
    if (jadwalJam[i] >= 0) {
      if (jadwalJSON != "[") jadwalJSON += ",";
      jadwalJSON += "{\"id\":" + String(i) + ",\"jam\":" + String(jadwalJam[i]) + ",\"menit\":" + String(jadwalMenit[i]) + "}";
    }
  }
  jadwalJSON += "]";

  String json = "{";
  json += "\"suhu\":"              + String(suhu, 1)              + ",";
  json += "\"kelembabanUdara\":"   + String(kelembabanUdara, 1)   + ",";
  json += "\"kelembabanTanah\":"   + String(kelembabanTanahPersen) + ",";
  json += "\"statusSiram\":\""     + statusSiram                  + "\",";
  json += "\"modeSiram\":\""       + modeSiram                    + "\",";
  json += "\"threshold\":"         + String(thresholdTanah)       + ",";
  json += "\"jam\":\""             + jamSekarang                  + "\",";
  json += "\"jadwal\":"            + jadwalJSON;
  json += "}";
  server.send(200, "application/json", json);
}

void handleMode() {
  if (server.hasArg("mode")) {
    modeSiram = server.arg("mode");
    if (modeSiram == "sensor") {
      statusSiram = "Mengecek...";
    } else {
      statusSiram = "Jangan Siram";
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleThreshold() {
  if (server.hasArg("value")) {
    thresholdTanah = server.arg("value").toInt();
  }
  server.send(200, "text/plain", "OK");
}

void handleJadwal() {
  if (server.hasArg("jam") && server.hasArg("menit")) {
    int j = server.arg("jam").toInt();
    int m = server.arg("menit").toInt();
    // Cari slot kosong
    for (int i = 0; i < MAX_JADWAL; i++) {
      if (jadwalJam[i] < 0) {
        jadwalJam[i]   = j;
        jadwalMenit[i] = m;
        break;
      }
    }
    simpanJadwal();
  }
  server.send(200, "text/plain", "OK");
}

void handleHapusJadwal() {
  if (server.hasArg("id")) {
    int id = server.arg("id").toInt();
    if (id >= 0 && id < MAX_JADWAL) {
      jadwalJam[id]   = -1;
      jadwalMenit[id] = -1;
      jadwalSudahTrigger[id] = false;
      simpanJadwal();
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleResetWifi() {
  server.send(200, "text/html",
    "<html><body style='background:#0d1f17;color:#e8f5e9;font-family:sans-serif;"
    "display:flex;align-items:center;justify-content:center;height:100vh;text-align:center'>"
    "<div><h2>🔄 Mereset WiFi...</h2>"
    "<p style='color:#a5d6a7;margin-top:10px'>NodeMCU akan restart.<br>"
    "Sambungkan HP ke WiFi <b>PlantSetup</b> untuk setup ulang.</p></div></body></html>"
  );
  delay(1000);
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  ESP.restart();
}


String getHTML() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>PLANT MONITOR SYS</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Rajdhani:wght@400;600;700&display=swap');

:root {
  --c:  #00ffe7;
  --c2: #00a8ff;
  --c3: #ff4d6d;
  --bg: #020d1a;
  --bg2: #040f1f;
  --dim: rgba(0,255,231,0.06);
  --border: rgba(0,255,231,0.15);
}

* { margin:0; padding:0; box-sizing:border-box; }

body {
  font-family: 'Rajdhani', sans-serif;
  background: var(--bg);
  color: var(--c);
  min-height: 100vh;
  overflow-x: hidden;
}

/* Grid background */
body::before {
  content:'';
  position:fixed; top:0; left:0; right:0; bottom:0;
  background-image:
    linear-gradient(rgba(0,255,231,0.03) 1px, transparent 1px),
    linear-gradient(90deg, rgba(0,255,231,0.03) 1px, transparent 1px);
  background-size: 40px 40px;
  pointer-events:none; z-index:0;
}

.wrap {
  position:relative; z-index:1;
  max-width: 500px;
  margin: 0 auto;
  padding: 20px 16px 40px;
}

/* HEADER */
.header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  margin-bottom: 24px;
  padding-bottom: 14px;
  border-bottom: 1px solid var(--border);
}
.header-left {}
.sys-label {
  font-family: 'Share Tech Mono', monospace;
  font-size: 0.65em;
  color: rgba(0,255,231,0.4);
  letter-spacing: 3px;
  text-transform: uppercase;
}
.sys-title {
  font-size: 1.4em;
  font-weight: 700;
  letter-spacing: 2px;
  color: var(--c);
  text-shadow: 0 0 20px rgba(0,255,231,0.5);
  line-height: 1.1;
}
.header-right { text-align:right; }
.jam-display {
  font-family: 'Share Tech Mono', monospace;
  font-size: 1.6em;
  color: var(--c2);
  text-shadow: 0 0 14px rgba(0,168,255,0.6);
  letter-spacing: 2px;
}
.status-dot {
  display: inline-block;
  width: 7px; height: 7px;
  border-radius: 50%;
  background: #00ff88;
  box-shadow: 0 0 8px #00ff88;
  margin-right: 6px;
  animation: pulse 1.5s infinite;
}
@keyframes pulse {
  0%,100% { opacity:1; } 50% { opacity:0.3; }
}
.online-label {
  font-family: 'Share Tech Mono', monospace;
  font-size: 0.65em;
  color: rgba(0,255,136,0.7);
  letter-spacing: 2px;
}

/* STATUS SIRAM — paling atas, paling dominan */
.status-box {
  background: var(--dim);
  border: 1px solid var(--border);
  border-radius: 4px;
  padding: 18px 20px;
  margin-bottom: 16px;
  position: relative;
  overflow: hidden;
}
.status-box::before {
  content:'';
  position:absolute; top:0; left:0;
  width:3px; height:100%;
  background: var(--c);
  box-shadow: 0 0 12px var(--c);
}
.status-tag {
  font-family: 'Share Tech Mono', monospace;
  font-size: 0.6em;
  color: rgba(0,255,231,0.4);
  letter-spacing: 3px;
  margin-bottom: 6px;
}
.status-value {
  font-size: 1.8em;
  font-weight: 700;
  letter-spacing: 1px;
}
.status-siram  { color: var(--c3); text-shadow: 0 0 16px rgba(255,77,109,0.6); }
.status-jangan { color: #00ff88;   text-shadow: 0 0 16px rgba(0,255,136,0.6); }
.status-waktu  { color: #ffd166;   text-shadow: 0 0 16px rgba(255,209,102,0.6); }

/* SENSOR GRID */
.sensor-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
  margin-bottom: 16px;
}
.sensor-card {
  background: var(--dim);
  border: 1px solid var(--border);
  border-radius: 4px;
  padding: 14px;
  position: relative;
}
.sensor-card.full { grid-column: 1 / -1; }
.s-tag {
  font-family: 'Share Tech Mono', monospace;
  font-size: 0.58em;
  color: rgba(0,255,231,0.35);
  letter-spacing: 2px;
  text-transform: uppercase;
  margin-bottom: 8px;
}
.s-val {
  font-size: 2em;
  font-weight: 700;
  color: #fff;
  line-height: 1;
}
.s-unit {
  font-size: 0.4em;
  color: rgba(0,255,231,0.5);
  margin-left: 2px;
}

/* Bar indikator tanah */
.soil-bar-wrap {
  margin-top: 10px;
  height: 4px;
  background: rgba(255,255,255,0.08);
  border-radius: 2px;
  overflow: hidden;
}
.soil-bar {
  height: 100%;
  border-radius: 2px;
  transition: width 0.8s ease;
  background: linear-gradient(90deg, var(--c3), var(--c));
}

/* MODE SELECTOR */
.mode-wrap {
  display: flex;
  gap: 8px;
  margin-bottom: 14px;
}
.mode-btn {
  flex:1; padding: 10px 8px;
  background: transparent;
  border: 1px solid var(--border);
  border-radius: 4px;
  color: rgba(0,255,231,0.4);
  font-family: 'Rajdhani', sans-serif;
  font-size: 0.85em;
  font-weight: 600;
  letter-spacing: 1px;
  cursor: pointer;
  transition: all .2s;
  text-transform: uppercase;
}
.mode-btn.active {
  background: rgba(0,255,231,0.1);
  border-color: var(--c);
  color: var(--c);
  box-shadow: 0 0 12px rgba(0,255,231,0.15);
}

/* PANEL */
.panel {
  background: var(--dim);
  border: 1px solid var(--border);
  border-radius: 4px;
  padding: 16px;
  margin-bottom: 14px;
}
.panel-title {
  font-family: 'Share Tech Mono', monospace;
  font-size: 0.6em;
  color: rgba(0,255,231,0.4);
  letter-spacing: 3px;
  text-transform: uppercase;
  margin-bottom: 14px;
  padding-bottom: 10px;
  border-bottom: 1px solid var(--border);
}

/* Threshold slider */
.slider-row {
  display: flex;
  align-items: center;
  gap: 10px;
}
input[type="range"] {
  flex:1; -webkit-appearance:none;
  height: 3px;
  background: rgba(0,255,231,0.15);
  border-radius: 2px; outline:none;
}
input[type="range"]::-webkit-slider-thumb {
  -webkit-appearance:none;
  width:14px; height:14px;
  border-radius:50%;
  background: var(--c);
  box-shadow: 0 0 8px var(--c);
  cursor:pointer;
}
.slider-val {
  font-family: 'Share Tech Mono', monospace;
  font-size: 0.9em;
  color: var(--c);
  min-width: 42px;
  text-align: right;
}
.hint {
  font-size: 0.75em;
  color: rgba(0,255,231,0.3);
  margin-top: 8px;
}

/* Jadwal list */
.jadwal-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 10px 12px;
  border: 1px solid var(--border);
  border-radius: 4px;
  margin-bottom: 8px;
  background: rgba(0,168,255,0.04);
}
.jadwal-time {
  font-family: 'Share Tech Mono', monospace;
  font-size: 1em;
  color: var(--c2);
  letter-spacing: 2px;
}
.btn-hapus {
  background: rgba(255,77,109,0.15);
  border: 1px solid rgba(255,77,109,0.3);
  color: var(--c3);
  border-radius: 4px;
  padding: 4px 10px;
  font-size: 0.75em;
  cursor: pointer;
  font-family: 'Share Tech Mono', monospace;
  letter-spacing: 1px;
}
.add-row {
  display: flex;
  gap: 8px;
  margin-top: 10px;
  align-items: center;
}
.add-row input[type="number"] {
  flex:1; padding: 10px;
  background: rgba(0,255,231,0.05);
  border: 1px solid var(--border);
  border-radius: 4px;
  color: var(--c);
  font-family: 'Share Tech Mono', monospace;
  font-size: 1em;
  text-align: center;
  outline: none;
}
.add-row input[type="number"]::placeholder { color: rgba(0,255,231,0.2); }
.sep { color: rgba(0,255,231,0.3); font-size:1.2em; }
.btn-add {
  padding: 10px 14px;
  background: rgba(0,255,231,0.1);
  border: 1px solid var(--c);
  color: var(--c);
  border-radius: 4px;
  font-family: 'Rajdhani', sans-serif;
  font-weight: 700;
  font-size: 0.85em;
  letter-spacing: 1px;
  cursor: pointer;
  text-transform: uppercase;
}

/* Footer */
.footer-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-top: 20px;
  padding-top: 14px;
  border-top: 1px solid var(--border);
}
.footer-txt {
  font-family: 'Share Tech Mono', monospace;
  font-size: 0.6em;
  color: rgba(0,255,231,0.2);
  letter-spacing: 2px;
}
.btn-reset {
  background: transparent;
  border: 1px solid rgba(0,255,231,0.15);
  color: rgba(0,255,231,0.25);
  border-radius: 4px;
  padding: 6px 12px;
  font-family: 'Share Tech Mono', monospace;
  font-size: 0.6em;
  letter-spacing: 2px;
  cursor: pointer;
  text-transform: uppercase;
}
.btn-reset:hover {
  border-color: var(--c3);
  color: var(--c3);
}

/* Corner decorations */
.corner-tl, .corner-br {
  position: fixed;
  width: 60px; height: 60px;
  pointer-events: none; z-index:0;
  opacity: 0.2;
}
.corner-tl {
  top:0; left:0;
  border-top: 2px solid var(--c);
  border-left: 2px solid var(--c);
}
.corner-br {
  bottom:0; right:0;
  border-bottom: 2px solid var(--c);
  border-right: 2px solid var(--c);
}
</style>
</head>
<body>

<div class="corner-tl"></div>
<div class="corner-br"></div>

<div class="wrap">

  <!-- HEADER -->
  <div class="header">
    <div class="header-left">
      <div class="sys-label">// SYSTEM v2.0</div>
      <div class="sys-title">PLANT<br>MONITOR</div>
    </div>
    <div class="header-right">
      <div class="jam-display" id="jamDisplay">--:--</div>
      <div style="margin-top:6px">
        <span class="status-dot"></span>
        <span class="online-label">ONLINE</span>
      </div>
    </div>
  </div>

  <!-- STATUS SIRAM -->
  <div class="status-box">
    <div class="status-tag">// REKOMENDASI</div>
    <div class="status-value" id="statusSiram">MENGECEK...</div>
  </div>

  <!-- SENSOR GRID -->
  <div class="sensor-grid">
    <div class="sensor-card">
      <div class="s-tag">// SUHU</div>
      <div class="s-val" id="suhu">--<span class="s-unit">°C</span></div>
    </div>
    <div class="sensor-card">
      <div class="s-tag">// UDARA</div>
      <div class="s-val" id="humidity">--<span class="s-unit">%</span></div>
    </div>
    <div class="sensor-card full">
      <div class="s-tag">// KELEMBABAN TANAH</div>
      <div class="s-val" id="soil">--<span class="s-unit">%</span></div>
      <div class="soil-bar-wrap">
        <div class="soil-bar" id="soilBar" style="width:0%"></div>
      </div>
    </div>
  </div>

  <!-- MODE SELECTOR -->
  <div class="mode-wrap">
    <button class="mode-btn" id="tabSensor" onclick="setMode('sensor')">◈ MODE SENSOR</button>
    <button class="mode-btn" id="tabJadwal" onclick="setMode('jadwal')">◷ MODE JADWAL</button>
  </div>

  <!-- PANEL SENSOR -->
  <div id="panelSensor" class="panel" style="display:none">
    <div class="panel-title">// KONFIGURASI SENSOR</div>
    <div class="slider-row">
      <span style="font-size:0.8em;color:rgba(0,255,231,0.5);letter-spacing:1px">THRESHOLD</span>
      <input type="range" id="thresholdSlider" min="10" max="80" step="5"
        oninput="updateThresholdLabel()" onchange="setThreshold()">
      <span class="slider-val" id="thresholdVal">--%</span>
    </div>
    <div class="hint">Tanah di bawah threshold → "SIRAM SEKARANG"</div>
  </div>

  <!-- PANEL JADWAL -->
  <div id="panelJadwal" class="panel" style="display:none">
    <div class="panel-title">// JADWAL SIRAM</div>
    <div id="jadwalList"></div>
    <div class="add-row">
      <input type="number" id="inputJam" min="0" max="23" placeholder="HH">
      <span class="sep">:</span>
      <input type="number" id="inputMenit" min="0" max="59" placeholder="MM">
      <button class="btn-add" onclick="tambahJadwal()">+ ADD</button>
    </div>
    <div class="hint" style="margin-top:10px">Relay aktif 5 detik sebagai sinyal fisik saat jadwal tiba</div>
  </div>

  <!-- FOOTER -->
  <div class="footer-row">
    <span class="footer-txt">AUTO-REFRESH / 2S</span>
    <button class="btn-reset" onclick="resetWifi()">⟳ GANTI WIFI</button>
  </div>

</div>

<script>
let modeAktif = 'sensor';

function fetchData() {
  fetch('/data').then(r => r.json()).then(d => {
    document.getElementById('suhu').innerHTML     = d.suhu + '<span class="s-unit">°C</span>';
    document.getElementById('humidity').innerHTML = d.kelembabanUdara + '<span class="s-unit">%</span>';
    document.getElementById('soil').innerHTML     = d.kelembabanTanah + '<span class="s-unit">%</span>';
    document.getElementById('soilBar').style.width = d.kelembabanTanah + '%';
    document.getElementById('jamDisplay').textContent = d.jam;

    const el = document.getElementById('statusSiram');
    el.textContent = d.statusSiram.toUpperCase();
    el.className = 'status-value';
    if (d.statusSiram.includes('Waktu')) {
      el.classList.add('status-waktu');
    } else if (d.statusSiram.includes('Siram') && !d.statusSiram.includes('Jangan')) {
      el.classList.add('status-siram');
    } else {
      el.classList.add('status-jangan');
    }

    if (d.modeSiram !== modeAktif) {
      modeAktif = d.modeSiram;
      updateTabs();
    }

    document.getElementById('thresholdSlider').value = d.threshold;
    document.getElementById('thresholdVal').textContent = d.threshold + '%';

    const list = document.getElementById('jadwalList');
    list.innerHTML = '';
    if (d.jadwal.length === 0) {
      list.innerHTML = '<div style="font-family:Share Tech Mono,monospace;font-size:0.75em;color:rgba(0,255,231,0.2);text-align:center;padding:12px;letter-spacing:2px">-- TIDAK ADA JADWAL --</div>';
    } else {
      d.jadwal.forEach(j => {
        const jam   = String(j.jam).padStart(2, '0');
        const menit = String(j.menit).padStart(2, '0');
        list.innerHTML += `
          <div class="jadwal-item">
            <span class="jadwal-time">▶ ${jam}:${menit} WIB</span>
            <button class="btn-hapus" onclick="hapusJadwal(${j.id})">HAPUS</button>
          </div>`;
      });
    }
  }).catch(e => console.error(e));
}

function setMode(mode) {
  modeAktif = mode;
  updateTabs();
  fetch('/mode?mode=' + mode).then(() => fetchData());
}

function updateTabs() {
  document.getElementById('tabSensor').className = 'mode-btn' + (modeAktif === 'sensor' ? ' active' : '');
  document.getElementById('tabJadwal').className = 'mode-btn' + (modeAktif === 'jadwal' ? ' active' : '');
  document.getElementById('panelSensor').style.display = modeAktif === 'sensor' ? 'block' : 'none';
  document.getElementById('panelJadwal').style.display = modeAktif === 'jadwal' ? 'block' : 'none';
}

function updateThresholdLabel() {
  document.getElementById('thresholdVal').textContent = document.getElementById('thresholdSlider').value + '%';
}

function setThreshold() {
  fetch('/threshold?value=' + document.getElementById('thresholdSlider').value).then(() => fetchData());
}

function tambahJadwal() {
  const jam   = document.getElementById('inputJam').value;
  const menit = document.getElementById('inputMenit').value;
  if (jam === '' || menit === '') { alert('Isi jam dan menit dulu!'); return; }
  if (jam < 0 || jam > 23)       { alert('Jam harus 0-23!'); return; }
  if (menit < 0 || menit > 59)   { alert('Menit harus 0-59!'); return; }
  fetch('/jadwal?jam=' + jam + '&menit=' + menit).then(() => {
    document.getElementById('inputJam').value   = '';
    document.getElementById('inputMenit').value = '';
    fetchData();
  });
}

function hapusJadwal(id) {
  fetch('/hapusjadwal?id=' + id).then(() => fetchData());
}

function resetWifi() {
  if (confirm('Reset WiFi? NodeMCU akan restart dan masuk mode setup ulang.')) {
    fetch('/resetwifi').then(() => {
      alert('NodeMCU sedang restart. Sambungkan HP ke WiFi "PlantSetup" untuk setup ulang.');
    });
  }
}

fetchData();
setInterval(fetchData, 2000);
updateTabs();
</script>
</body>
</html>
)rawliteral";
  return html;
}
