#include "clustered_netlist.h"
#include "flat_placement_types.h"
#include "atom_netlist_fwd.h"
#include "flat_placement_utils.h"
#include "physical_types_util.h"
#include "place_macro.h"
#include "vtr_assert.h"
#include "vtr_geometry.h"
#include "vtr_ndmatrix.h"
#include "vtr_random.h"
#include "vtr_time.h"
#include "vpr_types.h"

#include "globals.h"
#include "read_place.h"
#include "initial_placement.h"
#include "initial_noc_placment.h"
#include "vpr_utils.h"
#include "place_util.h"
#include "place_constraints.h"
#include "move_utils.h"
#include "region.h"
#include "noc_place_utils.h"
#include "vtr_vector.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <optional>
#include <queue>
#include <vector>

#ifdef VERBOSE
void print_clb_placement(const char* fname);
#endif

// Number of iterations that initial placement tries to place all blocks before throwing an error
static constexpr int MAX_INIT_PLACE_ATTEMPTS = 2;

// The amount of weight that will be added to previous unplaced block scores to ensure that failed blocks would be placed earlier next iteration
static constexpr int SORT_WEIGHT_PER_FAILED_BLOCK = 10;

// The amount of weight that will be added to each tile which is outside the floorplanning constraints
static constexpr int SORT_WEIGHT_PER_TILES_OUTSIDE_OF_PR = 100;

// The range limit to be used when searching for a neighbor in the centroid placement.
// The neighbor location should be within the defined range to the calculated centroid location.
static constexpr int CENTROID_NEIGHBOR_SEARCH_RLIM = 15;

/**
 * @brief Control routine for placing a macro.
 * First iteration of place_marco performs the following steps to place a macro:
 *  1) try_centroid_placement : tries to find a location based on the macro's logical connections.
 *  2) try_place_macro_randomly : if no smart location found in the centroid placement, the function tries
 *  to place it randomly for the max number of tries.
 *  3) try_place_macro_exhaustively : if neither placement algorithms work, the function will find a location
 *  for the macro by exhaustively searching all available locations.
 * If first iteration failed, next iteration calls dense placement for specific block types.
 *
 *   @param macros_max_num_tries Max number of tries for initial placement before switching to exhaustive placement.
 *   @param pl_macro The macro to be placed.
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *   @param blk_types_empty_locs_in_grid First location (lowest y) and number of remaining blocks in each column for the blk_id type.
 *   @param block_scores The block_scores (ranking of what to place next) for unplaced blocks connected to this macro should be updated.
 *   @param blk_loc_registry Placement block location information. To be filled with the location
 *   where pl_macro is placed.
 *   @param rng A random number generator.
 *
 * @return true if macro was placed, false if not.
 */
static bool place_macro(int macros_max_num_tries,
                        const t_pl_macro& pl_macro,
                        e_pad_loc_type pad_loc_type,
                        std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid,
                        vtr::vector<ClusterBlockId, t_block_score>& block_scores,
                        BlkLocRegistry& blk_loc_registry,
                        const FlatPlacementInfo& flat_placement_info,
                        vtr::RngContainer& rng);

/*
 * Assign scores to each block based on macro size and floorplanning constraints.
 * Used for relative placement, so that the blocks that are more difficult to place can be placed first during initial placement.
 * A higher score indicates that the block is more difficult to place.
 */
static vtr::vector<ClusterBlockId, t_block_score> assign_block_scores(const PlaceMacros& place_macros);

/**
 * @brief Tries to find y coordinate for macro head location based on macro direction
 *
 *   @param first_macro_loc The first available location that can place the macro blocks.
 *   @param pl_macro The macro to be placed.
 *   @param blk_loc_registry Placement block location information. To be filled with the location
 *   where pl_macro is placed.
 *
 * @return y coordinate of the location that macro head should be placed
 */
static int get_y_loc_based_on_macro_direction(t_grid_empty_locs_block_type first_macro_loc,
                                              const t_pl_macro& pl_macro);

/**
 * @brief Tries to get the first available location of a specific block type that can accommodate macro blocks
 *
 *   @param loc The first available location that can place the macro blocks.
 *   @param pl_macro The macro to be placed.
 *   @param blk_types_empty_locs_in_grid first location (lowest y) and number of remaining blocks in each column for the blk_id type
 *
 * @return index to a column of blk_types_empty_locs_in_grid that can accommodate pl_macro and location of first available location returned by reference
 */
static int get_blk_type_first_loc(t_pl_loc& loc, const t_pl_macro& pl_macro, std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid);

/**
 * @brief Updates the first available location (lowest y) and number of remaining blocks in the column that dense placement used to place the macro.
 *
 *   @param blk_type_column_index Index to a column in blk_types_empty_locs_in_grid that placed pl_macro in itself.
 *   @param block_type Logical block type of the macro blocks.
 *   @param pl_macro The macro to be placed.
 *   @param blk_types_empty_locs_in_grid first location (lowest y) and number of remaining blocks in each column for the blk_id type
 *
 */
static void update_blk_type_first_loc(int blk_type_column_index,
                                      t_logical_block_type_ptr block_type,
                                      const t_pl_macro& pl_macro,
                                      std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid);

/**
 * @brief  Initializes empty locations of the grid with a specific block type into vector for dense initial placement
 *
 *   @param block_type_index block type index that failed in previous initial placement iterations
 *
 * @return first location (lowest y) and number of remaining blocks in each column for the block_type_index
 */
static std::vector<t_grid_empty_locs_block_type> init_blk_types_empty_locations(int block_type_index);

/**
 * @brief  Helper function used when IO locations are to be randomly locked
 *
 *   @param pl_macro The macro to be fixed.
 *   @param loc The location at which the head of the macro is placed.
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *   @param block_locs Clustered block locations used to mark the IO blocks that are to be placed
 *   randomly as fixed.
 */
static inline void fix_IO_block_types(const t_pl_macro& pl_macro,
                                      t_pl_loc loc,
                                      e_pad_loc_type pad_loc_type,
                                      vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs);

/**
 * @brief  Determine whether a specific macro can be placed in a specific location.
 *
 *   @param loc The location at which the macro head member is placed.
 *   @param pr The PartitionRegion of the macro head member - represents its floorplanning constraints, is the size of
 *   the whole chip if the macro is not constrained.
 *   @param block_type Logical block type of the macro head member.
 *
 * @return True if the location is legal for the macro head member, false otherwise.
 */
static bool is_loc_legal(const t_pl_loc& loc,
                         const PartitionRegion& pr,
                         t_logical_block_type_ptr block_type);

/**
 * @brief  Helper function to choose a subtile in specified location if the type is compatible and an available one exists.
 *
 *   @param centroid The centroid location at which the subtile will be selected using its x, y, and layer.
 *   @param block_type Logical block type we would like to place here.
 *   @param block_loc_registry Information on where other blocks have been placed.
 *   @param pr The PartitionRegion of the block we are trying to place - represents its floorplanning constraints;
 *   it is the size of the whole chip if the block is not constrained.
 *   @param rng A random number generator to select a subtile from the available and compatible ones.
 *
 * @return True if the location is on the chip, legal, and at least one available subtile is found at that location;
 * false otherwise.
 */
static bool find_subtile_in_location(t_pl_loc& centroid,
                                     t_logical_block_type_ptr block_type,
                                     const BlkLocRegistry& blk_loc_registry,
                                     const PartitionRegion& pr,
                                     vtr::RngContainer& rng);

/**
 * @brief Calculates a centroid location for a block based on its placed connections.
 *
 *   @param pl_macro The macro to be placed.
 *   @param centroid specified location (x,y,subtile) for the pl_macro head member.
 *   @param blk_loc_registry Placement block location information. To be filled with the location
 *   where pl_macro is placed.
 *
 * @return a vector of blocks that are connected to this block but not yet placed so their scores can later be updated.
 */
static std::vector<ClusterBlockId> find_centroid_loc(const t_pl_macro& pl_macro,
                                                     t_pl_loc& centroid,
                                                     const BlkLocRegistry& blk_loc_registry);

/**
 * @brief  Tries to find a nearest location to the centroid location if calculated centroid location is not legal or is occupied.
 *
 *   @param centroid_loc Calculated location in try_centroid_placement function for the block.
 *   @param block_type Logical block type of the macro blocks.
 *   @param search_for_empty If set, the function tries to find an empty location.
 *   @param blk_loc_registry Placement block location information. To be filled with the location
 *   where pl_macro is placed.
 *
 * @return true if the function can find any location near the centroid one, false otherwise.
 */
static bool find_centroid_neighbor(ClusterBlockId block_id,
                                   t_pl_loc& centroid_loc,
                                   t_logical_block_type_ptr block_type,
                                   bool search_for_empty,
                                   int r_lim,
                                   const BlkLocRegistry& blk_loc_registry,
                                   vtr::RngContainer& rng);

/**
 * @brief  tries to place a macro at a centroid location of its placed connections.
 *  
 *   @param block_id The block to be placed.
 *   @param pl_macro The macro to be placed.
 *   @param pr The PartitionRegion of the macro - represents its floorplanning constraints, is the size of the whole chip if the macro is not
 *   constrained.
 *   @param block_type Logical block type of the macro blocks.
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *   @param block_scores The block_scores (ranking of what to place next) for unplaced blocks connected to this macro are updated in this routine.
 *   @param blk_loc_registry Placement block location information. To be filled with the location
 *   where pl_macro is placed.
 *   @param rng A random number generator for choosing a compatible subtile randomly.
 *
 * @return true if the macro gets placed, false if not.
 */
static bool try_centroid_placement(ClusterBlockId block_id,
                                   const t_pl_macro& pl_macro,
                                   const PartitionRegion& pr,
                                   t_logical_block_type_ptr block_type,
                                   e_pad_loc_type pad_loc_type,
                                   vtr::vector<ClusterBlockId, t_block_score>& block_scores,
                                   BlkLocRegistry& blk_loc_registry,
                                   const FlatPlacementInfo& flat_placement_info,
                                   vtr::RngContainer& rng);

/**
 * @brief Looks for a valid placement location for macro in second iteration, tries to place as many macros as possible in one column
 * and avoids fragmenting the available locations in one column.
 *
 *   @param pl_macro The macro to be placed.
 *   @param pr The PartitionRegion of the macro - represents its floorplanning constraints, is the size of the whole chip if the macro is not
 *   constrained.
 *   @param block_type Logical block type of the macro blocks.
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *   @param blk_types_empty_locs_in_grid first location (lowest y) and number of remaining blocks in each column for the blk_id type
 *   @param blk_loc_registry Placement block location information. To be filled with the location
 *   where pl_macro is placed.
 *
 * @return true if the macro gets placed, false if not.
 */
static bool try_dense_placement(const t_pl_macro& pl_macro,
                                const PartitionRegion& pr,
                                t_logical_block_type_ptr block_type,
                                e_pad_loc_type pad_loc_type,
                                std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid,
                                BlkLocRegistry& blk_loc_registry);

