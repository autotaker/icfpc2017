#include "Game.h"

#include "json/json.h"
#include <iostream>
#include <map>
#include <algorithm>
#include <memory>
#include <sstream>
#include <queue>
#include <cassert>


/*
 *  Constants
 */

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
static const char* REVERSE_ID_MAP = "reverse_id_map";
static const char* FUTURES_ENABLED = "futures_enabled";


/*
 *  JSON I/O helper
 */

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


/*
 *  Graph class
 */

bool
Graph::River::operator<(const Graph::River& rhs) const {
  return to < rhs.to;
}

Graph
Graph::from_json(const Json::Value& json) {
  Graph g;

  g.num_mines = json[MINES].asInt();
  g.num_vertices = json[SITES].asInt();

  g.rivers.resize(g.num_vertices);
  for (const auto& river : json[RIVERS]) {
    int source = river[0].asInt();
    int target = river[1].asInt();
    int punter = river[2].asInt();
    g.rivers[source].emplace_back(target, punter);
    g.rivers[target].emplace_back(source, punter);
  }

  return g;
}

std::tuple<Graph, std::vector<int>, std::map<int, int>>
Graph::from_json_setup(const Json::Value& json) {
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

Json::Value
Graph::to_json() const {
  Json::Value rivers_json;
  for (int i = 0; i < num_vertices; ++i) {
    for (auto& river : rivers[i]) if (i < river.to) {
      Json::Value r;
      r.append(i);
      r.append(river.to);
      r.append(river.punter);
      rivers_json.append(r);
    }
  }

  Json::Value json;
  json[SITES] = num_vertices;
  json[RIVERS] = rivers_json;
  json[MINES] = num_mines;

  return json;
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
  int64_t dummy;
  return evaluate(num_punters, distances, -1, {}, dummy);
}

std::vector<int64_t>
Graph::evaluate(
  int num_punters,
  const std::vector<std::vector<int>>& distances,
  int my_punter_id, const std::vector<int>& futures, int64_t& future_score) const {
  std::vector<int64_t> scores(num_punters, 0LL);

  int num_edges = 0;
  for (int i = 0; i < num_vertices; ++i) {
    num_edges += rivers[i].size();
  }

  const int MAX_EDGE = 1e5;
  int que_array[MAX_EDGE];
  std::unique_ptr<int[]> que_deleter;
  int* que = que_array;

  num_edges /= 2;
  if (num_edges + 1 > MAX_EDGE) {
    que_deleter.reset(new int[num_edges + 1]);
    que = que_deleter.get();
  } else {
    std::fill(que_array, que_array + num_edges + 1, 0);
  }


  std::unique_ptr<int[]> nxt_deleter;
  int nxt_array[MAX_EDGE];
  int* nxt = nxt_array;
  if (num_vertices > MAX_EDGE) {
    nxt_deleter.reset(new int[num_vertices]);
    nxt = nxt_deleter.get();
  } else {
    std::fill(nxt, nxt + num_vertices, 0);
  }

  // std::vector<std::vector<River>> es = rivers;
  std::unique_ptr<River*[]> es_deleter;
  River* es_array[MAX_EDGE];
  River** es = es_array;
  if (num_vertices > MAX_EDGE) {
    es_deleter.reset(new River*[num_vertices]);
    es = es_deleter.get();
  }

  River river_array[MAX_EDGE];
  std::unique_ptr<River[]> river_deleter;
  River* river_buf = river_array;
  if (num_edges > MAX_EDGE) {
    river_deleter.reset(new River[num_edges]);
    river_buf = river_deleter.get();
  }

  for (int i = 0; i < num_vertices; ++i) {
    es[i] = river_buf;
    river_buf += rivers[i].size();
    std::copy(rivers[i].begin(), rivers[i].end(), es[i]);
    std::sort(es[i], es[i] + rivers[i].size(), [] (const River& a, const River& b) {
      return a.punter < b.punter;
    });
  }

  std::unique_ptr<int[]> visited_deleter;
  int visited_array[MAX_EDGE];
  int* visited = visited_array;
  if (num_vertices > MAX_EDGE) {
    visited_deleter.reset(new int[num_vertices]);
    visited = visited_array;
  } else {
    std::fill(visited, visited + num_vertices, 0);
  }
  
  std::unique_ptr<int[]> reached_deleter;
  int reached_array[MAX_EDGE];
  int* reached = reached_array;
  if (num_vertices > MAX_EDGE) {
    reached_deleter.reset(new int[num_vertices]);
    reached = reached_deleter.get();
  } else {
    std::fill(reached, reached + num_vertices, 0);
  }


  future_score = 0;

  for (int punter = 0; punter < num_punters; ++punter) {
    for (int mine = 0; mine < num_mines; ++mine) {
      int reach_cnt = 0;
      int qb = 0, qe = 0;
      que[qe++] = mine;
      visited[mine] = 1;
      reached[reach_cnt++] = mine;
      while (qb < qe) {
        const int u = que[qb++];
        const int usize = rivers[u].size();
        while (nxt[u] < usize && es[u][nxt[u]].punter < punter) {
          ++nxt[u];
        }
        for (int ei = nxt[u]; ei < usize && es[u][ei].punter == punter; ++ei) {
          const int v = es[u][ei].to;
          if (!visited[v]) {
            que[qe++] = v;
            visited[v] = 1;
            reached[reach_cnt++] = v;
            scores[punter] += distances[mine][v] * distances[mine][v];
          }
        }
      }
      if (punter == my_punter_id && futures[mine] >= 0) {
        const int64_t dis = distances[mine][futures[mine]];
        const bool future_ok = visited[futures[mine]] == 1;
        future_score += (future_ok ? +1 : -1) * dis * dis * dis;
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


/*
 *  SetupSettings class
 */

Move::Move(int punter, int src, int to)
  : punter(punter), src(src), to(to) {

}

Move::Move(Json::Value json) {
  punter = json[0].asInt();
  src = json[1].asInt();
  to = json[2].asInt();
}

bool
Move::is_pass() const {
  return src == -1;
}

Json::Value
Move::to_json() const {
  Json::Value json;
  json.append(punter);
  json.append(src);
  json.append(to);
  return json;
}

SetupSettings::SetupSettings(
  const Json::Value& info,
  const std::vector<int>& futures)
  : info(info), futures(futures) {

}


/*
 *  Game class
 */

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

  if (json.isMember(PUNTER)) {  // Setup
    punter_id = json[PUNTER].asInt();
    num_punters = json[PUNTERS].asInt();

    std::tie(graph, reverse_id_map, id_map) = Graph::from_json_setup(json[MAP]);
    shortest_distances = graph.calc_shortest_distances();

    history = History();
    first_turn = true;

    futures_enabled = false;
    if (json.isMember(SETTINGS)) {
      const Json::Value& settings = json[SETTINGS];
      if (settings.isMember(FUTURES)) {
        futures_enabled = settings[FUTURES].asBool();
      }
    }

    const SetupSettings& setup_result = setup();
    const Json::Value next_info = setup_result.info;

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

    const Json::Value state = encode_state(next_info);

    res[READY] = json[PUNTER];
    res[STATE] = state;
  } else if (json.isMember(MOVE)) {   // GamePlay
    decode_state(json[STATE]);

    const Json::Value moves = json[MOVE][MOVES];
    for (const Json::Value& mv : moves) {
      if (mv.isMember(CLAIM)) {
        const int p = mv[CLAIM][PUNTER].asInt();
        const int src = id_map[mv[CLAIM][SOURCE].asInt()];
        const int to = id_map[mv[CLAIM][TARGET].asInt()];
        history.emplace_back(p, src, to);
        auto& rs = graph.rivers[src];
        auto& rt = graph.rivers[to];
        std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter = p;
        std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->punter = p;
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
    json_helper::write_json(res);
  }
}

int
Game::original_vertex_id(int vertex_id) const {
  return reverse_id_map[vertex_id];
}

Json::Value
Game::encode_state(const Json::Value& info) const {
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
  graph = Graph::from_json(state[GRAPH]);
  shortest_distances = graph.calc_shortest_distances();

  history = History();
  for (const Json::Value& mv : state[HISTORY]) {
    history.emplace_back(mv);
  }

  id_map.clear();
  reverse_id_map.clear();
  for (int i = 0; i < graph.num_vertices; ++i) {
    int org_id = state[REVERSE_ID_MAP][i].asInt();
    id_map[org_id] = i;
    reverse_id_map.push_back(org_id);
  }

  futures_enabled = state[FUTURES_ENABLED].asBool();
  futures = std::vector<int>();
  for (const Json::Value& f : state[FUTURES]) {
    futures.push_back(f.asInt());
  }

  info = state[INFO];
}

