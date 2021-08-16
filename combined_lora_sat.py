import serial
import requests
import time
import sys, getopt
import re
import random
import settings

import redis
from redis import Redis

from norby import *
from skyfield.api import load, wgs84

redis_db = redis.Redis(host='localhost', charset="utf-8", decode_responses=True)
location = wgs84.latlon(settings.lat, settings.lon)

def main(argv):
    global net_connect
    global broadcast
    global repeater
    global geolocate
    global duty_cycle

    try:
        opts, args = getopt.getopt(argv, "i:p:m:d:z:f:k:s:bcrgyn")
    except:
        print('Error')

    ser = serial.Serial('/dev/ttyACM0', 9600, timeout=2)
    print(ser.name)
    gateway = 'CHANGEME'
    duty_cycle = 10
    num_packets_per_min = 60 / int(duty_cycle)
    net_connect = 0
    broadcast = 0
    repeater = 0
    geolocate = 0
    print_all = 0
    norbi_visible = 0
    track_norbi = 0

    print(opts)
    for opt, arg in opts:
        if opt in ['-i']:
            gateway = arg

        elif opt in ['-n']:
            print('Setting up Norbi Sat')
            stations_url = 'http://tle.pe0sat.nl/kepler/mykepler.txt'
            satellites = load.tle_file(stations_url)
            by_name = {sat.name: sat for sat in satellites}
            satellite = by_name['NORBI']
            print(satellite)
            ts = load.timescale()
            track_norbi = 1

        elif opt in ['-p']:
            power = arg
            print('Set up radio module')
            set_power = '$P{}\n'.format(power)
            time.sleep(2)
            ser.write(set_power.encode('utf-8'))
            line = ser.readline().decode('utf-8').strip()
            print(line)

        elif opt in ['-f']:
            freq = arg
            print('Set up radio module freq')
            set_freq = '$F{}\n'.format(freq)
            time.sleep(2)
            ser.write(set_freq.encode('utf-8'))
            line = ser.readline().decode('utf-8').strip()
            print(line)

        elif opt in ['-k']:
            low_data = arg
            print('Set up low data rate')
            set_low_data = '$K{}\n'.format(low_data)
            time.sleep(2)
            ser.write(set_low_data.encode('utf-8'))
            line = ser.readline().decode('utf-8').strip()
            print(line)

        elif opt in ['-s']:
            str_bin = arg
            print('Set up Output')
            set_str_bin = '$S{}\n'.format(str_bin)
            time.sleep(2)
            ser.write(set_str_bin.encode('utf-8'))
            line = ser.readline().decode('utf-8').strip()
            print(line)

        elif opt in ['-z']:
            custom = arg
            print('Set up custom config  radio module')
            set_custom = '$C{}\n'.format(custom)
            time.sleep(2)
            ser.write(set_custom.encode('utf-8'))
            line = ser.readline().decode('utf-8').strip()
            print(line)

        elif opt in ['-m']:
            mode = arg
            set_mode = '$M{}\n'.format(mode)
            ser.write('\n'.encode('utf-8'))
            time.sleep(2)
            ser.write(set_mode.encode('utf-8'))
            line = ser.readline().decode('utf-8').strip()
            print(line)

        elif opt in ['-d']:
            duty_cycle = arg
            num_packets_per_min = 60 / int(duty_cycle)

        elif opt in ['-c']:
            net_connect = 1

        elif opt in ['-r']:
            repeater = 1

        elif opt in ['-y']:
            print('print all')
            print_all = 1

        elif opt in ['-b']:
            broadcast = 1

        elif opt in ['-g']:
            geolocate = 1

    print('ID = {}'.format(gateway))

    if gateway == 'CHANGEME':
        print('Please change default gateway ID name with the -i command')
        sys.exit()

    old_line = ""
    old_data = ""

    duty_cycle_store = []

    while True:
        if track_norbi == 1:
            difference = satellite - location

            t = ts.now()

            topocentric = difference.at(t)
#            print(topocentric.position.km)

            alt, az, distance = topocentric.altaz()

            if alt.degrees > 0:
                if norbi_visible == 0:
                    print('Norbi is above the horizon')
                    print('Start listening')
                    norbi_visible = 1

                    print('Set up radio module freq')
                    set_freq = '$F{}\n'.format(436.7)
                    time.sleep(2)
                    ser.write(set_freq.encode('utf-8'))
                    line = ser.readline().decode('utf-8').strip()
                    print(line)

                    set_mode = '$M{}\n'.format(6)
                    time.sleep(2)
                    ser.write(set_mode.encode('utf-8'))
                    line = ser.readline().decode('utf-8').strip()
                    print(line)

            else:
                if norbi_visible == 1:
                    print('Stop listening')

                    print('Set up radio module freq')
                    set_freq = '$F{}\n'.format(434.4)
                    time.sleep(2)
                    ser.write(set_freq.encode('utf-8'))
                    line = ser.readline().decode('utf-8').strip()
                    print(line)

                    set_mode = '$M{}\n'.format(5)
                    time.sleep(2)
                    ser.write(set_mode.encode('utf-8'))
                    line = ser.readline().decode('utf-8').strip()
                    print(line)

                norbi_visible = 0


