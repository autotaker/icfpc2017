#!/usr/bin/env python3
# -*- encoding: utf-8 -*-

# cympfhさんのAIのoffline版

import sys
import json
import copy

def read_json():
    s = ''
    while True:
        t = sys.stdin.read(1)
        if t == ':':
            break
        s += t
    n = int(s)
    s = ""
    while len(s) < n:
      s += sys.stdin.read(n - len(s))

    ret = json.loads(s)

    # For debug
    copied_ret = copy.deepcopy(ret)
    copied_ret["state"] = "omitted for log"
    sys.stderr.write('read: ' + json.dumps(copied_ret) + '\n')

    return ret

def write_json(obj):
    s = json.dumps(obj)
    sys.stdout.write("{}:{}".format(len(s), s))
    sys.stdout.flush()
    obj["state"] = "omitted for log"
    sys.stderr.write('write: ' + json.dumps(obj) + '\n')

def move_claim(state, s, t, p):
    write_json({'claim': {'punter': p, 'source': s, 'target': t}, 'state': state.to_json()})


def move_pass(state, p):
    write_json({'pass': {'punter': p}, 'state': state.to_json()})

class State:

  def __init__(self, my_id, owner, vs, edges, game_map, neigh):
    self.my_id = my_id
    self.owner = owner
    self.my_vertexs = vs
    self.edges = edges
    self.game_map = game_map
    self.neigh = neigh

  def to_json(self):
    return json.dumps({"my_id": self.my_id,
                       "owner": self.owner,
                       "vs": self.my_vertexs,
                       "edges": self.edges,
                       "game_map": self.game_map,
                       "neigh": self.neigh
    })

game = read_json()
if 'punter' in game:
    ## Setup
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
        edges[str((s, t))] = river
        neigh[s].append(river)
        neigh[t].append(river)

    owner = [None] * R
    my_vertexs = []  # [id]

    state = State(p, owner, my_vertexs, edges, game['map'], neigh)

    ## ready
    write_json({'ready': p, 'state': state.to_json()})
else:
    moves = game['move']['moves']
    state = json.loads(game['state'])
    p = state['my_id']
    owner = state['owner']
    my_vertexs = state['vs']
    edges = state['edges']
    game_map = state['game_map']
    neigh = state['neigh']

    for move in moves:
        if 'claim' in move:
            claim = move['claim']
            s = claim['source']
            t = claim['target']
            owner[edges[str((s, t))]['id']] = claim['punter']
        else:
            pass

    done = False

    for mine in game_map['mines']:
        for edge in neigh[mine]:
            s = edge['source']
            t = edge['target']
            if owner[ edges[str((s, t))]['id'] ] is None:
                my_vertexs.append(s)
                my_vertexs.append(t)
                owner[edges[str((s, t))]['id']] = p
                move_claim(State(p, owner, my_vertexs, edges, game_map, neigh), s, t, p)
                done = True
                break
            if done:
                break
        if done:
            break

    if done:
        exit(0)

    for s in my_vertexs:
        for edge in neigh[s]:
            s = edge['source']
            t = edge['target']
            if owner[ edges[str((s, t))]['id'] ] is not None:
                continue
            my_vertexs.append(s)
            my_vertexs.append(t)
            owner[edges[str((s, t))]['id']] = p
            move_claim(State(p, owner, my_vertexs, edges, game_map, neigh), s, t, p)
            done = True
            break
        if done:
            break

    if not done:
        move_pass(State(p, owner, my_vertexs, edges, game_map, neigh), p)
