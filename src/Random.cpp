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

#include "../src/Game.h"

using namespace std;
class RandomAI : public Game {
  Json::Value setup() const override;
  tuple<int, int, Json::Value> move() const override;
};

Json::Value RandomAI::setup() const {
  return Json::Value();
}

tuple<int, int, Json::Value> RandomAI::move() const {
  srand(time(NULL));

  std::vector<pair<int, int> > unused_edge;

  for (size_t i = 0; i < graph.rivers.size(); ++i) {
    for (const auto& r : graph.rivers[i]) {
      if (r.punter != -1) {
	unused_edge.emplace_back(i, r.to);
      }
    }
  }

  auto e = unused_edge[rand() % unused_edge.size()];

  return make_tuple(e.first, e.second, Json::Value());
}

int main()
{
  RandomAI ai;
  ai.run();
  return 0;
}
