#include "traffic_light.h"
#include "Arduino.h"

void Traffic_light::set_info_start() {
  _info_has_changed = true;
  _info_color = _blue;
  _info_text = "START";

  _sleep_state_active = false;
  _user_do_stuff_state_active = false;
  _start_state_active = true;
}

void Traffic_light::set_info_user_do_stuff() {
  _info_has_changed = true;
  _info_color = _green;
  _info_text = "CRIMPEN";
  _sleep_state_active = false;
  _user_do_stuff_state_active = true;
  _start_state_active = false;
}

void Traffic_light::set_info_machine_do_stuff() {
  _info_has_changed = true;
  _info_color = _red;
  _info_text = "WARTEN";
  _sleep_state_active = false;
  _user_do_stuff_state_active = false;
  _start_state_active = false;
}

void Traffic_light::set_info_sleep() {
  _info_has_changed = true;
  _info_color = _blue;
  _info_text = "SLEEP";
  _sleep_state_active = true;
  _user_do_stuff_state_active = false;
  _start_state_active = false;
}

String Traffic_light::get_info_color() { //
  return _info_color;
}
String Traffic_light::get_info_text() { //
  return _info_text;
}

bool Traffic_light::is_in_sleep_state() { //
  return _sleep_state_active;
}

bool Traffic_light::is_in_user_do_stuff_state() {
  return _user_do_stuff_state_active;
}
bool Traffic_light::is_in_start_state() { //
  return _start_state_active;
}

bool Traffic_light::info_has_changed() {
  if (_info_has_changed) {
    _info_has_changed = false;
    return true;
  }
  return false;
}

void Traffic_light::set_info_has_changed() { _info_has_changed = true; }