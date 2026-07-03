#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <Arduino.h>

Adafruit_VL53L0X sensor = Adafruit_VL53L0X();

// DRV8833 pins
const int AIN1 = 18;
const int AIN2 = 19;
const int NSLEEP = 5;

const int DETECT_DISTANCE = 100; // mm

enum LidState {
  CLOSED,
  OPEN
};

LidState lidState = CLOSED;

void motorOpen() {
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
}

void motorClose() {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
}

void motorStop() {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
}

void setup() {
  Serial.begin(115200);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(NSLEEP, OUTPUT);

  digitalWrite(NSLEEP, HIGH);

  Wire.begin(21, 22);

  if (!sensor.begin()) {
    Serial.println("VL53L0X not found");
    while (1);
  }

  sensor.startRangeContinuous();
}

void loop() {
  VL53L0X_RangingMeasurementData_t measure;

  sensor.rangingTest(&measure, false); // false = don't print debug info

  bool detected = false;

  if (measure.RangeStatus != 4) { // 4 = out of range
    int distance = measure.RangeMilliMeter;

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" mm");

    detected = distance < DETECT_DISTANCE;
  } else {
    Serial.println("Out of range");
  }

  if (detected && lidState == CLOSED) {
    Serial.println("Opening");
    motorOpen();
    delay(800);
    motorStop();
    lidState = OPEN;
  }

  if (!detected && lidState == OPEN) {
    Serial.println("Closing");
    motorClose();
    delay(800);
    motorStop();
    lidState = CLOSED;
  }

  delay(50);
}