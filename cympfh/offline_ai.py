#!/usr/bin/env python3
# -*- encoding: utf-8 -*-

# cympfhさんのAIのoffline版

import sys
import json
import copy

# ターンを跨いで持ち回す状態
class State:
    def __init__(self, state):
      self.my_id= state['my_id']
      self.owner = state['owner']
      self.my_vertexs = state['my_vertexs']
      self.edges = state['edges']
      self.game_map = state['game_map']
      self.neigh = state['neigh']

    def to_json(self):
      return json.dumps(
          {
              "my_id": self.my_id,
              "owner": self.owner,
              "my_vertexs": self.my_vertexs,
              "edges": self.edges,
              "game_map": self.game_map,
              "neigh": self.neigh
          })

# globalなstate
g_state = None

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
    sys.stderr.write('write: ' + json.dumps(obj) + '\n')
    obj["state"] = g_state.to_json()
    s = json.dumps(obj)
    sys.stdout.write("{}:{}".format(len(s), s))
    sys.stdout.flush()

def move_claim(s, t):
    write_json({'claim': {'punter': g_state.my_id, 'source': s, 'target': t}})


def move_pass():
    write_json({'pass': {'punter': g_state.my_id}})


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

    state_dict = {
      "my_id": game['punter'],
      "owner": [None] * R,
      "my_vertexs": [],
      "edges": edges,
      "game_map": game['map'],
      "neigh": neigh
    }

    # global stateの初期化
    g_state = State(state_dict)

    ## ready
    write_json({'ready': p})
else:
    moves = game['move']['moves']
    state_dict = json.loads(game['state'])

    # jsonのstateの情報から， g_stateを初期化する
    g_state = State(state_dict)

    for move in moves:
        if 'claim' in move:
            claim = move['claim']
            s = claim['source']
            t = claim['target']
            g_state.owner[g_state.edges[str((s, t))]['id']] = claim['punter']
        else:
            pass

    done = False

    for mine in g_state.game_map['mines']:
        for edge in g_state.neigh[mine]:
            s = edge['source']
            t = edge['target']
            if g_state.owner[ g_state.edges[str((s, t))]['id'] ] is None:
                g_state.my_vertexs.append(s)
                g_state.my_vertexs.append(t)
                g_state.owner[g_state.edges[str((s, t))]['id']] = g_state.my_id
                move_claim(s, t)
                done = True
                break
            if done:
                break
        if done:
            break

    if done:
        exit(0)

    for s in g_state.my_vertexs:
        for edge in g_state.neigh[s]:
            s = edge['source']
            t = edge['target']
            if g_state.owner[ g_state.edges[str((s, t))]['id'] ] is not None:
                continue
            g_state.my_vertexs.append(s)
            g_state.my_vertexs.append(t)
            g_state.owner[g_state.edges[str((s, t))]['id']] = g_state.my_id
            move_claim(s, t)
            done = True
            break
        if done:
            break

    if not done:
        move_pass()
