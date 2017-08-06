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
#include "../lib/MCTS_core.h"
#include "MCTS_AI.h"

using namespace std;

typedef pair<int, int> move_t;



const int TIMELIMIT_MS_FOR_SETUP = 9500;
const int TIMELIMIT_MS_FOR_MOVE = 950;

SetupSettings MCTS_AI::setup() const {
	MCTS_AI g = *this;
	MCTS_Core core(&g);
	vector<int> futures = core.get_futures(TIMELIMIT_MS_FOR_SETUP);
	for(auto p : futures) cerr << p << " "; cerr << endl;
	return SetupSettings(Json::Value(), futures);
//	return SetupSettings(Json::Value(), futures);
}

MoveResult MCTS_AI::move() const {
  MCTS_AI g = *this;
  MCTS_Core core(&g);
	auto p = core.get_play(TIMELIMIT_MS_FOR_MOVE);
	return make_tuple(p.first, p.second, Json::Value());
}

std::string MCTS_AI::name() const {
  return "MCTS";
}

int main()
{
	MCTS_AI ai;
	ai.run();
	return 0;
}
