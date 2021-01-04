import serial
import requests
import time
import sys, getopt

import redis
from redis import Redis

redis_db = redis.Redis(host='localhost', charset="utf-8", decode_responses=True)

def main(argv):

    ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=2)
    print(ser.name)
    gateway = 'AA'

    print('ID = {}'.format(gateway))

    if gateway == 'CHANGEME':
        print('Please change default gateway ID name with the -i command')
        sys.exit()

    old_line = ""
    old_data = ""

    while True:
        try:
            line = ser.readline().decode('utf-8').strip()
        except KeyboardInterrupt:
            sys.exit()
        except:
            line = None

        if line:
            if "]" in line and line[0] != "[" and line != old_line and "|" in line:
                print("{} {} ***--> ".format(time.strftime("<-- %d/%m/%Y %H:%M:%S"), line), end = '')

                line_split = line.split("|")
                if len(line_split) > 1:
                    rx_rssi = line_split[1]
                else:
                    rx_rssi = "0"

                data = line_split[0]
#                print("{}: {}".format(time.strftime("%d/%m/%Y %H:%M:%S"), line))
                try:
                    r = requests.post('http://www.ukhas.net/api/upload', json = {'origin': gateway, 'data' : data, 'rssi' : rx_rssi})
                    print("Uploaded")
                except:
                    print('Error no internet connection')
        else:
            pass

if __name__ == "__main__":
    main(sys.argv[1:])
