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
#include "UnionFind.h"

#include "FlowlightUtil.h"

namespace GenocideOption {

using namespace std;

int owner(const Graph& g, int src, int to) {
  const auto& rs = g.rivers[src];
  return std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter;
}

void claim_o(Graph& g, int src, int to, int p) {
  auto& rs = g.rivers[src];
  auto& rt = g.rivers[to];
  if (owner(g, src, to) == -1) {
    std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter = p;
    std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->punter = p;
  } else {
    std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->option = p;
    std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->option = p;    
  }
}

void unclaim_o(Graph& g, int src, int to, int p) {
  auto& rs = g.rivers[src];
  auto& rt = g.rivers[to];
  if (owner(g, src, to) == p) {
    std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter = -1;
    std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->punter = -1;    
  } else {
    std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->option = -1;
    std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->option = -1;    
  }
}

  void claim(Graph& g, int src, int to, int p) {
    auto& rs = g.rivers[src];
    auto& rt = g.rivers[to];
    std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter = p;
    std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->punter = p;
  }

  void bfs(const Graph &graph, int source, vector<int> &distance) {
    distance[source] = 0;
    queue<int> que;
    que.push(source);
    while (!que.empty()) {
      int v = que.front(); que.pop();
      for (const auto &river: graph.rivers[v]) {
        if (river.punter == -2) continue;
        if (distance[river.to] == -1) {
          que.push(river.to);
          distance[river.to] = distance[v] + 1;
        }
      }
    }
  }
  
  pair<int, int> select_single_future(const Game *game, int turn_limit, double epsilon = 0.2) {
    Graph g = game->get_graph();
    const auto &shortest_distances = game->get_shortest_distances();
    
    vector<vector<double> >  scores(g.num_mines, vector<double>(g.num_vertices, 0));
    for (int i = 0; i < 100; i++) {
      vector<tuple<int, int, int> > claimed;
      for (int v = 0; v < g.num_vertices; v++) {
        for (const auto &river: g.rivers[v]) {
          if (v < river.to && (double) rand() / RAND_MAX < epsilon) {
            claimed.push_back(make_tuple(v, river.to, river.punter));
            claim(g, v, river.to, -2);
          }
        }
      }

      for (int m = 0; m < g.num_mines; m++) {
        vector<int> distance_after_random_remove(g.num_vertices, -1);
        bfs(g, m, distance_after_random_remove);
        for (int v = 0; v < g.num_vertices; v++) {
          const int d1 = shortest_distances[m][v];
          const int d2 = distance_after_random_remove[v];
          if (d2 > 0 && d1 < turn_limit) {
            scores[m][v] += (double) (d1 * d1 * d1) / (d2 * d2);
          }
        }
      }
          
      for (const auto &c: claimed) {
        int src, dst, punter;
        tie(src, dst, punter) = c;
        claim(g, src, dst, punter);
      }
    }

    int best_source = 0;
    int best_target = 0;
    for (int m = 0; m < g.num_mines; m++) {
      for (int v = g.num_mines; v < g.num_vertices; v++) {
        if (scores[m][v] > scores[best_source][best_target]) {
          best_source = m;
          best_target = v;
        }
      }
    }
    return make_pair(best_source, best_target);
  }
  
  static UnionFind get_current_union_find(const Game &game, const Graph &graph) {
    UnionFind uf(graph.num_vertices);
    for (const auto &move: game.get_history()) {
      if (move.punter == game.get_punter_id()) {
        uf.unite(move.src, move.to);
      }
    }
    return uf;
  }

  static vector<vector<tuple<int, int, int> > > get_current_river_graph(const Graph &graph, UnionFind &uf) {
    vector<vector<tuple<int, int, int> > > current_rivers(graph.num_vertices);
    for (int v = 0; v < graph.num_vertices; v++) {
      for (const auto &river: graph.rivers[v]) {
        if (river.punter == -1) {
          const int w = river.to;
          int uv = uf.find(v);
          int uw = uf.find(w);
          current_rivers[uv].push_back(make_tuple(uw, v, w));
        }
      }
    }
    return current_rivers;
  }
  
