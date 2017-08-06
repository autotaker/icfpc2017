import json


class Game:
    def __init__(self, n, game_map_path):
        self.n = n
        self.game = json.load(open(game_map_path,'r'))
        for river in self.game['rivers']:
            river['owned_by'] = -1
        self.state = [ None for i in range(n) ]

        # For extra rules
        ## Futures
        self.settings = {"futures": True, "splurges": True}
        self.futures = [ [] for i in range(n) ]
        ## Splurge
        self.cont_pass = [ 0 for i in range(n) ] # count of continuous passes


    def get_river(self, source, target):
        for river in self.game['rivers']:
            if (river['source'] == source and river['target'] == target or
                river['source'] == target and river['target'] == source):
                return river

        return None

    def move_claim(self, player_id, claim):
        # validation
        if claim["punter"] != player_id:
            return "'punter' should be {}, but {}".format(player_id, claim["punter"])

        source = claim['source']
        target = claim['target']
        river = self.get_river(source, target)
        if river:
            if river['owned_by'] == -1:
                river['owned_by'] = player_id
                return None
            else:
                return "the river is owned by other player"
        return "no such river"

    def move_splurge(self, player_id, splurge):
        route = splurge["route"]
        # validation
        if splurge["punter"] != player_id:
            return "'punter' should be {}, but {}".format(player_id, splurge["punter"])

        path_len = len(route) - 1
        if not (path_len - 1 <= self.cont_pass[player_id]):
            return "path_len = {}, cont_pass = {}".format(
                path_len, self.cont_pass[player_id]
            )

        s = route[0]
        route_rivers = []
        for t in route[1:]:
            river = self.get_river(s, t)
            if river == None:
                return "No river ({}, {})".format(s, t)

            if river['owned_by'] != -1:
                return "{} is owned by other player".format(river)

            if river in route_rivers:
                return "{} occurs multiple times in route.".format(river)
            route_rivers.append(river)
            s = t

        for river in route_rivers:
            river['owned_by'] = player_id
        return None

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
