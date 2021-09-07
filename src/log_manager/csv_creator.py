#!/usr/bin/env python3

import csv
import getpass
from logging import log
from firebase_helper import firebase_helper
from datetime import datetime
from graph_creator import *

import time

class csv_creator():

    def __init__(self):
        self.firebase_helper=firebase_helper()
        return

    def create_csv(self):
        print()
        print('INITIALIZE DB CONNECTION ...')
        time.sleep(1.5)
        print('--------------------------------------------------------------------------------')
        print('GET LOGS FROM DB ...')
        time.sleep(1.5)
        print('--------------------------------------------------------------------------------')
        logs = self.firebase_helper.get_logs()

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
        
if __name__ == '__main__':
    csv_creator=csv_creator()
    csv_creator.create_csv()
    
