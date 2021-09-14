#!/usr/bin/env python3

import time
import os
from helper_and_subclasses.csv_creator import csv_creator
from helper_and_subclasses.graph_creator import graph_creator


# ------------------------------------------------------------------
# TO CREATE EXECUTABLE, RUN: pyinstaller --onefile .\show_sealless_log.py
# EXE CAN BE FOUND IN DIST FOLDER
# ------------------------------------------------------------------

class show_sealless_log():

    def __init__(self):

        self.graph_creator = graph_creator()
        self.csv_creator = csv_creator()
        return

    def get_user_path(self):
        return os.environ['USERPROFILE']
    
    def get_filepath(self):
        return self.get_user_path()+"/AppData/Roaming/SeallessLog/Logs.csv"
    
    def present_log(self):
        self.csv_creator.create_csv()
        print('--------------------------------------------------------------------------------')
        print('TRY TO OPEN CSV IN EXCEL')
        time.sleep(2)
        try:
            from subprocess import Popen
            p = Popen(self.get_filepath(), shell=True)
        except Exception as error:
            print(error)
        print('--------------------------------------------------------------------------------')
        print('TRY TO GENERATE GRAPH')
        time.sleep(4)
        try:
            self.graph_creator.plot_graph()
        except Exception as error:
            print(error)
        print('--------------------------------------------------------------------------------')
        print('')
        input('PRESS ENTER TO EXIT: ...')


if __name__ == '__main__':
    show_sealless_log = show_sealless_log()
    show_sealless_log.present_log()
