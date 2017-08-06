from flask import Flask, render_template, request, g, jsonify, redirect,abort, url_for
import sys, time, os
import subprocess
import sqlite3
import binascii
import json
import re
import threading
from multiprocessing import Process, Queue
from queue import Full
import traceback
from datetime import timedelta 

dbname = 'db.sqlite3'
task_queue = Queue(10)

def background_loop():
    db = sqlite3.connect(dbname)
    db.row_factory = sqlite3.Row
    while True:
        game_key = task_queue.get()
        print("launch_game", game_key)
        try:
            start_game(db,game_key)
        except:
            traceback.print_exc()

NUM_WORKERS = 2
workers = [ Process(target=background_loop) for _ in range(NUM_WORKERS) ]

app = Flask(__name__)

def get_db():
    db = getattr(g, '_database', None)
    if db is None:
        db = g._database = sqlite3.connect(dbname)
        db.row_factory = sqlite3.Row
    return db

@app.teardown_appcontext
def close_connection(exception):
    db = getattr(g, '_database', None)
    if db is not None:
        db.close()

app_base_dir = os.path.dirname(__file__)
ai_dir = os.path.join(app_base_dir, 'icfpc2017/bin')
map_dir = os.path.join(app_base_dir, 'icfpc2017/maps')

def get_AI_list():
    programs = os.listdir(ai_dir)
    programs = filter(lambda p: p[0].isupper(), programs)
    return list(programs)

def get_AI_src_list():
    programs = os.listdir(os.path.join(app_base_dir,'icfpc2017/AI'))
    programs = map(lambda p: p[:-4], filter(lambda p: p.endswith('.cpp'), programs))
    return list(programs)

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
    return open(os.path.join(app_base_dir, '../.git/refs/heads/master'), 'r').read()

@app.route("/")
def index():
    return render_template('index.html')

@app.route("/AI/register", methods =  ['GET','POST'])
def register_AI():
    programs = get_AI_src_list()
    programs = list(filter(lambda p: '#' not in p, programs)) 
    if request.method == 'GET':
        return render_template('register_AI.html', programs = programs)
    else:
        name = request.form['id']
        commit_id = get_submodule_commit_id() 
        cur = get_db().cursor()
        row = cur.execute('select * from AI where name = ? AND commit_id = ?', 
                          (name, commit_id)).fetchone()
        
        # compile
        cwd = os.path.join(app_base_dir, 'icfpc2017')
        proc = subprocess.Popen(['make', '-C', cwd, os.path.join('bin/',name)],
                                stdout = subprocess.PIPE, stderr = subprocess.PIPE,
                                universal_newlines = True)
        out, err = proc.communicate()
        err_msg = ''
        if proc.returncode != 0:
            err_msg = 'Compile Error!'
        else:
            bin_name = ai_name(name, commit_id)
            subprocess.run(['cp', os.path.join(cwd, 'bin/', name), 
                                  os.path.join(cwd, 'bin/', bin_name)])

        if err_msg:
            return render_template('register_AI.html', programs = programs,
                                   error_msg = err_msg, 
                                   log_stdout = out, log_stderr = err)
        elif row:
            return redirect(url_for("show_AI", key = row['key']))
        else:
            cur.execute('insert into AI (commit_id, name, status) values (?,?,?)',
                        (commit_id, name, 'READY'))
            row = cur.execute('select * from AI where name = ? AND commit_id = ?', 
                              (name, commit_id)).fetchone()
            get_db().commit()
            return redirect(url_for("show_AI", key = row['key']))

@app.route("/AI/list/")
def show_AI_list():
    cur = get_db().cursor()
    ai_list = cur.execute('select * from AI order by key desc').fetchall()
    return render_template('show_AI_list.html', ai_list = ai_list)

@app.route("/AI/show/<int:key>", methods =  ['GET','POST'])
def show_AI(key):
    cur = get_db().cursor()
    info = cur.execute('select * from AI where key = ?', (key,)).fetchone()
    if not info:
        abort(404)
    else:
        return render_template('show_AI.html', info = info)

@app.route("/vis/<game_id>")
def visualize(game_id = None):
    cur = get_db().cursor()
    row = cur.execute('select * from game where id = ?', (game_id,)).fetchone()
    return render_template('puntfx.html', unique = True, game = row)

