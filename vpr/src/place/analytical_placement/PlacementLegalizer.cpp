
#include "PlacementLegalizer.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <stack>
#include <unordered_set>
#include <vector>
#include "PartialPlacement.h"
#include "globals.h"
#include "vtr_assert.h"
#include "vtr_log.h"

namespace {

// TODO: This should probably be moved into its own class for ArchModel.
struct Bin {
    // The IDs of the nodes contained within this bin.
    std::unordered_set<size_t> contained_node_ids;
    // Is this bin blocked for any reason.
    // TODO: From the perspective of the atom being moved, if it cannot be
    //       moved into this block it should be considered as blocked. So this
    //       value may need to be computed at runtime.
    bool is_blocked = false;

    // For now store the bin locations in the struct.
    // FIXME: This can be optimized.
    double x_pos;
    double y_pos;
};

} // namespace

static inline size_t getBinId(size_t x, size_t y) {
    size_t grid_width = g_vpr_ctx.device().grid.width();
    return x + (y * grid_width);
}

static inline void getNeighborsOfBin(const Bin& bin, std::vector<size_t>& neighbors) {
    size_t grid_width = g_vpr_ctx.device().grid.width();
    size_t grid_height = g_vpr_ctx.device().grid.width();
    if (bin.x_pos >= 1)
        neighbors.push_back(getBinId(bin.x_pos - 1, bin.y_pos));
    if (bin.x_pos <= grid_width - 2)
        neighbors.push_back(getBinId(bin.x_pos + 1, bin.y_pos));
    if (bin.y_pos >= 1)
        neighbors.push_back(getBinId(bin.x_pos, bin.y_pos - 1));
    if (bin.y_pos <= grid_height - 2)
        neighbors.push_back(getBinId(bin.x_pos, bin.y_pos + 1));
}

// Helper method to compute the phi term in the durav algorithm.
static inline double computeMaxMovement(size_t iter) {
    return 100 * (iter + 1) * (iter + 1);
}

// FIXME: REMOVE THIS!
#define BIN_CAPACITY 10

// Helper method to get the supply of the bin.
//  returns 0 if the bin is empty
//  else returns node-size - 1
// FIXME: This implies that each bin can only hold one thing.
static inline size_t getSupply(const Bin& b) {
    if (b.contained_node_ids.empty())
        return 0;
    size_t bin_capacity = BIN_CAPACITY;
    if (b.contained_node_ids.size() <= bin_capacity)
        return 0;
    return b.contained_node_ids.size() - bin_capacity;
}

// Helper method to get the closest node in the src bin to the sink bin.
// NOTE: This is from the original solved position of the node in the src bin.
static inline size_t getClosestNode(const PartialPlacement& p_placement,
                                    std::vector<Bin>& bins,
                                    size_t src_bin_id,
                                    size_t sink_bin_id,
                                    double &smallest_block_dist) {
    double sink_bin_x = bins[sink_bin_id].x_pos;
    double sink_bin_y = bins[sink_bin_id].y_pos;

    const Bin& src_bin = bins[src_bin_id];
    VTR_ASSERT(!src_bin.contained_node_ids.empty()
                && "Cannot get closest block of empty bin.");
    size_t closest_node_id = 0;
    smallest_block_dist = std::numeric_limits<double>::infinity();
    for (size_t node_id : src_bin.contained_node_ids) {
        // NOTE: Slight change from original implementation. Not getting bin pos.
        double orig_node_bin_x = p_placement.node_loc_x[node_id];
        double orig_node_bin_y = p_placement.node_loc_y[node_id];
        double dx = orig_node_bin_x - sink_bin_x;
        double dy = orig_node_bin_y - sink_bin_y;
        double block_dist = (dx * dx) + (dy * dy);
        if (block_dist < smallest_block_dist) {
            closest_node_id = node_id;
            smallest_block_dist = block_dist;
        }
    }
    return closest_node_id;
}

// Helper method to compute the cost of moving objects from the src bin to the
// sink bin.
static inline double computeCost(const PartialPlacement& p_placement,
                                 std::vector<Bin>& bins,
                                 double psi,
                                 size_t src_bin_id,
                                 size_t sink_bin_id) {
    // If the src bin is empty, then there is nothing to move.
    if (bins[src_bin_id].contained_node_ids.empty())
        return std::numeric_limits<double>::infinity();

    // Get the weight, which is proportional to the size of the set that
    // contains the src.
    // FIXME: What happens when this is zero?
    size_t src_supply = getSupply(bins[src_bin_id]);
    double wt = static_cast<double>(src_supply);

    // Get the minimum quadratic movement to move a cell from the src bin to
    // the sink bin.
    // NOTE: This assumes no diagonal movements.
    double quad_mvmt;
    getClosestNode(p_placement, bins, src_bin_id, sink_bin_id, quad_mvmt);

    // If the movement is larger than psi, return infinity
    if (quad_mvmt >= psi)
        return std::numeric_limits<double>::infinity();

    // Following the equation from the Darav paper.
    return wt * quad_mvmt;
}

