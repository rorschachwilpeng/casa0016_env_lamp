#define ROTARY_ANGLE_SENSOR A0
#define ADC_REF 5
#define GROVE_VCC 5
#define FULL_ANGLE 300

void setup() {
  Serial.begin(115200);
  pinMode(ROTARY_ANGLE_SENSOR, INPUT);
}

void loop() {
  int sensor_value = analogRead(ROTARY_ANGLE_SENSOR);
  float voltage = (float)sensor_value * ADC_REF / 1023;
  float degrees = (voltage * FULL_ANGLE) / GROVE_VCC;

  Serial.print("Angle: ");
  Serial.print(degrees);
  Serial.println(" degrees");
  delay(500);
}
