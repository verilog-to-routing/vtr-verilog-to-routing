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
#include "device_grid.h"
#include "globals.h"
#include "partial_placement.h"
#include "physical_types.h"
#include "primitive_vector.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vpr_types.h"
#include "vtr_assert.h"
#include "vtr_geometry.h"
#include "vtr_log.h"
#include "vtr_ndmatrix.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"
#include "vtr_vector_map.h"

std::unique_ptr<PartialLegalizer> make_partial_legalizer(e_partial_legalizer legalizer_type,
                                                         const APNetlist& netlist) {
    // Based on the partial legalizer type passed in, build the partial legalizer.
    switch (legalizer_type) {
        case e_partial_legalizer::FLOW_BASED:
            return std::make_unique<FlowBasedLegalizer>(netlist);
        default:
            VPR_FATAL_ERROR(VPR_ERROR_AP,
                            "Unrecognized partial legalizer type");
            break;
    }
    return nullptr;
}

/**
 * @brief Get the scalar mass of the given model (primitive type).
 *
 * A model with a higher mass will take up more space in its bin which may force
 * more spreading of that type of primitive.
 *
 * TODO: This will be made more complicated later. Models may be weighted based
 *       on some factors.
 */
static inline float get_model_mass(const t_model* model) {
    // Currently, all models have a mass of one.
    (void)model;
    return 1.f;
}

/**
 * @brief Get the primitive mass of the given block.
 *
 * This returns an M-dimensional vector with each entry indicating the mass of
 * that primitive type in this block. M is the number of unique models
 * (primitive types) in the architecture.
 */
static inline PrimitiveVector get_primitive_mass(APBlockId blk_id,
                                                 const APNetlist& netlist) {
    PrimitiveVector mass;
    const t_pack_molecule* mol = netlist.block_molecule(blk_id);
    for (AtomBlockId atom_blk_id : mol->atom_block_ids) {
        // See issue #2791, some of the atom_block_ids may be invalid. They can
        // safely be ignored.
        if (!atom_blk_id.is_valid())
            continue;
        const t_model* model = g_vpr_ctx.atom().nlist.block_model(atom_blk_id);
        VTR_ASSERT_DEBUG(model->index >= 0);
        mass.add_val_to_dim(get_model_mass(model), model->index);
    }
    return mass;
}

// This method is being forward-declared due to the double recursion below.
// Eventually this should be made into a non-recursive algorithm for performance,
// however this is not in a performance critical part of the code.
static PrimitiveVector get_primitive_capacity(const t_pb_type* pb_type);

/**
 * @brief Get the amount of primitives this mode can contain.
 *
 * This is part of a double recursion, since a mode contains primitives which
 * themselves have modes.
 */
static PrimitiveVector get_primitive_capacity(const t_mode& mode) {
    // Accumulate the capacities of all the pbs in this mode.
    PrimitiveVector capacity;
    for (int pb_child_idx = 0; pb_child_idx < mode.num_pb_type_children; pb_child_idx++) {
        const t_pb_type& pb_type = mode.pb_type_children[pb_child_idx];
        PrimitiveVector pb_capacity = get_primitive_capacity(&pb_type);
        // A mode may contain multiple pbs of the same type, multiply the
        // capacity.
        pb_capacity *= pb_type.num_pb;
        capacity += pb_capacity;
    }
    return capacity;
}

/**
 * @brief Get the amount of primitives this pb can contain.
 *
 * This is the other part of the double recursion. A pb may have multiple modes.
 * Modes are made of pbs.
 */
static PrimitiveVector get_primitive_capacity(const t_pb_type* pb_type) {
    // Since a pb cannot be multiple modes at the same time, we do not
    // accumulate the capacities of the mode. Instead we need to "mix" the two
    // capacities as if the pb could choose either one.
    PrimitiveVector capacity;
    // If this is a leaf / primitive, create the base PrimitiveVector capacity.
    if (pb_type->num_modes == 0) {
        const t_model* model = pb_type->model;
        VTR_ASSERT(model != nullptr);
        VTR_ASSERT_DEBUG(model->index >= 0);
        capacity.add_val_to_dim(get_model_mass(model), model->index);
        return capacity;
    }
    // For now, we simply mix the capacities of modes by taking the max of each
    // dimension of the capcities. This provides an upper-bound on the amount of
    // primitives this pb can contain.
    for (int mode = 0; mode < pb_type->num_modes; mode++) {
        PrimitiveVector mode_capacity = get_primitive_capacity(pb_type->modes[mode]);
        capacity = PrimitiveVector::max(capacity, mode_capacity);
    }
    return capacity;
}

