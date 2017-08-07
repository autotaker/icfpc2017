#include "MCTS_core.h"

#include <algorithm>
#include <iostream>
#include <chrono>
#include <set>
#include <cassert>
#include <cmath>
#include <queue>

namespace {

  void apply_move(Graph &cur_state, move_t move, int cur_player, vector<int> &remaining_options) {
    /* apply "move" to "cur_state" */
    for(auto& r : cur_state.rivers[move.first]) {
      if (r.to == move.second) {
	if (r.punter != -1) {
	  assert(r.option == -1);
	  r.option = cur_player;
	  assert(remaining_options[cur_player] > 0);
	  remaining_options[cur_player] -= 1;
	} else {
	  r.punter = cur_player;
	}
      }
    }
    for(auto& r : cur_state.rivers[move.second]) {
      if (r.to == move.first) {
	if (r.punter != -1) {
	  assert(r.option == -1);
	  r.option = cur_player;
	} else {
	  r.punter = cur_player;
	}
      }
    }
  }

  set<int> get_visited_sites(const Game *game, const vector<Node*> visited_nodes, int punter) {
    set<int> vertices;
    const History &history = game->get_history();
    for (const auto &move: history) {
      if (move.punter == punter) {
	vertices.insert(move.to);
	vertices.insert(move.src);
      }
    }

    for (const auto &node: visited_nodes) {
      if (node->cur_player == punter) {
	vertices.insert(node->from);
	vertices.insert(node->to);
      }
    }
    return vertices;
  }

  move_t get_next_greedy(const Game *game, Graph &graph, const set<int> &visited_sites, int punter) {
    int src = -1, to = -1;
    pair<int, int> best_data(-1e9, -1); // pair of a next score and degree of a next vertex
    for (int v = 0; v < graph.num_vertices; v++) {
      for (auto &r : graph.rivers[v]) {
	auto nv = r.to;
	if (v > nv) {
	  continue;
	}
	if (r.punter != -1) {
	  continue;
	}

	const int v_visited = visited_sites.count(v);
	const int nv_visited = visited_sites.count(nv);

	if (v >= graph.num_mines && nv >= graph.num_mines && !v_visited && !nv_visited) {
	  continue;

	}

	Graph::River* nrit = nullptr;
	for (auto &nr : graph.rivers[nv]) {
	  if (nr.to == v) {
	    nrit = &nr;
	    break;
	  }
	}

	assert(nrit != nullptr && nrit -> punter == -1);
	r.punter = punter;
	nrit->punter = punter;
	const auto& next_point = graph.evaluate(game->get_num_punters(), game->get_shortest_distances())[punter];
	pair<int, int> current_data(next_point, graph.rivers[nv].size());
	if (current_data > best_data) {
	  best_data = current_data;
	  src = v;
	  to = nv;
	}
	r.punter = -1;
	nrit->punter = -1;
      }
    }
    return move_t(src, to);   
  }
}

void MCTS_Core::calc_connected_mine() {
  const auto& graph = parent->get_graph();
  connected_mine.resize(graph.num_vertices, -1);

  for (int i = 0; i < graph.num_mines; ++i) {
    if (connected_mine[i] != -1) continue;
    queue<int> q;
    q.push(i);
    connected_mine[i] = i;
    while (!q.empty()) {
      int cv = q.front();
      q.pop();
      for (const auto& r : graph.rivers[cv]) {
	if (connected_mine[r.to] != -1) continue;
	connected_mine[r.to] = i;
	q.push(r.to);
      }
    }
  }
}

