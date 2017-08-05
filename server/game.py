import json


class Game:
    def __init__(self, n, game_map_path):
        self.n = n
        self.game = json.load(open(game_map_path,'r'))
        for river in self.game['rivers']:
            river['owned_by'] = -1
        self.state = [ None for i in range(n) ]

        # For optional rules
        self.settings = {"futures": True}
        self.futures = [ [] for i in range(n) ]

    def move_claim(self, player_id, source, target):
        for river in self.game['rivers']:
            if (river['source'] == source and river['target'] == target or
                river['source'] == target and river['target'] == source):
                if river['owned_by'] == -1:
                    river['owned_by'] = player_id
                    return None
                else:
                    return "the river is owned by other player"
        return "no such river"


    def set_futures(self, punter_id, futures):
        dic = {}
        for fu in futures:
            src = fu["source"]
            dst = fu["target"]
            if not src in self.game["mines"]:# source must be a mine
                continue
            if dst in self.game["mines"]:# target must not be a mine
                continue
            dic[src] = dst

        filtered_futures = [ {"source": s, "target": t} for (s, t) in dic.items() ]
        self.futures[punter_id] = filtered_futures