/**
 * @brief Tries for MAX_INIT_PLACE_ATTEMPTS times to place all blocks considering their floorplanning constraints and the device size
 *
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *   @param constraints_file Used to read block locations if any constraints is available.
 *   @param blk_loc_registry Placement block location information. To be filled with the location
 *   where pl_macro is placed.
 *   @param rng A random number generator.
 */
static void place_all_blocks(const t_placer_opts& placer_opts,
                             vtr::vector<ClusterBlockId, t_block_score>& block_scores,
                             e_pad_loc_type pad_loc_type,
                             const char* constraints_file,
                             BlkLocRegistry& blk_loc_registry,
                             const PlaceMacros& place_macros,
                             const FlatPlacementInfo& flat_placement_info,
                             vtr::RngContainer& rng);

/**
 * @brief If any blocks remain unplaced after all initial placement iterations, this routine
 * throws an error indicating that initial placement can not be done with the current device size or
 * floorplanning constraints.
 */
static void check_initial_placement_legality(const BlkLocRegistry& blk_loc_registry);

static void check_initial_placement_legality(const BlkLocRegistry& blk_loc_registry) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& block_locs = blk_loc_registry.block_locs();

    int unplaced_blocks = 0;

    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        if (block_locs[blk_id].loc.x == INVALID_X) {
            VTR_LOG("Block %s (# %d) of type %s could not be placed during initial placement iteration %d\n",
                    cluster_ctx.clb_nlist.block_name(blk_id).c_str(),
                    blk_id,
                    cluster_ctx.clb_nlist.block_type(blk_id)->name.c_str(),
                    MAX_INIT_PLACE_ATTEMPTS - 1);
            unplaced_blocks++;
        }
    }

    if (unplaced_blocks > 0) {
        VPR_FATAL_ERROR(VPR_ERROR_PLACE,
                        "%d blocks could not be placed during initial placement; no spaces were available for them on the grid.\n"
                        "If VPR was run with floorplan constraints, the constraints may be too tight.\n",
                        unplaced_blocks);
    }

    for (auto movable_blk_id : blk_loc_registry.movable_blocks()) {
        if (block_locs[movable_blk_id].is_fixed) {
            VPR_FATAL_ERROR(VPR_ERROR_PLACE, "Fixed block was mistakenly marked as movable during initial placement.\n");
        }
    }

    for (const auto& logical_block_type : device_ctx.logical_block_types) {
        const auto& movable_blocks_of_type = blk_loc_registry.movable_blocks_per_type()[logical_block_type.index];
        for (const auto& movable_blk_id : movable_blocks_of_type) {
            if (block_locs[movable_blk_id].is_fixed) {
                VPR_FATAL_ERROR(VPR_ERROR_PLACE, "Fixed block %d of logical type %s was mistakenly marked as movable during initial placement.\n",
                                (size_t)movable_blk_id, logical_block_type.name.c_str());
            }
            if (cluster_ctx.clb_nlist.block_type(movable_blk_id)->index != logical_block_type.index) {
                VPR_FATAL_ERROR(VPR_ERROR_PLACE, "Clustered block %d of logical type %s was mistakenly marked as logical type %s.\n",
                                (size_t)movable_blk_id,
                                cluster_ctx.clb_nlist.block_type(movable_blk_id)->name.c_str(),
                                logical_block_type.name.c_str());
            }
        }
    }
}

bool is_block_placed(ClusterBlockId blk_id,
                     const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs) {
    return (block_locs[blk_id].loc.x != INVALID_X);
}

static bool is_loc_legal(const t_pl_loc& loc,
                         const PartitionRegion& pr,
                         t_logical_block_type_ptr block_type) {
    const auto& grid = g_vpr_ctx.device().grid;
    bool legal = false;

    //Check if the location is within its constraint region
    for (const auto& reg : pr.get_regions()) {
        const vtr::Rect<int>& reg_rect = reg.get_rect();
        const auto [layer_low, layer_high] = reg.get_layer_range();

        if (loc.layer > layer_high || loc.layer < layer_low) {
            continue;
        }

        if (reg_rect.coincident({loc.x, loc.y})) {
            //check if the location is compatible with the block type
            const auto& type = grid.get_physical_type({loc.x, loc.y, loc.layer});
            int height_offset = grid.get_height_offset({loc.x, loc.y, loc.layer});
            int width_offset = grid.get_width_offset({loc.x, loc.y, loc.layer});
            if (is_tile_compatible(type, block_type)) {
                //Check if the location is an anchor position
                if (height_offset == 0 && width_offset == 0) {
                    legal = true;
                    break;
                }
            }
        }
    }

    return legal;
}

bool find_subtile_in_location(t_pl_loc& centroid,
                              t_logical_block_type_ptr block_type,
                              const BlkLocRegistry& blk_loc_registry,
                              const PartitionRegion& pr,
                              vtr::RngContainer& rng) {
    //check if the location is on chip and legal, if yes try to update subtile
    if (is_loc_on_chip({centroid.x, centroid.y, centroid.layer}) && is_loc_legal(centroid, pr, block_type)) {
        //find the compatible subtiles
        const auto& device_ctx = g_vpr_ctx.device();
        const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[block_type->index];
        const auto& type = device_ctx.grid.get_physical_type({centroid.x, centroid.y, centroid.layer});
        const auto& compatible_sub_tiles = compressed_block_grid.compatible_sub_tile_num(type->index);

        //filter out the occupied subtiles
        const GridBlock& grid_blocks = blk_loc_registry.grid_blocks();
        std::vector<int> available_sub_tiles;
        available_sub_tiles.reserve(compatible_sub_tiles.size());
        for (int sub_tile : compatible_sub_tiles) {
            t_pl_loc pos = {centroid.x, centroid.y, sub_tile, centroid.layer};
            if (!grid_blocks.block_at_location(pos)) {
                available_sub_tiles.push_back(sub_tile);
            }
        }

        if (!available_sub_tiles.empty()) {
            centroid.sub_tile = available_sub_tiles[rng.irand((int)available_sub_tiles.size() - 1)];
            return true;
        }
    }

    return false;
}

static bool find_centroid_neighbor(ClusterBlockId block_id,
                                   t_pl_loc& centroid_loc,
                                   t_logical_block_type_ptr block_type,
                                   bool search_for_empty,
                                   int rlim,
                                   const BlkLocRegistry& blk_loc_registry,
                                   vtr::RngContainer& rng) {
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[block_type->index];
    const int num_layers = g_vpr_ctx.device().grid.get_num_layers();
    const int centroid_loc_layer_num = centroid_loc.layer;

    //Determine centroid location in the compressed space of the current block
    auto compressed_centroid_loc = get_compressed_loc_approx(compressed_block_grid,
                                                             centroid_loc,
                                                             num_layers);

    //range limit (rlim) set a limit for the neighbor search in the centroid placement
    //the neighbor location should be within the defined range to calculated centroid location
    int first_rlim = rlim;

    auto search_range = get_compressed_grid_target_search_range(compressed_block_grid,
                                                                compressed_centroid_loc[centroid_loc_layer_num],
                                                                first_rlim);

    int delta_cx = search_range.xmax - search_range.xmin;

    bool block_constrained = is_cluster_constrained(block_id);

    if (block_constrained) {
        bool intersect = intersect_range_limit_with_floorplan_constraints(block_id,
                                                                          search_range,
                                                                          delta_cx,
                                                                          centroid_loc_layer_num);
        if (!intersect) {
            return false;
        }
    }

    //Block has not been placed yet, so the "from" coords will be (-1, -1)
    int cx_from = OPEN;
    int cy_from = OPEN;
    int layer_from = centroid_loc_layer_num;

    t_physical_tile_loc to_compressed_loc;

    bool legal = find_compatible_compressed_loc_in_range(block_type,
                                                         delta_cx,
                                                         {cx_from, cy_from, layer_from},
                                                         search_range,
                                                         to_compressed_loc,
                                                         /*is_median=*/false,
                                                         centroid_loc_layer_num,
                                                         search_for_empty,
                                                         blk_loc_registry,
                                                         rng,
                                                         block_constrained);

    if (!legal) {
        return false;
    }

    compressed_grid_to_loc(block_type, to_compressed_loc, centroid_loc, rng);

    return legal;
}

static std::vector<ClusterBlockId> find_centroid_loc(const t_pl_macro& pl_macro,
                                                     t_pl_loc& centroid,
                                                     const BlkLocRegistry& blk_loc_registry) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& block_locs = blk_loc_registry.block_locs();

    float acc_weight = 0;
    float acc_x = 0;
    float acc_y = 0;
    bool find_layer = false;
    std::vector<int> layer_count(g_vpr_ctx.device().grid.get_num_layers(), 0);

    ClusterBlockId head_blk = pl_macro.members.at(0).blk_index;
    // For now, we put the macro in the same layer as the head block
    int head_layer_num = block_locs[head_blk].loc.layer;
    // If block is placed, we use the layer of the block. Otherwise, the layer will be determined later
    if (head_layer_num == OPEN) {
        find_layer = true;
    }
    std::vector<ClusterBlockId> connected_blocks_to_update;

    //iterate over the from block pins
    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.block_pins(head_blk)) {
        ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(pin_id);

        /* Ignore the special case nets which only connects a block to itself  *
         * Experimentally, it was found that this case greatly degrade QoR     */
        if (cluster_ctx.clb_nlist.net_sinks(net_id).size() == 1) {
            ClusterBlockId source = cluster_ctx.clb_nlist.net_driver_block(net_id);
            ClusterPinId sink_pin = *cluster_ctx.clb_nlist.net_sinks(net_id).begin();
            ClusterBlockId sink = cluster_ctx.clb_nlist.pin_block(sink_pin);
            if (sink == source) {
                continue;
            }
        }

        //if the pin is driver iterate over all the sinks
        if (cluster_ctx.clb_nlist.pin_type(pin_id) == PinType::DRIVER) {
            //ignore nets that are globally routed
            if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
                continue;
            }
            for (ClusterPinId sink_pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
                /* Ignore if one of the sinks is the block itself*/
                if (pin_id == sink_pin_id)
                    continue;

                if (!is_block_placed(cluster_ctx.clb_nlist.pin_block(sink_pin_id), block_locs)) {
                    //add unplaced block to connected_blocks_to_update vector to update its score later.
                    connected_blocks_to_update.push_back(cluster_ctx.clb_nlist.pin_block(sink_pin_id));
                    continue;
                }

                t_physical_tile_loc tile_loc = blk_loc_registry.get_coordinate_of_pin(sink_pin_id);
                if (find_layer) {
                    VTR_ASSERT(tile_loc.layer_num != OPEN);
                    layer_count[tile_loc.layer_num]++;
                }
                acc_x += tile_loc.x;
                acc_y += tile_loc.y;
                acc_weight++;
            }
        }

        //else the pin is sink --> only care about its driver
        else {
            ClusterPinId source_pin = cluster_ctx.clb_nlist.net_driver(net_id);
            if (!is_block_placed(cluster_ctx.clb_nlist.pin_block(source_pin), block_locs)) {
                //add unplaced block to connected_blocks_to_update vector to update its score later.
                connected_blocks_to_update.push_back(cluster_ctx.clb_nlist.pin_block(source_pin));
                continue;
            }

            t_physical_tile_loc tile_loc = blk_loc_registry.get_coordinate_of_pin(source_pin);
            if (find_layer) {
                VTR_ASSERT(tile_loc.layer_num != OPEN);
                layer_count[tile_loc.layer_num]++;
            }
            acc_x += tile_loc.x;
            acc_y += tile_loc.y;
            acc_weight++;
        }
    }

    //Calculate the centroid location
    if (acc_weight > 0) {
        centroid.x = acc_x / acc_weight;
        centroid.y = acc_y / acc_weight;
        if (find_layer) {
            auto max_element = std::max_element(layer_count.begin(), layer_count.end());
            VTR_ASSERT((*max_element) != 0);
            centroid.layer = (int)std::distance(layer_count.begin(), max_element);
        } else {
            centroid.layer = head_layer_num;
        }
    }

    return connected_blocks_to_update;
}

