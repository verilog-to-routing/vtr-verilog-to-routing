//
// Created by elgammal on 2022-09-13.
//
#include "pack_move_utils.h"
#include "globals.h"
#include "cluster_placement.h"
#include "re_cluster_util.h"
#include <string>

static void calculate_connected_clbs_to_moving_mol(const t_pack_molecule* mol_1, std::vector<ClusterBlockId>& connected_blocks);
#if 0
static void check_net_absorption(const AtomNetId& atom_net_id,
                          const ClusterBlockId & new_clb,
                          std::map<AtomNetId, int> direct_connections,
                          bool& previously_absorbed,
                          bool& newly_absorbed);

static void update_cutsize_for_net(int& new_cutsize,
                           bool previously_absorbed,
                           bool newly_absorbed);
#endif

#if 0
int calculate_cutsize_of_clb(ClusterBlockId clb_index) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    //Define the initial conditions
    int num_unabsorbed = 0;

    //list the atoms inside the current cluster
    for (auto& pin_id : cluster_ctx.clb_nlist.block_pins(clb_index)) {
        if (cluster_ctx.clb_nlist.pin_net(pin_id) != ClusterNetId::INVALID()) {
            ++num_unabsorbed;
        }
    }
    return num_unabsorbed;
}
#endif

int calculate_cutsize_change(const std::vector<molMoveDescription>& new_locs) {
    auto& atom_ctx = g_vpr_ctx.atom();

    std::unordered_set<AtomBlockId> moving_atoms;
    std::unordered_set<AtomNetId> moving_nets;
    int cutsize_change = 0;

    auto clb_1 = new_locs[0].new_clb;
    auto clb_2 = new_locs[1].new_clb;

    for (auto& new_loc : new_locs) {
        for (auto& atom : new_loc.molecule_to_move->atom_block_ids) {
            if (atom) {
                moving_atoms.insert(atom);
                for (auto& atom_pin : atom_ctx.nlist.block_pins(atom)) {
                    auto atom_net = atom_ctx.nlist.pin_net(atom_pin);
                    if (atom_net && atom_ctx.nlist.net_pins(atom_net).size() < LARGE_FANOUT_LIMIT)
                        moving_nets.insert(atom_net);
                }
            }
        }
    }

    for (auto& net_id : moving_nets) {
        bool net_has_pin_outside = false;
        std::unordered_set<ClusterBlockId> clbs_before;
        std::unordered_set<ClusterBlockId> clbs_after;

        for (auto& pin_id : atom_ctx.nlist.net_pins(net_id)) {
            if (net_has_pin_outside)
                break;

            auto atom_blk_id = atom_ctx.nlist.pin_block(pin_id);
            auto clb = atom_to_cluster(atom_blk_id);
            if (moving_atoms.count(atom_blk_id) == 0) { // this atom is NOT one of the moving blocks
                clbs_before.insert(clb);
                clbs_after.insert(clb);
            } else { // this atom is one of the moving blocks
                clbs_before.insert(clb);
                if (clb == clb_1)
                    clbs_after.insert(clb_2);
                else
                    clbs_after.insert(clb_1);
            }
        }
        if (clbs_before.size() == 1 && clbs_after.size() > 1)
            cutsize_change++;
        else if (clbs_before.size() > 1 && clbs_after.size() == 1)
            cutsize_change--;
    }
    return cutsize_change;
}
int absorbed_conn_change(const std::vector<molMoveDescription>& new_locs) {
    auto& atom_ctx = g_vpr_ctx.atom();

    // initialize the old and new cut sizes
    int newly_absorbed_conn = 0;

    // define some temporary
    AtomBlockId cur_atom;
    ClusterBlockId cur_clb;

    std::unordered_set<AtomBlockId> moving_atoms;
    for (auto& new_loc : new_locs) {
        for (auto& atom : new_loc.molecule_to_move->atom_block_ids) {
            if (atom)
                moving_atoms.insert(atom);
        }
    }

    for (auto& new_loc : new_locs) {
        ClusterBlockId new_block_id = new_loc.new_clb;
        ClusterBlockId old_block_id = atom_to_cluster(new_loc.molecule_to_move->atom_block_ids[new_loc.molecule_to_move->root]);

        for (auto& moving_atom : new_loc.molecule_to_move->atom_block_ids) {
            if (!moving_atom)
                continue;

            for (auto& atom_pin : atom_ctx.nlist.block_pins(moving_atom)) {
                AtomNetId atom_net = atom_ctx.nlist.pin_net(atom_pin);
                if (atom_ctx.nlist.net_pins(atom_net).size() > LARGE_FANOUT_LIMIT)
                    continue;

                for (auto& net_pin : atom_ctx.nlist.net_pins(atom_net)) {
                    cur_atom = atom_ctx.nlist.pin_block(net_pin);
                    if (moving_atoms.count(cur_atom))
                        continue;

                    cur_clb = atom_to_cluster(cur_atom);
                    if (cur_clb == new_block_id) {
                        newly_absorbed_conn++;
                    } else if (cur_clb == old_block_id) {
                        newly_absorbed_conn--;
                    }
                }
            }
        }
    }

    return newly_absorbed_conn;
}

