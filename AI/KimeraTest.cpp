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

    vector<NegAInoido::graph_type_t> choices = {
Flowlight,
Greedy2,
KakeUdon,
Ichigo,
MCSlowLight,
Genocide,
    };

    vector<float> b = {
        0.787818557434,0.718599791624,0.306901250464,1.36529983692,-30000.0,0.852030547093
    };

    vector<vector<float>> w = {
        {-0.000176279037998,-0.0132435383837,9.38397403307e-05,-4.27608047695e-10,-5.0377792824e-10,0.00547125729499,-0.0112799754017,2.74281413327e-05},
        {0.000208833303716,0.00526469579451,-6.00473725105e-05,-1.78519973428e-09,-1.4995330259e-10,-0.00389328106269,-0.0266979374491,0.000507536929946},
        {6.74367627805e-05,-0.00116679645691,2.35696973824e-06,5.46657597063e-10,-1.37239774023e-10,0.0100648320365,0.0497433885267,-2.48455580933e-05},
        {0.000314021485241,-0.00900630001732,-8.34752038888e-05,-1.2524804408e-10,-2.81684243246e-10,-0.0220651585666,-0.139649118172,8.23388558674e-06},
        {0.000776994857147,0.0565417428326,-0.000280158671046,0.132509464717,-6.42871863796e-11,-0.072141442248,-0.00746625396721,0.0012093163449},
        {0.000700241483823,0.00358147032051,-1.49090574476e-05,-0.0149766191541,3.7983906045e-10,-0.0433422979714,0.00804464961264,-0.00093951314126}
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

    auto x = make_feature(*this, graph);
    int mx = -1 * (1 << 20);
    int idx = 0;
    for (int i = 0; i < (int)b.size(); ++i) {
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
      case MCSlowLight: res = ai_setup<Genocide::Genocide>(); break;
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
  case MCSlowLight: r = trymove<Genocide::Genocide>(); break;
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
