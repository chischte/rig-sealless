/*
 * *****************************************************************************
 * RIG-SEALLESS
 * *****************************************************************************
 * Program for a fully-automatic endurance test rig for a mechanical tool
 * -----------------------------------------------------------------------------
 * Michael Wettstein
 * August 2021, Zürich
 * -----------------------------------------------------------------------------
 * RUNTIME:
 * Measured runtime: 220-250 micros
 * -----------------------------------------------------------------------------
 * *****************************************************************************
 */

// INCLUDE HEADERS *************************************************************

#include <ArduinoSTL.h> //      https://github.com/mike-matera/ArduinoSTL
#include <Controllino.h> //     PIO Controllino Library
#include <Cylinder.h> //        https://github.com/chischte/cylinder-library
#include <Debounce.h> //        https://github.com/chischte/debounce-library
#include <EEPROM_Counter.h> //  https://github.com/chischte/eeprom-counter-library
#include <Insomnia.h> //        https://github.com/chischte/insomnia-delay-library
#include <Nextion.h> //         PIO Nextion library
#include <SD.h> //              PIO Adafruit SD library

#include <controllino_plc/alias_colino.h> //      aliases when using an Arduino instead of a Controllino
#include <controllino_plc/cycle_step.h> //        blueprint of a cycle step
#include <controllino_plc/state_controller.h> //  keeps track of machine states

// DECLARE FUNCTIONS IF NEEDED FOR THE COMPILER: *******************************

void clear_text_field(String textField);
void hide_info_field();
void page_0_push(void *ptr);
void page_1_push(void *ptr);
void page_2_push(void *ptr);
void display_loop_page_1_left_side();
void display_loop_page_1_right_side();
void display_loop_page_2_left_side();
void display_loop_page_2_right_side();
void display_text_in_info_field(String text);
void update_cycle_name();
void update_upper_slider_value();
void update_lower_slider_value();
void update_upper_counter_value();
void update_switches_page_2_left();
void update_lower_counter_value();
void reset_lower_counter_value();
void increase_slider_value(int eeprom_value_number);
void decrease_slider_value(int eeprom_value_number);
void update_field_values_page_2();
void show_info_field();
void display_temperature();
void display_text_in_field(String text, String textField);
void error_stop_machine(String error_message);
void power_on_electrocylinder();
String get_main_cycle_display_string();
String add_suffix_to_eeprom_value(int eeprom_value_number, String suffix);

// DEFINE COUNTER ENUM ******************************************

enum counter {
  longtime_counter, //
  shorttime_counter, //
  cycle_duration, //
  max_temperature, //
  end_of_counter_enum // keep this entry
};
int counter_no_of_values = end_of_counter_enum;

// DEFINE PINS / GENERATE OBJECTS ************************************************************

EEPROM_Counter counter;
State_controller state_controller;

// INPUT PINS:
const byte PRESSURE_SENSOR_PIN = CONTROLLINO_A3;
Debounce sensor_sledge_startposition(CONTROLLINO_A4);
Debounce sensor_sledge_endposition(CONTROLLINO_A5);
Debounce sensor_foerderzylinder_in(CONTROLLINO_A7); // BROWN
Debounce sensor_foerderzylinder_out(CONTROLLINO_A6); // GREEN
Debounce emergency_stop_signal(CONTROLLINO_A10); //
Debounce bandsensor_zufuhr_oben(CONTROLLINO_A0); //
Debounce bandsensor_zufuhr_unten(CONTROLLINO_A1); //
Debounce bandbogensensor(CONTROLLINO_A8);
Debounce sensor_messerzylinder(CONTROLLINO_A9);
Debounce hydraulic_safety_sensor_1(CONTROLLINO_A11); //
Debounce hydraulic_safety_sensor_2(CONTROLLINO_A12); //

// OUTPUT PINS:
const byte FOERDERZYLINDER_LOGIC_POWER_RELAY = CONTROLLINO_R6; // WHITE // turn on >=50ms after start of "load voltage"
const byte TRENNRELAIS_ZYLINDER_1 = CONTROLLINO_R4; // turn off >=100ms before logic power off
const byte TRENNRELAIS_ZYLINDER_2 = CONTROLLINO_R5; // turn off >=100ms before logic power off
const byte FOERDERZYLINDER_MOVE_IN = CONTROLLINO_D9; // GREY
const byte FOERDERZYLINDER_MOVE_OUT = CONTROLLINO_D10; // PINK
const byte TEMP_SENSOR_PIN = CONTROLLINO_A2;
Cylinder cylinder_kuehlluft(CONTROLLINO_D13);
Cylinder cylinder_schlittenzuluft(CONTROLLINO_D4);
Cylinder cylinder_schlittenabluft(CONTROLLINO_R8);
Cylinder cylinder_auswerfer(CONTROLLINO_D3);
Cylinder cylinder_spanntaste(CONTROLLINO_D5);
Cylinder cylinder_crimptaste(CONTROLLINO_D2);
Cylinder cylinder_wippenhebel(CONTROLLINO_D0);
Cylinder cylinder_messer(CONTROLLINO_D1);
Cylinder cylinder_main_hauptluft(CONTROLLINO_D12);
Cylinder cylinder_hydr_pressure_valve(CONTROLLINO_D7);
Cylinder cylinder_hydr_vorklemme(CONTROLLINO_D15);
Cylinder cylinder_hydr_nachklemme(CONTROLLINO_D14);
Cylinder cylinder_hydr_vorschubklemme(CONTROLLINO_D6);
Cylinder cylinder_hydr_power_supply(CONTROLLINO_D11);

Insomnia nex_reset_button_timeout(10000); // pushtime to reset counter
Insomnia print_interval_timeout(1000);
Insomnia erase_force_value_timeout(5000);
Insomnia log_force_value_timeout(1000);
Insomnia machine_stopped_error_timeout(7000); // electrocylinder takes up to 20" to find start position
Insomnia hydraulic_timeout(120000); // hydraulic main power shuts down if timed out
Insomnia bandsensor_timeout(5000);
Insomnia bandbogen_timeout(1500);
Insomnia pressure_update_delay;
Insomnia temperature_update_delay;
Insomnia cycle_step_delay;

// GLOBAL VARIABLES: -----------------------------------------------------------
unsigned long cycle_start_millis = millis();
char reset_count = 0; // to monitor how many resets have been made
char strap_count_for_knife = 0; // only cut strap every n times

// LOGS AND EMAILS: ------------------------------------------------------------

void send_log_cycle_reset(long value) {
  Serial1.print("LOG;CYCLE_RESET;");
  Serial1.print(value);
  Serial1.println(";");
}

void send_log_cycle_total(long value) {
  Serial1.print("LOG;CYCLE_TOTAL;");
  Serial1.print(value);
  Serial1.println(";");
}

void send_log_force_tension(long value) {
  if (value > 100) {
    Serial1.print("LOG;FORCE_TENSION;");
    Serial1.print(value);
    Serial1.println(";");
  }
}

void send_log_start_tensioning() { //
  Serial1.println("LOG;START_TENSION;");
}

