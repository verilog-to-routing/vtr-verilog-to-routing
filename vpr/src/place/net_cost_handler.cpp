#include "net_cost_handler.h"


static bool driven_by_moved_block(const AtomNetId net,
                                  const std::vector<t_pl_moved_atom_block>& moved_blocks);

static bool driven_by_moved_block(const ClusterNetId net,
                                  const std::vector<t_pl_moved_block>& moved_blocks);



//Returns true if 'net' is driven by one of the blocks in 'blocks_affected'
static bool driven_by_moved_block(const AtomNetId net,
                                  const std::vector<t_pl_moved_atom_block>& moved_blocks) {
    const auto& atom_nlist = g_vpr_ctx.atom().nlist;
    bool is_driven_by_move_blk;
    AtomBlockId net_driver_block = atom_nlist.net_driver_block(
        net);

    is_driven_by_move_blk = std::any_of(moved_blocks.begin(), moved_blocks.end(), [&net_driver_block](const auto& move_blk) {
        return net_driver_block == move_blk.block_num;
    });

    return is_driven_by_move_blk;
}

//Returns true if 'net' is driven by one of the blocks in 'blocks_affected'
static bool driven_by_moved_block(const ClusterNetId net,
                                  const std::vector<t_pl_moved_block>& moved_blocks) {
    auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    bool is_driven_by_move_blk;
    ClusterBlockId net_driver_block = clb_nlist.net_driver_block(
        net);

    is_driven_by_move_blk = std::any_of(moved_blocks.begin(), moved_blocks.end(), [&net_driver_block](const auto& move_blk) {
        return net_driver_block == move_blk.block_num;
    });

    return is_driven_by_move_blk;
}

static int find_affected_nets_and_update_costs(
    const t_place_algorithm& place_algorithm,
    const PlaceDelayModel* delay_model,
    const PlacerCriticalities* criticalities,
    t_pl_atom_blocks_to_be_moved& blocks_affected,
    double& bb_delta_c,
    double& timing_delta_c) {

    const auto& atom_look_up = g_vpr_ctx.atom().lookup;
    const auto& atom_nlist = g_vpr_ctx.atom().nlist;
    const auto& clb_nlsit = g_vpr_ctx.clustering().clb_nlist;

    VTR_ASSERT_SAFE(bb_delta_c == 0.);
    VTR_ASSERT_SAFE(timing_delta_c == 0.);

    int num_affected_nets = 0;

    std::vector<ClusterPinId> affected_pins;

    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; iblk++) {
        AtomBlockId atom_blk_id = blocks_affected.moved_blocks[iblk].block_num;
        ClusterBlockId cluster_blk_id = atom_look_up.atom_clb(atom_blk_id);
        const auto& atom_old_loc = blocks_affected.moved_blocks[iblk].old_loc;
        const auto& atom_new_loc = blocks_affected.moved_blocks[iblk].new_loc;

        for (const AtomPinId& atom_pin: atom_nlist.block_pins(atom_blk_id)) {
            auto cluster_pins = cluster_pins_connected_to_atom_pin(atom_pin);
            for (const auto& cluster_pin : cluster_pins) {
                bool is_src_moving = false;
                if (atom_nlist.pin_type(atom_pin) == PinType::SINK) {
                    AtomNetId net_id = atom_nlist.pin_net(atom_pin);
                    is_src_moving = driven_by_moved_block(net_id, blocks_affected.moved_blocks);
                }
                t_pl_moved_block move_cluster_inf;
                move_cluster_inf.block_num = cluster_blk_id;
                move_cluster_inf.old_loc = t_pl_loc(atom_old_loc.x, atom_old_loc.y, atom_old_loc.sub_tile, atom_old_loc.layer);
                move_cluster_inf.new_loc = t_pl_loc(atom_new_loc.x, atom_new_loc.y, atom_new_loc.sub_tile, atom_new_loc.layer);
                update_net_info_on_pin_move(place_algorithm,
                                            delay_model,
                                            criticalities,
                                            cluster_blk_id,
                                            cluster_pin,
                                            move_cluster_inf,
                                            affected_pins,
                                            timing_delta_c,
                                            num_affected_nets,
                                            is_src_moving);


            }
        }
    }

    /* Now update the bounding box costs (since the net bounding     *
     * boxes are up-to-date). The cost is only updated once per net. */
    for (int inet_affected = 0; inet_affected < num_affected_nets;
         inet_affected++) {
        ClusterNetId net_id = ts_nets_to_update[inet_affected];

        proposed_net_cost[net_id] = get_net_cost(net_id,
                                                 &ts_bb_coord_new[net_id]);
        bb_delta_c += proposed_net_cost[net_id] - net_cost[net_id];
    }

    return num_affected_nets;
}

/**
 * @brief Find all the nets and pins affected by this swap and update costs.
 *
 * Find all the nets affected by this swap and update the bounding box (wiring)
 * costs. This cost function doesn't depend on the timing info.
 *
 * Find all the connections affected by this swap and update the timing cost.
 * For a connection to be affected, it not only needs to be on or driven by
 * a block, but it also needs to have its delay changed. Otherwise, it will
 * not be added to the affected_pins structure.
 *
 * For more, see update_td_delta_costs().
 *
 * The timing costs are calculated by getting the new connection delays,
 * multiplied by the connection criticalities returned by the timing
 * analyzer. These timing costs are stored in the proposed_* data structures.
 *
 * The change in the bounding box cost is stored in `bb_delta_c`.
 * The change in the timing cost is stored in `timing_delta_c`.
 *
 * @return The number of affected nets.
 */
static int find_affected_nets_and_update_costs(
    const t_place_algorithm& place_algorithm,
    const PlaceDelayModel* delay_model,
    const PlacerCriticalities* criticalities,
    t_pl_blocks_to_be_moved& blocks_affected,
    double& bb_delta_c,
    double& timing_delta_c) {
    VTR_ASSERT_SAFE(bb_delta_c == 0.);
    VTR_ASSERT_SAFE(timing_delta_c == 0.);
    auto& clb_nlsit = g_vpr_ctx.clustering().clb_nlist;

    int num_affected_nets = 0;

    /* Go through all the blocks moved. */
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; iblk++) {
        const auto& moving_block_inf = blocks_affected.moved_blocks[iblk];
        auto& affected_pins = blocks_affected.affected_pins;
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        /* Go through all the pins in the moved block. */
        for (ClusterPinId blk_pin : clb_nlsit.block_pins(blk)) {
            bool is_src_moving = false;
            if (clb_nlsit.pin_type(blk_pin) == PinType::SINK) {
                ClusterNetId net_id = clb_nlsit.pin_net(blk_pin);
                is_src_moving = driven_by_moved_block(net_id, blocks_affected.moved_blocks);
            }
            update_net_info_on_pin_move(place_algorithm,
                                        delay_model,
                                        criticalities,
                                        blk,
                                        blk_pin,
                                        moving_block_inf,
                                        affected_pins,
                                        timing_delta_c,
                                        num_affected_nets,
                                        is_src_moving);
        }
    }

    /* Now update the bounding box costs (since the net bounding     *
     * boxes are up-to-date). The cost is only updated once per net. */
    for (int inet_affected = 0; inet_affected < num_affected_nets;
         inet_affected++) {
        ClusterNetId net_id = ts_nets_to_update[inet_affected];

        proposed_net_cost[net_id] = get_net_cost(net_id,
                                                 &ts_bb_coord_new[net_id]);
        bb_delta_c += proposed_net_cost[net_id] - net_cost[net_id];
    }

    return num_affected_nets;
}
