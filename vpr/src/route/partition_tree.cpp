#include "partition_tree.h"
#include <cmath>
#include <memory>

PartitionTree::PartitionTree(const Netlist<>& netlist) {
    const auto& device_ctx = g_vpr_ctx.device();

    auto all_nets = std::vector<ParentNetId>(netlist.nets().begin(), netlist.nets().end());
    _root = build_helper(netlist, all_nets, 0, 0, device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);
}

std::unique_ptr<PartitionTreeNode> PartitionTree::build_helper(const Netlist<>& netlist, const std::vector<ParentNetId>& nets, int x1, int y1, int x2, int y2) {
    if (nets.empty())
        return nullptr;

    const auto& route_ctx = g_vpr_ctx.routing();
    auto out = std::make_unique<PartitionTreeNode>();

    /* Build ParaDRo-ish prefix sum lookup for each bin (coordinate) in the device.
     * Do this for every step with only given nets, because each cutline takes some nets out
     * of the game, so if we just built a global lookup it wouldn't yield accurate results.
     *
     * VPR's bounding boxes include the borders (see ConnectionRouter::timing_driven_expand_neighbour())
     * so try to include x=bb.xmax, y=bb.ymax etc. when calculating things. */
    int W = x2 - x1 + 1;
    int H = y2 - y1 + 1;

    VTR_ASSERT(W > 1 && H > 1);
    /* Cutlines are placed between integral coordinates.
     * For instance, x_total_before[0] assumes a cutline at x=0.5, so fanouts at x=0 are included but not
     * x=1. It's similar for x_total_after[0], which excludes fanouts at x=0 and includes x=1.
     * Note that we have W-1 possible cutlines for a W-wide box. */
    std::vector<int> x_total_before(W - 1, 0), x_total_after(W - 1, 0);
    std::vector<int> y_total_before(H - 1, 0), y_total_after(H - 1, 0);

    for (auto net_id : nets) {
        t_bb bb = route_ctx.route_bb[net_id];
        size_t fanouts = netlist.net_sinks(net_id).size();

        /* Inclusive start and end coords of the bbox relative to x1. Clamp to [x1, x2]. */
        int x_start = std::max(x1, bb.xmin) - x1;
        int x_end = std::min(bb.xmax, x2) - x1;
        /* Fill in the lookups assuming a cutline at x + 0.5. */
        for (int x = x_start; x < W - 1; x++) {
            x_total_before[x] += fanouts;
        }
        for (int x = 0; x < x_end; x++) {
            x_total_after[x] += fanouts;
        }
        int y_start = std::max(y1, bb.ymin) - y1;
        int y_end = std::min(bb.ymax, y2) - y1;
        for (int y = y_start; y < H - 1; y++) {
            y_total_before[y] += fanouts;
        }
        for (int y = 0; y < y_end; y++) {
            y_total_after[y] += fanouts;
        }
    }

    int best_score = std::numeric_limits<int>::max();
    float best_pos = std::numeric_limits<double>::quiet_NaN();
    Axis best_axis = Axis::X;

    int max_x_before = x_total_before[W - 2];
    int max_x_after = x_total_after[0];
    for (int x = 0; x < W - 1; x++) {
        int before = x_total_before[x];
        int after = x_total_after[x];
        if (before == max_x_before || after == max_x_after) /* Cutting here would leave no nets to the left or right */
            continue;
        int score = abs(x_total_before[x] - x_total_after[x]);
        if (score < best_score) {
            best_score = score;
            best_pos = x1 + x + 0.5; /* Lookups are relative to (x1, y1) */
            best_axis = Axis::X;
        }
    }

    int max_y_before = y_total_before[H - 2];
    int max_y_after = y_total_after[0];
    for (int y = 0; y < H - 1; y++) {
        int before = y_total_before[y];
        int after = y_total_after[y];
        if (before == max_y_before || after == max_y_after) /* Cutting here would leave no nets to the left or right (sideways) */
            continue;
        int score = abs(y_total_before[y] - y_total_after[y]);
        if (score < best_score) {
            best_score = score;
            best_pos = y1 + y + 0.5; /* Lookups are relative to (x1, y1) */
            best_axis = Axis::Y;
        }
    }

    /* Couldn't find a cutline: all cutlines result in a one-way cut */
    if (std::isnan(best_pos)) {
        out->nets = nets; /* We hope copy elision is smart enough to optimize this stuff out */
        return out;
    }

    /* Populate net IDs on each side and call next level of build_x */
    std::vector<ParentNetId> left_nets, right_nets, my_nets;

    if (best_axis == Axis::X) {
        for (auto net_id : nets) {
            t_bb bb = route_ctx.route_bb[net_id];
            if (bb.xmax < best_pos) {
                left_nets.push_back(net_id);
            } else if (bb.xmin > best_pos) {
                right_nets.push_back(net_id);
            } else {
                my_nets.push_back(net_id);
            }
        }

        out->left = build_helper(netlist, left_nets, x1, y1, std::floor(best_pos), y2);
        out->right = build_helper(netlist, right_nets, std::floor(best_pos + 1), y1, x2, y2);
    } else {
        VTR_ASSERT(best_axis == Axis::Y);
        for (auto net_id : nets) {
            t_bb bb = route_ctx.route_bb[net_id];
            if (bb.ymax < best_pos) {
                left_nets.push_back(net_id);
            } else if (bb.ymin > best_pos) {
                right_nets.push_back(net_id);
            } else {
                my_nets.push_back(net_id);
            }
        }

        out->left = build_helper(netlist, left_nets, x1, y1, x2, std::floor(best_pos));
        out->right = build_helper(netlist, right_nets, x1, std::floor(best_pos + 1), x2, y2);
    }

    out->nets = my_nets;
    out->cutline_axis = best_axis;
    out->cutline_pos = best_pos;
    return out;
}
