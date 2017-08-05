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

#ifdef HAVE_CPU_PROFILER
// $ apt install libgoogle-perftools-dev
// $ make LIBPROFILER='-lprofiler'
// $ ../bin/MCTS # execute binary
// $ google-pprof --svg ../bin/MCTS prof.out > prof.svg
#include <gperftools/profiler.h>
#endif


using namespace std;


SetupSettings MCTS_AI::setup() const {
	return Json::Value();
}

tuple<int, int, Json::Value> MCTS_AI::move() const {
  MCTS_AI g = *this;
	MCTS_Core core(&g);
	int timelimit_ms = 200;
	auto p = core.get_play(timelimit_ms);
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
