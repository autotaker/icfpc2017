#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <utility>
#include <string>
#include <cstring>
#include <queue>
#include <cmath>
#include <iomanip>
#include <cassert>
#include <numeric>

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
    Json::Value setup() const override;
    tuple<int, int, Json::Value> move() const override;
    int simulation(pair<int, int>&) const ;
};

int Ichigo::simulation(pair<int, int>&edge) const {

    trace(make_pair("simulation", edge));

    vector<vector<Graph::River>> rivers(graph.rivers.size());
    for (size_t i = 0; i < graph.rivers.size(); ++i) {
        for (const auto& r : graph.rivers[i]) {
            rivers[i].emplace_back(r.to, r.punter);
        }
    }

    vector<vector<bool>> own(graph.num_vertices, vector<bool>(num_punters, false));
    for (int i = 0; i < graph.num_vertices; ++i) {
        for (auto& r : rivers[i]) {
            if (i == edge.first && r.to == edge.second) {
                r.punter = punter_id;
                own[i][punter_id] = true;
                own[r.to][punter_id] = true;
            }
            if (r.punter >= 0) {
                own[i][r.punter] = true;
                own[r.to][r.punter] = true;
            }
        }
    }

    int p = punter_id + 1; p %= num_punters;
    for (;;) {
        trace(p);

        std::vector<pair<int, int> > no_edge;
        std::vector<pair<int, int> > yes_edge;
        for (int i = 0; i < graph.num_vertices; ++i) {
            for (const auto& r : rivers[i]) {
                if (r.punter != -1) continue;
                if (own[i][p] and own[r.to][p]) {
                    no_edge.emplace_back(i, r.to);
                } else {
                    yes_edge.emplace_back(i, r.to);
                }
            }
        }

        std::pair<int, int> e;
        if (not yes_edge.empty()) {
            e = choose(yes_edge);
        } else if (not no_edge.empty()) {
            e = choose(no_edge);
        } else {
            break;
        }

        own[e.first][p] = true;
        own[e.second][p] = true;
        for (auto& r : rivers[e.first]) { if (r.to == e.second) { r.punter = p; break; } }
        for (auto& r : rivers[e.second]) { if (r.to == e.first) { r.punter = p; break; } }
        trace(make_tuple("put", p, e));

        p++;
        p %= num_punters;
    }

    vector<int> scores(num_punters, 0);

    for (int s = 0; s < graph.num_mines; ++s) {
        vector<int> d(graph.num_vertices, 1 << 29); d[s] = 0;
        priority_queue<pair<int, int>> q; q.push({0, s});
        while (not q.empty()) {
            int u = q.top().second; q.pop();
            for (auto&e: rivers[u]) {
                if (e.punter != p) continue;
                int v = e.to;
                int d2 = d[u] + 1;
                if (d2 < d[v]) {
                    d[v] = d2;
                    q.push({ -d2, v });
                }
            }
        }
        for (int u = 0; u < graph.num_vertices; ++u) {
            scores[p] += d[u] * d[u];
        }
    }

    return scores[punter_id];
}

Json::Value Ichigo::setup() const {

    vector<vector<int>> neigh(graph.num_vertices);
    for (int i = 0; i < graph.num_vertices; ++i) {
        for (const auto& r : graph.rivers[i]) {
            neigh[i].push_back(r.to);
        }
    }

    Json::Value memo;

    for (int s = 0; s < graph.num_mines; ++s) {
        vector<int> d(graph.num_vertices, 1 << 29); d[s] = 0;
        priority_queue<pair<int, int>> q; q.push({0, s});
        while (not q.empty()) {
            int u = q.top().second; q.pop();
            for (int v: neigh[u]) {
                int d2 = d[u] + 1;
                if (d2 < d[v]) {
                    d[v] = d2;
                    q.push({ -d2, v });
                }
            }
        }

        for (int u = 0; u < graph.num_vertices; ++u) {
            memo[u][s] = d[u];
        }
    }

    return memo;
}

tuple<int, int, Json::Value> Ichigo::move() const {
    srand(time(NULL));

    std::set<int> mines;
    for (int i = 0; i < graph.num_mines; ++i)
        mines.insert(i);
    for (int i = 0; i < graph.num_vertices; ++i) {
        for (const auto& r : graph.rivers[i]) {
            if (r.punter == punter_id) {
                mines.insert(i);
                mines.insert(r.to);
            }
        }
    }

    std::vector<pair<int, int> > no_edge;
    std::vector<pair<int, int> > yes_edge;
    for (int i = 0; i < graph.num_vertices; ++i) {
        for (const auto& r : graph.rivers[i]) {
            if (r.punter != -1) continue;
            int a = mines.count(i);
            int b = mines.count(r.to);
            if (a > 0 and b > 0) {
                no_edge.emplace_back(i, r.to);
            } else {
                yes_edge.emplace_back(i, r.to);
            }
        }
    }
    trace(yes_edge); trace(no_edge);

    std::pair<int, int> e;
    if (not yes_edge.empty()) {
        int mx = -1 * (1<<20);
        for (auto &edge: yes_edge) {
            auto score = simulation(edge);
            if (score > mx) {
                mx = score;
                e = edge;
            }
        }
    } else {
        e = choose(no_edge);
    }

    return make_tuple(e.first, e.second, info);
}

int main()
{
    Ichigo ai;
    ai.run();
    return 0;
}
