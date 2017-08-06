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

class Galgalim : public Game {
protected:
  SetupSettings setup() const override;
  tuple<int, int, Json::Value> move() const override;
  string name() const override;

private:
  int64_t test_order(
    const vector<vector<int>>& dists,
    const vector<vector<int>>& prevs,
    const vector<int>& mine_ord,
    int& mines_united, pair<int, int>& first_unite) const;
  void update_connectivity(const Graph& g, vector<int>& conn, vector<int>& comp, int u) const;
};

string
Galgalim::name() const {
  return "GALGALIM";
}

SetupSettings
Galgalim::setup() const {
  return Json::Value();
}

tuple<int, int, Json::Value>
Galgalim::move() const {
  seed_seq seed = {(int)history.size(), punter_id};
  mt_engine.seed(seed);

  vector<vector<int>> cur_dists, cur_prevs;
  calc_cur_dists(cur_dists, cur_prevs);

  vector<int> unite_order(graph.num_mines);
  for (int i = 0; i < graph.num_mines; ++i) {
    unite_order[i] = i;
  }
  shuffle(unite_order.begin(), unite_order.end(), mt_engine);

  int united;
  pair<int, int> first_unite;
  int64_t eval = test_order(cur_dists, cur_prevs, unite_order, united, first_unite);
  for (int loop = 0; graph.num_mines > 1 && loop < 100; ++loop) {
    int a = randint(0, united);
    int b = randint(0, graph.num_mines - 1);
    if (a == b) {
      --loop;
      continue;
    }
    swap(unite_order[a], unite_order[b]);
    
    int nxt_united;
    pair<int, int> nxt_first_unite;
    int64_t nxt_eval = test_order(cur_dists, cur_prevs, unite_order, nxt_united, nxt_first_unite);
    if (eval < nxt_eval) {
      eval = nxt_eval;
      united = nxt_united;
      first_unite = nxt_first_unite;
    } else {
      swap(unite_order[a], unite_order[b]);
    }
  }

  if (united == 0) {  // greedy
    return make_tuple(first_unite.first, first_unite.second, Json::Value());
  } else {
    int mine = first_unite.first, c = first_unite.second;
    pair<int, int> e_first(-1, -1), e_last;
    for (int u = c; u != mine; u = cur_prevs[mine][u]) {
      if (owner(graph, u, cur_prevs[mine][u]) == -1) {
        if (e_first.first < 0) {
          e_first = {u, cur_prevs[mine][u]};
        }
        e_last = {u, cur_prevs[mine][u]};
      }
    }

    bool first_mine = min(e_first.first, e_first.second) < graph.num_mines;
    bool last_mine = min(e_last.first, e_last.second) < graph.num_mines;
    if (first_mine) {
      return make_tuple(e_first.first, e_first.second, Json::Value());
    } else if (last_mine) {
      return make_tuple(e_last.first, e_last.second, Json::Value());      
    } else {
      auto e = (randint(0, 1) ? e_first : e_last);
      return make_tuple(e.first, e.second, Json::Value());
    }
  }
}

int64_t
Galgalim::test_order(
  const vector<vector<int>>& dists,
  const vector<vector<int>>& prevs,
  const vector<int>& mine_ord,
  int& mines_united, pair<int, int>& first_unite) const {
  Graph g = graph;
  int num_edges = 0;
  for (int u = 0; u < g.num_vertices; ++u) {
    num_edges += g.rivers[u].size();
  }
  num_edges /= 2;

  const int lim = (num_edges - history.size() + num_punters - 1) / num_punters;
  int selected = 0;
  vector<int> connected(g.num_vertices, 0), component;
  update_connectivity(g, connected, component, mine_ord[0]);

  int first_sel = 1;
  mines_united = 0;
  first_unite = {-1, -1};
  for (int i = 1; i < g.num_mines; ++i) {
    const int mine = mine_ord[i];
    if (connected[mine]) {
      continue;
    }

    int nearest = -1, nearest_dist = INF;
    for (const int c : component) {
      if (dists[mine][c] < nearest_dist) {
        nearest_dist = dists[mine][c];
        nearest = c;
      }
    }
    if (nearest == -1) {
      continue;
    }

    int actual_dist = 0;
    for (int u = nearest; u != mine; u = prevs[mine][u]) {
      if (owner(g, u, prevs[mine][u]) == -1) {
        ++actual_dist;
      }
    }
    if (actual_dist + selected > lim) {
      break;
    }

    for (int u = nearest; u != mine; u = prevs[mine][u]) {
      if (owner(g, u, prevs[mine][u]) == -1) {
        claim(g, u, prevs[mine][u], punter_id);
        ++selected;
      }
    }

    update_connectivity(g, connected, component, mine);
    if (mines_united == 0) {
      first_sel = selected;
      first_unite = {mine, nearest};
    }
    ++mines_united;
  }

  vector<int64_t> weight(g.num_vertices, 0LL);
  for (int i = 0; i < g.num_vertices; ++i) {
    for (int m = 0; m < g.num_mines; ++m) {
      if (connected[m]) {
        weight[i] += dists[m][i] * dists[m][i];
      }
    }
  }

  while (selected < lim) {
    int64_t best_w = -1;
    int best_c, best_v;
    for (const int c : component) {
      for (const auto& river : g.rivers[c]) {
        if (connected[river.to] || river.punter != -1) {
          continue;
        }
        if (best_w < weight[river.to]) {
          best_w = weight[river.to];
          best_c = c;
          best_v = river.to;
        }
      }
    }
    if (best_w >= 0) {
      claim(g, best_c, best_v, punter_id);
      update_connectivity(g, connected, component, best_v);
      if (first_unite.first < 0) {
        first_unite = {best_c, best_v};
      }
    }
    ++selected;
  }

  return g.evaluate(num_punters, shortest_distances)[punter_id] * 1000 / first_sel / first_sel;
}

void
Galgalim::update_connectivity(const Graph& g, vector<int>& conn, vector<int>& comp, int start) const {
  queue<int> que;
  que.push(start);
  conn[start] = 1;
  comp.push_back(start);
  while (!que.empty()) {
    const int u = que.front(); que.pop();
    for (const auto& river : g.rivers[u]) {
      const int v = river.to;
      if (river.punter == punter_id && conn[v] == 0) {
        conn[v] = 1;
        comp.push_back(v);
        que.push(v);
      }
    }
  }
}

int main() {
  Galgalim GALGALIM;
  GALGALIM.run();
  return 0;
}
