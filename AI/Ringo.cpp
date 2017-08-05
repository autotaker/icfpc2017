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
    tuple<int, int, Json::Value> move() const override;
};

SetupSettings Ichigo::setup() const {
    vector<pair<int, int>> ps;
    for (int i = 0; i < graph.num_vertices; ++i) {
        for (int j = 0; j < (int)graph.rivers[i].size(); ++j) {
            int to = graph.rivers[i][j].to;
            if (i < graph.num_mines or to < graph.num_mines) {
                ps.emplace_back(i, j);
            }
        }
    }
    std::shuffle(ps.begin(), ps.end(), std::mt19937());
    auto info = Json::Value();
    for (int i = 0; i < (int)ps.size(); ++i) {
        info[i] = Json::Value();
        info[i][0] = ps[i].first;
        info[i][1] = ps[i].second;
    }
    return info;
}

tuple<int, int, Json::Value> Ichigo::move() const
{
    random_device dev;
    mt19937 rand(dev());

    // actively mine edge
    {
        vector<int> dims(graph.num_mines, 0);
        for (size_t idx = 0; idx < info.size(); ++idx) {
            int i = info[(int)idx][0].asInt();
            int j = info[(int)idx][1].asInt();
            int to = graph.rivers[i][j].to;
            int punter = graph.rivers[i][j].punter;
            int dim = dims[i < graph.num_mines ? i : to];
            if (punter == -1 and dim < 5) {
                return make_tuple(i, to, info);
            } else if (punter == punter_id) {
                if (i < graph.num_mines) dims[i]++;
                if (to < graph.num_mines) dims[to]++;
            }
        }
    }

    map<pair<int, int>, int> values;

    auto start_time = chrono::system_clock::now();

    Graph roll = graph;
    for (size_t _ = 0; _ < 1000; ++_) {
        auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start_time).count();
        if (elapsed_time >= 900) {
            break;
        }
        for (size_t i = 0; i < roll.rivers.size(); ++i) {
            for (auto& r : roll.rivers[i]) {
                if (r.punter != -1) { continue; }
                r.punter = punter_id;

                vector<Graph::River*> replaced;

                for (size_t i = 0; i < roll.rivers.size(); ++i) {
                    for (auto& r : roll.rivers[i]) {
                        if (r.punter != -1) { continue; }
                        r.punter = rand() % num_punters;
                        replaced.emplace_back(&r);
                    }
                }

                auto scores = roll.evaluate(num_punters, shortest_distances);
                values[{i, r.to}] += scores[punter_id] - scores[(punter_id + 1) % num_punters];

                for (auto&r: replaced) {
                    assert(r->punter != -1);
                    r->punter = -1;
                }
                r.punter = -1;
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
