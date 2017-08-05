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

#include "../lib/Game.h"

using namespace std;
class AI : public Game {
  Json::Value setup() const override;
  tuple<int, int, Json::Value> move() const override;
  std::string name() const override;
};

std::string AI::name() const {
  return "AoI";
}

Json::Value AI::setup() const {
  Json::Value degrees_;
  for (int i = 0; i < graph.num_vertices; i++) {
    degrees_.append((int)graph.rivers[i].size());
  }
  Json::Value mypath_;
  mypath_[0][0] = -1;
  mypath_[0][1] = -1;
  Json::Value ret_;
  ret_[0] = degrees_;
  ret_[1] = mypath_;
  return ret_;
}

static tuple<int,int, Json::Value> valueWithDeg(int a, int b, const std::vector<int>& degs, Json::Value mypath_) {
  Json::Value degs_;
  for (const auto &deg : degs) {
    degs_.append(deg);
  }
  if (a != -1 && b != -1) {
    Json::Value val_;
    val_[0] = a;
    val_[1] = b;
    mypath_.append(val_);
  }
  Json::Value ret_;
  ret_[0] = degs_;
  ret_[1] = mypath_;
  return make_tuple(a,b, ret_);
}

tuple<int,int, Json::Value> AI::move() const {
  // transofrm Json::Value to two vectors
  Json::Value degs_ = info[0];
  Json::Value mypath_ = info[1];

  map <int,int> erased_edges;
  for (auto it = history.rbegin(); it != history.rend(); it++) {
    const auto& move = *it;
    if (!move.is_pass()) {
      auto src = move.src;
      auto to = move.to;
      erased_edges[src]++;
      erased_edges[to]++;
    }
    if (move.punter == punter_id) {
      break;
    }
  }
  vector <int> degs(graph.num_vertices);
  vector<pair<int, int>> sorted_degs;
  for (int i = 0; i < graph.num_vertices; i++) {
    int d = degs_[i].asInt();
    d -= erased_edges[i];
    sorted_degs.emplace_back(d, i);
    degs[i] = d;
  }
  sort(sorted_degs.begin(), sorted_degs.end());
  reverse(sorted_degs.begin(), sorted_degs.end());
  // the lastly seclected edge
  int src = mypath_[mypath_.size()-1][0].asInt();
  int to = mypath_[mypath_.size()-1][1].asInt();
  int selectV = -1;
  if (src == -1 && to == -1) { // first move
    // select mine
    for (const auto& dv : sorted_degs) {
      int v = dv.second;
      if (v < graph.num_mines) {
        selectV = v;
        break;
      }
    }
  } else {
    selectV = to;
    if (degs[selectV] == 0) {
      for (int i = mypath_.size() -1; i >= 1; i--) {
        selectV = mypath_[i][0].asInt();
        if (degs[selectV] != 0) {
          break;
        }
      }
    }
  }
  if (selectV == -1 || degs[selectV] == 0) {
    return valueWithDeg(-1, -1, degs, mypath_);
  }


  int nextV = -1;
  int maxd = 0;
  int next_mineV = -1;
  int max_mind = 0;
  for (const auto& e : graph.rivers[selectV]) {
    int v = e.to;
    if ((maxd < degs[v] && e.punter == -1)) {
      nextV = v;
      maxd = degs[v];
    }
    if (e.punter == -1 && v < graph.num_mines && max_mind < degs[v]) {
      next_mineV = v;
      max_mind = degs[v];
    }
  }
  if (next_mineV != -1) {
    nextV = next_mineV;
  }
  if (nextV == -1) {
    return valueWithDeg(-1, -1, degs, mypath_);
  }
  return valueWithDeg(selectV, nextV, degs, mypath_);
}

int main()
{
  AI ai;
  ai.run();
  return 0;
}
