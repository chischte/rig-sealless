#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  Serial.println("EXIT SETUP"); // [A]
}

void loop() {
  float current = random(2000, 5400)/100.0;
  String prefix = "LOG;CURRENT_MAX;";
  String suffix = ";";
  Serial.println(prefix + current + suffix); // [A]

  delay(500); // wait for a second
}