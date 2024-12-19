#include <Adafruit_NeoPixel.h>

#define PIN 13
#define NUMPIXELS 49

Adafruit_NeoPixel pixels(NUMPIXELS, PIN);

void setup() {
  pixels.begin();
}

void loop() {
  // Light up LEDs with red color
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 0, 0)); // Red
  }
  pixels.show();
  delay(1000);

  // Turn off LEDs
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0)); // Off
  }
  pixels.show();
  delay(1000);
}
