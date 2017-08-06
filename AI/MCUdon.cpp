#include <iostream>

#include "Game.h"
#include "json/json.h"

#define _USE_AS_ENGINE
#include "SuUdon.cpp"
#include "MCTS_greedy.cpp"
#undef _USE_AS_ENGINE

using namespace std;

static const char* MODE = "mode";
static const char* ORG_INFO = "org_info";

class MCUdon : public Game {
  SetupSettings setup() const override;
  MoveResult move() const override;
  std::string name() const override;

  template<typename T> MoveResult trymove() const;
};

std::string
MCUdon::name() const {
  return "MC_Udon";
}

SetupSettings
MCUdon::setup() const {
  SuUdonAI::SuUdonAI udon;
  udon.import(*this);
  auto res = udon.setup();

  SetupSettings r = res;
  r.info = Json::Value();
  r.info[MODE] = 0;
  r.info[ORG_INFO] = res.info;
  return r;
}

template<typename T> MoveResult
MCUdon::trymove() const {
  T ai;
  ai.import(*this);
  return ai.move();
}

MoveResult
MCUdon::move() const {
  int mode = info[MODE].asInt();
  MoveResult r((Json::Value()));
  info_for_import = info[ORG_INFO];
  while (true) {
    switch (mode) {
      case 0: r = trymove<SuUdonAI::SuUdonAI>(); break;
      case 1: r = trymove<MCTS_greedy::MCTS_GREEDY_AI>(); break;
    }
    if (!r.valid) {
      mode = 1;
      MCTS_greedy::MCTS_GREEDY_AI ai;
      ai.import(*this);
      info_for_import = ai.walkin_setup();
    } else {
      break;
    }
  }

  MoveResult res = r;
  res.info = Json::Value();
  res.info[MODE] = mode;
  res.info[ORG_INFO] = r.info;
  return res;
}

int main() {
  MCUdon ai;
  ai.run();
  return 0;
}
