import serial
import requests
import time

gateway = 'AB'

ser = serial.Serial('/dev/ttyACM0', 9600, timeout=2)
print(ser.name)

old_line = ""

while True:
    line = ser.readline().decode('utf-8').strip()

    if line:
        if line[0] != '>':
            print("{} {}".format(time.strftime("<-- %d/%m/%Y %H:%M:%S"), line))

        else:
            print("{} {}".format(time.strftime("--- %d/%m/%Y %H:%M:%S"), line))

        if "]" in line and line[0] != "[" and line != old_line:

            line_split = line.split("|")
            if len(line_split) > 1:
                rx_rssi = line_split[1]
            else:
                rx_rssi = "0"

            data = line_split[0]
#            print("{}: {}".format(time.strftime("%d/%m/%Y %H:%M:%S"), line))
#            r = requests.post('http://www.ukhas.net/api/upload', json = {'origin': gateway, 'data' : data, 'rssi' : rx_rssi})

    file = open('UKHASnet-decoder/latest.txt')
    latest = file.readlines()
    file.close()
    try:
        if "]" in latest[0] and latest[0][0] != "[" and latest[0] != old_line:
#    if latest[0] != old_line:
            print("{} {}".format(time.strftime("--> %d/%m/%Y %H:%M:%S"), latest[0].rstrip()))
            old_line = latest[0]
            line_to_send = latest[0]
            tx_lines = line_to_send.splitlines()
            ser.write(tx_lines[0].encode('utf-8'))
    except:
        print('Error')
