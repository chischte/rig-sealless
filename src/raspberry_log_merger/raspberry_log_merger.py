
# Show list of serial devices:
import os
import sys
import time
import serial
import serial.tools.list_ports
from log import log

# Create log objects
log_object = log()

# Search available ports:

print('Search...')
ports = serial.tools.list_ports.comports(include_links=False)
for port in ports:
    print('Find port ' + port.device)

# Define serial ports:

controllino = serial.Serial(
    port='COM3',
    baudrate=115200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=0)

arduino = serial.Serial(
    port='COM5',
    baudrate=115200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=0)

print("connected to: " + controllino.portstr)
print("connected to: " + arduino.portstr)


def process_serial_read(readline):
    if(readline.decode('utf-8')==""):
        return
    readline=readline.split(';')
    if(readline[0] == 'LOG'):
        add_info_to_log(readline)

def upload_log(log_object):
    return

def set_log_completed():
    log_object.print_log()
    upload_log(log_object)
    log_object.reset_log()

def add_info_to_log(self, readline):
          
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

while True:

    # Read and process serial inputs:
    process_serial_read(controllino.readline())
    process_serial_read(arduino.readline())

    time.sleep(0.5)


ser.close()
