#!/usr/bin/env python3

from csv_creator import *
from graph_creator import *


# ------------------------------------------------------------------
# TO CREATE EXECUTABLE, RUN: pyinstaller --onefile log_presenter.py
# EXE CAN BE FOUND IN DIST FOLDER
# ------------------------------------------------------------------

class log_presenter():

    def __init__(self):
        self.csv_creator = csv_creator()
        self.graph_creator = graph_creator()
        return

    def present_log(self):
        self.csv_creator.create_csv()
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
            self.graph_creator.plot_graph()
        except Exception as error:
            print(error)
        print('--------------------------------------------------------------------------------')
        print('')
        input('PRESS ENTER TO EXIT: ...')


if __name__ == '__main__':
    log_presenter = log_presenter()
    log_presenter.present_log()
