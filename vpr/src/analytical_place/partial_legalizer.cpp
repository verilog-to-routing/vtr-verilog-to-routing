/**
 * @file
 * @author  Alex Singer and Robert Luo
 * @date    October 2024
 * @brief   The definitions of the Partial Legalizers used in the AP flow and
 *          their base class.
 */

#include "partial_legalizer.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <stack>
#include <unordered_set>
#include <utility>
#include <vector>
#include "ap_netlist.h"
#include "flat_placement_bins.h"
#include "flat_placement_density_manager.h"
#include "flat_placement_mass_calculator.h"
#include "model_grouper.h"
#include "partial_placement.h"
#include "prepack.h"
#include "primitive_dim_manager.h"
#include "primitive_vector.h"
#include "vpr_error.h"
#include "vtr_assert.h"
#include "vtr_geometry.h"
#include "vtr_log.h"
#include "vtr_math.h"
#include "vtr_prefix_sum.h"
#include "vtr_strong_id.h"
#include "vtr_time.h"
#include "vtr_vector.h"
#include "vtr_vector_map.h"

std::unique_ptr<PartialLegalizer> make_partial_legalizer(e_ap_partial_legalizer legalizer_type,
                                                         const APNetlist& netlist,
                                                         std::shared_ptr<FlatPlacementDensityManager> density_manager,
                                                         const Prepacker& prepacker,
                                                         const LogicalModels& models,
                                                         int log_verbosity) {
    // Based on the partial legalizer type passed in, build the partial legalizer.
    switch (legalizer_type) {
        case e_ap_partial_legalizer::Identity:
            return std::make_unique<IdentityPartialLegalizer>(netlist,
                                                              log_verbosity);
        case e_ap_partial_legalizer::FlowBased:
            return std::make_unique<FlowBasedLegalizer>(netlist,
                                                        density_manager,
                                                        models,
                                                        log_verbosity);
        case e_ap_partial_legalizer::BiPartitioning:
            return std::make_unique<BiPartitioningPartialLegalizer>(netlist,
                                                                    density_manager,
                                                                    prepacker,
                                                                    models,
                                                                    log_verbosity);
        default:
            VPR_FATAL_ERROR(VPR_ERROR_AP,
                            "Unrecognized partial legalizer type");
            break;
    }
    return nullptr;
}

/**
 * @brief Helper method to get the direct neighbors of the given bin.
 *
 * A direct neighbor of a bin is a bin which shares a side with the given bin on
 * the tile graph. Corners do not count.
 */
static std::unordered_set<FlatPlacementBinId> get_direct_neighbors_of_bin(
    FlatPlacementBinId bin_id,
    const FlatPlacementDensityManager& density_manager) {
    const vtr::Rect<double>& bin_region = density_manager.flat_placement_bins().bin_region(bin_id);
    int bl_x = bin_region.bottom_left().x();
    int bl_y = bin_region.bottom_left().y();
    size_t bin_width = bin_region.width();
    size_t bin_height = bin_region.height();
    size_t bin_layer = density_manager.flat_placement_bins().bin_layer(bin_id);
    // This is an unfortunate consequence of using double precision to store
    // the bounding box. We need to ensure that the bin represents a tile (not
    // part of a tile). If it did represent part of a tile, this algorithm
    // would need to change.
    VTR_ASSERT_DEBUG(static_cast<double>(bl_x) == bin_region.bottom_left().x() && static_cast<double>(bl_y) == bin_region.bottom_left().y() && static_cast<double>(bin_width) == bin_region.width() && static_cast<double>(bin_height) == bin_region.height());

    double placeable_region_width, placeable_region_height, placeable_region_depth;
    std::tie(placeable_region_width, placeable_region_height, placeable_region_depth) = density_manager.get_overall_placeable_region_size();

    // Add the neighbors.
    std::unordered_set<FlatPlacementBinId> neighbor_bin_ids;
    // Add unique tiles on left and right sides
    for (size_t ty = bl_y; ty < bl_y + bin_height; ty++) {
        if (bl_x >= 1)
            neighbor_bin_ids.insert(density_manager.get_bin(bl_x - 1, ty, bin_layer));
        if (bl_x <= (int)(placeable_region_width - bin_width - 1))
            neighbor_bin_ids.insert(density_manager.get_bin(bl_x + bin_width, ty, bin_layer));
    }
    // Add unique tiles on the top and bottom
    for (size_t tx = bl_x; tx < bl_x + bin_width; tx++) {
        if (bl_y >= 1)
            neighbor_bin_ids.insert(density_manager.get_bin(tx, bl_y - 1, bin_layer));
        if (bl_y <= (int)(placeable_region_height - bin_height - 1))
            neighbor_bin_ids.insert(density_manager.get_bin(tx, bl_y + bin_height, bin_layer));
    }
    // Add layers above and below (if any)
    if (bin_layer >= 1) {
        for (size_t tx = bl_x; tx < bl_x + bin_width; tx++) {
            for (size_t ty = bl_y; ty < bl_y + bin_height; ty++) {
                neighbor_bin_ids.insert(density_manager.get_bin(tx, ty, bin_layer - 1));
            }
        }
    }
    if (bin_layer < placeable_region_depth - 1) {
        for (size_t tx = bl_x; tx < bl_x + bin_width; tx++) {
            for (size_t ty = bl_y; ty < bl_y + bin_height; ty++) {
                neighbor_bin_ids.insert(density_manager.get_bin(tx, ty, bin_layer + 1));
            }
        }
    }

    // A bin cannot be a neighbor with itself.
    VTR_ASSERT_DEBUG(neighbor_bin_ids.count(bin_id) == 0);

    return neighbor_bin_ids;
}

/**
 * @brief Get the center point of a rect
 */
static inline vtr::Point<double> get_center_of_rect(vtr::Rect<double> rect) {
    return rect.bottom_left() + vtr::Point<double>(rect.width() / 2.0, rect.height() / 2.0);
}

void FlowBasedLegalizer::compute_neighbors_of_bin(FlatPlacementBinId src_bin_id, const LogicalModels& models) {
    // Make sure that this bin does not already have neighbors.
    VTR_ASSERT_DEBUG(bin_neighbors_.size() == 0);

    // Bins need to be neighbors to every possible molecule type so things can
    // flow properly.
    // Perform BFS to find the closest bins of each type. Where closest is in
    // manhattan distance.

    const FlatPlacementBins& flat_placement_bins = density_manager_->flat_placement_bins();
    size_t num_bins = flat_placement_bins.bins().size();

    // Create the queue and insert the source bin into it.
    std::queue<FlatPlacementBinId> q;
    q.push(src_bin_id);
    // Create visited flags for each bin. Set the source to visited.
    vtr::vector_map<FlatPlacementBinId, bool> bin_visited(num_bins, false);
    bin_visited[src_bin_id] = true;
    // Create a distance count for each bin from the src.
    vtr::vector_map<FlatPlacementBinId, unsigned> bin_distance(num_bins, 0);
    // Flags to check if a specific model has been found in the given direction.
    // In this case, direction is the direction of the largest component of the
    // manhattan distance between the source bin and the target bin.
    size_t num_models = models.all_models().size();
    vtr::vector<LogicalModelId, bool> up_found(num_models, false);
    vtr::vector<LogicalModelId, bool> down_found(num_models, false);
    vtr::vector<LogicalModelId, bool> left_found(num_models, false);
    vtr::vector<LogicalModelId, bool> right_found(num_models, false);
    // Flags to check if all models have been found in a given direction.
    bool all_up_found = false;
    bool all_down_found = false;
    bool all_left_found = false;
    bool all_right_found = false;
    bool all_models_found_in_all_directions = false;
    // The center of the source bin.
    const vtr::Rect<double>& src_bin_region = flat_placement_bins.bin_region(src_bin_id);
    vtr::Point<double> src_bin_center = get_center_of_rect(src_bin_region);
    // The result will be stored in this set.
    std::unordered_set<FlatPlacementBinId> neighbors;

    // Helper method to add a neighbor to the set of neighbors and update the
    // found flags for a given direction if this bin is new for a given model
    // type. This method returns true if every model has been found in the given
    // direction (i.e. dir_found is now all true).
    auto add_neighbor_if_new_dir = [&](FlatPlacementBinId target_bin_id,
                                       vtr::vector<LogicalModelId, bool>& dir_found) {
        bool all_found = true;
        // Go through all possible models
        for (LogicalModelId model_id : models.all_models()) {
            // If this model has been found in this direction, continue.
            if (dir_found[model_id])
                continue;
            // If this bin has this model in its capacity, we found a neighbor!
            const PrimitiveVector& target_bin_capacity = density_manager_->get_bin_capacity(target_bin_id);
            PrimitiveVectorDim dim = density_manager_->mass_calculator().get_model_dim(model_id);
            if (target_bin_capacity.get_dim_val(dim) > 0) {
                dir_found[model_id] = true;
                neighbors.insert(target_bin_id);
            } else {
                all_found = false;
            }
        }
        return all_found;
    };

    // Perform the BFS from the source node until all nodes have been explored
    // or all of the models have been found in all directions.
    while (!q.empty() && !all_models_found_in_all_directions) {
        // Pop the bin from the queue.
        FlatPlacementBinId bin_id = q.front();
        q.pop();
        // If the distance of this block from the source is too large, do not
        // explore.
        unsigned curr_bin_dist = bin_distance[bin_id];
        if (curr_bin_dist > max_bin_neighbor_dist_)
            continue;
        // Get the direct neighbors of the bin (neighbors that are directly
        // touching).
        auto direct_neighbors = get_direct_neighbors_of_bin(bin_id, *density_manager_);
        for (FlatPlacementBinId dir_neighbor_bin_id : direct_neighbors) {
            // If this neighbor has been visited, do not do anything.
            if (bin_visited[dir_neighbor_bin_id])
                continue;
            // Get the signed distance from the src bin to the target bin in the
            // x and y dimensions.
            const vtr::Rect<double>& dir_neighbor_bin_region = flat_placement_bins.bin_region(dir_neighbor_bin_id);
            vtr::Point<double> target_bin_center = get_center_of_rect(dir_neighbor_bin_region);
            double dx = target_bin_center.x() - src_bin_center.x();
            double dy = target_bin_center.y() - src_bin_center.y();
            // Is the target bin above the source bin?
            if (!all_up_found && dy >= std::abs(dx)) {
                all_up_found = add_neighbor_if_new_dir(dir_neighbor_bin_id, up_found);
            }
            // Is the target bin below the source bin?
            if (!all_down_found && dy <= -std::abs(dx)) {
                all_down_found = add_neighbor_if_new_dir(dir_neighbor_bin_id, down_found);
            }
            // Is the target bin to the right of the source bin?
            if (!all_right_found && dx >= std::abs(dy)) {
                all_right_found = add_neighbor_if_new_dir(dir_neighbor_bin_id, right_found);
            }
            // Is the target bin to the left of the source bin?
            if (!all_left_found && dx <= -std::abs(dy)) {
                all_left_found = add_neighbor_if_new_dir(dir_neighbor_bin_id, left_found);
            }
            // Mark this bin as visited and push it onto the queue.
            bin_visited[dir_neighbor_bin_id] = true;
            // Update the distance.
            bin_distance[dir_neighbor_bin_id] = curr_bin_dist + 1;
            // FIXME: This may be inneficient since it will do an entire BFS of
            //        the grid if a neighbor of a given type does not exist in
            //        a specific direction. Should add a check to see if it is
            //        worth pushing this bin into the queue.
            q.push(dir_neighbor_bin_id);
        }
        // Check if all of the models have been found in all directions.
        all_models_found_in_all_directions = all_up_found && all_down_found && all_left_found && all_right_found;
    }

    // Assign the results into the neighbors of the bin.
    bin_neighbors_[src_bin_id].assign(neighbors.begin(), neighbors.end());
}