void send_log_start_crimping() { //
  Serial1.println("LOG;START_CRIMP;");
}

void send_email_machine_stopped() { //
  Serial1.println("EMAIL;MACHINE_STOPPED;");
}

void send_email_button_pushed() { //
  Serial1.println("EMAIL;BUTTON_PUSHED;");
}

// NEXTION DISPLAY OBJECTS *****************************************************

// PAGE 0 ----------------------------------------------------------------------
NexPage nex_page_0 = NexPage(0, 0, "page0");
// PAGE 1 - LEFT SIDE ----------------------------------------------------------
NexPage nex_page_1 = NexPage(1, 0, "page1");
NexButton button_previous_step = NexButton(1, 6, "b1");
NexButton button_next_step = NexButton(1, 7, "b2");
NexButton button_reset_rig = NexButton(1, 5, "b0");
NexDSButton switch_play_pause = NexDSButton(1, 3, "bt0");
NexDSButton switch_step_auto_mode = NexDSButton(1, 4, "bt1");
// PAGE 1 - RIGHT SIDE ---------------------------------------------------------
// NexButton button_spanntaste = NexButton(1, 9, "b4");
// NexButton button_crimptaste = NexButton(1, 8, "b3");
NexButton button_schneiden = NexButton(1, 14, "b5");
NexButton button_schlitten = NexButton(1, 1, "b6");
NexDSButton switch_vorklemme = NexDSButton(1, 16, "bt2");
NexDSButton switch_nachklemme = NexDSButton(1, 17, "bt4");
NexDSButton switch_wippenhebel = NexDSButton(1, 10, "bt5");
NexDSButton switch_entlueften = NexDSButton(1, 13, "bt3");
// PAGE 2 - LEFT SIDE ----------------------------------------------------------
NexPage nex_page_2 = NexPage(2, 0, "page2");
NexButton button_slider_1_left = NexButton(2, 5, "b1");
NexButton button_slider_1_right = NexButton(2, 6, "b2");
NexButton button_slider_2_left = NexButton(2, 16, "b5");
NexButton button_slider_2_right = NexButton(2, 17, "b6");

// PAGE 2 - RIGHT SIDE ---------------------------------------------------------
NexButton button_reset_shorttime_counter = NexButton(2, 12, "b4");

// NEXTION DISPLAY - TOUCH EVENT LIST ******************************************

NexTouch *nex_listen_list[] = { //
    // PAGE 0:
    &nex_page_0,
    // PAGE 1 LEFT:
    &nex_page_1, &button_previous_step, &button_next_step, &button_reset_rig, &switch_play_pause,
    &switch_step_auto_mode,
    // PAGE 1 RIGHT:
    &button_schneiden, &switch_vorklemme, &switch_nachklemme, &switch_wippenhebel, &switch_entlueften,
    &button_schlitten,
    // PAGE 2 LEFT:
    &nex_page_2, &button_slider_1_left, &button_slider_1_right, &nex_page_2, &button_slider_2_left,
    &button_slider_2_right,
    // PAGE 2 RIGHT:
    &button_reset_shorttime_counter,
    // END OF LISTEN LIST:
    NULL};

// VARIABLES TO MONITOR NEXTION DISPLAY STATES *********************************

bool nex_state_schlittenabluft;
// bool nex_state_spanntaste;
// bool nex_state_crimptaste;
bool nex_state_vorklemme;
bool nex_state_nachklemme;
bool nex_state_wippenhebel;
bool nex_state_schlittenzuluft;
bool nex_state_messer;
bool nex_state_machine_running;
bool nex_state_step_mode = true;
byte nex_prev_cycle_step;
byte nex_current_page = 0;
long nex_cycle_duration;
long nex_max_temperature;
long nex_shorttime_counter;
long nex_longtime_counter;
int nex_prev_current_temperature;

// CREATE VECTOR CONTAINER FOR THE CYCLE STEPS OBJECTS ************************

int Cycle_step::object_count = 0; // enable object counting
std::vector<Cycle_step *> main_cycle_steps;

// NON NEXTION FUNCTIONS *******************************************************

void reset_flag_of_current_step() { main_cycle_steps[state_controller.get_current_step()]->reset_flags(); }

void foerderzylinder_zurueck() {
  digitalWrite(FOERDERZYLINDER_MOVE_OUT, HIGH);
  digitalWrite(FOERDERZYLINDER_MOVE_IN, LOW);
}

void foerderzylinder_foerdern() {
  digitalWrite(FOERDERZYLINDER_MOVE_OUT, LOW);
  digitalWrite(FOERDERZYLINDER_MOVE_IN, HIGH);
}

void foerderzylinder_stop() {
  digitalWrite(FOERDERZYLINDER_MOVE_OUT, LOW);
  digitalWrite(FOERDERZYLINDER_MOVE_IN, LOW);
}

void vent_sledge() {
  cylinder_schlittenzuluft.set(0);
  cylinder_schlittenabluft.set(0);
}

void stroke_wippenhebel() {
  cylinder_wippenhebel.set(1);
  delay(500);
  cylinder_wippenhebel.set(0);
}

void reset_pneumatics() {
  cylinder_messer.set(0);
  cylinder_schlittenzuluft.set(0);
  cylinder_schlittenabluft.set(0);
  cylinder_wippenhebel.set(0);
  cylinder_spanntaste.set(0);
  cylinder_crimptaste.set(0);
  cylinder_auswerfer.set(0);
}

void reset_hydraulics() {
  hydraulic_timeout.reset_time();
  cylinder_hydr_power_supply.set(1);
  cylinder_hydr_pressure_valve.set(1);
  cylinder_hydr_vorklemme.set(0);
  cylinder_hydr_nachklemme.set(0);
  cylinder_hydr_vorschubklemme.set(0);
  delay(1000); // time for Hydraulic cylinders to move back
}

void reset_machine_states() {
  state_controller.set_machine_stop();
  state_controller.set_error_mode(false);
  reset_flag_of_current_step();
  state_controller.set_current_step_to(0);
  reset_flag_of_current_step();
  clear_text_field("t4");
  hide_info_field();
}

void reset_electrocylinder() { //
  foerderzylinder_zurueck();
}

void reset_machine() {
  if (!emergency_stop_signal.get_raw_button_state()) {
    cylinder_hydr_pressure_valve.set(1);
    cylinder_main_hauptluft.set(1);
  }
  reset_machine_states();
  reset_pneumatics();
  if (!digitalRead(FOERDERZYLINDER_LOGIC_POWER_RELAY)) {
    // Logic has been powerded off
    power_on_electrocylinder(); // 8s delay
  }
  reset_electrocylinder();
  reset_hydraulics();
}

long measure_runtime() {
  static long previous_micros = micros();
  long time_elapsed = micros() - previous_micros;
  previous_micros = micros();
  return time_elapsed;
}

void move_sledge() {
  cylinder_schlittenzuluft.set(1);
  cylinder_schlittenabluft.set(1);
}

void block_sledge() {
  cylinder_schlittenzuluft.set(0);
  cylinder_schlittenabluft.set(1);
}

