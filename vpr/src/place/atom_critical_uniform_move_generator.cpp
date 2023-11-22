#include "atom_critical_uniform_move_generator.h"
#include "globals.h"
#include "place_constraints.h"

static std::pair<ClusterBlockId, AtomBlockId> getCriticalAtomBlock();

e_create_move AtomCriticalUniformMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, t_propose_action& /* proposed_action */, float rlim, const t_placer_opts& /* placer_opts */, const PlacerCriticalities* /* criticalities */) {
    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    ClusterBlockId cluster_blk_id = ClusterBlockId::INVALID();
    AtomBlockId atom_blk_id = AtomBlockId::INVALID();
    std::tie(cluster_blk_id, atom_blk_id) = getCriticalAtomBlock();

    if (cluster_blk_id == ClusterBlockId::INVALID() || atom_blk_id == AtomBlockId::INVALID()) {
        return e_create_move::ABORT; // Not a valid block
    }

    t_pl_loc from = place_ctx.block_locs[cluster_blk_id].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(cluster_blk_id);
    auto grid_from_type = g_vpr_ctx.device().grid.get_physical_type({from.x, from.y, from.layer});
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    t_pl_loc to;

    if (!find_to_loc_uniform(cluster_from_type, rlim, from, to, cluster_blk_id)) {
        return e_create_move::ABORT;
    }

    e_create_move create_move = ::create_move(blocks_affected, cluster_blk_id, to);

    //Check that all of the blocks affected by the move would still be in a legal floorplan region after the swap
    if (!floorplan_legal(blocks_affected)) {
        return e_create_move::ABORT;
    }

    return create_move;
}

static std::pair<ClusterBlockId, AtomBlockId> getCriticalAtomBlock() {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& cluster_netlist = cluster_ctx.clb_nlist;
    const auto& atom_netlist = g_vpr_ctx.atom().nlist;
    const auto& atom_lookup = g_vpr_ctx.atom().lookup;
    const auto& place_move_ctx = g_placer_ctx.move();
    const auto& place_ctx = g_vpr_ctx.placement();
    /* Pick a random block to be swapped with another random block.   */
    // pick it from the highly critical blocks
    if (place_move_ctx.highly_crit_pins.empty()) {
        return std::make_pair(ClusterBlockId::INVALID(), AtomBlockId::INVALID()); //No critical block
    }
    std::pair<ClusterNetId, int> crit_cluster_net_pin = place_move_ctx.highly_crit_pins[vtr::irand(place_move_ctx.highly_crit_pins.size() - 1)];
    ClusterBlockId cluster_crit_blk = cluster_netlist.net_driver_block(crit_cluster_net_pin.first);
    if (place_ctx.block_locs[cluster_crit_blk].is_fixed) {
        return std::make_pair(ClusterBlockId::INVALID(), AtomBlockId::INVALID()); //Block is fixed, cannot move
    }

    AtomNetId atom_crit_net = atom_lookup.atom_net(crit_cluster_net_pin.first);
    AtomBlockId atom_crit_blk = atom_netlist.net_driver_block(atom_crit_net);

    return std::make_pair(cluster_crit_blk, atom_crit_blk);
}
