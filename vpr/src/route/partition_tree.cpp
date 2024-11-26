#include "partition_tree.h"
#include <cmath>
#include <memory>
#include <unordered_set>

/** Minimum number of nets inside a partition to continue further partitioning.
 * Mostly an arbitrary limit. At a certain point, the quality lost due to disturbed net ordering 
 * and the task creation overhead outweighs the advantage of partitioning, so we should stop. */
constexpr size_t MIN_NETS_TO_PARTITION = 256;

PartitionTree::PartitionTree(const Netlist<>& netlist) {
    const auto& device_ctx = g_vpr_ctx.device();

    auto all_nets = std::unordered_set<ParentNetId>(netlist.nets().begin(), netlist.nets().end());
    _root = build_helper(netlist, all_nets, 0, 0, device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);
}

/** Build a branch of the PartitionTree given a set of \p nets and a bounding box.
 * Calls itself recursively with smaller and smaller bounding boxes until there are less
 * nets than \ref MIN_NETS_TO_PARTITION. */
std::unique_ptr<PartitionTreeNode> PartitionTree::build_helper(const Netlist<>& netlist, const std::unordered_set<ParentNetId>& nets, int x1, int y1, int x2, int y2) {
    if (nets.empty())
        return nullptr;

    const auto& route_ctx = g_vpr_ctx.routing();

    /* Only build this for 2 dimensions. Ignore the layers for now */
    const auto& device_ctx = g_vpr_ctx.device();
    int layer_max = device_ctx.grid.get_num_layers() - 1;

    auto out = std::make_unique<PartitionTreeNode>();

    if (nets.size() < MIN_NETS_TO_PARTITION) {
        out->bb = {x1, x2, y1, y2, 0, layer_max};
        out->nets = nets;
        /* Build net to ptree node lookup */
        for(auto net_id: nets){
            _net_to_ptree_node[net_id] = out.get();
        }
        return out;
    }

    /* Build ParaDRo-ish prefix sum lookup for each bin (coordinate) in the device.
     * Do this for every step with only given nets, because each cutline takes some nets out
     * of the game, so if we just built a global lookup it wouldn't yield accurate results.
     *
     * VPR's bounding boxes include the borders (see ConnectionRouter::timing_driven_expand_neighbour())
     * so try to include x=bb.xmax, y=bb.ymax etc. when calculating things. */
    int width = x2 - x1 + 1;
    int height = y2 - y1 + 1;

    VTR_ASSERT(width > 1 && height > 1);
    /* Cutlines are placed between integral coordinates.
     * For instance, x_total_before[0] assumes a cutline at x=0.5, so fanouts at x=0 are included but not
     * x=1. It's similar for x_total_after[0], which excludes fanouts at x=0 and includes x=1.
     * Note that we have W-1 possible cutlines for a W-wide box.
     *
     * Here, *_total_before holds total score of nets before the cutline and not intersecting it.
     * In ParaDRo this would be total_before + total_on. (same for total_after)*/
    std::vector<int> x_total_before(width - 1, 0), x_total_after(width - 1, 0), x_total_on(width - 1, 0);
    std::vector<int> y_total_before(height - 1, 0), y_total_after(height - 1, 0), y_total_on(height - 1, 0);

    for (auto net_id : nets) {
        t_bb bb = route_ctx.route_bb[net_id];
        size_t fanouts = netlist.net_sinks(net_id).size();

        /* Inclusive start and end coords of the bbox relative to x1. Clamp to [x1, x2]. */
        int x_start = std::max(x1, bb.xmin) - x1;
        int x_end = std::min(bb.xmax, x2) - x1;
        /* Fill in the lookups assuming a cutline at x + 0.5.
         * This means total_before includes the max coord of the bbox but
         * total_after does not include the min coord. */
        for (int x = x_end; x < width - 1; x++) {
            x_total_before[x] += fanouts;
        }
        for (int x = 0; x < x_start; x++) {
            x_total_after[x] += fanouts;
        }
        for (int x = x_start; x < x_end; x++) {
            x_total_on[x] += fanouts;
        }
        int y_start = std::max(y1, bb.ymin) - y1;
        int y_end = std::min(bb.ymax, y2) - y1;
        for (int y = y_end; y < height - 1; y++) {
            y_total_before[y] += fanouts;
        }
        for (int y = 0; y < y_start; y++) {
            y_total_after[y] += fanouts;
        }
        for (int y = y_start; y < y_end; y++) {
            y_total_on[y] += fanouts;
        }
    }

    int best_score = std::numeric_limits<int>::max();
    float best_pos = std::numeric_limits<double>::quiet_NaN();
    Axis best_axis = Axis::X;

    for (int x = 0; x < width - 1; x++) {
        int before = x_total_before[x];
        int after = x_total_after[x];
        if (before == 0 || after == 0) /* Cutting here would leave no nets to the left or right */
            continue;
        /* Now get a measure of "critical path": work on cutline + max(work on sides) */
        int score = x_total_on[x] + std::max(x_total_before[x], x_total_after[x]);
        // int score = std::abs(int(x_total_before[x]) - int(x_total_after[x]));
        if (score < best_score) {
            best_score = score;
            best_pos = x1 + x + 0.5; /* Lookups are relative to (x1, y1) */
            best_axis = Axis::X;
        }
    }

    for (int y = 0; y < height - 1; y++) {
        int before = y_total_before[y];
        int after = y_total_after[y];
        if (before == 0 || after == 0) /* Cutting here would leave no nets to the left or right (sideways) */
            continue;
        int score = y_total_on[y] + std::max(y_total_before[y], y_total_after[y]);
        // int score = std::abs(int(y_total_before[y]) - int(y_total_after[y]));
        if (score < best_score) {
            best_score = score;
            best_pos = y1 + y + 0.5; /* Lookups are relative to (x1, y1) */
            best_axis = Axis::Y;
        }
    }

    /* Couldn't find a cutline: all cutlines result in a one-way cut */
    if (std::isnan(best_pos)) {
        out->bb = {x1, x2, y1, y2, 0, layer_max};
        out->nets = nets;
        /* Build net to ptree node lookup */
        for(auto net_id: nets){
            _net_to_ptree_node[net_id] = out.get();
        }
    }

    /* Populate net IDs on each side and call next level of build_x */
    std::unordered_set<ParentNetId> left_nets, right_nets, my_nets;

    if (best_axis == Axis::X) {
        for (auto net_id : nets) {
            t_bb bb = route_ctx.route_bb[net_id];
            if (bb.xmax < best_pos) {
                left_nets.insert(net_id);
            } else if (bb.xmin > best_pos) {
                right_nets.insert(net_id);
            } else {
                my_nets.insert(net_id);
            }
        }

        out->left = build_helper(netlist, left_nets, x1, y1, std::floor(best_pos), y2);
        out->right = build_helper(netlist, right_nets, std::floor(best_pos + 1), y1, x2, y2);
    } else {
        VTR_ASSERT(best_axis == Axis::Y);
        for (auto net_id : nets) {
            t_bb bb = route_ctx.route_bb[net_id];
            if (bb.ymax < best_pos) {
                left_nets.insert(net_id);
            } else if (bb.ymin > best_pos) {
                right_nets.insert(net_id);
            } else {
                my_nets.insert(net_id);
            }
        }

        out->left = build_helper(netlist, left_nets, x1, y1, x2, std::floor(best_pos));
        out->right = build_helper(netlist, right_nets, x1, std::floor(best_pos + 1), x2, y2);
    }

    if(out->left)
        out->left->parent = out.get();
    if(out->right)
        out->right->parent = out.get();

    out->bb = {x1, x2, y1, y2, 0, 0};
    out->nets = my_nets;
    out->cutline_axis = best_axis;
    out->cutline_pos = best_pos;

    /* Build net to ptree node lookup */
    for(auto net_id: my_nets){
        _net_to_ptree_node[net_id] = out.get();
    }
    return out;
}