pair<int, int> MCTS_Core::get_play(int timelimit_ms) {
  for (int i = 0; i < MAX_LOG; ++i) {
    log_memo[i] = log(i * 1.0);
  }

  backup_graph();
  calc_maybe_unused_edge();
  calc_connected_mine();

  for(auto a : parent->get_futures()) {
    cerr << a << " ";
  } cerr << endl;

  auto start_time = chrono::system_clock::now();
  int n_simulated = 0;
  const vector<int> &futures = parent->get_futures();

  run_simulation(&root, futures);
  ++n_simulated;
  auto one_time = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start_time).count();

  for (;;) {
    auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start_time).count();
    if (elapsed_time + one_time + 50 >= timelimit_ms) break;
    run_simulation(&root, futures);
    n_simulated += 1;
  }

  vector<tuple<double, int, int>> candidates;
  cerr << "----" << endl;
  for(const auto &p : root.children) {
    Node *child = p.second.get();
    double e_payoff = child->payoffs[parent->get_punter_id()] * 1.0 / max(child->n_plays, 1);
    cerr << child->from << " -> " << child->to << " : E[payoff] = " << e_payoff << " (played " << child->n_plays << " times)"<< endl;
    candidates.emplace_back(e_payoff, child->from, child->to);
  }
  sort(candidates.rbegin(), candidates.rend());
  auto scores = parent->get_graph().evaluate(parent->get_num_punters(), parent->get_shortest_distances());
  cerr << "Punter: " << parent->get_punter_id() << endl;
  for(auto p : scores) {
    cerr << p << " ";
  }
  cerr << endl;

  auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start_time).count();
  cerr << "Elapsed time: " << elapsed_time << " msec" << endl;
  cerr << "Simulated " << n_simulated << " times" << endl;

  return make_pair(get<1>(candidates[0]), get<2>(candidates[0]));
}

void MCTS_Core::backup_graph() {
  const Graph& cur_state = parent->get_graph();
  const int total_edges = cur_state.num_edges;
  punter_back = punter_back_array;
  if (total_edges * 4 > MAX_EDGES) {
    punter_back_deleter.reset(new int[total_edges * 4]);
    punter_back = punter_back_deleter.get();
  }

  {
    int* p = punter_back;
    for (const auto& river : cur_state.rivers) {
      for (const auto& r : river) {
	*p = r.punter;
	++p;
	*p = r.option;
	++p;
      }
    }
  }
}

void MCTS_Core::rollback_graph(Graph* graph) const {
  int* p = punter_back;
  for (auto& river : graph->rivers) {
    for (auto& r : river) {
      r.punter = *p;
      ++p;
      r.option = *p;
      ++p;
    }
  }
}

void MCTS_Core::calc_maybe_unused_edge() {
  const Graph& g = parent->get_graph();
  const int total_edges = g.num_edges;
  const bool option_enabled = parent->get_options_enabled();

  num_maybe_unused_edge = 0;
  maybe_unused_edge = maybe_unused_edge_array;
  if (total_edges > MAX_EDGES) {
    maybe_unused_edge_deleter.reset(new PII[total_edges]);
    maybe_unused_edge = maybe_unused_edge_deleter.get();
  }
  
  initial_remaining_options.resize(parent->get_num_punters(), g.num_mines);
  for (size_t i = 0, rsize = g.rivers.size(); i < rsize; ++i) {
    for (size_t j = 0; j < g.rivers[i].size(); ++j) {
      const auto& r = g.rivers[i][j];
      if ((int)(i) >= r.to) continue;
      if (option_enabled) {
	if (r.option != -1) initial_remaining_options[r.option] -= 1;
	if (r.punter != -1 && r.option != -1) continue;
      } else {
	if (r.punter != -1) continue;
      }
      maybe_unused_edge[num_maybe_unused_edge] = PII(i, j);
      ++num_maybe_unused_edge;
    }
  }

  random_shuffle(maybe_unused_edge, maybe_unused_edge + num_maybe_unused_edge);
}

