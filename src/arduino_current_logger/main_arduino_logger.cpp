#include <Arduino.h>
#include <Insomnia.h> //         https://github.com/chischte/insomnia-delay-library

Insomnia status_print_delay;

// RS PRO AC/DC CURRENT CLAMP:
//

void setup() {
  Serial.begin(115200);
  Serial.println("EXIT SETUP"); // [A]
}

void loop() {
  float current = random(2000, 5400) / 100.0;
  String prefix = "LOG;CURRENT_MAX;";
  String suffix = ";";
  Serial.println(prefix + current + suffix); // [A]

  // Print Status:
  if (status_print_delay.delay_time_is_up(2000)) {
    Serial.println("LOG;CURRENT_LOGGER_RUNNING;");
  }

}