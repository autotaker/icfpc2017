#include <memory>
#include <map>

#include "Game.h"

using namespace std;

typedef pair<int, int> move_t;
struct Node {
  int n_plays, n_wins; /* number of plays, wins */
  int from, to; /* river */
  int cur_player; /* who's turn? */
	
  map<move_t, unique_ptr<Node>> children;

  Node(int cur_player, move_t move) : n_plays(0), n_wins(0), from(move.first), to(move.second), cur_player(cur_player) { };
};

struct MCTS_Core {
  const Game &parent;
  MCTS_Core(const Game &parent) : parent(parent), root(-1, make_pair(-1, -1)) {}
  void run_simulation();
  pair<int, int> get_play(int timelimit_ms);
  Node root;
};