/**
 * @brief Helper method for getting the flat position of an atom relative to a
 *        given offset.
 *
 * This method is useful for chained blocks where an atom may be a member of a
 * chain with the given offset. This gives the atom's position relative to the
 * head macro's tile location.
 */
static t_flat_pl_loc get_atom_relative_flat_loc(AtomBlockId atom_blk_id,
                                                const t_pl_offset& offset,
                                                const FlatPlacementInfo& flat_placement_info,
                                                const DeviceGrid& device_grid) {
    // Get the flat location of the atom and the offset.
    t_flat_pl_loc atom_pos = flat_placement_info.get_pos(atom_blk_id);
    t_flat_pl_loc flat_offset = t_flat_pl_loc((float)offset.x,
                                              (float)offset.y,
                                              (float)offset.layer);
    // Get the position of the head macro of the chain that this atom is a part of.
    atom_pos -= flat_offset;

    // This may put the atom off device (due to the flat placement not being fully
    // legal), so we clamp this to be within the device.
    atom_pos.x = std::clamp(atom_pos.x, 0.0f, (float)device_grid.width() - 0.001f);
    atom_pos.y = std::clamp(atom_pos.y, 0.0f, (float)device_grid.height() - 0.001f);
    atom_pos.layer = std::clamp(atom_pos.layer, 0.0f, (float)device_grid.get_num_layers() - 0.001f);

    return atom_pos;
}

// TODO: Should this return the unplaced_blocks_to_update_their_score?
static t_flat_pl_loc find_centroid_loc_from_flat_placement(const t_pl_macro& pl_macro,
                                                           const FlatPlacementInfo& flat_placement_info) {
    // Use the flat placement to compute the centroid of the given macro.
    // TODO: Instead of averaging, maybe use MODE (most frequently placed location).
    const DeviceGrid& device_grid = g_vpr_ctx.device().grid;
    unsigned acc_weight = 0;
    t_flat_pl_loc centroid({0.0f, 0.0f, 0.0f});
    for (const t_pl_macro_member& member : pl_macro.members) {
        const auto& cluster_atoms = g_vpr_ctx.clustering().atoms_lookup[member.blk_index];
        for (AtomBlockId atom_blk_id : cluster_atoms) {
            // TODO: We can get away with using less information.
            VTR_ASSERT(flat_placement_info.blk_x_pos[atom_blk_id] != FlatPlacementInfo::UNDEFINED_POS && flat_placement_info.blk_y_pos[atom_blk_id] != FlatPlacementInfo::UNDEFINED_POS && flat_placement_info.blk_layer[atom_blk_id] != FlatPlacementInfo::UNDEFINED_POS && flat_placement_info.blk_sub_tile[atom_blk_id] != FlatPlacementInfo::UNDEFINED_SUB_TILE);

            // Accumulate the x, y, layer, and sub_tile for each atom in each
            // member of the macro. The position should be relative to the head
            // macro's position such that the centroid is where all blocks think
            // the head macro should be.
            t_flat_pl_loc atom_pos = get_atom_relative_flat_loc(atom_blk_id,
                                                                member.offset,
                                                                flat_placement_info,
                                                                device_grid);
            centroid += atom_pos;
            acc_weight++;
        }
    }
    if (acc_weight > 0) {
        centroid /= static_cast<float>(acc_weight);
    }

    // If the root cluster is constrained, project the centroid onto its
    // partition region. This will move the centroid position to the closest
    // position within the partition region.
    ClusterBlockId head_cluster_id = pl_macro.members[0].blk_index;
    if (is_cluster_constrained(head_cluster_id)) {
        // Get the partition region of the head. This is the partition region
        // that affects the entire macro.
        const PartitionRegion& head_pr = g_vpr_ctx.floorplanning().cluster_constraints[head_cluster_id];
        // For each region, find the closest point in that region to the centroid
        // and save the closest of all regions.
        t_flat_pl_loc best_projected_pos = centroid;
        float best_distance = std::numeric_limits<float>::max();
        VTR_ASSERT_MSG(centroid.layer == 0,
                       "3D FPGAs not supported for this part of the code yet");
        for (const Region& region : head_pr.get_regions()) {
            const vtr::Rect<int>& rect = region.get_rect();
            // Note: We add 0.999 here since the partition region is in grid
            //       space, so it treats tile positions as having size 0x0 when
            //       they really are 1x1.
            float proj_x = std::clamp<float>(centroid.x, rect.xmin(), rect.xmax() + 0.999);
            float proj_y = std::clamp<float>(centroid.y, rect.ymin(), rect.ymax() + 0.999);
            float dx = std::abs(proj_x - centroid.x);
            float dy = std::abs(proj_y - centroid.y);
            float dist = dx + dy;
            if (dist < best_distance) {
                best_projected_pos.x = proj_x;
                best_projected_pos.y = proj_y;
                best_distance = dist;
            }
        }
        VTR_ASSERT_SAFE(best_distance != std::numeric_limits<float>::max());
        // Return the point within the partition region that is closest to the
        // original centroid.
        return best_projected_pos;
    }

    return centroid;
}

/**
 * @brief Returns the first available sub_tile (both compatible with the given
 *        compressed grid and is empty according the the blk_loc_registry) in
 *        the tile at the given grid_loc. Returns OPEN if no such sub_tile exists.
 */
static inline int get_first_available_sub_tile_at_grid_loc(const t_physical_tile_loc& grid_loc,
                                                           const BlkLocRegistry& blk_loc_registry,
                                                           const DeviceGrid& device_grid,
                                                           const t_compressed_block_grid& compressed_block_grid) {

    // Get the compatible sub-tiles from the compressed grid for this physical
    // tile type.
    const t_physical_tile_type_ptr phy_type = device_grid.get_physical_type(grid_loc);
    const auto& compatible_sub_tiles = compressed_block_grid.compatible_sub_tile_num(phy_type->index);

    // Return the first empty sub-tile from this list.
    for (int sub_tile : compatible_sub_tiles) {
        if (blk_loc_registry.grid_blocks().is_sub_tile_empty(grid_loc, sub_tile)) {
            return sub_tile;
        }
    }

    // If one cannot be found, return OPEN.
    return OPEN;
}

/**
 * @brief Find the nearest compatible location for the given macro as close to
 *        the src_flat_loc as possible.
 *
 * This method uses a BFS to find the closest legal location for the macro.
 *
 *  @param src_flat_loc
 *          The start location of the BFS. This is given as a flat placement to
 *          allow the search to trade-off different location options. For example,
 *          if src_loc was (1.6, 1.5), this tells the search that the cluster
 *          would prefer to be at tile (1, 1), but if it cannot go there and
 *          it had to go to one of the neighbors, it would prefer to be on the
 *          right.
 *  @param block_type
 *          The logical block type of the macro.
 *  @param macro
 *          The macro to place in the location.
 *  @param blk_loc_registry
 *
 *  @return Returns the closest legal location found. All of the dimensions will
 *          be OPEN if a locations could not be found.
 */
