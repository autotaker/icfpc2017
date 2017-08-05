#include "Game.h"

#include "json/json.h"
#include <iostream>
#include <map>
#include <algorithm>
#include <memory>
#include <sstream>
#include <queue>

static const char* PUNTER = "punter";
static const char* PUNTERS = "punters";
static const char* MAP = "map";
static const char* READY = "ready";
static const char* STATE = "state";
static const char* SITES = "sites";
static const char* RIVERS = "rivers";
static const char* MINES = "mines";
static const char* ID = "id";
static const char* SOURCE = "source";
static const char* TARGET = "target";
static const char* MOVE = "move";
static const char* MOVES = "moves";
static const char* CLAIM = "claim";
static const char* PASS = "pass";
static const char* STOP = "stop";
static const char* ME = "me";
// static const char* YOU = "you";

static const char* FIRST_TURN = "first_turn";
static const char* NUM_PUNTERS = "num_punters";
static const char* PUNTER_ID = "punter_id";
static const char* GRAPH = "graph";
static const char* HISTORY = "history";
static const char* INFO = "info";

namespace json_helper {
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
  Json::FastWriter writer;
  std::string json_str = writer.write(json);
  std::cout << json_str.size() << ":" << json_str << std::flush;
}
}

Graph::River::River(int to, int punter)
  : to(to), punter(punter) {

}

bool
Graph::River::operator<(const Graph::River& rhs) const {
  return to < rhs.to;
}

std::tuple<Graph, std::vector<int>, std::map<int, int>>
Graph::from_json(const Json::Value& json) {
  Graph g;
  std::map<int, int> id_map;
  std::vector<int> reverse_id_map;

  g.num_mines = json[MINES].size();
  g.num_vertices = json[SITES].size();

  int cur_id = 0;
  for (const auto& mine : json[MINES]) {
    int mine_id = mine.asInt();
    id_map[mine_id] = cur_id++;
    reverse_id_map.push_back(mine_id);
  }

  for (const auto& site : json[SITES]) {
    int site_id = site[ID].asInt();
    if (id_map.count(site_id)) continue;
    id_map[site_id] = cur_id++;
    reverse_id_map.push_back(site_id);
  }

  g.rivers.resize(g.num_vertices);
  for (const auto& river : json[RIVERS]) {
    int source = id_map[river[SOURCE].asInt()];
    int target = id_map[river[TARGET].asInt()];
    int punter = river.isMember(PUNTER) ? river[PUNTER].asInt() : -1;
    g.rivers[source].emplace_back(target, punter);
    g.rivers[target].emplace_back(source, punter);
  }
  for (int i = 0; i < g.num_vertices; ++i) {
    std::sort(g.rivers[i].begin(), g.rivers[i].end());
  }

  return std::tuple<Graph, std::vector<int>, std::map<int, int>>(g, reverse_id_map, id_map);
}

