import requests
import time
import sys

import redis
from redis import Redis

redis_db = redis.Redis(host='localhost', charset="utf-8", decode_responses=True)

id = input('ID: ')
text = input('Text: ')

tx_string = '6a:{}[{}]'.format(text.replace(' ', '_'), id)

print(tx_string)

if len(tx_string) < 64:
    redis_db.set((int(time.time()) - 1000), tx_string)
    print('Loaded into the broadcast queue')
else:
    print('Error string is too long')
