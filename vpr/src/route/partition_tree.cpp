#include "partition_tree.h"
#include <cmath>
#include <cstdint>
#include <memory>

/** Arbitrary limit to stop partitioning nets. At a certain point, the quality lost due to disturbed net ordering 
 * and the task creation overhead outweighs the advantage of partitioning, so we stop doing it. */
constexpr size_t MIN_NETS_TO_PARTITION = 256;

PartitionTree::PartitionTree(const Netlist<>& netlist, const vtr::vector<ParentNetId, uint32_t>& scores) {
    const auto& device_ctx = g_vpr_ctx.device();

    auto all_nets = std::vector<ParentNetId>(netlist.nets().begin(), netlist.nets().end());
    _root = build_helper(all_nets, scores, 0, 0, device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);
}

std::unique_ptr<PartitionTreeNode> PartitionTree::build_helper(const std::vector<ParentNetId>& nets, const vtr::vector<ParentNetId, uint32_t>& scores, int x1, int y1, int x2, int y2) {
    if (nets.empty())
        return nullptr;

    const auto& route_ctx = g_vpr_ctx.routing();
    auto out = std::make_unique<PartitionTreeNode>();

    if (nets.size() < MIN_NETS_TO_PARTITION){
        out->nets = nets;
        return out;
    }

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
     * Note that we have W-1 possible cutlines for a W-wide box.
     *
     * Here, *_total_before holds total score of nets before the cutline and not intersecting it.
     * In ParaDRo this would be total_before + total_on. (same for total_after)*/
    std::vector<uint64_t> x_total_before(W - 1, 0), x_total_after(W - 1, 0), x_total_on(W - 1, 0);
    std::vector<uint64_t> y_total_before(H - 1, 0), y_total_after(H - 1, 0), y_total_on(H - 1, 0);

    for (auto net_id : nets) {
        t_bb bb = route_ctx.route_bb[net_id];
        uint64_t score = scores[net_id];

        /* Inclusive start and end coords of the bbox relative to x1. Clamp to [x1, x2]. */
        int x_start = std::max(x1, bb.xmin) - x1;
        int x_end = std::min(bb.xmax, x2) - x1;
        /* Fill in the lookups assuming a cutline at x + 0.5.
         * This means total_before includes the max coord of the bbox but
         * total_after does not include the min coord. */
        for (int x = x_end; x < W - 1; x++) {
            x_total_before[x] += score;
        }
        for (int x = 0; x < x_start; x++) {
            x_total_after[x] += score;
        }
        for (int x = x_start; x < x_end; x++){
            x_total_on[x] += score;
        }
        int y_start = std::max(y1, bb.ymin) - y1;
        int y_end = std::min(bb.ymax, y2) - y1;
        for (int y = y_end; y < H - 1; y++) {
            y_total_before[y] += score;
        }
        for (int y = 0; y < y_start; y++) {
            y_total_after[y] += score;
        }
        for (int y = y_start; y < y_end; y++){
            y_total_on[y] += score;
        }
    }

    uint64_t best_score = std::numeric_limits<uint64_t>::max();
    float best_pos = std::numeric_limits<double>::quiet_NaN();
    Axis best_axis = Axis::X;

    for (int x = 0; x < W - 1; x++) {
        int before = x_total_before[x];
        int after = x_total_after[x];
        if (before == 0 || after == 0) /* Cutting here would leave no nets to the left or right */
            continue;
        /* Now get a measure of "critical path": work on cutline + max(work on sides)
         * Test: What happens if we discount max(sides)? */
        uint64_t score = x_total_on[x] + std::max(x_total_before[x], x_total_after[x]);
        // uint64_t score = std::abs(int(x_total_before[x]) - int(x_total_after[x]));
        if (score < best_score) {
            best_score = score;
            best_pos = x1 + x + 0.5; /* Lookups are relative to (x1, y1) */
            best_axis = Axis::X;
        } 
    }

    for (int y = 0; y < H - 1; y++) {
        int before = y_total_before[y];
        int after = y_total_after[y];
        if (before == 0 || after == 0) /* Cutting here would leave no nets to the left or right (sideways) */
            continue;
        uint64_t score = y_total_on[y] + std::max(y_total_before[y], y_total_after[y]);
        // uint64_t score = std::abs(int(y_total_before[y]) - int(y_total_after[y]));
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

        out->left = build_helper(left_nets, scores, x1, y1, std::floor(best_pos), y2);
        out->right = build_helper(right_nets, scores, std::floor(best_pos + 1), y1, x2, y2);
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

        out->left = build_helper(left_nets, scores, x1, y1, x2, std::floor(best_pos));
        out->right = build_helper(right_nets, scores, x1, std::floor(best_pos + 1), x2, y2);
    }

    out->nets = my_nets;
    out->nets = my_nets;
    out->cutline_axis = best_axis;
    out->cutline_pos = best_pos;
    return out;
}
