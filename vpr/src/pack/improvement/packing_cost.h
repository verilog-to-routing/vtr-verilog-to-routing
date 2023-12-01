#ifndef PACKING_COST_H
#define PACKING_COST_H

#include "vpr_types.h"
#include "pack_move_utils.h"
struct t_packing_attraction {
    float timinggain = 0;
    float connectiongain = 0;
    float sharinggain = 0;
};

const int HIGH_FANOUT_NET_THRESHOLD = 5;

float calculate_gain_from_attractions(const t_packing_attraction& attractions,
                                      const t_pack_molecule* molecule);

float calculate_molecule_attraction_to_cluster(const std::unordered_set<AtomBlockId>& moving_atoms,
                                               const std::unordered_set<AtomNetId>& moving_nets,
                                               const t_pack_molecule* molecule,
                                               ClusterBlockId clb);

bool evaluate_move_based_on_attraction(const std::vector<molMoveDescription>& proposed_moves);

#endif