#            print('Altitude:', alt)
#            print('Azimuth:', az)
#            print('Distance: {:.1f} km'.format(distance.km))

        try:
            line = ser.readline().decode('utf-8').strip()
#            ser.reset_input_buffer()
        except KeyboardInterrupt:
            sys.exit()
        except:
            line = None

        if line:
            if print_all == 1:
                print(line)

            if line[0] != '>':

                if "]" in line and line[0] != "[" and line != old_line:

                    line_split = line.split("|")

                    if len(line_split) > 1:
                        rx_rssi = line_split[1]
                    else:
                        rx_rssi = "0"

                    data = line_split[0].split(']')[0] + ']'
                    print("{} {} ***--> ".format(time.strftime("<-- %d/%m/%Y %H:%M:%S"), data), end = '')

                    if ':' in data:
                        redis_db.set('c{}'.format((int(time.time()))), data)
                    if net_connect == 1:
                        try:
                            r = requests.post('http://www.ukhas.net/api/upload', json = {'origin': gateway, 'data' : data, 'rssi' : rx_rssi})
                            print("Uploaded")
                        except:
                            print('Error no internet connection')
                    else:
                        print('Not Uploaded')


#                    Geolocate
                    if geolocate == 1:
                        packet_split = data.split('[')
                        if 'L' in  packet_split[0] and gateway not in data and '0.0000' not in data:
                            packet_source = packet_split[1].split(',')[0]
                            if packet_source[-1] == ']':
                                #print('Stripping end')
                                packet_source = packet_source.rstrip(']')
                            packet_parts = []
                            packet_parts = re.findall('[A-Z][^A-Z]*', data)
                            for parts in packet_parts:
                                if parts[0] == 'L':
                                    location_split = parts[1:].split(',')
                                    location_lat = location_split[0]
                                    location_lon = location_split[1]

                                    #print('Saving to Geolocate DB {}\n {}'.format(packet_source, parts[1:]))
                                    #print('{} {} {} {}'.format(packet_source, location_lon, location_lat, int(time.time())))
                                    redis_db.geoadd('geo-{}'.format(packet_source), location_lon, location_lat, int(time.time()))
                                    redis_db.geoadd('geo-current', location_lon, location_lat, packet_source )

#                    Repeater Code
                    if repeater == 1:
                        if gateway in data:
                            print('Not repeating again')
                        else:
    #                       reduce hops
                            hops = int(data[0])
                            if hops > 0:
                                add_ending = '{}{},{}]'.format(hops -1, data[1:-1], gateway)
                                time.sleep(random.random() * 3.0)
                                print("{} {}".format(time.strftime("<R> %d/%m/%Y %H:%M:%S"), add_ending.rstrip()))
                                if len(duty_cycle_store) < num_packets_per_min or (time.time() - 60.0) > duty_cycle_store[0]:
                                    # check if duty_cycle_store has more than or equal to num_packets_per_min, if it does then remove the first entry
                                    if len(duty_cycle_store) >= num_packets_per_min:
                                        duty_cycle_store.pop(0)
                                    # add time to store
                                    duty_cycle_store.append(time.time())

                                    ser.write(add_ending.encode('utf-8'))
                                else:
                                    print('Duty Cycle Hit')


                elif line[2] == ' ':
                    print('Found Hex String from Norby Sat')
                    removed_rssi = line.split('|')

                    final_data = removed_rssi[0].replace(" ", "")
                    print(final_data)

                    bytearray_ = bytearray.fromhex(final_data)

                    try:
                        target = Norby.from_bytes(bytearray_)
                        print(target.payload.sop_altitude_glonass)
                    except:
                       print('Not Norby')

                else:
                    print('')

            else:
                if "Sent" in line:
                    pass
                else:
                    print("{} {}".format(time.strftime("--- %d/%m/%Y %H:%M:%S"), line))

        jobs = redis_db.keys('*')
        if len(jobs) > 0:
            jobs.sort()
            if 'c' not in jobs[0] and 'geo' not in jobs[0]:
                latest = redis_db.get(jobs[0])
                if len(duty_cycle_store) < num_packets_per_min or (time.time() - 60.0) > duty_cycle_store[0]:

                    redis_db.delete(jobs[0])
                    if "]" in latest and latest[0] != "[" and latest != old_line:
                        old_line = latest
                        tx_data = latest[1:].split("[")
                        if tx_data[0] != old_data:
                            if broadcast == 0:
                                add_ending = '{},{}]'.format(latest[:-1], gateway)
                                print("{} {}".format(time.strftime("--> %d/%m/%Y %H:%M:%S"), add_ending.rstrip()))
                                ser.write(add_ending.encode('utf-8'))
                                # check if duty_cycle_store has more than or equal to num_packets_per_min, if it does then remove the first entry
                                if len(duty_cycle_store) >= num_packets_per_min:
                                    duty_cycle_store.pop(0)
                                # add time to store
                                duty_cycle_store.append(time.time())
                            old_data = tx_data[0]
                else:
                    #print('Duty Cycle Hit {}'.format(duty_cycle_store))
                    print('Duty Cycle Hit')

if __name__ == "__main__":
    main(sys.argv[1:])
