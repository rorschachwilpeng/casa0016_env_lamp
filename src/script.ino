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
#define INIT_TIMES 2 // how many time you wanna measrue for the init? Suggested range:[2,5]

/* Define the LCD refresh rate in milliseconds */
#define LCD_INTERVAL 2000 //The refresh interval is 2 second

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
int displayIndex = 0; // display current data index
static int selectedDataType=0;

////// LED
int brightness = 0; // init luminance
int brightnessDirection = 1; // Controls the direction of brightness change, 1 is to increase, -1 is to decrease
static unsigned long lastLEDTime = 0;
float globalBrightnessFactor = 1.0; // 全局亮度修饰因子，范围 [0.0, 1.0]

////// LCD
unsigned long lastDisplayTime = 0; // The last time it was displayed

//////  CO2 Sensor
bool isFirstData = true; // First time receive marker
static int dataCount = 0; // how many time collteced already
static float sumCO2 = 0, sumTemperature = 0, sumHumidity = 0; // sum val
bool isInitialized = false; // marker
//CO2
static float defaultCO2 = 0;
int MaxCO2=0;
//temp
static float defaultTemperature = 0;
int MaxTemp=0;
//humid
static float defaultHumidity = 0;
int MaxHumid=0;

//////  Encoder
float last_degrees = -1; // Used to store the angle of the last read, the initial value is set to an invalid angle
/********* ---------------------------------------------------------------------------------------------------------------------------------------------------------------- ********/
void setup() {
  Serial.begin(115200);

  // Encoder 
  pinMode(ROTARY_ANGLE_SENSOR, INPUT); // Set the knob sensor pin as input
  //LCD 
  lcd.begin(16, 2);
  lcd.setBacklight(255); //set the lightness to the maximum
  // SCD30
  if (!scd30.begin()) {
    Serial.println("Failed to find SCD30 chip");
    while (1) { delay(10); }
  }
  Serial.println("SCD30 Found!");
  pixels.begin();
}

