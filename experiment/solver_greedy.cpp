#include "Game.h"
#include "json/json.h"

#include <iostream>
#include <fstream>

void claim(Graph& g, int src, int to, int p) {
  auto& rs = g.rivers[src];
  auto& rt = g.rivers[to];
  std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter = p;
  std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->punter = p;
}

int main(int argc, char** argv) {
  char* filename = argv[1];

  std::ifstream ifs(filename);
  Json::Value json;
  Json::Reader reader;
  reader.parse(ifs, json);

  Graph graph;
  std::vector<int> reverse_id_map;
  std::map<int, int> id_map;
  std::tie(graph, reverse_id_map, id_map) = Graph::from_json_setup(json);

  Json::Value LOG;

  const auto dists = graph.calc_shortest_distances();

  int num_edges = 0;
  for (int i = 0; i < graph.num_vertices; ++i) {
    num_edges += graph.rivers[i].size();
  }
  num_edges /= 2;

  const int divs[] = {2, 3, 4, 8, 12, 16};

  int64_t cur_pt = 0;
  for (int i = 0; i < num_edges; ++i) {
    std::cout<<'\r'<<i<<' '<<num_edges<<' '<<cur_pt<<std::flush;
    int64_t best_pt = -1;
    int best_s = -1, best_t = -1;
    for (int u = 0; u < graph.num_vertices; ++u) {
      for (const auto& river : graph.rivers[u]) {
        if (u < river.to && river.punter == -1) {
          claim(graph, u, river.to, 0);
          int64_t pt = graph.evaluate(1, dists)[0];
          if (best_pt < pt) {
            best_pt = pt;
            best_s = u;
            best_t = river.to;
          }
          claim(graph, u, river.to, -1);
        }
      }
    }

    cur_pt = best_pt;
    claim(graph, best_s, best_t, 0);

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
      }
    }
  }

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
