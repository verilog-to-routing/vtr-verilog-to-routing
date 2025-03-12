
#pragma once

#include "vpr_types.h"
#include "move_transactions.h"
#include "compressed_grid.h"

class PlacerState;
class BlkLocRegistry;
class PlaceMacros;
class PlacerCriticalities;
namespace vtr {
class RngContainer;
}

/* Cut off for incremental bounding box updates.                          *
 * 4 is fastest -- I checked.                                             */
/* To turn off incremental bounding box updates, set this to a huge value */
constexpr size_t SMALL_NET = 4;

/* This is for the placement swap routines. A swap attempt could be       *
 * rejected, accepted or aborted (due to the limitations placed on the    *
 * carry chain support at this point).                                    */
enum class e_move_result {
    REJECTED,
    ACCEPTED,
    ABORTED
};

//This is to list all the available moves
enum class e_move_type {
    UNIFORM,
    MEDIAN,
    CENTROID,
    W_CENTROID,
    W_MEDIAN,
    CRIT_UNIFORM,
    FEASIBLE_REGION,
    NOC_ATTRACTION_CENTROID,
    NUMBER_OF_AUTO_MOVES,
    MANUAL_MOVE = NUMBER_OF_AUTO_MOVES,
    INVALID_MOVE
};

enum class e_create_move {
    VALID, //Move successful and legal
    ABORT, //Unable to perform move
};

/**
 * @brief Stores KArmedBanditAgent propose_action output to decide which
 *        move_type and which block_type should be chosen for the next action.
 *        The propose_action function can also leave blk_type empty to allow any
 *        random block type to be chosen to be swapped.
 */
struct t_propose_action {
    e_move_type move_type = e_move_type::INVALID_MOVE; ///<move type that propose_action chooses to perform
    int logical_blk_type_index = -1;                   ///<propose_action can choose block type or set it to -1 to allow any block type to be chosen
};

/**
 * @brief Stores a bounding box edge of a net with the timing
 *        criticality of the net terminal that caused this edge
 */
struct t_edge_cost {
    int edge;          //x or y coordinate of one edge of a bounding box
    float criticality; //the timing criticality of the net terminal that caused this edge
};

/**
 * @brief Stores the bounding box of a net in terms of the minimum and
 *        maximum coordinates of the blocks forming the net, clipped to
 *        the region: (1..device_ctx.grid.width()-2, 1..device_ctx.grid.height()-1)
 *        and the timing cost of the net terminal that caused each edge. 
 *        This is useful for some directed move generators.
 */
struct t_bb_cost {
    t_edge_cost xmin = {0, 0.0};
    t_edge_cost xmax = {0, 0.0};
    t_edge_cost ymin = {0, 0.0};
    t_edge_cost ymax = {0, 0.0};
    t_edge_cost layer_min = {0, 0.};
    t_edge_cost layer_max = {0, 0.};
};

/**
 * @brief Stores the different range limiters
 *
 * original_rlim: the original range limit calculated by the anneal
 * first_rlim: the first range limit calculated based on the design and device size
 * dm_rlim: the range limit around the center calculated by a directed move
 */
struct t_range_limiters {
    float original_rlim;
    float first_rlim;
    float dm_rlim;
};

e_create_move create_move(t_pl_blocks_to_be_moved& blocks_affected,
                          ClusterBlockId b_from,
                          t_pl_loc to,
                          const BlkLocRegistry& blk_loc_registry,
                          const PlaceMacros& place_macros);

/**
 * @brief Find the blocks that will be affected by a move of b_from to to_loc
 * @param blocks_affected Loaded by this routine and returned via reference; it lists the blocks etc. moved
 * @param b_from Id of the cluster-level block to be moved
 * @param to Where b_from will be moved to
 * @return e_block_move_result ABORT if either of the the moving blocks are already stored, or either of the blocks are fixed, to location is not
 * compatible, etc. INVERT if the "from" block is a single block and the "to" block is a macro. VALID otherwise.
 */
