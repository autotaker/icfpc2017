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

template <int POS, class TUPLE> void deploy(std::ostream &os, const TUPLE &tuple){}
template <int POS, class TUPLE, class H, class ...Ts> void deploy(std::ostream &os, const TUPLE &t){ os << (POS == 0 ? "" : ", ") << get<POS>(t); deploy<POS + 1, TUPLE, Ts...>(os, t); }
template <class T,class U> std::ostream& operator<<(std::ostream &os, std::pair<T,U> &p){ os << "(" << p.first <<", " << p.second <<")";return os; }
template <class T> std::ostream& operator<<(std::ostream &os, std::vector<T> &v){ int remain = v.size(); os << "{"; for(auto e: v) os << e << (--remain == 0 ? "}" : ", "); return os; }
template <class T> std::ostream& operator<<(std::ostream &os, std::set<T> &v){ int remain = v.size(); os << "{"; for(auto e: v) os << e << (--remain == 0 ? "}" : ", "); return os; }
template <class T, class K> std::ostream& operator<<(std::ostream &os, std::map<T, K> &mp){ int remain = mp.size(); os << "{"; for(auto e: mp) os << "(" << e.first << " -> " << e.second << ")" << (--remain == 0 ? "}" : ", "); return os; }


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

tuple<int,int, Json::Value> AI::move() const {
  int maxP = 0;
  int src = -1, to = -1;
  Graph myown_graph = graph;
  //cerr << "--------" << endl;
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
        if (next_point > maxP) {
          src = v;
          to = nv;
          maxP = next_point;
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

