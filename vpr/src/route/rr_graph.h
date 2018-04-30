#ifndef RR_GRAPH_H
#define RR_GRAPH_H

/* Include track buffers or not. Track buffers isolate the tracks from the
   input connection block. However, they are difficult to lay out in practice,
   and so are not currently used in commercial architectures. */
#define INCLUDE_TRACK_BUFFERS false

#include "device_grid.h"

enum e_graph_type {
    GRAPH_GLOBAL, /* One node per channel with wire capacity > 1 and full connectivity */
    GRAPH_BIDIR, /* Detailed bidirectional graph */
    GRAPH_UNIDIR, /* Detailed unidir graph, untilable */
    /* RESEARCH TODO: Get this option debugged */
    GRAPH_UNIDIR_TILEABLE /* Detail unidir graph with wire groups multiples of 2*L */
};
typedef enum e_graph_type t_graph_type;

/* Warnings about the routing graph that can be returned.
 * This is to avoid output messages during a value sweep */
enum {
    RR_GRAPH_NO_WARN = 0x00,
    RR_GRAPH_WARN_FC_CLIPPED = 0x01,
    RR_GRAPH_WARN_CHAN_WIDTH_CHANGED = 0x02
};

void create_rr_graph(
        const t_graph_type graph_type,
        const int num_block_types, const t_type_ptr block_types,
        const DeviceGrid& grid,
        t_chan_width *nodes_per_chan,
        const int num_arch_switches,
        t_det_routing_arch* det_routing_arch,
        const t_segment_inf * segment_inf,
        const enum e_base_cost_type base_cost_type,
        const bool trim_empty_channels,
        const bool trim_obs_channels,
        const t_direct_inf *directs, const int num_directs,
        int *num_rr_switches,
        int *Warnings,
        bool route_clock);

void free_rr_graph();

void dump_rr_graph(const char *file_name);
void print_rr_indexed_data(FILE * fp, int index); /* For debugging only */

//Returns a brief one-line summary of an RR node
std::string describe_rr_node(int inode);

void print_rr_node(FILE *fp, const std::vector<t_rr_node> &L_rr_node, int inode);

void init_fan_in(std::vector<t_rr_node>& L_rr_node, const int num_rr_nodes);

#endif

