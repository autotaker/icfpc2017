//#define _GLIBCXX_DEBUG

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
using namespace std;
class MizukiAI : public Game {
  SetupSettings setup() const override;
  MoveResult move() const override;
  std::string name() const override {
    return "MizukiAI";
  }
  
  std::vector<int> calc_connected_mine() const;
};

SetupSettings MizukiAI::setup() const {
  return Json::Value();
}

std::vector<int> MizukiAI::calc_connected_mine() const {
  std::vector<int> connected_mine(graph.num_vertices, -1);
  for (int i = 0; i < graph.num_mines; ++i) {
    if (connected_mine[i] != -1) continue;
    queue<int> q;
    q.push(i);
    connected_mine[i] = i;
    while (!q.empty()) {
      int cv = q.front();
      q.pop();
      for (const auto& r : graph.rivers[cv]) {
	if (r.punter != punter_id) continue;
	if (connected_mine[r.to] != -1) continue;
	connected_mine[r.to] = i;
	q.push(r.to);
      }
    }
  }
  return connected_mine;
}

template <int POS, class TUPLE> void deploy(std::ostream &os, const TUPLE &tuple){}
template <int POS, class TUPLE, class H, class ...Ts> void deploy(std::ostream &os, const TUPLE &t){ os << (POS == 0 ? "" : ", ") << get<POS>(t); deploy<POS + 1, TUPLE, Ts...>(os, t); }
std::ostream& operator<<(std::ostream &os, const std::pair<int,int> &p){ os << "(" << p.first <<", " << p.second <<")";return os; }
template <class T> std::ostream& operator<<(std::ostream &os, const std::vector<T> &v){ int remain = v.size(); os << "{"; for(auto e: v) os << e << (--remain == 0 ? "}" : ", "); return os; }
template <class T,class U> std::ostream& operator<<(std::ostream &os, const std::pair<T,U> &p){ os << "(" << p.first <<", " << p.second <<")";return os; }
template <class T> std::ostream& operator<<(std::ostream &os, const std::set<T> &v){ int remain = v.size(); os << "{"; for(auto e: v) os << e << (--remain == 0 ? "}" : ", "); return os; }
template <class T, class K> std::ostream& operator<<(std::ostream &os, const std::map<T, K> &mp){ int remain = mp.size(); os << "{"; for(auto e: mp) os << "(" << e.first << " -> " << e.second << ")" << (--remain == 0 ? "}" : ", "); return os; }

MoveResult MizukiAI::move() const
{
  int to = -1, from = -1;
  Graph mutable_graph = graph;
  auto connected_mine = calc_connected_mine();
  //vector<map<int,int>> rev_edge(graph.num_vertices);
  int remained_edges = 0;
  for (size_t i = 0; i < mutable_graph.rivers.size(); ++i) {
    for (size_t j = 0; j < mutable_graph.rivers[i].size(); ++j) {
      if (mutable_graph.rivers[i][j].punter == -1) {
        remained_edges += 1;
      }
    }
  }
  const int beam_width = 20;
  std::set <pair<long long, std::vector<pair<int,int> > > > current_selection;// score, vector (src, index)
  // initial
  current_selection.emplace(0, std::vector<pair<int,int>>());
  std::set <pair<long long, std::vector<pair<int,int> > > > next_selection;// score, vector (src, index)

  for (int beam_time = 0; beam_time < min(20, remained_edges); beam_time++) {
    next_selection.clear();
    for (const auto& selection : current_selection) {
      cerr << selection << endl;
      const auto& obtained_edges = selection.second;
      for (const auto&e : obtained_edges) {
        int v = e.first;
        int w =  mutable_graph.rivers[e.first][e.second].to;
        int index = lower_bound(graph.rivers[w].begin(), graph.rivers[w].end(), v) - graph.rivers[w].begin();
        mutable_graph.rivers[v][e.second].punter = punter_id;
        mutable_graph.rivers[w][index].punter = punter_id;
      }
      for (size_t i = 0; i < mutable_graph.rivers.size(); ++i) {
        for (size_t j = 0; j < mutable_graph.rivers[i].size(); ++j) {
          auto& r = mutable_graph.rivers[i][j];
          if (connected_mine[i] == connected_mine[r.to]) 
            continue;
          if (r.punter != -1 || (int) i < r.to) continue;
          r.punter = punter_id;
          int ridx = lower_bound(graph.rivers[r.to].begin(), graph.rivers[r.to].end(), i) - graph.rivers[r.to].begin();
          mutable_graph.rivers[r.to][ridx].punter = punter_id;
          auto score = mutable_graph.evaluate(num_punters, shortest_distances, punter_id)[punter_id];
          r.punter = -1;
          mutable_graph.rivers[r.to][ridx].punter = -1;
          
          bool should_change = next_selection.size() < beam_width;
          if (!should_change) {
            auto it = next_selection.begin();
            if (it->first < score) {
              next_selection.erase(it);
              should_change = true;
            }
          }
          if (should_change) {
            auto path = obtained_edges;
            path.emplace_back(i, j);
            next_selection.insert(make_pair(score, path));
          }
        }
      }
    }
    if (next_selection.size() == 0) {
      break;
    }
    swap(next_selection, current_selection);
  }
  {
    if ((*current_selection.rbegin()).first == 0) {
      for (size_t i = 0; i < mutable_graph.rivers.size(); ++i) {
        for (size_t j = 0; j < mutable_graph.rivers[i].size(); ++j) {
          if (mutable_graph.rivers[i][j].punter == -1) {
            from = i;
            to = mutable_graph.rivers[i][j].to;
            goto OUT;
          }
        }
      }
    } else {
      int i = (*current_selection.rbegin()).second[0].first;
      int j = (*current_selection.rbegin()).second[0].second;
      from = i;
      to = mutable_graph.rivers[i][j].to;
    }
  }
 OUT:
  cerr << from << " " << to << endl;
  return make_tuple(to, from, Json::Value());
}

int main()
{
  MizukiAI ai;
  ai.run();
  return 0;
}
