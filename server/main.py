
import sys, os
import subprocess
import binascii
import argparse
from game import *

parser = argparse.ArgumentParser(description='Offline mode simulator')
parser.add_argument('players', metavar = 'F', type=str, nargs = '+', help = "path to player programs") 
parser.add_argument('--map', type = str, help = "path to map json", default = '../maps/sample.json')
parser.add_argument('--eval', type = str, help = "path to evaluator", default = '../src/eval')

logpath = './log'


def launch_process(game_id, executable):
    base = os.path.basename(executable)
    errfile = logpath + ('/%s_%s_stderr.log' % (game_id, base))
    process = subprocess.Popen(executable, stdin = subprocess.PIPE, stdout = subprocess.PIPE, stderr=open(errfile,'w'), universal_newlines = True)
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

def communicate_client(cmd, obj, err):
    print('send', obj)
    s = subprocess.check_output(cmd, input = json.dumps(obj), stderr = err, universal_newlines=True)
    robj = json.loads(s[s.find(':')+1:])
    print('recv', robj)
    return robj

    
def main():
    argv = parser.parse_args()
    players = argv.players
    game_id = gen_game_id()
    print("Game id = %s" % game_id)


    # processes = [ ]
    n = len(players)
    # for player in players:
    #     processes.append(launch_process(game_id, player))
    
    print(argv.map)
    game = Game(len(players), argv.map)

    errs = [ open(logpath + ('/%s_%s_stderr.log' % (game_id, os.path.basename(p))), 'w') for p in players ]

    # setup
    for i, p in enumerate(players):
        obj = communicate_client(p, { "punter": i, "punters": n, "map" : game.game }, errs[i])
        game.state[i] = obj["state"]
    
    current = 0
    global_moves = []
    moves = [ { 'pass' : { 'punter' : i } } for i in range(n) ]
    for _ in range(len(game.game['rivers'])):
        p = players[current]
        state = game.state[current]
        move = communicate_client(p, { 'move' : {'moves' : moves}, 'state': state }, errs[current])
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
        global_moves.append(move)
        moves[current] = move
        current = (current + 1) % n
    # calculate score
    try:
        eval_proc = subprocess.Popen(argv.eval, universal_newlines = True, stdin = subprocess.PIPE, stdout = subprocess.PIPE)
        scores = communicate_client(argv.eval, { "punters" : n, "map" : game.game }, None)
    finally:
        eval_proc.kill()

        
    #for p in processes:
    #    # TODO 
    #    send_json(p, { 'stop' : moves, 'scores' : scores })

    logfile = logpath + ('/log_%s.json' % game_id)
    json.dump( { "setup" : game.game, "punters" : n, "moves" : global_moves } , open(logfile,'w') )




if __name__ == '__main__':
    main()
