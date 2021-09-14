#!/usr/bin/env python3

import os
import sys
import time
import serial
import serial.tools.list_ports
from helper_and_subclasses.log_object import log_object
from helper_and_subclasses.serial_scanner import serial_scanner
from helper_and_subclasses.firebase_helper import firebase_helper
from helper_and_subclasses.email_helper import email_helper

'''
--------------------------------------------------------------------------------
The "Controllino PLC" provides the "RPI Log Manager" with following info:
    • cycle-number
    • start of tensioning
    • tension force
    • start of crimping

The "Arduino Current Logger" provides the "RPI Log Manager" with following info:
    • peak current after rising above a certain current threshold

The "RPI Log Manager" receives all info, combines them to logs and pushes
the logs to firebase
--------------------------------------------------------------------------------
'''


class log_manager():

    def __init__(self):

        # MONITOR CONNECTION STATUS:
        self.arduino_port_is_available = False
        self.arduino_port_is_connected = False

        self.controllino_port_is_available = False
        self.controllino_port_is_connected = False

        # DEVICE IDENTIFIERS:
        # VID = VENDOR ID; PID = PRODUCT ID

        # ARDUINO:
        self.arduino_vid = 1027  # Arduino Uno: 9025
        self.arduino_pid = 24577  # Arduino Uno: 67
        self.arduino_port = 0
        self.arduino_serial = 0

        # CONTROLLINO:
        # The USB-Serial-Converter is connected to Controllino TX1/RX1
        self.usb_serial_converter_vid = 1659
        self.usb_serial_converter_pid = 8963
        self.controllino_port = 0
        self.controllino_serial = 0
        # self.controllino_vid=9025 # replaced by serial converter
        # self.controllino_pid=66 # replaced by serial converter

        self.serial_scanner = serial_scanner()
        self.log_object = log_object()
        self.firebase_helper = firebase_helper()
        self.email_helper = email_helper()

    def get_list_of_serial_devices(self):
        self.serial_scanner.print_list_of_serial_devices()

    def get_port_by_ids(self, vid, pid):
        try:
            ports = serial.tools.list_ports.comports(include_links=False)
            for port in ports:
                if(port.vid == vid) and (port.pid == pid):
                    device_port = port.device
                    return device_port

        except Exception as error:
            error_message = 'CAUGHT AN ERROR WHILE TRYING TO FIND DEVICE PORT'
            print(error_message, error)

    def reset_arduino_connection(self):
        self.arduino_port_is_available = False
        self.arduino_port_is_connected = False
        self.arduino_serial = 0

    def reset_controllino_connection(self):
        self.controllino_port_is_available = False
        self.controllino_port_is_connected = False
        self.controllino_serial = 0

    def get_port_of_arduino(self):
        if(self.arduino_port_is_available == False):
            print('---------------------------------------------')
            print('TRY TO FIND ARDUINO PORT BY IDS:')
            # Arduino Current Logger
            self.arduino_port = self.get_port_by_ids(
                self.arduino_vid,
                self.arduino_pid)
            if(self.arduino_port):
                self.arduino_port_is_available = True
                print(f'ARDUINO PORT: {self.arduino_port}')
            else:
                print('ARDUINO PORT NOT AVAILABLE')
                self.reset_arduino_connection
            print('---------------------------------------------')

    def get_port_of_controllino(self):
        if(self.controllino_port_is_available == False):
            print('---------------------------------------------')
            print('TRY TO FIND CONTROLLINO PORT BY IDS:')
            # USB-Serial-Converter @ Controllino TX1/RX1
            self.controllino_port = self.get_port_by_ids(
                self.usb_serial_converter_vid,
                self.usb_serial_converter_pid)
            if(self.controllino_port):
                self.controllino_port_is_available = True
                print(f'CONTROLLINO PORT: {self.controllino_port}')
            else:
                print('CONTROLLINO PORT NOT AVAILABLE')
                self.reset_controllino_connection
            print('---------------------------------------------')

    def get_port_connection(self, port):
        try:
            port_connection = serial.Serial(
                port=port,
                baudrate=115200,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                bytesize=serial.EIGHTBITS,
                timeout=0)
            return port_connection

        except Exception as error:
            error_message = 'CAUGHT AN ERROR WHILE TRYING TO GET PORT CONNECTION'
            print(error_message, error)

    def connect_to_arduino(self):
        if(self.arduino_port_is_available == True and self.arduino_port_is_connected == False):
            print('---------------------------------------------')
            print('TRY TO CONNECT ARDUINO:')
            try:
                self.arduino_serial = self.get_port_connection(self.arduino_port)
                print("CONNECTED TO: " + self.arduino_serial.portstr)
                self.arduino_port_is_connected = True
            except Exception as error:
                error_message = 'CAUGHT AN ERROR WHILE TRYING TO CONNECT TO ARDUINO'
                self.reset_arduino_connection()
                print(error_message, error)
            print('---------------------------------------------')

    def connect_to_controllino(self):
        if(self.controllino_port_is_available == True and self.controllino_port_is_connected == False):
            print('---------------------------------------------')
            print('TRY TO CONNECT TO CONTROLLINO:')
            try:
                self.controllino_serial = self.get_port_connection(self.controllino_port)
                print("CONNECTED TO: " + self.controllino_serial.portstr)
                self.controllino_port_is_connected = True

            except Exception as error:
                error_message = 'CAUGHT AN ERROR WHILE TRYING TO CONNECT TO CONTROLLINO'
                print(error_message, error)
                self.reset_controllino_connection()
            print('---------------------------------------------')

    def process_serial_read(self, readline):
        readline = readline.decode('utf-8')
        if(readline == ""):
            return
        print(readline)
        readline = readline.split(';')

        if(readline[0] == 'LOG'):
            self.add_info_to_log(readline)

        if(readline[0] == 'EMAIL'):
            self.send_email(readline)

    def upload_log(self):
        data = self.log_object.get_db_string()
        self.firebase_helper.push(data)

    def set_log_completed(self):
        self.log_object.print_log()
        # Do not upload empty log at startup:
        if(self.log_object.cycle_total != 0):
            self.upload_log()

        self.log_object.reset_log()

    def send_email(self, readline):
        if readline[1] == 'MACHINE_STOPPED':
            print('SEND EMAIL MACHINE STOPPED')
            self.email_helper.send_message_machine_stopped()
        if readline[1] == 'BUTTON_PUSHED':
            print('SEND EMAIL BUTTON PUSHED')
            self.email_helper.send_message_button_pushed()

    def add_info_to_log(self, readline):

        if readline[1] == 'CYCLE_TOTAL':
            self.set_log_completed()
            self.log_object.cycle_total = readline[2]

        if readline[1] == 'CYCLE_RESET':
            self.log_object.cycle_reset = readline[2]

        if readline[1] == 'FORCE_TENSION':
            self.log_object.tension_force = readline[2]

        if readline[1] == 'START_TENSION':
            self.log_object.set_tool_is_tensioning()

        if readline[1] == 'START_CRIMP':
            self.log_object.set_tool_is_crimping()

        if readline[1] == 'CURRENT_MAX':
            if(self.log_object.tool_is_tensioning):
                self.log_object.tension_current = readline[2]
            if(self.log_object.tool_is_crimping):
                self.log_object.crimp_current = readline[2]

    def read_arduino_serial(self):
        if(self.arduino_serial):
            try:
                log_manager.process_serial_read(self.arduino_serial.readline())
            except Exception as error:
                error_message = 'CAUGHT AN ERROR WHILE TRYING TO READ ARDUINO SERIAL'
                print(error_message, error)
                self.reset_arduino_connection()

    def read_controllino_serial(self):
        if(self.controllino_serial):
            try:
                log_manager.process_serial_read(self.controllino_serial.readline())
            except Exception as error:
                error_message = 'CAUGHT AN ERROR WHILE TRYING TO READ CONTROLLINO SERIAL'
                print(error_message, error)
                self.reset_controllino_connection()


if __name__ == '__main__':

    log_manager = log_manager()

    log_manager.get_list_of_serial_devices()

    time.sleep(3)

    while True:

        try:
            # FIND PORTS
            log_manager.get_port_of_arduino()
            log_manager.get_port_of_controllino()

            # ESTABLISH CONNECTION
            log_manager.connect_to_arduino()
            log_manager.connect_to_controllino()

            # READ AND PROCESS
            log_manager.read_arduino_serial()
            log_manager.read_controllino_serial()

        except Exception as error:
            error_message = 'CAUGHT AN ERROR IN THE MAIN LOOP, THIS SHOULD NOT HAPPEN !!!'
            print(error_message, error)

        time.sleep(0.5)

    ser.close()
