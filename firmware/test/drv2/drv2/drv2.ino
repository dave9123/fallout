// Simple test untuk DRV8833
// Wiring:
//   AIN1 -> GPIO2
//   AIN2 -> GPIO1
//   STBY -> 5V (langsung, tidak dikontrol dari ESP32)
//   VM   -> 5V
//   GND  -> GND (ESP32 & sumber 5V harus satu GND)
//   Motor -> OUT1 & OUT2

#define AIN1 2
#define AIN2 1

void setup() {
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);

  // pastikan motor diam dulu saat start
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);

  Serial.begin(115200);
  delay(1000);
  Serial.println("Mulai test motor...");
}

void loop() {
  // Putar arah 1
  Serial.println("Putar arah 1 (AIN1 HIGH, AIN2 LOW)");
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  delay(2000);

  // Berhenti (coast)
  Serial.println("Stop");
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  delay(1000);

  // Putar arah 2
  Serial.println("Putar arah 2 (AIN1 LOW, AIN2 HIGH)");
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  delay(2000);

  // Berhenti (coast)
  Serial.println("Stop");
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  delay(2000);
}