static inline t_pl_loc find_nearest_compatible_loc(const t_flat_pl_loc& src_flat_loc,
                                                   float max_displacement_threshold,
                                                   t_logical_block_type_ptr block_type,
                                                   const t_pl_macro& pl_macro,
                                                   const BlkLocRegistry& blk_loc_registry) {
    // This method performs a BFS over the compressed grid. This avoids searching
    // locations which obviously cannot implement this macro.
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[block_type->index];
    const DeviceGrid& device_grid = g_vpr_ctx.device().grid;
    const int num_layers = device_grid.get_num_layers();
    // This method does not support 3D FPGAs yet. The search performed will only
    // traverse the same layer as the src_loc.
    VTR_ASSERT(num_layers == 1);
    constexpr int layer = 0;

    // Get the closest (approximately) compressed location to the src location.
    // This does not need to be perfect (in fact I do not think it is), but the
    // closer it is, the faster the BFS will find the best solution.
    t_physical_tile_loc src_grid_loc(src_flat_loc.x, src_flat_loc.y, src_flat_loc.layer);
    const t_physical_tile_loc compressed_src_loc = compressed_block_grid.grid_loc_to_compressed_loc_approx(src_grid_loc);

    // Weighted-BFS search the compressed grid for an empty compatible subtile.
    size_t num_rows = compressed_block_grid.get_num_rows(layer);
    size_t num_cols = compressed_block_grid.get_num_columns(layer);
    vtr::NdMatrix<bool, 2> visited({num_cols, num_rows}, false);
    float best_dist = std::numeric_limits<float>::max();
    t_pl_loc best_loc(OPEN, OPEN, OPEN, OPEN);

    std::queue<t_physical_tile_loc> loc_queue;
    loc_queue.push(compressed_src_loc);
    while (!loc_queue.empty()) {
        // Pop the top element off the queue.
        t_physical_tile_loc loc = loc_queue.front();
        loc_queue.pop();

        // If this location has already been visited, skip it.
        if (visited[loc.x][loc.y])
            continue;
        visited[loc.x][loc.y] = true;

        // Get the minimum distance the cluster would need to move (relative to
        // its global placement solution) to be within the tile at the given
        // location.
        // Note: In compressed space, distances are not what they appear. We are
        //       using the true grid positions to get the truly closest loc.
        auto grid_loc = compressed_block_grid.compressed_loc_to_grid_loc(loc);
        float grid_dist = get_manhattan_distance_to_tile(src_flat_loc,
                                                         grid_loc,
                                                         device_grid);
        // If this distance is worst than the best we have seen.
        // NOTE: This prune is always safe (i.e. it will never remove a better
        //       solution) since this is a spatial graph and our objective is
        //       positional distance. The un-visitied neighbors of a node should
        //       have a higher distance than the current node.
        if (grid_dist >= best_dist)
            continue;

        // If this distance is beyond the max_displacement_threshold, drop this
        // location.
        if (grid_dist > max_displacement_threshold)
            continue;

        // In order to ensure our BFS finds the closest compatible location, we
        // traverse compressed grid locations which may not actually be valid
        // (i.e. no tile exists there). This is fine, we just need to check for
        // them to ensure we never try to put a cluster there.
        bool is_valid_compressed_loc = false;
        const auto& compressed_col_blk_map = compressed_block_grid.get_column_block_map(loc.x, layer);
        if (compressed_col_blk_map.count(loc.y) != 0)
            is_valid_compressed_loc = true;

        // If this distance is better than the best we have seen so far, try
        // to see if this is a better solution.
        if (is_valid_compressed_loc) {
            // Get a sub-tile at this location if it is available.
            int new_sub_tile = get_first_available_sub_tile_at_grid_loc(grid_loc,
                                                                        blk_loc_registry,
                                                                        device_grid,
                                                                        compressed_block_grid);
            if (new_sub_tile != OPEN) {
                // If a sub-tile is available, set this to be the first sub-tile
                // available and check if this site is legal for this macro.
                // Note: We are using the fully legality check here to check for
                //       floorplanning constraints and compatibility for all
                //       members of the macro. This prevents some macros being
                //       placed where they obviously cannot be implemented.
                t_pl_loc new_loc = t_pl_loc(grid_loc.x, grid_loc.y, new_sub_tile, grid_loc.layer_num);
                bool site_legal_for_macro = macro_can_be_placed(pl_macro,
                                                                new_loc,
                                                                true /*check_all_legality*/,
                                                                blk_loc_registry);
                if (site_legal_for_macro) {
                    // Update the best solition.
                    // Note: We need to keep searching since the compressed grid
                    //       may present a location which is closer in compressed
                    //       space earlier than a location which is closer in
                    //       grid space.
                    best_dist = grid_dist;
                    best_loc = new_loc;
                }
            }
        }

        // Push the neighbors (in the compressed grid) onto the queue.
        // This will push the neighbors left, right, above, and below the current
        // location. Some of these locations may not exist or may have already
        // been visited. The code above checks for these cases to prevent extra
        // work and invalid lookups. This must be done this way to ensure that
        // the closest location can be found efficiently.
        if (loc.x > 0) {
            t_physical_tile_loc new_comp_loc = t_physical_tile_loc(loc.x - 1,
                                                                   loc.y,
                                                                   loc.layer_num);
            loc_queue.push(new_comp_loc);
        }
        if (loc.x < (int)num_cols - 1) {
            t_physical_tile_loc new_comp_loc = t_physical_tile_loc(loc.x + 1,
                                                                   loc.y,
                                                                   loc.layer_num);
            loc_queue.push(new_comp_loc);
        }
        if (loc.y > 0) {
            t_physical_tile_loc new_comp_loc = t_physical_tile_loc(loc.x,
                                                                   loc.y - 1,
                                                                   loc.layer_num);
            loc_queue.push(new_comp_loc);
        }
        if (loc.y < (int)num_rows - 1) {
            t_physical_tile_loc new_comp_loc = t_physical_tile_loc(loc.x,
                                                                   loc.y + 1,
                                                                   loc.layer_num);
            loc_queue.push(new_comp_loc);
        }
    }

    return best_loc;
}

static bool try_centroid_placement(ClusterBlockId block_id,
                                   const t_pl_macro& pl_macro,
                                   const PartitionRegion& pr,
                                   t_logical_block_type_ptr block_type,
                                   e_pad_loc_type pad_loc_type,
                                   vtr::vector<ClusterBlockId, t_block_score>& block_scores,
                                   BlkLocRegistry& blk_loc_registry,
                                   const FlatPlacementInfo& flat_placement_info,
                                   vtr::RngContainer& rng) {
    auto& block_locs = blk_loc_registry.mutable_block_locs();

    t_pl_loc centroid_loc(OPEN, OPEN, OPEN, OPEN);
    std::vector<ClusterBlockId> unplaced_blocks_to_update_their_score;

    bool found_legal_subtile = false;

    int rlim = CENTROID_NEIGHBOR_SEARCH_RLIM;
    if (!flat_placement_info.valid) {
        // If a flat placement is not provided, use the centroid of connected
        // blocks which have already been placed.
        unplaced_blocks_to_update_their_score = find_centroid_loc(pl_macro, centroid_loc, blk_loc_registry);
        found_legal_subtile = find_subtile_in_location(centroid_loc, block_type, blk_loc_registry, pr, rng);
    } else {
        // If a flat placement is provided, use the flat placement to get the
        // centroid location of the macro.
        t_flat_pl_loc centroid_flat_loc = find_centroid_loc_from_flat_placement(pl_macro, flat_placement_info);
        // Then find the nearest legal location to this centroid for this macro.
        centroid_loc = find_nearest_compatible_loc(centroid_flat_loc,
                                                   static_cast<float>(rlim),
                                                   block_type,
                                                   pl_macro,
                                                   blk_loc_registry);
        // FIXME: After this point, if the find_nearest_compatible_loc function
        //        could not find a valid location, then nothing should be able to.
        //        Also the location it returns will be on the chip and in the PR
        //        by construction. Could save time by skipping those checks if
        //        needed.
        if (centroid_loc.x == OPEN) {
            // If we cannot find a nearest block, fall back on the original
            // find_centroid_loc function.
            // FIXME: We should really just skip this block and come back
            //        to it later. We do not want it taking space from
            //        someone else!
            unplaced_blocks_to_update_their_score = find_centroid_loc(pl_macro, centroid_loc, blk_loc_registry);
            found_legal_subtile = find_subtile_in_location(centroid_loc, block_type, blk_loc_registry, pr, rng);
        } else {
            found_legal_subtile = true;
        }
    }

    //no suggestion was available for this block type
    if (!is_loc_on_chip({centroid_loc.x, centroid_loc.y, centroid_loc.layer})) {
        return false;
    }

    //centroid suggestion was either occupied or does not match block type
    //try to find a near location that meet these requirements
    if (!found_legal_subtile) {
        bool neighbor_legal_loc = find_centroid_neighbor(block_id, centroid_loc, block_type, false, rlim, blk_loc_registry, rng);
        if (!neighbor_legal_loc) { //no neighbor candidate found
            return false;
        }
    }

    //no neighbor were found that meet all our requirements, should be placed with random placement
    if (!is_loc_on_chip({centroid_loc.x, centroid_loc.y, centroid_loc.layer}) || !pr.is_loc_in_part_reg(centroid_loc)) {
        return false;
    }

    auto& device_ctx = g_vpr_ctx.device();
    int width_offset = device_ctx.grid.get_width_offset({centroid_loc.x, centroid_loc.y, centroid_loc.layer});
    int height_offset = device_ctx.grid.get_height_offset({centroid_loc.x, centroid_loc.y, centroid_loc.layer});
    VTR_ASSERT(width_offset == 0);
    VTR_ASSERT(height_offset == 0);

    bool legal = try_place_macro(pl_macro, centroid_loc, blk_loc_registry);

    if (legal) {
        fix_IO_block_types(pl_macro, centroid_loc, pad_loc_type, block_locs);

        //after placing the current block, its connections' score must be updated.
        for (ClusterBlockId blk_id : unplaced_blocks_to_update_their_score) {
            block_scores[blk_id].number_of_placed_connections++;
        }
    }
    return legal;
}

static int get_y_loc_based_on_macro_direction(t_grid_empty_locs_block_type first_macro_loc, const t_pl_macro& pl_macro) {
    int y = first_macro_loc.first_avail_loc.y;

    /* if the macro member offset is positive, it means that macro head should be placed at the first location of first_macro_loc.
     * otherwise, macro head should be placed at the last available location to ensure macro_can_be_placed can check macro location correctly.
     */
    if (pl_macro.members.size() > 1) {
        if (pl_macro.members.at(1).offset.y < 0) {
            y += (pl_macro.members.size() - 1) * abs(pl_macro.members.at(1).offset.y);
        }
    }

    return y;
}

static void update_blk_type_first_loc(int blk_type_column_index,
                                      t_logical_block_type_ptr block_type,
                                      const t_pl_macro& pl_macro,
                                      std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid) {
    //check if dense placement could place macro successfully
    if (blk_type_column_index == -1 || blk_types_empty_locs_in_grid->size() <= (size_t)abs(blk_type_column_index)) {
        return;
    }

    const auto& device_ctx = g_vpr_ctx.device();

    //update the first available macro location in a specific column for the next macro
    blk_types_empty_locs_in_grid->at(blk_type_column_index).first_avail_loc.y += device_ctx.physical_tile_types.at(block_type->index).height * pl_macro.members.size();
    blk_types_empty_locs_in_grid->at(blk_type_column_index).num_of_empty_locs_in_y_axis -= pl_macro.members.size();
}

static int get_blk_type_first_loc(t_pl_loc& loc,
                                  const t_pl_macro& pl_macro,
                                  std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid) {
    //loop over all empty locations and choose first column that can accommodate macro blocks
    for (unsigned int empty_loc_index = 0; empty_loc_index < blk_types_empty_locs_in_grid->size(); empty_loc_index++) {
        auto first_empty_loc = blk_types_empty_locs_in_grid->at(empty_loc_index);

        //if macro size is larger than available locations in the specific column, should go to next available column
        if ((unsigned)first_empty_loc.num_of_empty_locs_in_y_axis < pl_macro.members.size()) {
            continue;
        }

        //set the coordinate of first location that can accommodate macro blocks
        loc.x = first_empty_loc.first_avail_loc.x;
        loc.y = get_y_loc_based_on_macro_direction(first_empty_loc, pl_macro);
        loc.layer = first_empty_loc.first_avail_loc.layer;
        loc.sub_tile = first_empty_loc.first_avail_loc.sub_tile;

        return empty_loc_index;
    }

    return -1;
}

static std::vector<t_grid_empty_locs_block_type> init_blk_types_empty_locations(int block_type_index) {
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[block_type_index];
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& grid = device_ctx.grid;
    int num_layers = grid.get_num_layers();

    //create a vector to store all columns containing block_type_index with their lowest y and number of remaining blocks
    std::vector<t_grid_empty_locs_block_type> block_type_empty_locs;

    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        int min_cx = compressed_block_grid.grid_loc_to_compressed_loc_approx({0, OPEN, layer_num}).x;
        int max_cx = compressed_block_grid.grid_loc_to_compressed_loc_approx({(int)grid.width() - 1, OPEN, layer_num}).x;

        //traverse all column and store their empty locations in block_type_empty_locs
        for (int x_loc = min_cx; x_loc <= max_cx; x_loc++) {
            t_grid_empty_locs_block_type empty_loc;
            const auto& block_rows = compressed_block_grid.get_column_block_map(x_loc, layer_num);
            auto first_avail_loc = block_rows.begin()->second;
            empty_loc.first_avail_loc.x = first_avail_loc.x;
            empty_loc.first_avail_loc.y = first_avail_loc.y;
            empty_loc.first_avail_loc.layer = first_avail_loc.layer_num;
            const auto& physical_type = grid.get_physical_type({first_avail_loc.x, first_avail_loc.y, first_avail_loc.layer_num});
            const auto& compatible_sub_tiles = compressed_block_grid.compatible_sub_tile_num(physical_type->index);
            empty_loc.first_avail_loc.sub_tile = *std::min_element(compatible_sub_tiles.begin(), compatible_sub_tiles.end());
            empty_loc.num_of_empty_locs_in_y_axis = block_rows.size();
            block_type_empty_locs.push_back(empty_loc);
        }
    }

    return block_type_empty_locs;
}

