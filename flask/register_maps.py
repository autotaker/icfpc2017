import sqlite3
import os
import json

dbname = 'db.sqlite3'

db = sqlite3.connect(dbname)
cur = db.cursor()

app_base_dir = os.path.dirname(__file__)
ai_dir = os.path.join(app_base_dir, 'icfpc2017/bin')
map_dir = os.path.join(app_base_dir, 'icfpc2017/maps')

def get_map_list():
    maps = os.listdir(map_dir)
    maps = filter(lambda p: p.endswith('.json') and p != 'maps.json', maps)
    return list(maps)

for m in get_map_list():
    obj = json.loads(open(os.path.join(map_dir, m), "r").read())
    size = len(obj['rivers'])
    if size < 100:
        tag = 'SMALL'
    elif size < 500:
        tag = 'MEDIUM'
    else:
        tag = 'LARGE'
    if cur.execute('select key from map where id = ?', (m,)).fetchone() is None:
        cur.execute('insert into map (id, size, tag) values (?,?,?)', (m,size,tag))
        db.commit()

        

