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
#include <ctime>

#include "../lib/Game.h"
#include "../lib/MCTS_core.h"
using namespace std;
class KakeUdonAI : public Game {
  SetupSettings setup() const override;
  MoveResult move() const override;

  bool is_free_river(const Graph::River&) const;
  // Calculate the shortest paths without using opponents' rivers
  std::vector<std::vector<int>> calc_my_shortest_distances() const;
  std::vector<int> calc_shortest_distance(int v) const;
  pair<int,int> decideFeature () const;
};

bool KakeUdonAI::is_free_river(const Graph::River &river) const {
  return river.punter == punter_id || river.punter == -1;
}

std::vector<int>
KakeUdonAI::calc_shortest_distance(int v) const {
  int num_edges = 0;
  for (int i = 0; i < graph.num_vertices; ++i) {
    for (auto river : graph.rivers[i]) {
      if (is_free_river(river)) {
        num_edges++;
      }
    }
  }

  std::unique_ptr<int[]> que(new int[(num_edges /= 2) + 1]);

  // calculate shortest distances from a mine
  std::vector<int> dist(graph.num_vertices, 1<<29);
  int qb = 0, qe = 0;
  que[qe++] = v;
  dist[v] = 0;
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
  return dist;
}


std::vector<std::vector<int>>
KakeUdonAI::calc_my_shortest_distances() const {
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

#define LIMIT_DISTANCE_THRESHOLD 0.15

pair<int,int> KakeUdonAI::decideFeature() const {
  int num_mine_edges = 0;
  for (int i = 0; i < graph.num_mines; i++) {
    num_mine_edges += graph.rivers[i].size();
  }
  if (num_mine_edges < 3 * num_punters) {
    return make_pair(-1,-1);
  }
  auto dists =  graph.calc_shortest_distances();
  auto num_of_turns = graph.num_edges / num_punters;
  auto limit_dist = (int) floor(num_of_turns * LIMIT_DISTANCE_THRESHOLD);

  // TODO(hiroh): smarter decidion for mine and site
  srand(time(NULL));
  int best_dist = 0;
  pair<int,int> best_pair{-1,-1};
  for(int i = 0; i < graph.num_mines; ++i) {
    int r = rand();
    int u = (i + r) % graph.num_mines;
    int site_num = (graph.num_vertices - graph.num_mines);
    for(int j = 0; j < site_num; ++j) {
      r = rand();
      int v = (i + r) % site_num + graph.num_mines;
      if (dists[u][v] < limit_dist) {
        auto d = dists[u][v];
        auto current_v = best_pair.second;
        if (d > best_dist) {
          best_dist = d;
          best_pair = make_pair(u, v);
        } else if (d == best_dist && graph.rivers[v].size() > graph.rivers[current_v].size()) {
          best_pair = make_pair(u, v);
        }
      }
    }
  }
  return best_pair;
}

SetupSettings KakeUdonAI::setup() const {
  auto fe = decideFeature();
  auto src = fe.first;
  auto target = fe.second;
  std::vector<int> futures;
  for (int i = 0; i < graph.num_mines; i++) {
    futures.push_back(i == src ? target : -1);
  }
  Json::Value info;
  info[0] = Json::Value(); //visited_nodes
  info[1][0] = src;
  info[1][1] = target;
  return SetupSettings(info, futures);
}

MoveResult KakeUdonAI::move() const {
  std::set<int> visited;

  for (const Json::Value& node : info[0]) {
    visited.insert(node.asInt());
  }

  auto orig_dists =  graph.calc_shortest_distances();
  auto my_dists =  calc_my_shortest_distances();


  const int all_turns = graph.num_edges;
  const int cur_turn = history.size() / num_punters;

  int best_s = -1;
  int best_t = -1;

  bool frontier_mode = (cur_turn == 0);

  int feature_target = info[1][1].asInt();
  // There exists a mine that is reachable and not visited. => connection mode
  // otherwise => greedy mode
  bool connection_mode = false;
  
  if (cur_turn == 0) {
    // should try to connect the sourced mine.
    int src_mine = info[1][0].asInt();
    int target_site = info[1][0].asInt();
    if (src_mine != -1) {
      int best_next_vertex = -1;
      // TODO(hiroh): better way to decide a selected vertex.
      auto dist_from_feature_target = calc_shortest_distance(feature_target);
      for (const auto& r : graph.rivers[src_mine]) {
        if (r.punter != -1 || best_next_vertex == target_site) {
          continue;
        }
        if (dist_from_feature_target[r.to] == dist_from_feature_target[src_mine] - 1) {
          best_next_vertex = r.to;
        }
      }
      if (best_next_vertex != -1) {
        best_s = src_mine;
        best_t = best_next_vertex;
        goto END;
      }
    }
  }
  
  //srand(punter_id + cur_turn);
  //srand(time(NULL));

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
  std::cerr << __LINE__ << endl;
  if (!connection_mode && feature_target != -1 && visited.find(feature_target) == visited.end()) {
    // If connection mode is false and feeature_target can be connected from mines, connection mode become false.
    for(int i = 0; i < graph.num_mines; ++i) {
      if (visited.find(i) == visited.end()) {
        continue;
      }
      if (my_dists[i][feature_target] < all_turns - cur_turn) {
        connection_mode = true;
        break;
      }
    }
  }
  std::cerr << __LINE__ << endl;
  
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
  std::cerr << __LINE__ << endl;
  if (!frontier_mode && connection_mode) { // Try to connect mines
    std::cerr << "Connection Mode!" << std::endl;
    int64_t best_pt = 1<<29; // smaller is better
    std::vector<int> dist_from_feature_target;
    if (feature_target != -1 && visited.find(feature_target) == visited.end()) {
      dist_from_feature_target = calc_shortest_distance(feature_target);
    }
    for (int u : visited) {
      for (const auto& river : graph.rivers[u]) {
        if (river.punter == -1 && visited.find(river.to) == visited.end()) {
          int pt = 1<<29;

          if (feature_target != -1 && visited.find(feature_target) == visited.end()) {
            pt = std::min(dist_from_feature_target[river.to], pt);
          }
          
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
    KakeUdonAI g = *this;
    MCTS_Core core(&g, 0.5);
    int timelimit_ms = 900;
    auto p = core.get_play(timelimit_ms);
    best_s = p.first;
    best_t = p.second;
    goto END;
    //std::cerr << "Greedy Mode!" << std::endl;
    // int64_t best_pt = -1; // larger is better
    // for (int u : visited) {
    //   for (const auto& river : graph.rivers[u]) {
    //     if (river.punter == -1 && visited.find(river.to) == visited.end()) {
    //       int64_t pt = 0;
    //       for (int i = 0; i < graph.num_mines; ++i) {
    //         if (visited.find(i) != visited.end()) {
    //           pt += orig_dists[i][river.to] * orig_dists[i][river.to];
    //         }
    //       }
    //       if (best_pt < pt) {
    //         best_pt = pt;
    //         best_s = u;
    //         best_t = river.to;
    //       }
    //     }
    //   }
    // }
  }

  frontier_mode |= (best_t < 0);
  std::cerr << __LINE__ << endl;
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

  Json::Value ninfo;
  int i = 0;
  for(auto v : visited) {
    ninfo[0][i++] = v;
  }
  ninfo[1] = info[1];
  std::cerr << "end!" << std::endl;
  return std::make_tuple(best_s, best_t, ninfo);
}

int main()
{
  KakeUdonAI ai;
  ai.run();
  return 0;
}
