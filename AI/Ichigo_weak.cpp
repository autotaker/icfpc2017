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

namespace Ichigo_weak {

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
public:
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

    const int LIMIT_MSEC = 850;

    Graph roll = graph;
    for(
            auto start_time = chrono::system_clock::now();
            chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start_time).count() < LIMIT_MSEC;
       ) {
        for (size_t i = 0; i < roll.rivers.size(); ++i) {
            for (auto& r : roll.rivers[i]) {
                if (r.punter != -1) { continue; }
                r.punter = punter_id;
                map<pair<int, int>, int> painted; painted[{i, r.to}] = punter_id;
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

}

#ifndef _USE_AS_ENGINE
int main()
{
    Ichigo_weak::Ichigo ai;
    ai.run();
    return 0;
}
#endif