// MEASURE AND DISPLAY FORCE ****************************************************

void display_force(int force) {
  if (nex_current_page == 1) {
    show_info_field();
    String force_string = String(force);
    String suffix = " N";
    display_text_in_info_field(force_string + suffix);
  }
}

float calculate_pressure_from_adc(float sensor_adc_value) {
  static const float voltsPerUnit = 0.03; // Controllino datasheet
  static const float max_sensor_voltage = 10; // Sensor datasheet
  static const float max_sensor_pressure = 10; // [barg]

  float sensor_voltage = sensor_adc_value * voltsPerUnit;
  float pressure = sensor_voltage / max_sensor_voltage * max_sensor_pressure;
  float cylinder_area = 15080; // [mm^2] measured from CAD, both cylinders without rod
  float float_force = cylinder_area * pressure / 10; // 10 to convert from [bar] to [N/mm^2]
  return int(float_force);
}

int get_force() {
  float sensor_adc_value = analogRead(PRESSURE_SENSOR_PIN);
  int force = calculate_pressure_from_adc(sensor_adc_value);
  return force;
}

void measure_and_display_max_force() {

  static int max_force;
  static int previous_max_force;

  int force = get_force();

  // Stop timers when force is rising:
  if (force > max_force) {
    max_force = force;
    erase_force_value_timeout.reset_time();
    log_force_value_timeout.reset_time();
  }

  // Update display every 100ms:
  if (max_force > previous_max_force && pressure_update_delay.delay_time_is_up(100)) {
    display_force(max_force);
    previous_max_force = max_force;
  }

  // Clear display:
  if (erase_force_value_timeout.has_timed_out()) {
    max_force = -1; // negative to make certain value updates
    previous_max_force = -1;
    erase_force_value_timeout.reset_time();
  }

  // Prevent logging of insignificant values:
  if (force < 200) {
    log_force_value_timeout.reset_time();
  }

  // Log max force value:
  if (log_force_value_timeout.has_timed_out()) {
    send_log_force_tension(max_force);
    log_force_value_timeout.reset_time();
  }
}

void measure_and_display_current_force() {
  int force = get_force();
  static int previous_force;
  static int min_difference = 50;

  if (abs(force - previous_force) >= min_difference && pressure_update_delay.delay_time_is_up(100)) {
    display_force(force);
    previous_force = force;
  }
}

// MEASURE AND DISPLAY TEMPERATURE *********************************************

float calculate_temperature_from_adc(float sensor_adc_value) {
  const float volts_per_unit = 0.03; // controllino datasheet
  const float max_sensor_voltage = 10; // min voltage = 0 V
  const float max_sensor_temp = 200; // range @ WAGO 857-810: 0-200°C

  float sensor_voltage = sensor_adc_value * volts_per_unit;
  float temperature = sensor_voltage / max_sensor_voltage * max_sensor_temp;
  return int(temperature);
}

int get_temperature() {
  float sensor_value = analogRead(TEMP_SENSOR_PIN);
  int temperature = calculate_temperature_from_adc(sensor_value);

  return temperature;
}

void display_temperature() {
  // TODO: IF TEMPERATURE HAS CHANGED MORE THAN ONE DEGREE, UPDATE:
  if (nex_prev_current_temperature != get_temperature() && nex_current_page == 1) {

    if (temperature_update_delay.delay_time_is_up(500)) {
      display_text_in_field("t = " + String(get_temperature()) + " C", "t10");
      nex_prev_current_temperature = get_temperature();
    }
  }
}

// NEXTION GENERAL DISPLAY FUNCTIONS *******************************************

void send_to_nextion() {
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
}

void update_display_counter() {
  long new_value = counter.get_value(longtime_counter);
  Serial2.print("t0.txt=");
  Serial2.print("\"");
  Serial2.print(new_value);
  Serial2.print("\"");
  send_to_nextion();
}

void show_info_field() {
  if (nex_current_page == 1) {
    Serial2.print("vis t4,1");
    send_to_nextion();
  }
}

void display_text_in_info_field(String text) {
  Serial2.print("t4");
  Serial2.print(".txt=");
  Serial2.print("\"");
  Serial2.print(text);
  Serial2.print("\"");
  send_to_nextion();
}

void hide_info_field() {
  if (nex_current_page == 1) {
    Serial2.print("vis t4,0");
    send_to_nextion();
  }
}

void clear_text_field(String textField) {
  Serial2.print(textField);
  Serial2.print(".txt=");
  Serial2.print("\"");
  Serial2.print(""); // erase text
  Serial2.print("\"");
  send_to_nextion();
}

void display_value_in_field(int value, String valueField) {
  Serial2.print(valueField);
  Serial2.print(".val=");
  Serial2.print(value);
  send_to_nextion();
}

void display_text_in_field(String text, String textField) {
  Serial2.print(textField);
  Serial2.print(".txt=");
  Serial2.print("\"");
  Serial2.print(text);
  Serial2.print("\"");
  send_to_nextion();
}

void toggle_ds_switch(String button) {
  Serial2.print("click " + button + ",1");
  send_to_nextion();
}

void set_momentary_button_high_or_low(String button, bool state) {
  Serial2.print("click " + button + "," + state);
  send_to_nextion();
}

// NEXTION TOUCH EVENT FUNCTIONS ***********************************************

// TOUCH EVENT FUNCTIONS PAGE 1 - LEFT SIDE ------------------------------------

void switch_play_pause_push(void *ptr) {
  state_controller.toggle_machine_running_state();
  nex_state_machine_running = !nex_state_machine_running;
  machine_stopped_error_timeout.reset_time();
  bandsensor_timeout.reset_time();
  bandbogen_timeout.reset_time();
}

void switch_step_auto_mode_push(void *ptr) {
  state_controller.toggle_step_auto_mode();
  nex_state_step_mode = state_controller.is_in_step_mode();
}

void button_stepback_push(void *ptr) {
  state_controller.set_machine_stop();
  reset_flag_of_current_step();
  state_controller.set_step_mode();
  state_controller.switch_to_previous_step();
  reset_flag_of_current_step();
}

void button_next_step_push(void *ptr) {
  state_controller.set_machine_stop();
  reset_flag_of_current_step();
  state_controller.set_step_mode();
  state_controller.switch_to_next_step();
  reset_flag_of_current_step();
}

void button_reset_rig_push(void *ptr) {
  reset_count = 0;
  state_controller.set_reset_mode(true);
  state_controller.set_run_after_reset(0);
  clear_text_field("t4"); // info field
  hide_info_field();
}

// TOUCH EVENT FUNCTIONS PAGE 1 - RIGHT SIDE -----------------------------------

void switch_wippenhebel_push(void *ptr) {
  cylinder_wippenhebel.toggle();
  nex_state_wippenhebel = !nex_state_wippenhebel;
}
void switch_vorklemme_push(void *ptr) {
  cylinder_hydr_vorklemme.toggle();
  nex_state_vorklemme = !nex_state_vorklemme;
}
void switch_nachklemme_push(void *ptr) {
  cylinder_hydr_nachklemme.toggle();
  nex_state_nachklemme = !nex_state_nachklemme;
}

