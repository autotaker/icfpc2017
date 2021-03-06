// Most part is copied from MCTS.cpp

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

namespace MCTS_greedy {

using namespace std;

typedef pair<int, int> move_t;
  
class MCTS_GREEDY_AI : public Game {
public:
	SetupSettings setup() const override;
	MoveResult move() const override;
	Json::Value walkin_setup() const override;
};


SetupSettings MCTS_GREEDY_AI::setup() const {
	return Json::Value();
}

MoveResult MCTS_GREEDY_AI::move() const {
  MCTS_GREEDY_AI g = *this;
	MCTS_Core core(&g, 0.5);
	int timelimit_ms = 950;
	auto p = core.get_play(timelimit_ms);
	return make_tuple(p.first, p.second, Json::Value());
}

Json::Value MCTS_GREEDY_AI::walkin_setup() const {
	return Json::Value();
}

}

#ifndef _USE_AS_ENGINE
int main()
{
	MCTS_greedy::MCTS_GREEDY_AI ai;
	ai.run();
	return 0;
}
#endif
