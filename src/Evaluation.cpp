#include "Game.h"
#include <iostream>
#include <vector>
#include <map>

#include "../jsoncpp/json/json.h"

Json::Value read_json() {
  int length = 0;
  while (true) {
    int c = std::cin.get();
    if (c == ':') {
      break;
    }
    length = length * 10 + (c - '0');
  }

  std::unique_ptr<char[]> json_buf(new char[length + 1]);
  std::cin.read(json_buf.get(), length);

  Json::Value json;
  Json::Reader reader;
  reader.parse(json_buf.get(), json);

  return json;
}

void write_json(const Json::Value& json) {
  std::stringstream ss;
  ss << json;
  std::string json_str = ss.str();
  std::cout << json_str.size() << ":" << json_str << std::endl;
}

int main() {
  Json::Value input = read_json();
  const int n = input["punters"].asInt();
  const Json::Value graph_json = input["map"];

  const auto tmp = Graph::from_json_with_renaming(graph_json);
  Graph graph = tmp.first;
  std::vector<int> reverse_id_map = tmp.second;
  std::map<int, int> id_map;
  for (int i = 0; i < n; ++i) {
    id_map[reverse_id_map[i]] = i;
  }

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

  std::vector<int64_t> score = graph.evaluate(n);
  Json::Value score_json;
  for (int i = 0; i < n; ++i) {
    score_json.append(score[i]);
  }
  write_json(score_json);

  return 0;
}
