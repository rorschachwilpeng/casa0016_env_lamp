#define BUTTON_PIN 12

void setup() {
  pinMode(BUTTON_PIN, INPUT);
  Serial.begin(115200);
}

void loop() {
  if (digitalRead(BUTTON_PIN) == HIGH) {
    Serial.println("Button Pressed!");
    delay(200);
  }
}
