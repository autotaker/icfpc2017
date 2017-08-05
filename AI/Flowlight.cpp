#include "Game.h"
#include "UnionFind.h"
#include <iostream>
#include <queue>
#include <tuple>
#include <cassert>
#include <set>
using namespace std;

namespace {
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
        const auto& next_point = graph.evaluate(game.get_num_punters(), game.get_shortest_distances())[punter];
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
  static pair<int, int> connectMove(const Game &game, const Graph &graph, int source, int target) {
    UnionFind uf(graph.num_vertices);
    for (const auto &move: game.get_history()) {
      if (move.punter == game.get_punter_id()) {
        uf.unite(move.src, move.to);
      }
    }
    
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

  
  class AI : public Game {
    SetupSettings setup() const override;
    tuple<int, int, Json::Value> move() const override;
    string name() const override;
  };

  string AI::name() const {
    return "Flowlight";
  }

  SetupSettings AI::setup() const {
    int best_source = 0;
    int best_target = 0;
    int best_distance = 0;
    int turn_threshold = get_num_edges(get_graph()) /  num_punters * 0.7;
    
    for (int i = 0; i < graph.num_mines; i++) {
      for (int j = graph.num_mines; j < graph.num_vertices; j++) {
        int d = shortest_distances[i][j];
        if (d > best_distance && d <= turn_threshold) {
          best_source = i;
          best_target = j;
          best_distance = d;
        }
      }
    }

    Json::Value info;
    info[0] = best_source;
    info[1] = best_target;
    vector<int> futures;
    for (int i = 0; i < graph.num_mines; i++) {
      futures.push_back(i == best_source ? best_target : -1);
    }
    return SetupSettings(info, futures);;
  }
  
  
  tuple<int,int, Json::Value> AI::move() const {
    pair<int, int> next_move = connectMove(*this, graph, info[0].asInt(), info[1].asInt());
    cerr << original_vertex_id(next_move.first) << " " << original_vertex_id(next_move.second) << endl;
    if (next_move.first == -1 || next_move.second == -1) {
      Graph g = get_graph();
      next_move = get_next_greedy(*this, g, get_visited_sites(*this, punter_id), punter_id);
    }
    return make_tuple(next_move.first, next_move.second, info);
  }
}


int main() {
  flowlight::AI ai;
  ai.run();
  return 0;
}