FlowBasedLegalizer::FlowBasedLegalizer(const APNetlist& netlist,
                                       std::shared_ptr<FlatPlacementDensityManager> density_manager,
                                       const LogicalModels& models,
                                       int log_verbosity)
    : PartialLegalizer(netlist, log_verbosity)
    , density_manager_(density_manager)
    , bin_neighbors_(density_manager_->flat_placement_bins().bins().size()) {

    // Connect the bins.
    for (FlatPlacementBinId bin_id : density_manager_->flat_placement_bins().bins()) {
        compute_neighbors_of_bin(bin_id, models);
    }
}

bool FlowBasedLegalizer::verify() const {
    if (density_manager_->verify() == false) {
        VTR_LOG("Flow-Based Legalizer Verify: Density Manager failed verification.\n");
    }
    // Make sure that the bins are connected correctly.
    for (FlatPlacementBinId bin_id : density_manager_->flat_placement_bins().bins()) {
        if (bin_neighbors_[bin_id].empty()) {
            VTR_LOG("Flow-Based Legalizer Verify: Found a bin with no neighbors.\n");
            return false;
        }
        // TODO: Should verify more about the connectivity. Such as every bin
        //       has a neighbor of each model type and a path exists from every
        //       bin to every other bin.
    }
    // If all above passed, then the bins are valid.
    return true;
}

// Helper method to compute the phi term in the durav algorithm.
static inline float computeMaxMovement(size_t iter) {
    return 100 * (iter + 1) * (iter + 1);
}

/**
 * @brief Find the minimum cost moveable block in the src_bin which is
 *        compatible with the target bin.
 *
 * Cost is the quadratic movement (distance squared) of the block from its
 * original position to the position it would be if it were moved into the bin.
 *
 *  @param src_bin          The bin that contains the blocks to move.
 *  @param target_bin       The bin to move blocks to.
 *  @param p_placement      The placement of the blocks prior to legalization.
 *  @param netlist          The APNetlist for the placement.
 *  @param density_manager  The density manager for this partial legalizer.
 *
 *  @return     A pair of the minimum cost moveable block and its cost.
 */
static inline std::pair<APBlockId, float> get_min_cost_block_in_bin(
    FlatPlacementBinId src_bin,
    FlatPlacementBinId target_bin,
    const PartialPlacement& p_placement,
    const APNetlist& netlist,
    const FlatPlacementDensityManager& density_manager) {
    // Get the min cost block and its cost.
    APBlockId min_cost_block;
    float min_cost = std::numeric_limits<float>::infinity();
    const FlatPlacementBins& bins = density_manager.flat_placement_bins();
    const FlatPlacementMassCalculator& mass_calculator = density_manager.mass_calculator();
    const std::unordered_set<APBlockId>& src_contained_blocks = bins.bin_contained_blocks(src_bin);
    // FIXME: If these were somehow pre-sorted, this can be made much cheaper.
    for (APBlockId blk_id : src_contained_blocks) {
        // If this block is fixed, it has infinite cost to move.
        if (netlist.block_mobility(blk_id) == APBlockMobility::FIXED)
            continue;
        const PrimitiveVector& block_mass = mass_calculator.get_block_mass(blk_id);
        // Is this block compatible with the target bin?
        // If the capacity of the target, projected onto the mass, is less than
        // the mass, then the block is not compatible.
        // TODO: We may want to add a cost term based on how much space is
        //       available in the bin?
        PrimitiveVector target_capacity = density_manager.get_bin_capacity(target_bin);
        target_capacity.project(block_mass);
        if (target_capacity < block_mass)
            continue;
        // Compute the quadratic movement (aka cost).
        const vtr::Rect<double>& target_bin_region = bins.bin_region(target_bin);
        const vtr::Point<double>& new_block_pos = density_manager.get_block_location_in_bin(blk_id,
                                                                                            target_bin_region,
                                                                                            p_placement);
        double dx = new_block_pos.x() - p_placement.block_x_locs[blk_id];
        double dy = new_block_pos.y() - p_placement.block_y_locs[blk_id];
        float cost = (dx * dx) + (dy * dy);
        // If this movement is the least we have seen, this is the min cost.
        // FIXME: We could add a cost weight to the block based on things such
        //        as timing. So critical blocks are less likely to move.
        if (cost < min_cost) {
            min_cost = cost;
            min_cost_block = blk_id;
        }
    }

    return std::make_pair(min_cost_block, min_cost);
}

/**
 * @brief Compute the cost of moving a block from the source bin into the
 *        target bin if a compatible block can be found.
 *
 *  @param src_bin          The bin that has blocks to be moved.
 *  @param target_bin       The bin to move the blocks into.
 *  @param psi              Algorithm parameter which represents the maximum
 *                          cost this function can return. This function will
 *                          return inf if the cost is larger than psi.
 *  @param p_placement      The placement of the blocks prior to legalization.
 *  @param netlist          The APNetlist for the placement.
 *  @param density_manager  The density manager for this partial legalizer.
 */
static inline float compute_cost(FlatPlacementBinId src_bin,
                                 FlatPlacementBinId target_bin,
                                 float psi,
                                 const PartialPlacement& p_placement,
                                 const APNetlist& netlist,
                                 const FlatPlacementDensityManager& density_manager) {
    // If the src bin is empty, then there is nothing to move.
    if (density_manager.flat_placement_bins().bin_contained_blocks(src_bin).size() == 0)
        return std::numeric_limits<float>::infinity();
    // Get the min cost block in the src bin which is compatible with the target
    // bin.
    APBlockId min_cost_block;
    float min_cost;
    std::tie(min_cost_block, min_cost) = get_min_cost_block_in_bin(src_bin,
                                                                   target_bin,
                                                                   p_placement,
                                                                   netlist,
                                                                   density_manager);
    // If no block can be moved to the target bin, return.
    if (std::isinf(min_cost))
        return std::numeric_limits<float>::infinity();
    // If the quadratic movement is larger than psi, return infinity.
    if (min_cost >= psi)
        return std::numeric_limits<float>::infinity();
    // Compute the weight, which is proportional to the number of blocks of the
    // same type as the min_cost block in the src bin.
    // This weight tries to keep blocks of the same type together.
    // This term can be found by taking the L1 norm of the projection of the
    // src bin's utilization on the direction of the mass.
    const FlatPlacementMassCalculator& mass_calculator = density_manager.mass_calculator();
    const PrimitiveVector& min_cost_block_mass = mass_calculator.get_block_mass(min_cost_block);
    PrimitiveVector weight_vec = density_manager.get_bin_utilization(src_bin);
    weight_vec.project(min_cost_block_mass);
    float weight = weight_vec.manhattan_norm();
    // Return the overall cost which is the quadratic movement times the weight.
    return weight * min_cost;
}

std::vector<std::vector<FlatPlacementBinId>> FlowBasedLegalizer::get_paths(
    FlatPlacementBinId src_bin_id,
    const PartialPlacement& p_placement,
    float psi) {
    VTR_LOGV(log_verbosity_ >= 20, "\tGetting paths...\n");
    const FlatPlacementBins& flat_placement_bins = density_manager_->flat_placement_bins();
    size_t num_bins = flat_placement_bins.bins().size();
    // Create a visited vector.
    vtr::vector_map<FlatPlacementBinId, bool> bin_visited(num_bins, false);
    bin_visited[src_bin_id] = true;
    // Create a cost array. The cost of a path is equal to the cost of its tail
    // bin.
    vtr::vector_map<FlatPlacementBinId, float> bin_cost(num_bins, 0.f);
    // Create a starting path.
    std::vector<FlatPlacementBinId> starting_path;
    starting_path.push_back(src_bin_id);
    // Create a FIFO queue.
    std::queue<std::vector<FlatPlacementBinId>> queue;
    queue.push(std::move(starting_path));
    // Create the resulting vector of paths.
    // TODO: Can we store this more efficiently as a tree?
    std::vector<std::vector<FlatPlacementBinId>> paths;
    // Perform the BFS to search for direct paths to flow the starting bin's
    // supply of primitives until it has found sufficient demand.
    PrimitiveVector demand;
    const PrimitiveVector& starting_bin_supply = get_bin_supply(src_bin_id);
    while (!queue.empty() && demand < starting_bin_supply) {
        // Pop the current bin off the queue.
        std::vector<FlatPlacementBinId>& p = queue.front();
        FlatPlacementBinId tail_bin_id = p.back();
        // Look over its neighbors
        for (FlatPlacementBinId neighbor_bin_id : bin_neighbors_[tail_bin_id]) {
            // If this bin has already been visited, skip it.
            if (bin_visited[neighbor_bin_id])
                continue;
            // Compute the cost of moving a block from the tail bin to its
            // neighbor.
            float cost = compute_cost(tail_bin_id,
                                      neighbor_bin_id,
                                      psi,
                                      p_placement,
                                      netlist_,
                                      *density_manager_);
            // If the cost is infinite, then the path cannot be made to this
            // neighbor bin.
            if (std::isinf(cost))
                continue;
            // Else, a path can be made.
            std::vector<FlatPlacementBinId> p_copy(p);
            bin_cost[neighbor_bin_id] = bin_cost[tail_bin_id] + cost;
            p_copy.push_back(neighbor_bin_id);
            bin_visited[neighbor_bin_id] = true;
            // Project the demand of the neighbor onto the starting supply to
            // get how much of the supply this bin can support. If this
            // projection is non-zero, this means that we can move blocks into
            // this bin as a target. If not, we can flow through it.
            // NOTE: This is different from Darav et al. Their original algorithm
            //       only terminated paths at empty bins. This caused the algorithm
            //       to never converge if all bins had 1 block in them. However
            //       this may impact performance since it stops as soon as it
            //       finds an open bin which may limit the flow. It also
            //       prevents the flow. This is something that needs to be
            //       investigated further...
            // FIXME: Perhaps we do not check if it is empty, but check if the
            //        demand is sufficiently large...
            PrimitiveVector neighbor_demand = get_bin_demand(neighbor_bin_id);
            neighbor_demand.project(starting_bin_supply);
            VTR_ASSERT_DEBUG(neighbor_demand.is_non_negative());
            // if (bins_[neighbor_bin_id].contained_blocks.size() == 0) {
            if (neighbor_demand.is_non_zero()) {
                // Add this to the resulting paths.
                paths.push_back(std::move(p_copy));
                // Accumulate the demand.
                demand += neighbor_demand;
            } else {
                // Add this path to the queue.
                queue.push(std::move(p_copy));
            }
        }
        // Pop the path from the queue. This pop is delayed to prevent copying
        // the path unnecessarily. This is allowed since this is a FIFO queue.
        queue.pop();
    }

    // Helpful debug messages.
    VTR_LOGV(log_verbosity_ >= 20, "\t\tSupply of source bin: %.2f\n",
             starting_bin_supply.manhattan_norm());
    VTR_LOGV(log_verbosity_ >= 20, "\t\tDemand of all paths from source: %.2f\n",
             starting_bin_supply.manhattan_norm());

    // Sort the paths in increasing order of cost.
    std::sort(paths.begin(), paths.end(), [&](const std::vector<FlatPlacementBinId>& a, const std::vector<FlatPlacementBinId>& b) {
        return bin_cost[a.back()] < bin_cost[b.back()];
    });

    return paths;
}

