#define SOIL_PIN A0

void setup() {
  Serial.begin(115200);
  Serial.println("Mulai test Soil Moisture Sensor...");
}

void loop() {
  int nilaiRaw = analogRead(SOIL_PIN);

  Serial.print("Nilai Raw: ");
  Serial.println(nilaiRaw);

  delay(1000); // baca tiap 1 detik
}
