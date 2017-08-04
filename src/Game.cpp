#include "Game.h"

#include "../jsoncpp/json/json.h"
#include <iostream>
#include <memory>

Graph::River::River(int to, int punter)
  : to(to), punter(punter) {

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

  std::unique_ptr<char[]> json_buf(new char[length + 1]);
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
