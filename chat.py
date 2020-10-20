import requests
import time
import sys

import redis
from redis import Redis

redis_db = redis.Redis(host='localhost', charset="utf-8", decode_responses=True)

id = input('ID: ')
text = input('Text: ')

tx_string = '4a:{}[{}]'.format(text.replace(' ', '_'), id)

print(tx_string)

redis_db.set((int(time.time()) - 1000), tx_string)

print('Pending')
