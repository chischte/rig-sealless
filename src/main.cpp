/*
 * *****************************************************************************
 * PTY-SEMI-AUTOMATIC-RIG
 * *****************************************************************************
 * Program for a semi-automatic endurance test rig for a mechanical tool
 * -----------------------------------------------------------------------------
 * Michael Wettstein
 * May 2020, Zürich
 * -----------------------------------------------------------------------------
 * RUNTIME:
 * Measured runtime in idle: about 130 micros
 * -----------------------------------------------------------------------------
 * TODO:
 * Stroke completed signal kommt zu früh (nach dem Schneiden), bereits wenndeshalb musste
 * temproär ein delay eingabaut werden!
 * Close front shield earlier
 * *****************************************************************************
 */

// INCLUDE HEADERS *************************************************************

#include <ArduinoSTL.h> //          https://github.com/mike-matera/ArduinoSTL
#include <Controllino.h> //         PIO Controllino Library
#include <Cylinder.h> //            https://github.com/chischte/cylinder-library
#include <Debounce.h> //            https://github.com/chischte/debounce-library
#include <EEPROM_Counter.h> //      https://github.com/chischte/eeprom-counter-library
#include <Insomnia.h> //            https://github.com/chischte/insomnia-delay-library
#include <Nextion.h> //             PIO Nextion library
#include <SD.h> //                  PIO Adafruit SD library
#include <alias_colino.h> //        aliases when using an Arduino instead of a Controllino
#include <cycle_step.h> //          blueprint of a cycle step
#include <state_controller.h> //    keeps track of machine states
#include <traffic_light.h> //       keeps track of user infos, manages text and colors

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
void update_traffic_light_field();
void set_traffic_light_field_text(String text);
void set_traffic_light_field_color(String color);
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
String get_main_cycle_display_string();
String get_continuous_cycle_display_string();
String add_suffix_to_eeprom_value(int eeprom_value_number, String suffix);

// DEFINE NAMES FOR THE CYCLE COUNTER ******************************************

enum counter {
  longtime_counter, //
  shorttime_counter, //
  upper_strap_feed, // [mm]
  lower_strap_feed, // [mm]
  end_of_counter_enum // keep this entry
};
int counter_no_of_values = end_of_counter_enum;

// DECLARE VARIABLES, PINS AND OBJECTS FOR THE STEPPER MOTORS ******************
const byte micro_step_factor = 2;
const long full_steps_per_mm = 80; // calculated from measurements
const byte stepper_direction_factor = 1; // set -1 to change direction

const byte UPPER_MOTOR_STEP_PIN = CONTROLLINO_D0;
const byte UPPER_MOTOR_DIRECTION_PIN = CONTROLLINO_D1;

const byte LOWER_MOTOR_STEP_PIN = CONTROLLINO_D3;
const byte LOWER_MOTOR_DIRECTION_PIN = CONTROLLINO_D4;

const byte PRESSURE_SENSOR_PIN = CONTROLLINO_A2;

// GENERATE OBJECTS ************************************************************

EEPROM_Counter counter;
State_controller state_controller;
Traffic_light traffic_light;

Cylinder cylinder_sledge_inlet(CONTROLLINO_D14);
Cylinder cylinder_sledge_vent(CONTROLLINO_D13);
Cylinder cylinder_blade(CONTROLLINO_D12);
Cylinder cylinder_frontclap(CONTROLLINO_D15);
Cylinder motor_upper_enable(CONTROLLINO_D2);
Cylinder motor_lower_enable(CONTROLLINO_D5);
Cylinder motor_upper_pulse(CONTROLLINO_D8);
Cylinder motor_lower_pulse(CONTROLLINO_D9);
Cylinder green_light_lamp(CONTROLLINO_D11);
Cylinder red_light_lamp(CONTROLLINO_D10);

Debounce sensor_sledge_startposition(CONTROLLINO_A0);
Debounce sensor_sledge_endposition(CONTROLLINO_A1);
// Debounce sensor_upper_strap(CONTROLLINO_A2);
// Debounce sensor_lower_strap(CONTROLLINO_A3);

Insomnia motor_output_timeout(259200000); // = 3 days// planned to prevent overheating
Insomnia motor_display_sleep_timeout(259000000); // to inform that brakes will soon release
Insomnia nex_reset_button_timeout(3000); // pushtime to reset counter
Insomnia print_interval_timeout(1000);
Insomnia erase_force_value_timeout(5000);
Insomnia pressure_update_delay;
Insomnia cycle_step_delay;
Insomnia upper_feed_delay;
Insomnia lower_feed_delay;

