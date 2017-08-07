import sys, time, os
import subprocess
import sqlite3
import binascii
import json
import re
import threading
from multiprocessing import Process, Queue, Value
from queue import Full
import traceback
from datetime import timedelta 
import random
import time
import signal

from api import *

def background_loop(i):
    while True:
        game_key = task_queue.get()
        print("launch_game", i, game_key)
        try:
            start_game(game_key)
        except:
            traceback.print_exc()

task_queue = Queue(30)

def random_worker():
    while True:
        print('loop',  os.getpid())
        with sqlite3.connect(dbname) as db:
            db.row_factory = sqlite3.Row
            l = db.cursor().execute("""
                select count(*) as c from game 
                inner join map on game.map_key = map.key 
                where status = 'RUNNING' and tag = 'LARGE'
                """).fetchone()['c']
            cands = ['SMALL'] * 5 + ['MEDIUM'] * 2
            if l < 5:
                cands.append('LARGE')
            if task_queue.empty():
                try:
                    tag = random.choice(cands)
                    r = random_match(db, tag)
                    print("match result:", r)
                except Exception:
                    traceback.print_exc()
            time.sleep(random.randrange(2))
    print("died!!!")

def main():
    if len(sys.argv) > 1:
        NUM_WORKERS = int(sys.argv[1])
    else:
        NUM_WORKERS = 2

    workers = [ Process(target=background_loop,args=(i,)) for i in range(NUM_WORKERS) ] + [ Process(target =random_worker)  ]
    with sqlite3.connect(dbname) as db:
        db.row_factory = sqlite3.Row
        db.cursor().execute("delete from game where status = 'RUNNING'")
        db.commit()

    for th in workers:
        th.start()
    try:
        prev_key = None
        while True:
            with sqlite3.connect(dbname) as db:
                db.row_factory = sqlite3.Row
                cur = db.cursor()
                game = cur.execute("select * from game where status = 'INQUEUE' order by created_at asc").fetchone()
                n = cur.execute("select count(*) as c from game where status= 'INQUEUE'").fetchone()['c']
                print("queue:", n)

                if game is not None and game['key'] != prev_key:
                    task_queue.put(game['key'])
                    prev_key = game['key']
                else:
                    time.sleep(1)
    finally:
        for th in workers:
            th.terminate()

if __name__ == '__main__':
    main()
