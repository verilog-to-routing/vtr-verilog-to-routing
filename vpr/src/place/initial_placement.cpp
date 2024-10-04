#include "vtr_memory.h"
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
#include "directed_moves_util.h"
#include "echo_files.h"


#include <ctime>
#include <cmath>
#include <iomanip>

#ifdef VERBOSE
void print_clb_placement(const char* fname);
#endif

// Number of iterations that initial placement tries to place all blocks before throwing an error
static constexpr int MAX_INIT_PLACE_ATTEMPTS = 2;

// The amount of weight that will be added to previous unplaced block scores to ensure that failed blocks would be placed earlier next iteration
static constexpr int SORT_WEIGHT_PER_FAILED_BLOCK = 10;

// The amount of weight that will be added to each tile which is outside the floorplanning constraints
static constexpr int SORT_WEIGHT_PER_TILES_OUTSIDE_OF_PR = 100;

/**
 * @brief Set chosen grid locations to EMPTY block id before each placement iteration
 *   
 *   @param unplaced_blk_types_index Block types that their grid locations must be cleared.
 *   @param blk_loc_registry Placement block location information. To be filled with the location
 *   where pl_macro is placed.
 */
static void clear_block_type_grid_locs(const std::unordered_set<int>& unplaced_blk_types_index,
                                       BlkLocRegistry& blk_loc_registry);

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
 * 
 * @return true if macro was placed, false if not.
 */
static bool place_macro(int macros_max_num_tries,
                        const t_pl_macro& pl_macro,
                        e_pad_loc_type pad_loc_type,
                        std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid,
                        vtr::vector<ClusterBlockId, t_block_score>& block_scores,
                        BlkLocRegistry& blk_loc_registry);

/*
 * Assign scores to each block based on macro size and floorplanning constraints.
 * Used for relative placement, so that the blocks that are more difficult to place can be placed first during initial placement.
 * A higher score indicates that the block is more difficult to place.
 */
static vtr::vector<ClusterBlockId, t_block_score> assign_block_scores();

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
static bool find_centroid_neighbor(t_pl_loc& centroid_loc,
                                   t_logical_block_type_ptr block_type,
                                   bool search_for_empty,
                                   const BlkLocRegistry& blk_loc_registry);

/**
 * @brief  tries to place a macro at a centroid location of its placed connections.
 *
 *   @param pl_macro The macro to be placed.
 *   @param pr The PartitionRegion of the macro - represents its floorplanning constraints, is the size of the whole chip if the macro is not
 *   constrained.
 *   @param block_type Logical block type of the macro blocks.
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *   @param block_scores The block_scores (ranking of what to place next) for unplaced blocks connected to this macro are updated in this routine.
 *   @param blk_loc_registry Placement block location information. To be filled with the location
 *   where pl_macro is placed.
 *
 * @return true if the macro gets placed, false if not.
 */
static bool try_centroid_placement(const t_pl_macro& pl_macro,
                                   const PartitionRegion& pr,
                                   t_logical_block_type_ptr block_type,
                                   e_pad_loc_type pad_loc_type,
                                   vtr::vector<ClusterBlockId, t_block_score>& block_scores,
                                   BlkLocRegistry& blk_loc_registry);

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
 */
static void place_all_blocks(const t_placer_opts& placer_opts,
                             vtr::vector<ClusterBlockId, t_block_score>& block_scores,
                             e_pad_loc_type pad_loc_type,
                             const char* constraints_file,
                             BlkLocRegistry& blk_loc_registry);

/**
 * @brief If any blocks remain unplaced after all initial placement iterations, this routine
 * throws an error indicating that initial placement can not be done with the current device size or
 * floorplanning constraints. 
 */
static void check_initial_placement_legality(const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs);

/**
 * @brief Fills movable_blocks in global PlacementContext
 */
static void alloc_and_load_movable_blocks(const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs);

