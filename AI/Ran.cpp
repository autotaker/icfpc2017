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

struct Data {
  int point;
  int degree;
}

bool tryChange(int* src, int* to, Data* best_data, const Data& current_data) {
  if (current_data.point > best_data->point) {
    *best_data = current_data;
    return true;
  } else if(current_data.point == best_data->point) {
    if (current_data->degree > best_data->degree) {
      *best_data = current_data;
      return true;
    }
  }
  return false;
}

  
tuple<int,int, Json::Value> AI::move() const {
  Data best_data;
  best_data.point = 0;
  best_data.degree = 0;
  
  int src = -1, to = -1;
  Graph myown_graph = graph;

  for (int v = 0; v < (int) myown_graph.num_vertices; v++) {
    for (auto& r :  myown_graph.rivers[v]) {
      auto nv = r.to;
      if (v > nv) {
        continue;
      }
      if (r.punter == -1) {
        Graph::River* nrit = nullptr;
        for (auto &nr : myown_graph.rivers[nv]) {
          if (nr.to == v) {
            nrit = &nr;
            break;
          }
        }
        assert(nrit != nullptr);
        r.punter = punter_id;
        nrit->punter = punter_id;
        auto next_point = myown_graph.evaluate(num_punters)[punter_id];
        Data current_data;
        current_data.point = next_point;
        current_data.degree = myown_graph.rivers[nv].size();
        if (shouldChange(&best_data, current_data)) {
          src = v;
          to = nv;
        }
        r.punter = -1;
        nrit->punter = -1;
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

