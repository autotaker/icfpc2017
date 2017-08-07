
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

MoveResult MonteGreedy::move() const {
	vector<vector<int>> adjm(graph.num_vertices, vector<int>(graph.num_vertices, 0)); /* adjm[i][j] : time_t << 1 | built? */
	vector<pair<double, move_t>> candidates;
	for(int i = 0; i < graph.num_vertices; i++) {
		for(const auto &r : graph.rivers[i]) {
			if (i >= r.to) continue;
			if (r.punter != -1) continue;
			candidates.emplace_back(0, make_pair(i, r.to));
		}
	}
	for(auto &p : candidates) {
		move_t move = p.second;

		const int LOOP_TIMES = 10000;
		vector<double> scores(LOOP_TIMES, 0.0);
		for(int t=1; t<=LOOP_TIMES; t++) {
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
			double score = 0.0;
			for(int m = 0; m < this->graph.num_mines; m++) {
				if (visited[m]) {
					score += shortest_distances[m][move.first] * shortest_distances[m][move.first];
					score += shortest_distances[m][move.second] * shortest_distances[m][move.second];
				}
			}
			scores[t] = score;
		}
		sort(scores.begin(), scores.end());
		double sum = 0;
		int num = int(LOOP_TIMES * 0.50);
		for(int i=0; i<num; i++) {
			sum += scores[i];
		}
		sum /= num;

		p.first = sum;
		cerr << move.first << " -> " << move.second << " : " << p.first << endl;
	}
	cerr << "------" << endl;
	sort(candidates.rbegin(), candidates.rend());
	move_t move = candidates[0].second;
	return make_tuple(move.first, move.second, Json::Value());
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
