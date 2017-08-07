#include <iostream>

#include "Game.h"
#include "json/json.h"

#define _USE_AS_ENGINE
#include "SuUdon.cpp"
#include "MCTS_greedy.cpp"
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
    SMALL,
    MEDIUM,
    LARGE
  };


  enum mode_t {
    MINE,
    GREEDY,
    DONE,
    FUTURE,
  };

  graph_type_t classify_graph() const;
  template<typename T> SetupSettings ai_setup() const;

  pair<mode_t, MoveResult> move_small_default(mode_t mode) const;
  pair<mode_t, MoveResult> move_small_future(mode_t mode) const;
  pair<mode_t, MoveResult> move_medium_default(mode_t mode) const;
  pair<mode_t, MoveResult> move_medium_future(mode_t mode) const;
  pair<mode_t, MoveResult> move_large_default(mode_t mode) const;
  pair<mode_t, MoveResult> move_large_future(mode_t mode) const;
};

std::string
NegAInoido::name() const {
  return "NegAInoido";
}

NegAInoido::graph_type_t
NegAInoido::classify_graph() const {
  if (graph.num_edges < 300) {
    return SMALL;
  } else if (graph.num_edges < 1500) {
    return MEDIUM;
  } else {
    return LARGE;
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
  mode_t mode;

  // TODO: implement
  switch (gtype) {
  case SMALL:
    if (futures_enabled) {
      res = ai_setup<SuUdonAI::SuUdonAI>();
      mode = MINE;
    } else {
      res = ai_setup<SuUdonAI::SuUdonAI>();
      mode = MINE;
    }
    break;
  case MEDIUM:
    if (futures_enabled) {
      res = ai_setup<SuUdonAI::SuUdonAI>();
      mode = MINE;
    } else {
      res = ai_setup<SuUdonAI::SuUdonAI>();
      mode = MINE;
    }
    break;
  case LARGE:
    if (futures_enabled) {
      res = ai_setup<SuUdonAI::SuUdonAI>();
      mode = MINE;
    } else {
      res = ai_setup<SuUdonAI::SuUdonAI>();
      mode = MINE;
    }
    break;
  }

  SetupSettings r = res;
  r.info = Json::Value();
  r.info[MODE] = mode;
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

pair<NegAInoido::mode_t, MoveResult>
NegAInoido::move_small_default(mode_t mode) const {
  // TODO
  MoveResult r((Json::Value()));
  while (true) {
    switch (mode) {
    case MINE: r = trymove<SuUdonAI::SuUdonAI>(); break;
    case GREEDY: r = trymove<MCTS_greedy::MCTS_GREEDY_AI>(); break;

    default:
      assert(false);
    }
    std::cerr<< "r.valid" << " " << r.valid << endl;
    if (!r.valid) {
      mode = GREEDY;
      MCTS_greedy::MCTS_GREEDY_AI ai;
      ai.import(*this);
      info_for_import = ai.walkin_setup();
    } else {
      break;
    }
  }
  return make_pair(mode, r);
}

pair<NegAInoido::mode_t, MoveResult>
NegAInoido::move_small_future(mode_t mode) const {
  // TODO
  return move_small_default(mode);
}

pair<NegAInoido::mode_t, MoveResult>
NegAInoido::move_medium_default(mode_t mode) const {
  // TODO
  return move_small_default(mode);
}

pair<NegAInoido::mode_t, MoveResult>
NegAInoido::move_medium_future(mode_t mode) const {
  // TODO
  return move_small_default(mode);
}

pair<NegAInoido::mode_t, MoveResult>
NegAInoido::move_large_default(mode_t mode) const {
  // TODO
  return move_small_default(mode);
}

pair<NegAInoido::mode_t, MoveResult>
NegAInoido::move_large_future(mode_t mode) const {
  // TODO
  return move_small_default(mode);
}


MoveResult
NegAInoido::move() const {
  mode_t mode = (mode_t) info[MODE].asInt();
  int gtype = info[GRAPH_TYPE].asInt();
  MoveResult r((Json::Value()));
  info_for_import = info[ORG_INFO];
  std::cerr<< "Turn: " << history.size() / num_punters << " " << mode << endl;

  switch (gtype) {
  case SMALL:
    if (futures_enabled) {
      tie(mode, r) = move_small_future(mode);
    } else {
      tie(mode, r) = move_small_default(mode);
    }
    break;
  case MEDIUM:
    if (futures_enabled) {
      tie(mode, r) = move_medium_future(mode);
    } else {
      tie(mode, r) = move_medium_default(mode);
    }
    break;
  case LARGE:
    if (futures_enabled) {
      tie(mode, r) = move_large_future(mode);
    } else {
      tie(mode, r) = move_large_default(mode);
    }
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
