#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <utility>
#include <string>
#include <cstring>
#include <queue>
#include <cmath>
#include <iomanip>
#include <cassert>
#include <numeric>
#include <chrono>

#include "../lib/Game.h"


using namespace std;

typedef pair<int, int> move_t;
struct Node {
	int n_plays, n_wins; /* number of plays, wins */
	int from, to; /* river */
	int cur_player; /* who's turn? */
	
	map<move_t, unique_ptr<Node>> children;

	Node(int cur_player, move_t move) : n_plays(0), n_wins(0), from(move.first), to(move.second), cur_player(cur_player) { };
};


class MCTS_AI : public Game {
	Json::Value setup() const override;
	tuple<int, int, Json::Value> move() const override;

	struct MCTS_Core {
		const MCTS_AI &parent;
		MCTS_Core(const MCTS_AI &parent) : parent(parent), root(-1, make_pair(-1, -1)) {}
		void run_simulation();
		pair<int, int> get_play();
		Node root;
	};
};


Json::Value MCTS_AI::setup() const {
	return Json::Value();
}

pair<int, int> MCTS_AI::MCTS_Core::get_play() {
	auto start_time = chrono::system_clock::now();
	for(int t=0; t<1000; t++) {
		run_simulation();
	}
	vector<tuple<double, int, int>> candidates;
	cerr << "----" << endl;
	for(const auto &p : root.children) {
		Node *child = &(*(p.second));
		double win_prob = child->n_wins * 1.0 / (child->n_plays || 1);
		cerr << child->from << " -> " << child->to << " : " << child->n_wins << "/" << child->n_plays << endl;
		candidates.emplace_back(win_prob, child->from, child->to);
	}
	sort(candidates.rbegin(), candidates.rend());
	auto scores = parent.graph.evaluate(parent.num_punters);
	cerr << "Punter: " << parent.punter_id << endl;
	for(auto p : scores) {
		cerr << p << " ";
	}
	cerr << endl;

	auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start_time).count();
	cerr << "Elapsed time: " << elapsed_time << " msec" << endl;
	return make_pair(get<1>(candidates[0]), get<2>(candidates[0]));
}


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

void MCTS_AI::MCTS_Core::run_simulation() {
	/* remaining_turns */
	int total_edges = 0;
	for (int i = 0; i < (int)parent.graph.rivers.size(); ++i) {
		for (const auto& r : parent.graph.rivers[i]) {
			if (i < r.to) total_edges += 1;
		}
	}
	int remaining_turns = total_edges - (int)parent.history.size();

	Node *cur_node = &root;

	Graph cur_state = parent.graph;

	set<int> visited;
	bool expanded = false;
	int cur_player = parent.punter_id;

	vector<Node*> visited_nodes;
	visited_nodes.push_back(cur_node);
	while(--remaining_turns >= 0) {
		int next_player = (cur_player + 1) % parent.num_punters;

		/* get next legal moves */
		vector<pair<double, move_t>> legal_moves;
		const double inf = 1e20;
		for(int i=0; i<(int)cur_state.rivers.size(); i++) {
			for(const auto& r : cur_state.rivers[i]) {
				if (r.punter == -1 && i < r.to) {
					move_t move(i, r.to);
					double uct;
					if (cur_node->children.count(move)) {
						Node *c = &*(cur_node->children[move]);
						uct = c->n_wins * 1.0 / c->n_plays + sqrt(2.0 * log(cur_node->n_plays * 1.0) / c->n_plays);
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
		//move_t move = legal_moves[rand() % legal_moves.size()].second;

		if (!expanded && cur_node->children.count(move) == 0) {
			/* expand node */
			expanded = true;
			cur_node->children[move] = unique_ptr<Node>(new Node(cur_player, move));
		}
		apply_move(cur_state, move, cur_player);

		if (cur_node->children.count(move)) {
			cur_node = &(*cur_node->children[move]);
			visited_nodes.push_back(cur_node);
		}

		cur_player = next_player;
	}

	/* determine winner of this playout */
	set<int> winners;
	vector<int64_t> scores = cur_state.evaluate(parent.num_punters);
	vector<pair<int64_t, int>> scores2;
	for(int i=0; i<(int)scores.size(); i++) {
		scores2.emplace_back(scores[i], i);
	}
	sort(scores2.rbegin(), scores2.rend());
	for(int i=0; i<(int)scores2.size(); i++) {
		if (i>0 && scores2[i-1].first != scores2[i].first) break; /* include all ties */
		winners.insert(scores2[i].second);
	}


	/* back propagate */
	for(auto p : visited_nodes) {
		p->n_plays += 1;
		if (winners.count(p->cur_player)) p->n_wins += 1;
	}
}

tuple<int, int, Json::Value> MCTS_AI::move() const {
	MCTS_Core core(*this);
	auto p = core.get_play();
	return make_tuple(p.first, p.second, Json::Value());
}

int main()
{
	MCTS_AI ai;
	ai.run();
	return 0;
}
