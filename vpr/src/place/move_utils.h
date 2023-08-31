#ifndef VPR_MOVE_UTILS_H
#define VPR_MOVE_UTILS_H
#include "vpr_types.h"
#include "move_transactions.h"
#include "compressed_grid.h"

/* Cut off for incremental bounding box updates.                          *
 * 4 is fastest -- I checked.                                             */
/* To turn off incremental bounding box updates, set this to a huge value */
#define SMALL_NET 4

/* This is for the placement swap routines. A swap attempt could be       *
 * rejected, accepted or aborted (due to the limitations placed on the    *
 * carry chain support at this point).                                    */
enum e_move_result {
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

//Records a reasons for an aborted move
void log_move_abort(const std::string& reason);

//Prints a breif report about aborted move reasons and counts
void report_aborted_moves();

e_create_move create_move(t_pl_blocks_to_be_moved& blocks_affected, ClusterBlockId b_from, t_pl_loc to);

e_block_move_result find_affected_blocks(t_pl_blocks_to_be_moved& blocks_affected, ClusterBlockId b_from, t_pl_loc to);

e_block_move_result record_single_block_swap(t_pl_blocks_to_be_moved& blocks_affected, ClusterBlockId b_from, t_pl_loc to);

e_block_move_result record_macro_swaps(t_pl_blocks_to_be_moved& blocks_affected, const int imacro_from, int& imember_from, t_pl_offset swap_offset);
e_block_move_result record_macro_macro_swaps(t_pl_blocks_to_be_moved& blocks_affected, const int imacro_from, int& imember_from, const int imacro_to, ClusterBlockId blk_to, t_pl_offset swap_offset);

e_block_move_result record_macro_move(t_pl_blocks_to_be_moved& blocks_affected,
                                      std::vector<ClusterBlockId>& displaced_blocks,
                                      const int imacro,
                                      t_pl_offset swap_offset);
e_block_move_result identify_macro_self_swap_affected_macros(std::vector<int>& macros, const int imacro, t_pl_offset swap_offset);
e_block_move_result record_macro_self_swaps(t_pl_blocks_to_be_moved& blocks_affected, const int imacro, t_pl_offset swap_offset);

bool is_legal_swap_to_location(ClusterBlockId blk, t_pl_loc to);

std::set<t_pl_loc> determine_locations_emptied_by_move(t_pl_blocks_to_be_moved& blocks_affected);

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
ClusterBlockId propose_block_to_move(int& logical_blk_type_index, bool highly_crit_block, ClusterNetId* net_from, int* pin_from);

/**
 * @brief Select a random block to be swapped with another block
 * 
 * @return BlockId of the selected block, ClusterBlockId::INVALID() if no block with specified block type found
 */
ClusterBlockId pick_from_block();

/**
 * @brief Find a block with a specific block type to be swapped with another block
 *
 *  @param logical_blk_type_index: the agent type of the moving block.
 * 
 * @return BlockId of the selected block, ClusterBlockId::INVALID() if no block with specified block type found
 */
ClusterBlockId pick_from_block(int logical_blk_type_index);

/**
 * @brief Select a random highly critical block to be swapped with another block
 * 
 * @return BlockId of the selected block, ClusterBlockId::INVALID() if no block with specified block type found
 */
ClusterBlockId pick_from_highly_critical_block(ClusterNetId& net_from, int& pin_from);

/**
 * @brief Find a block with a specific block type to be swapped with another block
 *
 *  @param logical_blk_type_index: the agent type of the moving block.
 * 
 * @return BlockId of the selected block, ClusterBlockId::INVALID() if no block with specified block type found
 */
ClusterBlockId pick_from_highly_critical_block(ClusterNetId& net_from, int& pin_from, int logical_blk_type_index);

bool find_to_loc_uniform(t_logical_block_type_ptr type,
                         float rlim,
                         const t_pl_loc from,
                         t_pl_loc& to,
                         ClusterBlockId b_from);

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
 *  @param blk_type: the type of the moving block
 *  @param from_loc: the original location of the moving block
 *  @param limit_coords: the region where I can move the block to
 *  @param to_loc: the new location that the function picked for the block
 */
bool find_to_loc_median(t_logical_block_type_ptr blk_type, const t_pl_loc& from_loc, const t_bb* limit_coords, t_pl_loc& to_loc, ClusterBlockId b_from);

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
                          ClusterBlockId b_from);

const std::string& move_type_to_string(e_move_type);

/* find to loaction helper functions */
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
                            t_pl_loc& to_loc);
/**
 * @brief find compressed location in a compressed range for a specific type in the given layer (to_layer_num)
 * 
 * type: defines the moving block type
 * min_cx, max_cx: the minimum and maximum x coordinates of the range in the compressed grid
 * min_cy, max_cx: the minimum and maximum y coordinates of the range in the compressed grid
 * cx_from, cy_from: the x and y coordinates of the old location 
 * cx_to, cy_to: the x and y coordinates of the new location on the compressed grid
 * is_median: true if this is called from find_to_loc_median
 * to_layer_num: the layer number of the new location (set by the caller)
 */
bool find_compatible_compressed_loc_in_range(t_logical_block_type_ptr type,
                                             const int delta_cx,
                                             const t_physical_tile_loc& from_loc,
                                             t_bb search_range,
                                             t_physical_tile_loc& to_loc,
                                             bool is_median,
                                             int to_layer_num);

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
 * @param num_layers
 * @return A compressed search range for each layer
 */
std::vector<t_bb> get_compressed_grid_target_search_range(const t_compressed_block_grid& compressed_block_grid,
                                                          const std::vector<t_physical_tile_loc>& compressed_locs,
                                                          float rlim,
                                                          int num_layers);

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
 * @param num_layers
 * @return
 */
std::vector<t_bb> get_compressed_grid_bounded_search_range(const t_compressed_block_grid& compressed_block_grid,
                                                           const std::vector<t_physical_tile_loc>& from_compressed_loc,
                                                           const std::vector<t_physical_tile_loc>& target_compressed_loc,
                                                           float rlim,
                                                           int num_layers);

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
bool intersect_range_limit_with_floorplan_constraints(t_logical_block_type_ptr type,
                                                      ClusterBlockId b_from,
                                                      t_bb& search_range,
                                                      int& delta_cx,
                                                      int layer_num);

std::string e_move_result_to_string(e_move_result move_outcome);

#endif
