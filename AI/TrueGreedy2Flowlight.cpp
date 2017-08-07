#include <iostream>

#include "Game.h"
#include "json/json.h"

#ifdef _USE_AS_ENGINE
#include "Greedy2.cpp"
#include "Flowlight.cpp"
#else
#define _USE_AS_ENGINE
#include "Greedy2.cpp"
#include "Flowlight.cpp"
#undef _USE_AS_ENGINE
#endif

namespace TrueGreedy2Flowlight {

using namespace std;

static const char* MODE = "mode";
static const char* ORG_INFO = "org_info";

class TrueGreedy2Flowlight : public Game {
public:
  SetupSettings setup() const override;
  MoveResult move() const override;
  std::string name() const override;

  template<typename T> MoveResult trymove() const;
};

std::string
TrueGreedy2Flowlight::name() const {
  return "TrueGreedy2Flowlight";
}

SetupSettings
TrueGreedy2Flowlight::setup() const {
  flowlight::AI udon;
  udon.import(*this);
  auto res = udon.setup();

  SetupSettings r = res;
  r.info = Json::Value();
  r.info[MODE] = 0;
  r.info[ORG_INFO] = res.info;
  return r;
}

template<typename T> MoveResult
TrueGreedy2Flowlight::trymove() const {
  T ai;
  ai.import(*this);
  return ai.move();
}

MoveResult
TrueGreedy2Flowlight::move() const {
  int mode = info[MODE].asInt();
  MoveResult r((Json::Value()));
  info_for_import = info[ORG_INFO];
  while (true) {
    switch (mode) {
      case 0: r = trymove<flowlight::AI>(); break;
      case 1: r = trymove<Greedy2::Greedy2>(); break;
    }
    if (!r.valid) {
      mode = 1;
      Greedy2::Greedy2 ai;
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

}

#ifndef _USE_AS_ENGINE
int main() {
  TrueGreedy2Flowlight::TrueGreedy2Flowlight ai;
  ai.run();
  return 0;
}
#endif
