#include "MCTS_core.h"

#include <algorithm>
#include <iostream>
#include <chrono>
#include <set>
#include <cassert>
#include <cmath>

namespace {

	void apply_move(Graph &cur_state, move_t move, int cur_player) {
		/* apply "move" to "cur_state" */
		for(auto& r : cur_state.rivers[move.first]) {
			if (r.to == move.second) {
				assert(r.punter == -1);
				r.punter = cur_player;
			}
		}
		for(auto& r : cur_state.rivers[move.second]) {
			if (r.to == move.first) {
				assert(r.punter == -1);
				r.punter = cur_player;
			}
		}
	}

  set<int> get_visited_sites(const Game &game, const vector<Node*> visited_nodes, int punter) {
    set<int> vertices;
    const History &history = game.get_history();
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
  
  move_t get_next_greedy(const Game &game, Graph &graph, const set<int> &visited_sites, int punter) {
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

        if (v_visited && nv_visited) {
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
        const auto& next_point = graph.evaluate(game.get_num_punters(), game.get_shortest_distances())[punter];
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

pair<int, int> MCTS_Core::get_play(int timelimit_ms) {
	auto start_time = chrono::system_clock::now();
	int n_simulated = 0;
	for (;;) {
		run_simulation();
		n_simulated += 1;
		auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start_time).count();
		if (elapsed_time >= timelimit_ms) break;
	}

	vector<tuple<double, int, int>> candidates;
	cerr << "----" << endl;
	for(const auto &p : root.children) {
		Node *child = &(*(p.second));
		double e_payoff = child->payoffs[parent.get_punter_id()] * 1.0 / max(child->n_plays, 1);
		cerr << child->from << " -> " << child->to << " : E[payoff] = " << e_payoff << " (played " << child->n_plays << " times)"<< endl;
		candidates.emplace_back(e_payoff, child->from, child->to);
	}
	sort(candidates.rbegin(), candidates.rend());
	auto scores = parent.get_graph().evaluate(parent.get_num_punters(), parent.get_shortest_distances());
	cerr << "Punter: " << parent.get_punter_id() << endl;
	for(auto p : scores) {
		cerr << p << " ";
	}
	cerr << endl;

	auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start_time).count();
	cerr << "Elapsed time: " << elapsed_time << " msec" << endl;
	cerr << "Simulated " << n_simulated << " times" << endl;

	return make_pair(get<1>(candidates[0]), get<2>(candidates[0]));
}


void MCTS_Core::run_simulation() {
	/* remaining_turns */
	int total_edges = 0;
	for (int i = 0; i < (int)parent.get_graph().rivers.size(); ++i) {
		for (const auto& r : parent.get_graph().rivers[i]) {
			if (i < r.to) total_edges += 1;
		}
	}
	int remaining_turns = total_edges - (int)parent.get_history().size();

	Node *cur_node = &root;

	Graph cur_state = parent.get_graph();

	set<int> visited;
	bool expanded = false;
	int cur_player = parent.get_punter_id();

	vector<Node*> visited_nodes;
	visited_nodes.push_back(cur_node);
	while(--remaining_turns >= 0) {
		int next_player = (cur_player + 1) % parent.get_num_punters();

		/* get next legal moves */
		vector<pair<double, move_t>> legal_moves;
		const double inf = 1e20;
		for(int i=0; i<(int)cur_state.rivers.size(); i++) {
			for(const auto& r : cur_state.rivers[i]) {
				if (r.punter == -1 && i < r.to) {
					move_t move(i, r.to);
					double uct;
					if (cur_node->children.count(move)) {
					  Node *c = cur_node->children[move].get();
					  uct = c->payoffs[cur_player] * 1.0 / c->n_plays / parent.get_num_punters() + sqrt(2.0 * log(cur_node->n_plays * 1.0) / c->n_plays);
					} else {
						uct = inf;
					}
					legal_moves.emplace_back(uct, move);
				}
			}
		}
		/* tie break when the UCT value is equal */
		sort(legal_moves.rbegin(), legal_moves.rend());
		int n_candidates = 0;
		for(n_candidates = 0; n_candidates < (int)legal_moves.size(); n_candidates++) {
			if (n_candidates && legal_moves[n_candidates-1].first != legal_moves[n_candidates].first) break;
		}
		assert(n_candidates <= (int)legal_moves.size());
		move_t move = legal_moves[rand() % n_candidates].second;

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
			cur_node->children[move] = unique_ptr<Node>(new Node(parent.get_num_punters(), cur_player, move));
		}
		apply_move(cur_state, move, cur_player);

		if (cur_node->children.count(move)) {
			cur_node = cur_node->children[move].get();
			visited_nodes.push_back(cur_node);
		}

		cur_player = next_player;
	}

	/* determine expected payoff of this playout */
	vector<int> payoffs(parent.get_num_punters());
	vector<int64_t> scores = cur_state.evaluate(parent.get_num_punters(), parent.get_shortest_distances());
	vector<pair<int64_t, int>> scores2;
	for(int i=0; i<(int)scores.size(); i++) {
		scores2.emplace_back(scores[i], i);
	}
	sort(scores2.rbegin(), scores2.rend());
	for(int i=0; i<(int)scores2.size();) {
		int j = i;
		while(j < (int)scores2.size()) {
			payoffs[scores2[j].second] = parent.get_num_punters() - i;
			j++;
			if (scores2[j-1].first != scores2[j].first) break;
		}
		i = j;
	}


	/* back propagate */
	for(auto p : visited_nodes) {
		p->n_plays += 1;
		for(int i=0; i<(int)parent.get_num_punters(); i++) p->payoffs[i] += payoffs[i];
	}
}

