#!/usr/bin/env python3
import sys
import json


def read_json():
    s = ''
    while True:
        t = sys.stdin.read(1)
        if t == ':':
            break
        s += t
    n = int(s)
    s = sys.stdin.read(n)
    sys.stderr.write('read: ' + s + '\n')
    return json.loads(s)


def write_json(obj):
    s = json.dumps(obj)
    sys.stderr.write('write: ' + s + '\n')
    sys.stdout.write("{}:{}".format(len(s), s))
    sys.stdout.flush()


def move_claim(s, t, p):
    write_json({'claim': {'punter': p, 'source': s, 'target': t}})


def move_pass(p):
    write_json({'pass': {'punter': p}})


## Setup
game = read_json()
p = game['punter']
n = game['punters']
num_vertex = len(game['map']['sites'])

R = len(game['map']['rivers'])
edges = {}
neigh = [[] for _ in range(num_vertex)]
for i, river in enumerate(game['map']['rivers']):
    river['id'] = i
    s = river['source']
    t = river['target']
    edges[s, t] = river
    neigh[s].append(river)
    neigh[t].append(river)

owner = [None] * R
my_vertexs = []  # [id]

## ready
write_json({'ready': p, 'state': 'ok'})

## Gameplay
while True:
    moves = read_json()['move']['moves']
    for move in moves:
        if 'claim' in move:
            claim = move['claim']
            s = claim['source']
            t = claim['target']
            owner[edges[s, t]['id']] = claim['punter']
        else:
            pass

    done = False

    for mine in game['map']['mines']:
        for edge in neigh[mine]:
            s = edge['source']
            t = edge['target']
            if owner[ edges[s, t]['id'] ] is None:
                move_claim(s, t, p)
                my_vertexs.append(s)
                my_vertexs.append(t)
                owner[edges[s, t]['id']] = p
                done = True
                break
            if done:
                break
        if done:
            break

    if done:
        continue

    for s in my_vertexs:
        for edge in neigh[s]:
            s = edge['source']
            t = edge['target']
            if owner[ edges[s, t]['id'] ] is not None:
                continue
            move_claim(s, t, p)
            my_vertexs.append(s)
            my_vertexs.append(t)
            owner[edges[s, t]['id']] = p
            done = True
            break

    if not done:
        move_pass(p)
