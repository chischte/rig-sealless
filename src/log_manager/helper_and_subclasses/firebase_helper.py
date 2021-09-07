#!/usr/bin/env python3

import pyrebase
from helper_and_subclasses.firebase_secret_config import *

class firebase_helper():

    def __init__(self):
        self.config = {
            "apiKey": apiKey,
            "authDomain": authDomain,
            "databaseURL": databaseURL,
            "storageBucket": storageBucket,
            "serviceAccount": secret_service_account_json
        }
        try:
            self.firebase = pyrebase.initialize_app(self.config)
        except Exception as error:
            print(error)
        
        self.db = self.firebase.database()
    
    def push(self,data):
        # self.db.push(data) # auto generated folder for every log
        # self.db.set(data) # every log overwrites previous log
        try:
            self.db.update(data) # every log generates a new line
        except Exception as error:
            print(error)

    def get_logs(self):
        try:
            db_data=self.db.child("logs").get().val()
            return db_data
        except Exception as error:
            print(error)

if __name__ == '__main__':

    firebase_helper = firebase_helper()
    firebase_helper.get_logs();