// NEXTION DISPLAY OBJECTS *****************************************************

// PAGE 0 ----------------------------------------------------------------------
NexPage nex_page_0 = NexPage(0, 0, "page0");
// PAGE 1 - LEFT SIDE ----------------------------------------------------------
NexPage nex_page_1 = NexPage(1, 0, "page1");
NexButton button_previous_step = NexButton(1, 6, "b1");
NexButton button_next_step = NexButton(1, 7, "b2");
NexButton button_reset_cycle = NexButton(1, 5, "b0");
NexButton button_traffic_light = NexButton(1, 15, "b8");
NexDSButton switch_step_auto_mode = NexDSButton(1, 4, "bt1");
// PAGE 1 - RIGHT SIDE ---------------------------------------------------------
NexButton button_upper_motor = NexButton(1, 9, "b4");
NexButton button_lower_motor = NexButton(1, 8, "b3");
NexButton button_cut = NexButton(1, 14, "b5");
NexButton button_sledge = NexButton(1, 1, "b6");
NexDSButton switch_motor_brake = NexDSButton(1, 10, "bt5");
NexDSButton switch_air_release = NexDSButton(1, 13, "bt3");
// PAGE 2 - LEFT SIDE ----------------------------------------------------------
NexPage nex_page_2 = NexPage(2, 0, "page2");
NexButton button_slider_1_left = NexButton(2, 5, "b1");
NexButton button_slider_1_right = NexButton(2, 6, "b2");
NexButton button_slider_2_left = NexButton(2, 16, "b5");
NexButton button_slider_2_right = NexButton(2, 17, "b6");
NexDSButton switch_continuous_mode = NexDSButton(2, 18, "bt3");

// PAGE 2 - RIGHT SIDE ---------------------------------------------------------
NexButton button_reset_shorttime_counter = NexButton(2, 12, "b4");

// NEXTION DISPLAY - TOUCH EVENT LIST ******************************************

NexTouch *nex_listen_list[] = { //
    // PAGE 0:
    &nex_page_0,
    // PAGE 1 LEFT:
    &nex_page_1, &button_previous_step, &button_next_step, &button_reset_cycle,
    &button_traffic_light, &switch_step_auto_mode,
    // PAGE 1 RIGHT:
    &button_cut, &switch_motor_brake, &switch_air_release, &button_sledge, &button_upper_motor,
    &button_lower_motor,
    // PAGE 2 LEFT:
    &nex_page_2, &button_slider_1_left, &button_slider_1_right, &nex_page_2, &button_slider_2_left,
    &button_slider_2_right, &switch_continuous_mode,
    // PAGE 2 RIGHT:
    &button_reset_shorttime_counter,
    // END OF LISTEN LIST:
    NULL};

// VARIABLES TO MONITOR NEXTION DISPLAY STATES *********************************

bool nex_state_air_release;
bool nex_state_upper_motor;
bool nex_state_lower_motor;
bool nex_state_motor_brake;
bool nex_state_sledge;
bool nex_state_blade;
bool nex_state_machine_running;
bool nex_state_step_mode = true;
bool nex_state_continuous_mode;
byte nex_prev_cycle_step;
byte nex_current_page = 0;
long nex_upper_strap_feed;
long nex_lower_strap_feed;
long nex_shorttime_counter;
long nex_longtime_counter;

// CREATE VECTOR CONTAINER FOR THE CYCLE STEPS OBJECTS ************************

int Cycle_step::object_count = 0; // enable object counting
std::vector<Cycle_step *> main_cycle_steps;
std::vector<Cycle_step *> continuous_cycle_steps;

// NON NEXTION FUNCTIONS *******************************************************

void set_initial_cylinder_states() {
  cylinder_blade.set(0);
  cylinder_frontclap.set(0);
  cylinder_sledge_inlet.set(0);
  cylinder_sledge_vent.set(0);
  motor_upper_enable.set(0);
  motor_upper_pulse.set(0);
  motor_lower_enable.set(0);
  motor_lower_pulse.set(0);
}

void reset_flag_of_current_step() {

  if (state_controller.is_in_auto_mode() || state_controller.is_in_step_mode()) {
    main_cycle_steps[state_controller.get_current_step()]->reset_flags();
  }
  if (state_controller.is_in_continuous_mode()) {
    continuous_cycle_steps[state_controller.get_current_step()]->reset_flags();
  }
}

