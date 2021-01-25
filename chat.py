import requests
import time
import sys
import curses

import redis
from redis import Redis

redis_db = redis.Redis(host='localhost', charset="utf-8", decode_responses=True)


def shutdown_chat():
    curses.nocbreak()
    stdscr.keypad(False)
    curses.echo()
    curses.endwin()

def send_text(tx_string):

#    print(tx_string)
    if len(tx_string) < 64:
        redis_db.set((int(time.time()) - 1000), tx_string)
#        print('Loaded into the broadcast queue')
    else:
        print('Error string is too long')

id = input('ID: ').upper()

stdscr = curses.initscr()
curses.noecho()
stdscr.nodelay(True)

max_y, max_x = stdscr.getmaxyx()
stdscr.border(0)
stdscr.addstr(2, 5, 'WV Chat System!', curses.A_BOLD)
stdscr.hline(3, 1, ord('*'), max_x - 2)
stdscr.addstr(4, 5, 'Press Q (shift q) to close this screen', curses.A_NORMAL)
stdscr.addstr(8, 5, 'Rolling Log: ', curses.A_NORMAL)
stdscr.addstr(5, 5, 'Send: ', curses.A_NORMAL)

input_array = ''
letter = 'a'
tx_x = 9
last_tx = ''

while True:
    try:
        ch = stdscr.getch()
        if ch == ord('Q'):
            shutdown_chat()
            sys.exit()

        elif ch == curses.KEY_ENTER or ch == 10:
            tx_string = '6{}:{}[{}]'.format(letter, input_array.replace(' ', '_'), id)
            last_tx = tx_string[1:-1]
            send_text(tx_string)
            stdscr.move(tx_x, 5)
            stdscr.clrtoeol()
            stdscr.addstr(tx_x, 5, '{} {}'.format(time.strftime("%d/%m/%Y %H:%M:%S"), tx_string), curses.A_NORMAL)
            input_array = ''
            tx_x = tx_x + 1
            if tx_x > 16:
                tx_x = 9

            stdscr.move(tx_x, 5)
            stdscr.clrtoeol()
#Return to input
            stdscr.move(5, 11)
            stdscr.clrtoeol()
            stdscr.refresh()

            letter = chr(ord(letter) + 1)
            if letter == '{':
                letter = 'b'


        elif ch == 127:
#	delete character
            current_y, current_x = stdscr.getyx()
            if (current_x - 1) >= 11:
                stdscr.delch(5, current_x - 1)
                stdscr.border(0)
                stdscr.refresh()
                input_array = input_array[:-1]

        else:
            if ch > 96 and ch < 123:
                stdscr.addch(ch)
                input_array = '{}{}'.format(input_array, chr(ch))
            if ch >= 48 and ch <= 57:
                stdscr.addch(ch)
                input_array = '{}{}'.format(input_array, chr(ch))
            if ch == 32:
                stdscr.addch(ch)
                input_array = '{}{}'.format(input_array, chr(ch))

        jobs = redis_db.keys('c*')
        if len(jobs) > 0:
            jobs.sort()
            latest = redis_db.get(jobs[0])
            redis_db.delete(jobs[0])
            if last_tx not in latest:
                stdscr.move(tx_x, 5)
                stdscr.clrtoeol()
                stdscr.addstr(tx_x, 5, '{} {}'.format(time.strftime("%d/%m/%Y %H:%M:%S"), latest), curses.A_NORMAL)
                tx_x = tx_x + 1
                if tx_x > 16:
                    tx_x = 9
                stdscr.move(tx_x, 5)
                stdscr.clrtoeol()
            else:
                if last_tx != '':
                    ack_id = latest.split(',')[-1][:-1]
                    stdscr.move(tx_x - 1, 5)
                    stdscr.addstr(tx_x - 1, max_x - 20, 'ACK: {}'.format(ack_id), curses.A_NORMAL)

#Return to input
            stdscr.move(5, 11)
            stdscr.clrtoeol()
            stdscr.refresh()

#        rx_log = redis_db.get('rx_log')

    except KeyboardInterrupt:
        print('\nExiting')
        shutdown_chat()
        sys.exit()
