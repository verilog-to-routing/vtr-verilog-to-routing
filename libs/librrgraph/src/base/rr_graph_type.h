#ifndef RR_GRAPH_TYPE_H
#define RR_GRAPH_TYPE_H

#include <vector>
#include "physical_types.h"

struct t_chan_width {
    int max = 0;
    int x_max = 0;
    int y_max = 0;
    int x_min = 0;
    int y_min = 0;
    std::vector<int> x_list;
    std::vector<int> y_list;
};

enum e_route_type {
    GLOBAL,
    DETAILED
};

enum e_graph_type {
    GRAPH_GLOBAL, /* One node per channel with wire capacity > 1 and full connectivity */
    GRAPH_BIDIR,  /* Detailed bidirectional graph */
    GRAPH_UNIDIR, /* Detailed unidir graph, untilable */
    /* RESEARCH TODO: Get this option debugged */
    GRAPH_UNIDIR_TILEABLE /* Detail unidir graph with wire groups multiples of 2*L */
};
typedef enum e_graph_type t_graph_type;

/* This map is used to get indices w.r.t segment_inf_x or segment_inf_y based on parallel_axis of a segment, 
 * from indices w.r.t the **unified** segment vector, segment_inf in devices context which stores all segments 
 * regardless of their axis. (see get_parallel_segs for more details)*/
typedef std::unordered_multimap<size_t, std::pair<size_t, e_parallel_axis>> t_unified_to_parallel_seg_index;

#endif