float absorbed_pin_terminals(const std::vector<molMoveDescription>& new_locs) {
    auto& atom_ctx = g_vpr_ctx.atom();

    // initialize the old and new cut sizes
    float absorbed_conn_change = 0;

    // define some temporary
    AtomBlockId cur_atom;
    ClusterBlockId cur_clb;
    std::unordered_set<AtomBlockId> moving_atoms;
    std::unordered_set<AtomNetId> moving_nets;
    for (auto& new_loc : new_locs) {
        for (auto& atom : new_loc.molecule_to_move->atom_block_ids) {
            if (atom) {
                moving_atoms.insert(atom);
                for (auto& atom_pin : atom_ctx.nlist.block_pins(atom)) {
                    moving_nets.insert(atom_ctx.nlist.pin_net(atom_pin));
                }
            }
        }
    }

    // iterate over the molecules that will be moving
    for (auto& new_loc : new_locs) {
        ClusterBlockId new_block_id = new_loc.new_clb;
        ClusterBlockId old_block_id = atom_to_cluster(new_loc.molecule_to_move->atom_block_ids[new_loc.molecule_to_move->root]);

        // iterate over atoms of the moving molecule
        for (auto& moving_atom : new_loc.molecule_to_move->atom_block_ids) {
            if (!moving_atom)
                continue;

            // iterate over the atom pins
            for (auto& atom_pin : atom_ctx.nlist.block_pins(moving_atom)) {
                AtomNetId atom_net = atom_ctx.nlist.pin_net(atom_pin);
                if (!moving_nets.count(atom_net))
                    continue;

                moving_nets.erase(atom_net);
                if (atom_ctx.nlist.net_pins(atom_net).size() > LARGE_FANOUT_LIMIT)
                    continue;

                int num_pins_in_new = 0;
                // iterate over the net pins
                for (auto& net_pin : atom_ctx.nlist.net_pins(atom_net)) {
                    cur_atom = atom_ctx.nlist.pin_block(net_pin);
                    if (!moving_atoms.count(cur_atom)) {
                        cur_clb = atom_to_cluster(cur_atom);
                        if (cur_clb == new_block_id) {
                            num_pins_in_new++;
                        } else if (cur_clb == old_block_id) {
                            num_pins_in_new--;
                        }
                    }
                }
                absorbed_conn_change += (float)(num_pins_in_new) / (float)atom_ctx.nlist.net_pins(atom_net).size();
            }
        }
    }

    return absorbed_conn_change;
}

bool evaluate_move_based_on_terminals(const std::vector<molMoveDescription>& new_locs) {
    return absorbed_pin_terminals(new_locs) > 0;
}