/**
 * @brief Helper method to get the primitive capacity of the given logical block
 *        type.
 *
 * This is the entry point to the double recursion.
 */
static inline PrimitiveVector get_primitive_capacity(const t_logical_block_type& block_type) {
    // If this logical block is empty, it cannot contain any primitives.
    if (block_type.is_empty())
        return PrimitiveVector();
    // The primitive capacity of a logical block is the primitive capacity of
    // its root pb.
    return get_primitive_capacity(block_type.pb_type);
}

/**
 * @brief Get the primitive capacity of the given sub_tile.
 *
 * Sub_tiles may reuse logical blocks between one another, therefore this method
 * requires that the capacities of all of the logical blocks have been
 * pre-calculated and stored in the given vector.
 *
 *  @param sub_tile                         The sub_tile to get the capacity of.
 *  @param logical_block_type_capacities    The capacities of all logical block
 *                                          types.
 */
static inline PrimitiveVector get_primitive_capacity(const t_sub_tile& sub_tile,
                                                     const std::vector<PrimitiveVector>& logical_block_type_capacities) {
    // Similar to getting the primitive capacity of the pb, sub_tiles have many
    // equivalent sites, but it can only be one of them at a time. Need to "mix"
    // the capacities of the different sites this sub_tile may be.
    PrimitiveVector capacity;
    for (t_logical_block_type_ptr block_type : sub_tile.equivalent_sites) {
        const PrimitiveVector& block_capacity = logical_block_type_capacities[block_type->index];
        // Currently, we take the max of each primitive dimension as an upper
        // bound on the capacity of the sub_tile.
        capacity = PrimitiveVector::max(capacity, block_capacity);
    }
    return capacity;
}

/**
 * @brief Get the primitive capacity of a tile of the given type.
 *
 * Tiles may reuse logical blocks between one another, therefore this method
 * requires that the capacities of all of the logical blocks have been
 * pre-calculated and stored in the given vector.
 *
 *  @param tile_type                        The tile type to get the capacity of.
 *  @param logical_block_type_capacities    The capacities of all logical block
 *                                          types.
 */
