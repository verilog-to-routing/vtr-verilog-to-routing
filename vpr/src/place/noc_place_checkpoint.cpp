
#include "noc_place_checkpoint.h"
#include "noc_place_utils.h"

NoCPlacementCheckpoint::NoCPlacementCheckpoint(NocCostHandler& noc_cost_handler)
    : noc_cost_handler_(noc_cost_handler)
    , valid_(false)
    , cost_(std::numeric_limits<double>::infinity()) {
    const auto& noc_ctx = g_vpr_ctx.noc();

    // Get all router clusters in the net-list
    const std::vector<ClusterBlockId>& router_bids = noc_ctx.noc_traffic_flows_storage.get_router_clusters_in_netlist();

    router_locations_.clear();

    // Initializes checkpoint locations to invalid
    for (const ClusterBlockId router_bid : router_bids) {
        router_locations_[router_bid] = t_pl_loc(OPEN, OPEN, OPEN, OPEN);
    }
}

void NoCPlacementCheckpoint::save_checkpoint(double cost, const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs) {
    const auto& noc_ctx = g_vpr_ctx.noc();

    const std::vector<ClusterBlockId>& router_bids = noc_ctx.noc_traffic_flows_storage.get_router_clusters_in_netlist();

    for (const ClusterBlockId router_bid : router_bids) {
        t_pl_loc loc = block_locs[router_bid].loc;
        router_locations_[router_bid] = loc;
    }
    valid_ = true;
    cost_ = cost;
}

void NoCPlacementCheckpoint::restore_checkpoint(t_placer_costs& costs,
                                                BlkLocRegistry& blk_loc_registry) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& device_ctx = g_vpr_ctx.device();
    GridBlock& grid_blocks = blk_loc_registry.mutable_grid_blocks();

    // Get all physical routers
    const auto& noc_phy_routers = noc_ctx.noc_model.get_noc_routers();

    // Clear all physical routers in placement
    for (const NocRouter& phy_router : noc_phy_routers) {
        t_physical_tile_loc phy_loc = phy_router.get_router_physical_location();

        grid_blocks.set_usage(phy_loc, 0);
        auto tile = device_ctx.grid.get_physical_type(phy_loc);

        for (const auto& sub_tile : tile->sub_tiles) {
            auto capacity = sub_tile.capacity;

            for (int k = 0; k < capacity.total(); k++) {
                const t_pl_loc loc(phy_loc, k + capacity.low);
                grid_blocks.set_block_at_location(loc, ClusterBlockId::INVALID());
            }
        }
    }

    // Place routers based on router_locations_
    for (const auto& [router_blk_id, location] : router_locations_) {
        blk_loc_registry.set_block_location(router_blk_id, location);
    }

    // Re-initialize routes and static variables that keep track of NoC-related costs
    VTR_ASSERT(noc_cost_handler_.points_to_same_block_locs(blk_loc_registry.block_locs()));
    noc_cost_handler_.reinitialize_noc_routing(costs, {});
}

bool NoCPlacementCheckpoint::is_valid() const {
    return valid_;
}

double NoCPlacementCheckpoint::get_cost() const {
    return cost_;
}
