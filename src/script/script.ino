// Headers
#include <LiquidCrystal.h>
#include <Adafruit_SCD30.h>
#include <Adafruit_NeoPixel.h>
# include <LiquidCrystal_PCF8574.h>

/********* ---------------------------------------------------------------------------------------------------------------------------------------------------------------- ********/
/* Macro definitions of Rotary angle sensor */
#define ROTARY_ANGLE_SENSOR A0
#define ADC_REF 5       // Reference voltage of ADC is 5V
#define GROVE_VCC 5     // VCC of the Grove interface is normally 5V
#define FULL_ANGLE 300  // Full value of the rotary angle is 300 degrees

/* Macro definitions of LED */
#define PIN 13
#define NUMPIXELS 60
#define REFRESHFREUENCY 60 // LED refresh frequency

/* Macro definitions of SCD30*/
#define INIT_TIMES 3 // Number of measurements for initialization. Suggested range: [2,5]

/* Define the LCD refresh rate in milliseconds */
#define LCD_INTERVAL 2000 // The refresh interval is 2 seconds

/* Define the LED refresh rate in milliseconds */
#define LED_INTERVAL 60



/********* ---------------------------------------------------------------------------------------------------------------------------------------------------------------- ********/
/* Components */
Adafruit_NeoPixel pixels(NUMPIXELS, PIN);
// set I2C address
LiquidCrystal_PCF8574 lcd(0x27);
// set SCD30
Adafruit_SCD30 scd30;

/* Component variables */
static bool isDataSelected = false; 
int displayIndex = 0; // Display current data index
static int selectedDataType=0;

////// LED
int brightness = 0; // Initial brightness
int brightnessDirection = 1; // Controls the direction of brightness change, 1 is to increase, -1 is to decrease
static unsigned long lastLEDTime = 0;
float globalBrightnessFactor = 1.0; // Global brightness adjustment factor, range [0.0, 1.0]

////// LCD
unsigned long lastDisplayTime = 0; // The last time the LCD was displayed

//////  CO2 Sensor
bool isFirstData = true; // Marker for first data received
static int dataCount = 0; // Number of collected measurements
static float sumCO2 = 0, sumTemperature = 0, sumHumidity = 0; // Sum of sensor values
bool isInitialized = false; // Initialization marker
// CO2
static float defaultCO2 = 0;
int MaxCO2=0;
int MinCO2=0;
// Temp
static float defaultTemperature = 0;
int MaxTemp=0;
int MinTemp=0;
// Humidity
static float defaultHumidity = 0;
int MaxHumid=0;
int MinHumid=0;

//////  Encoder
float last_degrees = -1; // Used to store the angle of the last read, the initial value is set to an invalid angle
/********* ---------------------------------------------------------------------------------------------------------------------------------------------------------------- ********/
void setup() {
  Serial.begin(115200);

  // Encoder 
  pinMode(ROTARY_ANGLE_SENSOR, INPUT); // Set the rotary angle sensor pin as input
  // LCD 
  lcd.begin(16, 2);
  lcd.setBacklight(255); // Set the backlight brightness to maximum
  // SCD30
  if (!scd30.begin()) {
    Serial.println("Failed to find SCD30 chip");
    while (1) { delay(10); }
  }
  Serial.println("SCD30 Found!");
  pixels.begin();
  clearLEDMemory();
  gradualStartup();//graduate light up LED
  pixels.show();
}

void loop() {
  unsigned long currentTime = millis();

  if (!isDataSelected) { // Waiting for user's datatype choice, default --> CO2
      handleUserSelection();
  } else{
      float sensor_val = getSensorData();
      if (isInitialized && defaultCO2 != 0 && defaultTemperature != 0 && defaultHumidity != 0) {
          // LCD and IO update
          if ((currentTime - lastDisplayTime) >= LCD_INTERVAL) {
            displaySensorData();
            lastDisplayTime = currentTime; // Update display time
          }
          // LED update
          if (currentTime - lastLEDTime >= LED_INTERVAL) {
              lightingLED(sensor_val); // Control the breathing light
              lastLEDTime = currentTime;
          }
      }   
  }
  delay(10); // Short delay to reduce CPU load
}
/********* ---------------------------------------------------------------------------------------------------------------------------------------------------------------- ********/
// Encoder logic
float getEncoderStage(){
    // Read the analog value from the rotary angle sensor
    int sensor_value = analogRead(ROTARY_ANGLE_SENSOR);
    // Convert sensor value to voltage
    float voltage = (float)sensor_value * ADC_REF / 1023;
    // Convert voltage to angle (in degrees)
    float degrees = (voltage * FULL_ANGLE) / GROVE_VCC;
    return degrees;
}

