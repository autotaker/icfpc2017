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
    return Json::Value();
}

tuple<int, int, Json::Value> Ichigo::move() const
{
    random_device dev;
    mt19937 rand(dev());

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
