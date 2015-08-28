#pragma once
#include <cstdio>
#include <ctime>
#include <cmath>

#include <assert.h>

#include <vector>
#include <algorithm>

#include "vpr_utils.h"
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "route_export.h"
#include "route_common.h"
#include "route_tree_timing.h"
#include "route_timing.h"
#include "path_delay.h"
#include "net_delay.h"
#include "stats.h"

/* to be called in timing_driven_expand_neighbours */
void print_start_end_to_sink(bool is_start, int itry, int inet, int target_node, int itarget);
void print_expand_neighbours(bool is_parent, int inode, int target_node, 
    int target_chan_type, int target_pin_x, int target_pin_y,
    float expected_cost, float future_Tdel, float new_tot_cost,
    float cur_basecost, float cong_cost);
void setup_max_min_criticality(float &max_crit, float &min_crit, 
        int &max_inet, int &min_inet, int &max_ipin, int &min_ipin, t_slack * slacks);
void print_critical_path_from_rt(struct s_heap *hptr, float target_criticality, int new_nodes_count, float actual_Tdel);
void setup_new_lookahead_dev(int furthest_wire_inode, int target_node, float total_Tdel, int new_nodes_count);
