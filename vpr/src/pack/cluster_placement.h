/*
 * Find placement for group of atom blocks in complex block
 * Author: Jason Luu
 */

#ifndef CLUSTER_PLACEMENT_H
#define CLUSTER_PLACEMENT_H
#include "arch_types.h"

t_cluster_placement_stats* alloc_and_load_cluster_placement_stats();
bool get_next_primitive_list(
    t_cluster_placement_stats* cluster_placement_stats,
    const t_pack_molecule* molecule,
    t_pb_graph_node** primitives_list);
void commit_primitive(t_cluster_placement_stats* cluster_placement_stats,
                      const t_pb_graph_node* primitive);
void set_mode_cluster_placement_stats(const t_pb_graph_node* complex_block,
                                      int mode);
void reset_cluster_placement_stats(
    t_cluster_placement_stats* cluster_placement_stats);
void free_cluster_placement_stats(
    t_cluster_placement_stats* cluster_placement_stats);

int get_array_size_of_molecule(const t_pack_molecule* molecule);
bool exists_free_primitive_for_atom_block(
    t_cluster_placement_stats* cluster_placement_stats,
    const AtomBlockId blk_id);

void reset_tried_but_unused_cluster_placements(
    t_cluster_placement_stats* cluster_placement_stats);

#endif
