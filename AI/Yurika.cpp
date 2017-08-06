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

const int TIMELIMIT_MS_FOR_MOVE = 950;
const int MCTS_EDGE_THRESHOLD = 200;

class AI : public Game {
  SetupSettings setup() const override;
  tuple<int, int, Json::Value> move() const override;
  std::string name() const override;
  pair<int,int> getBestData(int pid, const Data& base_data, const vector<int> &in_vertices, Graph& myown_graph) const;
};

std::string AI::name() const {
  return "Ran";
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

pair<int,int> AI::getBestData(int pid, const Data& base_data, const vector<int> &in_vertices, Graph& myown_graph) const {
  Data best_data = base_data;
  int src = -1, to = -1;
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
        src = v;
        to = nv;
      }
      r.punter = -1;
      nrit->punter = -1;
    }
  }
  return make_pair(src, to);
}


tuple<int,int, Json::Value> AI::move() const {
  Json::Value vertices_ = info;
  // if there is an edge connecting to mine,
  // try to connect them.
  // tie break is solved by degree.
 
  auto start_time = chrono::system_clock::now();
  
  std::vector<int> in_vertices(graph.num_vertices, 0);
  for (int i = 0, N = vertices_.size(); i < N; i++) {
    in_vertices[vertices_[i].asInt()] = 1;
  }
  
  vector<int> degs(graph.num_vertices, 0);
  for (int i = 0; i < graph.num_vertices; i++) {
    for (auto &r : graph.rivers[i]) {
      if (r.punter == -1) {
        degs[i] += 1;
      }
    }
  }

  vector<tuple<int,int,int>> mine_results[2];
  for (int i = 0; i < graph.num_mines; i++) {
    for (auto &r : graph.rivers[i]) {
      if (r.punter != -1) {
        continue;
      }
      int to = r.to;
      mine_results[in_vertices[i]].emplace_back(degs[to], i, to);
    }
  }

  if (mine_results[0].size() > 0) {
    Json::Value ret_vertices_ = vertices_;
    auto pick = *max_element(mine_results[0].begin(), mine_results[0].end());
    auto src = std::get<1>(pick);
    auto to = std::get<2>(pick);
    if (!in_vertices[src]) {
      ret_vertices_.append(src);
    }
    if (!in_vertices[to]) {
      ret_vertices_.append(to);
    }
    return make_tuple(src, to, ret_vertices_);
  } else if (mine_results[1].size() > 0){
    Json::Value ret_vertices_ = vertices_;
    auto pick = *max_element(mine_results[1].begin(), mine_results[1].end());
    auto src = std::get<1>(pick);
    auto to = std::get<2>(pick);
    if (!in_vertices[src]) {
      ret_vertices_.append(src);
    }
    if (!in_vertices[to]) {
      ret_vertices_.append(to);
    }
    return make_tuple(src, to, ret_vertices_);
  }
  

  
  Graph myown_graph = graph;
  const auto& points = myown_graph.evaluate(num_punters, shortest_distances);
  Data base_data;
  base_data.point = points[punter_id];
  base_data.degree = INF;

  // First, try to maximize my score
  auto src_to = getBestData(punter_id, base_data, in_vertices, myown_graph);
  //cerr << src_to.first << " " << src_to.second << " maximize me" << endl;
  if (src_to.first == -1 && src_to.second == -1) {
    // Fail. That is, there is no edge to increase my score.
    // Then, try to decrease the final another's score.
    vector<pair<ll, int>> enemys;
    for (int i = 0; i < num_punters; i++) {
      if (i == punter_id) {
        continue;
      }
      enemys.emplace_back(points[i], i);
    }
    sort(enemys.begin(), enemys.end());
    reverse(enemys.begin(), enemys.end());
    Data enemy_base_data;
    for (const auto&  e : enemys) {
      enemy_base_data.point = e.first;
      enemy_base_data.degree = INF;
      const auto& enemy_src_to = getBestData(e.second, enemy_base_data, in_vertices, myown_graph);
      if (enemy_src_to.first != -1 && enemy_src_to.second != -1) {
        src_to = enemy_src_to;
        break;
      }
      auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start_time).count();
      if (elapsed_time >= 900) {
        break;
      }
    }
  }
  int src = src_to.first;
  int to = src_to.second;
  
  if (src == -1 && to == -1) {
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
