#include "Game.h"

#include "json/json.h"
#include <iostream>
#include <map>
#include <algorithm>
#include <memory>
#include <sstream>
#include <queue>
#include <cassert>
#include <memory>

#ifdef HAVE_CPU_PROFILER
// $ apt install libgoogle-perftools-dev
// $ make LIBPROFILER='-lprofiler'
// $ ../bin/MCTS # execute binary
// $ google-pprof --svg ../bin/MCTS prof.out > prof.svg
#include <gperftools/profiler.h>
#include <unistd.h>

#endif

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
static const char* SPLURGES = "splurges";
static const char* SPLURGE = "splurge";
static const char* ROUTE = "route";
static const char* OPTIONS = "options";
static const char* OPTION = "option";

static const char* FIRST_TURN = "first_turn";
static const char* NUM_PUNTERS = "num_punters";
static const char* PUNTER_ID = "punter_id";
static const char* GRAPH = "graph";
static const char* HISTORY = "history";
static const char* INFO = "info";
static const char* REVERSE_ID_MAP = "reverse_id_map";
static const char* FUTURES_ENABLED = "futures_enabled";
static const char* SPLURGES_ENABLED = "splurges_enabled";
static const char* SPLURGE_LENGTH = "splurge_length";
static const char* OPTIONS_ENABLED = "options_enabled";
static const char* OPTIONS_BOUGHT = "options_bought";

namespace {
#ifdef HAVE_CPU_PROFILER
  char prof_name[1000];
  char mktemp_name[1000];
#endif

  void StartProfilerWrapper(int punter_id, const std::string& name, int turn) {
#ifdef HAVE_CPU_PROFILER
    // sampling rate 500/s (default sampling 100/s)
    char change_freq[] = "CPUPROFILE_FREQUENCY=500";
    putenv(change_freq);
    sprintf(prof_name, "/tmp/punter_%s_%d_turn_%d", name.c_str(), punter_id, turn);

    sprintf(mktemp_name,"%s_XXXXXX", prof_name);
    (void)mkstemp(mktemp_name);
    ProfilerStart(mktemp_name);
#else
    (void)(punter_id);
    (void)(name);
    (void)(turn);
#endif
  }

  void StopProfilerWrapper() {
#ifdef HAVE_CPU_PROFILER
    ProfilerStop();
    unlink(prof_name);
    symlink(mktemp_name, prof_name);
#endif
  }

}


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

int
Graph::owner(int u, int v) const {
  auto& rs = rivers[u];
  return std::lower_bound(rs.begin(), rs.end(), Graph::River{v})->punter;
}