// Helper method to get the paths to flow nodes between bins.
static inline void getPaths(const PartialPlacement& p_placement,
                            std::vector<Bin>& bins,
                            double psi,
                            size_t starting_bin_id,
                            std::vector<std::vector<size_t>>& paths) {
    // Create a visited vector.
    std::vector<bool> bin_visited(bins.size());
    std::fill(bin_visited.begin(), bin_visited.end(), false);
    bin_visited[starting_bin_id] = true;
    // Create a cost array. A path can be uniquely identified by its tail bin
    // cost.
    std::vector<double> bin_cost(bins.size());
    std::fill(bin_cost.begin(), bin_cost.end(), 0.0);
    // Create a starting path.
    std::vector<size_t> starting_path;
    starting_path.push_back(starting_bin_id);
    // Create a FIFO queue.
    std::queue<std::vector<size_t>> queue;
    queue.push(std::move(starting_path));

    size_t demand = 0;
    size_t starting_bin_supply = getSupply(bins[starting_bin_id]);
    while (!queue.empty() && demand < starting_bin_supply) {
        std::vector<size_t> &p = queue.front();
        size_t tail_bin_id = p.back();
        // Get the neighbors of the tail bin.
        std::vector<size_t> neighbor_bin_ids;
        getNeighborsOfBin(bins[tail_bin_id], neighbor_bin_ids);
        for (size_t neighbor_bin_id : neighbor_bin_ids) {
            // TODO: This is where the bin being blocked can be calculated.
            // FIXME: I do not think this should be here...
            //        This may be encapsulated in the neighbor connections.
            if (bins[neighbor_bin_id].is_blocked)
                continue;
            if (bin_visited[neighbor_bin_id])
                continue;
            double cost = computeCost(p_placement, bins, psi, tail_bin_id, neighbor_bin_id);
            if (cost < std::numeric_limits<double>::infinity()) {
                std::vector<size_t> p_copy(p);
                bin_cost[neighbor_bin_id] = bin_cost[tail_bin_id] + cost;
                p_copy.push_back(neighbor_bin_id);
                bin_visited[neighbor_bin_id] = true;
                // NOTE: Needed to change this from the algorithm since it was
                //       skipping partially filled bins. Maybe indicative of a
                //       bug somewhere.
                // if (bins[neighbor_bin_id].contained_node_ids.empty()) {
                if (bins[neighbor_bin_id].contained_node_ids.size() < BIN_CAPACITY) {
                    paths.push_back(std::move(p_copy));
                    demand += BIN_CAPACITY - bins[neighbor_bin_id].contained_node_ids.size();
                } else {
                    queue.push(std::move(p_copy));
                }
            }
        }
        queue.pop();
    }

    // Sort the paths in increasing order of cost.
    std::sort(paths.begin(), paths.end(), [&](std::vector<size_t>& a, std::vector<size_t>& b) {
        return bin_cost[a.back()] < bin_cost[b.back()];
    });
}

// Helper method to move cells along a given path.
static inline void moveCells(const PartialPlacement& p_placement,
                             std::vector<Bin>& bins,
                             double psi,
                             std::vector<size_t>& path) {
    VTR_ASSERT(!path.empty());
    size_t src_bin_id = path[0];
    std::stack<size_t> s;
    s.push(src_bin_id);
    size_t path_size = path.size();
    for (size_t path_index = 1; path_index < path_size; path_index++) {
        size_t sink_bin_id = path[path_index];
        double cost = computeCost(p_placement, bins, psi, src_bin_id, sink_bin_id);
        if (cost == std::numeric_limits<double>::infinity())
            return;
        //  continue;
        src_bin_id = sink_bin_id;
        s.push(sink_bin_id);
    }
    size_t sink_bin_id = s.top();
    s.pop();
    while (!s.empty()) {
        src_bin_id = s.top();
        s.pop();
        // Minor change to the algorithm proposed by Darav et al., find the
        // closest point in src to sink and move it to sink (instead of sorting
        // the whole list which is wasteful).
        // TODO: Verify this.
        Bin& src_bin = bins[src_bin_id];
        VTR_ASSERT(!src_bin.contained_node_ids.empty() && "Cannot move a node from an empty bin.");
        double smallest_block_dist;
        size_t closest_node_id = getClosestNode(p_placement, bins, src_bin_id, sink_bin_id, smallest_block_dist);
        src_bin.contained_node_ids.erase(closest_node_id);
        Bin& sink_bin = bins[sink_bin_id];
        sink_bin.contained_node_ids.insert(closest_node_id);

        sink_bin_id = src_bin_id;
    }
}

