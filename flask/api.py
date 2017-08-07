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

app_base_dir = os.path.dirname(__file__)
ai_dir = os.path.join(app_base_dir, 'icfpc2017/bin')
map_dir = os.path.join(app_base_dir, 'icfpc2017/maps')

dbname = 'db.sqlite3'

def get_AI_list():
    programs = os.listdir(ai_dir)
    programs = filter(lambda p: p[0].isupper(), programs)
    return list(programs)

def get_AI_src_list():
    programs = os.listdir(os.path.join(app_base_dir,'icfpc2017/AI'))
    programs = map(lambda p: p[:-4], filter(lambda p: p.endswith('.cpp'), programs))
    return list(programs)

def get_ready_AI(cur):
    return cur.execute("select * from AI where status = 'READY'").fetchall()


def get_map_list():
    maps = os.listdir(map_dir)
    maps = filter(lambda p: p.endswith('.json') and p != 'maps.json', maps)
    return list(maps)
def ai_name(name, commit_id):
    return "%s#%s" % (name, commit_id)

def get_game_players(db, game_key):
    cur = db.cursor()
    ai_list = cur.execute("""
        select AI.name as name, AI.commit_id as commit_id, AI.key as key,
               game_match.score as score, game_match.rank as rank
        from AI 
        inner join game_match 
        on AI.key = game_match.ai_key
        where game_match.game_key = ?
        order by game_match.play_order asc
        """, (game_key,)).fetchall()
    return ai_list

def get_submodule_commit_id():
    return open(os.path.join(app_base_dir, '../.git/refs/heads/master'), 'r').read().rstrip()

def get_recent_scores(db, ai_key, tag, limit = 10):
    cur = db.cursor()
    return cur.execute("""
        select score, rank , game.created_at as created_at, 
               map.id as map, game.id as game_id, c as count 
        from game_match
        inner join game on game.key = game_match.game_key
        inner join map  on map.key = game.map_key
        inner join (select game_key, count(game_key) as c 
            from game_match group by game_key) punters on punters.game_key = game.key
        where ai_key = ? and map.tag = ? and rank is not null 
              and created_at > datetime('now', '-1 hours')
        order by game.created_at desc 
        """, (ai_key, tag)).fetchall()

def update_rating(db, ai_key, limit = 10):
    cur = db.cursor()
    for tag,col in [('SMALL','small_rating'),('MEDIUM','med_rating'),('LARGE','large_rating')]:
        results = get_recent_scores(db, ai_key, tag, limit)
        point = 0
        n = len(list(results))
        for r in results:
            point += r['count'] - (r['rank'] - 1)
        if n > 0:
            cur.execute('update AI set ' + col + ' = ? where key = ? ', (point / n, ai_key))
    db.commit()

def start_game(game_key):
    with sqlite3.connect(dbname) as db:
        db.row_factory = sqlite3.Row

        cur = db.cursor()
        game = cur.execute('select * from game where key = ?', (game_key,)).fetchone()
        game_map = cur.execute('select id from map where key = ?', (game['map_key'],)).fetchone()
        cur.execute("update game set status = 'RUNNING' where key = ?", (game_key,)) 
        db.commit()

    try:
        print(game_map['id'])
        ai_list = get_game_players(db, game_key)
        map_path = os.path.join(map_dir, game_map['id'])
        ai_pathes = [ os.path.join(ai_dir, ai_name(ai['name'], ai['commit_id'])) for ai in ai_list ]
        log_file = os.path.join(app_base_dir, 'icfpc2017/server/log/log_%s.txt' % game['id'])
        log_file = open(log_file, 'w')
        simulator = os.path.join(app_base_dir, 'icfpc2017/server/main.py')
        proc = subprocess.run(['python3', simulator, 
                               '--id', game['id'], '--map', map_path, '--nodump'] + ai_pathes, 
                               stdout = log_file, stderr = log_file, universal_newlines = True)
        log_file.close()
        with sqlite3.connect(dbname) as db:
            db.row_factory = sqlite3.Row
            cur = db.cursor()
            if proc.returncode != 0:
                # error!
                
                cur.execute("update game set status = 'ERROR' where key = ?", (game_key,))
                db.commit()
            else:
                scores = json.loads(open(os.path.join(app_base_dir, 'icfpc2017/server/log/log_%s.json' % game['id'])).read())['moves'][-1]['scores']
                scores = list(map(lambda s: s['score'], scores))
                l = list(reversed(sorted(set(scores))))
                for i, ai in enumerate(ai_list):
                    rank = l.index(scores[i]) + 1
                    cur.execute("""
                        update game_match 
                        set score = ?, rank = ? 
                        where game_key = ? and play_order = ?
                        """, (scores[i], rank, game_key, i))
                cur.execute("update game set status = 'FINISHED' where key = ?", (game_key,))
                db.commit()
    except Exception as e:
        with sqlite3.connect(dbname) as db:
            db.row_factory = sqlite3.Row
            cur = db.cursor()
            cur.execute("update game set status = 'ERROR' where key = ?", (game_key,))
            db.commit()
        raise e

def create_game(db, ai_keys, map_key, prefix = ''):
    cur = db.cursor()
    game_id = prefix + binascii.hexlify(os.urandom(4)).decode('ascii')
    game_map = cur.execute('select * from map where key = ?',(map_key,)).fetchone()

    count = len(cur.execute("select * from game where status = 'INQUEUE'").fetchall())

    if count >= 25:
        return None, 'task queue is full, try again later'

    if game_map is None:
        return None, 'invalid map' 

    # validate ai_keys
    for ai_key in ai_keys:
        if cur.execute('select * from AI where key = ?', (ai_key,)).fetchone() is None:
            return None, 'invalid AI'

    cur.execute('insert into game (id, map_key, status) values (?,?,?)', 
                (game_id, map_key, 'INQUEUE'))
    db.commit()

    game = cur.execute('select * from game where id = ?', (game_id,)).fetchone()
    for i,ai_key in enumerate(ai_keys):
        cur.execute('insert into game_match (game_key, ai_key, play_order) values (?,?,?)'
                    ,(game['key'], ai_key, i))
    db.commit()
    return game , None

def prefered_player_num(tag):
    if tag == 'SMALL':
        return 4
    if tag == 'MEDIUM':
        return 8
    if tag == 'LARGE':
        return 16

def random_match(db,tag):
    print('rando match', tag)
    cur = db.cursor()
    ai_list = list(get_ready_AI(cur))
    punters = prefered_player_num(tag)
    random.shuffle(ai_list)
    ai_list = ai_list * (punters // len(ai_list)) + ai_list[:(punters % len(ai_list))]
    random.shuffle(ai_list)

    ai_keys = list(map(lambda x: x['key'], ai_list))
    game_map = cur.execute( 'select * from map where tag = ? order by RANDOM()', (tag,)).fetchone()
    return create_game(db, ai_keys, game_map['key'], prefix = 'random') 