vector<int> MCTS_Core::run_simulation(Node *p_root, const vector<int> &futures) {
  /* remaining_turns */
  const int total_edges = parent->get_graph().num_edges;
  int remaining_turns = total_edges - (int)parent->get_history().size();
  const bool option_enabled = parent->get_options_enabled();

  Node *cur_node = p_root;

  Graph& cur_state = *parent->mutable_graph();

  set<int> visited;
  bool expanded = false;
  int cur_player = parent->get_punter_id();

  vector<Node*> visited_nodes;
  visited_nodes.push_back(cur_node);


  vector<int> remaining_options = initial_remaining_options;

  int num_turn = 0;

  while(--remaining_turns >= 0) {
    ++num_turn;
    int next_player = (cur_player + 1) % parent->get_num_punters();

    const double inf = 1e20;
    double current_uct = -1;
    move_t current_move;

    for (int mue_idx = 0; mue_idx < num_maybe_unused_edge; ++mue_idx) {
      const auto& ue = maybe_unused_edge[mue_idx];
      const auto& r = cur_state.rivers[ue.first][ue.second];

      if (option_enabled) {
	if (r.punter != -1) {
	  if (r.option != -1) continue;
	  if (remaining_options[cur_player] <= 0) continue;
	  if (r.punter == cur_player) continue;
	}
      } else {
	if (r.punter != -1) continue;
      }

      move_t move(ue.first, r.to);
      double uct;
      auto it = cur_node->children.find(move);
      if (it != cur_node->children.end()) {
	Node *c = it->second.get();
	uct = c->payoffs[cur_player] * 1.0 / c->n_plays / parent->get_num_punters() / max_score + 
	  sqrt(2.0 * log_memo[std::min(cur_node->n_plays, MAX_LOG - 1)] / c->n_plays);
      } else {
	uct = inf;
      }
      if (current_uct < uct) {
	current_move = move;
	current_uct = uct;
      }
    }

    move_t move = current_move;

    if (move.first == inf) {
      const set<int> visited_sites = get_visited_sites(parent, visited_nodes, next_player);
      move_t greedy_move = get_next_greedy(parent, cur_state, visited_sites, next_player);
      if (greedy_move.first != -1 && greedy_move.second != -1) {
	move = greedy_move;
      }
    }

    if (!expanded && cur_node->children.count(move) == 0) {
      /* expand node */
      expanded = true;
      cur_node->children[move] = unique_ptr<Node>(new Node(parent->get_num_punters(), cur_player, move));
    }
    apply_move(cur_state, move, cur_player, remaining_options);

    auto it = cur_node->children.find(move);
    if (it != cur_node->children.end()) {
      cur_node = it->second.get();
      visited_nodes.push_back(cur_node);
    }

    cur_player = next_player;
    if (!expanded) {
      continue;
    }

    for (int mue_idx = 0; mue_idx < num_maybe_unused_edge; ++mue_idx) {
      const auto& ue = maybe_unused_edge[mue_idx];
      const auto& r = cur_state.rivers[ue.first][ue.second];
      int punter_id = rand() % parent->get_num_punters();
      if (option_enabled) {
	if (r.punter != -1) {
	  if (r.option != -1) continue;
	  if (remaining_options[punter_id] <= 0) continue;
	  if (r.punter == cur_player) continue;
	}
      } else {
	if (r.punter != -1) continue;
      }

      move_t move(ue.first, r.to);
      apply_move(cur_state, move, punter_id, remaining_options);
    }
    break;
  }

  /* determine expected payoff of this playout */
  vector<int> payoffs(parent->get_num_punters());
  int64_t future_score; /* dummy; assigned by the following call */
  vector<int64_t> scores = cur_state.evaluate(parent->get_num_punters(), parent->get_shortest_distances(), parent->get_punter_id(), futures, future_score);

  for(int i=0; i<(int)scores.size(); i++) {
    payoffs[i] = scores[i];
    max_score = max(max_score, scores[i] * 1.0);
  }

  /* back propagate */
  for(auto p : visited_nodes) {
    p->n_plays += 1;
    for(int i=0; i<(int)parent->get_num_punters(); i++) p->payoffs[i] += payoffs[i];
  }


  /* rollback graph */
  rollback_graph(&cur_state);

  if (future_score < - scores[parent->get_punter_id()] * 0.1) {
    payoffs[parent->get_punter_id()] = -10;
  }

  return payoffs;
}

