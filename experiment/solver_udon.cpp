#include "Game.h"
#include "json/json.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <set>

void claim(Graph& g, int src, int to, int p) {
  auto& rs = g.rivers[src];
  auto& rt = g.rivers[to];
  std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter = p;
  std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->punter = p;
}

std::vector<std::vector<int>> g_dists;
int g_num_edges;

double eval_greedy(Graph& g, int src, int to) {
  claim(g, src, to, 0);
  int64_t pt = g.evaluate(1, g_dists)[0];
  claim(g, src, to, -1);
  return pt;
}

double eval_connect(Graph& g, int src, int to, std::set<int>& visited) {
  int pt = 10000000;
  for(int i = 0; i < g.num_mines; ++i) {
    if (visited.find(i) == visited.end()) {
      pt = std::min(g_dists[i][to], pt);
    }
  }
  return -pt;
}

int main(int argc, char** argv) {
  char* filename = argv[1];
  int min_div = std::stoi(argv[2]);
  std::cout<<min_div<<std::endl;

  std::ifstream ifs(filename);
  Json::Value json;
  Json::Reader reader;
  reader.parse(ifs, json);

  Graph graph;
  std::vector<int> reverse_id_map;
  std::map<int, int> id_map;
  std::tie(graph, reverse_id_map, id_map) = Graph::from_json_setup(json);

  Json::Value LOG;

  g_dists = graph.calc_shortest_distances();
  auto dists = g_dists;

  int num_edges = 0;
  for (int i = 0; i < graph.num_vertices; ++i) {
    num_edges += graph.rivers[i].size();
  }
  num_edges /= 2;
  g_num_edges = num_edges;

  const int divs[] = {2, 3, 4, 8, 12, 16};

  int64_t cur_pt = 0;
  bool greedy_mode = false;
  std::set<int> visited_vs;
  visited_vs.insert(std::stoi(argv[3]));
  for (int i = 0; i < num_edges; ++i) {
    std::cout<<'\r'<<i<<' '<<num_edges<<' '<<cur_pt<<std::flush;

    bool flag = true;
    for(int i = 0; i < graph.num_mines; ++i) {
      flag &= (visited_vs.find(i) != visited_vs.end());
    }
    greedy_mode = flag;

    int best_s = -1, best_t = -1;
    if (greedy_mode) {
      double best_pt = -1;
      for (int u = 0; u < graph.num_vertices; ++u) {
        for (const auto& river : graph.rivers[u]) {
          if (u < river.to && river.punter == -1) {
            double pt = eval_greedy(graph, u, river.to);
            if (best_pt < pt) {
              best_pt = pt;
              best_s = u;
              best_t = river.to;
            }
          }
        }
      }
      claim(graph, best_s, best_t, 0);
      cur_pt = graph.evaluate(1, dists)[0];
    } else {
      double best_pt = -100000000;

      for (int u : visited_vs) {
        for (const auto& river : graph.rivers[u]) {
          if (river.punter == -1) {
            double pt = eval_connect(graph, u, river.to, visited_vs);
            if (best_pt < pt) {
              best_pt = pt;
              best_s = u;
              best_t = river.to;
            }
          }
        }
      }
      claim(graph, best_s, best_t, 0);
      cur_pt = graph.evaluate(1, dists)[0];
      visited_vs.insert(best_s);
      visited_vs.insert(best_t);
    }
    Json::Value move;
    int os, ot;
    move["scores"].append(cur_pt);
    move["claim"]["punter"] = 0;
    move["claim"]["source"] = os = reverse_id_map[best_s];
    move["claim"]["target"] = ot = reverse_id_map[best_t];
    LOG["moves"].append(move);

    for (Json::Value& river : json["rivers"]) {
      int rs = river["source"].asInt();
      int rt = river["target"].asInt();
      if ((rs == os && rt == ot) || (rs == ot && rt == os)) {
        river["owned_by"] = 0;
      }
    }

    const int selected = i + 1;
    for (int j = 0; j < 6; ++j) {
      if (num_edges / divs[j] == selected) {
        std::cout << "\n*****" << std::endl;
        std::cout << divs[j] << " " << cur_pt << std::endl;
        std::cout << "*****" << std::endl;
        if (divs[j] == min_div) {
          goto BREAK;
        }
      }
    }
  }
 BREAK:

  for (Json::Value& river : json["rivers"]) {
    if (!river.isMember("owned_by")) {
      river["owned_by"] = -1;
    }
  }

  LOG["punters"] = 1;
  LOG["setup"] = json;
  std::ofstream ofs("log.json");
  Json::FastWriter writer;
  std::string json_str = writer.write(LOG);
  ofs << json_str << std::endl;

  return 0;
}