float absorbed_pin_terminals_and_nets(const std::vector<molMoveDescription>& new_locs) {
    auto& atom_ctx = g_vpr_ctx.atom();

    // initialize the old and new cut sizes
    float absorbed_conn_change = 0;

    // define some temporary
    AtomBlockId cur_atom;
    ClusterBlockId cur_clb;
    std::unordered_set<AtomBlockId> moving_atoms;
    for (auto& new_loc : new_locs) {
        for (auto& atom : new_loc.molecule_to_move->atom_block_ids) {
            if (atom)
                moving_atoms.insert(atom);
        }
    }

    // iterate over the molecules that will be moving
    for (auto& new_loc : new_locs) {
        ClusterBlockId new_block_id = new_loc.new_clb;
        ClusterBlockId old_block_id = atom_to_cluster(new_loc.molecule_to_move->atom_block_ids[new_loc.molecule_to_move->root]);

        // iterate over atoms of the moving molecule
        for (auto& moving_atom : new_loc.molecule_to_move->atom_block_ids) {
            if (!moving_atom)
                continue;

            // iterate over the atom pins
            for (auto& atom_pin : atom_ctx.nlist.block_pins(moving_atom)) {
                AtomNetId atom_net = atom_ctx.nlist.pin_net(atom_pin);
                if (atom_ctx.nlist.net_pins(atom_net).size() > LARGE_FANOUT_LIMIT)
                    continue;

                int num_old_absorbed = 0;
                int num_new_absorbed = 0;
                // iterate over the net pins
                for (auto& net_pin : atom_ctx.nlist.net_pins(atom_net)) {
                    cur_atom = atom_ctx.nlist.pin_block(net_pin);
                    if (cur_atom == moving_atom) {
                        num_old_absorbed++;
                        num_new_absorbed++;
                    } else if (moving_atoms.count(cur_atom)) {
                        cur_clb = atom_to_cluster(cur_atom);
                        if (cur_clb == old_block_id) {
                            num_old_absorbed++;
                            num_new_absorbed++;
                        }
                    } else {
                        cur_clb = atom_to_cluster(cur_atom);
                        if (cur_clb == new_block_id) {
                            num_new_absorbed++;
                        } else if (cur_clb == old_block_id) {
                            num_old_absorbed++;
                        }
                    }
                }
                absorbed_conn_change += (float)(num_new_absorbed - num_old_absorbed) / (float)atom_ctx.nlist.net_pins(atom_net).size() + (int)(num_new_absorbed) / (int)atom_ctx.nlist.net_pins(atom_net).size() - (int)num_old_absorbed / (int)atom_ctx.nlist.net_pins(atom_net).size();
            }
        }
    }

    return absorbed_conn_change;
}

bool evaluate_move_based_on_terminals_and_nets(const std::vector<molMoveDescription>& new_locs) {
    return absorbed_pin_terminals_and_nets(new_locs) > 0;
}

float abosrbed_terminal_new_formula(const std::vector<molMoveDescription>& new_locs) {
    auto& atom_ctx = g_vpr_ctx.atom();

    // initialize the old and new cut sizes
    float absorbed_conn_change = 0;

    // define some temporary
    AtomBlockId cur_atom;
    ClusterBlockId cur_clb;
    std::unordered_set<AtomBlockId> moving_atoms;
    for (auto& new_loc : new_locs) {
        for (auto& atom : new_loc.molecule_to_move->atom_block_ids) {
            if (atom)
                moving_atoms.insert(atom);
        }
    }

    // iterate over the molecules that will be moving
    for (auto& new_loc : new_locs) {
        ClusterBlockId new_block_id = new_loc.new_clb;
        ClusterBlockId old_block_id = atom_to_cluster(new_loc.molecule_to_move->atom_block_ids[new_loc.molecule_to_move->root]);

        // iterate over atoms of the moving molecule
        for (auto& moving_atom : new_loc.molecule_to_move->atom_block_ids) {
            if (!moving_atom)
                continue;

            // iterate over the atom pins
            for (auto& atom_pin : atom_ctx.nlist.block_pins(moving_atom)) {
                AtomNetId atom_net = atom_ctx.nlist.pin_net(atom_pin);
                if (atom_ctx.nlist.net_pins(atom_net).size() > LARGE_FANOUT_LIMIT)
                    continue;

                int old_pin_outside = 0;
                int new_pin_outside = 0;
                // iterate over the net pins
                for (auto& net_pin : atom_ctx.nlist.net_pins(atom_net)) {
                    cur_atom = atom_ctx.nlist.pin_block(net_pin);
                    if (cur_atom == moving_atom) {
                        //old_pin_outside++;
                    } else if (moving_atoms.count(cur_atom)) {
                        cur_clb = atom_to_cluster(cur_atom);
                        if (cur_clb == new_block_id) {
                            old_pin_outside++;
                            new_pin_outside++;
                        }
                    } else {
                        cur_clb = atom_to_cluster(cur_atom);
                        if (cur_clb != new_block_id) {
                            new_pin_outside++;
                        }
                        if (cur_clb != old_block_id) {
                            old_pin_outside++;
                        }
                    }
                }
                float terminals = (float)atom_ctx.nlist.net_pins(atom_net).size();
                absorbed_conn_change += (float)old_pin_outside / (terminals - old_pin_outside + 1.) - (float)new_pin_outside / (terminals - new_pin_outside + 1.);
            }
        }
    }

    return absorbed_conn_change;
}