Graph
Graph::from_json(const Json::Value& json) {
  Graph g;

  g.num_mines = json[MINES].asInt();
  g.num_vertices = json[SITES].asInt();
  g.num_edges = json[RIVERS].size();

  g.rivers.resize(g.num_vertices);
  for (const auto& river : json[RIVERS]) {
    int source = river[0].asInt();
    int target = river[1].asInt();
    int punter = river[2].asInt();
    int option = river.size() > 3 ? river[3].asInt() : -1;
    g.rivers[source].emplace_back(target, punter, option);
    g.rivers[target].emplace_back(source, punter, option);
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
  g.num_edges = json[RIVERS].size();

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
      r.append(river.option);
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

  const int MAX_EDGE = 2e4;
  int que_array[MAX_EDGE];
  std::unique_ptr<int[]> que_deleter;
  int* que = que_array;

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
  if (num_edges + num_mines * num_punters > MAX_EDGE) {
    river_deleter.reset(new River[num_edges + num_mines * num_punters]);
    river_buf = river_deleter.get();
  }

  std::unique_ptr<int[]> rsize_deleter;
  int rsize_array[MAX_EDGE];
  int* rsize = rsize_array;
  if (num_vertices > MAX_EDGE) {
    rsize_deleter.reset(new int[num_vertices]);
    rsize = rsize_array;
  }

  for (int i = 0; i < num_vertices; ++i) {
    rsize[i] = rivers[i].size();
    es[i] = river_buf;
    river_buf += rsize[i];
    std::copy(rivers[i].begin(), rivers[i].end(), es[i]);
    for (const auto& river : rivers[i]) {
      if (river.option != -1) {
        *river_buf = River(river.to, river.option);
        ++river_buf;
        ++rsize[i];
      }
    }
    std::sort(es[i], es[i] + rsize[i], [] (const River& a, const River& b) {
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
    std::vector<int> computed_mine(num_mines);
    for (int mine = 0; mine < num_mines; ++mine) {
      if (computed_mine[mine]) continue;

      int reach_cnt = 0;
      int qb = 0, qe = 0;
      que[qe++] = mine;
      visited[mine] = 1;
      reached[reach_cnt++] = mine;
      while (qb < qe) {
        const int u = que[qb++];
        const int usize = rsize[u];
        while (nxt[u] < usize && es[u][nxt[u]].punter < punter) {
          ++nxt[u];
        }
        for (int ei = nxt[u]; ei < usize && es[u][ei].punter == punter; ++ei) {
          const int v = es[u][ei].to;
          if (!visited[v]) {
            que[qe++] = v;
            visited[v] = 1;
            reached[reach_cnt++] = v;
          }
        }
      }

      // calculate mine score which are reachable in this bfs.
      for (int tmine = mine; tmine < num_mines; ++tmine) {
	if (computed_mine[tmine]) continue;
	if (!visited[tmine]) continue;

	computed_mine[tmine] = true;
	if (punter == my_punter_id && futures[tmine] >= 0) {
	  const int64_t dis = distances[tmine][futures[tmine]];
	  const bool future_ok = visited[futures[tmine]] == 1;
	  future_score += (future_ok ? +1 : -1) * dis * dis * dis;
	}
	
	for (int i = 0; i < reach_cnt; ++i) {
	  int d = distances[tmine][reached[i]];
	  scores[punter] += d * d;
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
      for (const River& river : rivers[u]) if (river.punter == punter_id || river.option == punter_id) {
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
  : punter(punter), src(src), to(to), path() {

}

Move::Move(Json::Value json) {
  punter = json[0].asInt();
  src = json[1].asInt();
  to = json[2].asInt();
  path.clear();
  for (int i = 3; i < (int)json.size(); ++i) {
    path.push_back(json[i].asInt());
  }
}

Move::Move(int punter, const std::vector<int>& path)
  : punter(punter), src(-1), to(-1), path(path) {

}

bool
Move::is_pass() const {
  return src == -1 && path.empty();
}

bool
Move::is_claim() const {
  return src >= 0;
}

bool
Move::is_splurge() const {
  return !path.empty();
}

Json::Value
Move::to_json() const {
  Json::Value json;
  json.append(punter);
  json.append(src);
  json.append(to);
  for (const int v : path) {
    json.append(v);
  }
  return json;
}

SetupSettings::SetupSettings(
  const Json::Value& info,
  const std::vector<int>& futures)
  : info(info), futures(futures) {

}

MoveResult::MoveResult(const Json::Value& info)
  : src(-1), to(-1), splurge_path(), info(info) {

}

MoveResult::MoveResult(const std::tuple<int, int, Json::Value>& move) {
  std::tie(src, to, info) = move;
  splurge_path.clear();
}

MoveResult::MoveResult(const std::vector<int>& path, const Json::Value& info)
  : src(-1), to(-1), splurge_path(path), info(info) {

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
    splurges_enabled = false;
    options_enabled = false;
    if (json.isMember(SETTINGS)) {
      const Json::Value& settings = json[SETTINGS];
      if (settings.isMember(FUTURES)) {
        futures_enabled = settings[FUTURES].asBool();
      }
      if (settings.isMember(SPLURGES)) {
        splurges_enabled = settings[SPLURGES].asBool();
      }
      if (settings.isMember(OPTIONS)) {
        options_enabled = settings[OPTIONS].asBool();
      }
    }

    const SetupSettings& setup_result = setup();
    const Json::Value next_info = setup_result.info;

    {
      Json::Value futures_json;
      futures = setup_result.futures;
      while ((int)futures.size() < graph.num_mines) {
        futures.push_back(-1);
      }
      if (futures_enabled) {
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
    }
    splurge_length = 1;
    options_bought = 0;

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
      } else if (mv.isMember(SPLURGE)) {
        const int p = mv[CLAIM][PUNTER].asInt();
        std::vector<int> path;
        for (const Json::Value& v : mv[SPLURGE][ROUTE]) {
          path.push_back(id_map[v.asInt()]);
        }
        history.emplace_back(p, path);
        for (int i = 0; i + 1 < (int)path.size(); ++i) {
          const int src = path[i];
          const int to = path[i + 1];
          auto& rs = graph.rivers[src];
          auto& rt = graph.rivers[to];
          std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter = p;
          std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->punter = p;
        }
      } else if (mv.isMember(OPTION)) {
        const int p = mv[OPTION][PUNTER].asInt();
        const int src = id_map[mv[OPTION][SOURCE].asInt()];
        const int to = id_map[mv[OPTION][TARGET].asInt()];
        history.emplace_back(p, src, to);
        auto& rs = graph.rivers[src];
        auto& rt = graph.rivers[to];
        std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->option = p;
        std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->option = p;
      } else {
        const int p = mv[PASS][PUNTER].asInt();
        if (!first_turn || p < punter_id) {
          history.emplace_back(p, -1, -1);
        }
      }
    }

    StartProfilerWrapper(punter_id, name(), history.size());
    MoveResult move_res = move();
    StopProfilerWrapper();

    Json::Value json_move;
    if (!move_res.splurge_path.empty()) {
      res[SPLURGE][PUNTER] = punter_id;
      for (const int u : move_res.splurge_path) {
        res[SPLURGE][ROUTE].append(reverse_id_map[u]);
      }
      for (int i = 0; i+1 < (int)move_res.splurge_path.size(); ++i) {
        const int u = move_res.splurge_path[i];
        const int v = move_res.splurge_path[i + 1];
        if (graph.owner(u, v) != -1) {
          ++options_bought;
        }
      }
      splurge_length = 1;
    } else if (move_res.src == -1) {
      res[PASS][PUNTER] = punter_id;
      ++splurge_length;
    } else {
      const bool opt = graph.owner(move_res.src, move_res.to) != -1;
      const char* MV = opt ? OPTION : CLAIM;
      res[MV][PUNTER] = punter_id;
      res[MV][SOURCE] = reverse_id_map[move_res.src];
      res[MV][TARGET] = reverse_id_map[move_res.to];
      splurge_length = 1;
      if (opt) {
        ++options_bought;
      }
    }

    first_turn = false;
    res[STATE] = encode_state(move_res.info);
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

  state[FUTURES_ENABLED] = futures_enabled;
  for (int i = 0; i < (int)futures.size(); ++i) {
    state[FUTURES].append(futures[i]);
  }

  state[SPLURGES_ENABLED] = splurges_enabled;
  state[SPLURGE_LENGTH] = splurge_length;

  state[OPTIONS_ENABLED] = options_enabled;
  state[OPTIONS_BOUGHT] = options_bought;

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

  splurges_enabled = state[SPLURGES_ENABLED].asBool();
  splurge_length = state[SPLURGE_LENGTH].asInt();

  options_enabled = state[OPTIONS_ENABLED].asBool();
  options_bought = state[OPTIONS_BOUGHT].asInt();

  info = state[INFO];
}

void
Game::calc_shortest_paths(int src, std::vector<int>& dist, std::vector<int>& prev) const {
  std::deque<int> que;
  dist.resize(graph.num_vertices, INF);
  prev.resize(graph.num_vertices, -1);

  que.push_back(src);
  dist[src] = 0;
  while (!que.empty()) {
    const int v = que[0]; que.pop_front();
    for (const auto& river : graph.rivers[v]) {
      if (river.punter != -1 && river.punter != punter_id) {
        continue;
      }
      const int w = river.to;
      const int d = (river.punter == punter_id ? 0 : 1);
      if (dist[w] > dist[v] + d) {
        dist[w] = dist[v] + d;
        prev[w] = v;
        if (d == 0) {
          que.push_front(w);
        } else {
          que.push_back(w);
        }
      }
    }
  }
}

void
Game::calc_cur_dists(std::vector<std::vector<int>>& dists, std::vector<std::vector<int>>& prevs) const {
  for (int u = 0; u < graph.num_mines; ++u) {
    std::vector<int> dist, prev;
    calc_shortest_paths(u, dist, prev);

    dists.push_back(std::move(dist));
    prevs.push_back(std::move(prev));
  }
}

void
Game::import(const Game& meta_ai) {
  num_punters = meta_ai.num_punters;
  punter_id = meta_ai.punter_id;
  graph = meta_ai.graph;
  history = meta_ai.history;
  shortest_distances = meta_ai.shortest_distances;
  //////////////////////////////  //////////////////////////////
  //////////////////////////////  //////////////////////////////
  info = meta_ai.info_for_import; ////////////////////////////// 
  //////////////////////////////  //////////////////////////////
  //////////////////////////////  //////////////////////////////
  futures_enabled = meta_ai.futures_enabled;
  futures = meta_ai.futures;
  splurges_enabled = meta_ai.splurges_enabled;
  splurge_length = meta_ai.splurge_length;
  first_turn = meta_ai.first_turn;
  reverse_id_map = meta_ai.reverse_id_map;
  id_map = meta_ai.id_map;
}
