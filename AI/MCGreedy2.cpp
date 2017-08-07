#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <utility>
#include <string>
#include <cstring>
#include <queue>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <cassert>
#include <numeric>
#include <random>

#include "../lib/Game.h"

#include "../lib/MCTS_core.h"
#include "MCTS_AI.h"

#define trace(var) cerr<<">>> "<<#var<<" = "<<var<<endl;
#define choose(vec) (vec[rand() % vec.size()])

template<class S, class T> inline
std::ostream& operator<<(std::ostream&os, std::pair<S,T> p) {
  return os << '(' << p.first << ", " << p.second << ')';
}

template<class T, class U> inline
std::ostream& operator<<(std::ostream&os, std::tuple<T,U> t) {
  return os << '(' << std::get<0>(t) << ", " << std::get<1>(t) << ')';
}

template<class S, class T, class U> inline
std::ostream& operator<<(std::ostream&os, std::tuple<S,T,U> t) {
  return os << '(' << std::get<0>(t) << ", " << std::get<1>(t) << ", " << std::get<2>(t) << ')';
}

template<class T> inline
std::ostream& operator<<(std::ostream&os, std::set<T> v) {
  os << "(set"; for (T item: v) os << ' ' << item; os << ")"; return os;
}

template<class T> inline
std::ostream& operator<<(std::ostream&os, std::vector<T> v) {
  if (v.size() == 0) return os << "(empty)";
  os << v[0]; for (int i=1, len=v.size(); i<len; ++i) os << ' ' << v[i];
  return os;
}

using namespace std;
class Greedy2 : public Game {
  SetupSettings setup() const override;
  MoveResult move() const override;
  std::string name() const override {
    return "Greedy2";
  }
  
  void calc_connected_mine(std::vector<int>* connected_mine) const;
};

SetupSettings Greedy2::setup() const {
  return Json::Value();
}

void Greedy2::calc_connected_mine(std::vector<int>* connected_mine) const {
  connected_mine->resize(graph.num_vertices, -1);

  for (int i = 0; i < graph.num_mines; ++i) {
    if ((*connected_mine)[i] != -1) continue;
    queue<int> q;
    q.push(i);
    (*connected_mine)[i] = i;
    while (!q.empty()) {
      int cv = q.front();
      q.pop();
      for (const auto& r : graph.rivers[cv]) {
	if (r.punter != punter_id) continue;
	if ((*connected_mine)[r.to] != -1) continue;
	(*connected_mine)[r.to] = i;
	q.push(r.to);
      }
    }
  }
}

MoveResult Greedy2::move() const
{
  vector<vector<int>> dist, prev;
  calc_cur_dists(dist, prev);

  vector<int> connected_mine;
  calc_connected_mine(&connected_mine);

  {
    int mindist = INF;
    int from, to;

    for (int i = 0; i < graph.num_vertices; ++i) {
      if (connected_mine[i] == -1) continue;
      for (const auto& r : graph.rivers[i]) {
	if (r.punter != -1 || r.punter == punter_id) continue;
	
	if (connected_mine[r.to] != -1 &&
	    connected_mine[r.to] != connected_mine[i]) {
	  mindist = 0;
	  from = i;
	  to = r.to;
	  goto RET;
	}

	int tdist = INF;
	for (int j = 0; j < graph.num_mines; ++j) {
	  if (connected_mine[j] == i) continue;
	  if (dist[j][r.to] == INF) continue;
	  if (dist[j][r.to] >= dist[j][i]) continue;
	  tdist = min(tdist, dist[j][r.to]);
	}
	if (tdist < mindist) {
	  mindist = tdist;
	  from = i;
	  to = r.to;
	}
      }
    }
    
  RET:
    if (mindist < INF) {
      return make_tuple(from, to, Json::Value());
    }
  }
  
  int remaining_turns = graph.num_edges - (int)get_history().size();
  if (remaining_turns < 100) {
	  auto g = *this;
	  MCTS_Core core(&g);
	  auto p = core.get_play(950);
	  return make_tuple(p.first, p.second, Json::Value());
  }

  int current_max = -INF;
  int to, from;
  Graph mutable_graph = graph;

  map<pair<int, int>, int> rev_edge;
  for (size_t i = 0; i < mutable_graph.rivers.size(); ++i) {
    for (size_t j = 0; j < mutable_graph.rivers[i].size(); ++j) {
      rev_edge[make_pair(mutable_graph.rivers[i][j].to, i)] = j;
    }
  }

  for (size_t i = 0; i < mutable_graph.rivers.size(); ++i) {
    for (auto& r : mutable_graph.rivers[i]) {
      if (current_max > -INF && connected_mine[i] == connected_mine[r.to]) 
      	continue;
      if (r.punter != -1 || (int)i < r.to) continue;
      r.punter = punter_id;
      int ridx = rev_edge[make_pair(i, r.to)];
      mutable_graph.rivers[r.to][ridx].punter = punter_id;
      auto scores = mutable_graph.evaluate(num_punters, shortest_distances);
      scores[punter_id];
      r.punter = -1;
      mutable_graph.rivers[r.to][ridx].punter = -1;
      if (current_max < scores[punter_id]) {
	to = i;
	from = r.to;
	current_max = scores[punter_id];
      }
    }
  }
  std::cerr << current_max << std::endl;
  return make_tuple(to, from, info);
}

int main()
{
  Greedy2 ai;
  ai.run();
  return 0;
}