bool evaluate_move_based_on_terminals_new_formula(const std::vector<molMoveDescription>& new_locs) {
    return abosrbed_terminal_new_formula(new_locs) > 0;
}
#if 0
int update_cutsize_after_move(const std::vector<molMoveDescription>& new_locs,
                                        int original_cutsize) {
    auto& atom_ctx = g_vpr_ctx.atom();
    int new_cutsize = original_cutsize;
    std::map<AtomNetId, int> direct_connections;

    //iterate over the molecules that are moving
    for(auto new_loc : new_locs) {
        //iterate over the atom of a molecule
        for (int i_atom = 0; i_atom < new_loc.molecule_size; i_atom++) {
            if (new_loc.molecule_to_move->atom_block_ids[i_atom]) {
                //iterate over the moving atom pins
                for (auto& pin_id : atom_ctx.nlist.block_pins(new_loc.molecule_to_move->atom_block_ids[i_atom])) {
                    AtomNetId atom_net_id = atom_ctx.nlist.pin_net(pin_id);

                    //if this pin is connected to a net
                    if (atom_net_id) {
                        ClusterPinId cluster_pin;
                        bool previously_absorbed, newly_absorbed;

                        //check the status of this net (absorbed or not) before and after the proposed move
                        check_net_absorption(atom_net_id,
                                             new_loc.new_clb,
                                             direct_connections,
                                             previously_absorbed,
                                             newly_absorbed);

                        //update the cutsize based on the absorption of a net before and after the move
                        update_cutsize_for_net(new_cutsize,
                                               previously_absorbed,
                                               newly_absorbed);
                    }
                }
            }
        }
    }

    /* consider the case of swapping two atoms that are directly connected
     *
     * In this case, the algorithm will minimize the cutsize by one when iterating over the first atom pins and minimize it again
     * when iterating over the 2nd atom pins. However, the cutsize should remain the same. Hence,
     * We are increasing the cutsize by 2 for this specific case
     */
    for(auto& direct_conn: direct_connections) {
        if(direct_conn.second > 1) {
            new_cutsize += 2;
        }
    }
    return new_cutsize;
}
#endif

t_pack_molecule* pick_molecule_randomly() {
    auto& atom_ctx = g_vpr_ctx.atom();
#ifdef PACK_MULTITHREADED
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();
#endif
    bool molecule_picked = false;
    t_pack_molecule* molecule;

    while (!molecule_picked) {
        int rand_num = vtr::irand((int)atom_ctx.nlist.blocks().size() - 1);
        AtomBlockId random_atom = AtomBlockId(rand_num);
        ClusterBlockId clb_index = atom_to_cluster(random_atom);
        if (!clb_index)
            continue;
#ifdef PACK_MULTITHREADED
        if (packing_multithreading_ctx.mu[clb_index]->try_lock()) {
#endif
            auto rng = atom_ctx.atom_molecules.equal_range(random_atom);
            for (const auto& kv : vtr::make_range(rng.first, rng.second)) {
                molecule = kv.second;
                molecule_picked = true;
                break;
            }
#ifdef PACK_MULTITHREADED
        } else {
            continue; //CLB is already in-flight
        }
#endif
    }
    return molecule;
}

