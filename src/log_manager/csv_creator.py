#!/usr/bin/env python3

import csv
from logging import log
from firebase_helper import firebase_helper

#------------------------------------------------------------------
# TO CREATE EXECUTABLE, RUN: pyinstaller --onefile csv_creator.py
#------------------------------------------------------------------

class csv_creator():

    def __init__(self):
        return

if __name__ == '__main__':
    print()
    print('INITIALIZE DB CONNECTION ...')
    firebase_helper = firebase_helper()
    print('GET LOGS FROM DB ...')
    logs = firebase_helper.get_logs()

    # Create list
    log_list = [["TIMESTAMP", "CYCLE TOTAL", "CYCLE RESET", "TENSION FORCE [N]", "TENSION CURRENT [A]", "CRIMP CURRENT [A]"]]

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
    print('CREATE CSV ...')
    with open('logs.csv', 'w', newline='') as file:
        csv.writer(file, delimiter=';').writerows(log_list)
    print('CSV CREATED')
    print('')
    print('TRY TO OPEN CSV IN EXCEL')
    from subprocess import Popen
    p = Popen('logs.csv', shell=True)
    input('Press Enter to Exit...')
    
