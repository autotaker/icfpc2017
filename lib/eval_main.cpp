#include "Game.h"
#include <iostream>
#include <vector>
#include <map>
#include <memory>

#include "json/json.h"

int main() {
  Json::Value input = json_helper::read_json();
  const int n = input["punters"].asInt();
  const Json::Value graph_json = input["map"];

  Graph graph;
  std::vector<int> reverse_id_map;
  std::map<int, int> id_map;
  std::tie(graph, reverse_id_map, id_map) = Graph::from_json_setup(graph_json);

  std::map<std::pair<int, int>, int> owned_by;
  const Json::Value rivers = graph_json["rivers"];
  for (const auto& river : rivers) {
    const int src = id_map[river["source"].asInt()];
    const int dst = id_map[river["target"].asInt()];
    const int owner = river["owned_by"].asInt();
    owned_by[{src, dst}] = owned_by[{dst, src}] = owner;
  }

  for (int i = 0; i < graph.num_vertices; ++i) {
    for (auto& river : graph.rivers[i]) {
      const auto t = std::make_pair(i, river.to);
      if (owned_by.count(t)) {
        river.punter = owned_by[t];
      }
    }
  }

  const auto dists = graph.calc_shortest_distances();
  std::vector<int64_t> score = graph.evaluate(n, dists);
  std::vector<int64_t> score_future(n, 0LL);

  if (input.isMember("futures")) {
    const Json::Value& futures_all = input["futures"];
    for (int punter = 0; punter < n; ++punter) {
      const Json::Value& futures_json = futures_all[punter];

      std::vector<int> futures(graph.num_mines, -1);
      for (const Json::Value& future_entry : futures_json) {
        const int src = id_map[future_entry["source"].asInt()];
        const int dst = id_map[future_entry["target"].asInt()];
        futures[src] = dst;
      }

      score_future[punter] = graph.evaluate_future(punter, futures, dists);
    }
  }

  Json::Value score_json;
  for (int i = 0; i < n; ++i) {
    Json::Value score_info;
    score_info["basic_score"] = score[i];
    score_info["futures_score"] = score_future[i];
    score_info["score"] = score[i] + score_future[i];
    score_json.append(score_info);
  }
  json_helper::write_json(score_json);

  return 0;
}
