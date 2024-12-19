#include <Adafruit_SCD30.h>

Adafruit_SCD30 scd30;

void setup() {
  Serial.begin(115200);
  if (!scd30.begin()) {
    Serial.println("Failed to find SCD30 chip");
    while (1);
  }
  Serial.println("SCD30 Found!");
}

void loop() {
  if (scd30.dataReady()) {
    if (scd30.read()) {
      Serial.print("CO2: ");
      Serial.print(scd30.CO2);
      Serial.println(" ppm");
      Serial.print("Temperature: ");
      Serial.print(scd30.temperature);
      Serial.println(" C");
      Serial.print("Humidity: ");
      Serial.print(scd30.relative_humidity);
      Serial.println(" %");
    } else {
      Serial.println("Error reading sensor data");
    }
  }
  delay(2000);
}
