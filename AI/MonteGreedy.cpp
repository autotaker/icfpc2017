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

class MonteGreedy : public Game {
	SetupSettings setup() const override;
	MoveResult move() const override;
	
	std::string name() const override;
};

using namespace std;

typedef pair<int, int> move_t;


const int TIMELIMIT_MS_FOR_SETUP = 9500;
const int TIMELIMIT_MS_FOR_MOVE = 950;

SetupSettings MonteGreedy::setup() const {
	vector<int> futures(this->graph.num_mines, -1);
	return SetupSettings(Json::Value(), futures);
}

struct Choice {
	int n_simulated;
	move_t move;
	double uct;
	double sum_score;
	Choice(const move_t &move) : n_simulated(0), move(move), uct(1e20), sum_score(0.0) {}

	bool operator <(const Choice &c) const {
		return uct < c.uct;
	}
	void update(double score, int n_simulated_total, double max_score) {
		sum_score += score;
		n_simulated += 1;
		uct = sum_score / n_simulated / max_score + sqrt(2.0 * log(n_simulated_total * 1.0) / n_simulated);
	}
};

MoveResult MonteGreedy::move() const {
	vector<vector<int>> adjm(graph.num_vertices, vector<int>(graph.num_vertices, 0)); /* adjm[i][j] : time_t << 1 | built? */
	priority_queue<Choice> candidates;
	for(int i = 0; i < graph.num_vertices; i++) {
		for(const auto &r : graph.rivers[i]) {
			if (i >= r.to) continue;
			if (r.punter != -1) continue;
			candidates.emplace(Choice(move_t(i, r.to)));
		}
	}

	/*
	uct = c->payoffs[cur_player] * 1.0 / c->n_plays / parent->get_num_punters() / max_score + 
	  sqrt(2.0 * log_memo[std::min(cur_node->n_plays, MAX_LOG - 1)] / c->n_plays);
	  */

	const int LOOP_TIMES = 1000000;
	double max_score = 1;
	int n_simulated_total = 0;
	for(int t=1; t<=LOOP_TIMES; t++) {
		n_simulated_total++;
		Choice choice = candidates.top();
		candidates.pop();
		const move_t &move = choice.move;
		double score = 0;

		queue<int> q;
		vector<bool> visited(graph.num_vertices, false);
		q.push(move.first);
		q.push(move.second);
		visited[move.first] = visited[move.second] = true;
		while(!q.empty()) {
			int i = q.front();
			q.pop();
			for(const auto &r : graph.rivers[i]) {
				if (visited[r.to]) continue;
				if (r.punter != -1) {
					if (r.punter == punter_id) {
					} else {
						continue;
					}
				} else {
					int &x = adjm[i][r.to];
					if ((x >> 1) != t) {
						/* randomly color */
						x = (t<<1) | (rand() * 1.0 / RAND_MAX <= 1.0 / num_punters);
					}
					if (!(x&1)) continue;
				}
				visited[r.to] = true;
				q.push(r.to);
			}
		}
		for(int m = 0; m < this->graph.num_mines; m++) {
			if (visited[m]) {
				score += shortest_distances[m][move.first] * shortest_distances[m][move.first];
				score += shortest_distances[m][move.second] * shortest_distances[m][move.second];
			}
		}

		max_score = max(max_score, score);
		choice.update(score, n_simulated_total, max_score);
		candidates.push(choice);
	}
	cerr << "------" << endl;
	move_t best_move;
	double best = -1;
	while(!candidates.empty()) {
		Choice choice = candidates.top();
		cerr << choice.move.first << "->" << choice.move.second << ": " << choice.uct << " " << choice.n_simulated << " " << choice.sum_score / choice.n_simulated << endl;

		double avg = choice.sum_score / choice.n_simulated;
		if (best < avg) {
			best = avg;
			best_move = choice.move;
		}

		candidates.pop();
	}
	return make_tuple(best_move.first, best_move.second, Json::Value());
}

std::string MonteGreedy::name() const {
  return "MonteGreedy";
}

int main()
{
	MonteGreedy ai;
	ai.run();
	return 0;
}
