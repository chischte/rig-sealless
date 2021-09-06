#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  Serial.println("EXIT SETUP"); // [A]
  Serial1.println("EXIT SETUP SERIAL 1"); // [A]
}

void loop() {
  Serial.println("LOG;CURRENT_MAX;34.52;"); // [A]
  Serial1.println("Serial1"); // [A]
  delay(1500); // wait for a second
}