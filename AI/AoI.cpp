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
  SetupSettings setup() const override;
  tuple<int, int, Json::Value> move() const override;
  std::string name() const override;
  int getNextV(int v, const set<int>& selected_vertices,
               const std::vector<int>& degs) const;
};

std::string AI::name() const {
  return "AoI";
}

SetupSettings AI::setup() const {
  Json::Value degrees_;
  for (int i = 0; i < graph.num_vertices; i++) {
    degrees_.append((int)graph.rivers[i].size());
  }

  Json::Value vertices_;
  vertices_[0] = -1;
  Json::Value ret_;
  ret_[0] = degrees_;
  ret_[1] = vertices_;
  return ret_;
}

static tuple<int,int, Json::Value> valueWithDeg(int a, int b, int nextV, const std::vector<int>& degs, Json::Value vertices_) {
  Json::Value degs_;
  for (int deg : degs) {
    degs_.append(deg);
  }
  if (nextV != -1) {
    vertices_.append(nextV);
  }
  Json::Value ret_;
  ret_[0] = degs_;
  ret_[1] = vertices_;
  return make_tuple(a,b, ret_);
}

int AI::getNextV(int selectV, const set<int>& selected_vertices,
                 const std::vector<int>& degs) const {
  int nextV = -1;
  int maxd = 0;
  int next_mineV = -1;
  int max_mined = 0;
  for (const auto& e : graph.rivers[selectV]) {
    if (e.punter != -1) {
      continue;
    }
    int v = e.to;
    if (selected_vertices.find(v) != selected_vertices.end()) {
      continue;
    }
    if (maxd < degs[v]) {
      nextV = v;
      maxd = degs[v];
    }
    if (v < graph.num_mines && max_mined < degs[v]) {
      next_mineV = v;
      max_mined = degs[v];
    }
  }
  if (next_mineV != -1) {
    return next_mineV;
  } else {
    return nextV;
  }
}

tuple<int,int, Json::Value> AI::move() const {
  // transofrm Json::Value to two vectors
  Json::Value degs_ = info[0];
  Json::Value vertices_ = info[1];

  map <int,int> erased_edges;
  set<int> selected;
  for (int i = 1; i < (int) vertices_.size(); i++) {
    int v = vertices_[i].asInt();
    selected.insert(v);
  }
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
  // the lastly seclected vertex
  int cv = vertices_[vertices_.size()-1].asInt();
  int nextV = -1;
  int a = -1;
  int b = -1;
  if (cv == -1) { // first move
    // select mine
    for (const auto& dv : sorted_degs) {
      int v = dv.second;
      if (v < graph.num_mines) {
        nextV = getNextV(v, selected, degs);
        if (nextV != -1) {
          vertices_.append(v);
          a = v;
          b = nextV;
          break;
        }
      }
    }
  } else {
    int v = vertices_[vertices_.size() - 1].asInt();
    nextV = getNextV(v, selected, degs);
    if (nextV != -1) {
      a = v;
      b = nextV;
    }
    if (nextV == -1) {
      for (int i = 1; i < (int) vertices_.size(); i++) {
        int v = vertices_[i].asInt();
        nextV = getNextV(v, selected, degs);
        if (nextV != -1) {
          a = v;
          b = nextV;
          break;
        }
      }
    }
    // there is no vertex that can be picked.
    // Then, jump another vertex.
    if (nextV == -1) {
      for (const auto& dv : sorted_degs) {
        auto d = dv.first;
        auto v = dv.second;
        if (v < graph.num_vertices && d != 0) {
          nextV = getNextV(v, selected, degs);
          if (nextV != -1) {
            a = v;
            b = nextV;
          }
          break;
        }
      }
    }
    
    if (nextV == -1) {
      // cannot pick up mines
      for (const auto& dv : sorted_degs) {
        auto d = dv.first;
        auto v = dv.second;
        if (d != 0) {
          nextV = getNextV(v, selected, degs);
          if (nextV != -1) {
            a = v;
            b = nextV;
          }
          break;
        }
      }
    }
  }
  return valueWithDeg(a, b, nextV, degs, vertices_);
}

int main()
{
  AI ai;
  ai.run();
  return 0;
}
