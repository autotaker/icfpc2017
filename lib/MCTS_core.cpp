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

pair<int, int> MCTS_Core::get_play(int timelimit_ms) {
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
		Node *child = &(*(p.second));
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


vector<int> MCTS_Core::run_simulation(Node *p_root, const vector<int> &futures) {
	/* remaining_turns */
	int total_edges = 0;
	for (int i = 0; i < (int)parent->get_graph().rivers.size(); ++i) {
		for (const auto& r : parent->get_graph().rivers[i]) {
			if (i < r.to) total_edges += 1;
		}
	}
	int remaining_turns = total_edges - (int)parent->get_history().size();

	Node *cur_node = p_root;

	Graph& cur_state = *parent->mutable_graph();

        const int MAX_EDGES = 1e5;
	std::unique_ptr<int[]> punter_back_deleter;
        int punter_back_array[MAX_EDGES];
        int* punter_back = punter_back_array;
        if (total_edges * 2 > MAX_EDGES) {
          punter_back_deleter.reset(new int[total_edges * 2]);
          punter_back = punter_back_deleter.get();
        }

        {
          int* p = punter_back;
          for (const auto& river : cur_state.rivers) {
            for (const auto& r : river) {
              *p = r.punter;
              ++p;
            }
          }
        }


	set<int> visited;
	bool expanded = false;
	int cur_player = parent->get_punter_id();

	vector<Node*> visited_nodes;
	visited_nodes.push_back(cur_node);

	vector<pair<double, move_t>> legal_moves;

	while(--remaining_turns >= 0) {
		int next_player = (cur_player + 1) % parent->get_num_punters();

		/* get next legal moves */
                legal_moves.clear();
		const double inf = 1e20;
		for(int i=0; i<(int)cur_state.rivers.size(); i++) {
			for(const auto& r : cur_state.rivers[i]) {
				if (r.punter == -1 && i < r.to) {
					move_t move(i, r.to);
					double uct;
                                        auto it = cur_node->children.find(move);
					if (it != cur_node->children.end()) {
					  Node *c = it->second.get();
					  uct = c->payoffs[cur_player] * 1.0 / c->n_plays / parent->get_num_punters() + sqrt(2.0 * log(cur_node->n_plays * 1.0) / c->n_plays);
					} else {
						uct = inf;
					}
                                        if (!legal_moves.empty() && legal_moves[0].first < uct) {
                                          legal_moves.clear();
                                        }
					legal_moves.emplace_back(uct, move);
				}
			}
		}

		move_t move = legal_moves[rand() % legal_moves.size()].second;

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
		apply_move(cur_state, move, cur_player);

                auto it = cur_node->children.find(move);
		if (it != cur_node->children.end()) {
		  cur_node = it->second.get();
			visited_nodes.push_back(cur_node);
		}

		cur_player = next_player;
	}

	/* determine expected payoff of this playout */
	vector<int> payoffs(parent->get_num_punters());
	int64_t future_score; /* dummy; assigned by the following call */
	vector<int64_t> scores = cur_state.evaluate(parent->get_num_punters(), parent->get_shortest_distances(), parent->get_punter_id(), futures, future_score);

	vector<pair<int64_t, int>> scores2;
	for(int i=0; i<(int)scores.size(); i++) {
		scores2.emplace_back(scores[i], i);
	}
	sort(scores2.rbegin(), scores2.rend());
	for(int i=0; i<(int)scores2.size();) {
		int j = i;
		while(j < (int)scores2.size()) {
			payoffs[scores2[j].second] = parent->get_num_punters() - i;
			j++;
			if (scores2[j-1].first != scores2[j].first) break;
		}
		i = j;
	}


	/* back propagate */
	for(auto p : visited_nodes) {
		p->n_plays += 1;
		for(int i=0; i<(int)parent->get_num_punters(); i++) p->payoffs[i] += payoffs[i];
	}


        /* rollback graph */
        {
          int* p = punter_back;
          for (auto& river : cur_state.rivers) {
            for (auto& r : river) {
              r.punter = *p;
              ++p;
            }
          }
        }

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
