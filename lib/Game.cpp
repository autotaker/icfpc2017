#include "Game.h"

#include "json/json.h"
#include <iostream>
#include <map>
#include <algorithm>
#include <memory>
#include <sstream>
#include <queue>
#include <cassert>

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
static const char* SETTINGS = "settings";
static const char* FUTURES = "futures";

static const char* FIRST_TURN = "first_turn";
static const char* NUM_PUNTERS = "num_punters";
static const char* PUNTER_ID = "punter_id";
static const char* GRAPH = "graph";
static const char* HISTORY = "history";
static const char* INFO = "info";
static const char* FUTURES_ENABLED = "futures_enabled";

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

std::vector<std::vector<int>>
Graph::calc_shortest_distances() const {
  int num_edges = 0;
  for (int i = 0; i < num_vertices; ++i) {
    num_edges += rivers[i].size();
  }
  std::unique_ptr<int[]> que(new int[(num_edges /= 2) + 1]);

  std::vector<std::vector<int>> distances;
  for (int mine = 0; mine < num_mines; ++mine) {
    // calculate shortest distances from a mine
    std::vector<int> dist(num_vertices, 1<<29);
    int qb = 0, qe = 0;
    que[qe++] = mine;
    dist[mine] = 0;
    while (qb < qe) {
      const int u = que[qb++];
      for (const River& river : rivers[u]) {
        const int v = river.to;
        if (dist[v] > dist[u] + 1) {
          dist[v] = dist[u] + 1;
          que[qe++] = v;
        }
      }
    }
    distances.push_back(std::move(dist));
  }

  return distances;
}

std::vector<int64_t>
Graph::evaluate(
  int num_punters,
  const std::vector<std::vector<int>>& distances) const {
  std::vector<int64_t> scores(num_punters, 0LL);

  int num_edges = 0;
  for (int i = 0; i < num_vertices; ++i) {
    num_edges += rivers[i].size();
  }
  std::unique_ptr<int[]> que(new int[(num_edges /= 2) + 1]);

  std::vector<std::vector<River>> es = rivers;
  std::vector<int> nxt(num_vertices, 0);
  for (int i = 0; i < num_vertices; ++i) {
    sort(es[i].begin(), es[i].end(), [] (const River& a, const River& b) {
      return a.punter < b.punter;
    });
    es[i].emplace_back(-1, 1<<29);
  }

  std::vector<int> visited(num_vertices, 0);
  std::unique_ptr<int[]> reached(new int[num_vertices]);
  for (int punter = 0; punter < num_punters; ++punter) {
    for (int mine = 0; mine < num_mines; ++mine) {
      int reach_cnt = 0;
      int qb = 0, qe = 0;
      que[qe++] = mine;
      visited[mine] = 1;
      reached[reach_cnt++] = mine;
      while (qb < qe) {
        const int u = que[qb++];
        while (es[u][nxt[u]].punter < punter) {
          ++nxt[u];
        }
        for (int ei = nxt[u]; es[u][ei].punter == punter; ++ei) {
          const int v = es[u][ei].to;
          if (!visited[v]) {
            que[qe++] = v;
            visited[v] = 1;
            reached[reach_cnt++] = v;
            scores[punter] += distances[mine][v] * distances[mine][v];
          }
        }
      }
      for (int i = 0; i < reach_cnt; ++i) {
        visited[reached[i]] = 0;
      }
    }
  }

  return scores;
}

int64_t
Graph::evaluate_future(
  int punter_id, const std::vector<int>& futures,
  const std::vector<std::vector<int>>& distances) const {
  int num_edges = 0;
  for (int i = 0; i < num_vertices; ++i) {
    num_edges += rivers[i].size();
  }
  std::unique_ptr<int[]> que(new int[(num_edges /= 2) + 1]);

  int64_t res = 0;

  std::vector<int> visited(num_vertices, 0);
  std::unique_ptr<int[]> reached(new int[num_vertices]);
  for (int mine = 0; mine < num_mines; ++mine) {
    if (futures[mine] < 0) {
      continue;
    }
    int reach_cnt = 0;
    int qb = 0, qe = 0;
    que[qe++] = mine;
    visited[mine] = 1;
    reached[reach_cnt++] = mine;
    bool future_ok = false;
    while (qb < qe) {
      const int u = que[qb++];
      for (const River& river : rivers[u]) if (river.punter == punter_id) {
        const int v = river.to;
        if (!visited[v]) {
          que[qe++] = v;
          visited[v] = 1;
          reached[reach_cnt++] = v;
          if (v == futures[mine]) {
            future_ok = true;
            qb = qe;
            break;
          }
        }
      }
    }
    for (int i = 0; i < reach_cnt; ++i) {
      visited[reached[i]] = 0;
    }
    const int64_t dis = distances[mine][futures[mine]];
    res += (future_ok ? +1 : -1) * dis * dis * dis;
  }

  return res;
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

SetupSettings::SetupSettings(
  const Json::Value& info,
  const std::vector<int>& futures)
  : info(info), futures(futures) {

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
    shortest_distances = graph.calc_shortest_distances();

    history = History();
    first_turn = true;

    Json::Value next_graph = json[MAP];
    for (Json::Value& river : next_graph[RIVERS]) {
      river[PUNTER] = -1;
    }

    futures_enabled = false;
    futures = std::vector<int>(graph.num_mines, -1);

    if (json.isMember(SETTINGS)) {
      const Json::Value& settings = json[SETTINGS];
      if (settings.isMember(FUTURES)) {
        futures_enabled = settings[FUTURES].asBool();
      }
    }

    const SetupSettings& setup_result = setup();
    const Json::Value next_info = setup_result.info;
    const Json::Value state = encode_state(next_info, next_graph);

    if (futures_enabled) {
      Json::Value futures_json;
      futures = setup_result.futures;
      while ((int)futures.size() < graph.num_mines) {
        futures.push_back(-1);
      }
      futures_json.resize(0);
      for (int i = 0; i < graph.num_mines; ++i) {
        if (futures[i] >= 0) {
          Json::Value future;
          future[SOURCE] = reverse_id_map[i];
          future[TARGET] = reverse_id_map[futures[i]];
          futures_json.append(future);
        }
      }
      res[FUTURES] = futures_json;
    }

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
  for (int i = 0; i < (int)futures.size(); ++i) {
    state[FUTURES].append(futures[i]);
  }
  state[FUTURES_ENABLED] = futures_enabled;

  state[INFO] = info;
  return state;
}

void
Game::decode_state(Json::Value state) {
  first_turn = state.isMember(FIRST_TURN);
  num_punters = state[NUM_PUNTERS].asInt();
  punter_id = state[PUNTER_ID].asInt();

  std::tie(graph, reverse_id_map, id_map) = Graph::from_json(state[GRAPH]);
  shortest_distances = graph.calc_shortest_distances();

  history = History();
  for (const Json::Value& mv : state[HISTORY]) {
    history.emplace_back(mv);
  }

  futures_enabled = state[FUTURES_ENABLED].asBool();
  futures = std::vector<int>();
  for (const Json::Value& f : state[FUTURES]) {
    futures.push_back(f.asInt());
  }

  info = state[INFO];
}

