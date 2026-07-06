/*
 * Smart Trash Bin Control
 *
 * Connections:
 * - VL53LOX SCL -> ESP32-S3 Pin 5
 * - VL53LOX SDA -> ESP32-S3 Pin 6
 * - DRV8833 AIN1 -> ESP32-S3 Pin 1
 * - DRV8833 AIN2 -> ESP32-S3 Pin 2
 * - DRV8833 STBY -> ESP32-S3 3V3
 * - DRV8833 VM -> ESP32-S3 5V
 * - DRV8833 GND -> ESP32-S3 GND
 * - VL53LOX VCC -> ESP32-S3 3V3
 * - VL53LOX GND -> ESP32-S3 GND
 */

#include <Wire.h>
#include <VL53L0X.h>

VL53L0X sensor;

const int motorPin1 = 1; // DRV8833 AIN1
const int motorPin2 = 2; // DRV8833 AIN2
const int distanceThreshold = 200; // Distance threshold in mm

void setup() {
  Serial.begin(115200);
  Wire.begin(6, 5);
  
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  
  sensor.setTimeout(500);
  if (!sensor.init()) {
    Serial.println("Failed to detect and initialize sensor!");
    while (1);
  }
  sensor.startContinuous();
}

void loop() {
  uint16_t distance = sensor.readRangeContinuousMillimeters();
  
  if (sensor.timeoutOccurred()) {
    Serial.println("Sensor timeout!");
  } else {
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" mm");
    
    if (distance < distanceThreshold) {
      openLid();
    } else {
      closeLid();
    }
  }
  delay(100);
}

void openLid() {
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  delay(500); // durasi gerak buka, sesuaikan dengan mekanisme
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW); // stop
}

void closeLid() {
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
}