e_block_move_result find_affected_blocks(t_pl_blocks_to_be_moved& blocks_affected,
                                         ClusterBlockId b_from,
                                         t_pl_loc to,
                                         const BlkLocRegistry& blk_loc_registry,
                                         const PlaceMacros& place_macros);

e_block_move_result record_single_block_swap(t_pl_blocks_to_be_moved& blocks_affected,
                                             ClusterBlockId b_from,
                                             t_pl_loc to,
                                             const BlkLocRegistry& blk_loc_registry);

e_block_move_result record_macro_swaps(t_pl_blocks_to_be_moved& blocks_affected,
                                       const int imacro_from,
                                       int& imember_from,
                                       t_pl_offset swap_offset,
                                       const BlkLocRegistry& blk_loc_registry,
                                       const PlaceMacros& place_macros);

e_block_move_result record_macro_macro_swaps(t_pl_blocks_to_be_moved& blocks_affected,
                                             const int imacro_from,
                                             int& imember_from,
                                             const int imacro_to,
                                             ClusterBlockId blk_to,
                                             t_pl_offset swap_offset,
                                             const BlkLocRegistry& blk_loc_registry,
                                             const PlaceMacros& pl_macros);

e_block_move_result record_macro_move(t_pl_blocks_to_be_moved& blocks_affected,
                                      std::vector<ClusterBlockId>& displaced_blocks,
                                      const int imacro,
                                      t_pl_offset swap_offset,
                                      const BlkLocRegistry& blk_loc_registry,
                                      const PlaceMacros& place_macros);

e_block_move_result identify_macro_self_swap_affected_macros(std::vector<int>& macros,
                                                             const int imacro,
                                                             t_pl_offset swap_offset,
                                                             const BlkLocRegistry& blk_loc_registry,
                                                             const PlaceMacros& place_macros,
                                                             MoveAbortionLogger& move_abortion_logger);

e_block_move_result record_macro_self_swaps(t_pl_blocks_to_be_moved& blocks_affected,
                                            const int imacro,
                                            t_pl_offset swap_offset,
                                            const BlkLocRegistry& blk_loc_registry,
                                            const PlaceMacros& place_macros);

/**
 * @brief Check whether the "to" location is legal for the given "blk"
 * @param blk
 * @param to
 * @return True if this would be a legal move, false otherwise
 */
bool is_legal_swap_to_location(ClusterBlockId blk,
                               t_pl_loc to,
                               const BlkLocRegistry& blk_loc_registry);

/**
 * @brief Propose block for the RL agent based on required block type.
 *
 *  @param logical_blk_type_index: Index of the block type being perturbed, which is used to select the proper agent data
 *  @param highly_crit_block: block should be chosen from highly critical blocks.
 *  @param net_from: if block is chosen from highly critical blocks, should store the critical net id.
 *  @param pin_from: if block is chosen from highly critical blocks, should save its critical pin id.
 *
 * @return block id if any blocks found. ClusterBlockId::INVALID() if no block found.
 */
ClusterBlockId propose_block_to_move(const t_placer_opts& placer_opts,
                                     int& logical_blk_type_index,
                                     bool highly_crit_block,
                                     const PlacerCriticalities* placer_criticalities,
                                     ClusterNetId* net_from,
                                     int* pin_from,
                                     const PlacerState& placer_state,
                                     vtr::RngContainer& rng);

/**
 * @brief Find a block with a specific block type to be swapped with another block
 *
 * @param logical_blk_type_index The logical type of the moving block. If a negative value is passed,
 * the block is selected randomly from all movable blocks and not from a specific type.
 * @param rng A random number generator used to select a random block.
 * @param blk_loc_registry Contains movable blocks and movable blocks per type.
 * 
 * @return BlockId of the selected block, ClusterBlockId::INVALID() if no block with specified block type found
 */