static void check_initial_placement_legality(const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();

    int unplaced_blocks = 0;

    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        if (block_locs[blk_id].loc.x == INVALID_X) {
            VTR_LOG("Block %s (# %d) of type %s could not be placed during initial placement iteration %d\n",
                    cluster_ctx.clb_nlist.block_name(blk_id).c_str(),
                    blk_id,
                    cluster_ctx.clb_nlist.block_type(blk_id)->name,
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

    for (auto movable_blk_id : place_ctx.movable_blocks) {
        if (block_locs[movable_blk_id].is_fixed) {
            VPR_FATAL_ERROR(VPR_ERROR_PLACE, "Fixed block was mistakenly marked as movable during initial placement.\n");
        }
    }
    
    for (const auto& logical_block_type : device_ctx.logical_block_types) {
        const auto& movable_blocks_of_type = place_ctx.movable_blocks_per_type[logical_block_type.index];
        for (const auto& movable_blk_id : movable_blocks_of_type) {
            if (block_locs[movable_blk_id].is_fixed) {
                VPR_FATAL_ERROR(VPR_ERROR_PLACE, "Fixed block %d of logical type %s was mistakenly marked as movable during initial placement.\n",
                                (size_t)movable_blk_id, logical_block_type.name);
            }
            if (cluster_ctx.clb_nlist.block_type(movable_blk_id)->index != logical_block_type.index) {
                VPR_FATAL_ERROR(VPR_ERROR_PLACE, "Clustered block %d of logical type %s was mistakenly marked as logical type %s.\n",
                                (size_t)movable_blk_id,
                                cluster_ctx.clb_nlist.block_type(movable_blk_id)->name,
                                logical_block_type.name);
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

        if (reg_rect.contains({loc.x, loc.y})) {
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

static bool find_centroid_neighbor(t_pl_loc& centroid_loc,
                                   t_logical_block_type_ptr block_type,
                                   bool search_for_empty,
                                   const BlkLocRegistry& blk_loc_registry) {
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[block_type->index];
    const int num_layers = g_vpr_ctx.device().grid.get_num_layers();
    const int centroid_loc_layer_num = centroid_loc.layer;

    //Determine centroid location in the compressed space of the current block
    auto compressed_centroid_loc = get_compressed_loc_approx(compressed_block_grid,
                                                             centroid_loc,
                                                             num_layers);

    //range limit (rlim) set a limit for the neighbor search in the centroid placement
    //the neighbor location should be within the defined range to calculated centroid location
    int first_rlim = 15;

    auto search_range = get_compressed_grid_target_search_range(compressed_block_grid,
                                                                compressed_centroid_loc[centroid_loc_layer_num],
                                                                first_rlim);

    int delta_cx = search_range.xmax - search_range.xmin;

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
                                                         blk_loc_registry);

    if (!legal) {
        return false;
    }

    compressed_grid_to_loc(block_type, to_compressed_loc, centroid_loc);

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

                t_physical_tile_loc tile_loc = get_coordinate_of_pin(sink_pin_id, blk_loc_registry);
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

            t_physical_tile_loc tile_loc = get_coordinate_of_pin(source_pin, blk_loc_registry);
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

static bool try_centroid_placement(const t_pl_macro& pl_macro,
                                   const PartitionRegion& pr,
                                   t_logical_block_type_ptr block_type,
                                   e_pad_loc_type pad_loc_type,
                                   vtr::vector<ClusterBlockId, t_block_score>& block_scores,
                                   BlkLocRegistry& blk_loc_registry) {
    auto& block_locs = blk_loc_registry.mutable_block_locs();

    t_pl_loc centroid_loc(OPEN, OPEN, OPEN, OPEN);
    std::vector<ClusterBlockId> unplaced_blocks_to_update_their_score;

    unplaced_blocks_to_update_their_score = find_centroid_loc(pl_macro, centroid_loc, blk_loc_registry);

    //no suggestion was available for this block type
    if (!is_loc_on_chip({centroid_loc.x, centroid_loc.y, centroid_loc.layer})) {
        return false;
    }

    //centroid suggestion was either occupied or does not match block type
    //try to find a near location that meet these requirements
    bool neighbor_legal_loc = false;
    if (!is_loc_legal(centroid_loc, pr, block_type)) {
        neighbor_legal_loc = find_centroid_neighbor(centroid_loc, block_type, false, blk_loc_registry);
        if (!neighbor_legal_loc) { //no neighbor candidate found
            return false;
        }
    }

    //no neighbor were found that meet all our requirements, should be placed with random placement
    if (!is_loc_on_chip({centroid_loc.x, centroid_loc.y, centroid_loc.layer}) || !pr.is_loc_in_part_reg(centroid_loc)) {
        return false;
    }

    auto& device_ctx = g_vpr_ctx.device();
    //choose the location's subtile if the centroid location is legal.
    //if the location is found within the "find_centroid_neighbor", it already has a subtile
    //we don't need to find one again
    if (!neighbor_legal_loc) {
        const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[block_type->index];
        const auto& type = device_ctx.grid.get_physical_type({centroid_loc.x, centroid_loc.y, centroid_loc.layer});
        const auto& compatible_sub_tiles = compressed_block_grid.compatible_sub_tile_num(type->index);
        centroid_loc.sub_tile = compatible_sub_tiles[vtr::irand((int)compatible_sub_tiles.size() - 1)];
    }
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
                                      const t_pl_macro& pl_macro, std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid) {
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

    //If the user marked the IO block pad_loc_type as RANDOM, that means it should be randomly
    //placed and then stay fixed to that location, which is why the macro members are marked as fixed.
    const auto& type = device_ctx.grid.get_physical_type({loc.x, loc.y, loc.layer});
    if (is_io_type(type) && pad_loc_type == e_pad_loc_type::RANDOM) {
        for (const t_pl_macro_member& pl_macro_member : pl_macro.members) {
            block_locs[pl_macro_member.blk_index].is_fixed = true;
        }
    }
}

bool try_place_macro_randomly(const t_pl_macro& pl_macro,
                              const PartitionRegion& pr,
                              t_logical_block_type_ptr block_type,
                              e_pad_loc_type pad_loc_type,
                              BlkLocRegistry& blk_loc_registry) {
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
        region_index = vtr::irand(regions.size() - 1);
    } else {
        region_index = 0;
    }
    const Region& reg = regions[region_index];

    const vtr::Rect<int>& reg_rect = reg.get_rect();
    const auto [layer_low, layer_high] = reg.get_layer_range();

    int selected_layer = (layer_low == layer_high) ? layer_low : layer_low + vtr::irand(layer_high - layer_low);

    auto min_compressed_loc = compressed_block_grid.grid_loc_to_compressed_loc_approx({reg_rect.xmin(), reg_rect.ymin(), selected_layer});

    auto max_compressed_loc = compressed_block_grid.grid_loc_to_compressed_loc_approx({reg_rect.xmax(), reg_rect.ymax(), selected_layer});

    int delta_cx = max_compressed_loc.x - min_compressed_loc.x;

    t_physical_tile_loc to_compressed_loc;

    bool legal;

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
                                                    blk_loc_registry);


    if (!legal) {
        //No valid position found
        return false;
    }

    t_pl_loc loc;
    compressed_grid_to_loc(block_type, to_compressed_loc, loc);

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

    bool mac_can_be_placed = macro_can_be_placed(pl_macro, head_pos, /*check_all_legality=*/false, blk_loc_registry);

    if (mac_can_be_placed) {
        // Place down the macro
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
                        BlkLocRegistry& blk_loc_registry) {
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
        macro_placed = try_centroid_placement(pl_macro, pr, block_type, pad_loc_type, block_scores, blk_loc_registry);
    }
    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\t\tMacro is placed: %d\n", macro_placed);
    // If macro is not placed yet, try to place the macro randomly for the max number of random tries
    for (int itry = 0; itry < macros_max_num_tries && !macro_placed; itry++) {
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\t\tTry random place iter: %d\n", itry);
        macro_placed = try_place_macro_randomly(pl_macro, pr, block_type, pad_loc_type, blk_loc_registry);
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

static vtr::vector<ClusterBlockId, t_block_score> assign_block_scores() {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& floorplan_ctx = g_vpr_ctx.floorplanning();

    auto& pl_macros = place_ctx.pl_macros;

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
    for (const auto& pl_macro : pl_macros) {
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
                             BlkLocRegistry& blk_loc_registry) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
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
        clear_block_type_grid_locs(unplaced_blk_type_in_curr_itr, blk_loc_registry);
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

#ifdef VTR_ENABLE_DEBUG_LOGGING
            enable_placer_debug(placer_opts, blk_id);
#else
            (void)placer_opts;
#endif
            VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "Popped Block %d\n", size_t(blk_id));

            blocks_placed_since_heap_update++;

            bool block_placed = place_one_block(blk_id, pad_loc_type, &blk_types_empty_locs_in_grid[blk_id_type->index], &block_scores, blk_loc_registry);

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
                int imacro;
                get_imacro_from_iblk(&imacro, blk_id, place_ctx.pl_macros);
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

static void clear_block_type_grid_locs(const std::unordered_set<int>& unplaced_blk_types_index,
                                       BlkLocRegistry& blk_loc_registry) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& grid_blocks = blk_loc_registry.mutable_grid_blocks();
    auto& block_locs = blk_loc_registry.mutable_block_locs();

    bool clear_all_block_types = false;

    /* check if all types should be cleared
     * logical_block_types contain empty type, needs to be ignored.
     * Not having any type in unplaced_blk_types_index means that it is the first iteration, hence all grids needs to be cleared
     */
    if (unplaced_blk_types_index.size() == device_ctx.logical_block_types.size() - 1) {
        clear_all_block_types = true;
    }

    /* We'll use the grid to record where everything goes. Initialize to the grid has no
     * blocks placed anywhere.
     */
    for (int layer_num = 0; layer_num < device_ctx.grid.get_num_layers(); layer_num++) {
        for (int i = 0; i < (int)device_ctx.grid.width(); i++) {
            for (int j = 0; j < (int)device_ctx.grid.height(); j++) {
                const t_physical_tile_type_ptr type = device_ctx.grid.get_physical_type({i, j, layer_num});
                int itype = type->index;
                if (clear_all_block_types || unplaced_blk_types_index.count(itype)) {
                    grid_blocks.set_usage({i, j, layer_num}, 0);
                    for (int k = 0; k < device_ctx.physical_tile_types[itype].capacity; k++) {
                        grid_blocks.set_block_at_location({i, j, k, layer_num}, ClusterBlockId::INVALID());
                    }
                }
            }
        }
    }

    /* Similarly, mark all blocks as not being placed yet. */
    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        int blk_type = cluster_ctx.clb_nlist.block_type(blk_id)->index;
        if (clear_all_block_types || unplaced_blk_types_index.count(blk_type)) {
            block_locs[blk_id].loc = t_pl_loc();
        }
    }
}

void clear_all_grid_locs(BlkLocRegistry& blk_loc_registry) {
    auto& device_ctx = g_vpr_ctx.device();

    std::unordered_set<int> blk_types_to_be_cleared;
    const auto& logical_block_types = device_ctx.logical_block_types;

    // Insert all the logical block types into the set except the empty type
    // clear_block_type_grid_locs does not expect empty type to be among given types
    for (const t_logical_block_type& logical_type : logical_block_types) {
        if (!is_empty_type(&logical_type)) {
            blk_types_to_be_cleared.insert(logical_type.index);
        }
    }

    clear_block_type_grid_locs(blk_types_to_be_cleared, blk_loc_registry);
}

bool place_one_block(const ClusterBlockId blk_id,
                     enum e_pad_loc_type pad_loc_type,
                     std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid,
                     vtr::vector<ClusterBlockId, t_block_score>* block_scores,
                     BlkLocRegistry& blk_loc_registry) {
    const std::vector<t_pl_macro>& pl_macros = g_vpr_ctx.placement().pl_macros;
    const auto& block_locs = blk_loc_registry.block_locs();

    //Check if block has already been placed
    if (is_block_placed(blk_id, block_locs)) {
        return true;
    }

    bool placed_macro = false;

    //Lookup to see if the block is part of a macro
    int imacro;
    get_imacro_from_iblk(&imacro, blk_id, pl_macros);

    if (imacro != -1) { //If the block belongs to a macro, pass that macro to the placement routines
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\tBelongs to a macro %d\n", imacro);
        const t_pl_macro& pl_macro = pl_macros[imacro];
        placed_macro = place_macro(MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY, pl_macro, pad_loc_type, blk_types_empty_locs_in_grid, *block_scores, blk_loc_registry);
    } else {
        //If it does not belong to a macro, create a macro with the one block and then pass to the placement routines
        //This is done so that the initial placement flow can be the same whether the block belongs to a macro or not
        t_pl_macro_member macro_member;
        macro_member.blk_index = blk_id;
        macro_member.offset = t_pl_offset(0, 0, 0, 0);
        t_pl_macro pl_macro;
        pl_macro.members.push_back(macro_member);
        placed_macro = place_macro(MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY, pl_macro, pad_loc_type, blk_types_empty_locs_in_grid, *block_scores, blk_loc_registry);
    }

    return placed_macro;
}

static void alloc_and_load_movable_blocks(const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& device_ctx = g_vpr_ctx.device();

    place_ctx.movable_blocks.clear();
    place_ctx.movable_blocks_per_type.clear();

    size_t n_logical_blocks = device_ctx.logical_block_types.size();
    place_ctx.movable_blocks_per_type.resize(n_logical_blocks);


    // iterate over all clustered blocks and store block ids of movable ones
    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        const auto& loc = block_locs[blk_id];
        if (!loc.is_fixed) {
            place_ctx.movable_blocks.push_back(blk_id);

            const t_logical_block_type_ptr block_type = cluster_ctx.clb_nlist.block_type(blk_id);
            place_ctx.movable_blocks_per_type[block_type->index].push_back(blk_id);
        }
    }
}

void initial_placement(const t_placer_opts& placer_opts,
                       const char* constraints_file,
                       const t_noc_opts& noc_opts,
                       BlkLocRegistry& blk_loc_registry) {
    vtr::ScopedStartFinishTimer timer("Initial Placement");
    auto& block_locs = blk_loc_registry.mutable_block_locs();

    /* Initialize the grid blocks to empty.
     * Initialize all the blocks to unplaced.
     */
    clear_all_grid_locs(blk_loc_registry);

    /* Go through cluster blocks to calculate the tightest placement
     * floorplan constraint for each constrained block
     */
    propagate_place_constraints();

    /*Mark the blocks that have already been locked to one spot via floorplan constraints
     * as fixed, so they do not get moved during initial placement or later during the simulated annealing stage of placement*/
    mark_fixed_blocks(blk_loc_registry);

    // Compute and store compressed floorplanning constraints
    alloc_and_load_compressed_cluster_constraints();


    // read the constraint file and place fixed blocks
    if (strlen(constraints_file) != 0) {
        read_constraints(constraints_file, blk_loc_registry);
    }



    if(!placer_opts.read_initial_place_file.empty()) {
        const auto& grid = g_vpr_ctx.device().grid;
        read_place(nullptr, placer_opts.read_initial_place_file.c_str(), blk_loc_registry, false, grid);
    } else {
        if (noc_opts.noc) {
            // NoC routers are placed before other blocks
            initial_noc_placement(noc_opts, placer_opts, blk_loc_registry);
            propagate_place_constraints();
        }

        //Assign scores to blocks and placement macros according to how difficult they are to place
        vtr::vector<ClusterBlockId, t_block_score> block_scores = assign_block_scores();

        //Place all blocks
        place_all_blocks(placer_opts, block_scores, placer_opts.pad_loc_type, constraints_file, blk_loc_registry);
    }

    alloc_and_load_movable_blocks(block_locs);

    // ensure all blocks are placed and that NoC routing has no cycles
    check_initial_placement_legality(block_locs);

    //#ifdef VERBOSE
    //    VTR_LOG("At end of initial_placement.\n");
    //    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_INITIAL_CLB_PLACEMENT)) {
    //        print_clb_placement(getEchoFileName(E_ECHO_INITIAL_CLB_PLACEMENT));
    //    }
    //#endif
}
