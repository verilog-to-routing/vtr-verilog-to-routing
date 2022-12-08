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

    // initialize the old and new cut sizes
    int change_cutsize = 0;

    // define some temporary
    AtomBlockId cur_atom;
    ClusterBlockId cur_clb;
    std::set<ClusterBlockId> net_blocks;
    std::map<AtomNetId, int> nets_between_old_new_blks;

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

                net_blocks.clear();
                for (auto& net_pin : atom_ctx.nlist.net_pins(atom_net)) {
                    cur_atom = atom_ctx.nlist.pin_block(net_pin);
                    if (cur_atom == moving_atom)
                        continue;

                    cur_clb = atom_to_cluster(cur_atom);
                    net_blocks.insert(cur_clb);
                }
                if (net_blocks.size() == 1 && *(net_blocks.begin()) == old_block_id)
                    change_cutsize += 1;
                else if (net_blocks.size() == 1 && *(net_blocks.begin()) == new_block_id) {
                    change_cutsize -= 1;
                    if (nets_between_old_new_blks.find(atom_net) == nets_between_old_new_blks.end())
                        nets_between_old_new_blks.insert(std::make_pair(atom_net, 1));
                    else
                        nets_between_old_new_blks[atom_net]++;
                }
            }
        }
    }

    for (auto& direct_conn : nets_between_old_new_blks) {
        if (direct_conn.second > 1)
            change_cutsize += 2;
    }
    return change_cutsize;
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
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();

    bool molecule_picked = false;
    t_pack_molecule* molecule;

    while (!molecule_picked) {
        int rand_num = vtr::irand((int)atom_ctx.nlist.blocks().size() - 1);
        AtomBlockId random_atom = AtomBlockId(rand_num);
        ClusterBlockId clb_index = atom_to_cluster(random_atom);
        if (!clb_index)
            continue;
        if (packing_multithreading_ctx.mu[clb_index]->try_lock()) {
            auto rng = atom_ctx.atom_molecules.equal_range(random_atom);
            for (const auto& kv : vtr::make_range(rng.first, rng.second)) {
                molecule = kv.second;
                molecule_picked = true;
                break;
            }
        } else {
            continue; //CLB is already in-flight
        }
    }
    return molecule;
}

bool pick_molecule_connected(t_pack_molecule* mol_1, t_pack_molecule*& mol_2) {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();

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
        if (packing_multithreading_ctx.mu[clb_index_2]->try_lock()) {
            clb2_not_found = false;
        }
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

    packing_multithreading_ctx.mu[clb_index_2]->unlock();
    return false;
}

bool pick_molecule_connected_compatible_type(t_pack_molecule* mol_1, t_pack_molecule*& mol_2) {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();

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
        if (packing_multithreading_ctx.mu[clb_index_2]->try_lock()) {
            clb2_not_found = false;
        }
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

    packing_multithreading_ctx.mu[clb_index_2]->unlock();
    return false;
}

bool pick_molecule_connected_same_type(t_pack_molecule* mol_1, t_pack_molecule*& mol_2) {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();

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
        if (packing_multithreading_ctx.mu[clb_index_2]->try_lock()) {
            clb2_not_found = false;
        }
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
            else
                iteration++;
        }
    } while (iteration < 20);

    packing_multithreading_ctx.mu[clb_index_2]->unlock();
    return false;
}

bool pick_molecule_connected_same_size(t_pack_molecule* mol_1, t_pack_molecule*& mol_2) {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();

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
        if (packing_multithreading_ctx.mu[clb_index_2]->try_lock()) {
            clb2_not_found = false;
        }
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

    packing_multithreading_ctx.mu[clb_index_2]->unlock();
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