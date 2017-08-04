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

#include "../lib/Game.h"


using namespace std;


class MCTS : public Game {
	Json::Value setup() const override;
	tuple<int, int, Json::Value> move() const override;
	void run_simulation();
};


Json::Value MCTS::setup() const {
	return Json::Value();
}

typedef pair<int, int> move_t;
struct Node {
	int n_plays, n_wins; /* number of plays, wins */
	int from, to; /* river */
	int cur_player; /* who's turn? */

	
	map<move_t, unique_ptr<Node>> children;

	Node(int cur_player) : n_plays(0), n_wins(0), from(-1), to(-1), cur_player(cur_player) { };
};


void MCTS::run_simulation() {
	/* remaining_turns */
	int total_edges = 0;
	for (int i = 0; i < (int)graph.rivers.size(); ++i) {
		for (const auto& r : graph.rivers[i]) {
			if (i < r.to) total_edges += 1;
		}
	}
	int remaining_turns = total_edges - (int)this->history.size();

	Node cur_node(-1);

	Graph cur_state = this->graph;

	set<int> visited;
	bool expanded = false;
	int cur_player = this->punter_id;

	vector<Node*> visited_nodes;
	visited_nodes.push_back(&cur_node);
	while(--remaining_turns >= 0) {
		int next_player = (cur_player + 1) % this->num_punters;

		/* get next legal moves */
		vector<move_t> legal_moves;
		for(int i=0; i<(int)cur_state.rivers.size(); i++) {
			for(const auto& r : cur_state.rivers[i]) {
				if (r.punter == -1 && i < r.to) legal_moves.emplace_back(i, r.to);
			}
		}

		move_t move = legal_moves[rand() % legal_moves.size()];

		if (!expanded && cur_node.children.count(move) == 0) {
			/* expand node */
			expanded = true;
			cur_node.children[move] = unique_ptr<Node>(new Node(cur_player));
		}
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

		/*
		if (cur_node.children.count(move)) {
			cur_node = cur_node.children[move]
			visited_nodes.push_back(cur_node.
				[cur_node.children[move]]
		}
		*/

		visited_nodes.push_back(&cur_node);

		cur_player = next_player;
	}
}

tuple<int, int, Json::Value> MCTS::move() const {


	/*
	srand(time(NULL));

	UnionFind uf(graph.num_vertices);
	std::vector<pair<int, int> > no_edge;
	std::vector<pair<int, int> > yes_edge;

	for (size_t i = 0; i < graph.rivers.size(); ++i) {
		for (const auto& r : graph.rivers[i]) {
			if (r.punter == punter_id) {
				uf.unit(i, r.to);
			}
		}
	}

	for (size_t i = 0; i < graph.rivers.size(); ++i) {
		for (const auto& r : graph.rivers[i]) {
			if (r.punter != -1) continue;

			if (uf.find(i) == uf.find(r.to)) {
				no_edge.emplace_back(i, r.to);
			} else {
				yes_edge.emplace_back(i, r.to);
			}
		}
	}

	std::pair<int, int> e;

	if (yes_edge.empty()) {
		e = no_edge[rand() % no_edge.size()];
	} else {
		e = yes_edge[rand() % yes_edge.size()];
	}

	return make_tuple(e.first, e.second, Json::Value());
	*/
	return make_tuple(0, 1, Json::Value());
}

int main()
{
	MCTS ai;
	ai.run();
	return 0;
}
