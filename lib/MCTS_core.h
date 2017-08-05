#include <memory>
#include <map>

#include "Game.h"

using namespace std;

typedef pair<int, int> move_t;
struct Node {
  vector<int> payoffs; /* total payoff for each player */
  int n_plays; /* number of plays, wins */
  int from, to; /* river */
  int cur_player; /* who's turn? */
	
  map<move_t, unique_ptr<Node>> children;

  Node(int num_punters, int cur_player, move_t move) : payoffs(num_punters), n_plays(0), from(move.first), to(move.second), cur_player(cur_player) { };
};

struct MCTS_Core {
  const Game &parent;
  MCTS_Core(const Game &parent) : parent(parent), root(parent.get_num_punters(), -1, make_pair(-1, -1)) {}
  void reset_root() {
	  root = Node(parent.get_num_punters(), -1, make_pair(-1, -1));
  }
  vector<int> run_simulation(Node *p_root, const vector<int> &futures);
  pair<int, int> get_play(int timelimit_ms);
  vector<int> get_futures(int timelimit_ms);
  Node root;
  void run_futures_selection(vector<int> &futures, int target);
};
