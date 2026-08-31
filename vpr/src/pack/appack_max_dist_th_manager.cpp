/**
 * @file
 * @author  Alex Singer
 * @date    May 2025
 * @brief   Definition of the max distance threshold manager class.
 */

#include "appack_max_dist_th_manager.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "ap_argparse_utils.h"
#include "arch_util.h"
#include "device_grid.h"
#include "physical_types.h"
#include "physical_types_util.h"
#include "vpr_error.h"
#include "vpr_utils.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_prefix_sum.h"

void APPackMaxDistThManager::init(const std::vector<std::string>& max_dist_ths,
                                  const std::vector<t_logical_block_type>& logical_block_types,
                                  const DeviceGrid& device_grid) {
    // Compute the max device distance based on the width and height of the
    // device. This is the L1 (manhattan) distance.
    max_distance_on_device_ = device_grid.width() + device_grid.height();

    // Automatically set the max distance thresholds.
    auto_set_max_distance_thresholds(logical_block_types, device_grid);

    // If the max distance threshold strings have been set (they are not set to
    // auto), set the max distance thresholds based on the user-provided strings.
    VTR_ASSERT(!max_dist_ths.empty());
    if (max_dist_ths.size() != 1 || max_dist_ths[0] != "auto") {
        set_max_distance_thresholds_from_strings(max_dist_ths, logical_block_types);
    }

    // We build prefix sums across the device grid for each logical block type.
    // This is used to quickly lookup distances across the device, taking into
    // account any "holes". For example, columns of DSPs would have large empty
    // spaces between them; these prefix sums try to take these into account.
    // NOTE: We build this at the beginning even though the device size may change
    //       during packing. That should be fine since the flat placement is relative
    //       to this device size.
    logical_block_dist_lookups_.resize(logical_block_types.size());
    for (const t_logical_block_type& lb_type : logical_block_types) {
        VTR_ASSERT(lb_type.index < static_cast<int>(logical_block_dist_lookups_.size()));
        // To speed up the search below, we create a set of equivalent physical tile types
        // that can contain this logical block type.
        std::unordered_set<int> equivalent_tile_indices;
        for (t_physical_tile_type_ptr equiv_tile : lb_type.equivalent_tiles) {
            equivalent_tile_indices.insert(equiv_tile->index);
        }
        // We create prefix sums per layer, since different layers can have different layouts.
        logical_block_dist_lookups_[lb_type.index].resize(device_grid.get_num_layers());
        for (size_t layer_num = 0; layer_num < device_grid.get_num_layers(); layer_num++) {
            logical_block_dist_lookups_[lb_type.index][layer_num] = vtr::PrefixSum2D<unsigned>(
                device_grid.width(), device_grid.height(), [&](size_t x, size_t y) {
                    // Check if the tile at the given x, y location can accomodate the given logical block.
                    t_physical_tile_loc loc(x, y, layer_num);
                    t_physical_tile_type_ptr tile_type = device_grid.get_physical_type(loc);
                    bool is_compatible_tile = equivalent_tile_indices.contains(tile_type->index);
                    // If the tile is compatible, we put a 1 in the prefix sum, 0 otherwise.
                    // When we lookup into the prefix sum for distance, we use this quantity to
                    // decide how many compatible tiles are between two points.
                    return is_compatible_tile ? 1 : 0;
                });
        }
    }

    // Set the initialized flag to true.
    is_initialized_ = true;

    // Log the max distance thresholds for each logical block type. This is
    // similar to how the input and output pin utilizations are printed.
    VTR_LOG("APPack is using max distance thresholds: ");
    for (const t_logical_block_type& lb_ty : logical_block_types) {
        if (lb_ty.is_empty())
            continue;
        VTR_LOG("%s:%g ",
                lb_ty.name.c_str(),
                get_max_dist_threshold(lb_ty));
    }
    VTR_LOG("\n");
}

