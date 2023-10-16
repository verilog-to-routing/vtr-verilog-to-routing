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