void switch_entlueften_push(void *ptr) {
  cylinder_schlittenabluft.toggle();
  nex_state_schlittenabluft = !nex_state_schlittenabluft;
}
void button_schneiden_push(void *ptr) { cylinder_messer.set(1); }
void button_schneiden_pop(void *ptr) { cylinder_messer.set(0); }
void button_schlitten_push(void *ptr) { //
  cylinder_schlittenzuluft.set(1);
}
void button_schlitten_pop(void *ptr) { //
  cylinder_schlittenzuluft.set(0);
}

// TOUCH EVENT FUNCTIONS PAGE 2 - LEFT SIDE ------------------------------------

void button_upper_slider_left_push(void *ptr) { decrease_slider_value(cycle_duration); }

void button_upper_slider_right_push(void *ptr) { increase_slider_value(cycle_duration); }

void button_lower_slider_left_push(void *ptr) { decrease_slider_value(max_temperature); }

void button_lower_slider_right_push(void *ptr) { increase_slider_value(max_temperature); }

void increase_slider_value(int eeprom_value_number) {
  long max_value = 350; // [mm]
  long interval = 5;
  long current_value = counter.get_value(eeprom_value_number);

  if (current_value <= (max_value - interval)) {
    counter.set_value(eeprom_value_number, (current_value + interval));
  } else {
    counter.set_value(eeprom_value_number, max_value);
  }
}

void decrease_slider_value(int eeprom_value_number) {
  long min_value = 0; // [mm]
  long interval = 5;
  long current_value = counter.get_value(eeprom_value_number);

  if (current_value >= (min_value + interval)) {
    counter.set_value(eeprom_value_number, (current_value - interval));
  } else {
    counter.set_value(eeprom_value_number, min_value);
  }
}

// TOUCH EVENT FUNCTIONS PAGE 2 - RIGHT SIDE -----------------------------------

void button_reset_shorttime_counter_push(void *ptr) {
  counter.set_value(shorttime_counter, 0);

  // ACTIVATE TIMEOUT TO RESET LONGTIME COUNTER:
  nex_reset_button_timeout.reset_time();
  nex_reset_button_timeout.set_flag_activated(1);
}

void button_reset_shorttime_counter_pop(void *ptr) { nex_reset_button_timeout.set_flag_activated(0); }

// PAGE CHANGING EVENTS (TRIGGER UPDATE OF ALL DISPLAY ELEMENTS) ---------------

void page_0_push(void *ptr) { nex_current_page = 0; }

void page_1_push(void *ptr) {
  nex_current_page = 1;
  hide_info_field();

  // REFRESH BUTTON STATES:
  nex_prev_cycle_step = !state_controller.get_current_step();
  nex_state_step_mode = true;
  nex_state_schlittenabluft = 1;
  nex_state_wippenhebel = 0;
  nex_state_vorklemme = 0;
  nex_state_nachklemme = 0;
  nex_state_schlittenzuluft = 0;
  nex_state_messer = 0;
  nex_state_machine_running = 0;
}

void page_2_push(void *ptr) {
  nex_current_page = 2;
  update_field_values_page_2();
}

void update_field_values_page_2() {
  nex_cycle_duration = counter.get_value(nex_cycle_duration) - 1;
  nex_max_temperature = counter.get_value(nex_max_temperature) - 1;
  nex_shorttime_counter = counter.get_value(nex_shorttime_counter) - 1;
  nex_longtime_counter = counter.get_value(nex_longtime_counter) - 1;
  nex_prev_current_temperature = -1;
}

// DECLARE DISPLAY EVENT LISTENERS *********************************************

void attach_push_and_pop() {
  // PAGE 0 PUSH ONLY:
  nex_page_0.attachPush(page_0_push);
  // PAGE 1 PUSH ONLY:
  nex_page_1.attachPush(page_1_push);
  button_previous_step.attachPush(button_stepback_push);
  button_next_step.attachPush(button_next_step_push);
  button_reset_rig.attachPush(button_reset_rig_push);
  button_previous_step.attachPush(button_stepback_push);
  button_next_step.attachPush(button_next_step_push);
  switch_play_pause.attachPush(switch_play_pause_push);
  switch_step_auto_mode.attachPush(switch_step_auto_mode_push);
  switch_vorklemme.attachPush(switch_vorklemme_push);
  switch_nachklemme.attachPush(switch_nachklemme_push);
  switch_wippenhebel.attachPush(switch_wippenhebel_push);
  switch_entlueften.attachPush(switch_entlueften_push);
  // PAGE 1 PUSH AND POP:
  button_schneiden.attachPush(button_schneiden_push);
  button_schneiden.attachPop(button_schneiden_pop);
  button_schlitten.attachPush(button_schlitten_push);
  button_schlitten.attachPop(button_schlitten_pop);
  // PAGE 2 PUSH ONLY:
  nex_page_2.attachPush(page_2_push);
  button_slider_1_left.attachPush(button_upper_slider_left_push);
  button_slider_1_right.attachPush(button_upper_slider_right_push);
  button_slider_2_left.attachPush(button_lower_slider_left_push);
  button_slider_2_right.attachPush(button_lower_slider_right_push);
  // PAGE 2 PUSH AND POP:
  button_reset_shorttime_counter.attachPush(button_reset_shorttime_counter_push);
  button_reset_shorttime_counter.attachPop(button_reset_shorttime_counter_pop);
}

// DISPLAY SETUP ***************************************************************

void nextion_display_setup() {

  Serial2.begin(9600);

  // RESET NEXTION DISPLAY: (refresh display after PLC restart)
  send_to_nextion(); // needed to start communication
  Serial2.print("rest"); // Reset
  send_to_nextion();
  sendCommand("page 0");
  send_to_nextion();
  attach_push_and_pop();

  delay(3000);
  sendCommand("page 1"); // switch display to page x
  send_to_nextion();
}

// DISPLAY LOOPS ***************************************************************

void nextion_display_loop() {
  //****************************************************************************
  nexLoop(nex_listen_list); // check for any touch event

  if (nex_current_page == 1) {
    display_loop_page_1_left_side();
    display_loop_page_1_right_side();
  }

  if (nex_current_page == 2) {
    display_loop_page_2_left_side();
    display_loop_page_2_right_side();
  }
}

// DISPLAY LOOP PAGE 1 LEFT SIDE: -----------------------------------------------

void display_loop_page_1_left_side() {

  update_cycle_name();

  // UPDATE SWITCHSTATE "STEP"/"AUTO"-MODE:
  if (nex_state_step_mode != state_controller.is_in_step_mode()) {
    toggle_ds_switch("bt1");
    nex_state_step_mode = state_controller.is_in_step_mode();
  }
}

void update_main_cycle_name() {
  if (nex_prev_cycle_step != state_controller.get_current_step()) {
    String number = String(state_controller.get_current_step() + 1);
    String name = get_main_cycle_display_string();
    Serial.println(number + " " + name);
    display_text_in_field(number + " " + name, "t0");
    nex_prev_cycle_step = state_controller.get_current_step();
  }
}

