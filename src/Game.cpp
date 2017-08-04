#include "Game.h"

#include "../jsoncpp/json/json.h"
#include <iostream>
#include <memory>
#include <sstream>

static const char* PUNTER = "punter";
static const char* PUNTERS = "punters";
static const char* MAP = "map";
static const char* READY = "ready";
static const char* STATE = "state";
static const char* MOVE = "move";
static const char* CLAIM = "claim";
static const char* PASS = "pass";
static const char* SOURCE = "source";
static const char* TARGET = "target";
static const char* STOP = "target";

Graph::River::River(int to, int punter)
  : to(to), punter(punter) {

}

Move::Move(int punter, int src, int to)
  : punter(punter), src(src), to(to) {

}

Move::Move(Json::Value json) {
  punter = json[PUNTER].asInt();
  src = json[SOURCE].asInt();
  to = json[TARGET].asInt();
}

bool
Move::is_pass() const {
  return src == -1;
}

Json::Value
Move::to_json() const {
  Json::Value json;
  json[PUNTER] = punter;
  json[SOURCE] = src;
  json[TARGET] = to;
  return json;
}

void
Game::run() {
  int length = 0;
  while (true) {
    int c = std::cin.get();
    if (c == ':') {
      break;
    }
    length = length * 10 + (c - '0');
  }

  std::unique_ptr<char> json_buf(new char[length + 1]);
  std::cin.read(json_buf.get(), length);

  Json::Value json;
  Json::Reader reader;
  reader.parse(json_buf.get(), json);

  Json::Value res;
  if (json.isMember(PUNTER)) {
    punter_id = json[PUNTER].asInt();
    num_punters = json[PUNTERS].asInt();

    const auto tmp = Graph::from_json_with_renaming(json[MAP]);
    graph = tmp.first;
    reverse_id_map = tmp.second;
    calc_id_map();

    history = History();
    first_turn = true;

    const Json::Value next_info = setup();
    const Json::Value state = encode_state(next_info);
    res[READY] = json[PUNTER];
    res[STATE] = state;
  } else if (json.isMember(MOVE)) {
    decode_state(json);
    const Json::Value moves = json[MOVE];
    for (const Json::Value& mv : moves) {
      if (mv.isMember(CLAIM)) {
        const int p = mv[CLAIM][PUNTER].asInt();
        const int src = id_map[mv[CLAIM][SOURCE].asInt()];
        const int to = id_map[mv[CLAIM][TARGET].asInt()];
        history.emplace_back(p, src, to);
        for (auto& river : graph.rivers[src]) {
          if (river.to == to) {
            river.punter = p;
          }
        }
        for (auto& river : graph.rivers[to]) {
          if (river.to == src) {
            river.punter = p;
          }
        }
      } else {
        const int p = mv[PASS][PUNTER].asInt();
        if (!first_turn || p < punter_id) {
          history.emplace_back(p, -1, -1);
        }
      }
    }

    int src, to;
    Json::Value next_info;
    std::tie(src, to, next_info) = move();
    
    Json::Value json_move;
    if (src == -1) {
      res[PASS][PUNTER] = punter_id;
    } else {
      res[CLAIM][PUNTER] = punter_id;
      res[CLAIM][SOURCE] = reverse_id_map[src];
      res[CLAIM][TARGET] = reverse_id_map[to];
    }

    first_turn = false;
    res[STATE] = encode_state(next_info);
  }

  if (!json.isMember(STOP)) {
    std::stringstream ss;
    ss << res;
    std::string json_str = ss.str();
    std::cout << json_str.size() << ":" << json_str << std::endl;
  }
}

static const char* FIRST_TURN = "first_turn";
static const char* NUM_PUNTERS = "num_punters";
static const char* PUNTER_ID = "punter_id";
static const char* GRAPH = "graph";
static const char* HISTORY = "history";
static const char* INFO = "info";
static const char* REVERSE_ID_MAP = "reverse_id_map";

Json::Value
Game::encode_state(Json::Value info) const {
  Json::Value state;
  state[FIRST_TURN] = first_turn;
  state[NUM_PUNTERS] = num_punters;
  state[PUNTER_ID] = punter_id;
  state[GRAPH] = graph.to_json();
  for (const Move& mv : history) {
    state[HISTORY].append(mv.to_json());
  }
  for (int i = 0; i < graph.num_vertices; ++i) {
    state[REVERSE_ID_MAP].append(reverse_id_map[i]);
  }

  state[INFO] = info;
  return state;
}

void
Game::decode_state(Json::Value state) {
  first_turn = state.isMember(FIRST_TURN);
  num_punters = state[NUM_PUNTERS].asInt();
  punter_id = state[PUNTER_ID].asInt();
  graph = Graph::from_json(state[GRAPH]);

  history = History();
  for (const Json::Value& mv : state[HISTORY]) {
    history.emplace_back(mv);
  }
  reverse_id_map.clear();
  for (int i = 0; i < graph.num_vertices; ++i) {
    reverse_id_map.push_back(state[REVERSE_ID_MAP][i].asInt());
  }
  calc_id_map();

  info = state[INFO];
}

void
Game::calc_id_map() {
  for (int i = 0; i < graph.num_vertices; ++i) {
    id_map[reverse_id_map[i]] = i;
  }
}
