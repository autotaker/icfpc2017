
import sys, os
import subprocess
import binascii
import argparse
from game import *

parser = argparse.ArgumentParser(description='Offline mode simulator')
parser.add_argument('players', metavar = 'F', type=str, nargs = '+', help = "path to player programs")
parser.add_argument('--map', type = str, help = "path to map json", default = '../maps/sample.json')
parser.add_argument('--eval', type = str, help = "path to evaluator", default = '../bin/eval')
parser.add_argument('--verbose', help = "verbose output", action='store_true')

logpath = './log'

argv = parser.parse_args()


def gen_game_id():
    return binascii.hexlify(os.urandom(4)).decode('ascii')

def communicate_client(cmd, obj, log_stdin = None, log_stdout = None, log_stderr = None):
    if argv.verbose:
        print('send', obj)
    s = json.dumps(obj)
    s = "%d:%s" % (len(s), s)
    if log_stdin:
        log_stdin.write(s + '\n')
    s = subprocess.check_output(cmd, input = s, stderr = log_stderr, universal_newlines=True)
    if log_stdout:
        log_stdout.write(s + '\n')
    robj = json.loads(s[s.find(':')+1:])
    if argv.verbose:
        print('recv', robj)
    return robj


def main():
    players = argv.players
    game_id = gen_game_id()
    print("Game id = %s" % game_id)


    # processes = [ ]
    n = len(players)
    # for player in players:
    #     processes.append(launch_process(game_id, player))

    print(argv.map)
    game = Game(len(players), argv.map)

    log_errs = [ open(logpath + ('/%s_%d_%s_stderr.log' % (game_id, i, os.path.basename(p))), 'w') for i,p in enumerate(players) ]
    log_ins  = [ open(logpath + ('/%s_%d_%s_stdin.log' % (game_id, i, os.path.basename(p))), 'w') for i,p in enumerate(players) ]
    log_outs = [ open(logpath + ('/%s_%d_%s_stdout.log' % (game_id, i, os.path.basename(p))), 'w') for i,p in enumerate(players) ]



    # setup
    for i, p in enumerate(players):
        obj = communicate_client(p, { "punter": i, "punters": n, "map" : game.game }
                                , log_stdout = log_outs[i]
                                , log_stderr = log_errs[i]
                                , log_stdin = log_ins[i])
        game.state[i] = obj["state"]

    current = 0
    global_moves = []
    moves = [ { 'pass' : { 'punter' : i } } for i in range(n) ]
    for _ in range(len(game.game['rivers'])):
        p = players[current]
        state = game.state[current]
        move = communicate_client(p, { 'move' : {'moves' : moves}, 'state': state }
                                 , log_stdout = log_outs[i]
                                 , log_stderr = log_errs[i]
                                 , log_stdin = log_ins[i])
        if 'claim' in move:
            claim = move['claim']
            source = claim['source']
            target = claim['target']
            err = game.move_claim( current, source, target)
            if err:
                print("invalid move:", move, err)
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
        log_eval_stdin = open(logpath + ('/%s_%s_stdin.log' % (game_id, os.path.basename(argv.eval))), 'w')
        scores = communicate_client(argv.eval, { "punters" : n, "map" : game.game }, log_stdin = log_eval_stdin)
        log_eval_stdin.close()
    finally:
        for l in [log_errs, log_ins, log_outs]:
            for f in l:
                f.close()

    print("result", scores)


    #for p in processes:
    #    # TODO
    #    send_json(p, { 'stop' : moves, 'scores' : scores })

    logfile = logpath + ('/log_%s.json' % game_id)
    json.dump( { "setup" : game.game, "punters" : n, "moves" : global_moves } , open(logfile,'w') )




if __name__ == '__main__':
    main()
