#include "../lib/Game.h"
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <numeric>
#include <utility>
#include <random>
#include <cmath>
#include <cassert>
#include <cstdint>
using namespace std;


static void claim(Graph& g, int src, int to, int p) {
  auto& rs = g.rivers[src];
  auto& rt = g.rivers[to];
  std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter = p;
  std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->punter = p;
}

// vector
template <class T>
ostream& operator<<(ostream &os, const vector<T> &vec) {
  os << "[";
  for (typename vector<T>::size_type i = 0; i < vec.size(); ++i)
    os << (i ? ", " : "") << vec[i];
  os << "]";
  return os;
}


class Durio : public Game {
public:
  SetupSettings setup() const override;
  MoveResult move() const override;
  string name() const override;
private:
  template <typename T> T random_element(const vector<T>& v, mt19937& mt) const;
  int cmp_centrality(const vector<int>& lds, const vector<int>& rds) const;
  MoveResult try_connect(const vector<vector<int>>& dist_mines) const;
  MoveResult expand_tree() const;
};


template <typename T>
T
Durio::random_element(const vector<T>& v, mt19937& mt) const {
  return v[uniform_int_distribution<size_t>(0, v.size() - 1)(mt)];
}

int
Durio::cmp_centrality(const vector<int>& lds, const vector<int>& rds) const {
  double lc = 0.0, rc = 0.0;
  for (int v = 0; v < graph.num_vertices; ++v) if (lds[v] && rds[v]) {
    lc += lds[v] == INF ? 0.0 : 1.0 / lds[v];
    rc += rds[v] == INF ? 0.0 : 1.0 / rds[v];
  }
  return lc - rc;
}

string
Durio::name() const {
  return "Durio";
}

SetupSettings
Durio::setup() const {
  return Json::Value();
}

MoveResult
Durio::move() const {
  cerr << "\n===== Turn: " << history.size() + 1 << " =====" << endl;

  // compute distances among mines
  vector<vector<int>> dist_mines, sp_prev_mines;
  calc_cur_dists(dist_mines, sp_prev_mines);

  bool connect_mode = false;
  for (int m = 0; m < graph.num_mines; ++m)
    for (int n = 0; n < m; ++n)
      if (dist_mines[m][n] != 0 && dist_mines[m][n] != INF)
        connect_mode = true;

  if (connect_mode)
    return try_connect(dist_mines);
  return expand_tree();
}

