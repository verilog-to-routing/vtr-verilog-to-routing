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
int calculate_cutsize_change(const std::vector<molMoveDescription>& new_locs);

/* Calculate the change of the absorbed connection */
/* +ve means more connections are absorbed         */
int absorbed_conn_change(const std::vector<molMoveDescription>& new_locs);
bool evaluate_move_based_on_connection(const std::vector<molMoveDescription>& new_locs);

/* Calculate the number of abosrbed terminals of a net */
/* +ve means more terminal are now absorbed            */
float absorbed_pin_terminals(const std::vector<molMoveDescription>& new_locs);
bool evaluate_move_based_on_terminals(const std::vector<molMoveDescription>& new_locs);

/* Calculate the number of absorbed terminals of a net *
 * and add a bonus for absorbing the whole net         *
 * +ve means more terminals are now absorbed           */
float absorbed_pin_terminals_and_nets(const std::vector<molMoveDescription>& new_locs);
bool evaluate_move_based_on_terminals_and_nets(const std::vector<molMoveDescription>& new_locs);

float abosrbed_terminal_new_formula(const std::vector<molMoveDescription>& new_locs);
bool evaluate_move_based_on_terminals_new_formula(const std::vector<molMoveDescription>& new_locs);

bool evaluate_move_based_on_terminals_outside(const std::vector<molMoveDescription>& new_locs);

void init_clb_clb_conn_numbers(std::unordered_map<std::pair<ClusterBlockId, ClusterBlockId>, int, pair_hash>& conn_counts);
void print_block_connections(const std::unordered_map<std::pair<ClusterBlockId, ClusterBlockId>, int, pair_hash>& conn_count);
std::pair<std::pair<ClusterBlockId, ClusterBlockId>, int> get_max_value_pair(const std::unordered_map<std::pair<ClusterBlockId, ClusterBlockId>, int, pair_hash>& conn_count);
#if 0
int calculate_cutsize_of_clb(ClusterBlockId clb_index);
int update_cutsize_after_move(const std::vector<molMoveDescription>& new_locs,
                                        int original_cutsize);
#endif

#endif //VTR_PACK_MOVE_UTILS_H