ClusterBlockId pick_from_block(int logical_blk_type_index,
                               vtr::RngContainer& rng,
                               const BlkLocRegistry& blk_loc_registry);

/**
 * @brief Find a highly critical block with a specific block type to be swapped with another block.
 *
 * @param net_from The clustered net id of the critical connection of the selected block by this function.
 * To be filled by this function.
 * @param pin_from The pin id of the critical connection of the  selected block by this function.
 * To be filled by this function.
 * @param logical_blk_type_index The logical type of the moving block. If a negative value is passed,
 * the block is selected randomly from all movable blocks and not from a specific type.
 * @param placer_state Used to access the current placement's info, e.g. block locations and if they are fixed.
 * @param placer_criticalities Holds the clustered netlist connection criticalities.
 * @param rng A random number generator used to select a random highly critical block.
 * 
 * @return BlockId of the selected block, ClusterBlockId::INVALID() if no block with specified block type found.
 */
ClusterBlockId pick_from_highly_critical_block(ClusterNetId& net_from,
                                               int& pin_from,
                                               int logical_blk_type_index,
                                               const PlacerState& placer_state,
                                               const PlacerCriticalities& placer_criticalities,
                                               vtr::RngContainer& rng);

bool find_to_loc_uniform(t_logical_block_type_ptr type,
                         float rlim,
                         const t_pl_loc& from,
                         t_pl_loc& to,
                         ClusterBlockId b_from,
                         const BlkLocRegistry& blk_loc_registry,
                         vtr::RngContainer& rng);

// Accessor f_placer_breakpoint_reached
// return true when a placer breakpoint is reached
bool placer_breakpoint_reached();

// setter for f_placer_breakpoint_reached
// Should be setted in the breakpoint calculation algorithm
void set_placer_breakpoint_reached(bool);

/**
 * @brief Find a legal swap to location for the given type in a specific region.
 *
 * This function finds a legal swap to location for the type "blk_type" in a region 
 * defined in "limit_coords". It returns the location it picks in "to_loc" that is passed by reference. It also returns true
 * if it was able to find a compatible location and false otherwise.
 * It is similar to find_to_loc_uniform but searching in a defined range instead of searching in a range around the current block location.
 *
 *  @param blk_type the type of the moving block
 *  @param from_loc the original location of the moving block
 *  @param limit_coords the region where I can move the block to
 *  @param to_loc the new location that the function picked for the block
 *  @param b_from The unique ID of the clustered block whose median location is to be computed.
 *  @param blk_loc_registry Information about clustered block locations.
 */
bool find_to_loc_median(t_logical_block_type_ptr blk_type,
                        const t_pl_loc& from_loc,
                        const t_bb* limit_coords,
                        t_pl_loc& to_loc,
                        ClusterBlockId b_from,
                        const BlkLocRegistry& blk_loc_registry,
                        vtr::RngContainer& rng);

/**
 * @brief Find a legal swap to location for the given type in a range around a specific location.
 *
 * This function finds a legal swap to location for the type "blk_type" in a range around a specific location.
 * It returns the location it picks in "to_loc" that is passed by reference. It also returns true
 * if it was able to find a compatible location and false otherwise.
 * It is similar to find_to_loc_uniform but searching in a range around a defined location 
 * instead of searching in a range around the current block location.
 *
 *  @param blk_type: the type of the moving block
 *  @param from_loc: the original location of the moving block
 *  @param limit_coords: the region where I can move the block to
 *  @param to_loc: the new location that the function picked for the block
 */
bool find_to_loc_centroid(t_logical_block_type_ptr blk_type,
                          const t_pl_loc& from_loc,
                          const t_pl_loc& centeroid,
                          const t_range_limiters& range_limiters,
                          t_pl_loc& to_loc,
                          ClusterBlockId b_from,
                          const BlkLocRegistry& blk_loc_registry,
                          vtr::RngContainer& rng);

