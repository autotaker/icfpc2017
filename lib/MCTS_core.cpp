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

}

pair<int, int> MCTS_Core::get_play(int timelimit_ms) {
	for(auto a : parent.get_futures()) {
		cerr << a << " ";
	} cerr << endl;

	auto start_time = chrono::system_clock::now();
	int n_simulated = 0;
	for (;;) {
		run_simulation(&root);
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


vector<int> MCTS_Core::run_simulation(Node *p_root) {
	/* remaining_turns */
	int total_edges = 0;
	for (int i = 0; i < (int)parent.get_graph().rivers.size(); ++i) {
		for (const auto& r : parent.get_graph().rivers[i]) {
			if (i < r.to) total_edges += 1;
		}
	}
	int remaining_turns = total_edges - (int)parent.get_history().size();

	Node *cur_node = p_root;

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

	return payoffs;
}

void MCTS_Core::run_futures_selection(vector<int> &futures, int target) {
	/* change futures[target] and select best */
	/* regard the 1st level of tree as "dummy step", which selects futures[target] */
//	futures[target] = ;

	Node *cur_node = &root;
	int cur_player = parent.get_punter_id();
	int num_mines = parent.get_graph().num_mines;
	int num_vertices = parent.get_graph().num_vertices;
	/* get next legal moves */
	vector<pair<double, move_t>> legal_moves;
	const double inf = 1e20;
	for(int i=num_mines; i<num_vertices+1; i++) { /* do not select mine to mine */
		move_t move(target, i == num_vertices ? -1 : i); /* bet on (target -> i), where -1 means 'do not connect target to anywhere' */
		double uct;
		if (cur_node->children.count(move)) {
			Node *c = cur_node->children[move].get();
			uct = c->payoffs[cur_player] * 1.0 / c->n_plays / parent.get_num_punters() + sqrt(2.0 * log(cur_node->n_plays * 1.0) / c->n_plays);
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
		cur_node->children[move] = unique_ptr<Node>(new Node(parent.get_num_punters(), cur_player, move));
	}
	Node *child = cur_node->children[move].get();

	/* apply move */
	if (move.second == -1) { /* do not use futures[target] */
		futures[target] = -1;
	} else { /* weger (target -> move.second) !! */
		futures[target] = move.second;
	}

	vector<int> payoffs = run_simulation(child);
	for(int i=0; i<(int)payoffs.size(); i++) {
		child->n_plays += 1;
		child->payoffs[i] += payoffs[i];
	}
}

vector<int> MCTS_Core::get_futures(int timelimit_ms) {
	auto start_time = chrono::system_clock::now();

	int num_mines = parent.get_graph().num_mines;
	vector<int> futures(num_mines, 6);
	vector<int> perm(num_mines);
	for(int i=0; i<num_mines; i++) perm[i] = i;
	random_shuffle(perm.begin(), perm.end());
	/* fill out futures[perm[0]], futures[perm[1]]... */

	for(int i=0; i<num_mines; i++) {
		run_futures_selection(futures, perm[i]);
		int j = perm[i];

		this->reset_root();
		while(true) {
			run_futures_selection(futures, j);

			auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start_time).count();
			if (elapsed_time >= timelimit_ms * 1.0 / num_mines * (i+1)) break;
		}

//		root.children
	}

	return futures;
}
