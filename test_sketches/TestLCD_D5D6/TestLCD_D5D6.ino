#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // alamat 0x27, ukuran 16x2

void setup() {
  Wire.begin(D5, D6); // SDA = D5, SCL = D6
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Halo Ali!");
  lcd.setCursor(0, 1);
  lcd.print("LCD Berhasil!");
}

void loop() {
  // kosong, cukup tampil sekali aja
}
