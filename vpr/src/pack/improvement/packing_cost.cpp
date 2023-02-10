#include "packing_cost.h"
#include "re_cluster_util.h"

bool evaluate_move_based_on_attraction(const std::vector<molMoveDescription>& proposed_moves) {

    float gain = 0;

    // Kepp track of all the moving atoms
    std::unordered_set<AtomBlockId> moving_atoms;
    for(auto& proposed_move : proposed_moves) {
        for(auto& atom : proposed_move.molecule_to_move->atom_block_ids) {
            if(atom)
                moving_atoms.insert(atom);
        }
    }

    for(auto& proposed_move : proposed_moves) {
        const t_pack_molecule* moving_molecule = proposed_move.molecule_to_move;
        ClusterBlockId original_clb = atom_to_cluster(proposed_move.molecule_to_move->atom_block_ids[proposed_move.molecule_to_move->root]);
        ClusterBlockId proposed_clb = proposed_move.new_clb;
        gain += calculate_molecule_attraction_to_cluster(moving_molecule, proposed_clb);
        gain -= calculate_molecule_attraction_to_cluster(moving_molecule, original_clb);
    }
    return (gain > 0);
}

float calculate_molecule_attraction_to_cluster(const t_pack_molecule* molecule,
                                                              ClusterBlockId clb) {
    float attraction = 0;
    for (auto& atom : molecule->atom_block_ids) {
        if(atom) {
            attraction += calculate_atom_attraction_to_cluster(atom, clb);
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

float calculate_atom_attraction_to_cluster(AtomBlockId atom,
                                           ClusterBlockId clb) {
    t_packing_attraction attraction;

    return calculate_gain_from_attractions(attraction, atom);
}