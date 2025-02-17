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
#include <limits>
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
#include "globals.h"
#include "partial_placement.h"
#include "physical_types.h"
#include "primitive_vector.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vtr_assert.h"
#include "vtr_geometry.h"
#include "vtr_log.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"
#include "vtr_vector_map.h"

std::unique_ptr<PartialLegalizer> make_partial_legalizer(e_partial_legalizer legalizer_type,
                                                         const APNetlist& netlist,
                                                         std::shared_ptr<FlatPlacementDensityManager> density_manager) {
    // Based on the partial legalizer type passed in, build the partial legalizer.
    switch (legalizer_type) {
        case e_partial_legalizer::FLOW_BASED:
            return std::make_unique<FlowBasedLegalizer>(netlist, density_manager);
        default:
            VPR_FATAL_ERROR(VPR_ERROR_AP,
                            "Unrecognized partial legalizer type");
            break;
    }
    return nullptr;
}

/**
 * @brief Get the number of models in the device architecture.
 *
 * FIXME: These are stored in such an annoying way. It should be much easier
 *        to get this information!
 */
static inline size_t get_num_models() {
    size_t num_models = 0;
    t_model* curr_model = g_vpr_ctx.device().arch->models;
    while (curr_model != nullptr) {
        num_models++;
        curr_model = curr_model->next;
    }
    curr_model = g_vpr_ctx.device().arch->model_library;
    while (curr_model != nullptr) {
        num_models++;
        curr_model = curr_model->next;
    }
    return num_models;
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
    // This is an unfortunate consequence of using double precision to store
    // the bounding box. We need to ensure that the bin represents a tile (not
    // part of a tile). If it did represent part of a tile, this algorithm
    // would need to change.
    VTR_ASSERT_DEBUG(static_cast<double>(bl_x) == bin_region.bottom_left().x() &&
                     static_cast<double>(bl_y) == bin_region.bottom_left().y() &&
                     static_cast<double>(bin_width) == bin_region.width() &&
                     static_cast<double>(bin_height) == bin_region.height());

    double placeable_region_width, placeable_region_height, placeable_region_depth;
    std::tie(placeable_region_width, placeable_region_height, placeable_region_depth) = density_manager.get_overall_placeable_region_size();
    // Current does not handle 3D FPGAs
    VTR_ASSERT(placeable_region_depth == 1.0);

    // Add the neighbors.
    std::unordered_set<FlatPlacementBinId> neighbor_bin_ids;
    // Add unique tiles on left and right sides
    for (size_t ty = bl_y; ty < bl_y + bin_height; ty++) {
        if (bl_x >= 1)
            neighbor_bin_ids.insert(density_manager.get_bin(bl_x - 1, ty, 0.0));
        if (bl_x <= (int)(placeable_region_width - bin_width - 1))
            neighbor_bin_ids.insert(density_manager.get_bin(bl_x + bin_width, ty, 0.0));
    }
    // Add unique tiles on the top and bottom
    for (size_t tx = bl_x; tx < bl_x + bin_width; tx++) {
        if (bl_y >= 1)
            neighbor_bin_ids.insert(density_manager.get_bin(tx, bl_y - 1, 0.0));
        if (bl_y <= (int)(placeable_region_height - bin_height - 1))
            neighbor_bin_ids.insert(density_manager.get_bin(tx, bl_y + bin_height, 0.0));
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

void FlowBasedLegalizer::compute_neighbors_of_bin(FlatPlacementBinId src_bin_id, size_t num_models) {
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
    std::vector<bool> up_found(num_models, false);
    std::vector<bool> down_found(num_models, false);
    std::vector<bool> left_found(num_models, false);
    std::vector<bool> right_found(num_models, false);
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
                                       std::vector<bool>& dir_found) {
        bool all_found = true;
        // Go through all possible models
        for (size_t i = 0; i < num_models; i++) {
            // If this model has been found in this direction, continue.
            if (dir_found[i])
                continue;
            // If this bin has this model in its capacity, we found a neighbor!
            const PrimitiveVector& target_bin_capacity = density_manager_->get_bin_capacity(target_bin_id);
            if (target_bin_capacity.get_dim_val(i) > 0) {
                dir_found[i] = true;
                neighbors.insert(target_bin_id);
            } else {
                all_found = false;
            }
        }
        return all_found;
    };

    // Perform the BFS from the source node until all nodes have been explored
    // or all of the models have been found in all directions.
    while(!q.empty() && !all_models_found_in_all_directions) {
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
        all_models_found_in_all_directions = all_up_found && all_down_found &&
                                             all_left_found && all_right_found;
    }

    // Assign the results into the neighbors of the bin.
    bin_neighbors_[src_bin_id].assign(neighbors.begin(), neighbors.end());
}

FlowBasedLegalizer::FlowBasedLegalizer(const APNetlist& netlist,
                                       std::shared_ptr<FlatPlacementDensityManager> density_manager)
            : PartialLegalizer(netlist)
            , density_manager_(density_manager)
            , bin_neighbors_(density_manager_->flat_placement_bins().bins().size()) {

    // Connect the bins.
    size_t num_models = get_num_models();
    for (FlatPlacementBinId bin_id : density_manager_->flat_placement_bins().bins()) {
        compute_neighbors_of_bin(bin_id, num_models);
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
        std::vector<FlatPlacementBinId> &p = queue.front();
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
    std::sort(paths.begin(), paths.end(), [&](const std::vector<FlatPlacementBinId>& a,
                                              const std::vector<FlatPlacementBinId>& b) {
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

void FlowBasedLegalizer::legalize(PartialPlacement &p_placement) {
    VTR_LOGV(log_verbosity_ >= 10, "Running Flow-Based Legalizer\n");

    // Reset the bins from the previous iteration and prepare for this iteration.
    density_manager_->empty_bins();
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
        // The standard L2 norm would give more weigth to <0, 2>.
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

