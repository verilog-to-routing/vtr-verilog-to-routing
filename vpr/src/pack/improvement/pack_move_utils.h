//
// Created by elgammal on 2022-09-13.
//

#ifndef VTR_PACK_MOVE_UTILS_H
#define VTR_PACK_MOVE_UTILS_H

#include "vpr_types.h"

//#define pack_improve_debug

const int LARGE_FANOUT_LIMIT = 5;

struct molMoveDescription {
    t_pack_molecule* molecule_to_move = nullptr;
    int molecule_size = 0;
    ClusterBlockId new_clb = INVALID_BLOCK_ID;
};

t_pack_molecule* pick_molecule_randomly();
bool pick_molecule_connected(t_pack_molecule* mol_1, t_pack_molecule*& mol_2);
bool pick_molecule_connected_same_type(t_pack_molecule* mol_1, t_pack_molecule*& mol_2);
bool pick_molecule_connected_compatible_type(t_pack_molecule* mol_1, t_pack_molecule*& mol_2);
bool pick_molecule_connected_same_size(t_pack_molecule* mol_1, t_pack_molecule*& mol_2);

void build_mol_move_description(std::vector<molMoveDescription>& new_locs,
                                t_pack_molecule* mol_1,
                                ClusterBlockId clb_index_1,
                                t_pack_molecule* mol_2,
                                ClusterBlockId clb_index_2);

bool evaluate_move_based_on_cutsize(const std::vector<molMoveDescription>& new_locs);
bool evaluate_move_based_on_connection(const std::vector<molMoveDescription>& new_locs);

int calculate_cutsize_change(const std::vector<molMoveDescription>& new_locs);
int absorbed_conn_change(const std::vector<molMoveDescription>& new_locs);

#if 0
int calculate_cutsize_of_clb(ClusterBlockId clb_index);
int update_cutsize_after_move(const std::vector<molMoveDescription>& new_locs,
                                        int original_cutsize);
#endif

#endif //VTR_PACK_MOVE_UTILS_H
