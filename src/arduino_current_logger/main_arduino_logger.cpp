#include <Arduino.h>

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
}
// the loop function runs over and over again forever
void loop() {

  Serial.println("LOG;34.52"); // [A]
  delay(1500); // wait for a second
}