
# Show list of serial devices:
import os
import sys
import time
import serial
import serial.tools.list_ports
from log import log

# Create log objects
log_object = log()

arduino = 0
controllino = 0


def get_list_of_serial_devices():
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

def connect_to_serial_devices():
    global arduino
    global controllino

    ports = serial.tools.list_ports.comports(include_links=False)

    for port in ports:
        if(port.vid == 9025) and (port.pid == 66):
            arduino_port = port.device
            print(f'ARDUINO PORT: {arduino_port}')
        if(port.vid == 6790) and (port.pid == 29987):
            controllino_port = port.device
            print(f'CONTROLLINO PORT: {controllino_port}')

    controllino = serial.Serial(
        port=controllino_port,
        baudrate=115200,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=0)
    print("connected to: " + controllino.portstr)

    arduino = serial.Serial(
        port=arduino_port,
        baudrate=115200,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=0)
    print("connected to: " + arduino.portstr)

def process_serial_read(readline):
    readline = readline.decode('utf-8')
    if(readline == ""):
        return
    readline = readline.split(';')
    if(readline[0] == 'LOG'):
        add_info_to_log(readline)

def upload_log(log_object):
    return

def set_log_completed():
    log_object.print_log()
    upload_log(log_object)
    log_object.reset_log()

def add_info_to_log(readline):

    if readline[1] == 'CYCLE_TOTAL':
        set_log_completed()
        log_object.cycle_total = readline[2]

    if readline[1] == 'CYCLE_RESET':
        log_object.cycle_reset = readline[2]

    if readline[1] == 'FORCE':
        log_object.force = readline[2]

    if readline[1] == 'START_TENSION':
        log_object.set_tool_is_tensioning()

    if readline[1] == 'START_CRIMP':
        log_object.set_tool_is_crimping()

    if readline[1] == 'CURRENT_MAX':
        if(log_object.tool_is_tensioning):
            log_object.current_tension = readline[2]
        if(log_object.tool_is_crimping):
            log_object.crimp_current = readline[2]


get_list_of_serial_devices()
connect_to_serial_devices()

while True:

    # Read and process serial inputs:
    process_serial_read(controllino.readline())
    process_serial_read(arduino.readline())

    time.sleep(0.5)


ser.close()
