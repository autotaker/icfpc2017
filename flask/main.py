from flask import Flask, render_template, request, g, jsonify
import sys, time, os
import subprocess
import sqlite3
import binascii
import json
import re

dbname = 'db.sqlite3'
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

def get_map_list():
    maps = os.listdir(map_dir)
    maps = filter(lambda p: p.endswith('.json') and p != 'maps.json', maps)
    return list(maps)

def get_submodule_commit_id():
    return open(os.path.join(app_base_dir, '../.git/refs/heads/master'), 'r').read()


@app.route("/")
def index():
    return render_template('index.html')

@app.route("/battle/")
def battle():
    return render_template('battle.html', programs=get_AI_list()
                                        , maps=get_map_list()
                                        , commit_id = get_submodule_commit_id())

@app.route("/vis/<game_id>")
@app.route("/vis/")
def visualize(game_id = None):
    cur = get_db().cursor()
    if game_id:
        cur.execute('select * from battle where id = ?', (game_id,))
        row = cur.fetchone()
        return render_template('puntfx.html', unique = True, game = row)
    else:
        cur.execute('select * from battle order by created_at DESC LIMIT 10')
        rows = cur.fetchall()
        return render_template('puntfx.html', unique = False, games = rows)

@app.route("/game/<game_id>/")
def get_log_json(game_id):
    cur = get_db().cursor()
    row = cur.execute('select * from battle where id = ?', (game_id,)).fetchone()
    if not row:
        return jsonify({"error" : "no such battle"})
    if row['status'] != 'FINISHED':
        return jsonify({"error" : "the game is not finished"})
    row = dict(row)
    log_file = os.path.join(app_base_dir, 'icfpc2017/server/log/log_%s.json' % row['id'])
    d = json.load(open(log_file, 'r'))
    row['result'] = d
    return jsonify(row)

    

@app.route("/startbattle/", methods=['POST'])
def startbattle():
    game_id = binascii.hexlify(os.urandom(4)).decode('ascii')

    error = None

    ai1 = request.form['ai1'] 
    ai2 = request.form['ai2']
    ai_list = get_AI_list()

    game_map = request.form['map']

    if ai1 not in ai_list or ai2 not in ai_list:
        error = 'invalid ai'
        return render_template('battle.html', programs = get_AI_list()
                                            , maps = get_map_list()
                                            , commit_id = get_submodule_commit_id()
                                            , error_msg = error)

    ai1_path = os.path.join(ai_dir, ai1)
    ai2_path = os.path.join(ai_dir, ai2)

    map_path = os.path.join(map_dir, game_map)

    simulator = os.path.join(app_base_dir, 'icfpc2017/server/main.py')
    
    cur = get_db().cursor()
    cur.execute('insert into battle (id, status) values (?,?)', (game_id, 'RUNNING'))
    get_db().commit()

    proc = subprocess.Popen(['python3', simulator, '--id', game_id, '--map', map_path, ai1_path, ai2_path],
                            stdout = subprocess.PIPE, stderr = subprocess.PIPE, universal_newlines = True)
    out, err = proc.communicate()
    print(out)
    out = re.sub('Turn[^\n]*\n','',out)
    print(out)
    if proc.returncode != 0:
        cur.execute("update battle set status = 'ERROR' where id = ?", (game_id,))
        get_db().commit()
    else:
        cur.execute("update battle set status = 'FINISHED' where id = ?", (game_id,))
        get_db().commit()
    
    row = cur.execute('select id, status, created_at from battle where id = ?', (game_id,)).fetchone()

    created_at = row['created_at']
    status = row['status']

    return render_template('result.html', game_id = game_id
                                        , ai1 = ai1
                                        , ai2 = ai2
                                        , maps = get_map_list()
                                        , created_at = created_at
                                        , status = status
                                        , stdout = out
                                        , stderr = err)

if __name__ == "__main__":
    app.run()
