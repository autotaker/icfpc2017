#include <iostream>

#include "Game.h"
#include "json/json.h"

#define _USE_AS_ENGINE
#include "Flowlight.cpp"
#include "MCTS_greedy.cpp"
#include "Greedy2.cpp"
#include "KakeUdon.cpp"
#include "Ichigo_weak.cpp"
#include "Genocide.cpp"
#include "MCSlowLight.cpp"
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
Flowlight,
Greedy2,
KakeUdon,
Ichigo,
MCSlowLight,
Genocide,
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

    vector<NegAInoido::graph_type_t> choices = {
Flowlight,
Greedy2,
KakeUdon,
Ichigo,
MCSlowLight,
Genocide,
    };

    vector<float> b = {
        0.687248723467,0.669131293245,0.274049206003,1.29787110957,0.0724269001234,0.871307046885
    };

    vector<vector<float>> w = {
        {0.000143081288528,-0.000920165080967,3.21326435139e-05,-5.40988995444e-11,-0.0265642525304,0.0114791576084,-8.97721218696e-05},
        {0.000247389145987,0.00739049330714,-5.68057030318e-05,-1.22086962207e-09,-0.0110338113813,-0.0146038620957,0.000359560581264},
        {0.000144472757728,0.00212002912206,-1.22892227427e-05,6.53650321114e-10,0.00169919284695,0.0569392428296,-5.59666880156e-05},
        {0.000472137643745,-0.00226009993699,-0.000113536471877,9.43542148844e-11,-0.039235609002,-0.124879647861,-5.56421442917e-05},
        {0.000938898559101,0.0602377656568,-0.000333726640964,0.13048868542,-0.0786545212129,-0.00438445637404,0.00131897310793},
        {-0.000442138280474,-0.0247094265251,0.000445961532847,0.0105249506503,-0.00928299608434,0.00287619153473,-0.00231034223779}
    };

    if (not futures_enabled) {
        for (int i = 0; i < (int)b.size(); ++i) {
            if (choices[i] == Flowlight
                    or choices[i] == KakeUdon
                    or choices[i] == MCSlowLight
               ) {
                b[i] = -1000.0;
            }
        }
    }

    vector<double> f;
    f.push_back( graph.num_vertices );
    f.push_back( graph.num_mines );

    {
        int sum = 0;
        for (int i = 0; i < graph.num_vertices; ++i) {
            sum += graph.rivers[i].size();
        }
        f.push_back(sum);
    }

    {
        int mn = 1 << 29;
        for (int i = 0; i < graph.num_mines; ++i) {
            for (int j = i + 1; j < graph.num_mines; ++j) {
                mn = min<int>(mn, shortest_distances[i][j]);
            }
        }
        f.push_back(mn);
    }

    // {
    //     int mx = 0;
    //     for (int i = 0; i < graph.num_mines; ++i) {
    //         for (int j = 0; j < graph.num_vertices; ++j) {
    //             mx = max<int>(mx, shortest_distances[i][j]);
    //         }
    //     }
    //     f.push_back(mx);
    // }

    {
        double sum = 0.0;
        for (int i = 0; i < graph.num_mines; ++i) {
            for (int j = i + 1; j < graph.num_mines; ++j) {
                sum += shortest_distances[i][j];
            }
        }
        f.push_back(( (sum * 2 / graph.num_mines / (graph.num_mines + 1)) ));
    }

    {
        float sum = 0.0;
        for (int i = 0; i < graph.num_mines; ++i) {
            sum += graph.rivers[i].size();
        }
        f.push_back(((sum / graph.num_mines) ));
    }

    {
        float sum = 0.0;
        for (int i = 0; i < graph.num_vertices; ++i) {
            sum += graph.rivers[i].size();
        }
        f.push_back(((sum / graph.num_mines) ));
    }

    auto x = f;
    assert(x.size() == 7);
    double mx = -1 * (1 << 20);
    int idx = 0;
    for (int i = 0; i < (int)b.size(); ++i) {
        trace(i);
        float y = 0;
        y += b[i];
        cerr << y << endl;
        for (int j = 0; j < (int)x.size(); ++j) {
            y += x[j] * w[i][j];
            cerr << x[j] << " * " << w[i][j] << endl;
        }
        cerr << i << ": y=" << y << endl;
        trace(mx);
        if (mx < y) {
            mx = y;
            idx = i;
        }
    }

    if (choices[idx] == Greedy2) {
        cerr << "choice Genocide istead of Greedy2!!" << endl;
        return Genocide;
    }

    cerr << "choice " << idx << "-th choice" << endl;
    return choices[idx];
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
      case Flowlight: res = ai_setup<flowlight::AI>(); break;
      case Greedy2: res = ai_setup<Greedy2::Greedy2>(); break;
      case KakeUdon: res = ai_setup<KakeUdonAI::KakeUdonAI>(); break;
      case Ichigo: res = ai_setup<Ichigo_weak::Ichigo>(); break;
      case MCSlowLight: res = ai_setup<MCSlowLight::AI>(); break;
      case Genocide: res = ai_setup<Genocide::Genocide>(); break;
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
  case Flowlight: r = trymove<flowlight::AI>(); break;
  case Greedy2: r = trymove<Greedy2::Greedy2>(); break;
  case KakeUdon: r = trymove<KakeUdonAI::KakeUdonAI>(); break;
  case Ichigo: r = trymove<Ichigo_weak::Ichigo>(); break;
  case MCSlowLight: r = trymove<MCSlowLight::AI>(); break;
  case Genocide: r = trymove<Genocide::Genocide>(); break;
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
