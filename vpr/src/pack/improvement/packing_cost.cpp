#include "packing_cost.h"
#include "re_cluster_util.h"

bool evaluate_move_based_on_attraction(const std::vector<molMoveDescription>& proposed_moves) {
    auto& atom_ctx = g_vpr_ctx.atom();
    float gain = 0;

    // Keep track of all the moving atoms
    std::unordered_set<AtomBlockId> moving_atoms;

    for (auto& proposed_move : proposed_moves) {
        for (auto& atom : proposed_move.molecule_to_move->atom_block_ids) {
            if (atom) {
                moving_atoms.insert(atom);
            }
        }
    }

    for (auto& proposed_move : proposed_moves) {
        const t_pack_molecule* moving_molecule = proposed_move.molecule_to_move;
        ClusterBlockId original_clb = atom_to_cluster(moving_molecule->atom_block_ids[moving_molecule->root]);
        ClusterBlockId proposed_clb = proposed_move.new_clb;

        std::unordered_set<AtomNetId> moving_nets;
        for (auto& atom : moving_molecule->atom_block_ids) {
            if (atom) {
                for (auto& pin : atom_ctx.nlist.block_pins(atom)) {
                    auto net_id = atom_ctx.nlist.pin_net(pin);
                    if (net_id)
                        moving_nets.insert(net_id);
                }
            }
        }
        gain += calculate_molecule_attraction_to_cluster(moving_atoms, moving_nets, moving_molecule, proposed_clb);
        gain -= calculate_molecule_attraction_to_cluster(moving_atoms, moving_nets, moving_molecule, original_clb);
    }

    return (gain > 0);
}

float calculate_molecule_attraction_to_cluster(const std::unordered_set<AtomBlockId>& moving_atoms,
                                               const std::unordered_set<AtomNetId>& moving_nets,
                                               const t_pack_molecule* molecule,
                                               ClusterBlockId clb) {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& helper_ctx = g_vpr_ctx.mutable_cl_helper();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    float gain = 0;
    t_packing_attraction attraction;

    for (auto& net_id : moving_nets) {
        if ((int)atom_ctx.nlist.net_pins(net_id).size() > helper_ctx.high_fanout_thresholds.get_threshold(cluster_ctx.clb_nlist.block_type(clb)->name))
            continue;

        std::unordered_set<AtomBlockId> connected_moving_blocks;

        int num_stuck_connections = 0;
        bool net_shared = false;

        // calculate sharing gain
        auto pins = atom_ctx.nlist.net_pins(net_id);
        if (helper_ctx.net_output_feeds_driving_block_input[net_id] != 0)
            pins = atom_ctx.nlist.net_sinks(net_id);

        for (auto& pin : pins) {
            auto blk_id = atom_ctx.nlist.pin_block(pin);
            if (moving_atoms.count(blk_id)) {
                connected_moving_blocks.insert(blk_id);
                continue;
            }

            auto cluster = atom_to_cluster(blk_id);
            if (cluster == clb) {
                if (!net_shared) {
                    net_shared = true;
                    attraction.sharinggain++;
                }
                if (helper_ctx.timing_driven) {
                    if (atom_ctx.nlist.pin_type(pin) == PinType::SINK) {
                        auto net_driver_block = atom_ctx.nlist.net_driver_block(net_id);
                        if (moving_atoms.count(net_driver_block) != 0) {
                            float timinggain = helper_ctx.timing_info->setup_pin_criticality(pin);
                            attraction.timinggain = std::max(timinggain, attraction.timinggain);
                        }
                    } else if (atom_ctx.nlist.pin_type(pin) == PinType::DRIVER) {
                        for (auto& pin_id : atom_ctx.nlist.net_sinks(net_id)) {
                            auto net_sink_block = atom_ctx.nlist.pin_block(pin_id);
                            if (moving_atoms.count(net_sink_block) != 0) {
                                float timinggain = helper_ctx.timing_info->setup_pin_criticality(pin_id);
                                attraction.timinggain = std::max(timinggain, attraction.timinggain);
                            }
                        }
                    }
                }
            } else {
                num_stuck_connections++;
            }
        }
        attraction.connectiongain += 1 / (float)(0.1 + num_stuck_connections);
    }

    gain += calculate_gain_from_attractions(attraction, molecule);

    return gain;
}

float calculate_gain_from_attractions(const t_packing_attraction& attractions,
                                      const t_pack_molecule* molecule) {
    auto& atom_ctx = g_vpr_ctx.atom();

    float alpha = 0.75;
    float beta = 0.9;

    float gain;
    int num_used_pins = 0;
    for (auto& atom : molecule->atom_block_ids) {
        if (atom) {
            num_used_pins += atom_ctx.nlist.block_input_pins(atom).size();
            num_used_pins += atom_ctx.nlist.block_output_pins(atom).size();
        }
    }

    gain = ((1 - beta) * attractions.sharinggain + beta * attractions.connectiongain)
           / (num_used_pins);

    gain = alpha * attractions.timinggain + (1 - alpha) * gain;
    return gain;
}