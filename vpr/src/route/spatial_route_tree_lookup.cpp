#include "spatial_route_tree_lookup.h"

#include "globals.h"

SpatialRouteTreeLookup build_route_tree_spatial_lookup(ClusterNetId net, t_rt_node* rt_root) {
    constexpr float BIN_AREA_PER_SINK_FACTOR = 4;

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    int fanout = cluster_ctx.clb_nlist.net_sinks(net).size();

    t_bb bb = route_ctx.route_bb[net];
    float bb_area = (bb.xmax - bb.xmin) * (bb.ymax - bb.ymin);
    float bb_area_per_sink = bb_area / fanout;
    float bin_area = BIN_AREA_PER_SINK_FACTOR * bb_area_per_sink;

    float bin_dim = std::ceil(std::sqrt(bin_area));

    size_t bins_x = std::ceil(device_ctx.grid.width() / bin_dim);
    size_t bins_y = std::ceil(device_ctx.grid.height() / bin_dim);

    SpatialRouteTreeLookup spatial_lookup({bins_x, bins_y});

    update_route_tree_spatial_lookup_recur(rt_root, spatial_lookup);

    return spatial_lookup;
}

//Adds the sub-tree rooted at rt_node to the spatial look-up
void update_route_tree_spatial_lookup_recur(t_rt_node* rt_node, SpatialRouteTreeLookup& spatial_lookup) {
    auto& device_ctx = g_vpr_ctx.device();

    auto& rr_node = device_ctx.rr_nodes[rt_node->inode];

    int bin_xlow = grid_to_bin_x(rr_node.xlow(), spatial_lookup);
    int bin_ylow = grid_to_bin_y(rr_node.ylow(), spatial_lookup);
    int bin_xhigh = grid_to_bin_x(rr_node.xhigh(), spatial_lookup);
    int bin_yhigh = grid_to_bin_y(rr_node.yhigh(), spatial_lookup);

    spatial_lookup[bin_xlow][bin_ylow].push_back(rt_node);

    //We current look at the start/end locations of the RR nodes and add the node
    //to both bins if they are different
    //
    //TODO: Depending on bin size, long wires may end up being added only to bins at
    //      their start/end and may pass through bins along their length to which they
    //      are not added. If this becomes an issues, reconsider how we add nodes to
    //      bins
    if (bin_xhigh != bin_xlow || bin_yhigh != bin_ylow) {
        spatial_lookup[bin_xhigh][bin_yhigh].push_back(rt_node);
    }

    //Recurse
    for (t_linked_rt_edge* rt_edge = rt_node->u.child_list; rt_edge != nullptr; rt_edge = rt_edge->next) {
        update_route_tree_spatial_lookup_recur(rt_edge->child, spatial_lookup);
    }
}

size_t grid_to_bin_x(size_t grid_x, const SpatialRouteTreeLookup& spatial_lookup) {
    auto& device_ctx = g_vpr_ctx.device();

    int bin_width = std::ceil(float(device_ctx.grid.width()) / spatial_lookup.dim_size(0));

    return grid_x / bin_width;
}

size_t grid_to_bin_y(size_t grid_y, const SpatialRouteTreeLookup& spatial_lookup) {
    auto& device_ctx = g_vpr_ctx.device();

    int bin_height = std::ceil(float(device_ctx.grid.height()) / spatial_lookup.dim_size(1));

    return grid_y / bin_height;
}

bool validate_route_tree_spatial_lookup(t_rt_node* rt_node, const SpatialRouteTreeLookup& spatial_lookup) {
    auto& device_ctx = g_vpr_ctx.device();

    auto& rr_node = device_ctx.rr_nodes[rt_node->inode];

    int bin_xlow = grid_to_bin_x(rr_node.xlow(), spatial_lookup);
    int bin_ylow = grid_to_bin_y(rr_node.ylow(), spatial_lookup);
    int bin_xhigh = grid_to_bin_x(rr_node.xhigh(), spatial_lookup);
    int bin_yhigh = grid_to_bin_y(rr_node.yhigh(), spatial_lookup);

    bool valid = true;

    auto& low_bin_rt_nodes = spatial_lookup[bin_xlow][bin_ylow];
    if (std::find(low_bin_rt_nodes.begin(), low_bin_rt_nodes.end(), rt_node) == low_bin_rt_nodes.end()) {
        valid = false;
        VPR_THROW(VPR_ERROR_ROUTE, "Failed to find route tree node %d at (low coord %d,%d) in spatial lookup [bin %d,%d]",
                  rt_node->inode, rr_node.xlow(), rr_node.ylow(), bin_xlow, bin_ylow);
    }

    auto& high_bin_rt_nodes = spatial_lookup[bin_xhigh][bin_yhigh];
    if (std::find(high_bin_rt_nodes.begin(), high_bin_rt_nodes.end(), rt_node) == high_bin_rt_nodes.end()) {
        valid = false;
        VPR_THROW(VPR_ERROR_ROUTE, "Failed to find route tree node %d at (high coord %d,%d) in spatial lookup [bin %d,%d]",
                  rt_node->inode, rr_node.xhigh(), rr_node.yhigh(), bin_xhigh, bin_yhigh);
    }

    //Recurse
    for (t_linked_rt_edge* rt_edge = rt_node->u.child_list; rt_edge != nullptr; rt_edge = rt_edge->next) {
        valid &= validate_route_tree_spatial_lookup(rt_edge->child, spatial_lookup);
    }

    return valid;
}
