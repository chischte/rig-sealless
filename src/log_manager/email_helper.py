from email import message
import smtplib
import ssl
from email.mime.text import MIMEText
from email_secret_config import *
from datetime import datetime

class email_helper():

    def __init__(self):
        self.sender = 'dauertest.bst@gmail.com'
        #'m.neeser@signode.com'
        #'b.hubschmid@signode.com'
        #'a.keller@signode.com'
        
        self.receivers = ['m.wettstein@signode.com']

    def send_mail(self, subject, message_body):
        msg = MIMEText(message_body, 'html')
        msg['Subject'] = subject
        msg['From'] = self.sender
        msg['To'] = ','.join(self.receivers)
        try:
            s = smtplib.SMTP_SSL(host='smtp.gmail.com', port=465)
            s.login(user='dauertest.bst', password=email_password)
            s.sendmail(self.sender, self.receivers, msg.as_string())
            s.quit()
        except Exception as error:
            error_message = 'SEND EMAIL DID NOT WORK !!!'
            print(error_message, error)

    def send_message_machine_stopped(self):
        datestamp = datetime.now().strftime("%d/%m/%Y")
        timestamp = datetime.now().strftime("%H:%M")
        subject = f'BST DAUERTEST HAT GESTOPPT'
        message_body = f'\
        <h2> BST DAUERTEST HAT GESTOPPT AM {datestamp} UM {timestamp} UHR\
        <h3>Obwohl der Dauertest im Automatikbetrieb war, wurde während 60 Sekunden \
        kein Tool-Zyklus mehr erfolgreich durchgeführt.\
        <p>Mögliche Ursachen:\
            <ul>\
            <li>Band leer\
            <li>Störung am Gerät\
            <li>Störung Dauertestrig\
            <li>Störung Steuerung\
            </ul>\
        <h3>Der Log mit den aktuellen Messdaten kann hier eingesehen werden: \
        <p>"T:\Dauer Test\sealles_log.lnk"\
            '
        self.send_mail(subject, message_body)

    def send_message_button_pushed(self):
        datestamp = datetime.now().strftime("%d/%m/%Y")
        timestamp = datetime.now().strftime("%H:%M")
        subject = f'JEMAND HAT DEN KNOPF GEDRÜCKT !'
        message_body = f'\
        <h2> JEMAND HAT DEN KNOPF GEDRÜCKT AM {datestamp} UM {timestamp} UHR\
        <p>Mögliche Ursachen:\
            <ul>\
            <li>Jemand hat den Knopf gedrückt\
            '
        self.send_mail(subject, message_body)

if __name__ == '__main__':
    
    email_helper = email_helper()
    email_helper.send_message_machine_stopped()
    # email_helper.send_message_button_pushed()
