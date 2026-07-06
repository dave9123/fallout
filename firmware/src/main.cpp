#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

Adafruit_VL53L0X sensor;

// DRV8833 pins
const int AIN1 = 5;
const int AIN2 = 3;

const int DETECT_DISTANCE = 500;     // mm
const int MAX_VALID_DISTANCE = 8000; // Ignore anything above this
const int motorDelay = 300;          // ms
const int MOTOR_SPEED = 180;         // 0-255

// Number of consecutive "no object" readings before closing
const int OUT_OF_RANGE_THRESHOLD = 3;

// Sensor stuck detection
const int SAME_VALUE_THRESHOLD = 10;
int lastDistance = -1;
int sameValueCount = 0;

int outOfRangeCount = 0;

enum LidState
{
  CLOSED,
  OPEN
};

enum MotorState
{
  MOTOR_STOPPED,
  MOTOR_OPENING,
  MOTOR_CLOSING
};

LidState lidState = CLOSED;
MotorState motorState = MOTOR_STOPPED;

unsigned long motorStartTime = 0;

void motorOpen()
{
  for (int s = 0; s <= MOTOR_SPEED; s += 10)
  {
    analogWrite(AIN1, s);
    analogWrite(AIN2, 0);
    delay(5);
  }
}

void motorClose()
{
  for (int s = 0; s <= MOTOR_SPEED; s += 10)
  {
    analogWrite(AIN1, 0);
    analogWrite(AIN2, s);
    delay(5);
  }
}

void motorStop()
{
  analogWrite(AIN1, 0);
  analogWrite(AIN2, 0);
}

void setup()
{
  Serial.begin(115200);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);

  Wire.begin();

  if (!sensor.begin())
  {
    Serial.println("VL53L0X not found");
    while (1)
      ;
  }

  sensor.startRangeContinuous();
}

void loop()
{
  VL53L0X_RangingMeasurementData_t measure;
  sensor.rangingTest(&measure, false);

  bool detected = false;

  if (measure.RangeStatus != 4)
  {
    int distance = measure.RangeMilliMeter;

    // Detect sensor lockup (same reading repeatedly)
    if (distance == lastDistance)
    {
      sameValueCount++;

      if (sameValueCount > SAME_VALUE_THRESHOLD)
      {
        resetSensor();
        delay(50);
        return;
      }
    }
    else
    {
      lastDistance = distance;
      sameValueCount = 0;
    }

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" mm");

    // Only accept reasonable measurements
    if (distance < MAX_VALID_DISTANCE)
    {
      if (distance < DETECT_DISTANCE)
      {
        detected = true;
        outOfRangeCount = 0; // reset counter
      }
      else
      {
        outOfRangeCount++;
      }
    }
    else
    {
      // Treat absurd readings as out of range
      outOfRangeCount++;
    }
  }
  else
  {
    Serial.println("Out of range");
    outOfRangeCount++;

    // Reset repeated-reading detection
    lastDistance = -1;
    sameValueCount = 0;
  }

  // ---------------- Automatic Lid Control ----------------
  if (motorState == MOTOR_STOPPED)
  {

    if (detected && lidState == CLOSED)
    {
      Serial.println("Opening");
      motorOpen();
      delay(motorDelay);
      motorStop();
      lidState = OPEN;
    }

    if (lidState == OPEN && outOfRangeCount >= OUT_OF_RANGE_THRESHOLD)
    {
      Serial.println("Closing");
      motorClose();
      delay(motorDelay);
      motorStop();
      lidState = CLOSED;
      outOfRangeCount = 0;
    }

    delay(30);
  }