#include <Arduino.h>
#include <Insomnia.h> //         https://github.com/chischte/insomnia-delay-library

Insomnia status_print_delay;
// Insomnia current_clamp_reset_timeout(3000);
Insomnia current_clamp_reset_timeout(3000);

// PIN DEFINITION -----------------------------------------------------------
// INPUT:
const byte CURRENT_CLAMP_IN = A0;

// OUTPUT / POWER SUPPLY:
const byte CLAMP_POWER_PIN1 = A3;
const byte CLAMP_POWER_PIN2 = A4;

// VARIOUS FUNCTIONS -----------------------------------------------------------

void print_device_status() {
  if (status_print_delay.delay_time_is_up(3000)) {
    Serial.println("LOG;CURRENT_LOGGER_RUNNING;");
  }
}

float get_amps_from_clamp() {
  // Measured: 40A = analog read 88
  float amps = float(analogRead(CURRENT_CLAMP_IN)) * 40.0 / 88.0;
  return amps;
}

// MANAGE CLAMP POWER SUPPLY ---------------------------------------------------
// The clamp tool has to be switched off and on every few minutes to prevent
// auto power off
// Two output pins are used to draw less current per pin
// Ports command are used to switch the pins simultaneously
// PIN A3 = PORT C BIT 3
// PIN A4 = PORT C BIT 4

void switch_clamp_power_on() {
  PORTB = PORTB | 0b00011000; // set bit 3 and 4 high, leave rest alone
}

void switch_clamp_power_off() {
  PORTB = PORTB & 0b11100111; // clear bit 3 and 4, leave rest alone
}

void reset_current_clamp() {
  switch_clamp_power_off();
  delay(1000);
  switch_clamp_power_on();
}

void monitor_current_clamp_timeout() {
  if (current_clamp_reset_timeout.has_timed_out()) {
    reset_current_clamp();
    current_clamp_reset_timeout.reset_time();
  }
}

// LOG CURRENT -----------------------------------------------------------------
// Start monitoring when current exceeds threshold
// Log max current once when current falls below threshold
// Reset max current

void log_current(float current) {
  String prefix = "LOG;CURRENT_MAX;";
  String suffix = ";";
  Serial.println(prefix + current + suffix); // [A]
}

bool current_is_over_threshold(float current) {
  const float cycle_start_threshold = 15.0; // [A]
  const float cycle_stop_threshold = 10.0; //  [A]
  static bool current_is_over_threshold = false;

  if (current > cycle_start_threshold) {
    current_is_over_threshold = true;
  }

  if (current < cycle_stop_threshold) {
    current_is_over_threshold = false;
  }
  return current_is_over_threshold;
}

void log_max_current() {
  static float max_current = 0;

  float current = get_amps_from_clamp();

  if (current_is_over_threshold(current)) {
    if (current > max_current) {
      max_current = current;
    }
  } else if (max_current > 0) {
    log_current(max_current);
    max_current = 0; // reset max current.
  }
}

// SETUP -----------------------------------------------------------------------

void setup() {
  pinMode(CLAMP_POWER_PIN1, OUTPUT);
  pinMode(CLAMP_POWER_PIN2, OUTPUT);
  switch_clamp_power_on();

  Serial.begin(115200);
  Serial.println("EXIT SETUP");
}

// LOOP ------------------------------------------------------------------------

void loop() {

  log_max_current();

  print_device_status();

  monitor_current_clamp_timeout();

  // Serial.println(get_amps_from_clamp(), 2); // For debug and calibration
}