void APPackMaxDistThManager::auto_set_max_distance_thresholds(const std::vector<t_logical_block_type>& logical_block_types,
                                                              const DeviceGrid& device_grid) {

    // Compute the max distance thresholds of the different logical block types.
    float default_max_distance_th = std::max(default_max_dist_th_scale_ * max_distance_on_device_,
                                             default_max_dist_th_offset_);
    float logic_block_max_distance_th = std::max(logic_block_max_dist_th_scale_ * max_distance_on_device_,
                                                 logic_block_max_dist_th_offset_);
    float memory_max_distance_th = std::max(memory_max_dist_th_scale_ * max_distance_on_device_,
                                            memory_max_dist_th_offset_);
    float io_block_max_distance_th = std::max(io_max_dist_th_scale_ * max_distance_on_device_,
                                              io_max_dist_th_offset_);

    // Set all logical block types to have the default max distance threshold.
    logical_block_dist_thresholds_.resize(logical_block_types.size(), default_max_distance_th);

    // Find which (if any) of the logical block types most looks like a CLB block.
    t_logical_block_type_ptr logic_block_type = infer_logic_block_type(device_grid);

    // Go through each of the logical block types.
    for (const t_logical_block_type& lb_ty : logical_block_types) {
        // Skip the empty logical block type. This should not have any blocks.
        if (lb_ty.is_empty())
            continue;

        // Find which type(s) this logical block type looks like.
        bool has_memory = pb_type_contains_memory_pbs(lb_ty.pb_type);
        bool is_logic_block_type = (lb_ty.index == logic_block_type->index);
        bool is_io_block = pick_physical_type(&lb_ty)->is_io();

        // Update the max distance threshold based on the type. If the logical
        // block type looks like many block types at the same time (for example
        // a CLB which has memory slices within it), then take the average
        // of the max distance thresholds of those types.
        float max_distance_th_sum = 0.0f;
        unsigned block_category_count = 0;
        if (is_logic_block_type) {
            max_distance_th_sum += logic_block_max_distance_th;
            block_category_count++;
        }
        if (has_memory) {
            max_distance_th_sum += memory_max_distance_th;
            block_category_count++;
        }
        if (is_io_block) {
            max_distance_th_sum += io_block_max_distance_th;
            block_category_count++;
        }
        if (block_category_count > 0) {
            logical_block_dist_thresholds_[lb_ty.index] = max_distance_th_sum / static_cast<float>(block_category_count);
        }
    }
}

void APPackMaxDistThManager::set_max_distance_thresholds_from_strings(
    const std::vector<std::string>& max_dist_ths,
    const std::vector<t_logical_block_type>& logical_block_types) {

    std::vector<std::string> lb_type_names;
    std::unordered_map<std::string, int> lb_type_name_to_index;
    for (const t_logical_block_type& lb_ty : logical_block_types) {
        lb_type_names.push_back(lb_ty.name);
        lb_type_name_to_index[lb_ty.name] = lb_ty.index;
    }

    auto lb_to_floats_map = key_to_float_argument_parser(max_dist_ths, lb_type_names, 2);

    for (const auto& lb_name_to_floats_pair : lb_to_floats_map) {
        const std::string& lb_name = lb_name_to_floats_pair.first;
        const std::vector<float>& lb_floats = lb_name_to_floats_pair.second;
        VTR_ASSERT(lb_floats.size() == 2);
        float logical_block_max_dist_th_scale = lb_floats[0];
        float logical_block_max_dist_th_offset = lb_floats[1];

        if (logical_block_max_dist_th_scale < 0.0) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "APPack: Cannot have negative max distance threshold scale");
        }
        if (logical_block_max_dist_th_offset < 0.0) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "APPack: Cannot have negative max distance threshold offset");
        }

        // Compute the max distance threshold the user selected.
        float logical_block_max_dist_th = std::max(max_distance_on_device_ * logical_block_max_dist_th_scale,
                                                   logical_block_max_dist_th_offset);

        int lb_ty_index = lb_type_name_to_index[lb_name];
        logical_block_dist_thresholds_[lb_ty_index] = logical_block_max_dist_th;
    }
}

/**
 * @brief Get the number of tiles compatible with the given prefix sum lookup
 *        that lie strictly between loc1 and loc2 along the cheaper of the two
 *        axis-aligned L-shaped paths, within a single layer.
 *
 * Neither the loc1 tile nor the loc2 tile is counted.
 *
 * There are two ways to walk an L-shaped path between loc1 and loc2: move in X
 * then Y, or move in Y then X. This tries both and returns the min.
 *
 * The loc1 and loc2 coordinates are assumed to lie within the device grid.
 */
