#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <Arduino.h>

Adafruit_VL53L0X sensor = Adafruit_VL53L0X();

// DRV8833 pins
const int AIN1 = 3;
const int AIN2 = 5;

const int DETECT_DISTANCE = 500;      // mm
const int MAX_VALID_DISTANCE = 8000;  // Ignore anything above this
const int motorDelay = 300;           // ms
const int MOTOR_SPEED = 180;          // 0-255

// Number of consecutive "no object" readings before closing
const int OUT_OF_RANGE_THRESHOLD = 3;

int outOfRangeCount = 0;

enum LidState {
  CLOSED,
  OPEN
};

LidState lidState = CLOSED;

void motorOpen() {
  for (int s = 0; s <= MOTOR_SPEED; s += 10) {
    analogWrite(AIN1, s);
    analogWrite(AIN2, 0);
    delay(5);
  }
}

void motorClose() {
  for (int s = 0; s <= MOTOR_SPEED; s += 10) {
    analogWrite(AIN1, 0);
    analogWrite(AIN2, s);
    delay(5);
  }
}

void motorStop() {
  analogWrite(AIN1, 0);
  analogWrite(AIN2, 0);
}

void setup() {
  Serial.begin(115200);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);

  Wire.begin();

  if (!sensor.begin()) {
    Serial.println("VL53L0X not found");
    while (1);
  }

  sensor.startRangeContinuous();
}

void loop() {
  VL53L0X_RangingMeasurementData_t measure;

  sensor.rangingTest(&measure, false);

  bool detected = false;

  if (measure.RangeStatus != 4) {
    int distance = measure.RangeMilliMeter;

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" mm");

    // Only accept reasonable measurements
    if (distance < MAX_VALID_DISTANCE) {
      if (distance < DETECT_DISTANCE) {
        detected = true;
        outOfRangeCount = 0;   // reset counter
      } else {
        outOfRangeCount++;
      }
    } else {
      // Treat absurd readings as out of range
      outOfRangeCount++;
    }
  } else {
    Serial.println("Out of range");
    outOfRangeCount++;
  }

  if (detected && lidState == CLOSED) {
    Serial.println("Opening");
    motorOpen();
    delay(motorDelay);
    motorStop();
    lidState = OPEN;
  }

  if (lidState == OPEN && outOfRangeCount >= OUT_OF_RANGE_THRESHOLD) {
    Serial.println("Closing");
    motorClose();
    delay(motorDelay);
    motorStop();
    lidState = CLOSED;
    outOfRangeCount = 0;
  }

  delay(50);
}