void stop_machine() {
  set_initial_cylinder_states();
  state_controller.set_step_mode();
  state_controller.set_machine_stop();
}

void reset_machine() {
  state_controller.set_machine_stop();
  set_initial_cylinder_states();
  clear_text_field("t4");
  hide_info_field();
  state_controller.set_step_mode();
  state_controller.set_current_step_to(0);
  reset_flag_of_current_step();
  state_controller.set_reset_mode(false);
}

void motor_output_enable() {
  motor_upper_enable.set(1);
  motor_lower_enable.set(1);
  motor_output_timeout.reset_time();
  motor_display_sleep_timeout.reset_time();
}

void motor_output_disable() {
  motor_upper_enable.set(0);
  motor_lower_enable.set(0);
}

void motor_output_toggle() {
  if (motor_upper_enable.get_state()) {
    motor_output_disable();
  } else {
    motor_output_enable();
  }
}

void monitor_motor_output() {
  if (motor_output_timeout.has_timed_out()) {
    motor_output_disable();
  }
}

void start_upper_motor() {
  motor_output_enable();
  motor_upper_pulse.set(1);
}

void stop_upper_motor() { motor_upper_pulse.set(0); }

void start_lower_motor() {
  motor_output_enable();
  motor_lower_pulse.set(1);
}

void stop_lower_motor() { motor_lower_pulse.set(0); }

unsigned long calculate_feedtime_from_mm(long mm) {
  unsigned long feedtime = mm * 17;
  return feedtime;
}

void manage_signal_lights() {
  green_light_lamp.set(traffic_light.is_in_user_do_stuff_state());
  if (!motor_upper_enable.get_state()) {
    green_light_lamp.set(0);
  }
  red_light_lamp.set(!green_light_lamp.get_state());
}

void manage_traffic_lights() {

  // SHOW START SCREEN:
  if (!traffic_light.is_in_start_state() && !state_controller.machine_is_running()) {
    traffic_light.set_info_start();
  }

  // GO TO SLEEP:
  if (traffic_light.is_in_user_do_stuff_state() && motor_display_sleep_timeout.has_timed_out()) {
    traffic_light.set_info_sleep();
  }
  // WAKE UP:
  if (traffic_light.is_in_sleep_state() && !motor_display_sleep_timeout.has_timed_out()) {
    traffic_light.set_info_user_do_stuff();
  }

  manage_signal_lights();
}

long measure_runtime() {
  static long previous_micros = micros();
  long time_elapsed = micros() - previous_micros;
  previous_micros = micros();
  return time_elapsed;
}

void move_sledge() {
  cylinder_sledge_inlet.set(1);
  cylinder_sledge_vent.set(1);
}

void block_sledge() {
  cylinder_sledge_inlet.set(0);
  cylinder_sledge_vent.set(1);
}

void vent_sledge() {
  cylinder_sledge_inlet.set(0);
  cylinder_sledge_vent.set(0);
}

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

int measure_force() {
  float sensor_adc_value = analogRead(PRESSURE_SENSOR_PIN);
  int force = calculate_pressure_from_adc(sensor_adc_value);
  return force;
}

void measure_and_display_max_force() {
  int force = measure_force();
  static int max_force;
  static int previous_max_force;

  if (force > max_force) {
    max_force = force;
    erase_force_value_timeout.reset_time();
  }

  if (max_force > previous_max_force && pressure_update_delay.delay_time_is_up(100)) {
    display_force(max_force);
    previous_max_force = max_force;
  }

  if (erase_force_value_timeout.has_timed_out()) {
    max_force = -1; // negative to make certain value updates
    previous_max_force = -1;
    erase_force_value_timeout.reset_time();
  }
}

