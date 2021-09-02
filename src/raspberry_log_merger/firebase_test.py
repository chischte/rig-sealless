import pyrebase
from firebase_secret_config import *

config = {
    "apiKey": apiKey,
    "authDomain": authDomain,
    "databaseURL": databaseURL,
    "storageBucket": storageBucket,
    "serviceAccount": "firebase_secret_service_account.json"
}

firebase = pyrebase.initialize_app(config)
db = firebase.database()
data = {"name": "Mortimer 'Morty' Smidddth"}
db.push(data)
