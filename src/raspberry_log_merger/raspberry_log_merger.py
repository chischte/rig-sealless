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

    return log

log_cycle_total=0
log_cycle_reset=0
log_force=0
log_current_tension=0
log_current_crimp=0
tool_is_tensioning=False
tool_is_crimping=False

def add_info_to_log(log):
    global log_cycle_total
    global log_cycle_reset
    global log_force
    global log_current_tension
    global log_current_crimp
    global tool_is_tensioning
    global tool_is_crimping
    
    if(log[0]=='LOG'):
        print(log)

        if log[1]=='CYCLE_TOTAL':
            log_cycle_total=log[2]
        
        if log[1]=='CYCLE_RESET':
            log_cycle_reset=log[2]
        
        if log[1]=='FORCE':
            log_force=log[2]
        
        if log[1]=='START_TENSION':
            tool_is_tensioning=True
            tool_is_crimping=False
            
        if log[1]=='START_CRIMP':
            tool_is_tensioning=False
            tool_is_crimping=True
        
        if log[1]=='CURRENT_MAX':
            if(tool_is_crimping):
                log_current_crimp=log[2]
            if(tool_is_tensioning):
                log_current_tension=log[2]

while True:

    line_controllino = ser_controllino.readline().decode('utf-8')
    #print(line_controllino)
    
    line_arduino = ser_arduino.readline().decode('utf-8')
    #print(line_arduino)

    if(verify_log(line_controllino)):
        log=split_log(line_controllino)
        add_info_to_log(log)

    if(verify_log(line_arduino)):
        log=split_log(line_arduino)
        add_info_to_log(log)

    print(log_cycle_total,log_cycle_reset,log_force,log_current_tension,log_current_crimp)


    time.sleep(0.5)


ser.close()
