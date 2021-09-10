#include <Arduino.h>
#include <Insomnia.h> //         https://github.com/chischte/insomnia-delay-library

Insomnia status_print_delay;
Insomnia current_clamp_reset_timeout(120000);

// RS PRO AC/DC CURRENT CLAMP:
//

const byte CURRENT_CLAMP_IN = A0;
const byte CLAMP_POWER_PIN = A4;
const bool run_debug_plot_mode=true;

void setup() {
  pinMode(CLAMP_POWER_PIN,OUTPUT);
  digitalWrite(CLAMP_POWER_PIN,HIGH);
  Serial.begin(115200);
  Serial.println("EXIT SETUP"); // [A]
}

float calculate_amps(int analog_read){
  // measured: 40 AMPS = ANALOG READ 89
  float amps= float(analog_read)*40.0/88.0;
  return amps;
}
void run_plot_mode(){
  float amps= calculate_amps(analogRead(CURRENT_CLAMP_IN));
  Serial.println(amps,2);
  delay(100);
}

void run_log_max_mode(){
  float current = random(2000, 5400) / 100.0;
  String prefix = "LOG;CURRENT_MAX;";
  String suffix = ";";
  Serial.println(prefix + current + suffix); // [A]

  // Print Status:
  if (status_print_delay.delay_time_is_up(3000)) {
    Serial.println("LOG;CURRENT_LOGGER_RUNNING;");
  }
  
}
void monitor_current_clamp_timeout()
{
  if(current_clamp_reset_timeout.has_timed_out()){
    digitalWrite(CLAMP_POWER_PIN,LOW);
    delay(1000);
    digitalWrite(CLAMP_POWER_PIN,HIGH);
    current_clamp_reset_timeout.reset_time();
  }

}


void loop() {
  if(run_debug_plot_mode){
    run_plot_mode();
  }
  else{
    run_log_max_mode();
  }
  
  monitor_current_clamp_timeout();

}