bool pick_molecule_connected(t_pack_molecule* mol_1, t_pack_molecule*& mol_2) {
    auto& atom_ctx = g_vpr_ctx.atom();
#ifdef PACK_MULTITHREADED
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();
#endif
    std::vector<ClusterBlockId> connected_blocks;
    calculate_connected_clbs_to_moving_mol(mol_1, connected_blocks);
    if (connected_blocks.empty())
        return false;

    // pick a random clb block from the connected blocks
    bool clb2_not_found = true;
    ClusterBlockId clb_index_2;
    int iteration = 0;
    while (clb2_not_found && iteration < 20) {
        int rand_num = vtr::irand((int)connected_blocks.size() - 1);
        clb_index_2 = connected_blocks[rand_num];
#ifdef PACK_MULTITHREADED
        if (packing_multithreading_ctx.mu[clb_index_2]->try_lock()) {
#endif
            clb2_not_found = false;
#ifdef PACK_MULTITHREADED
        }
#endif
        iteration++;
    }

    if (clb2_not_found)
        return false;

    //pick a random molecule for the chosen block
    std::unordered_set<AtomBlockId>* atom_ids = cluster_to_atoms(clb_index_2);

    int rand_num = vtr::irand((int)atom_ids->size() - 1);
    auto it = atom_ids->begin();
    std::advance(it, rand_num);
    AtomBlockId atom_id = *it;
    auto rng = atom_ctx.atom_molecules.equal_range(atom_id);
    for (const auto& kv : vtr::make_range(rng.first, rng.second)) {
        mol_2 = kv.second;
        return true;
    }
#ifdef PACK_MULTITHREADED
    packing_multithreading_ctx.mu[clb_index_2]->unlock();
#endif
    return false;
}

bool pick_molecule_connected_compatible_type(t_pack_molecule* mol_1, t_pack_molecule*& mol_2) {
    auto& atom_ctx = g_vpr_ctx.atom();
#ifdef PACK_MULTITHREADED
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();
#endif

    std::vector<ClusterBlockId> connected_blocks;
    calculate_connected_clbs_to_moving_mol(mol_1, connected_blocks);
    if (connected_blocks.empty())
        return false;

    // pick a random clb block from the connected blocks
    bool clb2_not_found = true;
    ClusterBlockId clb_index_2;
    int iteration = 0;
    while (clb2_not_found && iteration < 10) {
        clb_index_2 = connected_blocks[vtr::irand((int)connected_blocks.size() - 1)];
#ifdef PACK_MULTITHREADED
        if (packing_multithreading_ctx.mu[clb_index_2]->try_lock()) {
#endif
            clb2_not_found = false;
#ifdef PACK_MULTITHREADED
        }
#endif
        iteration++;
    }

    if (clb2_not_found)
        return false;

    //pick a random molecule for the chosen block
    std::unordered_set<AtomBlockId>* atom_ids = cluster_to_atoms(clb_index_2);
    iteration = 0;
    const t_pb* pb_1 = atom_ctx.lookup.atom_pb(mol_1->atom_block_ids[mol_1->root]);
    do {
        int rand_num = vtr::irand((int)atom_ids->size() - 1);
        auto it = atom_ids->begin();
        std::advance(it, rand_num);
        AtomBlockId atom_id = *it;
        auto rng = atom_ctx.atom_molecules.equal_range(atom_id);
        for (const auto& kv : vtr::make_range(rng.first, rng.second)) {
            mol_2 = kv.second;
            const t_pb* pb_2 = atom_ctx.lookup.atom_pb(mol_2->atom_block_ids[mol_2->root]);
            if (strcmp(pb_1->pb_graph_node->pb_type->name, pb_2->pb_graph_node->pb_type->name) == 0)
                return true;
            else
                iteration++;
        }
    } while (iteration < 20);
#ifdef PACK_MULTITHREADED
    packing_multithreading_ctx.mu[clb_index_2]->unlock();
#endif
    return false;
}