void FlowBasedLegalizer::flow_blocks_along_path(const std::vector<FlatPlacementBinId>& path,
                                                const PartialPlacement& p_placement,
                                                float psi) {
    // Get the root bin of the path.
    VTR_ASSERT(!path.empty());
    FlatPlacementBinId src_bin_id = path[0];
    // Create a stack and put the src bin on top.
    std::stack<FlatPlacementBinId> s;
    s.push(src_bin_id);
    // Insert the bins in the path into the stack in reverse order (so the last
    // bin in the path is on top of the stack).
    size_t path_size = path.size();
    for (size_t path_idx = 1; path_idx < path_size; path_idx++) {
        FlatPlacementBinId sink_bin_id = path[path_idx];
        // Check that the cost of moving a block from the source bin to the sink
        // bin is non-infinite. According to the paper, this check is needed
        // since a previous flow on another path may have made this path not
        // necessary anymore.
        float cost = compute_cost(src_bin_id, sink_bin_id, psi,
                                  p_placement, netlist_, *density_manager_);
        if (std::isinf(cost))
            return;
        src_bin_id = sink_bin_id;
        s.push(sink_bin_id);
    }
    // Congo line the blocks along the path, starting from the tail and moving
    // forward.
    FlatPlacementBinId sink_bin_id = s.top();
    s.pop();
    while (!s.empty()) {
        src_bin_id = s.top();
        s.pop();
        // Minor change to the algorithm proposed by Darav et al., find the
        // closest point in src to sink and move it to sink (instead of sorting
        // the whole list which is wasteful).
        // TODO: Verify this. This is not the same as what was in the original
        //       algorithm.
        std::pair<APBlockId, float> p = get_min_cost_block_in_bin(src_bin_id,
                                                                  sink_bin_id,
                                                                  p_placement,
                                                                  netlist_,
                                                                  *density_manager_);
        // Move the block from the src bin to the sink bin.
        density_manager_->remove_block_from_bin(p.first, src_bin_id);
        density_manager_->insert_block_into_bin(p.first, sink_bin_id);

        sink_bin_id = src_bin_id;
    }
}

/**
 * @brief Prints the header of the per-iteration status of the flow-based
 *        legalizer.
 */
static void print_flow_based_legalizer_status_header() {
    VTR_LOG("---- ----- ------- ---------\n");
    VTR_LOG("Iter   Num Largest       Psi\n");
    VTR_LOG("     Overf     Bin          \n");
    VTR_LOG("      Bins  Supply          \n");
    VTR_LOG("---- ----- ------- ---------\n");
}

/**
 * @brief Print the current status of the flow-based legalizer (per-iteration).
 */
static void print_flow_based_legalizer_status(size_t iteration,
                                              size_t num_overfilled_bins,
                                              float largest_overfilled_bin_supply,
                                              float psi) {
    // Iteration
    VTR_LOG("%4zu", iteration);

    // Num overfilled bins
    VTR_LOG(" %5zu", num_overfilled_bins);

    // Largest overfilled bin supply
    VTR_LOG(" %7.1f", largest_overfilled_bin_supply);

    // Psi
    VTR_LOG(" %9.3e", psi);

    VTR_LOG("\n");

    fflush(stdout);
}

void FlowBasedLegalizer::legalize(PartialPlacement& p_placement) {
    VTR_LOGV(log_verbosity_ >= 10, "Running Flow-Based Legalizer\n");

    // Import the partial placement into bins.
    density_manager_->import_placement_into_bins(p_placement);
    // Verify that the placement was imported correctly.
    VTR_ASSERT_SAFE(density_manager_->verify());

    // Print the number of blocks in each bin visually before spreading.
    if (log_verbosity_ >= 15) {
        VTR_LOG("Bin utilization prior to spreading:\n");
        density_manager_->print_bin_grid();
    }

    // Print the status header to make printing the status clearer.
    if (log_verbosity_ >= 10) {
        print_flow_based_legalizer_status_header();
    }

    // Run the flow-based spreader.
    size_t flowBasedIter = 0;
    while (true) {
        // If we hit the maximum number of iterations, break.
        if (flowBasedIter >= max_num_iterations_) {
            VTR_LOGV(log_verbosity_ >= 10,
                     "Flow-Based legalizer hit max iteration limit.\n");
            break;
        }
        // If there are no overfilled bins, no more work to do.
        const std::unordered_set<FlatPlacementBinId>& overfilled_bins = density_manager_->get_overfilled_bins();
        if (overfilled_bins.empty()) {
            VTR_LOGV(log_verbosity_ >= 10,
                     "Flow-Based legalizer has no overfilled tiles. No further spreading needed.\n");
            break;
        }
        // Compute the max movement.
        double psi = computeMaxMovement(flowBasedIter);
        // Get the overfilled bins and sort them in increasing order of supply.
        // We take the manhattan (L1) norm here since we only care about the total
        // amount of overfill in each dimension. For example, a bin that has a
        // supply of <1, 1> is just as overfilled as a bin of supply <0, 2>.
        // The standard L2 norm would give more weight to <0, 2>.
        // NOTE: Although the supply should always be non-negative, we still
        //       take the absolute value in the norm for completeness.
        // TODO: This is a guess. Should investigate other norms.
        std::vector<FlatPlacementBinId> overfilled_bins_vec(overfilled_bins.begin(), overfilled_bins.end());
        std::sort(overfilled_bins_vec.begin(), overfilled_bins_vec.end(), [&](FlatPlacementBinId a, FlatPlacementBinId b) {
            return get_bin_supply(a).manhattan_norm() < get_bin_supply(b).manhattan_norm();
        });
        // Get the paths to flow blocks from the overfilled bins to the under
        // filled bins and flow the blocks.
        for (FlatPlacementBinId src_bin_id : overfilled_bins_vec) {
            // Get the list of candidate paths based on psi. A path is a list
            // of LegalizerBins traversed.
            //  NOTE: The paths are sorted by increasing cost within the
            //        getPaths method.
            std::vector<std::vector<FlatPlacementBinId>> paths = get_paths(src_bin_id,
                                                                           p_placement,
                                                                           psi);

            VTR_LOGV(log_verbosity_ >= 20, "\tNum paths: %zu\n", paths.size());
            // For each path, flow the blocks along the path.
            for (const std::vector<FlatPlacementBinId>& path : paths) {
                VTR_LOGV(log_verbosity_ >= 30, "\t\tPath length: %zu\n", path.size());
                // If the bin is no longer overfilled, no need to move any more
                // blocks along the paths.
                if (!density_manager_->bin_is_overfilled(src_bin_id))
                    break;
                // Move blocks over the paths.
                //  NOTE: This will only modify the bins. (actual block
                //        positions will not change (yet)).
                flow_blocks_along_path(path, p_placement, psi);
            }
        }

        // Print status of the flow based legalizer for debugging.
        if (log_verbosity_ >= 10) {
            // TODO: Get the total cell displacement for debugging.
            print_flow_based_legalizer_status(
                flowBasedIter,
                overfilled_bins_vec.size(),
                get_bin_supply(overfilled_bins_vec.back()).manhattan_norm(),
                psi);
        }

        // Increment the iteration.
        flowBasedIter++;
    }
    VTR_LOGV(log_verbosity_ >= 10,
             "Flow-Based Legalizer finished in %zu iterations.\n",
             flowBasedIter + 1);

    // Verify that the bins are valid before export.
    VTR_ASSERT(verify());

    // Print the number of blocks in each bin after spreading.
    if (log_verbosity_ >= 15) {
        VTR_LOG("Bin utilization after spreading:\n");
        density_manager_->print_bin_grid();
    }

    // Export the legalized placement to the partial placement.
    density_manager_->export_placement_from_bins(p_placement);
}

PerPrimitiveDimPrefixSum2D::PerPrimitiveDimPrefixSum2D(const FlatPlacementDensityManager& density_manager,
                                                       std::function<float(PrimitiveVectorDim, size_t, size_t, size_t)> lookup) {
    // Get the size that the prefix sums should be.
    size_t width, height, num_layers;
    std::tie(width, height, num_layers) = density_manager.get_overall_placeable_region_size();

    // Create each of the prefix sums.
    const PrimitiveDimManager& dim_manager = density_manager.mass_calculator().get_dim_manager();
    layer_dim_prefix_sum_.resize(num_layers);
    for (size_t layer = 0; layer < num_layers; layer++) {
        layer_dim_prefix_sum_[layer].resize(dim_manager.dims().size());
        for (PrimitiveVectorDim dim : density_manager.get_used_dims_mask().get_non_zero_dims()) {
            layer_dim_prefix_sum_[layer][dim] = vtr::PrefixSum2D<uint64_t>(
                width,
                height,
                [&](size_t x, size_t y) {
                    // Convert the floating point value into fixed point to prevent
                    // error accumulation in the prefix sum.
                    // Note: We ceil here since we do not want to lose information
                    //       on numbers that get very close to 0.
                    float val = lookup(dim, layer, x, y);
                    VTR_ASSERT_SAFE_MSG(val >= 0.0f,
                                        "PerPrimitiveDimPrefixSum2D expected to only hold positive values");
                    return std::ceil(val * fractional_scale_);
                });
        }
    }
}

float PerPrimitiveDimPrefixSum2D::get_dim_sum(PrimitiveVectorDim dim,
                                              const vtr::Rect<double>& region,
                                              size_t layer) const {
    VTR_ASSERT_SAFE(dim.is_valid());
    // Get the sum over the given region.
    uint64_t sum = layer_dim_prefix_sum_[layer][dim].get_sum(region.xmin(),
                                                             region.ymin(),
                                                             region.xmax() - 1,
                                                             region.ymax() - 1);

    // The sum is stored as a fixed point number. Cast into float by casting to
    // a float and dividing by the fractional scale.
    return static_cast<float>(sum) / fractional_scale_;
}

PrimitiveVector PerPrimitiveDimPrefixSum2D::get_sum(const std::vector<PrimitiveVectorDim>& dims,
                                                    const vtr::Rect<double>& region,
                                                    size_t layer) const {
    PrimitiveVector res;
    for (PrimitiveVectorDim dim : dims) {
        VTR_ASSERT_SAFE(res.get_dim_val(dim) == 0.0f);
        res.set_dim_val(dim, get_dim_sum(dim, region, layer));
    }
    return res;
}

