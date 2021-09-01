# GET DATA FROM CONTROLLINO PLC
# GET DATA FROM ARDUINO_CURRENT_LOGGER
# MERGE DATA TO LOG
# PUSH LOG TO FIREBASE

# Show list of serial devices:
import os
import sys
import time
import serial
import serial.tools.list_ports

print('Search...')
ports = serial.tools.list_ports.comports(include_links=False)
for port in ports :
    print('Find port '+ port.device)

ser = serial.Serial(
    port='COM3',\
    baudrate=115200,\
    parity=serial.PARITY_NONE,\
    stopbits=serial.STOPBITS_ONE,\
    bytesize=serial.EIGHTBITS,\
        timeout=0)

print("connected to: " + ser.portstr)
count=1

while True:
    line = ser.readline()
    print(line)
    time.sleep(0.5)

ser.close()

