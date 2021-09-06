#!/usr/bin/env python3

from email_helper import email_helper
import os
import sys
import time
import serial
import serial.tools.list_ports
from serial_scanner import *
from log_object import log_object
from firebase_helper import firebase_helper
from email_helper import email_helper


class log_manager():

    def __init__(self):

        # DEVICE IDENTIFIERS:
        # VID = VENDOR ID; PID = PRODUCT ID

        # ARDUINO:
        self.arduino_mega_vid = 6790
        self.arduino_mega_pid = 29987
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
            error_message = 'CAUGHT AN ERROR WHILE TRYING TO FIND DEVICE PORT !!!'
            print(error_message, error)

    def get_ports_of_devices(self):
        print('TRY TO FIND CONTROLLINO AND ARDUINO PORTS BY IDS:')
        # Arduino Mega Current Logger
        self.arduino_port = self.get_port_by_ids(
            self.arduino_mega_vid,
            self.arduino_mega_pid)
        print(f'ARDUINO PORT: {self.arduino_port}')

        # USB-Serial-Converter @ Controllino TX1/RX1
        self.controllino_port = self.get_port_by_ids(
            self.usb_serial_converter_vid,
            self.usb_serial_converter_pid)
        print(f'CONTROLLINO PORT: {self.controllino_port}')

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
            error_message = 'CAUGHT AN ERROR WHILE TRYING TO GET PORT CONNECTION !!!'
            print(error_message, error)

    def connect_to_serial_devices(self):
        try:
            print('---------------------------------------------')
            print('TRY TO CONNECT TO CONTROLLINO:')
            self.controllino_serial = self.get_port_connection(self.controllino_port)
            print("connected to: " + self.controllino_serial.portstr)

            print('TRY TO CONNECT ARDUINO:')
            self.arduino_serial = self.get_port_connection(self.arduino_port)
            print("connected to: " + self.arduino_serial.portstr)
        except Exception as error:
            error_message = 'CAUGHT AN ERROR WHILE TRYING TO CONNECT TO PORT !!!'
            print(error_message, error)

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
                self.log_object.current_tension = readline[2]
            if(self.log_object.tool_is_crimping):
                self.log_object.crimp_current = readline[2]

    def read_serial_ports(self):
        log_manager.process_serial_read(self.controllino_serial.readline())
        log_manager.process_serial_read(self.arduino_serial.readline())

if __name__ == '__main__':

    log_manager = log_manager()

    log_manager.get_list_of_serial_devices()
    log_manager.get_ports_of_devices()
    log_manager.connect_to_serial_devices()

    while True:

        try:
            # Read and process serial inputs:
            log_manager.read_serial_ports()
        except Exception as error:
            error_message = 'CAUGHT AN ERROR IN THE MAIN LOOP !!!'
            print(error_message, error)
            log_manager.get_list_of_serial_devices()
            log_manager.connect_to_serial_devices()
            time.sleep(3)

        time.sleep(0.5)

    ser.close()