static inline PrimitiveVector get_primitive_capacity(const t_physical_tile_type& tile_type,
                                                     const std::vector<PrimitiveVector>& logical_block_type_capacities) {
    // Accumulate the capacities of all the sub_tiles in the given tile type.
    PrimitiveVector capacity;
    for (const t_sub_tile& sub_tile : tile_type.sub_tiles) {
        PrimitiveVector sub_tile_capacity = get_primitive_capacity(sub_tile, logical_block_type_capacities);
        // A tile may contain many sub_tiles of the same type. Multiply by the
        // number of sub_tiles of this type.
        sub_tile_capacity *= sub_tile.capacity.total();
        capacity += sub_tile_capacity;
    }
    return capacity;
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
 * @brief Debug printing method to print the capacities of all logical blocks
 *        and physical tile types.
 */
static inline void print_capacities(const std::vector<PrimitiveVector>& logical_block_type_capacities,
                                    const std::vector<PrimitiveVector>& physical_tile_type_capacities,
                                    const std::vector<t_logical_block_type>& logical_block_types,
                                    const std::vector<t_physical_tile_type>& physical_tile_types) {
    // Get a linear list of all models.
    // TODO: Again, the way these models are stored is so annoying. It would be
    //       nice if they were already vectors!
    std::vector<t_model*> all_models;
    t_model* curr_model = g_vpr_ctx.device().arch->models;
    while (curr_model != nullptr) {
        if (curr_model->index >= (int)all_models.size())
            all_models.resize(curr_model->index + 1);
        all_models[curr_model->index] = curr_model;
        curr_model = curr_model->next;
    }
    curr_model = g_vpr_ctx.device().arch->model_library;
    while (curr_model != nullptr) {
        if (curr_model->index >= (int)all_models.size())
            all_models.resize(curr_model->index + 1);
        all_models[curr_model->index] = curr_model;
        curr_model = curr_model->next;
    }
    // Print the capacities.
    VTR_LOG("Logical Block Type Capacities:\n");
    VTR_LOG("------------------------------\n");
    VTR_LOG("name\t");
    for (t_model* model : all_models) {
        VTR_LOG("%s\t", model->name);
    }
    VTR_LOG("\n");
    for (const t_logical_block_type& block_type : logical_block_types) {
        const PrimitiveVector& capacity = logical_block_type_capacities[block_type.index];
        VTR_LOG("%s\t", block_type.name.c_str());
        for (t_model* model : all_models) {
            VTR_LOG("%.2f\t", capacity.get_dim_val(model->index));
        }
        VTR_LOG("\n");
    }
    VTR_LOG("\n");
    VTR_LOG("Physical Tile Type Capacities:\n");
    VTR_LOG("------------------------------\n");
    VTR_LOG("name\t");
    for (t_model* model : all_models) {
        VTR_LOG("%s\t", model->name);
    }
    VTR_LOG("\n");
    for (const t_physical_tile_type& tile_type : physical_tile_types) {
        const PrimitiveVector& capacity = physical_tile_type_capacities[tile_type.index];
        VTR_LOG("%s\t", tile_type.name.c_str());
        for (t_model* model : all_models) {
            VTR_LOG("%.2f\t", capacity.get_dim_val(model->index));
        }
        VTR_LOG("\n");
    }
    VTR_LOG("\n");
}

/**
 * @brief Helper method to get the direct neighbors of the given bin.
 *
 * A direct neighbor of a bin is a bin which shares a side with the given bin on
 * the tile graph. Corners do not count.
 */
static std::unordered_set<LegalizerBinId> get_direct_neighbors_of_bin(
                                        LegalizerBinId bin_id,
                                        const vtr::vector_map<LegalizerBinId, LegalizerBin>& bins,
                                        const vtr::NdMatrix<LegalizerBinId, 2> tile_bin) {
    const LegalizerBin& bin = bins[bin_id];
    int bl_x = bin.bounding_box.bottom_left().x();
    int bl_y = bin.bounding_box.bottom_left().y();
    size_t bin_width = bin.bounding_box.width();
    size_t bin_height = bin.bounding_box.height();
    // This is an unfortunate consequence of using double precision to store
    // the bounding box. We need to ensure that the bin represents a tile (not
    // part of a tile). If it did represent part of a tile, this algorithm
    // would need to change.
    VTR_ASSERT_DEBUG(static_cast<double>(bl_x) == bin.bounding_box.bottom_left().x() &&
                     static_cast<double>(bl_y) == bin.bounding_box.bottom_left().y() &&
                     static_cast<double>(bin_width) == bin.bounding_box.width() &&
                     static_cast<double>(bin_height) == bin.bounding_box.height());

    // Add the neighbors.
    std::unordered_set<LegalizerBinId> neighbor_bin_ids;
    // Add unique tiles on left and right sides
    for (size_t ty = bl_y; ty < bl_y + bin_height; ty++) {
        if (bl_x >= 1)
            neighbor_bin_ids.insert(tile_bin[bl_x - 1][ty]);
        if (bl_x <= (int)(tile_bin.dim_size(0) - bin_width - 1))
            neighbor_bin_ids.insert(tile_bin[bl_x + bin_width][ty]);
    }
    // Add unique tiles on the top and bottom
    for (size_t tx = bl_x; tx < bl_x + bin_width; tx++) {
        if (bl_y >= 1)
            neighbor_bin_ids.insert(tile_bin[tx][bl_y - 1]);
        if (bl_y <= (int)(tile_bin.dim_size(1) - bin_height - 1))
            neighbor_bin_ids.insert(tile_bin[tx][bl_y + bin_height]);
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

void FlowBasedLegalizer::compute_neighbors_of_bin(LegalizerBinId src_bin_id, size_t num_models) {
    // Make sure that this bin does not already have neighbors.
    VTR_ASSERT_DEBUG(bins_[src_bin_id].neighbors.size() == 0);

    // Bins need to be neighbors to every possible molecule type so things can
    // flow properly.
    // Perform BFS to find the closest bins of each type. Where closest is in
    // manhattan distance.

    // Create the queue and insert the source bin into it.
    std::queue<LegalizerBinId> q;
    q.push(src_bin_id);
    // Create visited flags for each bin. Set the source to visited.
    vtr::vector_map<LegalizerBinId, bool> bin_visited(bins_.size(), false);
    bin_visited[src_bin_id] = true;
    // Create a distance count for each bin from the src.
    vtr::vector_map<LegalizerBinId, unsigned> bin_distance(bins_.size(), 0);
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
    vtr::Point<double> src_bin_center = get_center_of_rect(bins_[src_bin_id].bounding_box);
    // The result will be stored in this set.
    std::unordered_set<LegalizerBinId> neighbors;

    // Helper method to add a neighbor to the set of neighbors and update the
    // found flags for a given direction if this bin is new for a given model
    // type. This method returns true if every model has been found in the given
    // direction (i.e. dir_found is now all true).
    auto add_neighbor_if_new_dir = [&](LegalizerBinId target_bin_id,
                                       std::vector<bool>& dir_found) {
        bool all_found = true;
        // Go through all possible models
        for (size_t i = 0; i < num_models; i++) {
            // If this model has been found in this direction, continue.
            if (dir_found[i])
                continue;
            // If this bin has this model in its capacity, we found a neighbor!
            if (bins_[target_bin_id].capacity.get_dim_val(i) > 0) {
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
        LegalizerBinId bin_id = q.front();
        q.pop();
        // If the distance of this block from the source is too large, do not
        // explore.
        unsigned curr_bin_dist = bin_distance[bin_id];
        if (curr_bin_dist > max_bin_neighbor_dist_)
            continue;
        // Get the direct neighbors of the bin (neighbors that are directly
        // touching).
        auto direct_neighbors = get_direct_neighbors_of_bin(bin_id, bins_, tile_bin_);
        for (LegalizerBinId dir_neighbor_bin_id : direct_neighbors) {
            // If this neighbor has been visited, do not do anything.
            if (bin_visited[dir_neighbor_bin_id])
                continue;
            // Get the signed distance from the src bin to the target bin in the
            // x and y dimensions.
            vtr::Point<double> target_bin_center = get_center_of_rect(bins_[dir_neighbor_bin_id].bounding_box);
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
    bins_[src_bin_id].neighbors.assign(neighbors.begin(), neighbors.end());
}

FlowBasedLegalizer::FlowBasedLegalizer(const APNetlist& netlist)
            : PartialLegalizer(netlist),
              // TODO: Pass the device grid in.
              tile_bin_({g_vpr_ctx.device().grid.width(), g_vpr_ctx.device().grid.height()}) {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;
    size_t grid_width = grid.width();
    size_t grid_height = grid.height();

    // Pre-compute the capacities of all logical blocks in the device.
    // logical_block_type::index -> PrimitiveVector
    std::vector<PrimitiveVector> logical_block_type_capacities(g_vpr_ctx.device().logical_block_types.size());
    for (const t_logical_block_type& logical_block_type : g_vpr_ctx.device().logical_block_types) {
        logical_block_type_capacities[logical_block_type.index] = get_primitive_capacity(logical_block_type);
    }
    // Pre-compute the capacities of all physical tile types in the device.
    // physical_tile_type::index -> PrimitiveVector
    std::vector<PrimitiveVector> physical_tile_type_capacities(g_vpr_ctx.device().physical_tile_types.size());
    for (const t_physical_tile_type& physical_tile_type : g_vpr_ctx.device().physical_tile_types) {
        physical_tile_type_capacities[physical_tile_type.index] = get_primitive_capacity(physical_tile_type, logical_block_type_capacities);
    }
    // Print these capacities. Helpful for debugging.
    if (log_verbosity_ > 1) {
        print_capacities(logical_block_type_capacities,
                         physical_tile_type_capacities,
                         g_vpr_ctx.device().logical_block_types,
                         g_vpr_ctx.device().physical_tile_types);
    }
    // Create the bins
    // This currently creates 1 bin per tile.
    for (size_t x = 0; x < grid_width; x++) {
        for (size_t y = 0; y < grid_height; y++) {
            // Ignoring 3D placement for now.
            t_physical_tile_loc tile_loc = {(int)x, (int)y, 0};
            // Is this the root location? Only create bins for roots.
            size_t width_offset = grid.get_width_offset(tile_loc);
            size_t height_offset = grid.get_height_offset(tile_loc);
            if (width_offset != 0 || height_offset != 0) {
                // If this is not a root, point the tile_bin_ lookup to the root
                // tile location.
                tile_bin_[x][y] = tile_bin_[x - width_offset][y - height_offset];
                continue;
            }
            // Create the bin
            LegalizerBinId new_bin_id = LegalizerBinId(bins_.size());
            LegalizerBin new_bin;
            // NOTE: The bounding box from the tile does not make sense in this
            //       context, making my own here based on the tile size and
            //       position.
            t_physical_tile_type_ptr tile_type = grid.get_physical_type(tile_loc);
            int width = tile_type->width;
            int height = tile_type->height;
            new_bin.bounding_box = vtr::Rect<double>(vtr::Point<double>(x, y),
                                                     vtr::Point<double>(x + width,
                                                                        y + height));
            // The capacity of the bin is the capacity of the tile it represents.
            new_bin.capacity = physical_tile_type_capacities[tile_type->index];
            bins_.push_back(std::move(new_bin));
            tile_bin_[x][y] = new_bin_id;
        }
    }

    // Get the number of models in the device.
    size_t num_models = get_num_models();
    // Connect the bins.
    // TODO: Should create a list of bin IDs to make this more efficient.
    for (size_t x = 0; x < grid_width; x++) {
        for (size_t y = 0; y < grid_height; y++) {
            // Ignoring 3D placement for now. Will likely require modification to
            // the solver and legalizer.
            t_physical_tile_loc tile_loc = {(int)x, (int)y, 0};
            // Is this the root location?
            if (grid.get_width_offset(tile_loc) != 0 ||
                grid.get_height_offset(tile_loc) != 0) {
                continue;
            }
            // Compute the neighbors of this bin.
            compute_neighbors_of_bin(tile_bin_[x][y], num_models);
        }
    }

    // Pre-compute the masses of the APBlocks
    VTR_LOGV(log_verbosity_ >= 10, "Pre-computing the block masses...\n");
    for (APBlockId blk_id : netlist.blocks()) {
        block_masses_.insert(blk_id, get_primitive_mass(blk_id, netlist));
    }
    VTR_LOGV(log_verbosity_ >= 10, "Finished pre-computing the block masses.\n");

    // Initialize the block_bins.
    block_bins_.resize(netlist.blocks().size(), LegalizerBinId::INVALID());
}

bool FlowBasedLegalizer::verify_bins() const {
    // Make sure that every block has a bin.
    for (APBlockId blk_id : netlist_.blocks()) {
        if (!block_bins_[blk_id].is_valid()) {
            VTR_LOG("Bin Verify: Found a block that is not in a bin.\n");
            return false;
        }
    }
    // Make sure that every tile has a bin.
    const DeviceGrid& device_grid = g_vpr_ctx.device().grid;
    if (tile_bin_.dim_size(0) != device_grid.width() ||
        tile_bin_.dim_size(1) != device_grid.height()) {
        VTR_LOG("Bin Verify: Tile-bin lookup does not contain every tile.\n");
        return false;
    }
    for (size_t x = 0; x < device_grid.width(); x++) {
        for (size_t y = 0; y < device_grid.height(); y++) {
            if (!tile_bin_[x][y].is_valid()) {
                VTR_LOG("Bin Verify: Found a tile with no bin.\n");
                return false;
            }
        }
    }
    // Make sure that every bin has the correct utilization, supply, and demand.
    for (const LegalizerBin& bin : bins_) {
        PrimitiveVector calc_utilization;
        for (APBlockId blk_id : bin.contained_blocks) {
            calc_utilization += block_masses_[blk_id];
        }
        if (bin.utilization != calc_utilization) {
            VTR_LOG("Bin Verify: Found a bin with incorrect utilization.\n");
            return false;
        }
        PrimitiveVector calc_supply = bin.utilization - bin.capacity;
        calc_supply.relu();
        if (bin.supply != calc_supply) {
            VTR_LOG("Bin Verify: Found a bin with incorrect supply.\n");
            return false;
        }
        PrimitiveVector calc_demand = bin.capacity - bin.utilization;
        calc_demand.relu();
        if (bin.demand != calc_demand) {
            VTR_LOG("Bin Verify: Found a bin with incorrect demand.\n");
            return false;
        }
        if (!bin.supply.is_non_negative()) {
            VTR_LOG("Bin Verify: Found a bin with a negative supply.\n");
            return false;
        }
        if (!bin.demand.is_non_negative()) {
            VTR_LOG("Bin Verify: Found a bin with a negative demand.\n");
            return false;
        }
        if (!bin.capacity.is_non_negative()) {
            VTR_LOG("Bin Verify: Found a bin with a negative capacity.\n");
            return false;
        }
        if (!bin.utilization.is_non_negative()) {
            VTR_LOG("Bin Verify: Found a bin with a negative utilization.\n");
            return false;
        }
        if (bin.neighbors.size() == 0) {
            VTR_LOG("Bin Verify: Found a bin with no neighbors.\n");
            return false;
        }
    }
    // Make sure all overfilled bins are actually overfilled.
    // TODO: Need to make sure that all non-overfilled bins are actually not
    //       overfilled.
    for (LegalizerBinId bin_id : overfilled_bins_) {
        const LegalizerBin& bin = bins_[bin_id];
        if (bin.supply.is_zero()) {
            VTR_LOG("Bin Verify: Found an overfilled bin that was not overfilled.\n");
            return false;
        }
    }
    // If all above passed, then the bins are valid.
    return true;
}

void FlowBasedLegalizer::reset_bins() {
    // Reset all of the bins by removing all of the contained blocks.
    for (LegalizerBin& bin : bins_) {
        bin.contained_blocks.clear();
        bin.utilization = PrimitiveVector();
        bin.compute_supply();
        bin.compute_demand();
    }
    // Reset the reverse lookup of block_bins_
    std::fill(block_bins_.begin(), block_bins_.end(), LegalizerBinId::INVALID());
    // No bin can be overfilled right now.
    overfilled_bins_.clear();
}

void FlowBasedLegalizer::import_placement_into_bins(const PartialPlacement& p_placement) {
    // TODO: Maybe import the fixed block locations in the constructor and
    //       then only import the moveable block locations.
    for (APBlockId blk_id : netlist_.blocks()) {
        size_t x_loc = p_placement.block_x_locs[blk_id];
        size_t y_loc = p_placement.block_y_locs[blk_id];
        LegalizerBinId bin_id = get_bin(x_loc, y_loc);
        insert_blk_into_bin(blk_id, bin_id);
    }
}

/**
 * @brief Get the location of a block assuming that it is placed within the
 *        given bin.
 *
 * This function will return the position of the block in the point within the
 * bin's bounding box which is closest to the original position of the block
 * (the position in p_placement).
 */
static inline vtr::Point<double> get_block_location_in_bin(APBlockId blk_id,
                                                           const LegalizerBin& bin,
                                                           const PartialPlacement& p_placement) {
    // A block cannot be placed on the right or top sides of the bounding box
    // of a bin; however they can be infinitely close to these sides. It is
    // arbitrary how close to the edge we place the blocks; opted to place them
    // as close as possible.
    double epsilon = 0.0001;
    double x = std::clamp<double>(p_placement.block_x_locs[blk_id],
                                  bin.bounding_box.bottom_left().x(),
                                  bin.bounding_box.top_right().x() - epsilon);
    double y = std::clamp<double>(p_placement.block_y_locs[blk_id],
                                  bin.bounding_box.bottom_left().y(),
                                  bin.bounding_box.top_right().y() - epsilon);
    return vtr::Point<double>(x, y);
}

void FlowBasedLegalizer::export_placement_from_bins(PartialPlacement& p_placement) const {
    // Updates the partial placement with the location of the blocks in the bin
    // by moving the blocks to the point within the bin closest to where they
    // were originally.
    // TODO: This should be investigated more. This may put blocks onto the edges
    //       of bins which may not be ideal.
    for (APBlockId blk_id : netlist_.blocks()) {
        // Only the moveable block locations should be exported.
        if (netlist_.block_mobility(blk_id) == APBlockMobility::FIXED)
            continue;
        // Project the coordinate of the block in the partial placement to the
        // closest point in the bin.
        LegalizerBinId bin_id = block_bins_[blk_id];
        VTR_ASSERT_DEBUG(bin_id.is_valid());
        const LegalizerBin& bin = bins_[bin_id];
        // Set the position of the block to the closest position in the bin to
        // where the block was.
        vtr::Point<double> new_blk_pos = get_block_location_in_bin(blk_id,
                                                                   bin,
                                                                   p_placement);
        p_placement.block_x_locs[blk_id] = new_blk_pos.x();
        p_placement.block_y_locs[blk_id] = new_blk_pos.y();
    }
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
 *  @param block_masses     A lookup for the masses of all blocks.
 *  @param p_placement      The placement of the blocks prior to legalization.
 *  @param netlist          The APNetlist for the placement.
 *
 *  @return     A pair of the minimum cost moveable block and its cost.
 */
static inline std::pair<APBlockId, float> get_min_cost_block_in_bin(
                    const LegalizerBin& src_bin,
                    const LegalizerBin& target_bin,
                    const vtr::vector_map<APBlockId, PrimitiveVector>& block_masses,
                    const PartialPlacement& p_placement,
                    const APNetlist& netlist) {
    // Get the min cost block and its cost.
    APBlockId min_cost_block;
    float min_cost = std::numeric_limits<float>::infinity();
    // FIXME: If these were somehow pre-sorted, this can be made much cheaper.
    for (APBlockId blk_id : src_bin.contained_blocks) {
        // If this block is fixed, it has infinite cost to move.
        if (netlist.block_mobility(blk_id) == APBlockMobility::FIXED)
            continue;
        const PrimitiveVector& block_mass = block_masses[blk_id];
        // Is this block compatible with the target bin?
        // If the capacity of the target, projected onto the mass, is less than
        // the mass, then the block is not compatible.
        // TODO: We may want to add a cost term based on how much space is
        //       available in the bin?
        PrimitiveVector target_capacity = target_bin.capacity;
        target_capacity.project(block_mass);
        if (target_capacity < block_mass)
            continue;
        // Compute the quadratic movement (aka cost).
        vtr::Point<double> new_block_pos = get_block_location_in_bin(blk_id,
                                                                     target_bin,
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
 *  @param block_masses     A lookup for the masses of all blocks.
 *  @param p_placement      The placement of the blocks prior to legalization.
 *  @param netlist          The APNetlist for the placement.
 */
static inline float compute_cost(const LegalizerBin& src_bin,
                                 const LegalizerBin& target_bin,
                                 float psi,
                                 const vtr::vector_map<APBlockId, PrimitiveVector>& block_masses,
                                 const PartialPlacement& p_placement,
                                 const APNetlist& netlist) {
    // If the src bin is empty, then there is nothing to move.
    if (src_bin.contained_blocks.size() == 0)
        return std::numeric_limits<float>::infinity();
    // Get the min cost block in the src bin which is compatible with the target
    // bin.
    APBlockId min_cost_block;
    float min_cost;
    std::tie(min_cost_block, min_cost) = get_min_cost_block_in_bin(src_bin,
                                                                   target_bin,
                                                                   block_masses,
                                                                   p_placement,
                                                                   netlist);
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
    PrimitiveVector weight_vec = src_bin.utilization;
    weight_vec.project(block_masses[min_cost_block]);
    float weight = weight_vec.manhattan_norm();
    // Return the overall cost which is the quadratic movement times the weight.
    return weight * min_cost;
}

std::vector<std::vector<LegalizerBinId>> FlowBasedLegalizer::get_paths(LegalizerBinId src_bin_id,
                                                                       const PartialPlacement& p_placement,
                                                                       float psi) {
    VTR_LOGV(log_verbosity_ >= 20, "\tGetting paths...\n");
    // Create a visited vector.
    vtr::vector_map<LegalizerBinId, bool> bin_visited(bins_.size(), false);
    bin_visited[src_bin_id] = true;
    // Create a cost array. The cost of a path is equal to the cost of its tail
    // bin.
    vtr::vector_map<LegalizerBinId, float> bin_cost(bins_.size(), 0.f);
    // Create a starting path.
    std::vector<LegalizerBinId> starting_path;
    starting_path.push_back(src_bin_id);
    // Create a FIFO queue.
    std::queue<std::vector<LegalizerBinId>> queue;
    queue.push(std::move(starting_path));
    // Create the resulting vector of paths.
    // TODO: Can we store this more efficiently as a tree?
    std::vector<std::vector<LegalizerBinId>> paths;
    // Perform the BFS to search for direct paths to flow the starting bin's
    // supply of primitives until it has found sufficient demand.
    PrimitiveVector demand;
    const PrimitiveVector& starting_bin_supply = bins_[src_bin_id].supply;
    while (!queue.empty() && demand < starting_bin_supply) {
        // Pop the current bin off the queue.
        std::vector<LegalizerBinId> &p = queue.front();
        LegalizerBinId tail_bin_id = p.back();
        // Look over its neighbors
        for (LegalizerBinId neighbor_bin_id : bins_[tail_bin_id].neighbors) {
            // If this bin has already been visited, skip it.
            if (bin_visited[neighbor_bin_id])
                continue;
            // Compute the cost of moving a block from the tail bin to its
            // neighbor.
            float cost = compute_cost(bins_[tail_bin_id],
                                      bins_[neighbor_bin_id],
                                      psi,
                                      block_masses_,
                                      p_placement,
                                      netlist_);
            // If the cost is infinite, then the path cannot be made to this
            // neighbor bin.
            if (std::isinf(cost))
                continue;
            // Else, a path can be made.
            std::vector<LegalizerBinId> p_copy(p);
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
            PrimitiveVector neighbor_demand = bins_[neighbor_bin_id].demand;
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
    std::sort(paths.begin(), paths.end(), [&](const std::vector<LegalizerBinId>& a,
                                              const std::vector<LegalizerBinId>& b) {
        return bin_cost[a.back()] < bin_cost[b.back()];
    });

    return paths;
}

void FlowBasedLegalizer::flow_blocks_along_path(const std::vector<LegalizerBinId>& path,
                                                const PartialPlacement& p_placement,
                                                float psi) {
    // Get the root bin of the path.
    VTR_ASSERT(!path.empty());
    LegalizerBinId src_bin_id = path[0];
    // Create a stack and put the src bin on top.
    std::stack<LegalizerBinId> s;
    s.push(src_bin_id);
    // Insert the bins in the path into the stack in reverse order (so the last
    // bin in the path is on top of the stack).
    size_t path_size = path.size();
    for (size_t path_idx = 1; path_idx < path_size; path_idx++) {
        LegalizerBinId sink_bin_id = path[path_idx];
        // Check that the cost of moving a block from the source bin to the sink
        // bin is non-infinite. According to the paper, this check is needed
        // since a previous flow on another path may have made this path not
        // necessary anymore.
        float cost = compute_cost(bins_[src_bin_id], bins_[sink_bin_id], psi,
                                  block_masses_, p_placement, netlist_);
        if (std::isinf(cost))
            return;
        src_bin_id = sink_bin_id;
        s.push(sink_bin_id);
    }
    // Congo line the blocks along the path, starting from the tail and moving
    // forward.
    LegalizerBinId sink_bin_id = s.top();
    s.pop();
    while (!s.empty()) {
        src_bin_id = s.top();
        s.pop();
        // Minor change to the algorithm proposed by Darav et al., find the
        // closest point in src to sink and move it to sink (instead of sorting
        // the whole list which is wasteful).
        // TODO: Verify this. This is not the same as what was in the original
        //       algorithm.
        std::pair<APBlockId, float> p = get_min_cost_block_in_bin(bins_[src_bin_id],
                                                                  bins_[sink_bin_id],
                                                                  block_masses_,
                                                                  p_placement,
                                                                  netlist_);
        // Move the block from the src bin to the sink bin.
        remove_blk_from_bin(p.first, src_bin_id);
        insert_blk_into_bin(p.first, sink_bin_id);

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

/**
 * @brief Debug method to print the current number of blocks contained in each
 *        bin visually.
 *
 * This method helps to see how the spreading is working.
 */
static void print_flow_based_bin_grid(const vtr::NdMatrix<LegalizerBinId, 2>& tile_bin,
                                       const vtr::vector_map<LegalizerBinId, LegalizerBin>& bins) {
    for (size_t y = 0; y < tile_bin.dim_size(1); y++) {
        for (size_t x = 0; x < tile_bin.dim_size(0); x++) {
            const LegalizerBin& bin = bins[tile_bin[x][y]];
            VTR_LOG("%3zu ", bin.contained_blocks.size());
        }
        VTR_LOG("\n");
    }
    VTR_LOG("\n");
}

void FlowBasedLegalizer::legalize(PartialPlacement &p_placement) {
    VTR_LOGV(log_verbosity_ >= 10, "Running Flow-Based Legalizer\n");

    // Reset the bins from the previous iteration and prepare for this iteration.
    reset_bins();
    // Import the partial placement into bins.
    import_placement_into_bins(p_placement);
    // Verify that the placement was imported correctly.
    VTR_ASSERT_SAFE(verify_bins());

    // Print the number of blocks in each bin visually before spreading.
    if (log_verbosity_ >= 15) {
        VTR_LOG("Bin utilization prior to spreading:\n");
        print_flow_based_bin_grid(tile_bin_, bins_);
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
        if (overfilled_bins_.empty()) {
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
        std::vector<LegalizerBinId> overfilled_bins_vec(overfilled_bins_.begin(), overfilled_bins_.end());
        std::sort(overfilled_bins_vec.begin(), overfilled_bins_vec.end(), [&](LegalizerBinId a, LegalizerBinId b) {
            return bins_[a].supply.manhattan_norm() < bins_[b].supply.manhattan_norm();
        });
        // Get the paths to flow blocks from the overfilled bins to the under
        // filled bins and flow the blocks.
        for (LegalizerBinId src_bin_id : overfilled_bins_vec) {
            // Get the list of candidate paths based on psi. A path is a list
            // of LegalizerBins traversed.
            //  NOTE: The paths are sorted by increasing cost within the
            //        getPaths method.
            std::vector<std::vector<LegalizerBinId>> paths = get_paths(src_bin_id,
                                                                       p_placement,
                                                                       psi);

            VTR_LOGV(log_verbosity_ >= 20, "\tNum paths: %zu\n", paths.size());
            // For each path, flow the blocks along the path.
            for (const std::vector<LegalizerBinId>& path : paths) {
                VTR_LOGV(log_verbosity_ >= 30, "\t\tPath length: %zu\n", path.size());
                // If the bin is no longer overfilled, no need to move any more
                // blocks along the paths.
                if (!bin_is_overfilled(src_bin_id))
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
                    bins_[overfilled_bins_vec.back()].supply.manhattan_norm(),
                    psi);
        }

        // Increment the iteration.
        flowBasedIter++;
    }
    VTR_LOGV(log_verbosity_ >= 10,
             "Flow-Based Legalizer finished in %zu iterations.\n",
             flowBasedIter + 1);

    // Verify that the bins are valid before export.
    VTR_ASSERT(verify_bins());

    // Print the number of blocks in each bin after spreading.
    if (log_verbosity_ >= 15) {
        VTR_LOG("Bin utilization after spreading:\n");
        print_flow_based_bin_grid(tile_bin_, bins_);
    }

    // Export the legalized placement to the partial placement.
    export_placement_from_bins(p_placement);
}

