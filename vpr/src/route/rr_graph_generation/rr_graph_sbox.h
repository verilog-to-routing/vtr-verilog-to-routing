#pragma once

#include <vector>
#include "rr_graph_type.h"
#include "rr_node_types.h"
#include "vtr_ndmatrix.h"

std::vector<int> get_switch_box_tracks(const int from_i,
                                       const int from_j,
                                       const int from_track,
                                       const e_rr_type from_type,
                                       const int to_i,
                                       const int to_j,
                                       const e_rr_type to_type,
                                       const std::vector<int>*** switch_block_conn);

vtr::NdMatrix<std::vector<int>, 3> alloc_and_load_switch_block_conn(t_chan_width* nodes_per_chan,
                                                                    enum e_switch_block_type switch_block_type,
                                                                    int Fs);

int get_simple_switch_block_track(enum e_side from_side, enum e_side to_side, int from_track, enum e_switch_block_type switch_block_type, const int from_chan_width, const int to_chan_width);