PrimitiveDimGrouper::PrimitiveDimGrouper(const Prepacker& prepacker,
                                         const LogicalModels& models,
                                         const FlatPlacementDensityManager& density_manager,
                                         const PrimitiveDimManager& dim_manager,
                                         int log_verbosity)
    : model_grouper_(prepacker, models, log_verbosity) {

    // Models are grouped together by the model grouper based on the pack
    // patterns provided by the architecture. Different models may be mapped to
    // the same primitive dim. As such, we need to perform another grouping in
    // a similar manner to group together the dims which must be spread together.
    //
    // We ignore unused dims to prevent them from being used in the spreading
    // algorithm. Since they are unused, we do not put them into groups.

    // Create an adjacency list connecting dimensions together which share models
    // that are grouped together.
    size_t num_dims = dim_manager.dims().size();
    vtr::vector<PrimitiveVectorDim, std::unordered_set<PrimitiveVectorDim>> adj_list(num_dims);
    const PrimitiveVector& used_dims_mask = density_manager.get_used_dims_mask();
    for (ModelGroupId group_id : model_grouper_.groups()) {
        // Collect all of the models in this group.
        const auto& models_in_group = model_grouper_.get_models_in_group(group_id);

        // Collect all of the used dimensions of the models in this group.
        std::unordered_set<PrimitiveVectorDim> dims_in_group;
        for (LogicalModelId model_id : models_in_group) {
            PrimitiveVectorDim dim = dim_manager.get_model_dim(model_id);
            // If this dim is unused, skip.
            if (used_dims_mask.get_dim_val(dim) == 0)
                continue;

            dims_in_group.insert(dim);
        }

        // If this group is empty (i.e. all dims are unused), pass.
        if (dims_in_group.empty())
            continue;

        // Create a bidirectional edge between the first dim and all other
        // dims in the group.
        PrimitiveVectorDim first_dim = *dims_in_group.begin();
        for (PrimitiveVectorDim dim : dims_in_group) {
            adj_list[dim].insert(first_dim);
            adj_list[first_dim].insert(dim);
        }
    }

    // Perform BFS to group the dims. This BFS will traverse all dims that are
    // connected to each other and put them into groups. Dims which have no
    // path between another dim will not be grouped together.
    std::queue<PrimitiveVectorDim> node_queue;
    dim_group_id_.resize(num_dims, PrimitiveGroupId::INVALID());
    for (PrimitiveVectorDim dim : dim_manager.dims()) {
        // If this dim is unused, skip it.
        // TODO: Maybe put unused dims into a special group.
        if (used_dims_mask.get_dim_val(dim) == 0)
            continue;

        // If this dim is already in a group, skip it.
        if (dim_group_id_[dim].is_valid()) {
            continue;
        }

        // Create a new group ID and put this dim in that group.
        PrimitiveGroupId group_id = PrimitiveGroupId(group_ids_.size());
        dim_group_id_[dim] = group_id;
        // Put this dim into the BFS queue to explore its neighbors.
        node_queue.push(dim);

        while (!node_queue.empty()) {
            // Pop the dim from the queue and explore its neighbors.
            PrimitiveVectorDim node_dim = node_queue.front();
            node_queue.pop();
            for (PrimitiveVectorDim neighbor_dim : adj_list[node_dim]) {
                // If this neighbor dim is already in the group, skip it.
                if (dim_group_id_[neighbor_dim].is_valid()) {
                    VTR_ASSERT_SAFE(dim_group_id_[neighbor_dim] == group_id);
                    continue;
                }
                // Put the neighbor in this group and push it to the queue.
                dim_group_id_[neighbor_dim] = group_id;
                node_queue.push(neighbor_dim);
            }
        }

        // Add this group to the list of all groups.
        group_ids_.push_back(group_id);
    }

    // Create a lookup between each group and the dims it contains.
    groups_.resize(groups().size());
    for (PrimitiveVectorDim dim : dim_manager.dims()) {
        // If this dim is unused, skip it.
        if (!dim_group_id_[dim].is_valid())
            continue;
        groups_[dim_group_id_[dim]].push_back(dim);
    }
}

BiPartitioningPartialLegalizer::BiPartitioningPartialLegalizer(
    const APNetlist& netlist,
    std::shared_ptr<FlatPlacementDensityManager> density_manager,
    const Prepacker& prepacker,
    const LogicalModels& models,
    int log_verbosity)
    : PartialLegalizer(netlist, log_verbosity)
    , density_manager_(density_manager)
    , dim_grouper_(prepacker,
                   models,
                   *density_manager,
                   density_manager->mass_calculator().get_dim_manager(),
                   log_verbosity) {
    // Compute the capacity prefix sum. Capacity is assumed to not change
    // between iterations of the partial legalizer.
    capacity_prefix_sum_ = PerPrimitiveDimPrefixSum2D(
        *density_manager,
        [&](PrimitiveVectorDim dim, size_t layer, size_t x, size_t y) {
            // Get the bin at this grid location.
            FlatPlacementBinId bin_id = density_manager_->get_bin(x, y, layer);
            // Get the capacity of the bin for this dim.
            float cap = density_manager_->get_bin_capacity(bin_id).get_dim_val(dim);
            VTR_ASSERT_SAFE(cap >= 0.0f);

            // Update the capacity with the target density. By multiplying by the
            // target density, we make the capacity appear smaller than it actually
            // is during partial legalization.
            float target_density = density_manager_->get_bin_target_density(bin_id);
            cap *= target_density;

            // Bins may be large, but the prefix sum assumes a 1x1 grid of
            // values. Normalize by the area of the bin to turn this into
            // a 1x1 bin equivalent.
            const vtr::Rect<double>& bin_region = density_manager_->flat_placement_bins().bin_region(bin_id);
            float bin_area = bin_region.width() * bin_region.height();
            VTR_ASSERT_SAFE(!vtr::isclose(bin_area, 0.0f));
            cap /= bin_area;

            return cap;
        });

    num_windows_partitioned_ = 0;
    num_blocks_partitioned_ = 0;
}

void BiPartitioningPartialLegalizer::print_statistics() {
    VTR_LOG("Bi-Partitioning Partial Legalizer Statistics:\n");
    VTR_LOG("\tTotal number of windows partitioned: %u\n", num_windows_partitioned_);
    VTR_LOG("\tTotal number of blocks partitioned: %u\n", num_blocks_partitioned_);
}

void BiPartitioningPartialLegalizer::legalize(PartialPlacement& p_placement) {
    VTR_LOGV(log_verbosity_ >= 10, "Running Bi-Partitioning Legalizer\n");

    // Prepare the density manager.
    density_manager_->import_placement_into_bins(p_placement);

    // Quick return. If there are no overfilled bins, there is nothing to spread.
    if (density_manager_->get_overfilled_bins().size() == 0) {
        VTR_LOGV(log_verbosity_ >= 10, "No overfilled bins. Nothing to legalize.\n");
        return;
    }

    if (log_verbosity_ >= 10) {
        size_t num_overfilled_bins = density_manager_->get_overfilled_bins().size();
        VTR_LOG("\tNumber of overfilled blocks before legalization: %zu\n",
                num_overfilled_bins);
        // FIXME: Make this a method in the density manager class.
        float avg_overfill = 0.f;
        for (FlatPlacementBinId overfilled_bin_id : density_manager_->get_overfilled_bins()) {
            avg_overfill += density_manager_->get_bin_overfill(overfilled_bin_id).manhattan_norm();
        }
        VTR_LOG("\t\tAverage overfill per overfilled bin: %f\n",
                avg_overfill / static_cast<float>(num_overfilled_bins));
    }

    // 1) Identify the groups that need to be spread
    std::unordered_set<PrimitiveGroupId> groups_to_spread;
    for (FlatPlacementBinId overfilled_bin_id : density_manager_->get_overfilled_bins()) {
        // Get the overfilled dims in this bin.
        const PrimitiveVector& overfill = density_manager_->get_bin_overfill(overfilled_bin_id);
        std::vector<PrimitiveVectorDim> overfilled_dims = overfill.get_non_zero_dims();
        // For each dim, insert its group into the set. Set will handle dupes.
        for (PrimitiveVectorDim dim : overfilled_dims) {
            groups_to_spread.insert(dim_grouper_.get_dim_group_id(dim));
        }
    }

    // 2) For each group, identify non-overlapping windows and spread
    vtr::Timer runtime_timer;
    float window_identification_time = 0.0f;
    float window_spreading_time = 0.0f;
    for (PrimitiveGroupId group_id : groups_to_spread) {
        VTR_LOGV(log_verbosity_ >= 10, "\tSpreading group %zu\n", group_id);
        // Identify non-overlapping spreading windows.
        float window_identification_start_time = runtime_timer.elapsed_sec();
        auto non_overlapping_windows = identify_non_overlapping_windows(group_id);
        window_identification_time += runtime_timer.elapsed_sec() - window_identification_start_time;
        VTR_ASSERT(non_overlapping_windows.size() != 0);

        // Spread the blocks over the non-overlapping windows.
        float window_spreading_start_time = runtime_timer.elapsed_sec();
        spread_over_windows(non_overlapping_windows, p_placement, group_id);
        window_spreading_time += runtime_timer.elapsed_sec() - window_spreading_start_time;
    }

    // FIXME: Remove this duplicate code...
    if (log_verbosity_ >= 10) {
        size_t num_overfilled_bins = density_manager_->get_overfilled_bins().size();
        VTR_LOG("\tNumber of overfilled blocks after legalization: %zu\n",
                num_overfilled_bins);
        // FIXME: Make this a method in the density manager class.
        float avg_overfill = 0.f;
        for (FlatPlacementBinId overfilled_bin_id : density_manager_->get_overfilled_bins()) {
            avg_overfill += density_manager_->get_bin_overfill(overfilled_bin_id).manhattan_norm();
        }
        VTR_LOG("\t\tAverage overfill per overfilled bin: %f\n",
                avg_overfill / static_cast<float>(num_overfilled_bins));
        VTR_LOG("\tTime spent identifying windows: %g\n", window_identification_time);
        VTR_LOG("\tTime spent spreading windows: %g\n", window_spreading_time);
    }

    // Export the legalized placement to the partial placement.
    density_manager_->export_placement_from_bins(p_placement);
}

std::vector<SpreadingWindow> BiPartitioningPartialLegalizer::identify_non_overlapping_windows(PrimitiveGroupId group_id) {

    // 1) Cluster the overfilled bins. This will make creating minimum spanning
    //    windows more efficient.
    auto overfilled_bin_clusters = get_overfilled_bin_clusters(group_id);

    // 2) For each of the overfilled bin clusters, create a minimum window such
    //    that there is enough space in the window for the atoms inside.
    auto windows = get_min_windows_around_clusters(overfilled_bin_clusters, group_id);

    // 3) Merge overlapping windows.
    merge_overlapping_windows(windows);

    // TODO: Investigate shrinking the windows.

    // 4) Move the blocks out of their bins and into the windows.
    move_blocks_into_windows(windows, group_id);

    return windows;
}

