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

using namespace std;



class MCTS_AI : public Game {
	Json::Value setup() const override;
	tuple<int, int, Json::Value> move() const override;
};


Json::Value MCTS_AI::setup() const {
	return Json::Value();
}

tuple<int, int, Json::Value> MCTS_AI::move() const {
	MCTS_Core core(*this);
	int timelimit_ms = 20;
	auto p = core.get_play(timelimit_ms);
	return make_tuple(p.first, p.second, Json::Value());
}

int main()
{
	MCTS_AI ai;
	ai.run();
	return 0;
}
