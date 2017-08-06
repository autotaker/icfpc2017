#include "Game.h"
#include "json/json.h"

#include <iostream>
#include <algorithm>
#include <fstream>
#include <memory>

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

void unclaim_all(Graph& g) {
  for (int i = 0; i < g.num_vertices; ++i) {
    for (auto& river : g.rivers[i]) {
      river.punter = -1;
    }
  }
}

int owner(const Graph& g, int src, int to) {
  const auto& rs = g.rivers[src];
  return std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter;
}

void bfs(const Graph& g, int start, std::vector<int>& prev, std::vector<int>& dist) {
  const int V = g.num_vertices;
  prev.resize(V, -1);
  dist.resize(V, 1<<29);

  int E = 0;
  for (int i = 0; i < V; ++i) {
    E += g.rivers[i].size();
  }
  E /= 2;

  std::unique_ptr<int[]> que(new int[E + 1]);
  int qb = 0, qe = 0;
  que[qe++] = start;
  dist[start] = 0;

  while (qb < qe) {
    const int u = que[qb++];
    for (const auto& river : g.rivers[u]) {
      const int v = river.to;
      if (dist[v] > dist[u] + 1) {
        dist[v] = dist[u] + 1;
        prev[v] = u;
        que[qe++] = v;
      }
    }
  }
}

int64_t path_and_greed(Graph& g, const std::vector<std::vector<int>>& prev, const std::vector<std::vector<int>>& dist, const std::vector<int>& ms, int limit) {
  unclaim_all(g);

  std::vector<int> conn(g.num_vertices, 0);

  int selected = 0;
  for (int i = 0; i+1 < g.num_mines; ++i) {
    int sm = ms[i], tm = ms[i + 1];
    int sel = 0;
    for (int u = tm; u != sm; u = prev[sm][u]) {
      if (owner(g, u, prev[sm][u]) == -1) {
        ++sel;
      }
    }
    if (sel + selected > limit) {
      break;
    }
    for (int u = tm; u != sm; u = prev[sm][u]) {
      if (owner(g, u, prev[sm][u]) == -1) {
        claim(g, u, prev[sm][u], 0);
        conn[u] = conn[prev[sm][u]] = 1;
        ++selected;
      }
    }
  }

  std::vector<int64_t> weight(g.num_vertices, 0LL);
  for (int i = 0; i < g.num_vertices; ++i) {
    for (int m = 0; m < g.num_mines; ++m) {
      // if (conn[m]) {
        weight[i] += dist[m][i] * dist[m][i];
      // }
    }
  }

  while (selected < limit) {
    int64_t best_w = -1;
    int best_u = -1, best_v = -1;
    for (int u = 0; u < g.num_vertices; ++u) {
      if (!conn[u]) {
        continue;
      }
      for (const auto& river : g.rivers[u]) {
        const int v = river.to;
        if (conn[v]) {
          continue;
        }
        if (best_w < weight[v]) {
          best_w = weight[v];
          best_u = u;
          best_v = v;
        }
      }
    }
    if (best_u >= 0) {
      claim(g, best_u, best_v, 0);
      conn[best_v] = 1;
    }
    ++selected;
  }

  return g.evaluate(1, dist)[0];
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

  std::vector<std::vector<int>> prev(graph.num_mines), dist(graph.num_mines);
  for (int s = 0; s < graph.num_mines; ++s) {
    bfs(graph, s, prev[s], dist[s]);
  }

  std::vector<int> ms(graph.num_mines);
  for (int i = 0; i < graph.num_mines; ++i) {
    ms[i] = i;
  }
  std::shuffle(ms.begin(), ms.end(), mt_engine);

  int64_t score = path_and_greed(graph, prev, dist, ms, num_select);
  for (int loop = 0; loop < 10000; ++loop) {
    int a = randint(0, graph.num_mines - 1);
    int b = randint(0, graph.num_mines - 1);
    if (a == b) {
      --loop;
      continue;
    }

    std::swap(ms[a], ms[b]);
    int64_t nxt_score = path_and_greed(graph, prev, dist, ms, num_select);
    if (score < nxt_score) {
      std::cerr<<score<<" -> "<<nxt_score<<std::endl;
      score = nxt_score;
    } else {
      std::swap(ms[a], ms[b]);
    }
  }

  std::cout << path_and_greed(graph, prev, dist, ms, num_select) << std::endl;

  for (Json::Value& river : json["rivers"]) {
    const int src = id_map[river["source"].asInt()];
    const int to = id_map[river["target"].asInt()];
    int own;
    river["owned_by"] = own = owner(graph, src, to);
    if (own == 0) {
      Json::Value move;
      move["scores"].append((int64_t) 0LL);
      move["claim"]["punter"] = 0;
      move["claim"]["source"] = river["source"];
      move["claim"]["target"] = river["target"];
      LOG["moves"].append(move);
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
