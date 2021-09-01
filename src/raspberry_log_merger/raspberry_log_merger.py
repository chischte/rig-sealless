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
for port in ports:
    print('Find port ' + port.device)

ser_controllino = serial.Serial(
    port='COM3',
    baudrate=115200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=0)

ser_arduino = serial.Serial(
    port='COM5',
    baudrate=115200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=0)


print("connected to: " + ser_controllino.portstr)
print("connected to: " + ser_arduino.portstr)
count = 1

def verify_log(log):
    if(log==""): #  remove empty reads
        return False
    return True

def split_log(log):
    
    log=log.split(';')
    print(log)

    return log


while True:
    line_controllino = ser_controllino.readline().decode('utf-8')
    #print(line_controllino)
    
    line_arduino = ser_arduino.readline().decode('utf-8')
    #print(line_arduino)

    if(verify_log(line_controllino)):
        split_log(line_controllino)

    if(verify_log(line_arduino)):
        split_log(line_arduino)


    time.sleep(0.5)


ser.close()
