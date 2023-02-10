#ifndef PACKING_COST_H
#define PACKING_COST_H

#include "vpr_types.h"
#include "pack_move_utils.h"
struct t_packing_attraction {
    float timinggain;
    float connectiongain;
    float sharinggain;
};

float calculate_gain_from_attractions(const t_packing_attraction& attraction,
                                      AtomBlockId atom);

float calculate_atom_attraction_to_cluster(AtomBlockId atom,
                                           ClusterBlockId clb);

float calculate_molecule_attraction_to_cluster(const t_pack_molecule* molecule,
                                               ClusterBlockId clb);

bool evaluate_move_based_on_attraction(const std::vector<molMoveDescription>& proposed_moves);

#endif