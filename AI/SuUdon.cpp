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

#include "../lib/Game.h"

class SuUdonAI : public Game {
  SetupSettings setup() const override;
  std::tuple<int, int, Json::Value> move() const override;


  bool is_free_river(const Graph::River&) const;
  // Calculate the shortest paths without using opponents' rivers
  std::vector<std::vector<int>> calc_my_shortest_distances() const;
};

bool SuUdonAI::is_free_river(const Graph::River &river) const {
  return river.punter == punter_id || river.punter == -1;
}

std::vector<std::vector<int>>
SuUdonAI::calc_my_shortest_distances() const {
  int num_edges = 0;
  for (int i = 0; i < graph.num_vertices; ++i) {
    for (auto river : graph.rivers[i]) {
      if (is_free_river(river)) {
        num_edges++;
      }
    }
  }

  std::unique_ptr<int[]> que(new int[(num_edges /= 2) + 1]);

  std::vector<std::vector<int>> distances;
  for (int mine = 0; mine < graph.num_mines; ++mine) {
    // calculate shortest distances from a mine
    std::vector<int> dist(graph.num_vertices, 1<<29);
    int qb = 0, qe = 0;
    que[qe++] = mine;
    dist[mine] = 0;
    while (qb < qe) {
      const int u = que[qb++];
      for (const Graph::River& river : graph.rivers[u]) {
        if (is_free_river(river)) {
          const int v = river.to;
          if (dist[v] > dist[u] + 1) {
            dist[v] = dist[u] + 1;
            que[qe++] = v;
          }
        }
      }
    }
    distances.push_back(std::move(dist));
  }

  return distances;
}

SetupSettings SuUdonAI::setup() const {
  return Json::Value();
}

std::tuple<int, int, Json::Value> SuUdonAI::move() const {
  srand(punter_id);

  std::set<int> visited;

  for (const Json::Value& node : info) {
    visited.insert(node.asInt());
  }

  auto orig_dists =  graph.calc_shortest_distances();
  auto my_dists =  calc_my_shortest_distances();

  int all_turns = 0;
  for (auto r: graph.rivers) {
    all_turns += r.size();
  }
  all_turns /= 2;
  all_turns /= num_punters;

  const int cur_turn = history.size() / num_punters;

  int best_s = -1;
  int best_t = -1;


  bool frontier_mode = (cur_turn == 0);

  // There exists a mine that is reachable and not visited. => connection mode
  // otherwise => greedy mode
  bool connection_mode = false;
  for(int i = 0; i < graph.num_mines; ++i) {
    if(visited.find(i) == visited.end()) {
      for(int v: visited) {
        if (my_dists[i][v] < all_turns - cur_turn) {
          connection_mode = true;
          break;
        }
      }
    }
    if (connection_mode) break;
  }

  if (cur_turn == 0) {
    std::cerr<< "My Player ID = " << punter_id <<std::endl;
  }
  std::cerr<< "===========" <<std::endl;
  std::cerr<< "Turn " << (cur_turn * 2) + punter_id << " (" << cur_turn << ")" <<std::endl;
  std::cerr<< "visited: [";
  for(auto v: visited) {
    std::cerr<< v << ", ";
  }
  std::cerr<< "]" <<std::endl;

  if (!frontier_mode && connection_mode) { // Try to connect mines
    std::cerr << "Connection Mode!" << std::endl;
    int64_t best_pt = 1<<29; // smaller is better

    for (int u : visited) {
      for (const auto& river : graph.rivers[u]) {
        if (river.punter == -1 && visited.find(river.to) == visited.end()) {
          int pt = 1<<29;

          for(int i = 0; i < graph.num_mines; ++i) {
            if (visited.find(i) == visited.end()) {
              pt = std::min(my_dists[i][river.to], pt);
            }
          }

          if (pt < best_pt) {
            best_pt = pt;
            best_s = u;
            best_t = river.to;
          }
        }
      }
    }

  } else if (!frontier_mode) { // Greedy mode
    std::cerr << "Greedy Mode!" << std::endl;
    int64_t best_pt = -1; // larger is better
    for (int u : visited) {
      for (const auto& river : graph.rivers[u]) {
        if (river.punter == -1 && visited.find(river.to) == visited.end()) {
          int64_t pt = 0;
          for (int i = 0; i < graph.num_mines; ++i) {
            if (visited.find(i) != visited.end()) {
              pt += orig_dists[i][river.to] * orig_dists[i][river.to];
            }
          }
          if (best_pt < pt) {
            best_pt = pt;
            best_s = u;
            best_t = river.to;
          }
        }
      }
    }
  }

  frontier_mode |= (best_t < 0);

  if (frontier_mode) {
    std::cerr << "Frontier mode!" << std::endl;

    // Try to get a disconnected new mine
    int r = rand();
    for(int i = 0; i < graph.num_mines; ++i) {
      int u = (i + r) % graph.num_mines;
      if (visited.find(u) != visited.end())
        continue;
      for (const auto& river : graph.rivers[u]) {
        if (river.punter == -1) {
          best_s = u;
          best_t = river.to;
          goto END;
        }
      }
    }

    // Try to get a disconnected new site
    int site_num = (graph.num_vertices - graph.num_mines);
    r = rand();
    for(int i = 0; i < site_num; ++i) {
      int u = (i + r) % site_num + graph.num_mines;
      if (visited.find(u) != visited.end())
        continue;
      for (const auto& river : graph.rivers[u]) {
        if (river.punter == -1) {
          best_s = u;
          best_t = river.to;
          goto END;
        }
      }
    }

    // If it is impossible, any unobtained river is allowed
    r = rand();
    for(int i = 0; i < graph.num_vertices; ++i) {
      int u = (i + r) % graph.num_vertices;
      for (const auto& river : graph.rivers[u]) {
        if (river.punter == -1) {
          best_s = u;
          best_t = river.to;
          goto END;
        }
      }
    }

  }

 END:

  if (0 <= best_s && 0 <= best_t) {
    if (visited.find(best_s) == visited.end()) {
      std::cerr << "Get " << ((best_s < graph.num_mines) ? "mine " : "site ") << best_s << "!" << std::endl;
    }
    if (visited.find(best_t) == visited.end()) {
      std::cerr << "Get " << ((best_t < graph.num_mines) ? "mine " : "site ") << best_t << "!!" << std::endl;
    }

    visited.insert(best_s);
    visited.insert(best_t);
  } else {
    std::cerr << "PASS!" << std::endl;
  }

  Json::Value info = Json::Value();
  int i = 0;
  for(auto v : visited) {
    info[i++] = v;
  }
  return std::make_tuple(best_s, best_t, info);
}

int main()
{
  SuUdonAI ai;
  ai.run();
  return 0;
}
