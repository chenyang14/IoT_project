import serial
from datetime import datetime
from pymongo import MongoClient
import smtplib
from email.mime.text import MIMEText

ser = serial.Serial('COM3', 9600)
try:
    uri = "mongodb://"  #mongoDB server 
    client = MongoClient(uri)
    print (client)
except:
    print ("connect to mongoDB failed")
    
db = client[''] #database to go
collect = db['temperature']
last_send = datetime.now()
first = 1

while 1:
    line = ser.readline()
    temperature = line.decode("utf-8").rstrip("\r\n")
    if temperature[0]=="S": continue

    data = {
            "temp": temperature,
            "time": datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
           }            
    data_id = collect.insert_one(data).inserted_id
    print (data_id , data["temp"], data["time"])

    if int(temperature[0]) >= 3 and ((datetime.now()-last_send).total_seconds()>600 or first==1):
        file = open("auth.txt", "r") #auth.txt with email log in password
        password = file.readline()
        last_send = datetime.now()
        first = 0
        content = "Temperature is " + temperature + ". TOO HIGH!!!"
        try:
            msg = MIMEText(content)
            msg['Subject'] = 'warning'
            msg['From'] = 'send@gmail.com' #sender
            msg['To'] = 'receive@gmail.com' #receiver
            s = smtplib.SMTP('smtp.gmail.com', 587)
            s.ehlo()
            s.starttls()
            s.ehlo()
            s.login("send", password)
            s.sendmail('send@gmail.com', 'receive@gmail.com', msg.as_string())
            s.quit()
        except Exception as e:
            print (str(e))

ser.close()