void loop() {
  unsigned long currentTime = millis();

  if (!isDataSelected) {// waiting for user's datatype choice, default --> CO2
      handleUserSelection();
  } else{
      float sensor_val = getSensorData();
      if (isInitialized && defaultCO2 != 0 && defaultTemperature != 0 && defaultHumidity != 0) {
          // LCD and IO update
          if (currentTime - lastDisplayTime >= LCD_INTERVAL) {
            displaySensorData();
            lastDisplayTime = currentTime;//update display time 
          }
          // LED update
          if (currentTime - lastLEDTime >= LED_INTERVAL) {
              adjustGlobalBrightnessFactor(); // 调整全局亮度
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
    // 读取旋钮传感器的模拟值
    int sensor_value = analogRead(ROTARY_ANGLE_SENSOR);
    // 将传感器值转换为电压
    float voltage = (float)sensor_value * ADC_REF / 1023;
    // 将电压转换为角度（单位：度）
    float degrees = (voltage * FULL_ANGLE) / GROVE_VCC;
    return degrees;
}

// user select the datatyple(which sensor data used to control the LED)
void handleUserSelection() {
    static unsigned long stableStartTime = 0; // 开始计时的时间戳
    static float lastStableAngle = -1;       // 上一次稳定角度
    float currentAngle = getEncoderStage();  // 获取当前旋钮角度

    // 显示当前选择
    displaySelectionOnLCD(currentAngle);

    // 检查角度是否稳定
    if (abs(currentAngle - lastStableAngle) <= 5) {
        // 如果当前角度稳定在上一次角度范围内
        if (millis() - stableStartTime > 3000) {
            // 角度稳定超过3秒，认为用户已选择该模式
            selectedDataType = mapAngleToDataType(currentAngle);
            isDataSelected = true;
            Serial.print("Selected Data Type: ");
            Serial.println(selectedDataType);
        }
    } else {
        // 如果当前角度发生变化，刷新稳定计时
        stableStartTime = millis();
        lastStableAngle = currentAngle;
    }
}

// grab the specific sensor data
float getSensorData() {
  // check SCD30 aviliability 
  if (scd30.dataReady()){
    if (scd30.read()) {
      // Initalization
      setInitialData(INIT_TIMES);
    } else {
      Serial.println("Error reading sensor data");
    }
  }

  // 根据 selectedDataType 的值处理传感器数据并控制LED
  if (selectedDataType == 1) {// 温度控制LED
      return scd30.temperature;
  } else if (selectedDataType == 2) {// 湿度控制LED
      return scd30.relative_humidity;
  } else if (selectedDataType == 3) {//CO2
      return scd30.CO2;
  }
}

// Map the Encoder rotation angle to the datatype
int mapAngleToDataType(float angle) {
    if (angle >= 0 && angle <= 60) return 1; // temperature
    if (angle > 60 && angle <= 120) return 2; // humidity
    if (angle > 120 && angle <= 210) return 3; // CO2
    return 3; // default as CO2
}

// display selection text on the LCD
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

void adjustGlobalBrightnessFactor() {
    // 获取旋钮角度
    float angle = getEncoderStage(); 
    
    // 将角度映射到全局亮度因子范围 [0.2, 1.0]
    float targetFactor = map(angle, 0, FULL_ANGLE, 0.2 * 100, 1.0 * 100) / 100.0;
    targetFactor = constrain(targetFactor, 0.2, 1.0); // 限制范围

    // 平滑调整亮度因子，避免跳变
    globalBrightnessFactor += (targetFactor - globalBrightnessFactor) * 0.2; // 平滑因子 0.2 可调整
}

// LED light logic
void lightingLED(int sen_val) { 
  // 如果没有完成初始化，不执行逻辑
  if (!isInitialized) return;

  // 调整亮度，控制呼吸灯效果
  brightness += brightnessDirection * 5; // 调整亮度步长为5
  if (brightness >= 255 || brightness <= 0) { // 调整呼吸范围
    brightnessDirection *= -1; // 反转亮度变化方向
  }

  //map concentration ranges
  int tar_color=-1;//target color
  if (selectedDataType==1){// temperature
    MaxTemp=defaultTemperature+15;
    tar_color = map(scd30.temperature, defaultTemperature, MaxTemp, 0, 255);

  } else if(selectedDataType==2){// humidity
    MaxHumid=defaultHumidity+15;
    tar_color = map(scd30.relative_humidity, defaultHumidity, MaxHumid, 0, 255); 

  } else if(selectedDataType==3){// CO2
    MaxCO2=defaultCO2+5000;
    tar_color = map(scd30.CO2, defaultCO2, MaxCO2, 0, 255);
  }

  tar_color = constrain(tar_color, 0, 255); // 限制在0到255之间
  int targetRed = map(tar_color, 0, 255, 0, 255);
  int targetGreen = map(tar_color, 0, 255, 255, 0);
  int targetBlue = 0;

  // 使用平滑过渡逻辑让颜色缓慢变化
  static int lastRed = 0, lastGreen = 255, lastBlue = 0;
  lastRed += (targetRed - lastRed) * 0.05; // 平滑因子调整为 0.05
  lastGreen += (targetGreen - lastGreen) * 0.05;
  lastBlue += (targetBlue - lastBlue) * 0.05;

  // // 调整颜色亮度，形成呼吸灯效果
  int adjustedRed = (lastRed * brightness) / 255;
  int adjustedGreen = (lastGreen * brightness) / 255;
  int adjustedBlue = (lastBlue * brightness) / 255;

 // 调整颜色亮度，同时叠加全局亮度因子
    // int adjustedRed = (lastRed * brightness * globalBrightnessFactor) / (255 * 255);
    // int adjustedGreen = (lastGreen * brightness * globalBrightnessFactor) / (255 * 255);
    // int adjustedBlue = (lastBlue * brightness * globalBrightnessFactor) / (255 * 255);

  // 更新LED颜色
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(adjustedRed, adjustedGreen, adjustedBlue));
  }
  pixels.show();
}

// Initialization
void setInitialData(int init_time) {
  if (isFirstData) {
    // 跳过第一次数据
    isFirstData = false;
  } else if (!isInitialized) {
      Serial.println("Initializing");
      lcd.clear();
      lcd.print("Initializing...");
      // 收集数据并计算平均值
      sumCO2 += scd30.CO2;
      sumTemperature += scd30.temperature;
      sumHumidity += scd30.relative_humidity;

      dataCount++;
      // 检查是否已收集五次数据
      if (dataCount == init_time) {
        defaultCO2 = sumCO2 / init_time;
        defaultTemperature = sumTemperature / init_time;
        defaultHumidity = sumHumidity / init_time;
        Serial.print("Default CO2: ");
        Serial.println(defaultCO2);
        Serial.print("Default Temperature: ");
        Serial.println(defaultTemperature);
        Serial.print("Default Humidity: ");
        Serial.println(defaultHumidity);
        isInitialized = true; // 设置初始化完成标志
      }
  }
}

// LCD and serial monitor IO
void displaySensorData() {
  lcd.clear(); // 清除LCD显示
  //IO
  Serial.print("CO2: ");
  Serial.print(scd30.CO2, 3);
  Serial.println(" ppm");

  Serial.print("Temp: ");
  Serial.print(scd30.temperature);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(scd30.relative_humidity);
  Serial.println(" %");
  
  // 根据displayIndex的值显示不同的数据
  switch (displayIndex) {
    case 0:
      // 打印CO2浓度
      lcd.print("CO2: ");
      lcd.print(scd30.CO2, 3);
      lcd.println(" ppm");
      break;
    case 1:
      // 打印温度
      lcd.print("Temp: ");
      lcd.print(scd30.temperature);
      lcd.println(" C");
      break;
    case 2:
      // 打印湿度
      lcd.print("Humidity: ");
      lcd.print(scd30.relative_humidity);
      lcd.println(" %");
      break;
  }
  
  // 更新displayIndex以滚动到下一个数据
  displayIndex = (displayIndex + 1) % 3;
}