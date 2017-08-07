#include <iostream>

#include "Game.h"
#include "json/json.h"

#define _USE_AS_ENGINE
#include "TrueGreedy2Flowlight.cpp"
#include "Ichigo_weak.cpp"
#include "Genocide.cpp"
#undef _USE_AS_ENGINE

using namespace std;

static const char* GRAPH_TYPE = "graph_type";
static const char* MODE = "mode";
static const char* ORG_INFO = "org_info";


class NegAInoido : public Game {
  SetupSettings setup() const override;
  MoveResult move() const override;
  std::string name() const override;

  template<typename T> MoveResult trymove() const;

  enum graph_type_t {
Small, Medium, Large
  };

  enum mode_t {
    MINE,
    GREEDY,
    DONE,
    FUTURE,
  };


  graph_type_t classify_graph() const;
  template<typename T> SetupSettings ai_setup() const;
  template<typename T> MoveResult ai_move() const;
  pair<mode_t, MoveResult> move_flowgith(mode_t mode) const;
};

std::string
NegAInoido::name() const {
  return "NegAInoido";
}

NegAInoido::graph_type_t
NegAInoido::classify_graph() const {
  if (graph.num_edges < 250) {
    return Small;
  } else if (graph.num_edges < 1300) {
    return Medium;
  } else {
    return Large;
  }
}

template<typename T> SetupSettings
NegAInoido::ai_setup() const {
  T ai;
  ai.import(*this);
  return ai.setup();
}


SetupSettings
NegAInoido::setup() const {
  graph_type_t gtype = classify_graph();

  SetupSettings res((Json::Value()));

  switch (gtype) {
    case Small:
      if (futures_enabled) res = ai_setup<TrueGreedy2Flowlight::TrueGreedy2Flowlight>();
      else                 res = ai_setup<Ichigo_weak::Ichigo>();
      break;
    case Medium:
      if (futures_enabled) res = ai_setup<TrueGreedy2Flowlight::TrueGreedy2Flowlight>();
      else                 res = ai_setup<Genocide::Genocide>();
      break;
    case Large:
      if (futures_enabled) res = ai_setup<Genocide::Genocide>();
      else                 res = ai_setup<Genocide::Genocide>();
      break;
  }

  SetupSettings r = res;
  r.info = Json::Value();
  r.info[GRAPH_TYPE] = gtype;
  r.info[ORG_INFO] = res.info;
  return r;
}

template<typename T> MoveResult
NegAInoido::trymove() const {
  T ai;
  ai.import(*this);
  return ai.move();
}

MoveResult
NegAInoido::move() const {
  mode_t mode = (mode_t) info[MODE].asInt();
  int gtype = info[GRAPH_TYPE].asInt();
  MoveResult r((Json::Value()));
  info_for_import = info[ORG_INFO];
  std::cerr<< "Turn: " << history.size() / num_punters << " " << mode << endl;

  switch (gtype) {
    case Small:
      if (futures_enabled) r = trymove<TrueGreedy2Flowlight::TrueGreedy2Flowlight>();
      else                 r = trymove<Ichigo_weak::Ichigo>();
      break;
    case Medium:
      if (futures_enabled) r = trymove<TrueGreedy2Flowlight::TrueGreedy2Flowlight>();
      else                 r = trymove<Genocide::Genocide>();
      break;
    case Large:
      if (futures_enabled) r = trymove<Genocide::Genocide>();
      else                 r = trymove<Genocide::Genocide>();
      break;
  }

  MoveResult res = r;
  res.info = Json::Value();
  res.info[GRAPH_TYPE] = info[GRAPH_TYPE].asInt();
  res.info[MODE] = mode;
  res.info[ORG_INFO] = r.info;
  return res;
}

int main() {
  NegAInoido ai;
  ai.run();
  return 0;
}
