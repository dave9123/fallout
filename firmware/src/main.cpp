#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

Adafruit_VL53L0X sensor;

// DRV8833 pins
const int AIN1 = 5;
const int AIN2 = 3;

// Settings
const int DETECT_DISTANCE = 500;      // mm
const int MAX_VALID_DISTANCE = 8000;  // Ignore absurd readings
const int MOTOR_SPEED = 200;
const unsigned long MOTOR_TIME_CLOSING = 300; // ms
const unsigned long MOTOR_TIME_OPENING = 800;

// Number of consecutive "no object" readings before closing
const int OUT_OF_RANGE_THRESHOLD = 3;

int outOfRangeCount = 0;

enum LidState {
  CLOSED,
  OPEN
};

enum MotorState {
  MOTOR_STOPPED,
  MOTOR_OPENING,
  MOTOR_CLOSING
};

LidState lidState = CLOSED;
MotorState motorState = MOTOR_STOPPED;

unsigned long motorStartTime = 0;

void startClose() {
  analogWrite(AIN1, MOTOR_SPEED);
  analogWrite(AIN2, 0);

  motorState = MOTOR_OPENING;
  motorStartTime = millis();

  Serial.println("Opening");
}

void startOpen() {
  analogWrite(AIN1, 0);
  analogWrite(AIN2, MOTOR_SPEED);

  motorState = MOTOR_CLOSING;
  motorStartTime = millis();

  Serial.println("Closing");
}

void stopMotor() {
  analogWrite(AIN1, 0);
  analogWrite(AIN2, 0);

  if (motorState == MOTOR_OPENING)
    lidState = OPEN;
  else if (motorState == MOTOR_CLOSING)
    lidState = CLOSED;

  motorState = MOTOR_STOPPED;
  Serial.println("Stopped");
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

  // ---------------- Serial Commands ----------------
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "u") {
      if (motorState == MOTOR_STOPPED)
        startClose();
    }
    else if (cmd == "d") {
      if (motorState == MOTOR_STOPPED)
        startOpen();
    }
    else if (cmd == "s") {
      stopMotor();
    }
  }

  // ---------------- Sensor ----------------
  VL53L0X_RangingMeasurementData_t measure;
  sensor.rangingTest(&measure, false);

  bool detected = false;

  if (measure.RangeStatus != 4) {
    int distance = measure.RangeMilliMeter;

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" mm");

    if (distance < MAX_VALID_DISTANCE) {
      if (distance < DETECT_DISTANCE) {
        detected = true;
        outOfRangeCount = 0;
      } else {
        outOfRangeCount++;
      }
    } else {
      outOfRangeCount++;
    }
  } else {
    Serial.println("Out of range");
    outOfRangeCount++;
  }

  // Automatic lid control (only when motor is idle)
  if (motorState == MOTOR_STOPPED) {

    if (detected && lidState == CLOSED) {
      startClose();
    }

    if (lidState == OPEN &&
        outOfRangeCount >= OUT_OF_RANGE_THRESHOLD) {
      startOpen();
      outOfRangeCount = 0;
    }
  }

  // ---------------- Motor Timer ----------------
  if ((motorState == MOTOR_CLOSING &&
      millis() - motorStartTime >= MOTOR_TIME_CLOSING) || (motorState == MOTOR_OPENING &&
  millis() - motorStartTime >= MOTOR_TIME_OPENING)) stopMotor();
}