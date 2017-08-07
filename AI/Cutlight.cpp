#include "Game.h"
#include "UnionFind.h"
#include <iostream>
#include <queue>
#include <tuple>
#include <cassert>
#include <set>
#include <algorithm>
using namespace std;

namespace {
  enum state_t {
    FUTURE,
    MINE,
    GREEDY,
    DONE,
  };


  // Copied from Galgalim.cpp
  /*
  void claim(Graph& g, int src, int to, int p) {
    auto& rs = g.rivers[src];
    auto& rt = g.rivers[to];
    std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter = p;
    std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->punter = p;
  }
  */

  int get_num_edges(const Graph &graph) {
    int num_edges = 0;
    for (int i = 0; i < graph.num_vertices; i++) {
      num_edges += graph.rivers[i].size();
    }
    return num_edges / 2;
  }

  set<int> get_visited_sites(const Game &game, int punter) {
    set<int> vertices;
    const History &history = game.get_history();
    for (const auto &move: history) {
      if (move.punter == punter) {
        vertices.insert(move.to);
        vertices.insert(move.src);
      }
    }
    return vertices;
  }

  pair<int, int> get_next_greedy(const Game &game, Graph &graph, const set<int> &visited_sites, int punter) {
    int src = -1, to = -1;
    pair<int, int> best_data(-1e9, -1); // pair of a next score and degree of a next vertex
    for (int v = 0; v < graph.num_vertices; v++) {
      for (auto &r : graph.rivers[v]) {
        auto nv = r.to;
        if (v > nv) {
          continue;
        }
        if (r.punter != -1) {
          continue;
        }

        const int v_visited = visited_sites.count(v);
        const int nv_visited = visited_sites.count(nv);

        if (v >= graph.num_mines && nv >= graph.num_mines && !v_visited && !nv_visited) {
          continue;

        }

        Graph::River* nrit = nullptr;
        for (auto &nr : graph.rivers[nv]) {
          if (nr.to == v) {
            nrit = &nr;
            break;
          }
        }

        assert(nrit != nullptr && nrit -> punter == -1);
        r.punter = punter;
        nrit->punter = punter;
        const int next_point = graph.evaluate(game.get_num_punters(), game.get_shortest_distances())[punter];
        pair<int, int> current_data(next_point, graph.rivers[nv].size());
        if (current_data > best_data) {
          best_data = current_data;
          src = v;
          to = nv;
        }
        r.punter = -1;
        nrit->punter = -1;
      }
    }
    return make_pair(src, to);
  }
}

namespace flowlight {
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

