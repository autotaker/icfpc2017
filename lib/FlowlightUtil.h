#ifndef FLOWLIGHTUTIL_H
#define FLOWLIGHTUTIL_H

#include "Game.h"

namespace flowlight {
  int get_num_edges(const Graph &graph);
  int is_passable_without_option(const Graph::River &river, int punter);
  Graph build_shortest_path_dag(const Graph &original_graph, int source, int target, int punter_id);
}

#endif /* FLOWLIGHTUTIL_H */
