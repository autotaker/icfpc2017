
import sys, os
import subprocess
import binascii
from game import *

errpath = './log'

def launch_process(game_id, executable):
    base = os.path.basename(executable)
    logfile = errpath + ('/err_%s_%s.log' % (game_id, base))
    process = subprocess.Popen(executable, stdin = subprocess.PIPE, stdout = subprocess.PIPE, stderr=open(logfile,'w'), universal_newlines = True)
    return process

def gen_game_id():
    return binascii.hexlify(os.urandom(4)).decode('ascii')

def send_json(process, obj):
    print('send', obj)
    s = json.dumps(obj)
    process.stdin.write(("%d:%s" % (len(s), s)))
    process.stdin.flush()

def recv_json(process):
    s = ""
    while True:
        ch = process.stdout.read(1)
        if ch == ':':
            break
        s += ch
    n = int(s) 
    obj = json.loads(process.stdout.read(n))
    print('recv', obj)
    return obj

    
def main():
    players = sys.argv[1:]
    game_id = gen_game_id()
    print("Game id = %s" % game_id)
    processes = [ ]
    n = len(players)
    try:
        for player in players:
            processes.append(launch_process(game_id, player))
        
        game = Game(len(players), 'maps/example.json')

        # setup
        for i, p in enumerate(processes):
            send_json(p, { "punter": i, "punters": n, "map" : game.game }) 
            # 
            obj = recv_json(p)
            game.state[i] = obj["state"]
        
        current = 0
        moves = [ { 'pass' : { 'punter' : i } } for i in range(n) ]
        for _ in range(len(game.game['rivers'])):
            p = processes[current]
            state = game.state[current]
            send_json(p, { 'move' : {'moves' : moves}, 'state': state })
            move = recv_json(p)
            if 'claim' in move:
                claim = move['claim']
                source = claim['source']
                target = claim['target']
                err = game.move_claim( current, source, target)
                if err:
                    move = { 'pass' : { 'punter' : current }}
            if 'state' in move:
                state = move['state']
                del move['state']
            
            game.state[current] = state
            moves[current] = move
            current = (current + 1) % n
        # calculate score
        for p in processes:
            # TODO 
            scores = [ 0 in range(n) ]
            send_json(p, { 'stop' : moves, 'scores' : scores })
        
    finally:
        for p in processes:
            p.kill()




if __name__ == '__main__':
    main()
