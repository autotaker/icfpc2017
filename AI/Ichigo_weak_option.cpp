#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <utility>
#include <string>
#include <cstring>
#include <queue>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <cassert>
#include <numeric>
#include <random>

#include "../lib/Game.h"


static int owner(const Graph& g, int src, int to) {
  const auto& rs = g.rivers[src];
  return std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter;
}

static void claim(Graph& g, int src, int to, int p) {
  auto& rs = g.rivers[src];
  auto& rt = g.rivers[to];
  if (owner(g, src, to) == -1) {
    std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter = p;
    std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->punter = p;
  } else {
    std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->option = p;
    std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->option = p;    
  }
}

static void unclaim(Graph& g, int src, int to, int p) {
  auto& rs = g.rivers[src];
  auto& rt = g.rivers[to];
  if (owner(g, src, to) == p) {
    std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->punter = -1;
    std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->punter = -1;    
  } else {
    std::lower_bound(rs.begin(), rs.end(), Graph::River{to})->option = -1;
    std::lower_bound(rt.begin(), rt.end(), Graph::River{src})->option = -1;    
  }
}


#define trace(var) cerr<<">>> "<<#var<<" = "<<var<<endl;
#define choose(vec) (vec[rand() % vec.size()])

template<class S, class T> inline
std::ostream& operator<<(std::ostream&os, std::pair<S,T> p) {
    return os << '(' << p.first << ", " << p.second << ')';
}

template<class T, class U> inline
std::ostream& operator<<(std::ostream&os, std::tuple<T,U> t) {
    return os << '(' << std::get<0>(t) << ", " << std::get<1>(t) << ')';
}

template<class S, class T, class U> inline
std::ostream& operator<<(std::ostream&os, std::tuple<S,T,U> t) {
    return os << '(' << std::get<0>(t) << ", " << std::get<1>(t) << ", " << std::get<2>(t) << ')';
}

template<class T> inline
std::ostream& operator<<(std::ostream&os, std::set<T> v) {
    os << "(set"; for (T item: v) os << ' ' << item; os << ")"; return os;
}

template<class T> inline
std::ostream& operator<<(std::ostream&os, std::vector<T> v) {
    if (v.size() == 0) return os << "(empty)";
    os << v[0]; for (int i=1, len=v.size(); i<len; ++i) os << ' ' << v[i];
    return os;
}

using namespace std;
class Ichigo : public Game {
    SetupSettings setup() const override;
    MoveResult move() const override;
};

SetupSettings Ichigo::setup() const {
    return Json::Value();
}

MoveResult Ichigo::move() const
{
    random_device dev;
    mt19937 rand(dev());

    map<pair<int, int>, int> values;

    const int LIMIT_MSEC = 300;

    vector<int> opt_used(num_punters, 0);
    for (int u = 0; u < graph.num_vertices; ++u) {
        for (const auto& river : graph.rivers[u]) {
            if (u < river.to && river.punter != -1 && river.option != -1) {
                ++opt_used[river.option];
            }
        }
    }
    int opt_usable = options_enabled ? graph.num_mines : 0;

    Graph roll = graph;
    for(
            auto start_time = chrono::system_clock::now();
            chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start_time).count() < LIMIT_MSEC;
       ) {
        for (size_t i = 0; i < roll.rivers.size(); ++i) {
            for (auto& r : roll.rivers[i]) {
                if (r.punter != -1) {
                    if (r.option != -1) continue;
                    if (r.punter == punter_id) continue;
                }
                if (r.punter != -1) {
                    if (opt_used[punter_id] == opt_usable) {
                        continue;
                    }
                    claim(roll, i, r.to, punter_id);
                } else {
                    claim(roll, i, r.to, punter_id);
                }
                map<pair<int, int>, int> painted;
                vector<Graph::River*> replaced;

                for (size_t i = 0; i < roll.rivers.size(); ++i) {
                    for (auto& r : roll.rivers[i]) {
                        if (r.punter != -1) { continue; }
                        if (painted.count({r.to, i})) {  // revere edge was painted
                            r.punter = painted[{r.to, i}];
                        } else {
                            r.punter = rand() % num_punters;
                        }
                        replaced.emplace_back(&r);
                    }
                }

                auto scores = roll.evaluate(num_punters, shortest_distances);
                // values[{i, r.to}] += scores[punter_id] > scores[(punter_id + 1) % num_punters] ? 1 : 0;
                int wins = 0; for (auto&s: scores) if (scores[punter_id] >= s) wins++;
                values[{i, r.to}] += wins;

                for (auto&r: replaced) {
                    assert(r->punter != -1);
                    r->punter = -1;
                }
                unclaim(roll, i, r.to, punter_id);
            }
        }
    }

    int mx = -1 * (1 << 20);
    pair<int, int> e;
    for (auto &kv: values) { e = kv.first; break; }
    for (auto &kv: values) {
        if (mx < kv.second) {
            mx = kv.second;
            e = kv.first;
        }
    }

    trace(make_pair("selected", e));
    return make_tuple(e.first, e.second, info);
}

int main()
{
    Ichigo ai;
    ai.run();
    return 0;
}
