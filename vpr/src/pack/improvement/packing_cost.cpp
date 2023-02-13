#include "packing_cost.h"
#include "re_cluster_util.h"

bool evaluate_move_based_on_attraction(const std::vector<molMoveDescription>& proposed_moves) {
    float gain = 0;

    // Kepp track of all the moving atoms
    std::unordered_set<AtomBlockId> moving_atoms;
    for (auto& proposed_move : proposed_moves) {
        for (auto& atom : proposed_move.molecule_to_move->atom_block_ids) {
            if (atom)
                moving_atoms.insert(atom);
        }
    }

    for (auto& proposed_move : proposed_moves) {
        const t_pack_molecule* moving_molecule = proposed_move.molecule_to_move;
        ClusterBlockId original_clb = atom_to_cluster(proposed_move.molecule_to_move->atom_block_ids[proposed_move.molecule_to_move->root]);
        ClusterBlockId proposed_clb = proposed_move.new_clb;
        gain += calculate_molecule_attraction_to_cluster(moving_atoms, moving_molecule, proposed_clb);
        gain -= calculate_molecule_attraction_to_cluster(moving_atoms, moving_molecule, original_clb);
    }
    return (gain > 0);
}

float calculate_molecule_attraction_to_cluster(const std::unordered_set<AtomBlockId>& moving_atoms,
                                               const t_pack_molecule* molecule,
                                               ClusterBlockId clb) {
    float attraction = 0;
    for (auto& atom : molecule->atom_block_ids) {
        if (atom) {
            attraction += calculate_atom_attraction_to_cluster(moving_atoms, atom, clb);
        }
    }
    return attraction;
}

float calculate_gain_from_attractions(const t_packing_attraction& attractions,
                                      AtomBlockId atom) {
    auto& atom_ctx = g_vpr_ctx.atom();

    float alpha = 0.75;
    float beta = 0.9;

    float gain;
    int num_used_pins = atom_ctx.nlist.block_pins(atom).size();
    gain = ((1 - beta) * attractions.sharinggain + beta * attractions.connectiongain)
           / (num_used_pins);

    gain = alpha * attractions.timinggain + (1 - alpha) * gain;
    return gain;
}

float calculate_atom_attraction_to_cluster(const std::unordered_set<AtomBlockId>& moving_atoms,
                                           AtomBlockId atom,
                                           ClusterBlockId clb) {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& helper_ctx = g_vpr_ctx.mutable_cl_helper();

    t_packing_attraction attraction;

    for (auto& atom_pin : atom_ctx.nlist.block_pins(atom)) {
        auto net_id = atom_ctx.nlist.pin_net(atom_pin);

        if ((int)atom_ctx.nlist.net_pins(net_id).size() > HIGH_FANOUT_NET_THRESHOLD)
            continue;

        int num_internal_connections = 0;
        int num_stuck_connections = 0;
        std::unordered_set<AtomPinId> connected_pins;

        // calculate sharing gain
        auto pins = atom_ctx.nlist.net_pins(net_id);
        if (helper_ctx.net_output_feeds_driving_block_input[net_id] != 0)
            pins = atom_ctx.nlist.net_sinks(net_id);

        for (auto& pin : pins) {
            auto blk_id = atom_ctx.nlist.pin_block(pin);
            if (moving_atoms.count(blk_id))
                continue;

            auto cluster = atom_to_cluster(blk_id);
            if (cluster == clb) {
                attraction.sharinggain++;
                num_internal_connections++;
                connected_pins.insert(pin);
            } else {
                num_stuck_connections++;
            }
        }

        // calculate connection gain
        for (auto& connected_pin : connected_pins) {
            if (atom_ctx.nlist.pin_type(connected_pin) == PinType::DRIVER || (atom_ctx.nlist.pin_type(connected_pin) == PinType::SINK && atom_pin == atom_ctx.nlist.net_driver(net_id))) {
                if (num_internal_connections > 1)
                    attraction.connectiongain -= 1 / (float)(1.5 * num_stuck_connections + 1 + 0.1);
                attraction.connectiongain += 1 / (float)(1.5 * num_stuck_connections + 0.1);
            }
        }

        for (auto& connected_pin : connected_pins) {
            if (atom_ctx.nlist.pin_type(connected_pin) == PinType::DRIVER) {
                float timinggain = helper_ctx.timing_info->setup_pin_criticality(atom_pin);
                attraction.timinggain = std::max(timinggain, attraction.timinggain);
            } else {
                VTR_ASSERT(atom_ctx.nlist.pin_type(connected_pin) == PinType::SINK);
                if (atom_pin == atom_ctx.nlist.net_driver(net_id)) {
                    float timinggain = helper_ctx.timing_info->setup_pin_criticality(connected_pin);
                    attraction.timinggain = std::max(timinggain, attraction.timinggain);
                }
            }
        }
    }
    return calculate_gain_from_attractions(attraction, atom);
}