bool pick_molecule_connected_same_type(t_pack_molecule* mol_1, t_pack_molecule*& mol_2) {
    auto& atom_ctx = g_vpr_ctx.atom();
#ifdef PACK_MULTITHREADED
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();
#endif

    std::vector<ClusterBlockId> connected_blocks;
    calculate_connected_clbs_to_moving_mol(mol_1, connected_blocks);
    if (connected_blocks.empty())
        return false;

    // pick a random clb block from the connected blocks
    bool clb2_not_found = true;
    ClusterBlockId clb_index_2;
    int iteration = 0;
    while (clb2_not_found && iteration < 10) {
        clb_index_2 = connected_blocks[vtr::irand((int)connected_blocks.size() - 1)];
#ifdef PACK_MULTITHREADED
        if (packing_multithreading_ctx.mu[clb_index_2]->try_lock()) {
#endif
            clb2_not_found = false;
#ifdef PACK_MULTITHREADED
        }
#endif
        iteration++;
    }

    if (clb2_not_found)
        return false;

    //pick a random molecule for the chosen block
    std::unordered_set<AtomBlockId>* atom_ids = cluster_to_atoms(clb_index_2);
    iteration = 0;
    const t_pb* pb_1 = atom_ctx.lookup.atom_pb(mol_1->atom_block_ids[mol_1->root]);

    do {
        int rand_num = vtr::irand((int)atom_ids->size() - 1);
        auto it = atom_ids->begin();
        std::advance(it, rand_num);
        AtomBlockId atom_id = *it;
        auto rng = atom_ctx.atom_molecules.equal_range(atom_id);
        for (const auto& kv : vtr::make_range(rng.first, rng.second)) {
            mol_2 = kv.second;
            const t_pb* pb_2 = atom_ctx.lookup.atom_pb(mol_2->atom_block_ids[mol_2->root]);
            if (pb_1->pb_graph_node->pb_type == pb_2->pb_graph_node->pb_type)
                return true;
            else {
                iteration++;
                break;
            }
        }
    } while (iteration < 10);
#ifdef PACK_MULTITHREADED
    packing_multithreading_ctx.mu[clb_index_2]->unlock();
#endif
    return false;
}

bool pick_molecule_connected_same_size(t_pack_molecule* mol_1, t_pack_molecule*& mol_2) {
    auto& atom_ctx = g_vpr_ctx.atom();
#ifdef PACK_MULTITHREADED
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();
#endif

    std::vector<ClusterBlockId> connected_blocks;
    calculate_connected_clbs_to_moving_mol(mol_1, connected_blocks);
    if (connected_blocks.empty())
        return false;

    int mol_1_size = get_array_size_of_molecule(mol_1);

    // pick a random clb block from the connected blocks
    bool clb2_not_found = true;
    ClusterBlockId clb_index_2;
    int iteration = 0;
    while (clb2_not_found && iteration < 10) {
        clb_index_2 = connected_blocks[vtr::irand((int)connected_blocks.size() - 1)];
#ifdef PACK_MULTITHREADED
        if (packing_multithreading_ctx.mu[clb_index_2]->try_lock()) {
#endif
            clb2_not_found = false;
#ifdef PACK_MULTITHREADED
        }
#endif
        ++iteration;
    }

    if (clb2_not_found)
        return false;

    //pick a random molecule for the chosen block
    std::unordered_set<AtomBlockId>* atom_ids = cluster_to_atoms(clb_index_2);
    iteration = 0;
    do {
        int rand_num = vtr::irand((int)atom_ids->size() - 1);
        auto it = atom_ids->begin();
        std::advance(it, rand_num);
        AtomBlockId atom_id = *it;
        auto rng = atom_ctx.atom_molecules.equal_range(atom_id);
        for (const auto& kv : vtr::make_range(rng.first, rng.second)) {
            mol_2 = kv.second;
            if (std::abs(mol_1_size - get_array_size_of_molecule(mol_2)) <= 1)
                return true;
            else
                iteration++;
        }
    } while (iteration < 20);
#ifdef PACK_MULTITHREADED
    packing_multithreading_ctx.mu[clb_index_2]->unlock();
#endif
    return false;
}

void build_mol_move_description(std::vector<molMoveDescription>& new_locs,
                                t_pack_molecule* mol_1,
                                ClusterBlockId clb_index_1,
                                t_pack_molecule* mol_2,
                                ClusterBlockId clb_index_2) {
    molMoveDescription temp;
    temp.molecule_to_move = mol_1;
    temp.new_clb = clb_index_2;
    temp.molecule_size = get_array_size_of_molecule(mol_1);
    new_locs.push_back(temp);

    temp.molecule_to_move = mol_2;
    temp.new_clb = clb_index_1;
    temp.molecule_size = get_array_size_of_molecule(mol_2);
    new_locs.push_back(temp);
}

bool evaluate_move_based_on_cutsize(const std::vector<molMoveDescription>& new_locs) {
    int change_in_cutsize = calculate_cutsize_change(new_locs);
    if (change_in_cutsize < 0)
        return true;
    else
        return false;
}

