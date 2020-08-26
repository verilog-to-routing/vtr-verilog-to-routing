#ifndef RR_GRAPH_UTIL_H
#define RR_GRAPH_UTIL_H

int seg_index_of_cblock(t_rr_type from_rr_type, int to_node);

int seg_index_of_sblock(int from_node, int to_node);

void reorder_rr_graph_nodes(const t_router_opts& router_opts);

#endif
