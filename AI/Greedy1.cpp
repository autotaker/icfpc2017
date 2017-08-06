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
class Greedy1 : public Game {
    SetupSettings setup() const override;
    MoveResult move() const override;
};

SetupSettings Greedy1::setup() const {
    return Json::Value();
}

MoveResult Greedy1::move() const
{
  int current_max = -1e9;
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
    Greedy1 ai;
    ai.run();
    return 0;
}