/**
 * @brief Helper method to check if the given PrimitiveVector has any values
 *        in the dim dimensions in the given group.
 *
 * This method assumes the vector is non-negative. If the vector had any negative
 * dimensions, it does not make sense to ask if it is in the group or not.
 */
static bool is_vector_in_group(const PrimitiveVector& vec,
                               PrimitiveGroupId group_id,
                               const PrimitiveDimGrouper& dim_grouper) {
    VTR_ASSERT_SAFE(vec.is_non_negative());
    const std::vector<PrimitiveVectorDim>& dims_in_group = dim_grouper.get_dims_in_group(group_id);
    for (PrimitiveVectorDim dim : dims_in_group) {
        float dim_val = vec.get_dim_val(dim);
        if (dim_val != 0.0f)
            return true;
    }
    return false;
}

/**
 * @brief Checks if the overfilled dims in the given overfilled bin is in the
 *        given dim group.
 *
 * This method does not check if the bin could be in the given group (for
 * example the capacity), this checks if the overfilled blocks are in the group.
 */
static bool is_overfilled_bin_in_group(FlatPlacementBinId overfilled_bin_id,
                                       PrimitiveGroupId group_id,
                                       const FlatPlacementDensityManager& density_manager,
                                       const PrimitiveDimGrouper& dim_grouper) {
    const PrimitiveVector& bin_overfill = density_manager.get_bin_overfill(overfilled_bin_id);
    VTR_ASSERT_SAFE(bin_overfill.is_non_zero());
    return is_vector_in_group(bin_overfill, group_id, dim_grouper);
}

/**
 * @brief Checks if the given AP block is in the given dim group.
 *
 * An AP block is in a dim group if it contains any dims in the dim group.
 */
static bool is_block_in_group(APBlockId blk_id,
                              PrimitiveGroupId group_id,
                              const FlatPlacementDensityManager& density_manager,
                              const PrimitiveDimGrouper& dim_grouper) {
    const PrimitiveVector& blk_mass = density_manager.mass_calculator().get_block_mass(blk_id);
    return is_vector_in_group(blk_mass, group_id, dim_grouper);
}

std::vector<FlatPlacementBinCluster> BiPartitioningPartialLegalizer::get_overfilled_bin_clusters(
    PrimitiveGroupId group_id) {
    // Use BFS over the overfilled bins to cluster them.
    std::vector<FlatPlacementBinCluster> overfilled_bin_clusters;
    // Maintain the distance from the last overfilled bin
    vtr::vector<FlatPlacementBinId, int> dist(density_manager_->flat_placement_bins().bins().size(), -1);
    for (FlatPlacementBinId overfilled_bin_id : density_manager_->get_overfilled_bins()) {
        // If this bin is not overfilled with the dims in the group, skip.
        if (!is_overfilled_bin_in_group(overfilled_bin_id,
                                        group_id,
                                        *density_manager_,
                                        dim_grouper_)) {
            continue;
        }
        // If this bin is already in a cluster, skip.
        if (dist[overfilled_bin_id] != -1)
            continue;
        dist[overfilled_bin_id] = 0;
        // Collect nearby bins into a vector.
        FlatPlacementBinCluster nearby_bins;
        nearby_bins.push_back(overfilled_bin_id);
        // Create a queue and insert the overfilled bin into it.
        std::queue<FlatPlacementBinId> bin_queue;
        bin_queue.push(overfilled_bin_id);
        while (!bin_queue.empty()) {
            // Pop a bin from queue.
            FlatPlacementBinId bin_node = bin_queue.front();
            bin_queue.pop();
            // If the node's distance from an overfilled bin is the max gap,
            // do not explore its neighbors.
            if (dist[bin_node] > max_bin_cluster_gap_)
                continue;
            // Explore the neighbors of this bin.
            for (FlatPlacementBinId neighbor : get_direct_neighbors_of_bin(bin_node, *density_manager_)) {
                int neighbor_dist = dist[bin_node] + 1;
                // If this neighbor has been explore with a better distance,
                // do not explore it.
                if (dist[neighbor] != -1 && dist[neighbor] <= neighbor_dist)
                    continue;
                // If the neighbor is an overfilled bin that we care about, add
                // it to the list of nearby bins and set its distance to 0.
                if (density_manager_->bin_is_overfilled(neighbor)
                    && is_overfilled_bin_in_group(neighbor, group_id, *density_manager_, dim_grouper_)) {
                    nearby_bins.push_back(neighbor);
                    dist[neighbor] = 0;
                } else {
                    dist[neighbor] = neighbor_dist;
                }
                // Enqueue the neighbor.
                bin_queue.push(neighbor);
            }
        }

        // Move the cluster into the vector of overfilled bin clusters.
        overfilled_bin_clusters.push_back(std::move(nearby_bins));
    }

    return overfilled_bin_clusters;
}

/**
 * @brief Helper method to decide if the given region's utilization is higher
 *        than its capacity.
 */
static bool is_region_overfilled(const vtr::Rect<double>& region,
                                 const PerPrimitiveDimPrefixSum2D& capacity_prefix_sum,
                                 const PerPrimitiveDimPrefixSum2D& utilization_prefix_sum,
                                 const std::vector<PrimitiveVectorDim>& dims,
                                 size_t layer_low,
                                 size_t layer_high) {
    for (size_t layer = layer_low; layer <= layer_high; layer++) {
        // Go through each dim in the dim group we are interested in.
        for (PrimitiveVectorDim dim : dims) {
            // Get the capacity of this region for this dim.
            float region_dim_capacity = capacity_prefix_sum.get_dim_sum(dim,
                                                                        region,
                                                                        layer);
            // Get the utilization of this region for this dim.
            float region_dim_utilization = utilization_prefix_sum.get_dim_sum(dim,
                                                                              region,
                                                                              layer);
            // If the utilization is higher than the capacity, then this region is
            // overfilled.
            // TODO: Look into adding some head room to account for rounding.
            if (region_dim_utilization > region_dim_capacity)
                return true;
        }
    }

    // If the utilization is less than or equal to the capacity for each dim
    // then this region is not overfilled.
    return false;
}

std::vector<SpreadingWindow> BiPartitioningPartialLegalizer::get_min_windows_around_clusters(
    const std::vector<FlatPlacementBinCluster>& overfilled_bin_clusters,
    PrimitiveGroupId group_id) {
    // TODO: Currently, we greedily grow the region by 1 in all directions until
    //       the capacity is larger than the utilization. This may not produce
    //       the minimum window. Should investigate "touching-up" the windows.
    // FIXME: It may be a good idea to sort the bins by their overfill here. Then
    //        we can check for overlap as we go.

    // Get the width, height, and number of layers for the spreading region.
    // This is used by the growing part of this routine to prevent the windows
    // from outgrowing the device.
    size_t width, height, num_layers;
    std::tie(width, height, num_layers) = density_manager_->get_overall_placeable_region_size();

    // Precompute a prefix sum for the current utilization of each 1x1 region
    // of the device. This needs to be recomputed every time the bins are
    // modified, so it is recomputed here.
    PerPrimitiveDimPrefixSum2D utilization_prefix_sum(
        *density_manager_,
        [&](PrimitiveVectorDim dim, size_t layer, size_t x, size_t y) {
            FlatPlacementBinId bin_id = density_manager_->get_bin(x, y, layer);
            // This is computed the same way as the capacity prefix sum above.
            const vtr::Rect<double>& bin_region = density_manager_->flat_placement_bins().bin_region(bin_id);
            float bin_area = bin_region.width() * bin_region.height();
            float util = density_manager_->get_bin_utilization(bin_id).get_dim_val(dim);
            VTR_ASSERT_SAFE(util >= 0.0f);
            return util / bin_area;
        });

    // Create windows for each overfilled bin cluster.
    std::vector<SpreadingWindow> windows;
    for (const std::vector<FlatPlacementBinId>& overfilled_bin_cluster : overfilled_bin_clusters) {
        // Create a new window for this cluster of bins.
        SpreadingWindow new_window;

        // Set the region of the window to the bounding box of the cluster of bins.
        size_t num_bins_in_cluster = overfilled_bin_cluster.size();
        VTR_ASSERT_SAFE(num_bins_in_cluster != 0);
        vtr::Rect<double>& region = new_window.region;
        region = density_manager_->flat_placement_bins().bin_region(overfilled_bin_cluster[0]);
        new_window.layer_low = density_manager_->flat_placement_bins().bin_layer(overfilled_bin_cluster[0]);
        new_window.layer_high = density_manager_->flat_placement_bins().bin_layer(overfilled_bin_cluster[0]);
        for (size_t i = 1; i < num_bins_in_cluster; i++) {
            region = vtr::bounding_box(region,
                                       density_manager_->flat_placement_bins().bin_region(overfilled_bin_cluster[i]));
            new_window.layer_low = std::min(new_window.layer_low, density_manager_->flat_placement_bins().bin_layer(overfilled_bin_cluster[i]));
            new_window.layer_high = std::min(new_window.layer_high, density_manager_->flat_placement_bins().bin_layer(overfilled_bin_cluster[i]));
        }

        // Grow the region until it is just large enough to not overfill
        while (true) {
            // Grow the region by 1 on all sides.
            double new_xmin = std::clamp<double>(region.xmin() - 1.0, 0.0, width);
            double new_xmax = std::clamp<double>(region.xmax() + 1.0, 0.0, width);
            double new_ymin = std::clamp<double>(region.ymin() - 1.0, 0.0, height);
            double new_ymax = std::clamp<double>(region.ymax() + 1.0, 0.0, height);
            // TODO: This may not be the best idea for layers. We may want to
            //       heuristically grow the layers only if we think that growing
            //       the region would not be beneficial.
            size_t new_layer_low = new_window.layer_low == 0 ? 0 : new_window.layer_low - 1;
            VTR_ASSERT_SAFE(num_layers > 0);
            size_t new_layer_high = std::clamp<size_t>(new_window.layer_high + 1, 0, num_layers - 1);

            // If the region did not grow, exit. This is a maximal bin.
            // TODO: Maybe print warning.
            if (new_xmin == region.xmin() &&
                new_xmax == region.xmax() &&
                new_ymin == region.ymin() &&
                new_ymax == region.ymax() &&
                new_layer_low == new_window.layer_low &&
                new_layer_high == new_window.layer_high)
            {
                break;
            }

            region.set_xmin(new_xmin);
            region.set_xmax(new_xmax);
            region.set_ymin(new_ymin);
            region.set_ymax(new_ymax);
            new_window.layer_low = new_layer_low;
            new_window.layer_high = new_layer_high;

            // If the region is no longer overfilled, stop growing.
            if (!is_region_overfilled(region, capacity_prefix_sum_, utilization_prefix_sum, dim_grouper_.get_dims_in_group(group_id), new_layer_low, new_layer_high))
                break;
        }
        // Insert this window into the list of windows.
        windows.emplace_back(std::move(new_window));
    }

    return windows;
}