static inline void fix_IO_block_types(const t_pl_macro& pl_macro,
                                      t_pl_loc loc,
                                      e_pad_loc_type pad_loc_type,
                                      vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs) {
    const auto& device_ctx = g_vpr_ctx.device();

    // If the user marked the IO block pad_loc_type as RANDOM, that means it should be randomly
    // placed and then stay fixed to that location, which is why the macro members are marked as fixed.
    const t_physical_tile_type_ptr type = device_ctx.grid.get_physical_type({loc.x, loc.y, loc.layer});
    if (type->is_io() && pad_loc_type == e_pad_loc_type::RANDOM) {
        for (const t_pl_macro_member& pl_macro_member : pl_macro.members) {
            block_locs[pl_macro_member.blk_index].is_fixed = true;
        }
    }
}

bool try_place_macro_randomly(const t_pl_macro& pl_macro,
                              const PartitionRegion& pr,
                              t_logical_block_type_ptr block_type,
                              e_pad_loc_type pad_loc_type,
                              BlkLocRegistry& blk_loc_registry,
                              vtr::RngContainer& rng) {
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[block_type->index];

    /*
     * Getting various values needed for the find_compatible_compressed_loc_in_range() routine called below.
     * Need to pass the from/to coords of the block, as well as the min/max x and y coords of where it can
     * be placed in the compressed grid.
     */

    //Block has not been placed yet, so the "from" coords will be (-1, -1)
    int cx_from = -1;
    int cy_from = -1;

    //If the block has more than one floorplan region, pick a random region to get the min/max x and y values
    int region_index;
    const std::vector<Region>& regions = pr.get_regions();
    if (regions.size() > 1) {
        region_index = rng.irand(regions.size() - 1);
    } else {
        region_index = 0;
    }
    const Region& reg = regions[region_index];

    const vtr::Rect<int>& reg_rect = reg.get_rect();
    const auto [layer_low, layer_high] = reg.get_layer_range();

    int selected_layer = (layer_low == layer_high) ? layer_low : layer_low + rng.irand(layer_high - layer_low);

    auto min_compressed_loc = compressed_block_grid.grid_loc_to_compressed_loc_approx({reg_rect.xmin(), reg_rect.ymin(), selected_layer});

    auto max_compressed_loc = compressed_block_grid.grid_loc_to_compressed_loc_approx({reg_rect.xmax(), reg_rect.ymax(), selected_layer});

    int delta_cx = max_compressed_loc.x - min_compressed_loc.x;

    t_physical_tile_loc to_compressed_loc;

    bool legal;

    // is_fixed_range is true since even if the block is not constrained,
    // the search range covers the entire region, so there is no need for
    // the search range to be adjusted
    legal = find_compatible_compressed_loc_in_range(block_type,
                                                    delta_cx,
                                                    {cx_from, cy_from, selected_layer},
                                                    {min_compressed_loc.x, max_compressed_loc.x,
                                                     min_compressed_loc.y, max_compressed_loc.y,
                                                     selected_layer, selected_layer},
                                                    to_compressed_loc,
                                                    /*is_median=*/false,
                                                    selected_layer,
                                                    /*search_for_empty=*/false,
                                                    blk_loc_registry,
                                                    rng,
                                                    /*is_range_fixed=*/true);

    if (!legal) {
        //No valid position found
        return false;
    }

    t_pl_loc loc;
    compressed_grid_to_loc(block_type, to_compressed_loc, loc, rng);

    auto& device_ctx = g_vpr_ctx.device();

    int width_offset = device_ctx.grid.get_width_offset({loc.x, loc.y, loc.layer});
    int height_offset = device_ctx.grid.get_height_offset({loc.x, loc.y, loc.layer});
    VTR_ASSERT(width_offset == 0);
    VTR_ASSERT(height_offset == 0);

    legal = try_place_macro(pl_macro, loc, blk_loc_registry);

    if (legal) {
        auto& block_locs = blk_loc_registry.mutable_block_locs();
        fix_IO_block_types(pl_macro, loc, pad_loc_type, block_locs);
    }

    return legal;
}

