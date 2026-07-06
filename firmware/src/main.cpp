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

// Rapid back-and-forth movement to simulate munching
void munchAnimation()
{
  Serial.println("Munching...");
  const int munchSpeed = 160; 
  const int munchTime = 120; // Short duration for a "twitch" effect
  const int chomps = 3;      // Number of chews

  for (int i = 0; i < chomps; i++)
  {
    // Open slightly (AIN1 high)
    analogWrite(AIN1, munchSpeed);
    analogWrite(AIN2, 0);
    delay(munchTime);

    // Close firmly (AIN2 high)
    analogWrite(AIN1, 0);
    analogWrite(AIN2, munchSpeed);
    delay(munchTime);
  }
  
  // Ensure the motor stops fully closed
  motorStop();
}

// Re-initialize the I2C bus and sensor on lockup/disconnect
void resetSensor()
{
  Serial.println("Attempting to reset and reconnect sensor...");
  
  Wire.begin();
  
  if (!sensor.init())
  {
    Serial.println("Failed to reconnect to VL53L0X!");
  }
  else
  {
    Serial.println("Sensor reconnected successfully.");
    sensor.setTimeout(500);
    sensor.startContinuous();
  }

  lastDistance = -1;
  sameValueCount = 0;
}

// Handle incoming 1-letter serial commands
void processSerialCommands()
{
  if (Serial.available() > 0)
  {
    char cmd = Serial.read();
    
    switch (cmd)
    {
      case 'o':
      case 'O':
        if (lidState == CLOSED)
        {
          Serial.println("Manual Override: Opening");
          driveMotorSequence(AIN1, AIN2);
          lidState = OPEN;
          outOfRangeCount = 0; // Reset so it doesn't immediately close
        }
        break;

      case 'c':
      case 'C':
        if (lidState == OPEN)
        {
          Serial.println("Manual Override: Closing");
          driveMotorSequence(AIN2, AIN1);
          lidState = CLOSED;
        }
        break;

      case 'm':
      case 'M':
        Serial.println("Manual Override: Munch");
        munchAnimation();
        // Make sure state reflects closed after munching
        lidState = CLOSED; 
        break;

      case 'r':
      case 'R':
        Serial.println("Manual Override: Reset Sensor");
        resetSensor();
        break;

      default:
        // Ignore spaces, newlines, and unrecognized characters
        break;
    }
  }
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
    Serial.println("VL53L0X not found at startup");
  }
  else
  {
    sensor.startContinuous();
  }

  Serial.println("System Ready. Send 'o' to open, 'c' to close, 'm' to munch, 'r' to reset.");
}

void loop()
{
  // 1. Check for manual serial commands first
  processSerialCommands();

  // 2. Read sensor data
  uint16_t distance = sensor.readRangeContinuousMillimeters();
  bool detected = false;

  if (!sensor.timeoutOccurred() && distance < MAX_VALID_DISTANCE)
  {
    if (distance == lastDistance)
    {
      sameValueCount++;
      if (sameValueCount > SAME_VALUE_THRESHOLD)
      {
        Serial.println("Sensor locked up (same value).");
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

    // Only print if reasonable to avoid console spam during timeouts
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" mm");

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
    if (sensor.timeoutOccurred()) 
    {
      Serial.println("Sensor Timeout! I2C may be disconnected.");
      resetSensor();
      delay(50);
      return;
    } 
    
    outOfRangeCount++;
    lastDistance = -1;
    sameValueCount = 0;
  }

  // 3. Automatic Lid Control
  if (motorState == MOTOR_STOPPED)
  {
    if (detected && lidState == CLOSED)
    {
      Serial.println("Opening (Auto)");
      driveMotorSequence(AIN1, AIN2);
      lidState = OPEN;
    }
    else if (lidState == OPEN && outOfRangeCount >= OUT_OF_RANGE_THRESHOLD)
    {
      Serial.println("Closing (Auto)");
      driveMotorSequence(AIN2, AIN1);
      
      delay(200);       
      munchAnimation(); 

      lidState = CLOSED;
      outOfRangeCount = 0;
    }

    delay(30);
  }
}