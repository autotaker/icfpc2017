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

#include "Game.h"

using namespace std;
class AI : public Game {
  Json::Value setup() const override;
  tuple<int, int, Json::Value> move() const override;
  std::string name() const override;
};

std::string AI::name() const {
  return "Ran";
}

Json::Value AI::setup() const {
  return Json::Value();
}

tuple<int,int, Json::Value> AI::move() const {
  int maxP = 0;
  int src = -1, to = -1;
  Graph myown_graph = graph;
  for (int v = 0; v < (int) myown_graph.num_vertices; v++) {
    for (auto& r :  myown_graph.rivers[v]) {
      auto nv = r.to;
      if (v > nv) {
        continue;
      }
      if (r.punter == -1) {
        r.punter = punter_id;
        auto next_point = myown_graph.evaluate(num_punters)[punter_id];
        if (next_point > maxP) {
          src = v;
          to = nv;
          maxP = next_point;
        }
        r.punter = -1;
      }
    }
  }
  
  if (src == -1 && to == -1) {
    // There is no such edge that my score increases.
    // In this case, I pick up any vertex.
    for (int v = 0; v < (int) myown_graph.num_vertices; v++) {
      for (const auto& r :  myown_graph.rivers[v]) {
        if (r.punter == -1) {
          src = v;
          to = r.to;
          goto out;
        }
      }
    }
  }
 out:
  return make_tuple(src, to, Json::Value());
}

int main()
{
  AI ai;
  ai.run();
  return 0;
}

