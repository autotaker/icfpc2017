#include <iostream>

#include "Game.h"
#include "json/json.h"

#define _USE_AS_ENGINE
#include "SuUdon.cpp"
#include "Flowlight.cpp"
#include "MCTS_greedy.cpp"
#include "Greedy2.cpp"
#include "KakeUdon.cpp"
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
      SuUdonAI,
      Flowlight,
      Greedy2,
      KakeUdonAI,
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
  pair<mode_t, MoveResult> move_suudon(mode_t mode) const;
  pair<mode_t, MoveResult> move_flowgith(mode_t mode) const;
};

std::string
NegAInoido::name() const {
  return "NegAInoido";
}

vector<float> make_feature(const Game& game, const Graph& graph) {
    vector<float> f;
    f.push_back((float) graph.num_vertices );
    f.push_back((float) graph.num_mines );
    {
        int sum = 0;
        for (int i = 0; i < graph.num_vertices; ++i) {
            sum += graph.rivers[i].size();
        }
        f.push_back(sum);
    }
    auto& a = game.get_shortest_distances();
    {
        int mn = 1 << 29;
        for (int i = 0; i < graph.num_mines; ++i) {
            for (int j = i + 1; j < graph.num_mines; ++j) {
                mn = min<int>(mn, a[i][j]);
            }
        }
        f.push_back(mn);
    }
    {
        int mx = 0;
        for (int i = 0; i < graph.num_mines; ++i) {
            for (int j = 0; j < graph.num_vertices; ++j) {
                mx = max<int>(mx, a[i][j]);
            }
        }
        f.push_back(mx);
    }
    {
        double sum = 0.0;
        for (int i = 0; i < graph.num_mines; ++i) {
            for (int j = i + 1; j < graph.num_mines; ++j) {
                sum += a[i][j];
            }
        }
        f.push_back(sum);
    }
    {
        float sum = 0.0;
        for (int i = 0; i < graph.num_mines; ++i) {
            sum += graph.rivers[i].size();
        }
        f.push_back(sum);
    }
    {
        float sum = 0.0;
        for (int i = 0; i < graph.num_vertices; ++i) {
            sum += graph.rivers[i].size();
        }
        f.push_back(sum / graph.num_mines);
    }
    return f;
}

NegAInoido::graph_type_t
NegAInoido::classify_graph() const {

    if (not futures_enabled) return SuUdonAI;

    vector<float> b = {
        0.726752242367,
        0.556867957589,
        0.805185815285,
        0.341493993099
    };
    vector<vector<float>> w = {
        {2.77718422e-04,  1.82986968e-03, -1.10125855e-04, -4.40479764e-02, 4.13244644e-11,  6.75749432e-03, -4.27708069e-03,  6.24460340e-04},
        { -1.01630678e-05,  8.76303940e-03,  1.81085494e-06,  1.70234946e-09, 3.84249175e-10, -1.28183163e-03, -2.23806328e-02, -2.48242710e-04},
        { -4.89839574e-04, -1.99689432e-02,  1.27875557e-04, -6.89555392e-10, -7.03462277e-10,  3.52028798e-02, -2.43441308e-02,  1.52909425e-04},
        { -5.72795145e-05, -3.79877588e-03,  3.54836376e-05,  1.21079799e-09, -1.22532237e-10,  1.62316715e-02,  4.64420672e-02, -1.90280073e-04}
    };

    auto x = make_feature(*this, graph);
    int mx = 0.0;
    int idx = 0;
    for (int i = 0; i < 4; ++i) {
        float y = b[i];
        for (int j = 0; j < (int)x.size(); ++j) {
            y += x[j] * w[i][j];
        }
        cerr << i << ": y=" << y << endl;
        if (mx < y) {
            mx = y;
            idx = i;
        }
    }

    if (idx == 0) {
        cerr << "choice Greedy2" << endl;
        return Greedy2;
    } else if (idx == 1) {
        cerr << "choice SuUdonAI" << endl;
        return SuUdonAI;
    } else if (idx == 2) {
        cerr << "choice Flowlight" << endl;
        return Flowlight;
    } else if (idx == 3) {
        cerr << "choice KakeUdonAI" << endl;
        return KakeUdonAI;
    }

    return SuUdonAI;
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

  if (not futures_enabled) {
      res = ai_setup<SuUdonAI::SuUdonAI>();
  } else {
      switch (gtype) {
      case Flowlight:
          res = ai_setup<flowlight::AI>();
          break;
      case SuUdonAI:
          res = ai_setup<SuUdonAI::SuUdonAI>();
          break;
      case Greedy2:
          res = ai_setup<Greedy2::Greedy2>();
          break;
      case KakeUdonAI:
          res = ai_setup<KakeUdonAI::KakeUdonAI>();
          break;
      }
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

pair<NegAInoido::mode_t, MoveResult>
NegAInoido::move_suudon(mode_t mode) const {
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

MoveResult
NegAInoido::move() const {
  mode_t mode = (mode_t) info[MODE].asInt();
  int gtype = info[GRAPH_TYPE].asInt();
  MoveResult r((Json::Value()));
  info_for_import = info[ORG_INFO];
  std::cerr<< "Turn: " << history.size() / num_punters << " " << mode << endl;

  switch (gtype) {
  case SuUdonAI:
      r = trymove<SuUdonAI::SuUdonAI>();
      break;
  case Flowlight:
      r = trymove<flowlight::AI>();
      break;
  case Greedy2:
      r = trymove<Greedy2::Greedy2>();
      break;
  case KakeUdonAI:
      r = trymove<KakeUdonAI::KakeUdonAI>();
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
