import requests
import time
import sys
import subprocess

import redis
from redis import Redis

redis_db = redis.Redis(host='localhost', charset="utf-8", decode_responses=True)

id = 'AB'
input = input('Text: ')

letter = 'a'

while True:
    temp = subprocess.Popen(['/opt/vc/bin/vcgencmd', 'measure_temp'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    stdout, stderr = temp.communicate()
    temp1 = str(stdout.decode('utf-8')).split('=')
    print(temp1)
    tx_string = '6{}T{}:{}[{}]'.format(letter, temp1[1][:4], input, id)
    print(tx_string)
    redis_db.set((int(time.time()) - 1000), tx_string)
    print('Loaded into the broadcast queue')
    letter = chr(ord(letter) + 1)
    if letter == '{':
        letter = 'b'

    time.sleep(300)
