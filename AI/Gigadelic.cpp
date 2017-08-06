#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <iostream>
#include <sstream>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <utility>
#include <numeric>
#include <algorithm>
#include <bitset>
#include <complex>
#include <array>
#include <list>
#include <stack>
#include <valarray>
#include <memory>
#include <random>

#include "Game.h"
#include "UnionFind.h"
#include "json/json.h"

using namespace std;

///
/// static functions/variables
static mt19937 mt_engine;

static int randint(int lo, int hi) {
  std::uniform_int_distribution<int> dist(lo, hi);
  return dist(mt_engine);
}

static void claim(Graph& g, int src, int to, int p) {
  auto& rs = g.rivers[src];
  auto& rt = g.rivers[to];
  std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter = p;
  std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->punter = p;
}

static int owner(const Graph& g, int src, int to) {
  const auto& rs = g.rivers[src];
  return std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter;
}
/// static functions/variables
///

struct Info {
  vector<vector<int>> conn_cnt;
  static const char* CONN_CNT;

  static Info from_json(const Json::Value& info);
  Json::Value to_json() const;

  Info() { }
  Info(const vector<vector<int>>& conn_cnt)
    : conn_cnt(conn_cnt) { }
};
const char* Info::CONN_CNT = "cc";

Info
Info::from_json(const Json::Value& info) {
  Info res;
  res.conn_cnt.clear();
  for (const Json::Value& c : info[CONN_CNT]) {
    vector<int> v;
    for (const Json::Value& ent : c) {
      v.push_back(ent.asInt());
    }
    res.conn_cnt.push_back(::move(v));
  }
  return res;
}

Json::Value
Info::to_json() const {
  Json::Value res;
  res[CONN_CNT].resize(conn_cnt.size());
  for (int i = 0; i < (int)conn_cnt.size(); ++i) {
    for (int j = 0; j < (int)conn_cnt[i].size(); ++j) {
      res[CONN_CNT][i].append(conn_cnt[i][j]);
    }
  }

  return res;
}

class Gigadelic : public Game {
protected:
  SetupSettings setup() const override;
  tuple<int, int, Json::Value> move() const override;
  string name() const override;

private:
  void random_removal(vector<vector<int>>& conn_cnt) const; 
};

string
Gigadelic::name() const {
  return "gigadelic";
}

SetupSettings
Gigadelic::setup() const {
  const int LIM = 20000000;
  const int M = graph.num_mines;

  vector<vector<int>> conn_cnt(M, vector<int>(M, 0));
  for (int loop = 0; loop < LIM; loop += graph.num_edges) {
    random_removal(conn_cnt);
  }

  return Info(conn_cnt).to_json();
}

tuple<int, int, Json::Value>
Gigadelic::move() const {
  seed_seq seed = {(int)history.size(), punter_id};
  mt_engine.seed(seed);

  auto conn_cnt = Info::from_json(info).conn_cnt;
  cerr<<"**********"<<endl;
  for (int i=0;i<graph.num_mines;++i){
    for (int j=0;j<graph.num_mines;++j){
      cerr<<conn_cnt[i][j]<<',';
    }
    cerr<<endl;
  }

  const int LIM = 1000000;
  for (int loop = 0; loop < LIM; loop += graph.num_edges) {
    random_removal(conn_cnt);
  }

  UnionFind cur_uf(graph.num_vertices);
  for (int u = 0; u < graph.num_vertices; ++u) {
    for (const auto& river : graph.rivers[u]) {
      if (river.punter == punter_id) {
        cur_uf.unite(u, river.to);
      }
    }
  }

  vector<pair<int, int>> mine_pairs;
  for (int i = 0; i < graph.num_mines; ++i) {
    for (int j = i + 1; j < graph.num_mines; ++j) {
      if (cur_uf.same(i, j)) {
        continue;
      }
      mine_pairs.emplace_back(i, j);
    }
  }

  sort(mine_pairs.begin(), mine_pairs.end(), [&] (const pair<int, int>& a, const pair<int, int>& b) {
    return conn_cnt[a.first][a.second] > conn_cnt[b.first][b.second];
  });

  vector<vector<int>> dists, prevs;
  calc_cur_dists(dists, prevs);
  for (const auto& mp : mine_pairs) {
    if (conn_cnt[mp.first][mp.second] == 0) {
      continue;
    }
    
    const int dis = dists[mp.first][mp.second];
    if (dis == INF || history.size() + dis * num_punters * 2 > graph.num_edges) {
      continue;
    }

    int u = mp.second;
    while (true) {
      if (owner(graph, u, prevs[mp.first][u]) == -1) {
        break;
      }
      u = prevs[mp.first][u];
    }
    return make_tuple(u, prevs[mp.first][u], Info(conn_cnt).to_json());
  }

  int64_t best_score = -INF;
  int best_u = -1, best_v = -1;
  Graph g = graph;
  for (int u = 0; u < g.num_vertices; ++u) {
    for (const auto& river : g.rivers[u]) {
      if (u < river.to && river.punter == -1) {
        claim(g, u, river.to, punter_id);
        int64_t score = g.evaluate(num_punters, shortest_distances)[punter_id];
        claim(g, u, river.to, -1);

        if (best_score < score) {
          best_score = score;
          best_u = u;
          best_v = river.to;
        }
      }
    }
  }

  return make_tuple(best_u, best_v, Info(conn_cnt).to_json());
}

void
Gigadelic::random_removal(vector<vector<int>>& conn_cnt) const {
  Graph g = graph;
  for (int u = 0; u < g.num_vertices; ++u) {
    for (const auto& river : g.rivers[u]) {
      if (river.punter == punter_id) {
        continue;
      }
      if (u < river.to && (river.punter >= 0 || randint(1, num_punters) != 1)) {
        claim(g, u, river.to, 0);
      }
    }
  }

  UnionFind uf(g.num_vertices);
  for (int u = 0; u < g.num_vertices; ++u) {
    for (const auto& river : g.rivers[u]) {
      if (river.punter == -1) {
        uf.unite(u, river.to);
      }
    }
  }

  for (int i = 0; i < g.num_mines; ++i) {
    for (int j = i + 1; j < g.num_mines; ++j) {
      if (uf.same(i, j)) {
        ++conn_cnt[i][j];
        ++conn_cnt[j][i];
      }
    }
  }
}

int main() {
  Gigadelic gigadelic;
  gigadelic.run();
  return 0;
}
