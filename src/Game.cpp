#include "Game.h"

#include "../jsoncpp/json/json.h"
#include <iostream>
#include <map>
#include <algorithm>
#include <memory>

Graph::River::River(int to, int punter)
  : to(to), punter(punter) {

}

Graph
Graph::from_json(const Json::Value &json) {
  Graph g;

  g.num_mines = json["mines"].size();
  g.num_vertices = json["sites"].size();

  g.rivers.resize(g.num_vertices);
  for (const auto &river : json["rivers"]) {
    int source = river["source"].asInt();
    int target = river["target"].asInt();
    g.rivers[source].push_back(target);
    g.rivers[target].push_back(source);
  }

  return g;
}

std::pair<Graph, std::vector<int>>
Graph::from_json_with_renaming(const Json::Value &json) {
  Graph g;
  std::map<int, int> table;
  std::vector<int> rev_table;

  g.num_mines = json["mines"].size();
  g.num_vertices = json["sites"].size();

  int cur_id = 0;
  for (const auto &mine : json["mines"]) {
    int mine_id = mine.asInt();
    table[mine_id] = cur_id++;
    rev_table.push_back(mine_id);
  }

  for (const auto &site : json["sites"]) {
    int site_id = site["id"].asInt();
    if (table.count(site_id)) continue;
    table[site_id] = cur_id++;
    rev_table.push_back(site_id);
  }

  g.rivers.resize(g.num_vertices);
  for (const auto &river : json["rivers"]) {
    int source = table[river["source"].asInt()];
    int target = table[river["target"].asInt()];
    g.rivers[source].push_back(target);
    g.rivers[target].push_back(source);
  }
  for (int i = 0; i < g.num_vertices; ++i) {
    std::sort(g.rivers[i].begin(), g.rivers[i].end());
  }

  return {g, rev_table};
}

Json::Value
Graph::to_json() const {
  Json::Value sites_json;
  for (int i = 0; i < num_vertices; ++i) {
    Json::Value s;
    s["id"] = i;
    sites_json.append(s);
  }

  Json::Value rivers_json;
  for (int i = 0; i < num_vertices; ++i) {
    for (auto &river : rivers[i]) if (i < river.to) {
      Json::Value r;
      r["source"] = i;
      r["target"] = river.to;
      r["p"] = river.punter;
      rivers_json.append(r);
    }
  }

  Json::Value mines_json;
  for (int i = 0; i < num_mines; ++i) {
    mines_json.append(i);
  }

  Json::Value json;
  json["sites"] = sites_json;
  json["rivers"] = rivers_json;
  json["mines"] = mines_json;

  return json;
}

Move::Move(int punter, int src, int to)
  : punter(punter), src(src), to(to) {

}

bool
Move::is_pass() const {
  return src == -1;
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
  if (json.isMember("punter")) {
    // TODO: nanka suru
    const Json::Value info = setup();
    const Json::Value state = encode_state(info);
    res["ready"] = json["punter"];
    res["state"] = state;
  } else if (json.isMember("move")) {
    // TODO: nanka suru
    int src, to;
    Json::Value info;
    std::tie(src, to, info) = move();

    Json::Value json_move;
    if (src == -1) {
      res["pass"]["punter"] = punter_id;
    } else {
      res["claim"]["punter"] = punter_id;
      res["claim"]["source"] = src;
      res["claim"]["target"] = to;
    }
    res["state"] = encode_state(info);
  }

  if (!json.isMember("stop")) {
    std::cout << res << std::endl;
  }
}
