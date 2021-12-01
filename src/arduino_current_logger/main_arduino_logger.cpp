#include <Arduino.h>
#include <Insomnia.h> //         https://github.com/chischte/insomnia-delay-library
#include <RunningMedian.h>

// PIN DEFINITION -----------------------------------------------------------
// INPUT:
const byte CURRENT_CLAMP_IN = A0;

// CREATE OBJECTS --------------------------------------------------------------
Insomnia status_print_delay;

// STORE PEAK VALUES:
RunningMedian currents_median_cache = RunningMedian(20);

// VARIOUS FUNCTIONS -----------------------------------------------------------

float calculate_amps_from_adc(int adc_value) {
  float adc_range = 1024;
  float amps_at_max = 100;
  float amps = float(adc_value) / adc_range * amps_at_max;
  return amps;
}

float get_amps_from_clamp() {
  float amps = calculate_amps_from_adc(analogRead(CURRENT_CLAMP_IN));
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
  const float cycle_start_threshold = 25.0; // [A]
  const float cycle_stop_threshold = 7; // [A]
  static bool current_is_over_threshold = false;

  if (current > cycle_start_threshold) {
    current_is_over_threshold = true;
  }

  if (current < cycle_stop_threshold) {
    current_is_over_threshold = false;
  }
  return current_is_over_threshold;
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

  Serial.begin(115200);
  Serial.println("EXIT SETUP");
}

// LOOP ------------------------------------------------------------------------

void loop() {

  print_device_status();

  log_max_current();

  // delayMicroseconds(100); // reduce number of measurments

  //Serial.println(get_amps_from_clamp(), 2); // For debug and calibration
}