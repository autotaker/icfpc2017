#pragma once

#include <vector>
#include "../jsoncpp/json/json.h"

struct Graph {
  struct River {
    int to;
    int punter;
    River(int to, int punter = -1);
  };

  int num_mines;
  int num_vertices;
  std::vector<std::vector<River>> rivers;

  Json::Value to_json() const;

  static Graph from_json(const Json::Value &json);
  static std::pair<Graph, std::vector<int>>
    from_json_with_renaming(const Json::Value &json);
};

struct Move {
  int punter;
  int src;
  int to;
  Move(int punter, int src, int to);

  bool is_pass() const;
};
using History = std::vector<Move>;

class Game {
private:
  int num_punters;
  int punter_id;
  Graph graph;
  History history;
  Json::Value info;

  Json::Value encode_state(Json::Value info) const;

public:
  virtual Json::Value setup() const = 0;
  virtual std::tuple<int, int, Json::Value> move() const = 0;

  void run();
};
