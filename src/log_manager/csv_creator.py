#!/usr/bin/env python3

from logging import setLogRecordFactory
from firebase_helper import firebase_helper


class csv_creator():

    def __init__(self):
        return


if __name__ == '__main__':
    firebase_helper = firebase_helper()
    logs = firebase_helper.get_logs()
    print(logs)
    for key in logs:
        value = logs[key]
        # print(key + ' : ' + value)
        values = value.split(";")
        # print(values)
        for value in values:
            print(value)
