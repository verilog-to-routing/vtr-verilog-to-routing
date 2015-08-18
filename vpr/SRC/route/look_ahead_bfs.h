#pragma once
#include "physical_types.h"
// new data structure
struct s_heap_2 {
    int prev_node;
    int inode;
    int position;
    float cost;
    float R_upstream;
    float C_upstream;
    float basecost_upstream;
};
void look_ahead_bfs(int start_inode, t_segment_inf *segment_inf);
void pre_cal_look_ahead(t_segment_inf *segment_inf, int num_segment);
void print_cost_map(char *fname);
void print_C_map(float **c_map);
void print_bfs_optimal_path(int term_node);