// User selects the datatype (which sensor data to use for LED control)
void handleUserSelection() {
    static unsigned long stableStartTime = 0; // Start timestamp for stability
    static float lastStableAngle = -1;       // Last stable angle
    float currentAngle = getEncoderStage();  // Get current rotary angle

    // Display the current selection
    displaySelectionOnLCD(currentAngle);

    // Check if the angle is stable
    if (abs(currentAngle - lastStableAngle) <= 5) {
        // If the current angle is stable within the range of the last angle
        if (millis() - stableStartTime > 3000) {
            // If the angle is stable for more than 3 seconds, consider the selection confirmed
            selectedDataType = mapAngleToDataType(currentAngle);
            isDataSelected = true;
            Serial.print("Selected Data Type: ");
            Serial.println(selectedDataType);
        }
    } else {
        // If the current angle changes, reset stability timing
        stableStartTime = millis();
        lastStableAngle = currentAngle;
    }
}

// Retrieve specific sensor data
float getSensorData() {
  // Check SCD30 availability 
  if (scd30.dataReady()){
    if (scd30.read()) {
      // Initialization
      setInitialData(INIT_TIMES);
    } else {
      Serial.println("Error reading sensor data");
    }
  }

  // Process sensor data based on selectedDataType and control LED
  if (selectedDataType == 1) { // Temperature controls LED
      return scd30.temperature;
  } else if (selectedDataType == 2) { // Humidity controls LED
      return scd30.relative_humidity;
  } else if (selectedDataType == 3) { // CO2 controls LED
      return scd30.CO2;
  }
}

// Initialization
void setInitialData(int init_time) {
  if (isFirstData) {
    // Skip the first set of data
    isFirstData = false;
  } else if (!isInitialized) {
      Serial.println("Initializing");
      lcd.clear();
      lcd.print("Initializing...");
      // Collect data and calculate the average
      sumCO2 += scd30.CO2;
      sumTemperature += scd30.temperature;
      sumHumidity += scd30.relative_humidity;

      dataCount++;
      // Check if sufficient data points have been collected
      if (dataCount == init_time) {
        
        //temp
        defaultTemperature = sumTemperature / init_time;
        MinTemp = defaultTemperature - 15;
        MaxTemp = defaultTemperature + 15;
        //CO2
        defaultCO2 = sumCO2 / init_time;
        MinCO2 = defaultCO2 - 1000;
        MaxCO2 = defaultCO2 + 5000;
        //humiditiy
        defaultHumidity = sumHumidity / init_time;
        MinHumid = defaultHumidity - 40;
        MaxHumid = defaultHumidity + 40;
        Serial.print("Default CO2: ");
        Serial.println(defaultCO2);
        Serial.print("Default Temperature: ");
        Serial.println(defaultTemperature);
        Serial.print("Default Humidity: ");
        Serial.println(defaultHumidity);
        isInitialized = true; // Set the initialization flag
      }
  }
}

// Map the Encoder rotation angle to the datatype
int mapAngleToDataType(float angle) {
    if (angle >= 0 && angle <= 60) return 1; // Temperature
    if (angle > 60 && angle <= 120) return 2; // Humidity
    if (angle > 120 && angle <= 210) return 3; // CO2
    return 3; // Default as CO2
}

// Display the selected text on the LCD
void displaySelectionOnLCD(float angle) {
    if (angle >= 0 && angle <= 60) {
        lcd.clear();
        lcd.print("Temperature");
    } else if (angle > 70 && angle <= 130) {
        lcd.clear();
        lcd.print("Humidity");
    } else if (angle > 140 && angle <= 200) {
        lcd.clear();
        lcd.print("CO2");
    }
}

