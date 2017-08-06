#include <memory>
#include <map>
#include <unordered_map>

#include "Game.h"

using namespace std;

typedef pair<int, int> move_t;
struct Node {
  vector<int> payoffs; /* total payoff for each player */
  int n_plays; /* number of plays, wins */
  int from, to; /* river */
  int cur_player; /* who's turn? */
	
  unordered_map<int, unique_ptr<Node>> children;
  
  Node(int num_punters, int cur_player, move_t move) : payoffs(num_punters), n_plays(0), from(move.first), to(move.second), cur_player(cur_player) { };
};

struct MCTS_Core {
  static const int MAX_LOG = 1024;
  double log_memo[MAX_LOG];

  static const int MAX_EDGES = 20000;
  int punter_back_array[MAX_EDGES];
  int *punter_back;
  std::unique_ptr<int[]> punter_back_deleter;

  vector<int> connected_mine;
  void calc_connected_mine();

  void backup_graph();
  void rollback_graph(Graph* graph) const;

  Game *parent;
  const double epsilon;
  MCTS_Core(Game *parent, const double epsilon = 0.0) : parent(parent), epsilon(epsilon), root(parent->get_num_punters(), -1, make_pair(-1, -1)), max_score(1.0) {}
  void run_simulation();

  void reset_root() {
	  root = Node(parent->get_num_punters(), -1, make_pair(-1, -1));
  }
  vector<int> run_simulation(Node *p_root, const vector<int> &futures);
  pair<int, int> get_play(int timelimit_ms);
  vector<int> get_futures(int timelimit_ms);
  Node root;
  void run_futures_selection(vector<int> &futures, int target);
  double max_score;
};