bool try_place_macro_exhaustively(const t_pl_macro& pl_macro,
                                  const PartitionRegion& pr,
                                  t_logical_block_type_ptr block_type,
                                  e_pad_loc_type pad_loc_type,
                                  BlkLocRegistry& blk_loc_registry) {
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[block_type->index];
    auto& block_locs = blk_loc_registry.mutable_block_locs();
    const GridBlock& grid_blocks = blk_loc_registry.grid_blocks();

    const std::vector<Region>& regions = pr.get_regions();

    bool placed = false;

    t_pl_loc to_loc;

    for (unsigned int reg = 0; reg < regions.size() && !placed; reg++) {
        const vtr::Rect<int> reg_rect = regions[reg].get_rect();
        const auto [layer_low, layer_high] = regions[reg].get_layer_range();

        for (int layer_num = layer_low; layer_num <= layer_high; layer_num++) {
            int min_cx = compressed_block_grid.grid_loc_to_compressed_loc_approx({reg_rect.xmin(), OPEN, layer_num}).x;
            int max_cx = compressed_block_grid.grid_loc_to_compressed_loc_approx({reg_rect.xmax(), OPEN, layer_num}).x;

            // There isn't any block of this type in this region
            if (min_cx == OPEN) {
                VTR_ASSERT(max_cx == OPEN);
                continue;
            }

            for (int cx = min_cx; cx <= max_cx && !placed; cx++) {
                const auto& block_rows = compressed_block_grid.get_column_block_map(cx, layer_num);
                auto y_lower_iter = block_rows.begin();
                auto y_upper_iter = block_rows.end();

                int y_range = std::distance(y_lower_iter, y_upper_iter);

                VTR_ASSERT(y_range >= 0);

                for (int dy = 0; dy < y_range && !placed; dy++) {
                    int cy = (y_lower_iter + dy)->first;

                    auto grid_loc = compressed_block_grid.compressed_loc_to_grid_loc({cx, cy, layer_num});
                    to_loc.x = grid_loc.x;
                    to_loc.y = grid_loc.y;
                    to_loc.layer = grid_loc.layer_num;

                    auto& grid = g_vpr_ctx.device().grid;
                    auto tile_type = grid.get_physical_type({to_loc.x, to_loc.y, layer_num});

                    if (regions[reg].get_sub_tile() != NO_SUBTILE) {
                        int subtile = regions[reg].get_sub_tile();

                        to_loc.sub_tile = subtile;
                        if (grid_blocks.block_at_location(to_loc) == ClusterBlockId::INVALID()) {
                            placed = try_place_macro(pl_macro, to_loc, blk_loc_registry);

                            if (placed) {
                                fix_IO_block_types(pl_macro, to_loc, pad_loc_type, block_locs);
                            }
                        }
                    } else {
                        for (const auto& sub_tile : tile_type->sub_tiles) {
                            if (is_sub_tile_compatible(tile_type, block_type, sub_tile.capacity.low)) {
                                int st_low = sub_tile.capacity.low;
                                int st_high = sub_tile.capacity.high;

                                for (int st = st_low; st <= st_high && !placed; st++) {
                                    to_loc.sub_tile = st;
                                    if (grid_blocks.block_at_location(to_loc) == ClusterBlockId::INVALID()) {
                                        placed = try_place_macro(pl_macro, to_loc, blk_loc_registry);
                                        if (placed) {
                                            fix_IO_block_types(pl_macro, to_loc, pad_loc_type, block_locs);
                                        }
                                    }
                                }
                            }

                            if (placed) {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    return placed;
}

static bool try_dense_placement(const t_pl_macro& pl_macro,
                                const PartitionRegion& pr,
                                t_logical_block_type_ptr block_type,
                                e_pad_loc_type pad_loc_type,
                                std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid,
                                BlkLocRegistry& blk_loc_registry) {
    t_pl_loc loc;
    int column_index = get_blk_type_first_loc(loc, pl_macro, blk_types_empty_locs_in_grid);

    //check if first available location is within the chip and macro's partition region, otherwise placement is not legal
    if (!is_loc_on_chip({loc.x, loc.y, loc.layer}) || !pr.is_loc_in_part_reg(loc)) {
        return false;
    }

    auto& device_ctx = g_vpr_ctx.device();

    int width_offset = device_ctx.grid.get_width_offset({loc.x, loc.y, loc.layer});
    int height_offset = device_ctx.grid.get_height_offset({loc.x, loc.y, loc.layer});
    VTR_ASSERT(width_offset == 0);
    VTR_ASSERT(height_offset == 0);

    bool legal = try_place_macro(pl_macro, loc, blk_loc_registry);

    if (legal) {
        auto& block_locs = blk_loc_registry.mutable_block_locs();
        fix_IO_block_types(pl_macro, loc, pad_loc_type, block_locs);
    }

    //Dense placement found a legal position for pl_macro;
    //We need to update first available location (lowest y) and number of remaining blocks for the next macro.
    update_blk_type_first_loc(column_index, block_type, pl_macro, blk_types_empty_locs_in_grid);
    return legal;
}

bool try_place_macro(const t_pl_macro& pl_macro,
                     t_pl_loc head_pos,
                     BlkLocRegistry& blk_loc_registry) {
    bool f_placer_debug = g_vpr_ctx.placement().f_placer_debug;
    const GridBlock& grid_blocks = blk_loc_registry.grid_blocks();

    VTR_LOGV_DEBUG(f_placer_debug, "\t\t\t\tTry to place the macro at %dx%dx%dx%d\n",
                   head_pos.x,
                   head_pos.y,
                   head_pos.sub_tile,
                   head_pos.layer);

    bool macro_placed = false;

    // If that location is occupied, do nothing.
    if (grid_blocks.block_at_location(head_pos)) {
        return macro_placed;
    }

    if (macro_can_be_placed(pl_macro, head_pos, /*check_all_legality=*/true, blk_loc_registry)) {
        macro_placed = true;
        VTR_LOGV_DEBUG(f_placer_debug, "\t\t\t\tMacro is placed at the given location\n");
        for (const t_pl_macro_member& pl_macro_member : pl_macro.members) {
            t_pl_loc member_pos = head_pos + pl_macro_member.offset;
            ClusterBlockId iblk = pl_macro_member.blk_index;
            blk_loc_registry.set_block_location(iblk, member_pos);
        } // Finish placing all the members in the macro
    }

    return macro_placed;
}

static bool place_macro(int macros_max_num_tries,
                        const t_pl_macro& pl_macro,
                        enum e_pad_loc_type pad_loc_type,
                        std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid,
                        vtr::vector<ClusterBlockId, t_block_score>& block_scores,
                        BlkLocRegistry& blk_loc_registry,
                        const FlatPlacementInfo& flat_placement_info,
                        vtr::RngContainer& rng) {
    const auto& block_locs = blk_loc_registry.block_locs();
    ClusterBlockId blk_id = pl_macro.members[0].blk_index;
    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\tHead of the macro is Block %d\n", size_t(blk_id));

    if (is_block_placed(blk_id, block_locs)) {
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\t\tBlock is already placed\n", size_t(blk_id));
        return true;
    }

    bool macro_placed = false;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();

    // Assume that all the blocks in the macro are of the same type
    auto block_type = cluster_ctx.clb_nlist.block_type(blk_id);

    const PartitionRegion& pr = (is_cluster_constrained(blk_id)) ? floorplanning_ctx.cluster_constraints[blk_id]
                                                                 : get_device_partition_region();

    //Enough to check head member of macro to see if its constrained because
    //constraints propagation was done earlier in initial placement.
    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug && is_cluster_constrained(blk_id),
                   "\t\t\tMacro's head is constrained\n");

    //If blk_types_empty_locs_in_grid is not NULL, means that initial placement has been failed in first iteration for this block type
    //We need to place densely in second iteration to be able to find a legal initial placement solution
    if (blk_types_empty_locs_in_grid != nullptr && !blk_types_empty_locs_in_grid->empty()) {
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\t\tTry dense placement\n");
        macro_placed = try_dense_placement(pl_macro, pr, block_type, pad_loc_type, blk_types_empty_locs_in_grid, blk_loc_registry);
    }

    if (!macro_placed) {
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\t\tTry centroid placement\n");
        macro_placed = try_centroid_placement(blk_id, pl_macro, pr, block_type, pad_loc_type, block_scores, blk_loc_registry, flat_placement_info, rng);
    }
    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\t\tMacro is placed: %d\n", macro_placed);
    // If macro is not placed yet, try to place the macro randomly for the max number of random tries
    for (int itry = 0; itry < macros_max_num_tries && !macro_placed; itry++) {
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\t\tTry random place iter: %d\n", itry);
        macro_placed = try_place_macro_randomly(pl_macro, pr, block_type, pad_loc_type, blk_loc_registry, rng);
    } // Finished all tries

    if (!macro_placed) {
        // if a macro still could not be placed after macros_max_num_tries times,
        // go through the chip exhaustively to find a legal placement for the macro
        // place the macro on the first location that is legal
        // then set macro_placed = true;
        // if there are no legal positions, error out

        // Exhaustive placement of carry macros
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\t\tTry exhaustive placement\n");
        macro_placed = try_place_macro_exhaustively(pl_macro, pr, block_type, pad_loc_type, blk_loc_registry);
    }
    return macro_placed;
}

static vtr::vector<ClusterBlockId, t_block_score> assign_block_scores(const PlaceMacros& place_macros) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& floorplan_ctx = g_vpr_ctx.floorplanning();
    ;

    t_block_score score;

    vtr::vector<ClusterBlockId, t_block_score> block_scores;

    block_scores.resize(cluster_ctx.clb_nlist.blocks().size());

    //GridTileLookup class provides info needed for calculating number of tiles covered by a region
    GridTileLookup grid_tiles;

    /*
     * For the blocks with no floorplan constraints, and the blocks that are not part of macros,
     * the block scores will remain at their default values assigned by the constructor
     * (macro_size = 0; floorplan_constraints = 0;
     */

    //go through all blocks and store floorplan constraints score
    //initialize number of placed connections to zero for all blocks
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        block_scores[blk_id].number_of_placed_connections = 0;
        if (is_cluster_constrained(blk_id)) {
            const PartitionRegion& pr = floorplan_ctx.cluster_constraints[blk_id];
            auto block_type = cluster_ctx.clb_nlist.block_type(blk_id);
            double floorplan_score = get_floorplan_score(blk_id, pr, block_type, grid_tiles);
            block_scores[blk_id].tiles_outside_of_floorplan_constraints = floorplan_score;
        }
    }

    //go through placement macros and store size of macro for each block
    for (const auto& pl_macro : place_macros.macros()) {
        int size = pl_macro.members.size();
        for (const auto& pl_macro_member : pl_macro.members) {
            block_scores[pl_macro_member.blk_index].macro_size = size;
        }
    }

    return block_scores;
}

static void place_all_blocks(const t_placer_opts& placer_opts,
                             vtr::vector<ClusterBlockId, t_block_score>& block_scores,
                             enum e_pad_loc_type pad_loc_type,
                             const char* constraints_file,
                             BlkLocRegistry& blk_loc_registry,
                             const PlaceMacros& place_macros,
                             const FlatPlacementInfo& flat_placement_info,
                             vtr::RngContainer& rng) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& device_ctx = g_vpr_ctx.device();
    auto blocks = cluster_ctx.clb_nlist.blocks();

    int number_of_unplaced_blks_in_curr_itr;

    //keep tracks of which block types can not be placed in each iteration
    std::unordered_set<int> unplaced_blk_type_in_curr_itr;

    auto criteria = [&block_scores](ClusterBlockId lhs, ClusterBlockId rhs) {
        int lhs_score = block_scores[lhs].macro_size + block_scores[lhs].number_of_placed_connections + SORT_WEIGHT_PER_TILES_OUTSIDE_OF_PR * block_scores[lhs].tiles_outside_of_floorplan_constraints + SORT_WEIGHT_PER_FAILED_BLOCK * block_scores[lhs].failed_to_place_in_prev_attempts;
        int rhs_score = block_scores[rhs].macro_size + block_scores[rhs].number_of_placed_connections + SORT_WEIGHT_PER_TILES_OUTSIDE_OF_PR * block_scores[rhs].tiles_outside_of_floorplan_constraints + SORT_WEIGHT_PER_FAILED_BLOCK * block_scores[rhs].failed_to_place_in_prev_attempts;

        return lhs_score < rhs_score;
    };

    // Keeps the first locations and number of remained blocks in each column for a specific block type.
    //[0..device_ctx.logical_block_types.size()-1][0..num_of_grid_columns_containing_this_block_type-1]
    std::vector<std::vector<t_grid_empty_locs_block_type>> blk_types_empty_locs_in_grid;

    for (auto iter_no = 0; iter_no < MAX_INIT_PLACE_ATTEMPTS; iter_no++) {
        //clear grid for a new placement iteration
        blk_loc_registry.clear_block_type_grid_locs(unplaced_blk_type_in_curr_itr);
        unplaced_blk_type_in_curr_itr.clear();

        // read the constraint file if the user has provided one and this is not the first attempt
        if (strlen(constraints_file) != 0 && iter_no != 0) {
            read_constraints(constraints_file, blk_loc_registry);
        }

        //resize the vector to store unplaced block types empty locations
        blk_types_empty_locs_in_grid.resize(device_ctx.logical_block_types.size());

        number_of_unplaced_blks_in_curr_itr = 0;

        //calculate heap update frequency based on number of blocks in the design
        int update_heap_freq = std::max((int)(blocks.size() / 100), 1);

        int blocks_placed_since_heap_update = 0;

        std::vector<ClusterBlockId> heap_blocks(blocks.begin(), blocks.end());
        std::make_heap(heap_blocks.begin(), heap_blocks.end(), criteria);

        while (!heap_blocks.empty()) {
            std::pop_heap(heap_blocks.begin(), heap_blocks.end(), criteria);
            auto blk_id = heap_blocks.back();
            heap_blocks.pop_back();

            auto blk_id_type = cluster_ctx.clb_nlist.block_type(blk_id);

            if constexpr (VTR_ENABLE_DEBUG_LOGGING_CONST_EXPR) {
                enable_placer_debug(placer_opts, blk_id);
            }

            VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "Popped Block %d\n", size_t(blk_id));

            blocks_placed_since_heap_update++;

            bool block_placed = place_one_block(blk_id,
                                                pad_loc_type,
                                                &blk_types_empty_locs_in_grid[blk_id_type->index],
                                                &block_scores,
                                                blk_loc_registry,
                                                place_macros,
                                                flat_placement_info,
                                                rng);

            //update heap based on update_heap_freq calculated above
            if (blocks_placed_since_heap_update % (update_heap_freq) == 0) {
                std::make_heap(heap_blocks.begin(), heap_blocks.end(), criteria);
                blocks_placed_since_heap_update = 0;
            }

            if (!block_placed) {
                VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "Didn't find a location the block\n", size_t(blk_id));
                //add current block to list to ensure it will be placed sooner in the next iteration in initial placement
                number_of_unplaced_blks_in_curr_itr++;
                block_scores[blk_id].failed_to_place_in_prev_attempts++;
                int imacro = place_macros.get_imacro_from_iblk(blk_id);
                if (imacro != -1) { //the block belongs to macro that contain a chain, we need to turn on dense placement in next iteration for that type of block
                    unplaced_blk_type_in_curr_itr.insert(blk_id_type->index);
                }
            }
        }

        //current iteration could place all of design's blocks, initial placement succeed
        if (number_of_unplaced_blks_in_curr_itr == 0) {
            VTR_LOG("Initial placement iteration %d has finished successfully\n", iter_no);
            return;
        }

        //loop over block types with macro that have failed to be placed, and add their locations in grid for the next iteration
        for (int itype : unplaced_blk_type_in_curr_itr) {
            blk_types_empty_locs_in_grid[itype] = init_blk_types_empty_locations(itype);
        }

        //print unplaced blocks in the current iteration
        VTR_LOG("Initial placement iteration %d has finished with %d unplaced blocks\n", iter_no, number_of_unplaced_blks_in_curr_itr);
    }
}

/**
 * @brief Gets or creates a macro for the given blk_id.
 *
 * If the block belongs to a macro, this method will return that macro object.
 * Note: This code should not create a copy of that macro object.
 *
 * If the block does not belong to a macro, it will create a "fake" macro that
 * only contains the given block.
 */
static inline t_pl_macro get_or_create_macro(ClusterBlockId blk_id,
                                             const PlaceMacros& place_macros) {
    // Lookup to see if the block is part of a macro
    int imacro = place_macros.get_imacro_from_iblk(blk_id);

    // If the block belongs to a macro, just return it.
    if (imacro != -1) {
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\tBelongs to a macro %d\n", imacro);
        return place_macros[imacro];
    }

    // If it does not belong to a macro, create a macro with the one block and then pass to the placement routines
    // This is done so that the initial placement flow can be the same whether the block belongs to a macro or not
    t_pl_macro_member macro_member;
    macro_member.blk_index = blk_id;
    macro_member.offset = t_pl_offset(0, 0, 0, 0);
    t_pl_macro pl_macro;
    pl_macro.members.push_back(macro_member);
    return pl_macro;
}

bool place_one_block(const ClusterBlockId blk_id,
                     enum e_pad_loc_type pad_loc_type,
                     std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid,
                     vtr::vector<ClusterBlockId, t_block_score>* block_scores,
                     BlkLocRegistry& blk_loc_registry,
                     const PlaceMacros& place_macros,
                     const FlatPlacementInfo& flat_placement_info,
                     vtr::RngContainer& rng) {
    const auto& block_locs = blk_loc_registry.block_locs();

    //Check if block has already been placed
    if (is_block_placed(blk_id, block_locs)) {
        return true;
    }

    // If this cluster block is contained within a macro, return it. If not
    // create a "fake" macro which only contains this block.
    t_pl_macro pl_macro = get_or_create_macro(blk_id, place_macros);

    // Try to place this macro.
    bool placed_macro = place_macro(MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY, pl_macro, pad_loc_type, blk_types_empty_locs_in_grid, *block_scores, blk_loc_registry, flat_placement_info, rng);

    // Return the status of the macro placement.
    return placed_macro;
}

static inline float get_flat_variance(const t_pl_macro& macro,
                                      const FlatPlacementInfo& flat_placement_info) {

    const DeviceGrid& device_grid = g_vpr_ctx.device().grid;

    // Find the flat centroid location of this macro. Then find the grid location
    // that this would be.
    t_flat_pl_loc centroid_flat_loc = find_centroid_loc_from_flat_placement(macro, flat_placement_info);
    t_physical_tile_loc centroid_grid_loc(centroid_flat_loc.x,
                                          centroid_flat_loc.y,
                                          centroid_flat_loc.layer);
    VTR_ASSERT(is_loc_on_chip(centroid_grid_loc));

    // Compute the variance.
    unsigned num_atoms = 0;
    float variance = 0.0f;
    for (const t_pl_macro_member& member : macro.members) {
        const auto& cluster_atoms = g_vpr_ctx.clustering().atoms_lookup[member.blk_index];
        for (AtomBlockId atom_blk_id : cluster_atoms) {
            // Get the atom position, offset by the member offset. This translates
            // all atoms to be as if they are in the head position of the macro.
            t_flat_pl_loc atom_pos = get_atom_relative_flat_loc(atom_blk_id,
                                                                member.offset,
                                                                flat_placement_info,
                                                                device_grid);

            // Get the amount this atom needs to be displaced in order to be
            // within the same tile as the centroid.
            float dist = get_manhattan_distance_to_tile(atom_pos,
                                                        centroid_grid_loc,
                                                        g_vpr_ctx.device().grid);

            // Accumulate the variance.
            variance += (dist * dist);
            num_atoms++;
        }
    }
    if (num_atoms > 0) {
        variance /= static_cast<float>(num_atoms);
    }
    return variance;
}

/**
 * @brief Print the status header for the AP initial placer.
 */
static void print_ap_initial_placer_header() {
    VTR_LOG("---- ---------- ---------- -------------\n");
    VTR_LOG("Pass Max Displ. Num Blocks Num Blocks   \n");
    VTR_LOG("     Threshold  Placed     Left Unplaced\n");
    VTR_LOG("---- ---------- ---------- -------------\n");
}

/**
 * @brief Print the status of the current iteration (pass) of the AP initial
 *        placer.
 */
static void print_ap_initial_placer_status(unsigned iteration,
                                           float max_displacement_threshold,
                                           size_t num_placed,
                                           size_t num_unplaced) {
    // Iteration
    VTR_LOG("%4u", iteration);

    // Max displacement threshold
    VTR_LOG(" %10g", max_displacement_threshold);

    // Num placed
    VTR_LOG(" %10zu", num_placed);

    // Num unplaced
    VTR_LOG(" %13zu", num_unplaced);

    VTR_LOG("\n");

    fflush(stdout);
}

/**
 * @brief Collects unplaced clusters and sorts such that clusters which should
 *        be placed first appear early in the list.
 *
 * The clusters are sorted based on how many clusters are in the macro that
 * contains this cluster and the standard deviation of the placement of atoms
 * within the cluster. Large macros with low standard deviation will be placed
 * first.
 */
static inline std::vector<ClusterBlockId> get_sorted_clusters_to_place(
    BlkLocRegistry& blk_loc_registry,
    const PlaceMacros& place_macros,
    const ClusteredNetlist& cluster_netlist,
    const FlatPlacementInfo& flat_placement_info) {

    const auto& cluster_constraints = g_vpr_ctx.floorplanning().cluster_constraints;

    // Create a list of clusters to place.
    std::vector<ClusterBlockId> clusters_to_place;
    clusters_to_place.reserve(cluster_netlist.blocks().size());
    for (ClusterBlockId blk_id : cluster_netlist.blocks()) {
        if (!is_block_placed(blk_id, blk_loc_registry.block_locs())) {
            clusters_to_place.push_back(blk_id);
        }
    }

    // Get the max macro size. This is used for scoring macros.
    size_t max_macro_size = 1;
    for (const t_pl_macro& macro : place_macros.macros()) {
        max_macro_size = std::max(max_macro_size, macro.members.size());
    }

    // Sort the list of clusters to place based on some criteria. The clusters
    // earlier in the list will get first dibs on where to be placed.
    constexpr float macro_size_weight = 1.0f;
    constexpr float std_dev_weight = 4.0f;
    vtr::vector<ClusterBlockId, float> cluster_score(cluster_netlist.blocks().size(), 0.0f);
    vtr::vector<ClusterBlockId, float> cluster_constr_area(cluster_netlist.blocks().size(), std::numeric_limits<float>::max());
    for (ClusterBlockId blk_id : cluster_netlist.blocks()) {
        // Compute the standard deviation of the positions of all atoms in the
        // given macro. This is a measure of how much the atoms "want" to be
        // at the centroid location.
        t_pl_macro pl_macro = get_or_create_macro(blk_id, place_macros);
        float variance = get_flat_variance(pl_macro, flat_placement_info);
        float std_dev = std::sqrt(variance);
        // Normalize the standard deviation to be a number between 0 and 1.
        float normalized_std_dev = std_dev / (std_dev + 1.0f);

        // Get the "size" of the macro. This is the number of members that are
        // within the macro, where we consider clusters which are not part of
        // macros as having 0 size. Macros tend to be harder to place.
        float macro_size = pl_macro.members.size();
        if (place_macros.get_imacro_from_iblk(blk_id) == -1)
            macro_size = 0.0f;
        // Normalize the macro size to be a number between 0 and 1.
        float normalized_macro_size = macro_size / static_cast<float>(max_macro_size);

        // Compute the cost. Clusters wth a higher cost will be placed first.
        // Cost is proportional to macro size since larger macros are more
        // challenging to place and should be placed earlier if possible.
        // Cost is inversly proportional to standard deviation, since clusters
        // that contain atoms that all want to be within the same cluster
        // should be placed first.
        cluster_score[blk_id] = (macro_size_weight * normalized_macro_size)
                                + (std_dev_weight * (1.0f - normalized_std_dev));

        // If the cluster is constrained, compute how much area its constrained
        // region takes up. This will be used to place "more constrained" blocks
        // first.
        // TODO: The cluster constrained area can be incorperated into the cost
        //       somehow.
        ClusterBlockId macro_head_blk = pl_macro.members[0].blk_index;
        if (is_cluster_constrained(macro_head_blk)) {
            const PartitionRegion& pr = cluster_constraints[macro_head_blk];
            float area = 0.0f;
            for (const Region& region : pr.get_regions()) {
                const vtr::Rect<int> region_rect = region.get_rect();
                // Note: Add 1 here since the width is in grid space (i.e. width
                //       of 0 means it can only be placed in 1 x coordinate).
                area += (region_rect.width() + 1) * (region_rect.height() + 1);
            }
            cluster_constr_area[blk_id] = area;
        }
    }
    std::stable_sort(clusters_to_place.begin(), clusters_to_place.end(), [&](ClusterBlockId lhs, ClusterBlockId rhs) {
        // Sort the list such that:
        // 1) Clusters that are constrained to less area on the device are placed
        //    first.
        if (cluster_constr_area[lhs] != cluster_constr_area[rhs]) {
            return cluster_constr_area[lhs] < cluster_constr_area[rhs];
        }
        // 2) Higher score clusters are placed first.
        return cluster_score[lhs] > cluster_score[rhs];
    });

    return clusters_to_place;
}

/**
 * @brief Tries to place all of the given clusters as closed to their flat
 *        placement as possible (minimum displacement from flat placement).
 *        Returns false if any clusters could not be placed.
 *
 * This function will place clusters in passes. In the first pass, it will try
 * to place clusters exactly where their global placement is (according to the
 * atoms contained in the cluster). In the second pass, all unplaced clusters
 * will try to be placed within 1 tile of where they wanted to be placed.
 * Subsequent passes will then try to place clusters at exponentially farther
 * distances.
 */
static inline bool place_blocks_min_displacement(std::vector<ClusterBlockId>& clusters_to_place,
                                                 enum e_pad_loc_type pad_loc_type,
                                                 BlkLocRegistry& blk_loc_registry,
                                                 const PlaceMacros& place_macros,
                                                 const ClusteredNetlist& cluster_netlist,
                                                 const FlatPlacementInfo& flat_placement_info) {

    const DeviceGrid& device_grid = g_vpr_ctx.device().grid;

    // Compute the max L1 distance on the device. If we cannot find a location
    // to place a cluster within this distance, then no legal location exists.
    float max_distance_on_device = device_grid.width() + device_grid.height();

    // Print some logging information and the status header.
    VTR_LOG("Number of blocks to be placed: %zu\n", clusters_to_place.size());
    VTR_LOG("Max distance on device: %g\n", max_distance_on_device);
    print_ap_initial_placer_header();

    // Iteratively search for legal locations to place blocks. With each
    // iteration, we search farther and father away from the global placement
    // solution. The idea is to place blocks where they want first, then if they
    // cannot be placed we let other blocks try to be placed where they want
    // before trying to place the blocks elsewhere.
    //
    // A list to keep track of the blocks which were unplaced in this iteration.
    std::vector<ClusterBlockId> unplaced_blocks;
    unplaced_blocks.reserve(clusters_to_place.size());
    // The max displacement threshold for the search. We will not search farther
    // than this distance when searching for legal location.
    // Note: Distance here is the amount we would need to displace the block
    //       from its global placement solution to be put in the target tile.
    float max_displacement_threshold = 0.0f;
    float prev_max_displacement_threshold = -1.0f;
    size_t iter = 0;
    // We stop searching when the previous max_displacement threshold was larger
    // than the maximum distance on the device. This implies that the entire device
    // was searched.
    while (prev_max_displacement_threshold < max_distance_on_device) {
        // Early exit. If there is nothing to place in this iteration, just break.
        if (clusters_to_place.size() == 0)
            break;

        // Try to place each cluster in their cost order.
        for (ClusterBlockId blk_to_place : clusters_to_place) {
            // If this block is part of a macro, another member of that macro
            // may have placed it already. Just skip in that case.
            if (is_block_placed(blk_to_place, blk_loc_registry.block_locs())) {
                continue;
            }

            // Get the macro that contains this block, or create a temporary
            // macro that only contains this block.
            t_pl_macro pl_macro = get_or_create_macro(blk_to_place, place_macros);

            // Get the flat centroid location of the macro.
            t_flat_pl_loc centroid_flat_loc = find_centroid_loc_from_flat_placement(pl_macro, flat_placement_info);

            // Find a legal, open site closest to the flat cenetroid location
            // (within the displacement threshold).
            auto block_type = cluster_netlist.block_type(blk_to_place);
            t_pl_loc centroid_loc = find_nearest_compatible_loc(centroid_flat_loc,
                                                                max_displacement_threshold,
                                                                block_type,
                                                                pl_macro,
                                                                blk_loc_registry);

            // If a location could not be found, add to list of unplaced blocks
            // and skip.
            if (centroid_loc.x == OPEN) {
                unplaced_blocks.push_back(blk_to_place);
                continue;
            }

            // The find_nearest_compatible_loc function above should only return
            // a location which can legally accomodate the macro (if it found a
            // location). Double check these to be safe.
            VTR_ASSERT_SAFE(!blk_loc_registry.grid_blocks().block_at_location(centroid_loc));
            VTR_ASSERT_SAFE(macro_can_be_placed(pl_macro, centroid_loc, false, blk_loc_registry));

            // Place the macro
            for (const t_pl_macro_member& pl_macro_member : pl_macro.members) {
                t_pl_loc member_pos = centroid_loc + pl_macro_member.offset;
                ClusterBlockId iblk = pl_macro_member.blk_index;
                blk_loc_registry.set_block_location(iblk, member_pos);
            }

            // Finally, if the user asked for random pad locations and this is
            // an IO block, lock down the macro at this location so the placer
            // can't move it.
            // TODO: This is not technically "random" since the AP flow is
            //       choosing good places to put IO blocks based on the GP stage
            //       of the flow. This should be investigated to see if a more
            //       random distribution of IO pads are necessary.
            fix_IO_block_types(pl_macro, centroid_loc, pad_loc_type, blk_loc_registry.mutable_block_locs());
        }

        // Print the status of this iteration for debugging.
        print_ap_initial_placer_status(iter,
                                       max_displacement_threshold,
                                       clusters_to_place.size() - unplaced_blocks.size(),
                                       unplaced_blocks.size());

        // The clusters to place in the next iteration is the unplaced clusters
        // from this iteration. Swap these two vectors and clear the unplaced
        // blocks to be filled next iteration.
        clusters_to_place.swap(unplaced_blocks);
        unplaced_blocks.clear();

        // Update the max displacement threshold.
        // We exponentially increase the threshold. We first begin by trying
        // to place all the clusters exactly where they want, then we increase
        // the threshold from there. The idea is we spend more iterations with
        // low displacement values, then rapidly increase it to get a solution
        // sooner.
        prev_max_displacement_threshold = max_displacement_threshold;
        if (max_displacement_threshold == 0.0f) {
            max_displacement_threshold = 1.0f;
        } else {
            max_displacement_threshold *= 2.0f;
        }

        // Increase the iteration for status printing.
        iter++;
    }

    if (clusters_to_place.size() > 0) {
        VTR_LOG("Unable to place all clusters.\n");
        VTR_LOG("Clusters left unplaced:\n");
        // TODO: Increase the log verbosity of this.
        for (ClusterBlockId blk_id : clusters_to_place) {
            VTR_LOG("\t%s\n", cluster_netlist.block_name(blk_id).c_str());
        }
    }

    // Check if anything has not been placed, if so, signal to the calling function.
    if (clusters_to_place.size() > 0)
        return false;

    return true;
}

/**
 * @brief Places all blocks in the clustered netlist as close to the global
 *        placement produced by the AP flow. Returns false if any blocks could
 *        not be placed.
 *
 * This function places the blocks in stages. The goal of this stage-based
 * approach is to place clusters which are challenging to place first. Within
 * each stage, the clusters are ordered based on heuristics such that the most
 * impactful clusters get first dibs on placement.
 */
static inline bool place_all_blocks_ap(enum e_pad_loc_type pad_loc_type,
                                       BlkLocRegistry& blk_loc_registry,
                                       const PlaceMacros& place_macros,
                                       const FlatPlacementInfo& flat_placement_info) {

    const ClusteredNetlist& cluster_netlist = g_vpr_ctx.clustering().clb_nlist;

    // Get a list of clusters to place, sorted based on different heuristics
    // to try to give more important clusters first dibs on the placement.
    std::vector<ClusterBlockId> sorted_cluster_list = get_sorted_clusters_to_place(blk_loc_registry,
                                                                                   place_macros,
                                                                                   cluster_netlist,
                                                                                   flat_placement_info);

    // 1: Get the constrained clusters and place them first. For now, we place
    //    constrained clusters first to prevent other clusters from taking their
    //    spot if they are constrained to one and only one site.
    // TODO: This gives clusters with region constraints VIP access to the
    //       placement. This may not give the best results. This should be
    //       investigated more once region constraints are more supported in the
    //       AP flow.
    std::vector<ClusterBlockId> constrained_clusters;
    constrained_clusters.reserve(sorted_cluster_list.size());
    for (ClusterBlockId blk_id : sorted_cluster_list) {
        if (is_cluster_constrained(blk_id))
            constrained_clusters.push_back(blk_id);
    }

    if (constrained_clusters.size() > 0) {
        VTR_LOG("Placing constrained clusters...\n");
        bool all_clusters_placed = place_blocks_min_displacement(constrained_clusters,
                                                                 pad_loc_type,
                                                                 blk_loc_registry,
                                                                 place_macros,
                                                                 cluster_netlist,
                                                                 flat_placement_info);
        VTR_LOG("\n");
        if (!all_clusters_placed) {
            VTR_LOG("Could not place all constrained clusters, falling back on the non-AP initial placement.\n");
            return false;
        }
    }

    // 2. Get all of the large macros and place them next. Large macros have a
    //    hard time finding a place to go since they take up so much space. They
    //    also can have a larger impact on the quality of the placement, so we
    //    give them dibs on placement early.
    std::vector<ClusterBlockId> large_macro_clusters;
    large_macro_clusters.reserve(sorted_cluster_list.size());
    for (ClusterBlockId blk_id : sorted_cluster_list) {
        // If this block has been placed, skip it.
        if (is_block_placed(blk_id, blk_loc_registry.block_locs()))
            continue;

        // Get the size of the macro this block is a part of.
        t_pl_macro pl_macro = get_or_create_macro(blk_id, place_macros);
        size_t macro_size = pl_macro.members.size();
        if (macro_size > 1) {
            // If the size of the macro is larger than 1 (there is more than
            // one cluster in this macro) add to the list.
            large_macro_clusters.push_back(blk_id);
        }
    }

    if (large_macro_clusters.size() > 0) {
        VTR_LOG("Placing clusters that are part of larger macros...\n");
        bool all_clusters_placed = place_blocks_min_displacement(large_macro_clusters,
                                                                 pad_loc_type,
                                                                 blk_loc_registry,
                                                                 place_macros,
                                                                 cluster_netlist,
                                                                 flat_placement_info);
        VTR_LOG("\n");
        if (!all_clusters_placed) {
            VTR_LOG("Could not place all large macros, falling back on the non-AP initial placement.\n");
            return false;
        }
    }

    // 3. Place the rest of the clusters. These clusters will be unconstrained
    //    and be of small size; so they are more free to fill in the gaps left
    //    behind.
    std::vector<ClusterBlockId> clusters_to_place;
    clusters_to_place.reserve(sorted_cluster_list.size());
    for (ClusterBlockId blk_id : sorted_cluster_list) {
        // If this block has been placed, skip it.
        if (is_block_placed(blk_id, blk_loc_registry.block_locs()))
            continue;

        clusters_to_place.push_back(blk_id);
    }

    if (clusters_to_place.size() > 0) {
        VTR_LOG("Placing general clusters...\n");
        bool all_clusters_placed = place_blocks_min_displacement(clusters_to_place,
                                                                 pad_loc_type,
                                                                 blk_loc_registry,
                                                                 place_macros,
                                                                 cluster_netlist,
                                                                 flat_placement_info);
        VTR_LOG("\n");
        if (!all_clusters_placed) {
            VTR_LOG("Could not place all clusters, falling back on the non-AP initial placement.\n");
            return false;
        }
    }

    return true;
}

void initial_placement(const t_placer_opts& placer_opts,
                       const char* constraints_file,
                       const t_noc_opts& noc_opts,
                       BlkLocRegistry& blk_loc_registry,
                       const PlaceMacros& place_macros,
                       std::optional<NocCostHandler>& noc_cost_handler,
                       const FlatPlacementInfo& flat_placement_info,
                       vtr::RngContainer& rng) {
    vtr::ScopedStartFinishTimer timer("Initial Placement");

    // Initialize the block loc registry.
    blk_loc_registry.init();

    /*Mark the blocks that have already been locked to one spot via floorplan constraints
     * as fixed, so they do not get moved during initial placement or later during the simulated annealing stage of placement*/
    mark_fixed_blocks(blk_loc_registry);

    // read the constraint file and place fixed blocks
    if (strlen(constraints_file) != 0) {
        read_constraints(constraints_file, blk_loc_registry);
    }

    if (!placer_opts.read_initial_place_file.empty()) {
        const auto& grid = g_vpr_ctx.device().grid;
        read_place(nullptr, placer_opts.read_initial_place_file.c_str(), blk_loc_registry, false, grid);
    } else {
        if (noc_opts.noc) {
            // NoC routers are placed before other blocks
            initial_noc_placement(noc_opts, blk_loc_registry, place_macros, noc_cost_handler.value(), rng);
            propagate_place_constraints(place_macros);
        }

        //Place all blocks
        bool all_blocks_placed = false;

        // First try to place all of the blocks using AP
        if (flat_placement_info.valid) {
            all_blocks_placed = place_all_blocks_ap(placer_opts.pad_loc_type,
                                                    blk_loc_registry,
                                                    place_macros,
                                                    flat_placement_info);

            // If AP failed to place all of the blocks, reset the placement solution
            // so we can fall back on the original initial placement algorithm.
            if (!all_blocks_placed) {
                blk_loc_registry.clear_all_grid_locs();
                if (strlen(constraints_file) != 0) {
                    read_constraints(constraints_file, blk_loc_registry);
                }
            }
        }

        // If all of the blocks have not been placed (i.e. AP is turned off or
        // is disabled), try to place all of the clusters using heuristics.
        if (!all_blocks_placed) {
            // Assign scores to blocks and placement macros according to how difficult they are to place
            vtr::vector<ClusterBlockId, t_block_score> block_scores = assign_block_scores(place_macros);

            place_all_blocks(placer_opts, block_scores, placer_opts.pad_loc_type,
                             constraints_file, blk_loc_registry, place_macros,
                             flat_placement_info, rng);
        }
    }

    // Update the movable blocks vectors in the block loc registry.
    blk_loc_registry.alloc_and_load_movable_blocks();

    // ensure all blocks are placed and that NoC routing has no cycles
    check_initial_placement_legality(blk_loc_registry);
}