void update_cycle_name() {
  if (state_controller.is_in_step_mode() || state_controller.is_in_auto_mode()) {
    update_main_cycle_name();
  }
}

String get_main_cycle_display_string() {
  int current_step = state_controller.get_current_step();
  String display_text_cycle_name = main_cycle_steps[current_step]->get_display_text();
  return display_text_cycle_name;
}

// DISPLAY LOOP PAGE 1 RIGHT SIDE: ---------------------------------------------

void display_loop_page_1_right_side() {

  // UPDATE SWITCHES:
  if (state_controller.machine_is_running() != nex_state_machine_running) {
    toggle_ds_switch("bt0");
    nex_state_machine_running = !nex_state_machine_running;
  }
  if (cylinder_schlittenabluft.get_state() != nex_state_schlittenabluft) {
    toggle_ds_switch("bt3");
    nex_state_schlittenabluft = !nex_state_schlittenabluft;
  }
  if (cylinder_wippenhebel.get_state() != nex_state_wippenhebel) {
    toggle_ds_switch("bt5");
    nex_state_wippenhebel = !nex_state_wippenhebel;
  }
  if (cylinder_hydr_vorklemme.get_state() != nex_state_vorklemme) {
    toggle_ds_switch("bt2");
    nex_state_vorklemme = !nex_state_vorklemme;
  }
  if (cylinder_hydr_nachklemme.get_state() != nex_state_nachklemme) {
    toggle_ds_switch("bt4");
    nex_state_nachklemme = !nex_state_nachklemme;
  }

  // UPDATE BUTTONS:
  if (cylinder_schlittenzuluft.get_state() != nex_state_schlittenzuluft) {
    bool state = cylinder_schlittenzuluft.get_state();
    String button = "b6";
    set_momentary_button_high_or_low(button, state);
    nex_state_schlittenzuluft = cylinder_schlittenzuluft.get_state();
  }

  if (cylinder_messer.get_state() != nex_state_messer) {
    bool state = cylinder_messer.get_state();
    String button = "b5";
    set_momentary_button_high_or_low(button, state);
    nex_state_messer = cylinder_messer.get_state();
  }
}

// DIPLAY LOOP PAGE 2 LEFT SIDE: -----------------------------------------------

void display_loop_page_2_left_side() {
  update_upper_slider_value();
  update_lower_slider_value();
  update_switches_page_2_left();

  // UPDATE SWITCH:
}

void update_upper_slider_value() {
  if (counter.get_value(cycle_duration) != nex_cycle_duration) {
    display_text_in_field(add_suffix_to_eeprom_value(cycle_duration, "s"), "t24");
    nex_cycle_duration = counter.get_value(cycle_duration);
  }
}

void update_lower_slider_value() {
  if (counter.get_value(max_temperature) != nex_max_temperature) {
    display_text_in_field(add_suffix_to_eeprom_value(max_temperature, " C"), "t22");
    nex_max_temperature = counter.get_value(max_temperature);
  }
}

String add_suffix_to_eeprom_value(int eeprom_value_number, String suffix) {
  String value = String(counter.get_value(eeprom_value_number));
  String space = " ";
  String suffixed_string = value + space + suffix;
  return suffixed_string;
}

void update_switches_page_2_left() {}

// DIPLAY LOOP PAGE 2 RIGHT SIDE: ----------------------------------------------

void display_loop_page_2_right_side() {
  update_upper_counter_value();
  update_lower_counter_value();
  reset_lower_counter_value();
}

void update_upper_counter_value() {
  if (nex_longtime_counter != counter.get_value(longtime_counter)) {
    display_text_in_field(String(counter.get_value(longtime_counter)), "t20");
    nex_longtime_counter = counter.get_value(longtime_counter);
  }
}

void update_lower_counter_value() {
  // UPDATE LOWER COUNTER:
  if (nex_shorttime_counter != counter.get_value(shorttime_counter)) {
    display_text_in_field(String(counter.get_value(shorttime_counter)), "t21");
    nex_shorttime_counter = counter.get_value(shorttime_counter);
  }
}

void reset_lower_counter_value() {
  if (nex_reset_button_timeout.is_marked_activated()) {
    if (nex_reset_button_timeout.has_timed_out()) {
      counter.set_value(longtime_counter, 0);
    }
  }
}

// CLASSES FOR THE MAIN CYCLE STEPS ********************************************
// STEP-MODE AND AUTO MODE

// SCHLITTEN ENTLÜFTEN
class Luft_ablassen : public Cycle_step {
  String get_display_text() { return "LUFT ABLASSEN"; }
  bool pressure_low = false;

  void do_initial_stuff() {
    vent_sledge();
    pressure_low = false;
    cycle_step_delay.set_unstarted();
    cycle_start_millis = millis();
  }
  void do_loop_stuff() {
    if (get_force() < 1000) {
      pressure_low = true;
    }

    if (pressure_low) {
      if (cycle_step_delay.delay_time_is_up(200)) {
        set_loop_completed();
      }
    }
  }
};

// VORKLEMME ÖFFNEN
class Vorklemme_auf : public Cycle_step {
  String get_display_text() { return "VORKLEMME AUF"; }

  void do_initial_stuff() {
    cylinder_hydr_vorklemme.set(0);
    cycle_step_delay.set_unstarted();
  }
  void do_loop_stuff() {
    if (cycle_step_delay.delay_time_is_up(800)) {
      set_loop_completed();
    }
  }
};

// SCHLITTEN ZURÜCKFAHREN
class Schlitten_zurueck : public Cycle_step {
  String get_display_text() { return "SCHLITTEN ZURUECK"; }

  void do_initial_stuff() { move_sledge(); }
  void do_loop_stuff() {
    if (sensor_sledge_startposition.get_raw_button_state()) {
      vent_sledge();
      set_loop_completed();
    }
  }
};

// NACHKLEMME ÖFFNEN
class Nachklemme_auf : public Cycle_step {
  String get_display_text() { return "NACHKLEMME AUF"; }

  void do_initial_stuff() {
    cylinder_hydr_nachklemme.set(0);
    cycle_step_delay.set_unstarted();
  }
  void do_loop_stuff() {
    if (cycle_step_delay.delay_time_is_up(200)) {
      set_loop_completed();
    }
  }
};

// AUSWERFER BETÄTIGEN
class Auswerfen : public Cycle_step {
  String get_display_text() { return "AUSWERFEN"; }

  void do_initial_stuff() {}
  void do_loop_stuff() {
    cylinder_auswerfer.set(1);
    set_loop_completed();
  }
};

// FÖRDERKLEMME SCHLIESSEN
class Foerderklemme_zu : public Cycle_step {
  String get_display_text() { return "FOERDERKLEMME ZU"; }

  void do_initial_stuff() {
    cylinder_hydr_vorschubklemme.set(1);
    cycle_step_delay.set_unstarted();
  }
  void do_loop_stuff() {
    if (cycle_step_delay.delay_time_is_up(800)) {
      set_loop_completed();
    }
  }
};

