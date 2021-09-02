#!/usr/bin/env python3

import pyrebase
from firebase_secret_config import *


class firebase_logger():

    def __init__(self):
        self.config = {
            "apiKey": apiKey,
            "authDomain": authDomain,
            "databaseURL": databaseURL,
            "storageBucket": storageBucket,
            "serviceAccount": "firebase_secret_service_account.json"
        }
        self.firebase = pyrebase.initialize_app(self.config)
        self.db = self.firebase.database()
        self.data = {"name": "Mordddtimer 'Morddty' Smidddddth"}
        self.db.push(self.data)


if __name__ == '__main__':

    firebase_logger = firebase_logger()