void BiPartitioningPartialLegalizer::merge_overlapping_windows(
    std::vector<SpreadingWindow>& windows) {

    // Within the spatial lookup map, we want to store the start and end of the
    // region that contains the window. However, we do not want to store the
    // boundary of that region since we are ok with two windows touching at their
    // boundaries. Hence, we pad a small epsilon around the region.
    // NOTE: This number should be smaller than the units used for the windows.
    //       For example, if the windows are placed over grid-tile locations, this
    //       just needs to be smaller than 1.
    double epsilon = 0.0001;

    // Create a spatial lookup between the x-position of the left / right side of
    // each window and the ID of that window. Here we use the index of the window
    // in the windows vector as an ID.
    // TODO: The choice of x here is arbitrary; need to investigate better
    //       multi-dimensional lookup structures.
    std::multimap<double, size_t> x_sorted_windows;
    size_t num_windows = windows.size();
    for (size_t i = 0; i < num_windows; i++) {
        // Add the lower and upper bound of the window into the map. Add a small
        // epsilon to exclude the border of the window. We do not care if two
        // windows share a border, only if they overlap.
        // NOTE: This will add duplicates; however it ensures that the overlapping
        //       region is found in the lower algorithm.
        const SpreadingWindow& window = windows[i];
        x_sorted_windows.insert(std::make_pair(window.region.xmin() + epsilon, i));
        x_sorted_windows.insert(std::make_pair(window.region.xmax() - epsilon, i));
    }

    // Merge overlapping windows.
    // TODO: This is a very basic merging process which will identify the
    //       minimum region containing both windows; however, after merging it
    //       is very likely that this window will now be too large. Need to
    //       investigate shrinking the windows after merging.
    // FIXME: This loop is O(N^2) with the number of overfilled bins which may
    //        get expensive as the circuit sizes increase. Should investigate
    //        spatial sorting structures (like kd-trees) to help keep this fast.
    //        Another idea is to merge windows early on (before growing them).
    // Need to keep track of which windows have been merged or not to prevent
    // merging windows multiple times.
    std::vector<bool> merged_window(num_windows, false);
    for (size_t i = 0; i < num_windows; i++) {
        // If this window has been merged into another window, do not explore it.
        if (merged_window[i])
            continue;

        // After merging with another window, the bounds of the window is updated
        // and another window may now become merged. We need to recheck again after
        // this occurs.
        while (true) {
            // Get the lower and upper bounds in the spatial lookup. This will
            // give a list of all windows which could possibly overlap right now
            // (at least in the x dimension).
            // NOTE: There may be duplicates; but that's ok since these checks
            //       are fast.
            auto lower = x_sorted_windows.lower_bound(windows[i].region.xmin());
            auto upper = x_sorted_windows.upper_bound(windows[i].region.xmax());

            // Go through the windows which may overlap with this window.
            bool recheck_for_overlap = false;
            for (auto it = lower; it != upper;) {
                size_t other_window_idx = it->second;

                // If we find the window we are exploring, remove it from the map.
                // We will re-add it after it is explored to update it since its
                // bounds would have changed.
                if (other_window_idx == i) {
                    it = x_sorted_windows.erase(it);
                    continue;
                }

                // If this window has already been merged do not check it, we want
                // to check the root merging window.
                // Erase this window too. There is no reason to keep it in the map.
                if (merged_window[other_window_idx]) {
                    it = x_sorted_windows.erase(it);
                    continue;
                }

                // Check for overlap
                if (windows[i].overlaps(windows[other_window_idx])) {
                    // If overlap, merge with this region and mark the window as
                    // finished.
                    // Here, the merged region is the bounding box around the two
                    // regions.
                    double old_window_area = windows[i].window_area();
                    windows[i].merge_window_area(windows[other_window_idx]);
                    merged_window[other_window_idx] = true;

                    // If the area of the region increased, this means that the
                    // window may overlap new windows. Thus we need to recheck.
                    double new_window_area = windows[i].window_area();
                    if (old_window_area != new_window_area) {
                        recheck_for_overlap = true;
                        break;
                    }

                    // After merging the window, it no longer needs to be in the map.
                    it = x_sorted_windows.erase(it);
                    continue;
                }

                // Increment the iterator if we get here.
                ++it;
            }

            // If we do not have to recheck for overlap, we can break.
            if (!recheck_for_overlap) {
                break;
            }
        }

        // Update the positional map with the new bounds of this window.
        // This will be used by future explored windows to check for overlap.
        x_sorted_windows.insert(std::make_pair(windows[i].region.xmin() + epsilon, i));
        x_sorted_windows.insert(std::make_pair(windows[i].region.xmax() - epsilon, i));
    }

    // Collect the windows that were not merged into other windows.
    std::vector<SpreadingWindow> non_overlapping_windows;
    non_overlapping_windows.reserve(num_windows);
    for (size_t i = 0; i < num_windows; i++) {
        if (merged_window[i])
            continue;
        non_overlapping_windows.emplace_back(std::move(windows[i]));
    }

    // Store the results into the input window.
    windows = std::move(non_overlapping_windows);
}

void BiPartitioningPartialLegalizer::move_blocks_into_windows(
    std::vector<SpreadingWindow>& non_overlapping_windows,
    PrimitiveGroupId group_id) {
    // Move the blocks from their bins into the windows that should contain them.
    // TODO: It may be good for debugging to check if the windows have nothing
    //       to move. This may indicate a problem (overfilled bins of fixed
    //       blocks, overlapping windows, etc layer.
    for (SpreadingWindow& window : non_overlapping_windows) {
        // Iterate over all bins that this window covers.
        // TODO: This is a bit crude and should somehow be made more robust.
        size_t lower_x = window.region.xmin();
        size_t upper_x = window.region.xmax() - 1;
        size_t lower_y = window.region.ymin();
        size_t upper_y = window.region.ymax() - 1;
        size_t layer_low = window.layer_low;
        size_t layer_high = window.layer_high;
        for (size_t layer = layer_low; layer <= layer_high; layer++) {
            for (size_t x = lower_x; x <= upper_x; x++) {
                for (size_t y = lower_y; y <= upper_y; y++) {
                    // Get all of the movable blocks from the bin.
                    std::vector<APBlockId> moveable_blks;
                    FlatPlacementBinId bin_id = density_manager_->get_bin(x, y, layer);
                    const auto& bin_contained_blocks = density_manager_->flat_placement_bins().bin_contained_blocks(bin_id);
                    moveable_blks.reserve(bin_contained_blocks.size());
                    for (APBlockId blk_id : bin_contained_blocks) {
                        // If this block is not moveable, do not move it.
                        if (netlist_.block_mobility(blk_id) != APBlockMobility::MOVEABLE)
                            continue;
                        // If this block is not in the group, do not move it.
                        if (!is_block_in_group(blk_id, group_id, *density_manager_, dim_grouper_))
                            continue;

                        moveable_blks.push_back(blk_id);
                    }
                    // Remove the moveable blocks from their bins and store into
                    // the windows.
                    for (APBlockId blk_id : moveable_blks) {
                        density_manager_->remove_block_from_bin(blk_id, bin_id);
                        window.contained_blocks.push_back(blk_id);
                    }
                }
            }
        }
    }
}

void BiPartitioningPartialLegalizer::spread_over_windows(std::vector<SpreadingWindow>& non_overlapping_windows,
                                                         const PartialPlacement& p_placement,
                                                         PrimitiveGroupId group_id) {
    if (log_verbosity_ >= 10) {
        VTR_LOG("\tIdentified %zu non-overlapping spreading windows.\n",
                non_overlapping_windows.size());

        if (log_verbosity_ >= 20) {
            for (const SpreadingWindow& window : non_overlapping_windows) {
                VTR_LOG("\t\t[(%.1f, %.1f), (%.1f, %.1f)]\n",
                        window.region.xmin(), window.region.ymin(),
                        window.region.xmax(), window.region.ymax());
                PrimitiveVector window_capacity;
                for (size_t layer = window.layer_low; layer <= window.layer_high; layer++) {
                    window_capacity += capacity_prefix_sum_.get_sum(dim_grouper_.get_dims_in_group(group_id),
                                                                    window.region,
                                                                    layer);
                }
                PrimitiveVector window_mass;
                for (APBlockId blk_id : window.contained_blocks) {
                    const PrimitiveVector& blk_mass = density_manager_->mass_calculator().get_block_mass(blk_id);
                    window_mass += blk_mass;
                }
                VTR_LOG("\t\t\tCapacity: %f\n",
                        window_capacity.manhattan_norm());
                VTR_LOG("\t\t\tMass: %f\n",
                        window_mass.manhattan_norm());
                VTR_LOG("\t\t\tNumber of contained blocks: %zu\n",
                        window.contained_blocks.size());
            }
        }
    }

    // Insert the windows into a queue for spreading.
    std::queue<SpreadingWindow> window_queue;
    for (SpreadingWindow& window : non_overlapping_windows) {
        window_queue.push(std::move(window));
    }

    // For each window in the queue:
    //      1) If the window is small enough, do not partition further.
    //      2) Partition the window
    //      3) Partition the blocks into the window partitions
    //      4) Insert the new windows into the queue
    std::vector<SpreadingWindow> finished_windows;
    while (!window_queue.empty()) {
        // Get a reference to the front of the queue but do not pop it yet. We
        // can save time from having to copy the element out since these windows
        // contain vectors.
        SpreadingWindow& window = window_queue.front();

        // Check if the window is empty. This can happen when there is odd
        // numbers of blocks or when things do not perfectly fit.
        if (window.contained_blocks.empty()) {
            // If the window does not contain any blocks, pop it from the queue
            // and do not put it in finished windows. There is no point
            // operating on it further.
            window_queue.pop();
            continue;
        }

        // 1) Check if the window is small enough (one bin in size).
        // TODO: Perhaps we can make this stopping criteria more intelligent.
        //       Like stopping when we know there is only one bin within the
        //       window.
        double window_area = window.window_area();
        if (window_area <= 1.0) {
            finished_windows.emplace_back(std::move(window));
            window_queue.pop();
            continue;
        }

        num_windows_partitioned_++;
        num_blocks_partitioned_ += window.contained_blocks.size();

        // 2) Partition the window.
        auto partitioned_window = partition_window(window, group_id);

        // 3) Partition the blocks.
        partition_blocks_in_window(window, partitioned_window, group_id, p_placement);

        // 4) Enqueue the new windows.
        window_queue.push(std::move(partitioned_window.lower_window));
        window_queue.push(std::move(partitioned_window.upper_window));

        // Pop the top element off the queue. This will invalidate the window
        // object.
        window_queue.pop();
    }

    if (log_verbosity_ >= 10) {
        VTR_LOG("\t%zu finalized windows.\n",
                finished_windows.size());

        if (log_verbosity_ >= 30) {
            for (const SpreadingWindow& window : finished_windows) {
                VTR_LOG("\t\t[(%.1f, %.1f), (%.1f, %.1f)]\n",
                        window.region.xmin(), window.region.ymin(),
                        window.region.xmax(), window.region.ymax());
                PrimitiveVector window_capacity;
                for (size_t layer = window.layer_low; layer <= window.layer_high; layer++) {
                    window_capacity += capacity_prefix_sum_.get_sum(dim_grouper_.get_dims_in_group(group_id),
                                                                    window.region,
                                                                    layer);
                }
                PrimitiveVector window_mass;
                for (APBlockId blk_id : window.contained_blocks) {
                    const PrimitiveVector& blk_mass = density_manager_->mass_calculator().get_block_mass(blk_id);
                    window_mass += blk_mass;
                }
                VTR_LOG("\t\t\tCapacity: %f\n",
                        window_capacity.manhattan_norm());
                VTR_LOG("\t\t\tMass: %f\n",
                        window_mass.manhattan_norm());
                VTR_LOG("\t\t\tNumber of contained blocks: %zu\n",
                        window.contained_blocks.size());
            }
        }
    }

    // Move the blocks into the bins.
    move_blocks_out_of_windows(finished_windows);

    // Verify that the bins are valid after moving blocks back from windows.
    VTR_ASSERT_SAFE(density_manager_->verify());
}

