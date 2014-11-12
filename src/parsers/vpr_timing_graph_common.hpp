#ifndef VPR_TIMING_GRAPH_PARSE_COMMON_H
#define VPR_TIMING_GRAPH_PARSE_COMMON_H

#include <vector>

typedef struct ipin_iblk_s {
    int ipin;
    int iblk;
} pin_blk_t;

typedef struct domain_skew_iodelay_s {
    int domain;
    float skew;
    float iodelay;
} domain_skew_iodelay_t;

typedef struct edge_s {
    int to_node;
    float delay;
} edge_t;

typedef struct node_arr_req_s {
    int node_id;
    float T_arr;
    float T_req;
} node_arr_req_t;

typedef struct timing_graph_level_s {
    int level;
    std::vector<int>* node_ids;
} timing_graph_level_t;

enum class TN_Type; //Forward declaration

typedef struct node_s {
    int node_id;
    TN_Type type;
    int ipin;
    int iblk;
    int domain;
    float skew;
    float iodelay;
    std::vector<edge_t>* out_edges;
} node_t;

#endif