// FÖRDERN
class Foerdern : public Cycle_step {
  String get_display_text() { return "FOERDERN"; }

  void do_initial_stuff() {
    foerderzylinder_foerdern();
    machine_stopped_error_timeout.set_time(25000);
  }
  void do_loop_stuff() {
    if (sensor_foerderzylinder_in.switched_high()) {
      machine_stopped_error_timeout.reset_time();
      machine_stopped_error_timeout.set_time(7000);
      set_loop_completed();
    }
  }
};

// VORKLEMME SCHLIESSEN
class Vorklemme_zu : public Cycle_step {
  String get_display_text() { return "VORKLEMME ZU"; }

  void do_initial_stuff() {
    cylinder_hydr_vorklemme.set(1);
    cycle_step_delay.set_unstarted();
  }
  void do_loop_stuff() {
    if (cycle_step_delay.delay_time_is_up(200)) {
      set_loop_completed();
    }
  }
};

// MESSER AB

class Schneiden : public Cycle_step {
  String get_display_text() { return "SCHNEIDEN"; }
  int cut_retries = 0;

  void do_initial_stuff() {
    cut_retries = 0;
    vent_sledge();
    cycle_step_delay.set_unstarted();
  }

  void try_cutting_twice() {
    cylinder_messer.stroke(1000, 500);

    // If sensor does not switch high during the set stroke time,
    // knife did not cut through and a second stroke will be started.
    if (cylinder_messer.stroke_completed()) {
      cut_retries++;
      cylinder_messer.set(0);
      delay(500);
    }

    if (cut_retries == 3) {
      strap_count_for_knife = 0;
      cylinder_messer.abort_stroke();
      set_loop_completed();
      error_stop_machine("ERROR MESSER");
    }

    if (sensor_messerzylinder.switched_high()) {
      strap_count_for_knife = 0;
      cylinder_messer.abort_stroke();
      cylinder_messer.set(0);
      delay(500);
      set_loop_completed();
    }
  }
  void do_loop_stuff() {

    if (strap_count_for_knife == 3) {
      try_cutting_twice();
    } else {
      if (cycle_step_delay.delay_time_is_up(600)) {
        strap_count_for_knife++;
        set_loop_completed();
      }
    }
  }
};

// FÖRDERKLEMME ÖFFNEN
class Foerdereinheit_auf : public Cycle_step {
  String get_display_text() { return "FOERDERKLEMME AUF"; }

  void do_initial_stuff() {
    cylinder_hydr_vorschubklemme.set(0);
    cycle_step_delay.set_unstarted();
  }
  void do_loop_stuff() {
    if (cycle_step_delay.delay_time_is_up(600)) {
      set_loop_completed();
    }
  }
};

// FÖRDERZYLINDER ZURÜCK
class Foerderzylinder_zurueck : public Cycle_step {
  String get_display_text() { return "FOERDERER ZURUECK"; }

  void do_initial_stuff() {
    foerderzylinder_zurueck();
    cycle_step_delay.set_unstarted();
  }
  void do_loop_stuff() {
    // if (sensor_foerderzylinder_out.switched_high()) {
    //   set_loop_completed();
    // }
    if (cycle_step_delay.delay_time_is_up(500)) {
      set_loop_completed();
    }
  }
};

// AUSWERFER ZURÜCK
class Auswerfer_zurueck : public Cycle_step {
  String get_display_text() { return "AUSWERFER ZURUECK"; }

  void do_initial_stuff() { vent_sledge(); }
  void do_loop_stuff() {
    cylinder_auswerfer.set(0);
    set_loop_completed();
  }
};

// // MESSER AUF
// class Messer_auf : public Cycle_step {
//   String get_display_text() { return "MESSER AUF"; }

//   void do_initial_stuff() {
//     vent_sledge();
//     cycle_step_delay.set_unstarted();
//   }
//   void do_loop_stuff() {
//     cylinder_messer.set(0);
//     if (cycle_step_delay.delay_time_is_up(500)) {
//       set_loop_completed();
//     }
//   }
// };

// WIPPENHEBEL SCHLIESSEN
class Tool_wippe_zu : public Cycle_step {
  String get_display_text() { return "WIPPE ZU"; }

  void do_initial_stuff() {
    cylinder_wippenhebel.set(0);
    cycle_step_delay.set_unstarted();
  }
  void do_loop_stuff() {
    if (cycle_step_delay.delay_time_is_up(700)) {
      set_loop_completed();
    }
  }
};

// FÖRDERKLEMME SCHLIESSEN

// MIT FÖRDERZYLINDER AKTIV SPANNEN UM BOGEN ZU VERMEIDEN
class Aktiv_spannen : public Cycle_step {
  String get_display_text() { return "AKTIV SPANNEN"; }
  void do_initial_stuff() { foerderzylinder_foerdern(); }
  void do_loop_stuff() { set_loop_completed(); }
};

// GERÄT SPANNEN
class Tool_spannen : public Cycle_step {
  String get_display_text() { return "TOOL SPANNEN"; }
  bool has_reached_sensor = false;

  void do_initial_stuff() {
    has_reached_sensor = false;
    send_log_start_tensioning();
    block_sledge();
    cylinder_spanntaste.set(1);
    cycle_step_delay.set_unstarted();
  }
  void do_loop_stuff() {
    // if (cycle_step_delay.delay_time_is_up(4000)) {
    //   cylinder_spanntaste.set(0);
    //   set_loop_completed();
    // }
    if (sensor_sledge_endposition.switched_high()) {
      has_reached_sensor = true;
    }

    if (has_reached_sensor) {
      if (cycle_step_delay.delay_time_is_up(600)) {
        cylinder_spanntaste.set(0);
        set_loop_completed();
      }
    }
  }
};

// MIT FÖRDERZYLINDER AKTIV SPANNEN UM BOGEN ZU VERMEIDEN
class Foerderzylinder_stop : public Cycle_step {
  String get_display_text() { return "FOERDER STOP"; }
  void do_initial_stuff() { foerderzylinder_stop(); }
  void do_loop_stuff() { set_loop_completed(); }
};

// NACHKLEMME SCHLIESSEN
class Nachklemme_zu : public Cycle_step {
  String get_display_text() { return "NACHKLEMME ZU"; }

  void do_initial_stuff() {
    cylinder_hydr_nachklemme.set(1);
    cycle_step_delay.set_unstarted();
  }
  void do_loop_stuff() {
    if (cycle_step_delay.delay_time_is_up(400)) {
      set_loop_completed();
    }
  }
};

// FÖRDERKLEMME ÖFFNEN

// FÖRDERZYLINDER ZURÜCK

// GERÄT CRIMPEN
class Tool_crimp : public Cycle_step {
  String get_display_text() { return "TOOL CRIMP"; }

  void do_initial_stuff() { send_log_start_crimping(); }
  void do_loop_stuff() {
    cylinder_crimptaste.stroke(500, 3000);
    if (cylinder_crimptaste.stroke_completed()) {
      set_loop_completed();
    }
  }
};

