#include <LiquidCrystal_PCF8574.h>

LiquidCrystal_PCF8574 lcd(0x27);

void setup() {
  lcd.begin(16, 2);
  lcd.setBacklight(255);
  lcd.clear();
  lcd.print("LCD Test");
}

void loop() {
  lcd.clear();
  lcd.print("Line 1: Hello!");
  delay(1000);
  lcd.setCursor(0, 1);
  lcd.print("Line 2: World!");
  delay(1000);
}
