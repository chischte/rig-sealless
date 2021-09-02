#!/usr/bin/env python3

import os
import sys
import time
import serial
import serial.tools.list_ports
from log import log
from firebase_logger import firebase_logger

class rapberry_log_merger():

    def __init__(self):
        self.arduino = 0
        self.controllino = 0
        self.log_object = log()
        self.firebase_logger = firebase_logger()

    def get_list_of_serial_devices(self):
        # Search available ports:
        print('Search...')
        ports = serial.tools.list_ports.comports(include_links=False)
        print('---------------------------------------------')
        print('PORTSCAN RESULTS:')
        for port in ports:
            print('---------------------------------------------')
            print(port.device)
            print(port.manufacturer)
            print(port.hwid)
            print(port.serial_number)  # Controllino: 85830303039351315291
            print(port.description)  # Arduino Mega: USB-SERIAL CH340 (COM5)
            print(port.vid)
            print(port.pid)
            print('---------------------------------------------')

    def connect_to_serial_devices(self):

        ports = serial.tools.list_ports.comports(include_links=False)

        for port in ports:
            if(port.vid == 9025) and (port.pid == 66):
                arduino_port = port.device
                print(f'ARDUINO PORT: {arduino_port}')
            if(port.vid == 6790) and (port.pid == 29987):
                controllino_port = port.device
                print(f'CONTROLLINO PORT: {controllino_port}')

        self.controllino = serial.Serial(
            port=controllino_port,
            baudrate=115200,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS,
            timeout=0)
        print("connected to: " + self.controllino.portstr)

        self.arduino = serial.Serial(
            port=arduino_port,
            baudrate=115200,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS,
            timeout=0)
        print("connected to: " + self.arduino.portstr)

    def process_serial_read(self, readline):
        readline = readline.decode('utf-8')
        if(readline == ""):
            return
        readline = readline.split(';')
        if(readline[0] == 'LOG'):
            self.add_info_to_log(readline)

    def upload_log(self):
        data=self.log_object.get_db_string()
        self.firebase_logger.push(data)

    def set_log_completed(self):
        self.log_object.print_log()
        self.upload_log()
        self.log_object.reset_log()

    def add_info_to_log(self, readline):

        if readline[1] == 'CYCLE_TOTAL':
            self.set_log_completed()
            self.log_object.cycle_total = readline[2]

        if readline[1] == 'CYCLE_RESET':
            self.log_object.cycle_reset = readline[2]

        if readline[1] == 'FORCE':
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
        log_merger.process_serial_read(self.controllino.readline())
        log_merger.process_serial_read(self.arduino.readline())


if __name__ == '__main__':

    log_merger = rapberry_log_merger()

    log_merger.get_list_of_serial_devices()
    log_merger.connect_to_serial_devices()

    while True:

        # Read and process serial inputs:
        log_merger.read_serial_ports()

        time.sleep(0.5)

    ser.close()
