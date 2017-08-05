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
    River(int to, int punter = -1);
    bool operator<(const River& rhs) const;
  };

  int num_mines;
  int num_vertices;
  std::vector<std::vector<River>> rivers;

  static std::tuple<Graph, std::vector<int>, std::map<int, int>>
    from_json(const Json::Value& json);

  std::vector<int64_t> evaluate(int num_punters) const;
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

class Game {
protected:
  int num_punters;
  int punter_id;
  Graph graph;
  History history;
  Json::Value info;

  int original_vertex_id(int vertex_id) const;

  // futures
  bool futures_enabled;
  std::vector<int> futures;
  void buy_future(int mine, int site);

private:
  bool first_turn;
  std::vector<int> reverse_id_map;
  std::map<int, int> id_map;

  Json::Value encode_state(const Json::Value& info, const Json::Value& next_graph) const;
  void decode_state(Json::Value state);

  void handshake() const;

public:
  const Graph& get_graph() const {
    return graph;
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

  virtual Json::Value setup() const = 0;
  virtual std::tuple<int, int, Json::Value> move() const = 0;

  virtual std::string name() const { return "AI"; };

  void run();
};