std::vector<int64_t>
Graph::evaluate(int num_punters) const {
  std::vector<int64_t> scores(num_punters, 0LL);

  for (int mine = 0; mine < num_mines; ++mine) {
    // calculate shortest distances from a mine
    std::vector<int> distances(num_vertices, 1<<29);
    std::queue<int> que;
    que.push(mine);
    distances[mine] = 0;
    while (!que.empty()) {
      const int u = que.front();
      que.pop();
      for (const River& river : rivers[u]) {
        const int v = river.to;
        if (distances[v] > distances[u] + 1) {
          distances[v] = distances[u] + 1;
          que.push(v);
        }
      }
    }

    // calculate score for each punter
    for (int punter = 0; punter < num_punters; ++punter) {
      std::vector<int> visited(num_vertices, 0);
      que = std::queue<int>();
      que.push(mine);
      visited[mine] = 1;
      while (!que.empty()) {
        const int u = que.front();
        que.pop();
        for (const River& river : rivers[u]) if (river.punter == punter) {
          const int v = river.to;
          if (!visited[v]) {
            scores[punter] += distances[v] * distances[v];
            visited[v] = 1;
            que.push(v);
          }
        }
      }
    }
  }

  return scores;
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
Game::handshake() const {
  const std::string myname = name();

  Json::Value json;
  json[ME] = myname;
  json_helper::write_json(json);

  json = json_helper::read_json();
}

void
Game::run() {
  handshake();

  Json::Value json = json_helper::read_json();

  Json::Value res;
  if (json.isMember(PUNTER)) {
    punter_id = json[PUNTER].asInt();
    num_punters = json[PUNTERS].asInt();

    std::tie(graph, reverse_id_map, id_map) = Graph::from_json(json[MAP]);

    history = History();
    first_turn = true;

    Json::Value next_graph = json[MAP];
    for (Json::Value& river : next_graph[RIVERS]) {
      river[PUNTER] = -1;
    }

    const Json::Value next_info = setup();
    const Json::Value state = encode_state(next_info, next_graph);
    res[READY] = json[PUNTER];
    res[STATE] = state;
  } else if (json.isMember(MOVE)) {
    decode_state(json[STATE]);
    const Json::Value moves = json[MOVE][MOVES];
    std::map<std::pair<int, int>, int> claims;
    for (const Json::Value& mv : moves) {
      if (mv.isMember(CLAIM)) {
        const int p = mv[CLAIM][PUNTER].asInt();
        const int org_src = mv[CLAIM][SOURCE].asInt();
        const int org_to = mv[CLAIM][TARGET].asInt();
        const int src = id_map[org_src];
        const int to = id_map[org_to];
        history.emplace_back(p, src, to);
        auto& rs = graph.rivers[src];
        auto& rt = graph.rivers[to];
        std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter = p;
        std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->punter = p;
        claims[{org_src, org_to}] = p;
      } else {
        const int p = mv[PASS][PUNTER].asInt();
        if (!first_turn || p < punter_id) {
          history.emplace_back(p, -1, -1);
        }
      }
    }

    Json::Value next_graph = json[STATE][GRAPH];
    for (Json::Value& river : next_graph[RIVERS]) {
      const int org_src = river[SOURCE].asInt();
      const int org_to = river[TARGET].asInt();
      if (claims.count({org_src, org_to})) {
        river[PUNTER] = claims[{org_src, org_to}];
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
      int org_src = reverse_id_map[src];
      int org_to = reverse_id_map[to];

      bool has_edge = false;
      for (Json::Value& river : json[STATE][GRAPH][RIVERS]) {
        has_edge |= river[SOURCE].asInt() == org_src && river[TARGET].asInt() == org_to;
      }
      if (!has_edge) {
        std::swap(org_src, org_to);
      }
      res[CLAIM][SOURCE] = org_src;
      res[CLAIM][TARGET] = org_to;
    }

    first_turn = false;
    res[STATE] = encode_state(next_info, next_graph);
  }

  if (!json.isMember(STOP)) {
    json_helper::write_json(res);
  }
}

int
Game::original_vertex_id(int vertex_id) const {
  return reverse_id_map[vertex_id];
}

Json::Value
Game::encode_state(const Json::Value& info, const Json::Value& next_graph) const {
  Json::Value state;
  state[FIRST_TURN] = first_turn;
  state[NUM_PUNTERS] = num_punters;
  state[PUNTER_ID] = punter_id;
  state[GRAPH] = next_graph;
  for (const Move& mv : history) {
    state[HISTORY].append(mv.to_json());
  }

  state[INFO] = info;
  return state;
}

void
Game::decode_state(Json::Value state) {
  first_turn = state.isMember(FIRST_TURN);
  num_punters = state[NUM_PUNTERS].asInt();
  punter_id = state[PUNTER_ID].asInt();

  std::tie(graph, reverse_id_map, id_map) = Graph::from_json(state[GRAPH]);

  history = History();
  for (const Json::Value& mv : state[HISTORY]) {
    history.emplace_back(mv);
  }

  info = state[INFO];
}