// LED light logic
void lightingLED(float sen_val) {
    if (!isInitialized) return; // Skip if not initialized

    // Breathing effect adjustment
    brightness += brightnessDirection * 5;
    if (brightness >= 255 || brightness <= 0) {
        brightnessDirection *= -1; // Reverse brightness direction
    }

    int tar_color = -1; // Target color intensity
    int targetRed = 0, targetGreen = 0, targetBlue = 0; // RGB values

    if (selectedDataType == 3) { // CO2
        // Map CO2 to green-to-red transition
        tar_color = map(sen_val, defaultCO2, MaxCO2, 0, 255);
        tar_color = constrain(tar_color, 0, 255); // Clamp target color

        // Green to Red transition
        targetRed = map(tar_color, 0, 255, 0, 255); // Increase red
        targetGreen = map(tar_color, 0, 255, 255, 0); // Decrease green
        targetBlue = 0; // No blue in CO2 mode
    } else {
        // Retain existing logic for temperature and humidity
        float hysteresis = (selectedDataType == 1 || selectedDataType == 2) ? 1.5 : 0;
        if (selectedDataType == 1) { // Temperature
            if (sen_val < defaultTemperature - hysteresis) { // Blue to Green
                tar_color = map(sen_val, MinTemp, defaultTemperature - hysteresis, 255, 0);
                targetRed = 0;
                targetGreen = map(tar_color, 255, 0, 0, 255);
                targetBlue = map(tar_color, 255, 0, 255, 0);
            } else if (sen_val > defaultTemperature + hysteresis) { // Green to Red
                tar_color = map(sen_val, defaultTemperature + hysteresis, MaxTemp, 0, 255);
                targetRed = map(tar_color, 0, 255, 0, 255);
                targetGreen = map(tar_color, 0, 255, 255, 0);
                targetBlue = 0;
            }
        } else if (selectedDataType == 2) { // Humidity
            if (sen_val < defaultHumidity - hysteresis) { // Blue to Green
                tar_color = map(sen_val, MinHumid, defaultHumidity - hysteresis, 255, 0);
                targetRed = 0;
                targetGreen = map(tar_color, 255, 0, 0, 255);
                targetBlue = map(tar_color, 255, 0, 255, 0);
            } else if (sen_val > defaultHumidity + hysteresis) { // Green to Red
                tar_color = map(sen_val, defaultHumidity + hysteresis, MaxHumid, 0, 255);
                targetRed = map(tar_color, 0, 255, 0, 255);
                targetGreen = map(tar_color, 0, 255, 255, 0);
                targetBlue = 0;
            }
        }
    }

    // Smooth color transitions
    if (tar_color >= 0) {
        static int lastRed = 0, lastGreen = 255, lastBlue = 0;
        const float SMOOTH_FACTOR = 0.1; // Smoothing factor
        lastRed += (targetRed - lastRed) * SMOOTH_FACTOR;
        lastGreen += (targetGreen - lastGreen) * SMOOTH_FACTOR;
        lastBlue += (targetBlue - lastBlue) * SMOOTH_FACTOR;

        // Adjust brightness for breathing effect
        int adjustedRed = (lastRed * brightness) / 255;
        int adjustedGreen = (lastGreen * brightness) / 255;
        int adjustedBlue = (lastBlue * brightness) / 255;

        // Update LED colors
        for (int i = 0; i < NUMPIXELS; i++) {
            pixels.setPixelColor(i, pixels.Color(adjustedRed, adjustedGreen, adjustedBlue));
        }
        pixels.show();
    }
}

// Graduately start up LED
void gradualStartup() {
    for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0)); // close all the light
    }
    pixels.show();
    delay(500); // make sure the stability

    for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(50, 50, 50)); // light up slowly
        pixels.show();
        delay(50); // light up LED one by one
    }
}

// clean the LED cache
void clearLEDMemory() {
    for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0)); // clean color
    }
    pixels.show(); // refresh cache
}



// LCD and serial monitor IO
void displaySensorData() {
  lcd.clear(); // Clear the LCD display
  // Serial output
  Serial.print("CO2: ");
  Serial.print(scd30.CO2, 3);
  Serial.println(" ppm");

  Serial.print("Temp: ");
  Serial.print(scd30.temperature);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(scd30.relative_humidity);
  Serial.println(" %");
  
  // Display different data based on displayIndex
  switch (displayIndex) {
    case 0:
      // Print CO2 concentration
      lcd.print("CO2: ");
      lcd.print(scd30.CO2, 3);
      lcd.println(" ppm");
      break;
    case 1:
      // Print temperature
      lcd.print("Temp: ");
      lcd.print(scd30.temperature);
      lcd.println(" C");
      break;
    case 2:
      // Print humidity
      lcd.print("Humidity: ");
      lcd.print(scd30.relative_humidity);
      lcd.println(" %");
      break;
  }
  
  // Update displayIndex to scroll to the next data
  displayIndex = (displayIndex + 1) % 3;
}

