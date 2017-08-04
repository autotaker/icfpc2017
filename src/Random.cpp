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

#include "../src/Game.h"

struct UnionFind{
  /*
    verified

    size manipulation
    http://codeforces.com/contest/516/submission/9910341
  */
  
  std::vector<int> par, size;
  std::vector<int> vis;
  int vcnt;
  int n;
  UnionFind(int n):par(n), size(n), vis(n), vcnt(0), n(n){
    init();
  };

  void init(){
    ++vcnt;
    // fill(size.begin(), size.end(), 1);
    // fill(par.begin(), par.end(), -1);
  }
  
  int find(int x){
    if(vis[x] != vcnt){
      vis[x] = vcnt;
      par[x] = -1;
      size[x] = 1;
    }
    
    if(par[x]==-1) return x;
    return par[x] = find(par[x]);
  }
		
  bool unit(int a,int b){
    int ap = find(a);
    int bp = find(b);
    
    if(ap == bp)
      return false;
    size[bp] += size[ap];
    par[ap] = bp;
    return true;
  }
  
  int& getsize(int a){
    return size[find(a)];
  }
};


using namespace std;
class RandomAI : public Game {
  Json::Value setup() const override;
  tuple<int, int, Json::Value> move() const override;
};

Json::Value RandomAI::setup() const {
  return Json::Value();
}

tuple<int, int, Json::Value> RandomAI::move() const {
  srand(time(NULL));

  UnionFind uf(graph.num_vertices);
  std::vector<pair<int, int> > no_edge;
  std::vector<pair<int, int> > yes_edge;

  for (size_t i = 0; i < graph.rivers.size(); ++i) {
    for (const auto& r : graph.rivers[i]) {
      if (r.punter == punter_id) {
	uf.unit(i, r.to);
      }
    }
  }

  for (size_t i = 0; i < graph.rivers.size(); ++i) {
    for (const auto& r : graph.rivers[i]) {
      if (r.punter != -1) continue;
      
      if (uf.find(i) == uf.find(r.to)) {
	no_edge.emplace_back(i, r.to);
      } else {
	yes_edge.emplace_back(i, r.to);	
      }
    }
  }

  std::pair<int, int> e;

  if (yes_edge.empty()) {
    e = no_edge[rand() % no_edge.size()];
  } else {
    e = yes_edge[rand() % yes_edge.size()];
  }

  return make_tuple(e.first, e.second, Json::Value());
}

int main()
{
  RandomAI ai;
  ai.run();
  return 0;
}
