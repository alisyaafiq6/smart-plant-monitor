#include <DHT.h>

#define DHTPIN D4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println("Mulai test DHT11...");
  dht.begin();
}

void loop() {
  delay(2000); // DHT11 butuh waktu minimal 2 detik antar pembacaan

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Gagal membaca sensor DHT11!");
    return;
  }

  Serial.print("Suhu: ");
  Serial.print(t);
  Serial.print(" C, Kelembaban: ");
  Serial.print(h);
  Serial.println(" %");
}
