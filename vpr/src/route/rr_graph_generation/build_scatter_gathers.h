#pragma once

#include "scatter_gather_types.h"
#include "rr_types.h"

#include <vector>

void alloc_and_load_scatter_gather_connections(const std::vector<t_scatter_gather_pattern>& scatter_gather_patterns,
                                               const std::vector<bool>& inter_cluster_rr,
                                               const t_chan_details& chan_details_x,
                                               const t_chan_details& chan_details_y);