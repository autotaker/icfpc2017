#include "../lib/Game.h"
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <numeric>
#include <utility>
#include <random>
#include <cassert>
#include <cstdint>
using namespace std;

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
  double compute_centrality(const vector<int>& ds) const;
  MoveResult try_connect(const vector<vector<int>>& dist_mines) const;
  MoveResult expand_tree() const;
};


template <typename T>
T
Durio::random_element(const vector<T>& v, mt19937& mt) const {
  return v[uniform_int_distribution<size_t>(0, v.size() - 1)(mt)];
}

double
Durio::compute_centrality(const vector<int>& ds) const {
  return accumulate(ds.begin(), ds.end(), 0.0, [&](double x, int d) {
    return x + (d && d != INF ? 1.0 / d : 0.0);
  });
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
  int best = INF;
  int src_mine = -1, to_mine = -1;
  for (int m = 0; m < graph.num_mines; ++m) {
    for (int n = 0; n < graph.num_mines; ++n) {
      int c = commute_count[m][n];
      int d = dist_mines[m][n];
      if (d != 0 && d != INF && c * d < best) { // can improve?
        src_mine = m;
        to_mine = n;
        best = c * d;
      }
    }
  }

  assert(src_mine != -1);
  if (compute_centrality(dist_mines[src_mine]) > compute_centrality(dist_mines[to_mine]))
    swap(src_mine, to_mine);

  cerr << "src_mine: " << src_mine << " / " << "to_mine: " << to_mine << endl;
  cerr << "src C: " << compute_centrality(dist_mines[src_mine]) << " / ";
  cerr << "to C:" << compute_centrality(dist_mines[to_mine]) << endl;
  cerr << "distance: " << dist_mines[src_mine][to_mine] << endl;

  int best_src = -1, best_to = -1;
  double best_centrality = -1;

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
          double c = compute_centrality(dist);
          if (c > best_centrality) {
            best_src = v;
            best_to = w;
            best_centrality = c;
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
  return make_tuple(-1, -1, info);
}

int main() {
  Durio durio;
  durio.run();
  return 0;
}
