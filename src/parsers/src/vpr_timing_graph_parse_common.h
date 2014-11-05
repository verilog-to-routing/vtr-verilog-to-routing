#ifndef VPR_TIMING_GRAPH_PARSE_COMMON_H
#define VPR_TIMING_GRAPH_PARSE_COMMON_H

typedef struct ipin_iblk {
    int ipin;
    int iblk;
} pin_blk_t;

typedef struct domain_skew_iodelay {
    int domain;
    float skew;
    float iodelay;
} domain_skew_iodelay_t;

typedef struct edge_t {
    int to_node;
    float delay;
} edge_t;

#endif