  pair<int, int> select_single_future(const Game &game) {
    Graph g = game.get_graph();
    //const auto &shortest_distances = game.get_shortest_distances();

    const int mine = rand() % g.num_mines;
    vector<int> distances(g.num_vertices, -1);
    bfs(g, mine, distances);

    vector<pair<int, int>> vs;
    for (int i = g.num_mines; i < g.num_vertices; ++i) {
      vs.emplace_back(distances[i], i);
    }
    sort(vs.begin(), vs.end());

    int k = rand() % vs.size();
    return {mine, vs[k].second};
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
      for (int s = 0; s < graph.num_vertices; s++) {
        for (const auto &river: graph.rivers[s]) {
          if (river.punter == -1 && river.to > s) {
            const int t = river.to;

            vector<int> distance(graph.num_vertices, graph.num_vertices + 1);
            vector<double> sigma(graph.num_vertices, 0);

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


  pair<int, int> get_next_mine(const Game &game, const Graph &graph, int prev_source, int prev_target) {
    UnionFind uf(get_current_union_find(game, graph));
    assert(uf.same(prev_source, prev_target));

    int best_mine = 0;
    int best_distance = graph.num_vertices;

    const auto current_rivers = get_current_river_graph(graph, uf);
    // shitty copy paste
    vector<int> distance(graph.num_vertices, graph.num_vertices + 1);
    distance[uf.find(prev_source)] = 0;

    queue<int> que;
    que.push(uf.find(prev_source));
    while (!que.empty()) {
      const int uv = que.front(); que.pop();
      const int d = distance[uv];
      for (const auto &river: current_rivers[uv]) {
        int uw, v, w;
        tie(uw, v, w) = river;
        const int nd = d + 1;
        if (nd < distance[uw]) {
          distance[uw] = nd;
          que.push(uw);
        }
      }
    }

    for (int m = 0; m < graph.num_mines; m++) {
      if (uf.same(m, prev_source)) continue;
      if (distance[uf.find(m)] < best_distance) {
        best_distance = distance[uf.find(m)];
        best_mine = m;
      }
    }

    if (best_distance == graph.num_vertices) {
      // oteage
      return make_pair(prev_source, prev_target);
    } else {
      return make_pair(prev_source, best_mine);
    }
  }
    // vector<int> greedyFutureSelector(const Graph &graph) {
    //   int best_source = 0;
    //   int best_target = 0;
    //   int best_distance = 0;
    //   int turn_threshold = get_num_edges(get_graph()) /  num_punters * 0.7;

    //   for (int i = 0; i < graph.num_mines; i++) {
    //     for (int j = graph.num_mines; j < graph.num_vertices; j++) {
    //       int d = shortest_distances[i][j];
    //       if (d > best_distance && d <= turn_threshold) {
    //         best_source = i;
    //         best_target = j;
    //         best_distance = d;
    //       }
    //     }
    //   }

    //   vector<int> futures;
    //   for (int i = 0; i < graph.num_mines; i++) {
    //     futures.push_back(i == best_source ? best_target : -1);
    //   }
    //   return futures;
    // }


  class AI : public Game {
    SetupSettings setup() const override;
    MoveResult move() const override;
    string name() const override;
  };

  string AI::name() const {
    return "Flowlight";
  }

  SetupSettings AI::setup() const {
    srand(punter_id);
    // int turn_threshold = get_num_edges(get_graph()) /  num_punters * 0.7;
    pair<int, int> best_future = select_single_future(*this);
    Json::Value info;
    info[0] = best_future.first;
    info[1] = best_future.second;
    info[2] = int(state_t::FUTURE);
    vector<int> futures;
    for (int i = 0; i < graph.num_mines; i++) {
      futures.push_back(i == best_future.first ? best_future.second : -1);
    }
    cerr << "Best pair: " << original_vertex_id(best_future.first) << " " << original_vertex_id(best_future.second) << endl;
    return SetupSettings(info, futures);;
  }


  MoveResult AI::move() const {
    int source = info[0].asInt();
    int target = info[1].asInt();
    state_t state = state_t(info[2].asInt());

    pair<int, int> next_move(-1, -1);
    UnionFind uf = get_current_union_find(*this, get_graph());


    if (state == state_t::FUTURE) {
      if (uf.same(source, target)) {
        pair<int, int> next_mine = get_next_mine(*this, graph, source, target);
        source = next_mine.first;
        target = next_mine.second;
      }

      next_move = connect_move(*this, graph, source, target);
      if (next_move.first == -1 || next_move.second == -1) {
        state = state_t::MINE;
      }
    }

    if (state == state_t::MINE) {
      if (uf.same(source, target)) {
        pair<int, int> next_mine = get_next_mine(*this, graph, source, target);
        source = next_mine.first;
        target = next_mine.second;
      }

      next_move = connect_move(*this, graph, source, target);
      if (next_move.first == -1 || next_move.second == -1) {
        state = state_t::GREEDY;
      }
    }

    if (state == state_t::GREEDY) {
      Graph g = get_graph();
      next_move = get_next_greedy(*this, g, get_visited_sites(*this, punter_id), punter_id);
      if (next_move.first == -1 || next_move.second == -1) {
        state = state_t::DONE;
      }
    }

    Json::Value next_info;
    next_info[0] = source;
    next_info[1] = target;
    next_info[2] = int(state);
    return make_tuple(next_move.first, next_move.second, next_info);
  }
}


int main() {
  flowlight::AI ai;
  ai.run();
  return 0;
}
