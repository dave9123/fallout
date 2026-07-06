#include <Wire.h>
#include <Adafruit_VL53L0X.h>

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

void setup() {
    Serial.begin(115200);
    Wire.begin();

    if (!lox.begin()) {
        Serial.println("Failed to initialize VL53L0X! Check connections.");
        while (1);
    }
    Serial.println("VL53L0X Initialized.");
}

void loop() {
    VL53L0X_RangingMeasurementData_t measure;
    lox.rangingTest(&measure, false);

    if (measure.RangeStatus != 4) { // Status 4 means no object detected
        Serial.print("Distance: ");
        Serial.print(measure.RangeMilliMeter);
        Serial.println(" mm");
    } else {
        Serial.println("Out of range.");
    }

    delay(500);
}

  

  