void MCTS_Core::run_futures_selection(vector<int> &futures, int target) {
  /* change futures[target] and select best */
  /* regard the 1st level of tree as "dummy step", which selects futures[target] */
  //	futures[target] = ;

  Node *cur_node = &root;
  int cur_player = parent->get_punter_id();
  int num_mines = parent->get_graph().num_mines;
  int num_vertices = parent->get_graph().num_vertices;
  /* get next legal moves */
  vector<pair<double, move_t>> legal_moves;
  const double inf = 1e20;
  for(int i=num_mines; i<num_vertices+1; i++) { /* do not select mine to mine */
    move_t move(target, i == num_vertices ? -1 : i); /* bet on (target -> i), where -1 means 'do not connect target to anywhere' */
    double uct;
    if (cur_node->children.count(move)) {
      Node *c = cur_node->children[move].get();
      uct = c->payoffs[cur_player] * 1.0 / c->n_plays / parent->get_num_punters() + sqrt(2.0 * log(cur_node->n_plays * 1.0) / c->n_plays);
    } else {
      uct = inf;
    }
    legal_moves.emplace_back(uct, move);
  }
  /* tie break when the UCT value is equal */
  sort(legal_moves.rbegin(), legal_moves.rend());
  int n_candidates = 0;
  for(n_candidates = 0; n_candidates < (int)legal_moves.size(); n_candidates++) {
    if (n_candidates && legal_moves[n_candidates-1].first != legal_moves[n_candidates].first) break;
  }
  assert(n_candidates <= (int)legal_moves.size());
  move_t move = legal_moves[rand() % n_candidates].second;

  /* expand node */
  if (cur_node->children.count(move) == 0) {
    cur_node->children[move] = unique_ptr<Node>(new Node(parent->get_num_punters(), cur_player, move));
  }
  Node *child = cur_node->children[move].get();

  /* apply move */
  if (move.second == -1) { /* do not use futures[target] */
    futures[target] = -1;
  } else { /* weger (target -> move.second) !! */
    futures[target] = move.second;
  }

  vector<int> payoffs = run_simulation(child, futures);
  for(int i=0; i<(int)payoffs.size(); i++) {
    child->n_plays += 1;
    child->payoffs[i] += payoffs[i];
  }
  /*
    if (payoffs[cur_player] < 1.0) {
    child->payoffs[cur_player] -= 1e10;
    }
  */
  cur_node->n_plays += 1;
}

vector<int> MCTS_Core::get_futures(int timelimit_ms) {
  auto start_time = chrono::system_clock::now();

  backup_graph();
  calc_maybe_unused_edge();
  calc_connected_mine();

  int num_mines = parent->get_graph().num_mines;
  vector<int> futures(num_mines, -1);
  vector<int> perm(num_mines);
  for(int i=0; i<num_mines; i++) perm[i] = i;
  random_shuffle(perm.begin(), perm.end());
  /* fill out futures[perm[0]], futures[perm[1]]... */

  for(int i=0; i<num_mines; i++) {
    run_futures_selection(futures, perm[i]);
    int j = perm[i];

    this->reset_root(); /* in fact, you should reuse some node... */
    while(true) {
      run_futures_selection(futures, j);

      auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start_time).count();
      if (elapsed_time >= timelimit_ms * 1.0 / num_mines * (i+1)) break;
    }

    vector<tuple<double, int, int>> candidates;
    cerr << "----" << endl;
    for(const auto &p : root.children) {
      Node *child = p.second.get();
      double e_payoff = child->payoffs[parent->get_punter_id()] * 1.0 / max(child->n_plays, 1);
      cerr << child->from << " -> " << child->to << " : E[payoff] = " << e_payoff << " (played " << child->n_plays << " times)"<< endl;
      candidates.emplace_back(e_payoff, child->from, child->to);
    }
    sort(candidates.rbegin(), candidates.rend());
    auto scores = parent->get_graph().evaluate(parent->get_num_punters(), parent->get_shortest_distances());
    assert(get<1>(candidates[0]) == j);

    double e_payoff = get<0>(candidates[0]);
    if (e_payoff < 1.0) {
      futures[j] = -1; /* bad case */
    } else {
      futures[j] = get<2>(candidates[0]);
    }

    cerr << "FUTURES: " << j << " -> " << futures[j] << endl;

    if (futures[j] != -1) break; /* TODO: ONLY USE ONE FUTURES */

    //		root.children
  }

  return futures;
}
