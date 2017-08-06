#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include "json/json.h"

namespace json_helper {

Json::Value read_json();
void write_json(const Json::Value& json);

}

struct Graph {
  struct River {
    int to;
    int punter;
    River(){}
    River(int to, int punter = -1) : to{to}, punter{punter} {}
    bool operator<(const River& rhs) const;
  };

  int num_mines;
  int num_vertices;
  int num_edges;    // the number of undirected edges
  std::vector<std::vector<River>> rivers;

  static Graph from_json(const Json::Value& json);
  static std::tuple<Graph, std::vector<int>, std::map<int, int>>
    from_json_setup(const Json::Value& json);

  Json::Value to_json() const;

  std::vector<int64_t> evaluate(
    int num_punters,
    const std::vector<std::vector<int>>& distances) const;
  std::vector<int64_t> evaluate(
    int num_punters,
    const std::vector<std::vector<int>>& distances,
    int my_punter_id, const std::vector<int>& futures, int64_t& future_score) const;

  std::vector<std::vector<int>> calc_shortest_distances() const;
  int64_t evaluate_future(
    int punter_id, const std::vector<int>& futures,
    const std::vector<std::vector<int>>& distances) const;
};

struct Move {
  int punter;
  int src;
  int to;
  Move(int punter, int src, int to);
  Move(Json::Value json);

  bool is_pass() const;
  Json::Value to_json() const;
};
using History = std::vector<Move>;

struct SetupSettings {
  Json::Value info;
  std::vector<int> futures;

  SetupSettings(
    const Json::Value& info,
    const std::vector<int>& futures = {});
};

class Game {
protected:
  const int INF = 1001001001;

  int num_punters;
  int punter_id;
  Graph graph;
  History history;
  std::vector<std::vector<int>> shortest_distances;
  Json::Value info;

  int original_vertex_id(int vertex_id) const;

  // futures
  bool futures_enabled;
  std::vector<int> futures;

  void calc_cur_dists(std::vector<std::vector<int>>& dists, std::vector<std::vector<int>>& prevs) const;

private:
  bool first_turn;
  std::vector<int> reverse_id_map;
  std::map<int, int> id_map;

  Json::Value encode_state(const Json::Value& info) const;
  void decode_state(Json::Value state);

  void handshake() const;

public:
  const std::vector<int>& get_futures() const {
    return futures;
  }

  const Graph& get_graph() const {
    return graph;
  }

  Graph* mutable_graph() {
    return &graph;
  }

  const std::vector<std::vector<int>>& get_shortest_distances() const {
    return shortest_distances;
  }

  int get_punter_id() const {
    return punter_id;
  }

  int get_num_punters() const {
    return num_punters;
  }

  const History& get_history() const {
    return history;
  }

  virtual SetupSettings setup() const = 0;
  virtual std::tuple<int, int, Json::Value> move() const = 0;

  virtual std::string name() const { return "AI"; };

  void run();
};
