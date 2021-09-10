#include <Arduino.h>
#include <Insomnia.h> //         https://github.com/chischte/insomnia-delay-library

Insomnia status_print_delay;
Insomnia current_clamp_reset_timeout(120000);

// RS PRO AC/DC CURRENT CLAMP:
//

const byte CURRENT_CLAMP_IN = A0;
const byte CLAMP_POWER_PIN = A4;
const bool run_debug_plot_mode = true;

float get_amps_from_clamp() {
  int analog_read = analogRead(CURRENT_CLAMP_IN);

  // measured: 40 AMPS = ANALOG READ 89
  float amps = float(analog_read) * 40.0 / 88.0;
  return amps;
}
void run_plot_mode() {
  Serial.println(get_amps_from_clamp(), 2);
  delay(100);
}

// MANAGE CLAMP POWER SUPPLY ---------------------------------------------------
// The camp too has o be switch off and on every few minutes to prevent
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
  digitalWrite(CLAMP_POWER_PIN, HIGH);
  switch_clamp_power_on();
}
// -----------------------------------------------------------------------------

void print_device_status() {
  if (status_print_delay.delay_time_is_up(3000)) {
    Serial.println("LOG;CURRENT_LOGGER_RUNNING;");
  }
}

// Start monitoring when current exceeds threshold
// Log max current ONCE when current falls below threshold
// Reset max current
// Wait until current exceds threshold again
void run_log_max_mode() {
  static float max_current = -1.0; // -1 means not activated
  float cycle_start_threshold = 20.0; // [A]
  float cycle_stop_threshold = 15.0; // [A]
  static bool current_is_over_threshold = false;

  float current_current = get_amps_from_clamp();

  if (current_current > cycle_start_threshold) {
    current_is_over_threshold = true;
  }
  if (current_current < cycle_stop_threshold) {
    current_is_over_threshold = false;
  }

  if (current_is_over_threshold) {
    if (current_current > max_current) {
      max_current = current_current;
    }
  }

  if (!current_is_over_threshold) {
    if (max_current > 0) {
      //log current
      max_current = -1;
    }
  }

  float current = random(2000, 5400) / 100.0;
  String prefix = "LOG;CURRENT_MAX;";
  String suffix = ";";

  Serial.println(prefix + current + suffix); // [A]

  print_device_status();
}

void monitor_current_clamp_timeout() {
  if (current_clamp_reset_timeout.has_timed_out()) {
    reset_current_clamp();
    current_clamp_reset_timeout.reset_time();
  }
}

// SETUP -----------------------------------------------------------------------

void setup() {
  pinMode(CLAMP_POWER_PIN, OUTPUT);
  digitalWrite(CLAMP_POWER_PIN, HIGH);
  Serial.begin(115200);
  Serial.println("EXIT SETUP"); // [A]
}

// LOOP ------------------------------------------------------------------------

void loop() {

  if (run_debug_plot_mode) {
    run_plot_mode();
  } else {
    run_log_max_mode();
  }

  monitor_current_clamp_timeout();
}