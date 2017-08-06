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
    Json::Value info;
    info["turn"] = punter_id + 1;

    int diag;
    {
        diag = 0;
        for (int i = 0; i < graph.num_mines; ++i) {
            for (int j = 0; j < graph.num_vertices; ++j) {
                if (diag < shortest_distances[i][j]) {
                    diag = shortest_distances[i][j];
                }
            }
        }
    }
    info["diag"] = diag;

    return info;
}

// (mine-reachable, max-length)
tuple<bool, int> f(const Graph& graph, int s, int punter_id) {

    bool reachable = false;
    int max_length = 0;

    const int inf = 1 << 29;
    vector<int> visited(graph.num_vertices, inf);
    visited[s] = 0;

    priority_queue<pair<int, int>> q; q.push({0, s});

    while (not q.empty()) {
        int u; u = q.top().second; q.pop();
        for (auto&r: graph.rivers[u]) {
            if (r.punter != punter_id) continue;
            auto v = r.to;
            if (v < graph.num_mines) reachable = true;
            int len = visited[u] + 1;
            if (visited[v] > len) {
                visited[v] = len;
                q.push({-len, v});
                if (len > max_length) { max_length = len; }
            }
        }
    }

    return {reachable, max_length};
}

tuple<int, int, Json::Value> Ichigo::move() const
{
    random_device dev;
    mt19937 rand(dev());

    // info
    Json::Value next_info = info;
    int turn = info["turn"].asInt(); trace(turn);
    int diag = info["diag"].asInt(); trace(diag);
    next_info["turn"] = turn + num_punters;

    map<pair<int, int>, int> values;

    Graph roll = graph;
    for(
            auto start_time = chrono::system_clock::now();
            chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start_time).count() < 900;
       ) {
        for (size_t i = 0; i < roll.rivers.size(); ++i) {
            for (auto& r : roll.rivers[i]) {
                if (r.punter != -1) { continue; }
                r.punter = punter_id;
#ifdef FIX
                map<pair<int, int>, int> painted; painted[{i, r.to}] = punter_id;
#endif

                vector<Graph::River*> replaced;

                for (size_t i = 0; i < roll.rivers.size(); ++i) {
                    for (auto& r : roll.rivers[i]) {
                        if (r.punter != -1) { continue; }

                        // active disturbance
                        for (int pid = 0; pid < num_punters; ++pid) {
                            if (pid == punter_id) continue;
                            bool r1, r2; int l1, l2;
                            tie(r1, l1) = f(graph, i, pid);
                            tie(r2, l2) = f(graph, r.to, pid);
                            if ((l1 + l2 > 2) and (r1 or r2)) { // future
                                r.punter = pid;
                            }
                            if (l1 + l2 > 20) {  // too long path
                                r.punter = pid;
                            }
                        }

#ifdef FIX
                        if (painted.count({r.to, i})) {  // revere edge was painted
                            r.punter = painted[{r.to, i}];
                        }
#endif
                        r.punter = rand() % num_punters;
                        replaced.emplace_back(&r);
                    }
                }

                auto scores = roll.evaluate(num_punters, shortest_distances);

                // mine-edge bonus
                if ((int)i < graph.num_mines or r.to < graph.num_mines) {
                    scores[punter_id] += 30;
                }

                // imcomplete information
                for (int p = 0; p < num_punters; ++p) {
                    if (p == punter_id) continue;
                    if (rand() % 2 == 0) {
                        scores[p] *= 2;
                    } else {
                        scores[p] = scores[p] * 5 / 3;
                    }
                }

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

int main()
{
    Ichigo ai;
    ai.run();
    return 0;
}
