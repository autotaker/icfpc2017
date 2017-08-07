#include "FlowlightUtil.h"
#include <queue>
#include <cassert>
using namespace std;

void bfs(const Graph &g, int source, int target, int punter_id, vector<int> &distance) {
  distance[source] = 0;
  deque<int> que;
  que.push_back(source);
  while (!que.empty()) {
    const int v = que.front(); que.pop_front();
    const int d = distance[v];

    if (v == target) {
      break;
    }
      
    for (const auto &river: g.rivers[v]) {
      const int w = river.to;
      if (river.punter == -1) {
        const int nd = d + 1;
        if (nd < distance[w]) {
          que.push_back(w);
          distance[w] = nd;
        }
      } else if (river.punter == punter_id) {
        const int nd = d;
        if (nd < distance[w]) {
          que.push_front(w);
          distance[w] = nd;
        }
      } else {
        // do nothing
      }
    }
  }
}

namespace flowlight {
  int get_num_edges(const Graph &graph) {
    int num_edges = 0;
    for (int i = 0; i < graph.num_vertices; i++) {
      num_edges += graph.rivers[i].size();
    }
    return num_edges / 2;
  }

  int is_passable_without_option(const Graph::River &river, int punter) {
    return river.punter == -1 || river.punter == punter;
  }
  
  Graph build_shortest_path_dag(const Graph &original_graph, int source, int target, int punter_id) {
    Graph g;
    g.num_mines = original_graph.num_mines;
    g.num_edges = get_num_edges(original_graph);
    g.num_vertices = original_graph.num_vertices;
    g.rivers = vector<vector<Graph::River>>(g.num_vertices);

    vector<int> distance_from_source(g.num_vertices, g.num_vertices);
    vector<int> distance_from_target(g.num_vertices, g.num_vertices);
    bfs(original_graph, source, target, punter_id, distance_from_source);
    bfs(original_graph, target, source, punter_id, distance_from_target);
    assert(distance_from_source[target] == distance_from_target[source]);
    
    const int spd = distance_from_source[target];
    for (int v = 0; v < original_graph.num_vertices; v++) {
      for (const auto &river: original_graph.rivers[v]) {
        const int w = river.to;
        if (river.punter == -1) {
          if ((spd == distance_from_source[v] + 1 + distance_from_target[w]) ||
              (spd == distance_from_source[w] + 1 + distance_from_target[v])) {
            g.rivers[v].push_back(river);
          }
        } else if (river.punter == punter_id) {
          if ((spd == distance_from_source[v] + distance_from_target[w]) ||
              (spd == distance_from_source[w] + distance_from_target[v])) {
            g.rivers[v].push_back(river);
          }
        }
      }
    }
    return g;
  }
}