  static pair<int, int> connect_move(const Game &game, const Graph &graph, int source, int target) {
    UnionFind uf(graph.num_vertices);
    for (const auto &move: game.get_history()) {
      if (move.punter == game.get_punter_id()) {
        uf.unite(move.src, move.to);
      }
    }
    
    vector<vector<tuple<int, int, int> > > current_rivers(get_current_river_graph(graph, uf));
    if (uf.find(source) != uf.find(target)) {
      int best_edge_source = -1;
      int best_edge_target = -1;
      int worst_distance = -1;
      double worst_sigma = -1;

      const Graph spd_dag = flowlight::build_shortest_path_dag(graph, source, target, game.get_punter_id());
      for (int s = 0; s < spd_dag.num_vertices; s++) {
        for (const auto &river: spd_dag.rivers[s]) {
          if (river.punter == -1 && river.to > s) {
            const int t = river.to;
          
            vector<int> distance(spd_dag.num_vertices, spd_dag.num_vertices + 1);
            vector<double> sigma(spd_dag.num_vertices, 0);
            
            distance[uf.find(source)] = 0;
            sigma[uf.find(source)] = 1;
          
            queue<int> que;
            que.push(uf.find(source));
            while (!que.empty()) {
              const int uv = que.front(); que.pop();
              const int d = distance[uv];
              if (uv == uf.find(target)) {
                break;
              }
            
              for (const auto &river: current_rivers[uv]) {
                int uw, v, w;
                tie(uw, v, w) = river;
                if (v == s && w == t) continue;
                if (v == t && w == s) continue;
              
                const int nd = d + 1;
                if (nd < distance[uw]) {
                  distance[uw] = nd;
                  sigma[uw] = sigma[uv];
                  que.push(uw);
                } else if (nd == distance[uw]) {
                  sigma[uw] += sigma[uv];
                }
              }
            }
          
            if (distance[uf.find(target)] > worst_distance ||
                (distance[uf.find(target)] == worst_distance &&
                 sigma[uf.find(target)] < worst_sigma)) {
              best_edge_source = s;
              best_edge_target = t;
              worst_distance = distance[uf.find(target)];
              worst_sigma = sigma[uf.find(target)];
            }
          }
        }
      }
      return make_pair(best_edge_source, best_edge_target);
    } else {
      return make_pair(-1, -1);
    }

  }

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

class GenocideOption : public Game {
public:
  SetupSettings setup() const override;
  MoveResult move() const override;
  Json::Value walkin_setup() const override;
  std::string name() const override {
    return "GenocideOption";
  }
  
  void calc_connected_mine(std::vector<int>* connected_mine) const;
};

SetupSettings GenocideOption::setup() const {
  return Json::Value();
}

void GenocideOption::calc_connected_mine(std::vector<int>* connected_mine) const {
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

MoveResult GenocideOption::move() const
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
      for (int m = 0; m < graph.num_mines; ++m) {
        if (connected_mine[i] == connected_mine[m]) {
          continue;
        }
        if (mindist > dist[m][i]) {
          mindist = dist[m][i];
          from = m;
          to = i;
        }
      }
    }
    
  RET:
    if (mindist < INF) {
      auto it = connect_move(*this, graph, from, to);
      return make_tuple(it.first, it.second, info);
    }
  }
  
  Graph g = graph;
  int64_t best_score = -1;
  int best_u = -1, best_v = -1;
  const int opt_remain = (options_enabled ? g.num_mines : 0) - options_bought;
  for (int u = 0; u < g.num_vertices; ++u) {
    if (connected_mine[u] == -1) continue;
    for (const auto& river : g.rivers[u]) {
      bool ok = false;
      if (river.punter == -1) {
        ok = true;
      } else {
        ok = (river.punter != punter_id && river.option == -1);
        ok &= opt_remain > 0;
      }
      if (!ok) {
        continue;
      }

      claim_o(g, u, river.to, punter_id);
      int64_t score = g.evaluate(num_punters, shortest_distances)[punter_id];
      unclaim_o(g, u, river.to, punter_id);

      if (best_score < score) {
        best_score = score;
        best_u = u;
        best_v = river.to;
      }
    }
  }

  if (best_u < 0) {
    for (int u = 0; best_u < 0 && u < g.num_vertices; ++u) {
      for (const auto& river : g.rivers[u]) {
        if (river.punter == -1) {
          best_u = u;
          best_v = river.to;
          break;
        }
      }
    }
  }

  return make_tuple(best_u, best_v, info);
}

Json::Value
GenocideOption::walkin_setup() const {
  return Json::Value();
}

}

#ifndef _USE_AS_ENGINE
int main()
{
  GenocideOption::GenocideOption ai;
  ai.run();
  return 0;
}
#endif