PartitionedWindow BiPartitioningPartialLegalizer::partition_window(
    SpreadingWindow& window,
    PrimitiveGroupId group_id) {

    PartitionedWindow partitioned_window;

    // We always start by partitioning the layer first.
    if (window.layer_low != window.layer_high) {
        // Set the pivot to be the middle of the layer.
        // TODO: We should do a proper search here to find a good partition line.
        double pivot = (window.layer_high - window.layer_low) / 2.0;

        partitioned_window.partition_dir = e_partition_dir::PLANAR;
        partitioned_window.pivot_pos = pivot;
        partitioned_window.lower_window.region = window.region;
        partitioned_window.upper_window.region = window.region;

        partitioned_window.lower_window.layer_low = window.layer_low;
        partitioned_window.lower_window.layer_high = pivot;
        partitioned_window.upper_window.layer_low = pivot + 1;
        partitioned_window.upper_window.layer_high = window.layer_high;

        return partitioned_window;
    }
    VTR_ASSERT_SAFE(window.layer_low == window.layer_high);
    size_t layer = window.layer_low;

    // Search for the ideal partition line on the window. Here, we attempt each
    // partition and measure how well this cuts the capacity of the region in
    // half. Cutting the capacity of the region in half should allow the blocks
    // within the region to also be cut in half (assuming a good initial window
    // was chosen). This should allow the spreader to spread things more evenly
    // and converge faster. Hence, it is worth spending more time trying to find
    // better partition lines.
    //
    // Here, we compute the score of a partition as a number between 0 and 1
    // which represents how balanced the partition is. 0 means that all of the
    // capacity is on one side of the partition, 1 means that the capacities of
    // the two partitions are perfectly balanced (equal on both sides).
    float best_score = -1.0f;
    const std::vector<PrimitiveVectorDim>& dims = dim_grouper_.get_dims_in_group(group_id);

    // First, try all of the vertical partitions.
    double min_pivot_x = std::floor(window.region.xmin()) + 1.0;
    double max_pivot_x = std::ceil(window.region.xmax()) - 1.0;
    for (double pivot_x = min_pivot_x; pivot_x <= max_pivot_x; pivot_x++) {
        // Cut the region at this cut line.
        auto lower_region = vtr::Rect<double>(vtr::Point<double>(window.region.xmin(),
                                                                 window.region.ymin()),
                                              vtr::Point<double>(pivot_x,
                                                                 window.region.ymax()));

        auto upper_region = vtr::Rect<double>(vtr::Point<double>(pivot_x,
                                                                 window.region.ymin()),
                                              vtr::Point<double>(window.region.xmax(),
                                                                 window.region.ymax()));

        // Compute the capacity of each partition for the dims that we care
        // about.
        // TODO: This can be made better by looking at the mass of all blocks
        //       within the window and scaling the capacity based on that.
        float lower_window_capacity = capacity_prefix_sum_.get_sum(dims, lower_region, layer).manhattan_norm();
        lower_window_capacity = std::max(lower_window_capacity, 0.0f);
        float upper_window_capacity = capacity_prefix_sum_.get_sum(dims, upper_region, layer).manhattan_norm();
        upper_window_capacity = std::max(upper_window_capacity, 0.0f);

        // Compute the score of this partition line. The score is simply just
        // the minimum of the two capacities dividided by the maximum of the
        // two capacities.
        float smaller_capacity = std::min(lower_window_capacity, upper_window_capacity);
        float larger_capacity = std::max(lower_window_capacity, upper_window_capacity);
        float cut_score = smaller_capacity / larger_capacity;

        // If this is the best cut we have ever seen, save it as the result.
        if (cut_score > best_score) {
            best_score = cut_score;
            partitioned_window.partition_dir = e_partition_dir::VERTICAL;
            partitioned_window.pivot_pos = pivot_x;
            partitioned_window.lower_window.region = lower_region;
            partitioned_window.upper_window.region = upper_region;

            partitioned_window.lower_window.layer_low = layer;
            partitioned_window.lower_window.layer_high = layer;
            partitioned_window.upper_window.layer_low = layer;
            partitioned_window.upper_window.layer_high = layer;
        }
    }

    // Next, try all of the horizontal partitions.
    double min_pivot_y = std::floor(window.region.ymin()) + 1.0;
    double max_pivot_y = std::ceil(window.region.ymax()) - 1.0;
    for (double pivot_y = min_pivot_y; pivot_y <= max_pivot_y; pivot_y++) {
        // Cut the region at this cut line.
        auto lower_region = vtr::Rect<double>(vtr::Point<double>(window.region.xmin(),
                                                                 window.region.ymin()),
                                              vtr::Point<double>(window.region.xmax(),
                                                                 pivot_y));

        auto upper_region = vtr::Rect<double>(vtr::Point<double>(window.region.xmin(),
                                                                 pivot_y),
                                              vtr::Point<double>(window.region.xmax(),
                                                                 window.region.ymax()));

        // Compute the capacity of each partition for the dims that we care
        // about.
        // TODO: This can be made better by looking at the mass of all blocks
        //       within the window and scaling the capacity based on that.
        float lower_window_capacity = capacity_prefix_sum_.get_sum(dims, lower_region, layer).manhattan_norm();
        lower_window_capacity = std::max(lower_window_capacity, 0.0f);
        float upper_window_capacity = capacity_prefix_sum_.get_sum(dims, upper_region, layer).manhattan_norm();
        upper_window_capacity = std::max(upper_window_capacity, 0.0f);

        // Compute the score of this partition line. The score is simply just
        // the minimum of the two capacities dividided by the maximum of the
        // two capacities.
        float smaller_capacity = std::min(lower_window_capacity, upper_window_capacity);
        float larger_capacity = std::max(lower_window_capacity, upper_window_capacity);
        float cut_score = smaller_capacity / larger_capacity;

        // If this is the best cut we have ever seen, save it as the result.
        if (cut_score > best_score) {
            best_score = cut_score;
            partitioned_window.partition_dir = e_partition_dir::HORIZONTAL;
            partitioned_window.pivot_pos = pivot_y;
            partitioned_window.lower_window.region = lower_region;
            partitioned_window.upper_window.region = upper_region;

            partitioned_window.lower_window.layer_low = layer;
            partitioned_window.lower_window.layer_high = layer;
            partitioned_window.upper_window.layer_low = layer;
            partitioned_window.upper_window.layer_high = layer;
        }
    }

    VTR_ASSERT_MSG(best_score >= 0.0f,
                   "Could not find a partition line for given window");

    return partitioned_window;
}

/**
 * @brief Helper method to decide if we should move a block from one window
 *        to another given the change in overfills.
 *
 *  @param window_curr_overfill
 *      The overfill of this window if the block is not moved to other.
 *  @param window_new_overfill
 *      The overfill of this window if the block is moved to other.
 *  @param other_window_curr_overfill
 *      The overfill of the other window if the block is not moved to other.
 *  @param other_window_new_overfill
 *      THe overfill of the other window if the block is moved to other.
 */
static bool should_move_blk_to_other_window(const PrimitiveVector& window_curr_overfill,
                                            const PrimitiveVector& window_new_overfill,
                                            const PrimitiveVector& other_window_curr_overfill,
                                            const PrimitiveVector& other_window_new_overfill) {
    // Check what would be the gain of moving this block to the other partition.
    // Here, we define gain as the amount the overfill of the window would
    // decrease due to moving this block to the other side.
    float gain = window_curr_overfill.sum() - window_new_overfill.sum();
    // If there is no gain for removing this from the current window, keep it where it is.
    if (gain == 0.0)
        return false;

    // Compute how much the other window would "lose" due to having this
    // block on the other side.
    // Here, we define loss as the amount the overfill of the other window would
    // increase due to moving this block to the other side.
    float loss = other_window_new_overfill.sum() - other_window_curr_overfill.sum();
    // If we would lose more (i.e. make the overfill worse) by moving to
    // other window than we would gain, keep on this side.
    if (loss >= gain)
        return false;

    // If we reach this point, this means that there is more gain to moving this
    // block to the other window than there is loss. Move it.
    return true;
}

/**
 * @brief Helper method for trying to move a block with the given mass from its
 *        window to the other window if, by moving this block, we can balance
 *        the overfill of the two windows better.
 *
 * Returns true if the block was moved.
 *
 * If this block is moved, the utilizations and the overfills will be updated
 * accordingly, but the block itself will remain unmoved.
 * NOTE: This may seem strange, it is; this is to prevent updating the primitive
 *       vectors too much, which tends to take a long time.
 *
 *  @param blk_mass
 *      The mass of the block to move from this window to the other window.
 *  @param window_curr_utilization
 *      The current utilization of this window.
 *  @param window_curr_overfill
 *      The current overfill of this window.
 *  @param other_window_curr_utilization
 *      The current utilization of the other window.
 *  @param other_window_curr_overfill
 *      The current overfill of the other window.
 *  @param other_window_capacity
 *      The capacity of the other window.
 */
static bool try_move_blk_to_other_window(const PrimitiveVector& blk_mass,
                                         PrimitiveVector& window_curr_utilization,
                                         PrimitiveVector& window_curr_overfill,
                                         PrimitiveVector& other_window_curr_utilization,
                                         PrimitiveVector& other_window_curr_overfill,
                                         const PrimitiveVector& other_window_capacity) {
    // Compute the overfill this window would have if we remove the block from it.
    PrimitiveVector window_new_overfill = window_curr_overfill - blk_mass;
    window_new_overfill.relu();

    // Compute the overfill of the other window if the block moved into it.
    PrimitiveVector other_window_new_utilization = other_window_curr_utilization + blk_mass;
    PrimitiveVector other_window_new_overfill = other_window_new_utilization - other_window_capacity;
    other_window_new_overfill.relu();

    // Check if we should move this block to the other window, given the change
    // in overfill.
    bool should_move_block = should_move_blk_to_other_window(window_curr_overfill,
                                                             window_new_overfill,
                                                             other_window_curr_overfill,
                                                             other_window_new_overfill);
    // If we should not move it, return false.
    if (!should_move_block)
        return false;

    // If we should move it, update the overfills of each window.
    // NOTE: Here we are avoiding doing operations on the primitive vectors as
    //       much as possible.
    window_curr_utilization -= blk_mass;
    window_curr_overfill = std::move(window_new_overfill);
    other_window_curr_utilization = std::move(other_window_new_utilization);
    other_window_curr_overfill = std::move(other_window_new_overfill);

    // The block was moved.
    return true;
}