// WIPPENHEBEL BETÄTIGEN
class Tool_wippe_auf : public Cycle_step {
  String get_display_text() { return "TOOL WIPPENHEBEL"; }

  void do_initial_stuff() {
    counter.count_one_up(shorttime_counter);
    send_log_cycle_reset(counter.get_value(shorttime_counter));
    counter.count_one_up(longtime_counter);
    send_log_cycle_total(counter.get_value(longtime_counter));
    cylinder_wippenhebel.set(1);
    reset_count = 0;
  }
  void do_loop_stuff() { set_loop_completed(); }
};

// PAUSE
class Tool_pause : public Cycle_step {
  String get_display_text() { return "PAUSE"; }
  unsigned long cycle_duration_millis;
  unsigned long cycle_time_elapsed;
  unsigned long cycle_time_remaining; // [ms]

  void do_initial_stuff() {
    cycle_step_delay.set_unstarted();

    cycle_duration_millis = counter.get_value(cycle_duration) * 1000;
    cycle_time_elapsed = millis() - cycle_start_millis;

    // Calculate delay time:
    if (cycle_duration_millis > cycle_time_elapsed) {
      cycle_time_remaining = cycle_duration_millis - cycle_time_elapsed;
    } else {
      cycle_time_remaining = 0;
    }
  }
  void do_loop_stuff() {
    machine_stopped_error_timeout.reset_time();

    static int previous_timeout_time = 0;
    int timeout_time = cycle_step_delay.get_remaining_delay_time() / 1000;
    if (timeout_time != previous_timeout_time) {
      display_text_in_field("PAUSE " + String(timeout_time), "t0");
      previous_timeout_time = timeout_time;
    }

    if (cycle_step_delay.delay_time_is_up(cycle_time_remaining)) {
      set_loop_completed();
    }
  }
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void get_electrocylinder_endpoints() {
  Serial.println("START ENDPUNKTINITIALISIERUNG ELEKTROZYLINDER:");

  foerderzylinder_foerdern();
  Serial.println("FÄHRT ZUR STARTPOSITION");
  while (!sensor_foerderzylinder_in.get_raw_button_state()) {
  }
  Serial.println("STARTPOSITION ERREICHT");

  foerderzylinder_zurueck();
  Serial.println("FÄHRT ZUR ENDPOSITION");
  while (!sensor_foerderzylinder_out.get_raw_button_state()) {
  }
  Serial.println("ENDPOSITION ERREICHT");
  Serial.println("ZYLINDERINITIALISIERUNG ABGESCHLOSSEN");
}

void power_on_electrocylinder() {
  Serial.println("ELEKTROZYLINDER POWER ON:");
  // Festo specification ELGS-BS
  // Turn on logic power >=50ms after load power:
  delay(200);
  digitalWrite(FOERDERZYLINDER_LOGIC_POWER_RELAY, HIGH);

  delay(8000); // wait until cylinder has initialized
  digitalWrite(TRENNRELAIS_ZYLINDER_1, HIGH);
  digitalWrite(TRENNRELAIS_ZYLINDER_2, HIGH);
  // get_electrocylinder_endpoints();
}

void power_off_electrocylinder() {
  digitalWrite(FOERDERZYLINDER_MOVE_IN, LOW);
  digitalWrite(FOERDERZYLINDER_MOVE_OUT, LOW);

  // Disconnect electrocylinder analog outputs:
  digitalWrite(TRENNRELAIS_ZYLINDER_1, LOW);
  digitalWrite(TRENNRELAIS_ZYLINDER_2, LOW);
  // Festo specification ELGS-BS
  // Turn off logic power >= 100ms after analog outputs:
  delay(200);
  digitalWrite(FOERDERZYLINDER_LOGIC_POWER_RELAY, LOW);
}

void stop_machine(String error_message) {
  state_controller.set_machine_stop();
  state_controller.set_step_mode();
  reset_machine();
  cylinder_hydr_pressure_valve.set(0);
  stroke_wippenhebel();
  cylinder_main_hauptluft.set(0);
  power_off_electrocylinder();
  show_info_field();
  display_text_in_info_field(error_message);
}

void error_stop_machine(String error_message) {
  stop_machine(error_message);
  send_email_machine_stopped();
  state_controller.set_error_mode(true);
}

void monitor_emergency_signal() {

  static bool emergency_stop_active = false;

  // (RE-)START SYSTEM
  if (emergency_stop_signal.switched_low()) {
    emergency_stop_active = false;
    power_on_electrocylinder();
    hydraulic_timeout.reset_time();
    cylinder_hydr_pressure_valve.set(1);
    cylinder_main_hauptluft.set(1);
  }

  // STOP SYSTEM (LOOP RUNS ONLY ONCE)
  if (emergency_stop_signal.switched_high()) {
    emergency_stop_active = true;
    stop_machine("NOT AUS AKTIV");
  }

  // KEEP SYSTEM STOPPED (LOOP KEEPS RUNNING)
  if (emergency_stop_active) {
    state_controller.set_machine_stop();
    cylinder_hydr_pressure_valve.set(0);
    cylinder_main_hauptluft.set(0);
  }
}

void monitor_error_timeouts() {

    // TIMEOUT IF NO LOWER STRAP DETECTED:
  if (bandsensor_timeout.has_timed_out()) {
    error_stop_machine("KEIN BAND");
  }

  // TIMEOUT IF STRAP IS BENT INTO A CURVE:
  else if (bandbogen_timeout.has_timed_out()) {
    error_stop_machine("BAND GEBOGEN");
  }

  // TIMEOUT IF STUCK IN A CYCLE:
  else if (machine_stopped_error_timeout.has_timed_out()) {
    show_info_field();
    display_text_in_info_field("STOPPED ...");
    delay(2000);
    machine_stopped_error_timeout.reset_time();
    bandsensor_timeout.reset_time();
    bandbogen_timeout.reset_time();

    if (reset_count == 0) {
      // Reset and restart:
      show_info_field();
      display_text_in_info_field("RESET 1");
      delay(2000);
      reset_count++;
      state_controller.set_reset_mode(1);
      state_controller.set_run_after_reset(1);
    }

    else if (reset_count == 1) {
      // Reset and restart:
      show_info_field();
      display_text_in_info_field("RESET 2");
      delay(2000);
      reset_count++;
      state_controller.set_reset_mode(1);
      state_controller.set_run_after_reset(1);
    }

    else if (reset_count == 2) {
      // Stop and show error:
      error_stop_machine("TIMEOUT ERROR");
    }
  }


}

void monitor_temperature_error() {
  if (get_temperature() > counter.get_value(max_temperature)) {
    error_stop_machine("TEMPERATURE ERROR");
  }
}

// MAIN SETUP ******************************************************************

void setup() {
  //------------------------------------------------
  // SETUP PIN MODES:
  pinMode(TRENNRELAIS_ZYLINDER_1, OUTPUT);
  pinMode(TRENNRELAIS_ZYLINDER_2, OUTPUT);
  pinMode(FOERDERZYLINDER_LOGIC_POWER_RELAY, OUTPUT);
  pinMode(FOERDERZYLINDER_MOVE_IN, OUTPUT);
  pinMode(FOERDERZYLINDER_MOVE_OUT, OUTPUT);

  //------------------------------------------------
  // PUSH THE CYCLE STEPS INTO THE VECTOR CONTAINER:
  // PUSH SEQUENCE = CYCLE SEQUENCE !
  main_cycle_steps.push_back(new Luft_ablassen);
  main_cycle_steps.push_back(new Vorklemme_auf);
  main_cycle_steps.push_back(new Schlitten_zurueck);
  main_cycle_steps.push_back(new Nachklemme_auf);
  main_cycle_steps.push_back(new Auswerfen);
  main_cycle_steps.push_back(new Foerderklemme_zu);
  main_cycle_steps.push_back(new Foerdern);
  main_cycle_steps.push_back(new Vorklemme_zu);
  main_cycle_steps.push_back(new Schneiden);
  main_cycle_steps.push_back(new Foerdereinheit_auf);
  main_cycle_steps.push_back(new Foerderzylinder_zurueck);
  main_cycle_steps.push_back(new Auswerfer_zurueck);
  // main_cycle_steps.push_back(new Messer_auf);
  main_cycle_steps.push_back(new Tool_wippe_zu);
  main_cycle_steps.push_back(new Foerderklemme_zu);
  main_cycle_steps.push_back(new Aktiv_spannen);
  main_cycle_steps.push_back(new Tool_spannen);
  main_cycle_steps.push_back(new Foerderzylinder_stop);
  main_cycle_steps.push_back(new Nachklemme_zu);
  main_cycle_steps.push_back(new Foerdereinheit_auf);
  main_cycle_steps.push_back(new Foerderzylinder_zurueck);
  main_cycle_steps.push_back(new Tool_crimp);
  main_cycle_steps.push_back(new Tool_wippe_auf);
  main_cycle_steps.push_back(new Tool_pause);
  //------------------------------------------------
  // CONFIGURE THE STATE CONTROLLER:
  int no_of_main_cycle_steps = main_cycle_steps.size();
  state_controller.set_no_of_steps(no_of_main_cycle_steps);
  //------------------------------------------------
  // SETUP COUNTER:
  counter.setup(0, 1023, counter_no_of_values);
  //------------------------------------------------
  Serial.begin(115200);
  Serial1.begin(115200); // used to log to raspberry log merger via USB-Serial-Converter @ TX1/RX1
  state_controller.set_step_mode();

  //------------------------------------------------
  nextion_display_setup();
  // REQUIRED STEP TO MAKE SKETCH WORK AFTER RESET:
  reset_flag_of_current_step();

  if (!emergency_stop_signal.get_raw_button_state()) { // emergency stop not activated
    power_on_electrocylinder();
    cylinder_hydr_pressure_valve.set(1);
    cylinder_main_hauptluft.set(1);
  }
  cylinder_hydr_power_supply.set(1);
  hydraulic_timeout.reset_time();
  Serial.println("EXIT SETUP");
}

// MAIN LOOP *******************************************************************

void run_step_or_auto_mode() {

  // IF STEP IS COMPLETED SWITCH TO NEXT STEP:
  if (main_cycle_steps[state_controller.get_current_step()]->is_completed()) {
    state_controller.switch_to_next_step();
    reset_flag_of_current_step();
  }

  // IN STEP MODE, THE RIG STOPS AFTER EVERY COMPLETED STEP:
  if (state_controller.step_switch_has_happend()) {
    hydraulic_timeout.reset_time();
    if (state_controller.is_in_step_mode()) {
      state_controller.set_machine_stop();
    }
    // IF MACHINE STATE IS RUNNING IN AUTO MODE,
    // THE "MACHINE STOPPED ERROR TIMEOUT" RESETS AFTER EVERY STEP:
    if (state_controller.is_in_auto_mode() && state_controller.machine_is_running()) {
      machine_stopped_error_timeout.reset_time();
    }
  }

  // IF MACHINE STATE IS "RUNNING", RUN CURRENT STEP:
  if (state_controller.machine_is_running()) {
    main_cycle_steps[state_controller.get_current_step()]->do_stuff();
  }

  // MEASURE AND DISPLAY PRESSURE
  if (!state_controller.is_in_error_mode()) {
    measure_and_display_max_force();
  }
}

void loop() {

  // MONITOR EMERGENCY SIGNAL:
  monitor_emergency_signal();

  // MEASURE AND DISPLAY MOTOR TEMPERATURE
  display_temperature();

  // CONTROL COOLING AIR
  cylinder_kuehlluft.set(state_controller.is_in_auto_mode());

  // DO NOT WATCH TIMEOUTS IF MACHINE IS NOT RUNNING (PAUSE):
  if (!state_controller.machine_is_running()) {
    machine_stopped_error_timeout.reset_time();
    bandsensor_timeout.reset_time();
    bandbogen_timeout.reset_time();
  }

  // MONITOR ERRORS ONLY WHEN RIG IS RUNNING IN AUTO MODE:
  if (state_controller.machine_is_running() && state_controller.is_in_auto_mode()) {
    monitor_error_timeouts();
    monitor_temperature_error();
  }

  // UPDATE DISPLAY:
  nextion_display_loop();

  // RUN STEP OR AUTO MODE:
  if (state_controller.is_in_step_mode() || state_controller.is_in_auto_mode()) {
    run_step_or_auto_mode();
  }

  // RUN RESET IF RESET IS ACTIVATED:
  if (state_controller.reset_mode_is_active()) {
    reset_machine();
    stroke_wippenhebel();
    state_controller.set_reset_mode(0);

    if (state_controller.run_after_reset_is_active()) {
      state_controller.set_auto_mode();
      state_controller.set_machine_running();
    } else {
      state_controller.set_step_mode();
    }
  }

  // CHECK IF STRAP AVAILABLE:
  if (bandsensor_zufuhr_unten.get_raw_button_state() && bandsensor_zufuhr_oben.get_raw_button_state()) {
    bandsensor_timeout.reset_time();
  }

  // CHECK IF STRAP IS NOT BENT INTO A CURVE:
  if (bandbogensensor.get_raw_button_state()) {
    bandbogen_timeout.reset_time();
  }

  // DISPLAY DEBUG INFOMATION:
  //unsigned long runtime = measure_runtime();
  if (print_interval_timeout.has_timed_out()) {
    //Serial.println(runtime);
    print_interval_timeout.reset_time();
  }
  // POWER OFF HYDRAULIC AFTER A WHILE OF INACTIVITY
  if (hydraulic_timeout.has_timed_out()) {
    cylinder_hydr_power_supply.set(0);
  }

  // POWER OFF HYDRAULIC IF HYDRAULIC SAFETY SENSOR IS LOW
  if (!hydraulic_safety_sensor_1.get_raw_button_state() || !hydraulic_safety_sensor_2.get_raw_button_state()) {
    cylinder_hydr_power_supply.set(0);
  }
}

// END OF PROGRAM **************************************************************