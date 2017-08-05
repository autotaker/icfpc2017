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
    int simulation(int, vector<pair<int, int>>&) const ;
};

int Ichigo::simulation(int selected, vector<pair<int, int>>&edges) const {

    trace(make_pair("simulation", edges[selected]));

    vector<vector<Graph::River>> rivers(graph.rivers.size());
    for (size_t i = 0; i < graph.rivers.size(); ++i) {
        for (const auto& r : graph.rivers[i]) {
            if (r.punter != -1) {
                rivers[i].emplace_back(r.to, r.punter);
            }
        }
    }

    int p = punter_id;
    for (size_t i = 0; i < edges.size(); ++i) {
        if ((int)i == selected) continue;
        p = (p + 1) % num_punters;
        int s = edges[i].first;
        int t = edges[i].second;
        rivers[s].emplace_back(t, p);
        rivers[t].emplace_back(s, p);
    }

    {
        auto edge = edges[selected];
        int s = edge.first;
        int t = edge.second;
        rivers[s].emplace_back(t, punter_id);
        rivers[t].emplace_back(s, punter_id);
    }

    vector<int> scores(num_punters, 0);
    for (int mine = 0; mine < graph.num_mines; ++mine) {
        // calculate shortest distances from a mine
        std::vector<int> distances(graph.num_vertices, 1<<29);
        std::queue<int> que;
        que.push(mine);
        distances[mine] = 0;
        while (!que.empty()) {
            const int u = que.front();
            que.pop();
            for (const auto& river : rivers[u]) {
                const int v = river.to;
                if (distances[v] > distances[u] + 1) {
                    distances[v] = distances[u] + 1;
                    que.push(v);
                }
            }
        }
        // calculate score for each punter
        for (int punter = 0; punter < num_punters; ++punter) {
            std::vector<int> visited(graph.num_vertices, 0);
            que = std::queue<int>();
            que.push(mine);
            visited[mine] = 1;
            while (!que.empty()) {
                const int u = que.front();
                que.pop();
                for (const auto& river : rivers[u]) if (river.punter == punter) {
                    const int v = river.to;
                    if (!visited[v]) {
                        scores[punter] += distances[v] * distances[v];
                        visited[v] = 1;
                        que.push(v);
                    }
                }
            }
        }
    }

    int max_score = 0;
    for (int i = 0; i < num_punters; ++i) {
        int a = scores[punter_id] - scores[i];
        if (a > max_score) max_score = a;
    }
    return max_score;
}

SetupSettings Ichigo::setup() const {
    return Json::Value();
}

tuple<int, int, Json::Value> Ichigo::move() const {
    srand(time(NULL));

    vector<pair<int, int>> rest_edges;
    for (size_t i = 0; i < graph.rivers.size(); ++i) {
        for (const auto& r : graph.rivers[i]) {
            if (r.punter == -1) {
                rest_edges.emplace_back(i, r.to);
            }
        }
    }

    int mx = -1 * (1 << 20);
    std::pair<int, int> e;

    for (int _ = 0; _ < 10; ++_) {
        std::shuffle(rest_edges.begin(), rest_edges.end(), std::mt19937());
        for (size_t i = 0; i < rest_edges.size(); ++i) {
            auto score = simulation(i, rest_edges);
            if (score > mx) {
                mx = score;
                e = rest_edges[i];
            }
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
