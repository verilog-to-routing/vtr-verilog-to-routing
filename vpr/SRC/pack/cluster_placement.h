/* 
Find placement for group of logical blocks in complex block
Author: Jason Luu
*/

#ifndef CLUSTER_PLACEMENT_H
#define CLUSTER_PLACEMENT_H
#include "arch_types.h"
#include "util.h"

t_cluster_placement_stats *alloc_and_load_cluster_placement_stats();
void get_next_primitive_list(INOUTP t_cluster_placement_stats *cluster_placement_stats, INP t_pack_molecule *molecule, INOUTP t_pb_graph_node **primitives_list);
void commit_primitive(INOUTP t_cluster_placement_stats *cluster_placement_stats, INP t_pb_graph_node *primitive);
void reset_cluster_placement_stats(INOUTP t_cluster_placement_stats *cluster_placement_stats);
void free_cluster_placement_stats(INOUTP t_cluster_placement_stats *cluster_placement_stats);

#endif
