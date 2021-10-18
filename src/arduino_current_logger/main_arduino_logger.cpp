#include <Arduino.h>
#include <Insomnia.h> //         https://github.com/chischte/insomnia-delay-library
#include <RunningMedian.h>

// PIN DEFINITION -----------------------------------------------------------
// INPUT:
const byte CURRENT_CLAMP_IN = A0;

// OUTPUT / POWER SUPPLY:
const byte CLAMP_POWER_PIN1 = A1;
const byte CLAMP_POWER_PIN2 = A2;
const byte CLAMP_POWER_PIN3 = A3;
const byte CLAMP_POWER_PIN4 = A4;
const byte CLAMP_POWER_PIN5 = A5;

// CREATE OBJECTS
Insomnia status_print_delay;
Insomnia current_clamp_reset_timeout(12L * 60 * 1000); // 12 minutes

RunningMedian currents_median_cache = RunningMedian(10);

// VARIOUS FUNCTIONS -----------------------------------------------------------

float get_amps_from_clamp() {
  // Measured: 40A = analog read 88
  float amps = float(analogRead(CURRENT_CLAMP_IN)) * 40.0 / 88.0;
  return amps;
}

void print_device_status() {
  if (status_print_delay.delay_time_is_up(5000)) {
    float current = get_amps_from_clamp();
    String status_string = "LOG;CURRENT_LOGGER_RUNNING;";
    status_string += current;
    status_string += ";";
    Serial.println(status_string);
  }
}

bool current_is_over_threshold(float current) {
  const float cycle_start_threshold = 6.0; // [A]
  const float cycle_stop_threshold = 2.0; //  [A]
  static bool current_is_over_threshold = false;

  if (current > cycle_start_threshold) {
    current_is_over_threshold = true;
  }

  if (current < cycle_stop_threshold) {
    current_is_over_threshold = false;
  }
  return current_is_over_threshold;
}

// MANAGE CLAMP POWER SUPPLY ---------------------------------------------------
// The clamp tool has to be switched off and on every few minutes to prevent
// auto power off
// Five output pins are used to draw less current per pin
// Port commands are used to switch the pins simultaneously

void switch_clamp_power_on() {
  PORTC = PORTC | 0b00111110; // set bits 1-5 high, leave rest alone
}

void switch_clamp_power_off() {
  PORTC = PORTC & 0b11000001; // clear bits 1-5, leave rest alone
}

void reset_current_clamp() {
  switch_clamp_power_off();
  delay(1000);
  switch_clamp_power_on();
}

void monitor_current_clamp_timeout() {
  // Reset current clamp if timeout timed out and tool is not measuring current.

  if (current_clamp_reset_timeout.has_timed_out()) {
    if (!current_is_over_threshold(get_amps_from_clamp())) {
      reset_current_clamp();
      current_clamp_reset_timeout.reset_time();
    }
  }
}

// LOG CURRENT -----------------------------------------------------------------
// Start monitoring when current exceeds threshold
// Log max current once when current falls below threshold
// The logged current is the smallest value of the 5 biggest measurements

void log_current(float current) {
  String prefix = "LOG;CURRENT_MAX;";
  String suffix = ";";
  Serial.println(prefix + current + suffix); // [A]
}

void log_max_current() {

  float current = get_amps_from_clamp();

  // Store measurements:
  if (current_is_over_threshold(current)) {
    // Store first measurement:
    if (isnan(currents_median_cache.getLowest())) {
      currents_median_cache.add(current);
      // Store bigger values:
    } else if (current > currents_median_cache.getLowest()) {
      currents_median_cache.add(current);
    }
    // Log current and clear all values after tool-cycle is completed:
  } else if (currents_median_cache.getCount() > 0) {
    log_current(currents_median_cache.getMedian());
    currents_median_cache.clear(); // Sets current-count to 0 and values to nans
  }
}

// SETUP -----------------------------------------------------------------------

void setup() {
  pinMode(CLAMP_POWER_PIN1, OUTPUT);
  pinMode(CLAMP_POWER_PIN2, OUTPUT);
  pinMode(CLAMP_POWER_PIN3, OUTPUT);
  pinMode(CLAMP_POWER_PIN4, OUTPUT);
  pinMode(CLAMP_POWER_PIN5, OUTPUT);
  switch_clamp_power_on();

  Serial.begin(115200);
  Serial.println("EXIT SETUP");
}

// LOOP ------------------------------------------------------------------------

void loop() {

  log_max_current();

  print_device_status();

  monitor_current_clamp_timeout();

  delay(5); // reduce number of measurments

  //Serial.println(get_amps_from_clamp(), 2); // For debug and calibration
}