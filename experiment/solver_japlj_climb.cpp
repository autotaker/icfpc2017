#include "Game.h"
#include "json/json.h"

#include <iostream>
#include <fstream>

#include <random>

std::mt19937 mt_engine(2);

int randint(int lo, int hi) {
  std::uniform_int_distribution<int> dist(lo, hi);
  return dist(mt_engine);
}

double randreal() {
  std::uniform_real_distribution<double> dist(0.0, 1.0);
  return dist(mt_engine);
}

void claim(Graph& g, int src, int to, int p) {
  auto& rs = g.rivers[src];
  auto& rt = g.rivers[to];
  std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter = p;
  std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->punter = p;
}

int owner(const Graph& g, int src, int to) {
  const auto& rs = g.rivers[src];
  return std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter;
}

int main(int argc, char** argv) {
  char* filename = argv[1];
  const int div = atoi(argv[2]);

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

  const int num_select = num_edges / div;

  std::vector<int> init(num_edges, 0);
  std::vector<std::pair<int, int>> edges(num_edges);
  for (int i = 0; i < num_select; ++i) {
    init[i] = 1;
  }
  std::shuffle(init.begin(), init.end(), mt_engine);

  for (int u = 0, ptr = 0; u < graph.num_vertices; ++u) {
    for (const auto& river : graph.rivers[u]) {
      if (u >= river.to) {
        continue;
      }
      edges[ptr] = {u, river.to};
      if (init[ptr] == 1) {
        claim(graph, u, river.to, 0);
      }
      ++ptr;
    }
  }

  int64_t cur_pt = graph.evaluate(1, dists)[0];
  for (int loop = 0; loop < 1000000; ++loop) {
    int a, b;
    do {
      a = randint(0, num_edges - 1);
    } while (init[a] == 0);
    do {
      b = randint(0, num_edges - 1);
    } while (init[b] == 1);
    claim(graph, edges[a].first, edges[a].second, -1);
    claim(graph, edges[b].first, edges[b].second, 0);
    int64_t nxt_pt = graph.evaluate(1, dists)[0];
    if (cur_pt < nxt_pt) {
      std::cout<<cur_pt<< " -> "<<nxt_pt<<std::endl;
      std::swap(init[a], init[b]);
      cur_pt = nxt_pt;
    } else {
      claim(graph, edges[a].first, edges[a].second, 0);
      claim(graph, edges[b].first, edges[b].second, -1);      
    }
  }

  for (Json::Value& river : json["rivers"]) {
    const int src = id_map[river["source"].asInt()];
    const int to = id_map[river["target"].asInt()];
    int own;
    river["owned_by"] = own = owner(graph, src, to);
    if (own == 0) {
      Json::Value move;
      move["scores"].append(0LL);
      move["claim"]["punter"] = 0;
      move["claim"]["source"] = river["source"];
      move["claim"]["target"] = river["target"];
      LOG["moves"].append(move);
    }
  }

  std::cout << graph.evaluate(1, dists)[0] << std::endl;

  LOG["punters"] = 1;
  LOG["setup"] = json;
  std::ofstream ofs("log.json");
  Json::FastWriter writer;
  std::string json_str = writer.write(LOG);
  ofs << json_str << std::endl;

  return 0;
}