inline bool net_in_ptree_node(ParentNetId net_id, const PartitionTreeNode* node){
    auto& route_ctx = g_vpr_ctx.routing();
    const t_bb& bb = route_ctx.route_bb[net_id];
    return bb.xmin >= node->bb.xmin && bb.xmax <= node->bb.xmax && bb.ymin >= node->bb.ymin && bb.ymax <= node->bb.ymax;
}

void PartitionTree::update_nets(const std::vector<ParentNetId>& nets) {
    for(auto net_id: nets){
        PartitionTreeNode* old_ptree_node = _net_to_ptree_node[net_id];
        PartitionTreeNode* new_ptree_node = old_ptree_node;
        while(!net_in_ptree_node(net_id, new_ptree_node))
            new_ptree_node = new_ptree_node->parent;
        old_ptree_node->nets.erase(net_id);
        new_ptree_node->nets.insert(net_id);
        _net_to_ptree_node[net_id] = new_ptree_node;
    }
}

/** Delete all vnets from this tree */
void PartitionTree::clear_vnets(void) {
    std::stack<PartitionTreeNode*> stack;
    stack.push(_root.get());
    while(!stack.empty()){
        PartitionTreeNode* node = stack.top();
        stack.pop();
        node->vnets.clear();
        if(node->left)
            stack.push(node->left.get());
        if(node->right)
            stack.push(node->right.get());
    }
}