bool evaluate_move_based_on_connection(const std::vector<molMoveDescription>& new_locs) {
    float change_in_absorbed_conn = absorbed_conn_change(new_locs);

    return (change_in_absorbed_conn > 0);
}
/********* static functions ************/
/***************************************/
#if 0
static void check_net_absorption(const AtomNetId& atom_net_id,
                          const ClusterBlockId & new_clb,
                          std::map<AtomNetId, int> direct_connections,
                          bool& previously_absorbed,
                          bool& newly_absorbed) {
    auto& atom_ctx = g_vpr_ctx.atom();

    //check the status of the atom net before the move (absorbed or not)
    ClusterNetId clb_net_id = atom_ctx.lookup.clb_net(atom_net_id);
    if(clb_net_id == ClusterNetId::INVALID()) {
        previously_absorbed = true;
    } else {
        previously_absorbed = false;
    }

    //check the status of the atom net after the move (absorbed or not)
    newly_absorbed = true;
    AtomBlockId  atom_block_id;
    ClusterBlockId  clb_index;
    for(auto& net_pin_id : atom_ctx.nlist.net_pins(atom_net_id)) {
        atom_block_id = atom_ctx.nlist.pin_block(net_pin_id);
        clb_index = atom_ctx.lookup.atom_clb(atom_block_id);
        if(clb_index == new_clb) {
            if(direct_connections.find(atom_net_id) == direct_connections.end()) {
                direct_connections.insert(std::make_pair(atom_net_id, 1));
            } else {
                ++direct_connections[atom_net_id];
            }
        }
        if(clb_index != new_clb) {
            newly_absorbed = false;
            break;
        }
    }
}
static void update_cutsize_for_net(int& new_cutsize, bool previously_absorbed, bool newly_absorbed) {
    if(previously_absorbed && !newly_absorbed) {
        new_cutsize++;
    } else if(!previously_absorbed && newly_absorbed) {
        new_cutsize--;
    }
}
#endif

static void calculate_connected_clbs_to_moving_mol(const t_pack_molecule* mol_1, std::vector<ClusterBlockId>& connected_blocks) {
    // get the clb index of the first molecule
    ClusterBlockId clb_index_1 = atom_to_cluster(mol_1->atom_block_ids[mol_1->root]);

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();

    t_logical_block_type_ptr block_type_1 = cluster_ctx.clb_nlist.block_type(clb_index_1);
    t_logical_block_type_ptr block_type_2;

    AtomNetId cur_net;
    AtomBlockId cur_atom;
    ClusterBlockId cur_clb;

    // Calculate the connected blocks to the moving molecule
    for (auto& atom_id : mol_1->atom_block_ids) {
        if (atom_id) {
            for (auto& atom_pin : atom_ctx.nlist.block_pins(atom_id)) {
                cur_net = atom_ctx.nlist.pin_net(atom_pin);
                if (atom_ctx.nlist.net_pins(cur_net).size() > LARGE_FANOUT_LIMIT)
                    continue;
                for (auto& net_pin : atom_ctx.nlist.net_pins(cur_net)) {
                    cur_atom = atom_ctx.nlist.pin_block(net_pin);
                    cur_clb = atom_to_cluster(cur_atom);
                    block_type_2 = cluster_ctx.clb_nlist.block_type(cur_clb);
                    if (cur_clb != clb_index_1 && block_type_1 == block_type_2)
                        connected_blocks.push_back(cur_clb);
                }
            }
        }
    }
}

/************* CLB-CLB connection count hash table helper functions ***************/
void init_clb_clb_conn_numbers(std::unordered_map<std::pair<ClusterBlockId, ClusterBlockId>, int, pair_hash>& conn_counts) {
    auto& atom_ctx = g_vpr_ctx.atom();

    for (auto atom_net : atom_ctx.nlist.nets()) {
        if (atom_ctx.nlist.net_pins(atom_net).size() > 7)
            continue;

        std::unordered_set<ClusterBlockId> clusters;
        for (auto atom_pin_it = atom_ctx.nlist.net_pins(atom_net).begin(); atom_pin_it != atom_ctx.nlist.net_pins(atom_net).end(); atom_pin_it++) {
            auto clb1 = atom_to_cluster(atom_ctx.nlist.pin_block(*atom_pin_it));
            clusters.insert(clb1);
            for (auto atom_pin_it2 = atom_pin_it + 1; atom_pin_it2 != atom_ctx.nlist.net_pins(atom_net).end(); atom_pin_it2++) {
                auto clb2 = atom_to_cluster(atom_ctx.nlist.pin_block(*atom_pin_it2));
                if (clusters.count(clb2) == 0) {
                    if (conn_counts.find({clb1, clb2}) == conn_counts.end())
                        conn_counts.insert({{clb1, clb2}, 1});
                    else
                        conn_counts[{clb1, clb2}]++;

                    clusters.insert(clb2);
                }
            }
        }
    }
}