void measure_and_display_current_force() {
  int force = measure_force();
  static int previous_force;
  static int min_difference = 50;

  if (abs(force - previous_force) >= min_difference &&
      pressure_update_delay.delay_time_is_up(100)) {
    display_force(force);
    previous_force = force;
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

void button_traffic_light_push(void *ptr) {

  if (traffic_light.is_in_start_state()) {
    state_controller.set_machine_running();
    nex_state_machine_running = !nex_state_machine_running;
  }
  if (traffic_light.is_in_sleep_state()) {
    motor_output_enable(); // wakes the tool up
  }
}

void switch_step_auto_mode_push(void *ptr) {
  state_controller.toggle_step_auto_mode();
  nex_state_step_mode = state_controller.is_in_step_mode();
}

void button_stepback_push(void *ptr) {
  state_controller.set_machine_stop();
  reset_flag_of_current_step();
  state_controller.switch_to_previous_step();
  reset_flag_of_current_step();
}

void button_next_step_push(void *ptr) {
  state_controller.set_machine_stop();
  reset_flag_of_current_step();
  state_controller.switch_to_next_step();
  reset_flag_of_current_step();
}

void button_reset_cycle_push(void *ptr) {
  reset_flag_of_current_step();
  set_initial_cylinder_states();
  state_controller.set_reset_mode(true);
  clear_text_field("t4");
  hide_info_field();
}

// TOUCH EVENT FUNCTIONS PAGE 1 - RIGHT SIDE -----------------------------------

void switch_motor_brake_push(void *ptr) {
  motor_output_toggle();
  nex_state_motor_brake = !nex_state_motor_brake;
}
void button_motor_oben_push(void *ptr) { //
  start_upper_motor();
}
void button_motor_oben_pop(void *ptr) { //
  stop_upper_motor();
}
void button_motor_unten_push(void *ptr) { //
  start_lower_motor();
}
void button_motor_unten_pop(void *ptr) { //
  stop_lower_motor();
}
void switch_air_release_push(void *ptr) {
  cylinder_sledge_vent.toggle();
  nex_state_air_release = !nex_state_air_release;
}
void button_schneiden_push(void *ptr) {
  cylinder_blade.set(1);
  cylinder_frontclap.set(1);
}
void button_schneiden_pop(void *ptr) {
  cylinder_blade.set(0);
  cylinder_frontclap.set(0);
}
void button_schlitten_push(void *ptr) { //
  cylinder_sledge_inlet.set(1);
}
void button_schlitten_pop(void *ptr) { //
  cylinder_sledge_inlet.set(0);
}

// TOUCH EVENT FUNCTIONS PAGE 2 - LEFT SIDE ------------------------------------

void button_upper_slider_left_push(void *ptr) { decrease_slider_value(upper_strap_feed); }
void button_upper_slider_right_push(void *ptr) { increase_slider_value(upper_strap_feed); }
void button_lower_slider_left_push(void *ptr) { decrease_slider_value(lower_strap_feed); }
void button_lower_slider_right_push(void *ptr) { increase_slider_value(lower_strap_feed); }
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

void switch_continuous_mode_push(void *ptr) {
  state_controller.set_continuous_mode();
  nex_state_continuous_mode = !nex_state_continuous_mode;
}

// TOUCH EVENT FUNCTIONS PAGE 2 - RIGHT SIDE -----------------------------------

void button_reset_shorttime_counter_push(void *ptr) {
  counter.set_value(shorttime_counter, 0);

  // ACTIVATE TIMEOUT TO RESET LONGTIME COUNTER:
  nex_reset_button_timeout.reset_time();
  nex_reset_button_timeout.set_flag_activated(1);
}
void button_reset_shorttime_counter_pop(void *ptr) {
  nex_reset_button_timeout.set_flag_activated(0);
}

// PAGE CHANGING EVENTS (TRIGGER UPDATE OF ALL DISPLAY ELEMENTS) ---------------

void page_0_push(void *ptr) { nex_current_page = 0; }
void page_1_push(void *ptr) {
  nex_current_page = 1;
  hide_info_field();

  // REFRESH BUTTON STATES:
  nex_prev_cycle_step = !state_controller.get_current_step();
  nex_state_step_mode = true;
  nex_state_air_release = 1;
  nex_state_motor_brake = 0;
  nex_state_upper_motor = 0;
  nex_state_sledge = 0;
  nex_state_blade = 0;
  nex_state_lower_motor = 0;
  nex_state_machine_running = 0;
  traffic_light.set_info_has_changed();
}
void page_2_push(void *ptr) {
  nex_current_page = 2;
  update_field_values_page_2();
}
void update_field_values_page_2() {
  nex_upper_strap_feed = counter.get_value(nex_upper_strap_feed) - 1;
  nex_lower_strap_feed = counter.get_value(nex_lower_strap_feed) - 1;
  nex_shorttime_counter = counter.get_value(nex_upper_strap_feed) - 1;
  nex_longtime_counter = counter.get_value(nex_upper_strap_feed) - 1;
  //nex_state_continuous_mode = true;
}

// DECLARE DISPLAY EVENT LISTENERS *********************************************

void attach_push_and_pop() {
  // PAGE 0 PUSH ONLY:
  nex_page_0.attachPush(page_0_push);
  // PAGE 1 PUSH ONLY:
  nex_page_1.attachPush(page_1_push);
  button_previous_step.attachPush(button_stepback_push);
  button_next_step.attachPush(button_next_step_push);
  button_reset_cycle.attachPush(button_reset_cycle_push);
  button_previous_step.attachPush(button_stepback_push);
  button_next_step.attachPush(button_next_step_push);
  button_traffic_light.attachPush(button_traffic_light_push);
  switch_step_auto_mode.attachPush(switch_step_auto_mode_push);
  switch_motor_brake.attachPush(switch_motor_brake_push);
  switch_air_release.attachPush(switch_air_release_push);
  // PAGE 1 PUSH AND POP:
  button_upper_motor.attachPush(button_motor_oben_push);
  button_upper_motor.attachPop(button_motor_oben_pop);
  button_lower_motor.attachPush(button_motor_unten_push);
  button_lower_motor.attachPop(button_motor_unten_pop);
  button_cut.attachPush(button_schneiden_push);
  button_cut.attachPop(button_schneiden_pop);
  button_sledge.attachPush(button_schlitten_push);
  button_sledge.attachPop(button_schlitten_pop);
  // PAGE 2 PUSH ONLY:
  nex_page_2.attachPush(page_2_push);
  button_slider_1_left.attachPush(button_upper_slider_left_push);
  button_slider_1_right.attachPush(button_upper_slider_right_push);
  button_slider_2_left.attachPush(button_lower_slider_left_push);
  button_slider_2_right.attachPush(button_lower_slider_right_push);
  switch_continuous_mode.attachPush(switch_continuous_mode_push);
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
  traffic_light.set_info_start();

  delay(4000);
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
  update_traffic_light_field();

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

void update_continuous_cycle_name() {
  if (nex_prev_cycle_step != state_controller.get_current_step()) {
    String number = String(state_controller.get_current_step() + 1);
    String name = get_continuous_cycle_display_string();
    Serial.println(number + " " + name);
    display_text_in_field(number + " " + name, "t0");
    nex_prev_cycle_step = state_controller.get_current_step();
  }
}

void update_cycle_name() {
  if (state_controller.is_in_step_mode() || state_controller.is_in_auto_mode()) {
    update_main_cycle_name();
  }

  if (state_controller.is_in_continuous_mode()) {
    update_continuous_cycle_name();
  }
}

String get_main_cycle_display_string() {
  int current_step = state_controller.get_current_step();
  String display_text_cycle_name = main_cycle_steps[current_step]->get_display_text();
  return display_text_cycle_name;
}

String get_continuous_cycle_display_string() {
  int current_step = state_controller.get_current_step();
  String display_text_cycle_name = continuous_cycle_steps[current_step]->get_display_text();
  return display_text_cycle_name;
}

void update_traffic_light_field() {
  if (traffic_light.info_has_changed()) {
    String color = traffic_light.get_info_color();
    set_traffic_light_field_color(color);
    String text = traffic_light.get_info_text();
    set_traffic_light_field_text(text);
  }
}

void set_traffic_light_field_text(String text) { display_text_in_field(text, "b8"); }

void set_traffic_light_field_color(String color) {
  Serial2.print("b8.bco=" + color);
  send_to_nextion();
}

// DISPLAY LOOP PAGE 1 RIGHT SIDE: ---------------------------------------------

void display_loop_page_1_right_side() {

  // UPDATE SWITCHES:
  if (cylinder_sledge_vent.get_state() != nex_state_air_release) {
    toggle_ds_switch("bt3");
    nex_state_air_release = !nex_state_air_release;
  }
  if (motor_upper_enable.get_state() != nex_state_motor_brake) {
    toggle_ds_switch("bt5");
    nex_state_motor_brake = !nex_state_motor_brake;
  }

  // UPDATE BUTTONS:
  if (cylinder_sledge_inlet.get_state() != nex_state_sledge) {
    bool state = cylinder_sledge_inlet.get_state();
    String button = "b6";
    set_momentary_button_high_or_low(button, state);
    nex_state_sledge = cylinder_sledge_inlet.get_state();
  }
  if (motor_upper_pulse.get_state() != nex_state_upper_motor) {
    bool state = motor_upper_pulse.get_state();
    String button = "b4";
    set_momentary_button_high_or_low(button, state);
    nex_state_upper_motor = motor_upper_pulse.get_state();
  }
  if (cylinder_blade.get_state() != nex_state_blade) {
    bool state = cylinder_blade.get_state();
    String button = "b5";
    set_momentary_button_high_or_low(button, state);
    nex_state_blade = cylinder_blade.get_state();
  }
  if (motor_lower_pulse.get_state() != nex_state_lower_motor) {
    bool state = motor_lower_pulse.get_state();
    String button = "b3";
    set_momentary_button_high_or_low(button, state);
    nex_state_lower_motor = motor_lower_pulse.get_state();
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
  if (counter.get_value(upper_strap_feed) != nex_upper_strap_feed) {
    display_text_in_field(add_suffix_to_eeprom_value(upper_strap_feed, "mm"), "t4");
    nex_upper_strap_feed = counter.get_value(upper_strap_feed);
  }
}
void update_lower_slider_value() {
  if (counter.get_value(lower_strap_feed) != nex_lower_strap_feed) {
    display_text_in_field(add_suffix_to_eeprom_value(lower_strap_feed, "mm"), "t2");
    nex_lower_strap_feed = counter.get_value(lower_strap_feed);
  }
}
String add_suffix_to_eeprom_value(int eeprom_value_number, String suffix) {
  String value = String(counter.get_value(eeprom_value_number));
  String space = " ";
  String suffixed_string = value + space + suffix;
  return suffixed_string;
}
void update_switches_page_2_left() {
  if (state_controller.is_in_continuous_mode() != nex_state_continuous_mode) {
    toggle_ds_switch("bt3");
    nex_state_continuous_mode = !nex_state_continuous_mode;
  }
}

// DIPLAY LOOP PAGE 2 RIGHT SIDE: ----------------------------------------------

void display_loop_page_2_right_side() {
  update_upper_counter_value();
  update_lower_counter_value();
  reset_lower_counter_value();
}

void update_upper_counter_value() {
  if (nex_longtime_counter != counter.get_value(longtime_counter)) {
    display_text_in_field(String(counter.get_value(longtime_counter)), "t10");
    nex_longtime_counter = counter.get_value(longtime_counter);
  }
}
void update_lower_counter_value() {
  // UPDATE LOWER COUNTER:
  if (nex_shorttime_counter != counter.get_value(shorttime_counter)) {
    display_text_in_field(String(counter.get_value(shorttime_counter)), "t12");
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

//------------------------------------------------------------------------------
class User_do_stuff : public Cycle_step {
  String get_display_text() { return "SPANNEN + CRIMPEN"; }
  int substep = 0;

  void do_initial_stuff() {
    block_sledge();
    motor_output_enable();
    traffic_light.set_info_user_do_stuff();
    substep = 0;
    show_info_field();
    display_text_in_info_field("ZUGKRAFT");
    cycle_step_delay.set_unstarted();
  }
  void do_loop_stuff() {
    if (sensor_sledge_endposition.switched_high()) {
      substep = 1;
    }
    if (substep == 1) {
      if (cycle_step_delay.delay_time_is_up(3700)) {
        counter.count_one_up(shorttime_counter);
        counter.count_one_up(longtime_counter);
        set_loop_completed();
      }
    }
  }
};
//------------------------------------------------------------------------------
class Release_air : public Cycle_step {
  String get_display_text() { return "LUFT ABLASSEN"; }

  void do_initial_stuff() {
    traffic_light.set_info_machine_do_stuff();
    motor_output_enable();
    vent_sledge();
    cycle_step_delay.set_unstarted();
  }
  void do_loop_stuff() {
    if (cycle_step_delay.delay_time_is_up(2000)) {
      set_loop_completed();
    }
  }
};
//------------------------------------------------------------------------------
class Release_brake : public Cycle_step {
  String get_display_text() { return "BREMSE LOESEN"; }

  void do_initial_stuff() {
    vent_sledge();
    hide_info_field();
    traffic_light.set_info_machine_do_stuff();
    motor_output_disable();
    cycle_step_delay.set_unstarted();
  }
  void do_loop_stuff() {
    if (cycle_step_delay.delay_time_is_up(1000)) {
      set_loop_completed();
    }
  }
};
//------------------------------------------------------------------------------
class Sledge_back : public Cycle_step {
  String get_display_text() { return "ZURUECKFAHREN"; }

  void do_initial_stuff() {
    traffic_light.set_info_machine_do_stuff();
    motor_output_disable();
    move_sledge();
    cycle_step_delay.set_unstarted();
  }
  void do_loop_stuff() {
    if (cycle_step_delay.delay_time_is_up(1800)) {
      vent_sledge();
      set_loop_completed();
    }
  }
};
//------------------------------------------------------------------------------
class Cut_strap : public Cycle_step {
  String get_display_text() { return "SCHNEIDEN"; }

  void do_initial_stuff() {
    traffic_light.set_info_machine_do_stuff();
    motor_output_disable();
    vent_sledge();
    cylinder_frontclap.set(1);
  }
  void do_loop_stuff() {
    cylinder_blade.stroke(1300, 1000);
    if (cylinder_blade.stroke_completed()) {
      cylinder_frontclap.set(0);
      set_loop_completed();
    }
  }
};
//------------------------------------------------------------------------------
class Feed_straps : public Cycle_step {
  String get_display_text() { return "BAND VORSCHIEBEN"; }
  bool upper_strap_completed = false;
  bool lower_strap_completed = false;

  void do_initial_stuff() {
    traffic_light.set_info_machine_do_stuff();
    upper_strap_completed = false;
    lower_strap_completed = false;
    upper_feed_delay.set_unstarted();
    lower_feed_delay.set_unstarted();
    cycle_step_delay.set_unstarted();
    block_sledge();
    start_upper_motor();
    start_lower_motor();
  }
  void do_loop_stuff() {

    unsigned long upper_feedtime = calculate_feedtime_from_mm(counter.get_value(upper_strap_feed));
    if (upper_feed_delay.delay_time_is_up(upper_feedtime)) {
      stop_upper_motor();
      upper_strap_completed = true;
    }

    unsigned long lower_feedtime = calculate_feedtime_from_mm(counter.get_value(lower_strap_feed));
    if (lower_feed_delay.delay_time_is_up(lower_feedtime)) {
      stop_lower_motor();
      lower_strap_completed = true;
    }

    if (upper_strap_completed && lower_strap_completed) {
      if (cycle_step_delay.delay_time_is_up(500)) {
        motor_output_disable();
        set_loop_completed();
      }
    }
  }
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

// CLASSES FOR CONTINUOUS MODE *************************************************

class Continuous_vent : public Cycle_step {
  String get_display_text() { return "ENTLUEFTEN"; }

  void do_initial_stuff() {
    traffic_light.set_info_user_do_stuff();
    vent_sledge();
    cycle_step_delay.set_unstarted();
  }
  void do_loop_stuff() {
    if (cycle_step_delay.delay_time_is_up(1000)) {
      set_loop_completed();
    }
  }
};
//------------------------------------------------------------------------------
class Continuous_sledge_back : public Cycle_step {
  String get_display_text() { return "ZURUECKFAHREN"; }
  bool has_reached_startpoint = false;

  void do_initial_stuff() {
    traffic_light.set_info_user_do_stuff();
    motor_output_disable();
    move_sledge();
    has_reached_startpoint = false;
    cycle_step_delay.set_unstarted();
  }
  void do_loop_stuff() {

    if (sensor_sledge_startposition.get_button_state()) {
      has_reached_startpoint = true;
    }

    if (has_reached_startpoint) {
      if (cycle_step_delay.delay_time_is_up(4000)) {
        block_sledge();
        motor_output_enable();
        set_loop_completed();
      }
    }
  }
};
//------------------------------------------------------------------------------
class Continuous_release_pulses : public Cycle_step {
  String get_display_text() { return "PULSEN"; }
  int substep = 1;

  void do_initial_stuff() {
    block_sledge();
    motor_output_enable();
    traffic_light.set_info_user_do_stuff();
    substep = 1;
    show_info_field();
    display_text_in_info_field("ZUGKRAFT");
    cycle_step_delay.set_unstarted();
  }
  void do_loop_stuff() {
    if (substep == 1) {
      if (cycle_step_delay.delay_time_is_up(1500)) {
        vent_sledge();
        substep = 2;
      }
    }
    if (substep == 2) {
      if (cycle_step_delay.delay_time_is_up(100)) {
        block_sledge();
        substep = 1;
      }
    }
    if (sensor_sledge_endposition.switched_high()) {
      set_loop_completed();
    }
  }
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

// STEPPER MOTOR SETUP *********************************************************

void setup_stepper_motors() {

  // PINS:
  pinMode(UPPER_MOTOR_DIRECTION_PIN, OUTPUT);
  pinMode(LOWER_MOTOR_DIRECTION_PIN, OUTPUT);
  digitalWrite(UPPER_MOTOR_DIRECTION_PIN, HIGH);
  digitalWrite(LOWER_MOTOR_DIRECTION_PIN, HIGH);
}

// MAIN SETUP ******************************************************************

void setup() {
  setup_stepper_motors();
  set_initial_cylinder_states();
  //------------------------------------------------
  // SETUP PIN MODES:
  // n.a.

  //------------------------------------------------
  // PUSH THE CYCLE STEPS INTO THE VECTOR CONTAINER:
  // PUSH SEQUENCE = CYCLE SEQUENCE!
  main_cycle_steps.push_back(new User_do_stuff);
  main_cycle_steps.push_back(new Release_air);
  main_cycle_steps.push_back(new Release_brake);
  main_cycle_steps.push_back(new Sledge_back);
  main_cycle_steps.push_back(new Cut_strap);
  main_cycle_steps.push_back(new Feed_straps);
  //------------------------------------------------
  // CONFIGURE THE STATE CONTROLLER:
  int no_of_main_cycle_steps = main_cycle_steps.size();
  state_controller.set_no_of_steps(no_of_main_cycle_steps);
  //------------------------------------------------
  // PUSH THE STEPS FOR CONTINUOUS MODE IN A CONTAINER:
  continuous_cycle_steps.push_back(new Continuous_vent);
  continuous_cycle_steps.push_back(new Continuous_sledge_back);
  continuous_cycle_steps.push_back(new Continuous_release_pulses);
  int no_of_continuous_cycle_steps = continuous_cycle_steps.size();
  state_controller.set_no_of_continuous_steps(no_of_continuous_cycle_steps);
  //------------------------------------------------
  // SETUP COUNTER:
  counter.setup(0, 1023, counter_no_of_values);
  //------------------------------------------------
  Serial.begin(115200);
  state_controller.set_auto_mode();
  state_controller.set_machine_running();
  Serial.println("EXIT SETUP");
  //------------------------------------------------
  nextion_display_setup();
  // REQUIRED STEP TO MAKE SKETCH WORK AFTER RESET:
  reset_flag_of_current_step();
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
    if (state_controller.is_in_step_mode()) {
      state_controller.set_machine_stop();
    }
  }

  // IF MACHINE STATE IS "RUNNING", RUN CURRENT STEP:
  if (state_controller.machine_is_running()) {
    main_cycle_steps[state_controller.get_current_step()]->do_stuff();
  }

  // MEASURE AND DISPLAY PRESSURE
  measure_and_display_max_force();
}

void run_continuous_mode() {
  // IF STEP IS COMPLETED SWITCH TO NEXT STEP:
  if (continuous_cycle_steps[state_controller.get_current_step()]->is_completed()) {
    state_controller.switch_to_next_step();
    reset_flag_of_current_step();
  }

  // IF MACHINE STATE IS "RUNNING", RUN CURRENT STEP:
  if (state_controller.machine_is_running()) {
    continuous_cycle_steps[state_controller.get_current_step()]->do_stuff();
  }

  // MEASURE AND DISPLAY PRESSURE
  measure_and_display_current_force();
}

void loop() {

  // UPDATE DISPLAY:
  nextion_display_loop();

  // MONITOR MOTOR BRAKE TO PREVENT FROM OVERHEATING:
  monitor_motor_output();

  // RUN STEP OR AUTO MODE:
  if (state_controller.is_in_step_mode() || state_controller.is_in_auto_mode()) {
    run_step_or_auto_mode();
  }

  // RUN CONTINUOUS MODE:
  if (state_controller.is_in_continuous_mode()) {
    run_continuous_mode();
  }

  // RESET RIG IF RESET IS ACTIVATED:
  if (state_controller.reset_mode_is_active()) {
    reset_machine();
  }

  // MANAGE TRAFFIC LIGHTS:
  manage_traffic_lights();

  // DISPLAY DEBUG INFOMATION:
  unsigned long runtime = measure_runtime();
  if (print_interval_timeout.has_timed_out()) {
    //Serial.println(runtime);
    print_interval_timeout.reset_time();
  }
}

// END OF PROGRAM **************************************************************