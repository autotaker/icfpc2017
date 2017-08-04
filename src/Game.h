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
  Move(Json::Value json);

  bool is_pass() const;
  Json::Value to_json() const;
};
using History = std::vector<Move>;

class Game {
private:
  bool first_turn;
  int num_punters;
  int punter_id;
  Graph graph;
  History history;
  Json::Value info;

  std::vector<int> reverse_id_map;
  std::map<int, int> id_map;
  void calc_id_map();

  Json::Value encode_state(const Json::Value& info) const;
  void decode_state(Json::Value state);

public:
  virtual Json::Value setup() const = 0;
  virtual std::tuple<int, int, Json::Value> move() const = 0;

  void run();
};
