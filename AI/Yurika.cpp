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

#include "Game.h"
#include "../lib/MCTS_core.h"


using namespace std;

using ll = long long;

struct Data {
  ll point;
  int degree;
};

struct MyMove {
  int src;
  int to;
  ll score;
};

class AI : public Game {
  SetupSettings setup() const override;
  tuple<int, int, Json::Value> move() const override;
  std::string name() const override;
  MyMove getBestData(int pid, const Data& base_data, const vector<int> &in_vertices, Graph& myown_graph) const;
};

std::string AI::name() const {
  return "Yurika";
}

SetupSettings AI::setup() const {
  Json::Value vertices;
  return vertices;// it should be empty first.
}

bool shouldChange(Data* best_data, const Data& current_data) {
  if (current_data.point > best_data->point) {
    *best_data = current_data;
    return true;
  } else if(current_data.point == best_data->point) {
    if (current_data.degree > best_data->degree) {
      *best_data = current_data;
      return true;
    }
  }
  return false;
}

#define INF 1070000000LL

MyMove AI::getBestData(int pid, const Data& base_data, const vector<int> &in_vertices, Graph& myown_graph) const {
  Data best_data = base_data;
  MyMove next_move;
  next_move.src = 0; next_move.to = 0; next_move.score = 0;
  
  for (int v = 0; v < (int) myown_graph.num_vertices; v++) {
    for (auto& r :  myown_graph.rivers[v]) {
      auto nv = r.to;
      if (v > nv) {
        continue;
      }
      if (r.punter != -1) {
        continue;
      }
      if (v >= myown_graph.num_mines && nv >= myown_graph.num_mines && !in_vertices[v] && !in_vertices[nv]) {
        continue;
      }
      if (in_vertices[v] && in_vertices[nv]) {
        continue;
      }
      Graph::River* nrit = nullptr;
      for (auto &nr : myown_graph.rivers[nv]) {
        if (nr.to == v) {
          nrit = &nr;
          break;
        }
      }
      assert(nrit != nullptr);
      r.punter = pid;
      nrit->punter = pid;
      const auto& next_point = myown_graph.evaluate(num_punters, shortest_distances)[pid];
      Data current_data;
      current_data.point = next_point;
      current_data.degree = myown_graph.rivers[nv].size();
      if (shouldChange(&best_data, current_data)) {
        next_move.src = v;
        next_move.to = nv;
        next_move.score = next_point;
      }
      r.punter = -1;
      nrit->punter = -1;
    }
  }
  return next_move;
}


tuple<int,int, Json::Value> AI::move() const {
  Json::Value vertices_ = info;
  // if there is an edge connecting to mine,
  // try to connect them.
  // tie break is solved by degree.
 
  //auto start_time = chrono::system_clock::now();
  
  std::vector<int> in_vertices(graph.num_vertices, 0);
  for (int i = 0, N = vertices_.size(); i < N; i++) {
    in_vertices[vertices_[i].asInt()] = 1;
  }
  
  Graph myown_graph = graph;
  const auto& scores = myown_graph.evaluate(num_punters, shortest_distances);
  
  std::vector<ll> increase_scores(num_punters);
  std::vector<MyMove> next_move(num_punters);
  for (int i = 0; i < num_punters; i++) {
    Data base_data;
    base_data.point = scores[i];
    base_data.degree = INF;
    next_move[i]  = getBestData(i, base_data, in_vertices, myown_graph);
    increase_scores[i] = next_move[i].score - scores[i];
  }
  assert(num_punters > punter_id);
  
  ll myscore = increase_scores[punter_id];
  ll best_score = 0;
  int target_punter = -1;
  for (int i = 0; i < num_punters; i++) {
    if (i == punter_id) {
      continue;
    }
    if (best_score < increase_scores[i]) {
      best_score = increase_scores[i];
      target_punter = i;
    }
  }
  int src = -1;
  int to = -1;
  if (myscore > 0) {
    src = next_move[punter_id].src;
    to = next_move[punter_id].to;
  }
  if (target_punter != -1) {
    if (best_score > myscore * num_punters || myscore == 0){
      src = next_move[target_punter].src;
      to = next_move[target_punter].to;
    }
  }
  if (src == -1 && to == -1) {
    // Nobody can get point.
    // Then, get any edge
    // TODO(hiroh): get the edge around another player
    for (int i = 0; i < graph.num_vertices; i++) {
      for (const auto& r : graph.rivers[i]) {
        if (r.punter == -1) {
          src = i;
          to = r.to;
          goto out;
        }
      }
    }
  }
  
 out:
  assert(src != -1 && to != -1);
  Json::Value ret_vertices_ = vertices_;
  if (!in_vertices[src]) {
    ret_vertices_.append(src);
  }
  if (!in_vertices[to]) {
    ret_vertices_.append(to);
  }
  return make_tuple(src, to, ret_vertices_);
}

int main()
{
  AI ai;
  ai.run();
  return 0;
}
