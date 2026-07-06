#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>

VL53L0X sensor;

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

void motorStop()
{
  analogWrite(AIN1, 0);
  analogWrite(AIN2, 0);
}

// Reusable function to ramp up the motor, hold, and stop
void driveMotorSequence(int drivePin, int gndPin)
{
  for (int s = 0; s <= MOTOR_SPEED; s += 10)
  {
    analogWrite(drivePin, s);
    analogWrite(gndPin, 0);
    delay(5);
  }
  
  delay(motorDelay);
  motorStop();
}

void resetSensor()
{
  Serial.println("Resetting sensor due to I2C lockup...");
  
  // Re-initialize the I2C bus (helps clear hung I2C states on many microcontrollers)
  Wire.begin(); 
  
  sensor.setTimeout(500);
  if (!sensor.init())
  {
    Serial.println("Failed to reconnect to VL53L0X!");
  }
  else
  {
    Serial.println("Sensor reconnected successfully.");
    sensor.startContinuous();
  }

  // Reset lockup trackers regardless of success so it doesn't get stuck in an infinite reset loop
  lastDistance = -1;
  sameValueCount = 0;
}

void setup()
{
  Serial.begin(115200);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);

  Wire.begin();

  sensor.setTimeout(500);
  if (!sensor.init())
  {
    Serial.println("VL53L0X not found");
    while (1)
      ;
  }

  sensor.startContinuous();
}

void loop()
{
  uint16_t distance = sensor.readRangeContinuousMillimeters();
  bool detected = false;

  // Outer check handles the MAX_VALID_DISTANCE (acting as range status validation)
  if (!sensor.timeoutOccurred() && distance < MAX_VALID_DISTANCE)
  {
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

    // Simplified logic: we already know it's < MAX_VALID_DISTANCE
    if (distance < DETECT_DISTANCE)
    {
      detected = true;
      outOfRangeCount = 0;
    }
    else
    {
      outOfRangeCount++;
    }
  }
  else
  {
    if (sensor.timeoutOccurred()) {
        Serial.println("Sensor Timeout!");
        resetSensor(); // Attempt to recover if the sensor totally timed out
    } else {
        Serial.println("Out of range");
    }
    
    outOfRangeCount++;
    lastDistance = -1;
    sameValueCount = 0;
  }

  // ---------------- Automatic Lid Control ----------------
  if (motorState == MOTOR_STOPPED)
  {
    if (detected && lidState == CLOSED)
    {
      Serial.println("Opening");
      driveMotorSequence(AIN1, AIN2);
      lidState = OPEN;
    }
    else if (lidState == OPEN && outOfRangeCount >= OUT_OF_RANGE_THRESHOLD)
    {
      Serial.println("Closing");
      driveMotorSequence(AIN2, AIN1);
      lidState = CLOSED;
      outOfRangeCount = 0;
    }

    delay(30);
  }
}