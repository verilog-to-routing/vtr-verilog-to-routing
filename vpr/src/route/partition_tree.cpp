#include "partition_tree.h"
#include <memory>

PartitionTree::PartitionTree(const Netlist<>& netlist) {
    const auto& device_ctx = g_vpr_ctx.device();

    auto all_nets = std::vector<ParentNetId>(netlist.nets().begin(), netlist.nets().end());
    _root = build_helper(netlist, all_nets, 0, 0, device_ctx.grid.width(), device_ctx.grid.height());
}

std::unique_ptr<PartitionTreeNode> PartitionTree::build_helper(const Netlist<>& netlist, const std::vector<ParentNetId>& nets, int x1, int y1, int x2, int y2) {
    if (nets.empty())
        return nullptr;

    const auto& route_ctx = g_vpr_ctx.routing();
    auto out = std::make_unique<PartitionTreeNode>();

    /* Find best cutline. In ParaDRo this is done using prefix sums, but
     * life is too short to implement them, therefore I'm just doing a linear search,
     * and the complexity is O((fpga width + height) * #nets * log2(w+h * #nets)).
     * What we are searching for is the cutline with the most balanced workload (# of fanouts)
     * on the sides. */
    int left, right, mine;
    int score;
    /* TODO: maybe put all of this into a tuple or struct? */
    int best_score = std::numeric_limits<int>::max();
    int best_pos = -1, best_left = -1, best_right = -1;
    enum { X,
           Y } best_axis
        = X;

    for (int x = x1 + 1; x < x2; x++) {
        left = right = mine = 0;
        for (auto net_id : nets) {
            t_bb bb = route_ctx.route_bb[net_id];
            size_t fanout = netlist.net_sinks(net_id).size();
            if (bb.xmin < x && bb.xmax < x) {
                left += fanout;
            } else if (bb.xmin > x && bb.xmax > x) {
                right += fanout;
            } else if (bb.xmin <= x && bb.xmax >= x) {
                mine += fanout;
            } else {
                VTR_ASSERT(false); /* unreachable */
            }
        }
        score = abs(left - right);
        if (score < best_score) {
            best_score = score;
            best_left = left;
            best_right = right;
            best_pos = x;
            best_axis = X;
        }
    }
    for (int y = y1 + 1; y < y2; y++) {
        left = right = mine = 0;
        for (auto net_id : nets) {
            t_bb bb = route_ctx.route_bb[net_id];
            size_t fanout = netlist.net_sinks(net_id).size();
            if (bb.ymin < y && bb.ymax < y) {
                left += fanout;
            } else if (bb.ymin > y && bb.ymax > y) {
                right += fanout;
            } else if (bb.ymin <= y && bb.ymax >= y) {
                mine += fanout;
            } else {
                VTR_ASSERT(false); /* unreachable */
            }
        }
        score = abs(left - right);
        if (score < best_score) {
            best_score = score;
            best_left = left;
            best_right = right;
            best_pos = y;
            best_axis = Y;
        }
    }

    /* If one of the sides has 0 nets in the best arrangement,
     * there's no use in partitioning this: no parallelism comes out of it. */
    if (best_left == 0 || best_right == 0) {
        out->nets = std::move(nets);
        return out;
    }

    /* Populate net IDs on each side
     * and call next level of build_partition_trees. */
    std::vector<ParentNetId> left_nets, right_nets, my_nets;

    if (best_axis == X) {
        for (auto net_id : nets) {
            t_bb bb = route_ctx.route_bb[net_id];
            if (bb.xmin < best_pos && bb.xmax < best_pos) {
                left_nets.push_back(net_id);
            } else if (bb.xmin > best_pos && bb.xmax > best_pos) {
                right_nets.push_back(net_id);
            } else if (bb.xmin <= best_pos && bb.xmax >= best_pos) {
                my_nets.push_back(net_id);
            } else {
                VTR_ASSERT(false); /* unreachable */
            }
        }

        out->left = build_helper(netlist, left_nets, x1, y1, best_pos, y2);
        out->right = build_helper(netlist, right_nets, best_pos, y2, x2, y2);
    } else {
        VTR_ASSERT(best_axis == Y);
        for (auto net_id : nets) {
            t_bb bb = route_ctx.route_bb[net_id];
            if (bb.ymin < best_pos && bb.ymax < best_pos) {
                left_nets.push_back(net_id);
            } else if (bb.ymin > best_pos && bb.ymax > best_pos) {
                right_nets.push_back(net_id);
            } else if (bb.ymin <= best_pos && bb.ymax >= best_pos) {
                my_nets.push_back(net_id);
            } else {
                VTR_ASSERT(false); /* unreachable */
            }
        }

        out->left = build_helper(netlist, left_nets, x1, best_pos, x2, y2);
        out->right = build_helper(netlist, right_nets, x1, y1, x2, best_pos);
    }

    out->nets = std::move(my_nets);
    out->cutline_axis = best_axis;
    out->cutline_pos = best_pos;
    return out;
}
