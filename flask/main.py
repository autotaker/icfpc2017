from flask import Flask, render_template, request, g, jsonify, redirect,abort, url_for
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

dbname = 'db.sqlite3'
random_start = Value('i', 0)

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
    print('teardown')
    if db is not None:
        db.close()

@app.route("/")
def index():
    return render_template('index.html')

@app.route("/gitpull/", methods = ['POST'])
def gitpull():
    proc = subprocess.Popen(["git","pull"], stdout = subprocess.PIPE, stderr = subprocess.PIPE, universal_newlines = True)
    err_msg = ''
    out = ''
    err = ''
    try:
        out, err = proc.communicate(timeout = 10)
    except:
        subprocess.TimeoutExpired
        err_msg = 'Timeouted'
    return render_template('gitpull.html', log_stdout = out, log_stderr = err, err_msg = err_msg)
    




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

@app.route("/updateratings/", methods = ['POST'])
def update_AI_ratings():
    cur = get_db().cursor()
    ai_list = get_ready_AI(cur)
    for ai in ai_list:
        print("update rating:", ai['name'])
        update_rating(get_db(), ai['key'])
    return redirect(url_for("show_AI_list"))

@app.route("/AI/toggle/<int:key>", methods = ['POST'])
def toggle_status(key):
    cur = get_db().cursor()
    info = cur.execute('select * from AI where key = ?', (key,)).fetchone()
    if not info:
        abort(400)
    nstatus = info['status']
    if info['status'] == 'READY':
        nstatus = 'OLD'
    if info['status'] == 'OLD':
        nstatus = 'READY'
    
    cur.execute('update AI set status = ? where key = ?', (nstatus, key))
    get_db().commit()
    return jsonify('success')
    
@app.route("/AI/list/")
def show_AI_list():
    if 'verbose' in request.args:
        verbose = True
    else:
        verbose = False
    cur = get_db().cursor()
    ai_list = cur.execute('select * from AI order by key desc').fetchall()
    return render_template('show_AI_list.html', ai_list = ai_list, verbose = verbose)

@app.route("/map/show/<int:key>")
def show_map(key):
    cur = get_db().cursor()
    game_map = cur.execute('select * from map where key = ?', (key,)).fetchone()
    if game_map is None:
        abort(404)
    game_list = cur.execute("""
        select * from game 
        where map_key = ? 
        order by created_at desc limit 30
        """, (key,)).fetchall()
    return render_template('show_map.html', 
                           info = game_map, 
                           game_list = game_list)

@app.route("/map/list/")
def show_map_list():
    cur = get_db().cursor()
    map_list = cur.execute('select * from map order by size').fetchall()
    return render_template('show_map_list.html', map_list = map_list)

@app.route("/AI/show/<int:key>", methods =  ['GET','POST'])
def show_AI(key):
    cur = get_db().cursor()
    info = cur.execute('select * from AI where key = ?', (key,)).fetchone()
    if not info:
        abort(404)
    small_results = get_recent_scores(get_db(), key, 'SMALL')
    med_results = get_recent_scores(get_db(), key, 'MEDIUM')
    large_results = get_recent_scores(get_db(), key, 'LARGE')
    return render_template('show_AI.html', info = info, 
                           small_results = small_results, 
                           med_results = med_results, 
                           large_results = large_results)

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

    
@app.route("/battle/", methods=['GET','POST'])
def battle():
    punters = int(request.args.get('punters','2'))
    if request.method == 'POST':
        
        punters = int(request.form['punters'])
        ai_keys = [ request.form['ai_key_%d' % i] for i in range(punters) ]
        cur = get_db().cursor()
        game, err = create_game(get_db(), ai_keys, int(request.form['map']))
        if err:
            err_msg = err
            ai_list = get_ready_AI(cur)
            maps = cur.execute("select * from map order by size asc").fetchall()
            return render_template('battle.html', maps = maps, 
                                                  ai_list = ai_list, 
                                                  error_msg = err_msg, 
                                                  punters = 2)
        return redirect(url_for('show_game', game_id = game['id']))
    else:
        cur = get_db().cursor()
        ai_list = cur.execute("select * from AI where status = 'READY'").fetchall()
        maps = cur.execute("select * from map order by size asc").fetchall()
        return render_template('battle.html', maps = maps, ai_list = ai_list, punters = punters)
        
@app.route("/game/<game_id>")
def show_game(game_id):
    cur = get_db().cursor()
    game = cur.execute('select * from game where id = ?', (game_id,)).fetchone()
    if game is None:
        abort(404)
    game_map = cur.execute('select * from map where key = ?', (game['map_key'],)).fetchone()
    ai_list = get_game_players(get_db(), game['key'])
    return render_template('result.html', game = game, ai_list = ai_list, game_map = game_map)


@app.route("/start/",methods=['POST'])
def start():
    random_start.value = 1
    print('started')
    return jsonify('started')

@app.route("/stop/",methods=['POST'])
def stop():
    print('stopped')
    random_start.value = 0
    return jsonify('stopped')

@app.route('/summary/json')
def summary():
    cur = get_db().cursor()
    rows = cur.execute("""
        select map.id as map_id,
               map.tag as map_tag,
               map.size as map_size,
               punters.c as punters,
               AI.name as ai_id,
               AI.key as ai_key,
               game.created_at as created_at,
               game_match.play_order as play_order,
               game_match.score as score,
               game_match.rank as rank
        from game_match
        inner join game on game.key = game_match.game_key
        inner join map on game.map_key = map.key
        inner join AI on game_match.ai_key = AI.key
        inner join (select game_key, count(game_key) as c 
            from game_match group by game_key) punters on punters.game_key = game.key
        where game.status = 'FINISHED'
        order by game.created_at desc
        """).fetchall()
    return jsonify(list(map(dict,rows)))


@app.route("/game/")
def show_game_list():
    cur = get_db().cursor()
    games = cur.execute("""
        select game.created_at as created_at,
               game.key as key,
               game.id  as id,
               game.status as status,
               map.id as map_name,
               map.size as map_size,
               map.tag as map_tag
        from game 
        inner join map on game.map_key = map.key
           order by created_at desc limit 50
           """).fetchall()
    
    games = [ dict(game) for game in games ]
    for game in games:
        game['players'] = get_game_players(get_db(), game['key'])

    return render_template('show_game_list.html', games = games)

app.config['SEND_FILE_MAX_AGE_DEFAULT'] = timedelta(seconds=5)

"""
def handler(sig, obj):
    print('sigint')
    for w in workers:
        w.terminate()
    sys.exit(0)
    """

if __name__ == "__main__":
    # signal.signal(signal.SIGINT, handler)
    app.run()
