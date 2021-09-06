#!/usr/bin/env python3

# USE THIS PROGRAM TO IDENTIFY CONNECTED DEVICES

from email_helper import email_helper
import os
import sys
import time
import serial
import serial.tools.list_ports
from log_object import log_object
from firebase_helper import firebase_helper
from email_helper import email_helper


class serial_scanner():

    # def __init__(self):
    #     return

    def print_list_of_serial_devices(self):
        try:
            # Search available ports:
            print('---------------------------------------------')
            print('SEARCH AVAILABLE PORTS')
            ports = serial.tools.list_ports.comports(include_links=False)
            print('---------------------------------------------')
            print('PORTSCAN RESULTS:')
            for port in ports:
                print('---------------------------------------------')
                print(port.device)
                print(port.manufacturer)
                print(port.hwid)
                print(port.serial_number)
                print(port.description)
                print(port.vid)
                print(port.pid)
                print('---------------------------------------------')
            print('---------------------------------------------')
        except Exception as error:
            error_message = 'CAUGHT AN ERROR WHILE SEARCHING PORTS !!!'
            print(error_message, error)


if __name__ == '__main__':
    serial_scanner = serial_scanner()
    serial_scanner.print_list_of_serial_devices()
