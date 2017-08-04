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

  Json::Value encode_state(const Json::Value& info) const;

public:
  virtual Json::Value setup() const = 0;
  virtual std::tuple<int, int, Json::Value> move() const = 0;

  void run();
};
