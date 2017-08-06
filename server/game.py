import json


class Game:
    def __init__(self, n, game_map_path):
        self.n = n
        self.game = json.load(open(game_map_path,'r'))
        for river in self.game['rivers']:
            river['owned_by'] = -1
        self.state = [ None for i in range(n) ]

        # For extra rules
        self.settings = {"futures": True, "splurges": True, "options": True}
        ## Futures
        self.futures = [ [] for i in range(n) ]
        ## Splurges
        self.cont_pass = [ 0 for i in range(n) ] # count of continuous passes
        ## Options
        count_mines = len(self.game['mines'])
        self.option_owners = {}
        self.rest_options = [ count_mines for i in range(n) ] # rest of count of option call

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
        used_option_counter = 0
        for t in route[1:]:
            river = self.get_river(s, t)
            if river == None:
                return "No river ({}, {})".format(s, t)

            if river in route_rivers:
                return "{} occurs multiple times in route.".format(river)

            if river['owned_by'] != -1:
                if not self.settings['options']:# Can't use 'options'
                    return "{} is owned by other player".format(river)
                elif self.rest_options[player_id] < used_option_counter + 1:
                    return "too many option call in splurge (rest {})".format(self.rest_options[player_id])
                elif river['owned_by'] == player_id:
                    return "this river is already my river"
                elif (s, t) in self.option_owners:
                    return "Option of ({},{}) is already owned by {}".format(s, t, self.option_owners[(s, t)])
                used_option_counter += 1

            route_rivers.append(river)
            s = t

        # Passed all validations
        for river in route_rivers:
            if river['owned_by'] == -1:
                river['owned_by'] = player_id
            else:
                self.option_owners[(river['source'], river['target'])] = player_id
                self.option_owners[(river['target'], river['source'])] = player_id
                self.rest_options[player_id] -= 1

        return None

    def move_option(self, player_id, claim):
        # validation
        if claim["punter"] != player_id:
            return "'punter' should be {}, but {}".format(player_id, claim["punter"])
        if not (0 < self.rest_options[player_id]):
            return "impossible to call `option` " + self.rest_options[player_id]

        source = claim['source']
        target = claim['target']
        river = self.get_river(source, target)
        if river:
            if river['owned_by'] == -1:
                return "[Option Error]: the river is NOT owned by other players"
            if river['owned_by'] == player_id:
                return "this river is already my river"
            if (source, target) in self.option_owners:
                return "Option of ({},{}) is already owned by {}".format(source, target, self.option_owners[(source, target)])

            self.option_owners[(source, target)] = player_id
            self.option_owners[(target, source)] = player_id
            return None
        return "no such river"