MoveResult
Durio::try_connect(const vector<vector<int>>& dist_mines) const {
  uint32_t my_seed = (punter_id + history.size()) * 2654435769;
  mt19937 mt(my_seed);

  // detect connected components
  int cur_group_id = 0;
  vector<int> group_id(graph.num_vertices, -1);
  for (int v = 0; v < graph.num_vertices; ++v) {
    if (group_id[v] != -1) continue;
    queue<int> q;
    q.push(v);
    group_id[v] = cur_group_id;
    while (!q.empty()) {
      int u = q.front(); q.pop();
      for (const auto& r : graph.rivers[u]) {
        if (r.punter != -1 && r.punter != punter_id) continue;
        int w = r.to;
        if (group_id[w] == -1) {
          q.push(w);
          group_id[w] = cur_group_id;
        }
      }
    }
    ++cur_group_id;
  }

  vector<vector<int>> group_vs(cur_group_id);
  for (int v = 0; v < graph.num_vertices; ++v)
    group_vs[group_id[v]].push_back(v);

  // find most connected component
  int best_con_score = -1, best_group_id = -1;
  for (int i = 0; i < cur_group_id; ++i) {
    const auto& vs = group_vs[i];
    int num_mines = lower_bound(vs.begin(), vs.end(), graph.num_mines) - vs.begin();
    int num_sites = vs.size();
    if (num_mines * num_sites > best_con_score) { // can improve?
      best_con_score = num_mines * num_sites;
      best_group_id = i;
    }
  }

  int max_turn = std::min(1000 * graph.num_edges, 100000000);  // need better setting
  vector<int> last_visited(graph.num_mines, -1);
  vector<vector<int>> commute_count(graph.num_mines, vector<int>(graph.num_mines, 0));

  queue<int> que_internal;
  vector<int> next_cand;
  vector<int> visited_internal(graph.num_vertices, -1);

  // random walk: choose initial node
  int cur_v = random_element(group_vs[best_group_id], mt);
  cerr << "target component: " << group_vs[best_group_id] << endl;

  // do random walk
  for (int t = 0; t < max_turn; ++t) {
    next_cand.clear();

    // gather adjacent nodes (contract rivers that claimed by me)
    que_internal.push(cur_v);
    visited_internal[cur_v] = t;
    while (!que_internal.empty()) {
      int u = que_internal.front(); que_internal.pop();

      // at mine now
      if (u < graph.num_mines) {
        for (int w = 0; w < graph.num_mines; ++w)
          if (last_visited[u] < last_visited[w])
            ++commute_count[u][w];
        last_visited[u] = t;
      }

      for (const auto& r : graph.rivers[u]) {
        int w = r.to;
        if (r.punter == punter_id) {  // river claimed by me
          if (visited_internal[w] != t) {
            que_internal.push(w);
            visited_internal[w] = t;
          }
        } else if (r.punter == -1)  // unclaimed
          next_cand.push_back(w);
      }
    }

    // choose next node
    cur_v = random_element(next_cand, mt);
  }

  // find least connected mines
  double best = 1e100;
  int src_mine = -1, to_mine = -1;
  for (int m = 0; m < graph.num_mines; ++m) {
    for (int n = 0; n < graph.num_mines; ++n) {
      int c = commute_count[m][n];
      int d = dist_mines[m][n];
      if (d != 0 && d != INF && c * sqrt(d) < best) { // can improve?
        src_mine = m;
        to_mine = n;
        best = c * sqrt(d);
      }
    }
  }

  assert(src_mine != -1);
  if (cmp_centrality(dist_mines[src_mine], dist_mines[to_mine]) > 0)
    swap(src_mine, to_mine);

  int best_src = -1, best_to = -1;
  vector<int> best_ds(graph.num_vertices, INF);

  // search desirable river
  queue<int> q;
  vector<int> visited(graph.num_vertices, 0);
  q.push(src_mine);
  visited[src_mine] = 1;
  while (!q.empty()) {
    int v = q.front(); q.pop();
    for (const auto& r : graph.rivers[v]) {
      int w = r.to;
      if (visited[w]) continue;
      if (r.punter == punter_id) {
        q.push(w);
        visited[w] = 1;
      } else if (r.punter == -1) {
        if (dist_mines[to_mine][w] == dist_mines[src_mine][to_mine] - 1) {
          vector<int> dist, sp_prev;
          calc_shortest_paths(w, dist, sp_prev);
          if (cmp_centrality(dist, best_ds) > 0) {
            best_src = v;
            best_to = w;
            best_ds = dist;
          }
        }
        visited[w] = 1;
      }
    }
  }

  cerr << "select " << best_src << ", " << best_to << endl;
  return make_tuple(best_src, best_to, info);
}

MoveResult
Durio::expand_tree() const {
  Graph g = graph;
  int64_t best_w = -1;
  int best_u = -1, best_v = -1;

  for (int u = 0; u < g.num_vertices; ++u) {
    for (const auto& river : g.rivers[u]) {
      if (u < river.to && river.punter == -1) {
        claim(g, u, river.to, punter_id);
        int64_t score = g.evaluate(num_punters, shortest_distances)[punter_id];
        claim(g, u, river.to, -1);

        if (best_w < score) {
          best_w = score;
          best_u = u;
          best_v = river.to;
        }
      }
    }
  }

  return make_tuple(best_u, best_v, info);
}

int main() {
  Durio durio;
  durio.run();
  return 0;
}
