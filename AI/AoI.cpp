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
class AI : public Game {
  Json::Value setup() const override;
  tuple<int, int, Json::Value> move() const override;
};

Json::Value AI::setup() const {
  //cerr << "setup" << endl;
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
  //cerr << "setup is finished" << endl;
  return ret_;
}

static tuple<int,int, Json::Value> valueWithDeg(int a, int b, const std::vector<int>& degs, Json::Value mypath_) {
  Json::Value degs_;
  for (int i = 0; i< degs.size(); i++) {
    degs_.append(degs[i]);
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
  //cerr << "move" << endl;
  // transofrm Json::Value to two vectors
  Json::Value degs_ = info[0];
  Json::Value mypath_ = info[1];
  
  map <int,int> erased_edges;
  for (const auto& move : history) {
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
  }
  if (selectV == -1 || degs[selectV] == 0) {
    return valueWithDeg(-1, -1, degs, mypath_);
  }
  int nextV = -1;
  int maxd = 0;
  for (const auto& e : graph.rivers[selectV]) {
    int v = e.to;
    if (maxd < degs[v] && e.punter == -1) {
      nextV = v;
      maxd = degs[v];
    }
  }
  if (nextV == -1) {
    return valueWithDeg(-1, -1, degs, mypath_);
  }
  //  cerr << "move is end" << endl;
  return valueWithDeg(selectV, nextV, degs, mypath_);
}

int main()
{
  AI ai;
  ai.run();
  return 0;
}
