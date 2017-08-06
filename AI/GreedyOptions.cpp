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

static int owner(const Graph& g, int src, int to) {
  const auto& rs = g.rivers[src];
  return std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter;
}

static void claim(Graph& g, int src, int to, int p) {
  auto& rs = g.rivers[src];
  auto& rt = g.rivers[to];
  if (owner(g, src, to) == -1) {
    std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter = p;
    std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->punter = p;
  } else {
    std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->option = p;
    std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->option = p;    
  }
}

static void unclaim(Graph& g, int src, int to, int p) {
  auto& rs = g.rivers[src];
  auto& rt = g.rivers[to];
  if (owner(g, src, to) == p) {
    std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter = -1;
    std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->punter = -1;    
  } else {
    std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->option = -1;
    std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->option = -1;    
  }
}

class GreedyOptions : public Game {
  SetupSettings setup() const override;
  MoveResult move() const override;
  std::string name() const override;
};

std::string GreedyOptions::name() const {
  return "GreedyOptions";
}

SetupSettings GreedyOptions::setup() const {
  return Json::Value();
}

MoveResult GreedyOptions::move() const {
  int64_t best_w = -INF;
  int best_u = -1, best_v = -1;

  Graph g = graph;
  for (int u = 0; u < g.num_vertices; ++u) {
    for (const auto& river : g.rivers[u]) {
      if (u < river.to) {
        bool trial = false;
        trial |= river.punter == -1;
        if (options_enabled && options_bought < g.num_mines) {
          trial |= river.punter != punter_id && river.option == -1;
        }
        if (trial) {
          claim(g, u, river.to, punter_id);
          int64_t score = g.evaluate(num_punters, shortest_distances)[punter_id];
          unclaim(g, u, river.to, punter_id);
          if (best_w < score) {
            best_w = score;
            best_u = u;
            best_v = river.to;
          }
        }
      }
    }
  }

  return make_tuple(best_u, best_v, Json::Value());
}

int main()
{
  GreedyOptions ai;
  ai.run();
  return 0;
}
