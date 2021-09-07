#!/usr/bin/env python3

import csv
import getpass
from logging import log
from firebase_helper import firebase_helper
from datetime import datetime
from graph_creator import *
import pandas.plotting._matplotlib #required for pyinstaller
import time

# ------------------------------------------------------------------
# TO CREATE EXECUTABLE, RUN: pyinstaller --onefile csv_creator.py
# EXE CAN BE FOUND IN DIST FOLDER
# ------------------------------------------------------------------


class csv_creator():

    def __init__(self):
        return


if __name__ == '__main__':
    print()
    print('INITIALIZE DB CONNECTION ...')
    firebase_helper = firebase_helper()
    time.sleep(1.5)
    print('--------------------------------------------------------------------------------')
    print('GET LOGS FROM DB ...')
    time.sleep(1.5)
    print('--------------------------------------------------------------------------------')
    logs = firebase_helper.get_logs()

    timestamp=datetime.now().strftime("%d/%m/%Y %H:%M:%S")

    # Create list
    log_list = [["","","","","","",""]]
 
    # Add Date info
    log_list.append(["LOGS SEALLESS TEST RIG"])
    log_list.append(["File creation date:", datetime.now().strftime("%d/%m/%Y")])
    log_list.append(["File creation time:", datetime.now().strftime("%H:%M:%S")])
    log_list.append([])
    
    log_list.append(["", "total testrig cycles", "cycles since last counter reset", "maximum force", "peak battery current", "peak battery current"])
    
    # Add column header
    log_list.append(["TIMESTAMP", "CYCLES TOTAL", "CYCLES RESET", "TENSION FORCE", "TENSION CURRENT", "CRIMP CURRENT"])
 
    # Add unit header
    log_list.append(["", "", "", "[N]", "[A]", "[A]"])

    # Get log values
    for timestamp in logs:
        values = logs[timestamp]
        values = values.split(";")
        csv_timestamp = timestamp
        csv_cycle_total = values[0]
        csv_cycle_reset = values[1]
        csv_tension_force = values[2]
        csv_tension_current = values[3]
        csv_crimp_current = values[4]
        log_list.append([csv_timestamp, csv_cycle_total, csv_cycle_reset, csv_tension_force, csv_tension_current, csv_crimp_current])
    for log in log_list:
        print(log)

    # Create CSV from list
    print('--------------------------------------------------------------------------------')
    print('CREATE CSV ...')
    with open('logs.csv', 'w', newline='') as file:
        csv.writer(file, delimiter=';').writerows(log_list)
    time.sleep(2)
    print('CSV CREATED')
    print('--------------------------------------------------------------------------------')
    print('TRY TO OPEN CSV IN EXCEL')
    time.sleep(2)
    try:
        from subprocess import Popen
        p = Popen('logs.csv', shell=True)
    except Exception as error:
        print(error)
    print('--------------------------------------------------------------------------------')
    print('TRY TO GENERATE GRAPH')
    time.sleep(4)
    try:
        graph_creator=graph_creator()
        graph_creator.plot_graph()
    except Exception as error:
        print(error)
    print('--------------------------------------------------------------------------------')
    print('')
    input('PRESS ENTER TO EXIT: ...')
