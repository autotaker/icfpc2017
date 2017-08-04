import json


class Game:
    def __init__(self, n, game_map_path):
        self.n = n
        self.game = json.load(open(game_map_path,'r'))
        for river in self.game['rivers']:
            river['owned_by'] = -1
        self.state = [ None for i in range(n) ]
    
    def move_claim(self, player_id, source, target):
        for river in self.game['rivers']:
            if river['source'] == source and river['target'] == target:
                if river['owned_by'] == -1:
                    river['owned_by'] = player_id
                    return None
                else:
                    return "the river is owned by other player"
        return "no such river"


