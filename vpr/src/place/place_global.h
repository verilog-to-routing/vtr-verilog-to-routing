/**
 * @file
 * @brief This file contains all the global data structures referenced across
 *        multiple files in ./vpr/src/place.
 *
 * These global data structures were originally local to place.cpp, and they
 * were referenced by a lot of routines local to place.cpp. However, to shorten
 * the file size of place.cpp, these routines are moved to other files.
 *
 * Instead of elongating the argument list of the moved routines, I moved the
 * data structures to here so that they can be easily shared across different
 * files.
 *
 * For detailed descriptions on what each data structure stores, please see
 * place.cpp, where these variables are defined.
 */

#pragma once
#include <vector>
#include "vtr_vector.h"
#include "vpr_net_pins_matrix.h"
#include "timing_place.h"

extern vtr::vector<ClusterNetId, double> net_cost, proposed_net_cost;
extern vtr::vector<ClusterNetId, char> bb_updated_before;
extern ClbNetPinsMatrix<float> connection_delay;
extern ClbNetPinsMatrix<float> proposed_connection_delay;
extern ClbNetPinsMatrix<float> connection_setup_slack;
extern PlacerTimingCosts connection_timing_cost;
extern ClbNetPinsMatrix<double> proposed_connection_timing_cost;
extern vtr::vector<ClusterNetId, double> net_timing_cost;
extern vtr::vector<ClusterNetId, t_bb> bb_coords, bb_num_on_edges;
extern vtr::vector<ClusterNetId, t_bb> ts_bb_coord_new, ts_bb_edge_new;
extern float** chanx_place_cost_fac;
extern float** chany_place_cost_fac;
extern std::vector<ClusterNetId> ts_nets_to_update;