void BiPartitioningPartialLegalizer::partition_blocks_in_window(
    SpreadingWindow& window,
    PartitionedWindow& partitioned_window,
    PrimitiveGroupId group_id,
    const PartialPlacement& p_placement) {

    SpreadingWindow& lower_window = partitioned_window.lower_window;
    SpreadingWindow& upper_window = partitioned_window.upper_window;

    // Get the capacity of each window partition.
    const std::vector<PrimitiveVectorDim>& dims = dim_grouper_.get_dims_in_group(group_id);
    PrimitiveVector lower_window_capacity;
    for (size_t layer = lower_window.layer_low; layer <= lower_window.layer_high; layer++) {
        lower_window_capacity += capacity_prefix_sum_.get_sum(dims,
                                                              lower_window.region,
                                                              layer);
    }
    PrimitiveVector upper_window_capacity;
    for (size_t layer = upper_window.layer_low; layer <= upper_window.layer_high; layer++) {
        upper_window_capacity += capacity_prefix_sum_.get_sum(dims,
                                                              upper_window.region,
                                                              layer);
    }

    VTR_ASSERT_SAFE(lower_window_capacity.is_non_negative());
    VTR_ASSERT_SAFE(upper_window_capacity.is_non_negative());

    // FIXME: We need to take into account the current utilization of the
    //        fixed blocks... We need to take into account that they are there.
    //        Currently we assume the underfill is the capacity
    //        Without this, we may overfill blocks which have fixed blocks in
    //        them.

    // If the lower window has no space, put all of the blocks in the upper window.
    if (lower_window_capacity.is_zero()) {
        upper_window.contained_blocks = std::move(window.contained_blocks);
        return;
    }
    // If the upper window has no space, put all of the blocks in the lower window.
    if (upper_window_capacity.is_zero()) {
        lower_window.contained_blocks = std::move(window.contained_blocks);
        return;
    }

    // Sort the blocks and get the pivot index. The pivot index is the index in
    // the windows contained block which decides which sub-window the block
    // wants to be in. The blocks at indices [0, pivot) want to be in the lower
    // window, blocks at indices [pivot, num_blks) want to be in the upper window.
    // This want is based on the solved positions of the blocks.
    size_t pivot;
    if (partitioned_window.partition_dir == e_partition_dir::VERTICAL) {
        // Sort the blocks in the window by the x coordinate.
        std::stable_sort(window.contained_blocks.begin(), window.contained_blocks.end(), [&](APBlockId a, APBlockId b) {
            return p_placement.block_x_locs[a] < p_placement.block_x_locs[b];
        });
        auto upper = std::upper_bound(window.contained_blocks.begin(),
                                      window.contained_blocks.end(),
                                      partitioned_window.pivot_pos,
                                      [&](double value, APBlockId blk_id) {
                                          return value < p_placement.block_x_locs[blk_id];
                                      });
        pivot = std::distance(window.contained_blocks.begin(), upper);
    } else if (partitioned_window.partition_dir == e_partition_dir::HORIZONTAL)  {
        // Sort the blocks in the window by the y coordinate.
        std::stable_sort(window.contained_blocks.begin(), window.contained_blocks.end(), [&](APBlockId a, APBlockId b) {
            return p_placement.block_y_locs[a] < p_placement.block_y_locs[b];
        });
        auto upper = std::upper_bound(window.contained_blocks.begin(),
                                      window.contained_blocks.end(),
                                      partitioned_window.pivot_pos,
                                      [&](double value, APBlockId blk_id) {
                                          return value < p_placement.block_y_locs[blk_id];
                                      });
        pivot = std::distance(window.contained_blocks.begin(), upper);
    } else {
        VTR_ASSERT(partitioned_window.partition_dir == e_partition_dir::PLANAR);
        // Sort the blocks in the window by the z coordinate.
        std::stable_sort(window.contained_blocks.begin(), window.contained_blocks.end(), [&](APBlockId a, APBlockId b) {
            return p_placement.block_layer_nums[a] < p_placement.block_layer_nums[b];
        });
        auto upper = std::upper_bound(window.contained_blocks.begin(),
                                      window.contained_blocks.end(),
                                      partitioned_window.pivot_pos,
                                      [&](double value, APBlockId blk_id) {
                                          return value < p_placement.block_layer_nums[blk_id];
                                      });
        pivot = std::distance(window.contained_blocks.begin(), upper);
    }

    // 1) Put everything on the side that they want to be on.
    std::vector<APBlockId> lower_contained_blocks(window.contained_blocks.begin(),
                                                  window.contained_blocks.begin() + pivot);
    std::vector<APBlockId> upper_contained_blocks(window.contained_blocks.begin() + pivot,
                                                  window.contained_blocks.end());

    // Compute the overfill of each sub-window
    PrimitiveVector lower_window_utilization;
    for (APBlockId blk_id : lower_contained_blocks) {
        const PrimitiveVector& blk_mass = density_manager_->mass_calculator().get_block_mass(blk_id);
        lower_window_utilization += blk_mass;
    }
    PrimitiveVector lower_window_overfill = lower_window_utilization - lower_window_capacity;
    lower_window_overfill.relu();

    PrimitiveVector upper_window_utilization;
    for (APBlockId blk_id : upper_contained_blocks) {
        const PrimitiveVector& blk_mass = density_manager_->mass_calculator().get_block_mass(blk_id);
        upper_window_utilization += blk_mass;
    }
    PrimitiveVector upper_window_overfill = upper_window_utilization - upper_window_capacity;
    upper_window_overfill.relu();

    // If both windows are not overfilled, we are done.
    if (lower_window_overfill.is_zero() && upper_window_overfill.is_zero()) {
        lower_window.contained_blocks = std::move(lower_contained_blocks);
        upper_window.contained_blocks = std::move(upper_contained_blocks);
        return;
    }

    // 2) Resolve any overfill by moving blocks over the partition line.
    // Here, we try to move blocks that are closest to the partition line such
    // that the overfill on both sides is balanced.

    // Try to move things from lower to upper greedily.
    std::vector<APBlockId> lower_blocks_to_move;
    lower_blocks_to_move.reserve(lower_contained_blocks.size());
    // We start from the blocks closest to the partition line and move away.
    for (int i = lower_contained_blocks.size() - 1; i >= 0; i--) {
        // If the lower window has no overfill, there is no reason to move anything.
        if (lower_window_overfill.is_zero())
            break;

        // Get the mass of this block.
        APBlockId lower_blk = lower_contained_blocks[i];
        const PrimitiveVector& blk_mass = density_manager_->mass_calculator().get_block_mass(lower_blk);

        // Using the mass, try to move the block to the other window if it would
        // improve the imbalance of overfill.
        // If the block is moved, this will update the window utilizations and
        // overfills accordingly.
        bool block_was_moved = try_move_blk_to_other_window(blk_mass,
                                                            lower_window_utilization,
                                                            lower_window_overfill,
                                                            upper_window_utilization,
                                                            upper_window_overfill,
                                                            upper_window_capacity);

        if (block_was_moved) {
            // If the block was moved, add this block to the list of blocks to
            // move and invalidate the block in the current list.
            // Here, we mark the block as invalid and later we will compress the vector.
            lower_blocks_to_move.push_back(lower_blk);
            lower_contained_blocks[i] = APBlockId::INVALID();
        }
    }

    // Try to move things from upper to lower greedily.
    std::vector<APBlockId> upper_blocks_to_move;
    upper_blocks_to_move.reserve(upper_contained_blocks.size());
    // We start from the blocks closest to the partition line and move away.
    for (size_t i = 0; i < upper_contained_blocks.size(); i++) {
        // If the upper window has no overfill, there is no reason to move anything.
        if (upper_window_overfill.is_zero())
            break;

        // Get the mass of this block.
        APBlockId upper_blk = upper_contained_blocks[i];
        const PrimitiveVector& blk_mass = density_manager_->mass_calculator().get_block_mass(upper_blk);

        // Using the mass, try to move the block to the other window if it would
        // improve the imbalance of overfill.
        // If the block is moved, this will update the window utilizations and
        // overfills accordingly.
        bool block_was_moved = try_move_blk_to_other_window(blk_mass,
                                                            upper_window_utilization,
                                                            upper_window_overfill,
                                                            lower_window_utilization,
                                                            lower_window_overfill,
                                                            lower_window_capacity);

        if (block_was_moved) {
            // If the block was moved, add this block to the list of blocks to
            // move and invalidate the block in the current list.
            // Here, we mark the block as invalid and later we will compress the vector.
            upper_blocks_to_move.push_back(upper_blk);
            upper_contained_blocks[i] = APBlockId::INVALID();
        }
    }

    // Move the blocks into the windows.
    //  Here we maintain the order the best we can to make future sorts faster.
    // Compress the lower contained blocks which have not moved.
    lower_window.contained_blocks.reserve(lower_contained_blocks.size() - lower_blocks_to_move.size() + upper_blocks_to_move.size());
    for (APBlockId lower_blk : lower_contained_blocks) {
        if (lower_blk.is_valid())
            lower_window.contained_blocks.push_back(lower_blk);
    }
    // Insert the upper blocks which have moved into the lower partition.
    lower_window.contained_blocks.insert(lower_window.contained_blocks.end(),
                                         upper_blocks_to_move.begin(),
                                         upper_blocks_to_move.end());

    // Insert the lower blocks which have moved into the upper partition. Here,
    // they were inserted in reverse order, so we reverse them before inserting.
    // TODO: Investigate if this actually has an improvements.
    upper_window.contained_blocks.reserve(upper_contained_blocks.size() - upper_blocks_to_move.size() + lower_blocks_to_move.size());
    std::reverse(lower_blocks_to_move.begin(), lower_blocks_to_move.end());
    upper_window.contained_blocks.insert(upper_window.contained_blocks.end(),
                                         lower_blocks_to_move.begin(),
                                         lower_blocks_to_move.end());
    // Compress the upper contained blocks which have not moved.
    for (APBlockId upper_blk : upper_contained_blocks) {
        if (upper_blk.is_valid())
            upper_window.contained_blocks.push_back(upper_blk);
    }
}

void BiPartitioningPartialLegalizer::move_blocks_out_of_windows(
    std::vector<SpreadingWindow>& finished_windows) {

    for (const SpreadingWindow& window : finished_windows) {
        // Get the bin at the center of the window.
        vtr::Point<double> center = get_center_of_rect(window.region);
        VTR_ASSERT(window.layer_low == window.layer_high);
        size_t layer = window.layer_low;
        FlatPlacementBinId bin_id = density_manager_->get_bin(center.x(), center.y(), layer);

        // Move all blocks in the window into this bin.
        for (APBlockId blk_id : window.contained_blocks) {
            // Note: The blocks should have been removed from their original
            //       bins when they were put into the windows. There are asserts
            //       within the density manager class which will verify this.
            density_manager_->insert_block_into_bin(blk_id, bin_id);
        }
    }
}
