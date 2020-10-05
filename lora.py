import serial
import requests
import time
import sys, getopt


def main(argv):
    global net_connect
    global broadcast

    try:
        opts, args = getopt.getopt(argv, "i:p:bc")
    except:
        print('Error')

    ser = serial.Serial('/dev/ttyACM0', 9600, timeout=2)
    print(ser.name)
    gateway = 'AB1'
    net_connect = 0
    broadcast = 0

    for opt, arg in opts:
        if opt in ['-i']:
            gateway = arg
        elif opt in ['-p']:
            power = arg
            print('Set up radio module')
            set_power = '$P{}'.format(power)
            ser.write(set_power.encode('utf-8'))

        elif opt in ['-c']:
            net_connect = 1

        elif opt in ['-b']:
            broadcast = 1

    print('ID = {}'.format(gateway))

    old_line = ""
    old_data = ""

    while True:
        line = ser.readline().decode('utf-8').strip()

        if line:
            if line[0] != '>':
                print("{} {} ***--> ".format(time.strftime("<-- %d/%m/%Y %H:%M:%S"), line), end = '')


                if "]" in line and line[0] != "[" and line != old_line:

                    line_split = line.split("|")
                    if len(line_split) > 1:
                        rx_rssi = line_split[1]
                    else:
                        rx_rssi = "0"

                    data = line_split[0]
#                print("{}: {}".format(time.strftime("%d/%m/%Y %H:%M:%S"), line))
                    if net_connect == 1:
                        r = requests.post('http://www.ukhas.net/api/upload', json = {'origin': gateway, 'data' : data, 'rssi' : rx_rssi})
                        print("Uploaded")
                    else:
                        print('Not Uploaded')
                else:
                    print('')

            else:
                if "Sent" in line:
                    pass
                else:
                    print("{} {}".format(time.strftime("--- %d/%m/%Y %H:%M:%S"), line))

        file = open('UKHASnet-decoder/latest.txt')
        latest = file.readlines()
        file.close()
        try:
            if "]" in latest[0] and latest[0][0] != "[" and latest[0] != old_line:
#    if latest[0] != old_line:
                old_line = latest[0]
                line_to_send = latest[0]
                tx_lines = line_to_send.splitlines()
                tx_data = tx_lines[0][1:].split("[")
                if tx_data[0] != old_data:
                    if broadcast == 0:
                        print("{} {}".format(time.strftime("--> %d/%m/%Y %H:%M:%S"), latest[0].rstrip()))
                        ser.write(tx_lines[0].encode('utf-8'))
                    old_data = tx_data[0]
#                else:
#                    print('Already sent this packet')
        except:
            print('Error')

if __name__ == "__main__":
    main(sys.argv[1:])