void print_block_connections(const std::unordered_map<std::pair<ClusterBlockId, ClusterBlockId>, int, pair_hash>& conn_count) {
    for (const auto& block_pair_count : conn_count) {
        VTR_LOG("Block : %d is connected to Block: %d with %d direct connections.\n",
                block_pair_count.first.first, block_pair_count.first.second, block_pair_count.second);
    }
}

std::pair<std::pair<ClusterBlockId, ClusterBlockId>, int> get_max_value_pair(const std::unordered_map<std::pair<ClusterBlockId, ClusterBlockId>, int, pair_hash>& conn_count) {
    auto max_iter = std::max_element(conn_count.begin(), conn_count.end(),
                                     [](const auto& a, auto& b) { return a.second < b.second; });
    return *max_iter;
}

bool evaluate_move_based_on_terminals_outside(const std::vector<molMoveDescription>& new_locs) {
    auto& atom_ctx = g_vpr_ctx.atom();

    int pins_in1_before, pins_in2_before, pins_in1_after, pins_in2_after, pins_outside_before, pins_outside_after;
    double cost = 0;
    std::unordered_set<AtomBlockId> moving_atoms;

    for (auto& new_loc : new_locs) {
        for (auto& moving_atom : new_loc.molecule_to_move->atom_block_ids) {
            if (moving_atom) {
                moving_atoms.insert(moving_atom);
            }
        }
    }

    // iterate over moves proposed (a swap is two moves)
    std::unordered_set<AtomNetId> moving_nets;
    for (auto& new_loc : new_locs) {
        auto cur_clb = atom_to_cluster(new_loc.molecule_to_move->atom_block_ids[new_loc.molecule_to_move->root]);
        // iterate over atoms in the moving molcule
        for (auto& moving_atom : new_loc.molecule_to_move->atom_block_ids) {
            if (moving_atom) {
                // iterate over moving atom pins
                for (auto& moving_atom_pin : atom_ctx.nlist.block_pins(moving_atom)) {
                    auto atom_net = atom_ctx.nlist.pin_net(moving_atom_pin);
                    if (atom_ctx.nlist.net_pins(atom_net).size() > LARGE_FANOUT_LIMIT)
                        continue;

                    // Make sure that we didn't count this net before
                    if (moving_nets.count(atom_net))
                        continue;

                    moving_nets.insert(atom_net);
                    pins_in1_before = 0;
                    pins_in2_before = 0;
                    pins_in1_after = 0;
                    pins_in2_after = 0;
                    pins_outside_before = 0;
                    pins_outside_after = 0;

                    for (auto& pin : atom_ctx.nlist.net_pins(atom_net)) {
                        auto atom = atom_ctx.nlist.pin_block(pin);
                        auto cluster = atom_to_cluster(atom);
                        if (moving_atoms.count(atom)) {
                            if (cluster == cur_clb) {
                                pins_in1_before++;
                                pins_in2_after++;
                            } else {
                                pins_in2_before++;
                                pins_in1_after++;
                            }
                        } else {
                            if (cluster == cur_clb) {
                                pins_in1_before++;
                                pins_in1_after++;
                            } else if (cluster == new_loc.new_clb) {
                                pins_in2_before++;
                                pins_in2_after++;
                            } else {
                                pins_outside_before++;
                                pins_outside_after++;
                            }
                        }
                    }
                    cost += (double)std::max(pins_in1_after, pins_in2_after) / (pins_outside_after + std::min(pins_in1_after, pins_in2_after) + 1.) - (double)std::max(pins_in1_before, pins_in2_before) / (pins_outside_before + std::min(pins_in1_before, pins_in2_before) + 1.);
                }
            }
        }
    }
    return (cost > 0);
}