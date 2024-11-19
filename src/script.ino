#include <LiquidCrystal.h>
#include <Adafruit_SCD30.h>
#include <Adafruit_NeoPixel.h>

#define PIN 13
#define NUMPIXELS 20
#define DELAYVAL 100 // 减少延迟以使呼吸灯变化更快
#define MIN_CO2 0    // CO2浓度的最小值
#define MAX_CO2 20000 // CO2浓度的最大值

Adafruit_NeoPixel pixels(NUMPIXELS, PIN);

// 初始化LCD对象，将LCD的控制引脚与Arduino的数字引脚关联
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

Adafruit_SCD30 scd30;

// LED
int brightness = 0; // 初始亮度
int brightnessDirection = 1; // 控制亮度变化的方向，1为增加，-1为减少
int displayIndex = 0; // 当前显示的数据索引

// Sensor Data
static float defaultCO2 = 0;
static float defaultTemperature = 0;
static float defaultHumidity = 0;
bool isFirstData = true; // First time receive marker
static int dataCount = 0; // 已收集数据的次数
static float sumCO2 = 0, sumTemperature = 0, sumHumidity = 0; // 用于计算平均值
int MaxCO2=defaultCO2+5000;
bool isInitialized = false; // 标志，表示是否已完成初始化

void setup() {
  lcd.begin(16, 2);
  Serial.begin(115200);
  if (!scd30.begin()) {
    Serial.println("Failed to find SCD30 chip");
    while (1) { delay(10); }
  }
  Serial.println("SCD30 Found!");
  pixels.begin();
}

void loop() {
  static unsigned long lastTime = 0; // 记录上次更新LED灯的时间
  unsigned long currentTime = millis();

  // 检查SCD30是否有新的数据准备好
  if (scd30.dataReady() && (currentTime - lastTime > DELAYVAL)) {
    if (!scd30.read()) {
      Serial.println("Error reading sensor data");
    } else {
      // 数据准备好了
      setInitialData(5);
      if (isInitialized && defaultCO2 != 0 && defaultTemperature != 0 && defaultHumidity != 0) {
        displaySensorData(); // 显示传感器数据
      }
      lastTime = currentTime; // 更新LED灯的时间
    }
  }

  lightingLED(); // 呼吸灯效果
  delay(10); // 短暂延迟以减少CPU负载
}

void lightingLED() {
  // 如果没有完成初始化，不执行逻辑
  if (!isInitialized) return;

  // 定义静态变量用于时间控制
  static unsigned long lastUpdate = 0;       // 上次更新的时间
  const unsigned long updateInterval = 60;  // 调整更新频率

  unsigned long currentTime = millis();
  if (currentTime - lastUpdate < updateInterval) return; // 控制更新频率
  lastUpdate = currentTime; // 更新时间

  // 调整亮度，控制呼吸灯效果
  brightness += brightnessDirection * 5; // 调整亮度步长为5
  if (brightness >= 255 || brightness <= 0) { // 调整呼吸范围
    brightnessDirection *= -1; // 反转亮度变化方向
  }

  // 根据CO2浓度设置目标颜色
  int co2Level = map(scd30.CO2, defaultCO2, MaxCO2, 0, 255); // 映射浓度范围
  co2Level = constrain(co2Level, 0, 255); // 限制在0到255之间

  int targetRed = map(co2Level, 0, 255, 0, 255);
  int targetGreen = map(co2Level, 0, 255, 255, 0);
  int targetBlue = 0;

  // 使用平滑过渡逻辑让颜色缓慢变化
  static int lastRed = 0, lastGreen = 255, lastBlue = 0;
  lastRed += (targetRed - lastRed) * 0.05; // 平滑因子调整为 0.05
  lastGreen += (targetGreen - lastGreen) * 0.05;
  lastBlue += (targetBlue - lastBlue) * 0.05;

  // 调整颜色亮度，形成呼吸灯效果
  int adjustedRed = (lastRed * brightness) / 255;
  int adjustedGreen = (lastGreen * brightness) / 255;
  int adjustedBlue = (lastBlue * brightness) / 255;

  // 更新LED颜色
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(adjustedRed, adjustedGreen, adjustedBlue));
  }
  pixels.show();
}




// Initialize data
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

// 函数：显示传感器数据
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