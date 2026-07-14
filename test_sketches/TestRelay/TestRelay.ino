#define RELAY_PIN D7

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // mulai dari OFF
  Serial.println("Mulai test Relay...");
}

void loop() {
  Serial.println("Relay ON");
  digitalWrite(RELAY_PIN, HIGH);
  delay(2000);

  Serial.println("Relay OFF");
  digitalWrite(RELAY_PIN, LOW);
  delay(2000);
}