@app.route("/game/<game_id>/json")
def get_log_json(game_id):
    cur = get_db().cursor()
    row = cur.execute('select * from game where id = ?', (game_id,)).fetchone()
    if not row:
        return jsonify({"error" : "no such battle"})
    row = dict(row)
    players = get_game_players(get_db(), row['key'])
    row['players'] = list(map(dict,players))

    if row['status'] == 'ERROR':
        return jsonify({"error" : "the game is broken"})
    elif row['status'] == 'FINISHED':
        log_file = os.path.join(app_base_dir, 'icfpc2017/server/log/log_%s.json' % row['id'])
        d = json.load(open(log_file, 'r'))
        row['result'] = d
        return jsonify(row)
    elif row['status'] == 'RUNNING':
        log_file = os.path.join(app_base_dir, 'icfpc2017/server/log/log_%s_current.json' % row['id'])
        try:
            d = json.load(open(log_file, 'r'))
            row['result'] = d
            return jsonify(row)
        except Exception:
            return jsonify({"error" : "parse error. try again"})

def start_game(db,game_key):
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
                               '--id', game['id'], '--map', map_path] + ai_pathes,
                               stdout = log_file, stderr = log_file, universal_newlines = True)
        log_file.close()
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
        cur.execute("update game set status = 'ERROR' where key = ?", (game_key,))
        raise e

@app.route("/battle/", methods=['GET','POST'])
def battle():
    if request.method == 'POST':
        game_id = binascii.hexlify(os.urandom(4)).decode('ascii')

        punters = int(request.form['punters'])
        ai_keys = [ request.form['ai_key_%d' % i] for i in range(punters) ]
        print(ai_keys)

        cur = get_db().cursor()
        game_map = cur.execute('select * from map where key = ?',(int(request.form['map']),)).fetchone()

        # validate ai_keys
        for ai_key in ai_keys:
            if cur.execute('select * from AI where key = ?', (ai_key,)).fetchone() is None:
                abort(400)

        cur.execute('insert into game (id, map_key, status) values (?,?,?)', 
                    (game_id, game_map['key'], 'INQUEUE'))
        get_db().commit()
        game = cur.execute('select * from game where id = ?', (game_id,)).fetchone()
        for i,ai_key in enumerate(ai_keys):
            cur.execute('insert into game_match (game_key, ai_key, play_order) values (?,?,?)'
                        ,(game['key'], ai_key, i))
        get_db().commit()
        
        try: 
            task_queue.put(game['key'], block = False)
        except Full:
            cur.execute("update game set status = 'ERROR' where key = ?", (game['key'],))
            get_db().commit()
            err_msg = 'task queue is full, try again later'
            ai_list = cur.execute("select * from AI where status = 'READY'").fetchall()
            maps = cur.execute("select * from map order by size asc").fetchall()
            return render_template('battle.html', maps = maps, ai_list = ai_list, error_msg = err_msg)
        return redirect(url_for('show_game', game_id = game['id']))
    else:
        cur = get_db().cursor()
        ai_list = cur.execute("select * from AI where status = 'READY'").fetchall()
        maps = cur.execute("select * from map order by size asc").fetchall()
        return render_template('battle.html', maps = maps, ai_list = ai_list)
        
        # return render_template('result.html', game_id = game_id
        #                                     , ai1 = ai1
        #                                     , ai2 = ai2
        #                                     , maps = get_map_list()
        #                                     , created_at = created_at
        #                                     , status = status
        #                                     , stdout = out
        #                                     , stderr = err)
@app.route("/game/<game_id>")
def show_game(game_id):
    cur = get_db().cursor()
    game = cur.execute('select * from game where id = ?', (game_id,)).fetchone()
    if game is None:
        abort(404)
    ai_list = get_game_players(get_db(), game['key'])
    return render_template('result.html', game = game, ai_list = ai_list)

@app.route("/game/")
def show_game_list():
    cur = get_db().cursor()
    games = cur.execute('select * from game order by created_at desc limit 30').fetchall()
    
    games = [ dict(game) for game in games ]
    for game in games:
        game['players'] = get_game_players(get_db(), game['key'])

    return render_template('show_game_list.html', games = games)

for th in workers:
    th.start()
app.config['SEND_FILE_MAX_AGE_DEFAULT'] = timedelta(seconds=5)

if __name__ == "__main__":
    app.run()