const std::string& move_type_to_string(e_move_type);

/* find to location helper functions */
/**
 * @brief convert compressed location to normal location
 *
 * blk_type: the type of the moving block
 * cx: the x coordinate of the compressed location
 * cy: the y coordinate of the compressed location
 * loc: the uncompressed output location (returned in reference)
 */
void compressed_grid_to_loc(t_logical_block_type_ptr blk_type,
                            t_physical_tile_loc compressed_loc,
                            t_pl_loc& to_loc,
                            vtr::RngContainer& rng);

/**
 * @brief Tries to find an compatible empty subtile with the given type at
 * the given location. If such a subtile could be found, the subtile number
 * is returned. Otherwise, -1 is returned to indicate that there are no
 * compatible subtiles at the given location.
 *
 * @param type logical block type
 * @param to_loc The location to be checked
 * @param grid_blocks A mapping from grid locations to clustered blocks placed there.
 *
 * @return int The subtile number if there is an empty compatible subtile, otherwise -1
 * is returned to indicate that there are no empty subtiles compatible with the given type..
 */
int find_empty_compatible_subtile(t_logical_block_type_ptr type,
                                  const t_physical_tile_loc& to_loc,
                                  const GridBlock& grid_blocks,
                                  vtr::RngContainer& rng);

/**
 * @brief find compressed location in a compressed range for a specific type in the given layer (to_layer_num)
 * 
 * type: defines the moving block type
 * search_range: the minimum and maximum coordinates of the search range in the compressed grid
 * from_loc: the coordinates of the old location
 * to_loc: the coordinates of the new location on the compressed grid
 * is_median: true if this is called from find_to_loc_median
 * to_layer_num: the layer number of the new location (set by the caller)
 * search_for_empty: indicates that the returned location must be empty
 */
bool find_compatible_compressed_loc_in_range(t_logical_block_type_ptr type,
                                             int delta_cx,
                                             const t_physical_tile_loc& from_loc,
                                             t_bb search_range,
                                             t_physical_tile_loc& to_loc,
                                             bool is_median,
                                             int to_layer_num,
                                             bool search_for_empty,
                                             const BlkLocRegistry& blk_loc_registry,
                                             vtr::RngContainer& rng);

/**
 * @brief Get the the compressed loc from the uncompressed loc (grid_loc)
 * @note This assumes the grid_loc corresponds to a location of the block type that compressed_block_grid stores its
 * compressed location. Otherwise, it would raise an assertion error.
 * @param compressed_block_grid The class that stores the compressed block grid of the block
 * @param grid_loc The actual location of the block
 * @param num_layers The number of layers (dice) of the FPGA
 * @return Returns the compressed location of the block on each layer
 */
std::vector<t_physical_tile_loc> get_compressed_loc(const t_compressed_block_grid& compressed_block_grid,
                                                    t_pl_loc grid_loc,
                                                    int num_layers);

/**
 * @brief Get the the compressed loc from the uncompressed loc (grid_loc). Return the nearest compressed location
 * if grid_loc doesn't fall on a block of the type that compressed_block_grid stores its compressed location.
 * @param compressed_block_grid
 * @param grid_loc
 * @param num_layers
 * @return
 */
std::vector<t_physical_tile_loc> get_compressed_loc_approx(const t_compressed_block_grid& compressed_block_grid,
                                                           t_pl_loc grid_loc,
                                                           int num_layers);

/**
 * @brief This function calculates the search range around the compressed locs, based on the given rlim value and
 * the number of rows/columns containing the same block type as the one that compressed_loc belongs to.
 * If rlim is greater than the number of columns containing the block type on the right side of the compressed_loc,
 * the search range from the right is limited by that number. Similar constraints apply to other sides as well. The
 * function returns the final search range based on these conditions.
 * @param compressed_block_grid
 * @param compressed_locs
 * @param rlim
 * @return A compressed search range for each layer
 */
