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

#ifdef HAVE_CPU_PROFILER
// $ apt install libgoogle-perftools-dev
// $ make LIBPROFILER='-lprofiler'
// $ ../bin/MCTS # execute binary
// $ google-pprof --svg ../bin/MCTS prof.out > prof.svg
#include <gperftools/profiler.h>
#endif

using namespace std;

typedef pair<int, int> move_t;


class MCTS_AI : public Game {
	SetupSettings setup() const override;
	tuple<int, int, Json::Value> move() const override;

};

const int TIMELIMIT_MS = 950;

SetupSettings MCTS_AI::setup() const {
	MCTS_Core core(*this);
	vector<int> futures = core.get_futures(TIMELIMIT_MS);
	for(auto p : futures) cerr << p << " "; cerr << endl;
	return SetupSettings(Json::Value(), futures);
}

tuple<int, int, Json::Value> MCTS_AI::move() const {
	MCTS_Core core(*this);
	auto p = core.get_play(TIMELIMIT_MS);
	return make_tuple(p.first, p.second, Json::Value());
}

int main()
{
#ifdef HAVE_CPU_PROFILER
  ProfilerStart("prof.out");
#endif
	MCTS_AI ai;
	ai.run();

#ifdef HAVE_CPU_PROFILER
	ProfilerStop();
#endif
	return 0;
}
