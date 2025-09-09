#pragma once

#include "scatter_gather_types.h"
#include "rr_types.h"
#include "rr_graph_type.h"
#include "switchblock_scatter_gather_common_utils.h"

#include <vector>

struct t_chan_loc {
    t_physical_tile_loc location;
    e_rr_type chan_type;
    e_side side;
};

struct t_sg_candidate {
    t_chan_loc chan_loc;
    t_wire_switchpoint wire_switchpoint;
};

struct t_bottleneck_link {
    std::vector<t_sg_candidate> gather_fanin_connections;
    std::vector<t_sg_candidate> scatter_fanout_connections;
};

std::vector<t_bottleneck_link> alloc_and_load_scatter_gather_connections(const std::vector<t_scatter_gather_pattern>& scatter_gather_patterns,
                                                                         const std::vector<bool>& inter_cluster_rr,
                                                                         const t_chan_details& chan_details_x,
                                                                         const t_chan_details& chan_details_y,
                                                                         const t_chan_width& nodes_per_chan);