t_bb get_compressed_grid_target_search_range(const t_compressed_block_grid& compressed_block_grid,
                                             const t_physical_tile_loc& compressed_locs,
                                             float rlim);

/**
 * @brief This function calculates the search range based on the given rlim value and the number of columns/rows
 * containing the same resource type as the one specified in the compressed_block_grid.
 * The search range is determined in a square shape, with from_compressed_loc as one of the corners and
 * directed towards the target_compressed_loc. The function returns the final search range based on these conditions.
 * @Note This function differs from get_compressed_grid_target_search_range as it doesn't have from_compressed_loc
 * in the center of the search range.
 * @param compressed_block_grid
 * @param from_compressed_loc
 * @param target_compressed_loc
 * @param rlim
 * @return
 */
t_bb get_compressed_grid_bounded_search_range(const t_compressed_block_grid& compressed_block_grid,
                                              const t_physical_tile_loc& from_compressed_loc,
                                              const t_physical_tile_loc& target_compressed_loc,
                                              float rlim);

/*
 * If the block to be moved (b_from) has a floorplan constraint, this routine changes the max and min coords
 * in the compressed grid (min_cx, min_cy, max_cx, max_cy) to make sure the range limit is within the floorplan constraint.
 *
 * Returns false if there is no intersection between the range limit and the floorplan constraint,
 * true otherwise.
 *
 *
 * If region size of the block's floorplan constraints is greater than 1, the block is constrained to more than one rectangular region.
 * In this case, we return true (i.e. the range limit intersects with
 * the floorplan constraints) to simplify the problem. This simplification can be done because
 * this routine is done for cpu time optimization, so we do not have to necessarily check each
 * complicated case to get correct functionality during place moves.
 *
 * The intersection takes place in the layer (die) specified by layer_num.
 *
 */
bool intersect_range_limit_with_floorplan_constraints(ClusterBlockId b_from,
                                                      t_bb& search_range,
                                                      int& delta_cx,
                                                      int layer_num);

std::string e_move_result_to_string(e_move_result move_outcome);

/**
 * @brif Iterate over all layers that have a physical tile at the x-y location specified by "loc" that can accommodate "logical_block".
 * If the location in the layer specified by "layer_num" is empty, return that layer. Otherwise,
 * return a layer that is not occupied at that location. If there isn't any, again, return the layer of loc.
 */
int find_free_layer(t_logical_block_type_ptr logical_block,
                    const t_pl_loc& loc,
                    const BlkLocRegistry& blk_loc_registry);

int get_random_layer(t_logical_block_type_ptr logical_block, vtr::RngContainer& rng);

/**
 * @brief Iterate over all layers and get the maximum x and y over that layers that have a valid value. set the layer min and max
 * based on the layers that have a valid BB.
 * @param tbb_vec
 * @return 3D bounding box
 */
t_bb union_2d_bb(const std::vector<t_2D_bb>& tbb_vec);

/**
 * @brief Iterate over all layers and get the maximum x and y over that layers that have a valid value. Create the "num_edge" in a similar way. This data structure
 * stores how many blocks are on each edge of the BB. set the layer min and max based on the layers that have a valid BB.
 * @param num_edge_vec
 * @param bb_vec
 * @return num_edge, 3D bb
 */
std::pair<t_bb, t_bb> union_2d_bb_incr(const std::vector<t_2D_bb>& num_edge_vec,
                                       const std::vector<t_2D_bb>& bb_vec);

/**
 * @brief If the block ID passed to the placer_debug_net parameter of the command line is equal to blk_id, or if any of the nets
 * connected to the block share the same ID as the net ID passed to the placer_debug_net parameter of the command line,
 * then debugging information should be printed.
 *
 * @param placer_opts
 * @param blk_id The ID of the block that is considered to be moved
 */
void enable_placer_debug(const t_placer_opts& placer_opts,
                         ClusterBlockId blk_id);

