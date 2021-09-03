import smtplib
import ssl
from email.mime.text import MIMEText
from email_secret_config import *

sender = 'dauertest.bst@gmail.com'
receivers = ['m.wettstein@signode.com']
body_of_email = '\
<h1> DAUERTEST HAT GESTOPPT <h1>\
'

msg = MIMEText(body_of_email, 'html')
msg['Subject'] = 'DAUERTEST HAT GESTOPPT'
msg['From'] = sender
msg['To'] = ','.join(receivers)

s = smtplib.SMTP_SSL(host='smtp.gmail.com', port=465)
s.login(user='dauertest.bst', password=email_password)
s.sendmail(sender, receivers, msg.as_string())
s.quit()