static unsigned get_num_tiles_between(const vtr::PrefixSum2D<unsigned>& lookup,
                                      const t_flat_pl_loc& loc1,
                                      const t_flat_pl_loc& loc2) {
    size_t x1 = static_cast<size_t>(loc1.x);
    size_t y1 = static_cast<size_t>(loc1.y);
    size_t x2 = static_cast<size_t>(loc2.x);
    size_t y2 = static_cast<size_t>(loc2.y);

    size_t x_lo = std::min(x1, x2);
    size_t x_hi = std::max(x1, x2);
    size_t y_lo = std::min(y1, y2);
    size_t y_hi = std::max(y1, y2);

    // The corner is only a real intermediate tile if the path turns. If the
    // path is axis-aligned, the "corner" is one of the endpoints and must not
    // be counted.
    bool path_turns = (x1 != x2) && (y1 != y2);

    // Path 1: walk along X at row y1, then along Y at column x2. The corner
    // tile is at (x2, y1). The X-leg and Y-leg interiors exclude the endpoints
    // and the corner; the corner is added back separately when the path turns.
    unsigned x_then_y = 0;
    if (x_hi - x_lo >= 2)
        x_then_y += lookup.get_sum(x_lo + 1, y1, x_hi - 1, y1);
    if (y_hi - y_lo >= 2)
        x_then_y += lookup.get_sum(x2, y_lo + 1, x2, y_hi - 1);
    if (path_turns)
        x_then_y += lookup.get_sum(x2, y1, x2, y1);

    // Path 2: walk along Y at column x1, then along X at row y2. The corner
    // tile is at (x1, y2).
    unsigned y_then_x = 0;
    if (y_hi - y_lo >= 2)
        y_then_x += lookup.get_sum(x1, y_lo + 1, x1, y_hi - 1);
    if (x_hi - x_lo >= 2)
        y_then_x += lookup.get_sum(x_lo + 1, y2, x_hi - 1, y2);
    if (path_turns)
        y_then_x += lookup.get_sum(x1, y2, x1, y2);

    return std::min(x_then_y, y_then_x);
}

float APPackMaxDistThManager::get_distance_between_points(const t_flat_pl_loc& loc1,
                                                          const t_flat_pl_loc& loc2,
                                                          t_logical_block_type_ptr lb_type) const {
    // NOTE: It is assumed that loc1 and loc2 are on the device. This is currently
    //       guaranteed by how flat placements are currently constructed. The code
    //       that looks up into the prefix sums has asserts within it. It is
    //       challenging to put an assert here for this case since the device size
    //       can change during placement.

    // This returns the manhattan distance between loc1 and loc2 in valid tile
    // locations. Tiles that cannot accomodate the given logical block type count
    // 0 towards the distance; this makes blocks separated by "holes" (such as
    // different DSP columns) appear as far apart as they actually are.
    VTR_ASSERT_SAFE(lb_type != nullptr);
    VTR_ASSERT_SAFE(lb_type->index < static_cast<int>(logical_block_dist_lookups_.size()));
    const std::vector<vtr::PrefixSum2D<unsigned>>& lb_dist_lookup = logical_block_dist_lookups_[lb_type->index];

    // If both points are in the same tile, there is no distance between them.
    if (std::floor(loc1.x) == std::floor(loc2.x)
        && std::floor(loc1.y) == std::floor(loc2.y)
        && std::floor(loc1.layer) == std::floor(loc2.layer)) {
        return 0.0f;
    }

    size_t layer1 = static_cast<size_t>(loc1.layer);
    size_t layer2 = static_cast<size_t>(loc2.layer);
    unsigned z_dist = static_cast<unsigned>(std::abs(loc2.layer - loc1.layer));

    // Count the compatible tiles strictly between the two points, ignoring the
    // z axis. When the points are on different layers, project the 2D path onto
    // each endpoint's layer, take the cheaper of the two, and add the z hops
    // separately. This under-counts some 3D paths that would turn on a third
    // layer, but that is acceptable for max distance thresholding.
    unsigned tiles_between = get_num_tiles_between(lb_dist_lookup[layer1], loc1, loc2);
    if (layer1 != layer2) {
        tiles_between = std::min(tiles_between,
                                 get_num_tiles_between(lb_dist_lookup[layer2], loc1, loc2));
    }

    // Add 1 for the cost of leaving loc1's tile. Without this, two compatible
    // tiles right next to each other would appear as a distance of 0. loc1 and
    // loc2 are known to be different tiles here.
    float manh_dist = static_cast<float>(z_dist + tiles_between + 1);

    // TODO: We are ignoring the intra-tile distances. The code above assumes that the locations are integer-aligned,
    //       but that is not the case in AP. Frankly it should not make a huge difference, but it should be investigated.
    //       The result may be off by up to ~1 per axis due to where within its tile each location sits.

    return manh_dist;
}