// Flow-Based Legalizer based off work by Darav et al.
// https://dl.acm.org/doi/10.1145/3289602.3293896
void FlowBasedLegalizer::legalize(PartialPlacement &p_placement) {
    VTR_LOG("Running Flow-Based Legalizer\n");

    // Initialize the bins with the node positions.
    // TODO: Encapsulate this into a class.
    size_t grid_width = g_vpr_ctx.device().grid.width();
    size_t grid_height = g_vpr_ctx.device().grid.height();
    VTR_LOG("\tGrid size = (%zu, %zu)\n", grid_width, grid_height);
    std::vector<Bin> bins(grid_width * grid_height);
    for (size_t i = 0; i < grid_width; i++) {
        for (size_t j = 0; j < grid_height; j++) {
            size_t bin_id = getBinId(i, j);
            bins[bin_id].x_pos = i;
            bins[bin_id].y_pos = j;
        }
    }
    // FIXME: What about the fixed blocks? Shouldnt they be put into the bins?
    for (size_t node_id = 0; node_id < p_placement.num_moveable_nodes; node_id++) {
        double node_x = p_placement.node_loc_x[node_id];
        double node_y = p_placement.node_loc_y[node_id];
        // FIXME: This conversion is not necessarily correct. The bin position
        //        should be the center of the tile, not the corner.
        size_t bin_id = getBinId(node_x, node_y);
        bins[bin_id].contained_node_ids.insert(node_id);
    }
    // Label the IO tiles as obstacles.
    // FIXME: This should be dynamically calculated, as explained above.
    // Do not know how to do this properly. For now assume the IO is on the
    // perimeter without the corners.
    for (size_t i = 0; i < grid_width; i++) {
        bins[getBinId(i, 0)].is_blocked = true;
        bins[getBinId(i, grid_height - 1)].is_blocked = true;
    }
    for (size_t j = 0; j < grid_height; j++) {
        bins[getBinId(0, j)].is_blocked = true;
        bins[getBinId(grid_width - 1, j)].is_blocked = true;
    }

    // Run the flow-based spreader.
    size_t flowBasedIter = 0;
    while (true) {
        if (flowBasedIter > 1000) {
            VTR_LOG("HIT MAX ITERATION!!!\n");
            break;
        }
        // Compute the max movement.
        double psi = computeMaxMovement(flowBasedIter);

        // Get the overfilled bins and sort them in increasing order of supply.
        std::vector<size_t> overfilled_bin_ids;
        size_t num_bins = bins.size();
        for (size_t bin_id = 0; bin_id < num_bins; bin_id++) {
            if (getSupply(bins[bin_id]) > 0)
                overfilled_bin_ids.push_back(bin_id);
        }
        if (overfilled_bin_ids.empty()) {
            // VTR_LOG("No overfilled bins! No spreading needed!\n");
            break;
        }
        std::sort(overfilled_bin_ids.begin(), overfilled_bin_ids.end(), [&](size_t a, size_t b) {
            return getSupply(bins[a]) < getSupply(bins[b]);
        });
        // VTR_LOG("Num overfilled bins: %zu\n", overfilled_bin_ids.size());
        // VTR_LOG("\tLargest overfilled bin supply: %zu\n", getSupply(bins[overfilled_bin_ids.back()]));
        // VTR_LOG("\tpsi = %f\n", psi);

        for (size_t bin_id : overfilled_bin_ids) {
            // Get the list of candidate paths based on psi.
            //  NOTE: The paths are sorted by increasing cost within the
            //        getPaths method.
            std::vector<std::vector<size_t>> paths;
            getPaths(p_placement, bins, psi, bin_id, paths);

            // VTR_LOG("\tNum paths: %zu\n", paths.size());
            for (std::vector<size_t>& path : paths) {
                if (getSupply(bins[bin_id]) == 0)
                    continue;
                // Move cells over the paths.
                //  NOTE: This will modify the bins. (actual block positions
                //        will not change (yet)).
                moveCells(p_placement, bins, psi, path);
            }
        }

        // TODO: Get the total cell displacement for debugging.

        flowBasedIter++;
    }
    VTR_LOG("Flow-Based Legalizer finished in %zu iterations.\n", flowBasedIter + 1);

    // Update the partial placement with the spread blocks.
    // TODO: Look into only updating the nodes that actually moved.
    for (const Bin& bin : bins) {
        for (size_t node_id : bin.contained_node_ids) {
            // TODO: This movement is a bit strange. It should probably move to
            //       the center of the block or the edge closest to the solved
            //       position.
            double node_x = p_placement.node_loc_x[node_id];
            double node_y = p_placement.node_loc_y[node_id];
            double offset_x = node_x - std::floor(node_x);
            double offset_y = node_y - std::floor(node_y);
            p_placement.node_loc_x[node_id] = bin.x_pos + offset_x;
            p_placement.node_loc_y[node_id] = bin.y_pos + offset_y;
        }
    }
}

void FullLegalizer::legalize(PartialPlacement& p_placement) {
    (void)p_placement;
    VTR_LOG("Running Full Legalizer\n");
}

