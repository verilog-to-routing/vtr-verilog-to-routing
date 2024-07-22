/**
 * @file net_cost_handler.cpp
 * @brief This file contains the implementation of functions used to update placement cost when a new move is proposed/committed.
 * 
 * VPR placement cost consists of three terms which represent wirelength, timing, and NoC cost. 
 * 
 * To get an estimation of the wirelength of each net, the Half Perimeter Wire Length (HPWL) approach is used. In this approach, 
 * half of the perimeter of the bounding box which contains all terminals of the net is multiplied by a correction factor, 
 * and the resulting number is considered as an estimation of the bounding box. 
 * 
 * Currently, we have two types of bounding boxes: 3D bounding box (or Cube BB) and per-layer bounding box. 
 * If the FPGA grid is a 2D structure, a Cube bounding box is used, which will always have the z direction equal to 1. For 3D architectures, 
 * the user can specify the type of bounding box. If no type is specified, the RR graph is analyzed. If all inter-die connections happen from OPINs, 
 * the Cube bounding box is chosen; otherwise, the per-layer bounding box is chosen. In the Cube bounding box, when a net is stretched across multiple layers, 
 * the edges of the bounding box are determined by all of the blocks on all layers. 
 * When the per-layer bounding box is used, a separate bounding box for each layer is created, and the wirelength estimation for each layer is calculated. 
 * To get the total wirelength of a net, the wirelength estimation on all layers is summed up. For more details, please refer to Amin Mohaghegh's MASc thesis. 
 * 
 * For timing estimation, the placement delay model is used. For 2D architectures, you can think of the placement delay model as a 2D array indexed by dx and dy. 
 * To get a delay estimation of a connection (from a source to a sink), first, dx and dy between these two points should be calculated, 
 * and these two numbers are the indices to access this 2D array. By default, the placement delay model is created by iterating over the router lookahead 
 * to get the minimum cost for each dx and dy.
 * 
 * @date July 12, 2024
 */
#include "net_cost_handler.h"
#include "globals.h"
#include "placer_globals.h"
#include "move_utils.h"
#include "place_timing_update.h"
#include "noc_place_utils.h"
#include "vtr_math.h"

using std::max;
using std::min;

/**
 * @brief for the states of the bounding box. 
 * Stored as char for memory efficiency.                              
 */
enum class NetUpdateState {
    NOT_UPDATED_YET,
    UPDATED_ONCE,
    GOT_FROM_SCRATCH
};

/** 
 * @brief The error tolerance due to round off for the total cost computation. 
 * When we check it from scratch vs. incrementally. 0.01 means that there is a 1% error tolerance.      
 */
#define ERROR_TOL .01

const int MAX_FANOUT_CROSSING_COUNT = 50;

/** 
 * @brief Crossing counts for nets with different #'s of pins.  From 
 * ICCAD 94 pp. 690 - 695 (with linear interpolation applied by me).   
 * Multiplied to bounding box of a net to better estimate wire length  
 * for higher fanout nets. Each entry is the correction factor for the 
 * fanout index-1
 */
static const float cross_count[MAX_FANOUT_CROSSING_COUNT] = {/* [0..49] */ 1.0, 1.0, 1.0, 1.0828,
                                      1.1536, 1.2206, 1.2823, 1.3385, 1.3991, 1.4493, 1.4974, 1.5455, 1.5937,
                                      1.6418, 1.6899, 1.7304, 1.7709, 1.8114, 1.8519, 1.8924, 1.9288, 1.9652,
                                      2.0015, 2.0379, 2.0743, 2.1061, 2.1379, 2.1698, 2.2016, 2.2334, 2.2646,
                                      2.2958, 2.3271, 2.3583, 2.3895, 2.4187, 2.4479, 2.4772, 2.5064, 2.5356,
                                      2.5610, 2.5864, 2.6117, 2.6371, 2.6625, 2.6887, 2.7148, 2.7410, 2.7671,
                                      2.7933};

/** 
 * @brief Matrices below are used to precompute the inverse of the average   
 * number of tracks per channel between [subhigh] and [sublow].  Access 
 * them as chan?_place_cost_fac[subhigh][sublow].  They are used to     
 * speed up the computation of the cost function that takes the length  
 * of the net bounding box in each dimension, divided by the average    
 * number of tracks in that direction; for other cost functions they    
 * will never be used.                                                  
 */
static vtr::NdMatrix<float, 2> chanx_place_cost_fac({0, 0}); // [0...device_ctx.grid.width()-2]
static vtr::NdMatrix<float, 2> chany_place_cost_fac({0, 0}); // [0...device_ctx.grid.height()-2]

/**
 * @brief Cost of a net, and a temporary cost of a net used during move assessment. 
 * We also use negative cost values in proposed_net_cost as a flag to indicate that 
 * the cost of a net has not yet been updated.
 */
static vtr::vector<ClusterNetId, double> net_cost, proposed_net_cost;

/**                                              *
 * @brief Flag array to indicate whether the specific bounding box has been updated
 * in this particular swap or not. If it has been updated before, the code    
 * must use the updated data, instead of the out-of-date data passed into the 
 * subroutine, particularly used in try_swap(). The value NOT_UPDATED_YET     
 * indicates that the net has not been updated before, UPDATED_ONCE indicated 
 * that the net has been updated once, if it is going to be updated again, the
 * values from the previous update must be used. GOT_FROM_SCRATCH is only     
 * applicable for nets larger than SMALL_NETS and it indicates that the       
 * particular bounding box is not incrementally updated, and hence the
 * bounding box is got from scratch, so the bounding box would definitely be
 * right, DO NOT update again.                                                   
 */
static vtr::vector<ClusterNetId, NetUpdateState> bb_updated_before; // [0...cluster_ctx.clb_nlist.nets().size()-1]

/* The following arrays are used by the try_swap function for speed.   */

/**
 * The wire length estimation is based on the bounding box of the net. In the case of the 2D architecture,
 * we use a 3D BB with the z-dimension (layer) set to 1. In the case of 3D architecture, there 2 types of bounding box:
 * 3D and per-layer. The type is determined at the beginning of the placement and stored in the placement context.
 *
 *
 * If the bonding box is of the type 3D, ts_bb_coord_new and ts_bb_edge_new are used. Otherwise, layer_ts_bb_edge_new and
 * layer_ts_bb_coord_new are used.
 */

/* [0...cluster_ctx.clb_nlist.nets().size()-1] -> 3D bounding box*/
static vtr::vector<ClusterNetId, t_bb> ts_bb_coord_new, ts_bb_edge_new;
/* [0...cluster_ctx.clb_nlist.nets().size()-1][0...num_layers-1] -> 2D bonding box on a layer*/
static vtr::vector<ClusterNetId, std::vector<t_2D_bb>> layer_ts_bb_edge_new, layer_ts_bb_coord_new;
/* [0...cluster_ctx.clb_nlist.nets().size()-1][0...num_layers-1] -> number of sink pins on a layer*/
static vtr::Matrix<int> ts_layer_sink_pin_count;
/* [0...num_afftected_nets] -> net_id of the affected nets */
static std::vector<ClusterNetId> ts_nets_to_update;

/**
 * @param net
 * @param moved_blocks
 * @return True if the driver block of the net is among the moving blocks
 */
static bool driven_by_moved_block(const ClusterNetId net,
                                  const int num_blocks,
                                  const std::vector<t_pl_moved_block>& moved_blocks);
/**
 * @brief Update the bounding box (3D) of the net connected to blk_pin. The old and new locations of the pin are
 * stored in pl_moved_block. The updated bounding box will be stored in ts data structures. Do not update the net 
 * cost here since it should only be updated once per net, not once per pin.
 * @param net
 * @param blk
 * @param blk_pin
 * @param pl_moved_block
 */
static void update_net_bb(const ClusterNetId& net,
                          const ClusterBlockId& blk,
                          const ClusterPinId& blk_pin,
                          const t_pl_moved_block& pl_moved_block);

/**
 * @brief Calculate the new connection delay and timing cost of all the
 * sink pins affected by moving a specific pin to a new location. Also 
 * calculates the total change in the timing cost.
 * @param delay_model
 * @param criticalities
 * @param net
 * @param pin
 * @param affected_pins Updated by this routine to store the sink pins whose delays are changed due to moving the block
 * @param delta_timing_cost Computed by this routine and returned by reference.
 * @param is_src_moving True if "pin" is a sink pin and its driver is among the moving blocks
 */
static void update_td_delta_costs(const PlaceDelayModel* delay_model,
                                  const PlacerCriticalities& criticalities,
                                  const ClusterNetId net,
                                  const ClusterPinId pin,
                                  std::vector<ClusterPinId>& affected_pins,
                                  double& delta_timing_cost,
                                  bool is_src_moving);

/**
 * @brief if "net" is not already stored as an affected net, mark it in ts_nets_to_update and increment num_affected_nets
 * @param net ID of a net affected by a move
 * @param num_affected_nets Incremented if this is a new net affected, and returned via reference.
 */
static void record_affected_net(const ClusterNetId net, int& num_affected_nets);

/**
 * @brief Call suitable function based on the bounding box type to update the bounding box of the net connected to pin_id. Also,
 * call the function to update timing information if the placement algorithm is timing-driven.
 * @param place_algorithm Placement algorithm
 * @param delay_model Timing delay model used by placer
 * @param criticalities Connections timing criticalities
 * @param blk_id Block ID of that the moving pin blongs to.
 * @param pin_id Pin ID of the moving pin
 * @param moving_blk_inf Data structure that holds information, e.g., old location and new locatoin, about all moving blocks
 * @param affected_pins Netlist pins which are affected, in terms placement cost, by the proposed move.
 * @param timing_delta_c Timing cost change based on the proposed move
 * @param num_affected_nets A pointer to the first free element of ts_nets_to_update. If a new net is added, the pointer should be increamented.
 * @param is_src_moving Is the moving pin the source of a net.
 */
static void update_net_info_on_pin_move(const t_place_algorithm& place_algorithm,
                                               const PlaceDelayModel* delay_model,
                                               const PlacerCriticalities* criticalities,
                                               const ClusterBlockId& blk_id,
                                               const ClusterPinId& pin_id,
                                               const t_pl_moved_block& moving_blk_inf,
                                               std::vector<ClusterPinId>& affected_pins,
                                               double& timing_delta_c,
                                               int& num_affected_nets,
                                               bool is_src_moving);
                                            
/**
 * @brief Update the 3D bounding box of "net_id" incrementally based on the old and new locations of a pin on that net
 * @details Updates the bounding box of a net by storing its coordinates in the bb_coord_new data structure and the 
 * number of blocks on each edge in the bb_edge_new data structure. This routine should only be called for large nets, 
 * since it has some overhead relative to just doing a brute force bounding box calculation. The bounding box coordinate 
 * and edge information for inet must be valid before this routine is called. Currently assumes channels on both sides of 
 * the CLBs forming the edges of the bounding box can be used.  Essentially, I am assuming the pins always lie on the 
 * outside of the bounding box. The x and y coordinates are the pin's x and y coordinates. IO blocks are considered to be one 
 * cell in for simplicity.     
 * @param bb_edge_new Number of blocks on the edges of the bounding box
 * @param bb_coord_new Coordinates of the bounding box
 * @param num_sink_pin_layer_new Number of sinks of the given net on each layer
 * @param pin_old_loc The old location of the moving pin
 * @param pin_new_loc The new location of the moving pin
 * @param src_pin Is the moving pin driving the net
 */
static void update_bb(ClusterNetId net_id,
                      t_bb& bb_edge_new,
                      t_bb& bb_coord_new,
                      vtr::NdMatrixProxy<int, 1> num_sink_pin_layer_new,
                      t_physical_tile_loc pin_old_loc,
                      t_physical_tile_loc pin_new_loc,
                      bool src_pin);

/**
 * @brief Calculate the 3D bounding box of "net_id" from scratch (based on the block locations stored in place_ctx) and
 * store them in bb_coord_new
 * @param net_id ID of the net for which the bounding box is requested
 * @param bb_coord_new Computed by this function and returned by reference.
 * @param num_sink_pin_layer Store the number of sink pins of "net_id" on each layer
 */
static void get_non_updatable_bb(ClusterNetId net_id,
                                  t_bb& bb_coord_new,
                                  vtr::NdMatrixProxy<int, 1> num_sink_pin_layer);


/**
 * @brief Update the bounding box (per-layer) of the net connected to blk_pin. The old and new locations of the pin are
 * stored in pl_moved_block. The updated bounding box will be stored in ts data structures.
 * @details Finds the bounding box of a net and stores its coordinates in the bb_coord_new 
 * data structure.  This routine should only be called for small nets, since it does not 
 * determine enough information for the bounding box to be updated incrementally later.                
 * Currently assumes channels on both sides of the CLBs forming the edges of the bounding box 
 * can be used.  Essentially, I am assuming the pins always lie on the outside of the 
 * bounding box.            
 * @param net ID of the net for which the bounding box is requested
 * @param blk ID of the moving block
 * @param blk_pin ID of the pin connected to the net
 * @param pl_moved_block Placement info about
 */
static void update_net_layer_bb(const ClusterNetId& net,
                                const ClusterBlockId& blk,
                                const ClusterPinId& blk_pin,
                                const t_pl_moved_block& pl_moved_block);

/**
 * @brief Calculate the per-layer bounding box of "net_id" from scratch (based on the block locations stored in place_ctx) and
 * store them in bb_coord_new
 * @param net_id ID of the net for which the bounding box is requested
 * @param bb_coord_new Computed by this function and returned by reference.
 * @param num_sink_layer Store the number of sink pins of "net_id" on each layer
 */
static void get_non_updatable_layer_bb(ClusterNetId net_id,
                                       std::vector<t_2D_bb>& bb_coord_new,
                                       vtr::NdMatrixProxy<int, 1> num_sink_layer);


/**
 * @brief Update the per-layer bounding box of "net_id" incrementally based on the old and new locations of a pin on that net
 * @details Updates the bounding box of a net by storing its coordinates in the bb_coord_new data structure and 
 * the number of blocks on each edge in the bb_edge_new data structure. This routine should only  be called for 
 * large nets, since it has some overhead relative to just doing a brute force bounding box calculation. 
 * The bounding box coordinate and edge information for inet must be valid before  this routine is called. 
 * Currently assumes channels on both sides of the CLBs forming the   edges of the bounding box can be used.  
 * Essentially, I am assuming the pins always lie on the outside of the bounding box. The x and y coordinates 
 * are the pin's x and y coordinates. IO blocks are considered to be one cell in for simplicity.
 * @param bb_edge_new Number of blocks on the edges of the bounding box
 * @param bb_coord_new Coordinates of the bounding box
 * @param num_sink_pin_layer_new Number of sinks of the given net on each layer
 * @param pin_old_loc The old location of the moving pin
 * @param pin_new_loc The new location of the moving pin
 * @param is_output_pin Is the moving pin of the type output
 */
static void update_layer_bb(ClusterNetId net_id,
                            std::vector<t_2D_bb>& bb_edge_new,
                            std::vector<t_2D_bb>& bb_coord_new,
                            vtr::NdMatrixProxy<int, 1> bb_pin_sink_count_new,
                            t_physical_tile_loc pin_old_loc,
                            t_physical_tile_loc pin_new_loc,
                            bool is_output_pin);

/**
* @brief This function is called in update_layer_bb to update the net's bounding box incrementally if
* the pin under consideration change layer.
 * @param net_id ID of the net which the moving pin belongs to
 * @param pin_old_loc Old location of the moving pin
 * @param pin_new_loc New location of the moving pin
 * @param curr_bb_edge The current known number of blocks of the net on bounding box edges
 * @param curr_bb_coord The current known boudning box of the net
 * @param bb_pin_sink_count_new The updated number of net's sinks on each layer
 * @param bb_edge_new The new bb edge calculated by this function
 * @param bb_coord_new The new bb calculated by this function
 */
static inline void update_bb_layer_changed(ClusterNetId net_id,
                                           const t_physical_tile_loc& pin_old_loc,
                                           const t_physical_tile_loc& pin_new_loc,
                                           const std::vector<t_2D_bb>& curr_bb_edge,
                                           const std::vector<t_2D_bb>& curr_bb_coord,
                                           vtr::NdMatrixProxy<int, 1> bb_pin_sink_count_new,
                                           std::vector<t_2D_bb>& bb_edge_new,
                                           std::vector<t_2D_bb>& bb_coord_new);
                                
/**
 * @brief Calculate the per-layer BB of a large net from scratch and update coord, edge, and num_sink_pin_layer data structures.
 * @details This routine finds the bounding box of each net from scratch when the bounding box is of type per-layer (i.e. from 
 * only the block location information). It updates the coordinate, number of pins on each edge information, and the 
 * number of sinks on each layer. It should only be called when the bounding box information is not valid.
 * @param net_id ID of the net which the moving pin belongs to
 * @param coords Bounding box coordinates of the net. It is calculated in this function
 * @param num_on_edges Net's number of blocks on the edges of the bounding box. It is calculated in this function.
 * @param num_sink_pin_layer Net's number of sinks on each layer, calculated in this function.
 */
static void get_layer_bb_from_scratch(ClusterNetId net_id,
                                      std::vector<t_2D_bb>& num_on_edges,
                                      std::vector<t_2D_bb>& coords,
                                      vtr::NdMatrixProxy<int, 1> layer_pin_sink_count);

/**
 * @brief Given the per-layer BB, calculate the wire-length cost of the net on each layer
 * and return the sum of the costs
 * @param net_id ID of the net which cost is requested
 * @param bb Per-layer bounding box of the net
 * @return Wirelength cost of the net
 */
static double get_net_layer_bb_wire_cost(ClusterNetId /* net_id */,
                                 const std::vector<t_2D_bb>& bb,
                                 const vtr::NdMatrixProxy<int, 1> layer_pin_sink_count);

/**
 * @brief Given the per-layer BB, calculate the wire-length estimate of the net on each layer
 * and return the sum of the lengths
 * @param net_id ID of the net which wirelength estimate is requested
 * @param bb Bounding box of the net
 * @return Wirelength estimate of the net
 */
static double get_net_wirelength_from_layer_bb(ClusterNetId /* net_id */,
                                                const std::vector<t_2D_bb>& bb,
                                                const vtr::NdMatrixProxy<int, 1> layer_pin_sink_count);

/**
 * @brief This function is called in update_layer_bb to update the net's bounding box incrementally if
 * the pin under consideration is not changing layer.
 * @param net_id ID of the net which the moving pin belongs to
 * @param pin_old_loc Old location of the moving pin
 * @param pin_new_loc New location of the moving pin
 * @param curr_bb_edge The current known number of blocks of the net on bounding box edges
 * @param curr_bb_coord The current known boudning box of the net
 * @param bb_pin_sink_count_new The updated number of net's sinks on each layer
 * @param bb_edge_new The new bb edge calculated by this function
 * @param bb_coord_new The new bb calculated by this function
 */
static inline void update_bb_same_layer(ClusterNetId net_id,
                                        const t_physical_tile_loc& pin_old_loc,
                                        const t_physical_tile_loc& pin_new_loc,
                                        const std::vector<t_2D_bb>& curr_bb_edge,
                                        const std::vector<t_2D_bb>& curr_bb_coord,
                                        vtr::NdMatrixProxy<int, 1> bb_pin_sink_count_new,
                                        std::vector<t_2D_bb>& bb_edge_new,
                                        std::vector<t_2D_bb>& bb_coord_new);

/**
 * @brief If the moving pin is of type type SINK, update bb_pin_sink_count_new which stores the number of sink pins on each layer of "net_id"
 * @param pin_old_loc Old location of the moving pin
 * @param pin_new_loc New location of the moving pin
 * @param curr_layer_pin_sink_count Updated number of sinks of the net on each layer
 * @param bb_pin_sink_count_new The updated number of net's sinks on each layer
 * @param is_output_pin Is the moving pin of the type output
 */
static void update_bb_pin_sink_count(const t_physical_tile_loc& pin_old_loc,
                                     const t_physical_tile_loc& pin_new_loc,
                                     const vtr::NdMatrixProxy<int, 1> curr_layer_pin_sink_count,
                                     vtr::NdMatrixProxy<int, 1> bb_pin_sink_count_new,
                                     bool is_output_pin);

/**
 * @brief Update the data structure for large nets that keep track of
 * the number of blocks on each edge of the bounding box. If the moving block
 * is the only block on one of the edges, the bounding box is calculated from scratch.
 * Since this function is used for large nets, it updates the bounding box incrementally.
 * @param net_id ID of the net which the moving pin belongs to
 * @param bb_edge_new The new bb edge calculated by this function
 * @param bb_coord_new The new bb calculated by this function
 * @param bb_layer_pin_sink_count The updated number of net's sinks on each layer
 * @param old_num_block_on_edge The current known number of blocks of the net on bounding box edges
 * @param old_edge_coord The current known boudning box of the net
 * @param new_num_block_on_edge The new bb calculated by this function
 * @param new_edge_coord The new bb edge calculated by this function
 *
 */
static inline void update_bb_edge(ClusterNetId net_id,
                                  std::vector<t_2D_bb>& bb_edge_new,
                                  std::vector<t_2D_bb>& bb_coord_new,
                                  vtr::NdMatrixProxy<int, 1> bb_layer_pin_sink_count,
                                  const int& old_num_block_on_edge,
                                  const int& old_edge_coord,
                                  int& new_num_block_on_edge,
                                  int& new_edge_coord);

/**
 * @brief When BB is being updated incrementally, the pin is moving to a new layer, and the BB is of the type "per-layer,
 * use this function to update the BB on the new layer.
 * @param new_pin_loc New location of the pin
 * @param bb_edge_old bb_edge prior to moving the pin
 * @param bb_coord_old bb_coord prior to moving the pin
 * @param bb_edge_new New bb edge calculated by this function
 * @param bb_coord_new new bb coord calculated by this function
 */
static void add_block_to_bb(const t_physical_tile_loc& new_pin_loc,
                            const t_2D_bb& bb_edge_old,
                            const t_2D_bb& bb_coord_old,
                            t_2D_bb& bb_edge_new,
                            t_2D_bb& bb_coord_new);

/**
 * @brief Calculate the 3D BB of a large net from scratch and update coord, edge, and num_sink_pin_layer data structures.
 * @details This routine finds the bounding box of each net from scratch (i.e. from only the block location information).  It updates both the       
 * coordinate and number of pins on each edge information. It should only be called when the bounding box 
 * information is not valid.
 * @param net_id ID of the net which the moving pin belongs to
 * @param coords Bounding box coordinates of the net. It is calculated in this function
 * @param num_on_edges Net's number of blocks on the edges of the bounding box. It is calculated in this function.
 * @param num_sink_pin_layer Net's number of sinks on each layer, calculated in this function.
 */
static void get_bb_from_scratch(ClusterNetId net_id,
                                t_bb& coords,
                                t_bb& num_on_edges,
                                vtr::NdMatrixProxy<int, 1> num_sink_pin_layer);

/**
 * @brief Given the 3D BB, calculate the wire-length cost of the net
 * @param net_id ID of the net which cost is requested
 * @param bb Bounding box of the net
 * @return Wirelength cost of the net
 */
static double get_net_cost(ClusterNetId net_id, const t_bb& bb);



/**
 * @brief Given the 3D BB, calculate the wire-length estimate of the net
 * @param net_id ID of the net which wirelength estimate is requested
 * @param bb Bounding box of the net
 * @return Wirelength estimate of the net
 */
static double get_net_wirelength_estimate(ClusterNetId net_id, const t_bb& bb);



/**
 * @brief To mitigate round-off errors, every once in a while, the costs of nets are summed up from scratch.
 * This functions is called to do that for bb cost. It doesn't calculate the BBs from scratch, it would only add the costs again.
 * @return Total bb (wirelength) cost for the placement
 */
static double recompute_bb_cost();

/**
 * @brief To get the wirelength cost/est, BB perimeter is multiplied by a factor to approximately correct for the half-perimeter 
 * bounding box wirelength's underestimate of wiring for nets with fanout greater than 2.
 * @return Multiplicative wirelength correction factor
 */
static double wirelength_crossing_count(size_t fanout);

/**
 * @brief Calculates and returns the total bb (wirelength) cost change that would result from moving the blocks 
 * indicated in the blocks_affected data structure.
 * @param num_affected_nets Number of valid elements in ts_bb_coord_new 
 * @param bb_delta_c Cost difference after and before moving the block
 */
static void set_bb_delta_cost(const int num_affected_nets, double& bb_delta_c);

/******************************* End of Function definitions ************************************/

//Returns true if 'net' is driven by one of the blocks in 'blocks_affected'
static bool driven_by_moved_block(const ClusterNetId net,
                                  const int num_blocks,
                                  const std::vector<t_pl_moved_block>& moved_blocks) {
    auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    bool is_driven_by_move_blk = false;
    ClusterBlockId net_driver_block = clb_nlist.net_driver_block(
        net);

    for (int block_num = 0; block_num < num_blocks; block_num++) {
        if (net_driver_block == moved_blocks[block_num].block_num) {
            is_driven_by_move_blk = true;
            break;
        }
    }

    return is_driven_by_move_blk;
}

static void update_net_bb(const ClusterNetId& net,
                          const ClusterBlockId& blk,
                          const ClusterPinId& blk_pin,
                          const t_pl_moved_block& pl_moved_block) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    if (cluster_ctx.clb_nlist.net_sinks(net).size() < SMALL_NET) {
        //For small nets brute-force bounding box update is faster

        if (bb_updated_before[net] == NetUpdateState::NOT_UPDATED_YET) { //Only once per-net
            get_non_updatable_bb(net,
                                 ts_bb_coord_new[net],
                                 ts_layer_sink_pin_count[size_t(net)]);
        }
    } else {
        //For large nets, update bounding box incrementally
        int iblk_pin = tile_pin_index(blk_pin);
        bool src_pin = cluster_ctx.clb_nlist.pin_type(blk_pin) == PinType::DRIVER;

        t_physical_tile_type_ptr blk_type = physical_tile_type(blk);
        int pin_width_offset = blk_type->pin_width_offset[iblk_pin];
        int pin_height_offset = blk_type->pin_height_offset[iblk_pin];

        //Incremental bounding box update
        update_bb(net,
                  ts_bb_edge_new[net],
                  ts_bb_coord_new[net],
                  ts_layer_sink_pin_count[size_t(net)],
                  {pl_moved_block.old_loc.x + pin_width_offset,
                  pl_moved_block.old_loc.y + pin_height_offset,
                  pl_moved_block.old_loc.layer},
                  {pl_moved_block.new_loc.x + pin_width_offset,
                  pl_moved_block.new_loc.y + pin_height_offset,
                  pl_moved_block.new_loc.layer},
                  src_pin);
    }
}

static void update_net_layer_bb(const ClusterNetId& net,
                                const ClusterBlockId& blk,
                                const ClusterPinId& blk_pin,
                                const t_pl_moved_block& pl_moved_block) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    if (cluster_ctx.clb_nlist.net_sinks(net).size() < SMALL_NET) {
        //For small nets brute-force bounding box update is faster

        if (bb_updated_before[net] == NetUpdateState::NOT_UPDATED_YET) { //Only once per-net
            get_non_updatable_layer_bb(net,
                                       layer_ts_bb_coord_new[net],
                                       ts_layer_sink_pin_count[size_t(net)]);
        }
    } else {
        //For large nets, update bounding box incrementally
        int iblk_pin = tile_pin_index(blk_pin);

        t_physical_tile_type_ptr blk_type = physical_tile_type(blk);
        int pin_width_offset = blk_type->pin_width_offset[iblk_pin];
        int pin_height_offset = blk_type->pin_height_offset[iblk_pin];

        auto pin_dir = get_pin_type_from_pin_physical_num(blk_type, iblk_pin);

        //Incremental bounding box update
        update_layer_bb(net,
                        layer_ts_bb_edge_new[net],
                        layer_ts_bb_coord_new[net],
                        ts_layer_sink_pin_count[size_t(net)],
                        {pl_moved_block.old_loc.x + pin_width_offset,
                         pl_moved_block.old_loc.y + pin_height_offset,
                         pl_moved_block.old_loc.layer},
                        {pl_moved_block.new_loc.x + pin_width_offset,
                         pl_moved_block.new_loc.y + pin_height_offset,
                         pl_moved_block.new_loc.layer},
                        pin_dir == e_pin_type::DRIVER);
    }
}

static void update_td_delta_costs(const PlaceDelayModel* delay_model,
                                  const PlacerCriticalities& criticalities,
                                  const ClusterNetId net,
                                  const ClusterPinId pin,
                                  std::vector<ClusterPinId>& affected_pins,
                                  double& delta_timing_cost,
                                  bool is_src_moving) {

    /**
     * Assumes that the blocks have been moved to the proposed new locations.
     * Otherwise, the routine comp_td_single_connection_delay() will not be
     * able to calculate the most up to date connection delay estimation value.
     *
     * If the moved pin is a driver pin, then all the sink connections that are
     * driven by this driver pin are considered.
     *
     * If the moved pin is a sink pin, then it is the only pin considered. But
     * in some cases, the sink is already accounted for if it is also driven
     * by a driver pin located on a moved block. Computing it again would double
     * count its affect on the total timing cost change (delta_timing_cost).
     *
     * It is possible for some connections to have unchanged delays. For instance,
     * if we are using a dx/dy delay model, this could occur if a sink pin moved
     * to a new position with the same dx/dy from its net's driver pin.
     *
     * We skip these connections with unchanged delay values as their delay need
     * not be updated. Their timing costs also do not require any update, since
     * the criticalities values are always kept stale/unchanged during an block
     * swap attempt. (Unchanged Delay * Unchanged Criticality = Unchanged Cost)
     *
     * This is also done to minimize the number of timing node/edge invalidations
     * for incremental static timing analysis (incremental STA).
     */
    auto& cluster_ctx = g_vpr_ctx.clustering();

    const auto& connection_delay = g_placer_ctx.timing().connection_delay;
    auto& connection_timing_cost = g_placer_ctx.mutable_timing().connection_timing_cost;
    auto& proposed_connection_delay = g_placer_ctx.mutable_timing().proposed_connection_delay;
    auto& proposed_connection_timing_cost = g_placer_ctx.mutable_timing().proposed_connection_timing_cost;

    if (cluster_ctx.clb_nlist.pin_type(pin) == PinType::DRIVER) {
        /* This pin is a net driver on a moved block. */
        /* Recompute all point to point connection delays for the net sinks. */
        for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net).size();
             ipin++) {
            float temp_delay = comp_td_single_connection_delay(delay_model, net,
                                                               ipin);
            /* If the delay hasn't changed, do not mark this pin as affected */
            if (temp_delay == connection_delay[net][ipin]) {
                continue;
            }

            /* Calculate proposed delay and cost values */
            proposed_connection_delay[net][ipin] = temp_delay;

            proposed_connection_timing_cost[net][ipin] = criticalities.criticality(net, ipin) * temp_delay;
            delta_timing_cost += proposed_connection_timing_cost[net][ipin]
                                 - connection_timing_cost[net][ipin];

            /* Record this connection in blocks_affected.affected_pins */
            ClusterPinId sink_pin = cluster_ctx.clb_nlist.net_pin(net, ipin);
            affected_pins.push_back(sink_pin);
        }
    } else {
        /* This pin is a net sink on a moved block */
        VTR_ASSERT_SAFE(cluster_ctx.clb_nlist.pin_type(pin) == PinType::SINK);

        /* Check if this sink's net is driven by a moved block */
        if (!is_src_moving) {
            /* Get the sink pin index in the net */
            int ipin = cluster_ctx.clb_nlist.pin_net_index(pin);

            float temp_delay = comp_td_single_connection_delay(delay_model, net,
                                                               ipin);
            /* If the delay hasn't changed, do not mark this pin as affected */
            if (temp_delay == connection_delay[net][ipin]) {
                return;
            }

            /* Calculate proposed delay and cost values */
            proposed_connection_delay[net][ipin] = temp_delay;

            proposed_connection_timing_cost[net][ipin] = criticalities.criticality(net, ipin) * temp_delay;
            delta_timing_cost += proposed_connection_timing_cost[net][ipin]
                                 - connection_timing_cost[net][ipin];

            /* Record this connection in blocks_affected.affected_pins */
            affected_pins.push_back(pin);
        }
    }
}

///@brief Record effected nets.
static void record_affected_net(const ClusterNetId net,
                                int& num_affected_nets) {
    /* Record effected nets. */
    if (proposed_net_cost[net] < 0.) {
        /* Net not marked yet. */
        ts_nets_to_update[num_affected_nets] = net;
        num_affected_nets++;

        /* Flag to say we've marked this net. */
        proposed_net_cost[net] = 1.;
    }
}

static void update_net_info_on_pin_move(const t_place_algorithm& place_algorithm,
                                               const PlaceDelayModel* delay_model,
                                               const PlacerCriticalities* criticalities,
                                               const ClusterBlockId& blk_id,
                                               const ClusterPinId& pin_id,
                                               const t_pl_moved_block& moving_blk_inf,
                                               std::vector<ClusterPinId>& affected_pins,
                                               double& timing_delta_c,
                                               int& num_affected_nets,
                                               bool is_src_moving) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(pin_id);
    VTR_ASSERT_SAFE_MSG(net_id,
                        "Only valid nets should be found in compressed netlist block pins");

    if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
        //TODO: Do we require anyting special here for global nets?
        //"Global nets are assumed to span the whole chip, and do not effect costs."
        return;
    }

    /* Record effected nets */
    record_affected_net(net_id, num_affected_nets);

    const auto& cube_bb = g_vpr_ctx.placement().cube_bb;

    /* Update the net bounding boxes. */
    if (cube_bb) {
        update_net_bb(net_id, blk_id, pin_id, moving_blk_inf);
    } else {
        update_net_layer_bb(net_id, blk_id, pin_id, moving_blk_inf);
    }

    if (place_algorithm.is_timing_driven()) {
        /* Determine the change in connection delay and timing cost. */
        update_td_delta_costs(delay_model,
                              *criticalities,
                              net_id,
                              pin_id,
                              affected_pins,
                              timing_delta_c,
                              is_src_moving);
    }
}

static void get_non_updatable_bb(ClusterNetId net_id,
                                t_bb& bb_coord_new,
                                vtr::NdMatrixProxy<int, 1> num_sink_pin_layer) {
    //TODO: account for multiple physical pin instances per logical pin

    int xmax, ymax, layer_max, xmin, ymin, layer_min, x, y, layer;
    int pnum;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();

    ClusterBlockId bnum = cluster_ctx.clb_nlist.net_driver_block(net_id);
    pnum = net_pin_to_tile_pin_index(net_id, 0);

    x = place_ctx.block_locs[bnum].loc.x
        + physical_tile_type(bnum)->pin_width_offset[pnum];
    y = place_ctx.block_locs[bnum].loc.y
        + physical_tile_type(bnum)->pin_height_offset[pnum];
    layer = place_ctx.block_locs[bnum].loc.layer;

    xmin = x;
    ymin = y;
    layer_min = layer;
    xmax = x;
    ymax = y;
    layer_max = layer;

    for (int layer_num = 0; layer_num < device_ctx.grid.get_num_layers(); layer_num++) {
        num_sink_pin_layer[layer_num] = 0;
    }

    for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
        bnum = cluster_ctx.clb_nlist.pin_block(pin_id);
        pnum = tile_pin_index(pin_id);
        x = place_ctx.block_locs[bnum].loc.x
            + physical_tile_type(bnum)->pin_width_offset[pnum];
        y = place_ctx.block_locs[bnum].loc.y
            + physical_tile_type(bnum)->pin_height_offset[pnum];
        layer = place_ctx.block_locs[bnum].loc.layer;

        if (x < xmin) {
            xmin = x;
        } else if (x > xmax) {
            xmax = x;
        }

        if (y < ymin) {
            ymin = y;
        } else if (y > ymax) {
            ymax = y;
        }

        if (layer < layer_min) {
            layer_min = layer;
        } else if (layer > layer_max) {
            layer_max = layer;
        }

        num_sink_pin_layer[layer]++;
    }

    /* Now I've found the coordinates of the bounding box.  There are no *
     * channels beyond device_ctx.grid.width()-2 and                     *
     * device_ctx.grid.height() - 2, so I want to clip to that.  As well,*
     * since I'll always include the channel immediately below and the   *
     * channel immediately to the left of the bounding box, I want to    *
     * clip to 1 in both directions as well (since minimum channel index *
     * is 0).  See route_common.cpp for a channel diagram.               */

    bb_coord_new.xmin = max(min<int>(xmin, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    bb_coord_new.ymin = max(min<int>(ymin, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
    bb_coord_new.layer_min = max(min<int>(layer_min, device_ctx.grid.get_num_layers() - 1), 0);
    bb_coord_new.xmax = max(min<int>(xmax, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    bb_coord_new.ymax = max(min<int>(ymax, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
    bb_coord_new.layer_max = max(min<int>(layer_max, device_ctx.grid.get_num_layers() - 1), 0);
}

static void get_non_updatable_layer_bb(ClusterNetId net_id,
                                       std::vector<t_2D_bb>& bb_coord_new,
                                       vtr::NdMatrixProxy<int, 1> num_sink_layer) {
    //TODO: account for multiple physical pin instances per logical pin

    auto& device_ctx = g_vpr_ctx.device();
    int num_layers = device_ctx.grid.get_num_layers();
    for (int layer_num = 0; layer_num < device_ctx.grid.get_num_layers(); layer_num++) {
        num_sink_layer[layer_num] = 0;
    }

    int pnum;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    ClusterBlockId bnum = cluster_ctx.clb_nlist.net_driver_block(net_id);
    pnum = net_pin_to_tile_pin_index(net_id, 0);

    int src_x = place_ctx.block_locs[bnum].loc.x
                + physical_tile_type(bnum)->pin_width_offset[pnum];
    int src_y = place_ctx.block_locs[bnum].loc.y
                + physical_tile_type(bnum)->pin_height_offset[pnum];

    std::vector<int> xmin(num_layers, src_x);
    std::vector<int> ymin(num_layers, src_y);
    std::vector<int> xmax(num_layers, src_x);
    std::vector<int> ymax(num_layers, src_y);

    for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
        bnum = cluster_ctx.clb_nlist.pin_block(pin_id);
        pnum = tile_pin_index(pin_id);
        int x = place_ctx.block_locs[bnum].loc.x
                + physical_tile_type(bnum)->pin_width_offset[pnum];
        int y = place_ctx.block_locs[bnum].loc.y
                + physical_tile_type(bnum)->pin_height_offset[pnum];

        int layer_num = place_ctx.block_locs[bnum].loc.layer;
        num_sink_layer[layer_num]++;
        if (x < xmin[layer_num]) {
            xmin[layer_num] = x;
        } else if (x > xmax[layer_num]) {
            xmax[layer_num] = x;
        }

        if (y < ymin[layer_num]) {
            ymin[layer_num] = y;
        } else if (y > ymax[layer_num]) {
            ymax[layer_num] = y;
        }
    }

    /* Now I've found the coordinates of the bounding box.  There are no *
     * channels beyond device_ctx.grid.width()-2 and                     *
     * device_ctx.grid.height() - 2, so I want to clip to that.  As well,*
     * since I'll always include the channel immediately below and the   *
     * channel immediately to the left of the bounding box, I want to    *
     * clip to 1 in both directions as well (since minimum channel index *
     * is 0).  See route_common.cpp for a channel diagram.               */
    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        bb_coord_new[layer_num].layer_num = layer_num;
        bb_coord_new[layer_num].xmin = max(min<int>(xmin[layer_num], device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
        bb_coord_new[layer_num].ymin = max(min<int>(ymin[layer_num], device_ctx.grid.height() - 2), 1); //-2 for no perim channels
        bb_coord_new[layer_num].xmax = max(min<int>(xmax[layer_num], device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
        bb_coord_new[layer_num].ymax = max(min<int>(ymax[layer_num], device_ctx.grid.height() - 2), 1); //-2 for no perim channels
    }
}

static void update_bb(ClusterNetId net_id,
                      t_bb& bb_edge_new,
                      t_bb& bb_coord_new,
                      vtr::NdMatrixProxy<int, 1> num_sink_pin_layer_new,
                      t_physical_tile_loc pin_old_loc,
                      t_physical_tile_loc pin_new_loc,
                      bool src_pin) {
    //TODO: account for multiple physical pin instances per logical pin
    const t_bb *curr_bb_edge, *curr_bb_coord;

    auto& device_ctx = g_vpr_ctx.device();
    auto& place_move_ctx = g_placer_ctx.move();

    const int num_layers = device_ctx.grid.get_num_layers();

    pin_new_loc.x = max(min<int>(pin_new_loc.x, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    pin_new_loc.y = max(min<int>(pin_new_loc.y, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
    pin_new_loc.layer_num = max(min<int>(pin_new_loc.layer_num, device_ctx.grid.get_num_layers() - 1), 0);
    pin_old_loc.x = max(min<int>(pin_old_loc.x, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    pin_old_loc.y = max(min<int>(pin_old_loc.y, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
    pin_old_loc.layer_num = max(min<int>(pin_old_loc.layer_num, device_ctx.grid.get_num_layers() - 1), 0);

    /* Check if the net had been updated before. */
    if (bb_updated_before[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
        /* The net had been updated from scratch, DO NOT update again! */
        return;
    }

    vtr::NdMatrixProxy<int, 1> curr_num_sink_pin_layer = (bb_updated_before[net_id] == NetUpdateState::NOT_UPDATED_YET) ? 
    place_move_ctx.num_sink_pin_layer[size_t(net_id)] : num_sink_pin_layer_new;

    if (bb_updated_before[net_id] == NetUpdateState::NOT_UPDATED_YET) {
        /* The net had NOT been updated before, could use the old values */
        curr_bb_edge = &place_move_ctx.bb_num_on_edges[net_id];
        curr_bb_coord = &place_move_ctx.bb_coords[net_id];
        bb_updated_before[net_id] = NetUpdateState::UPDATED_ONCE;
    } else {
        /* The net had been updated before, must use the new values */
        curr_bb_coord = &bb_coord_new;
        curr_bb_edge = &bb_edge_new;
    }

    /* Check if I can update the bounding box incrementally. */

    if (pin_new_loc.x < pin_old_loc.x) { /* Move to left. */

        /* Update the xmax fields for coordinates and number of edges first. */

        if (pin_old_loc.x == curr_bb_coord->xmax) { /* Old position at xmax. */
            if (curr_bb_edge->xmax == 1) {
                get_bb_from_scratch(net_id, bb_coord_new, bb_edge_new, num_sink_pin_layer_new);
                bb_updated_before[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
                return;
            } else {
                bb_edge_new.xmax = curr_bb_edge->xmax - 1;
                bb_coord_new.xmax = curr_bb_coord->xmax;
            }
        } else { /* Move to left, old position was not at xmax. */
            bb_coord_new.xmax = curr_bb_coord->xmax;
            bb_edge_new.xmax = curr_bb_edge->xmax;
        }

        /* Now do the xmin fields for coordinates and number of edges. */

        if (pin_new_loc.x < curr_bb_coord->xmin) { /* Moved past xmin */
            bb_coord_new.xmin = pin_new_loc.x;
            bb_edge_new.xmin = 1;
        } else if (pin_new_loc.x == curr_bb_coord->xmin) { /* Moved to xmin */
            bb_coord_new.xmin = pin_new_loc.x;
            bb_edge_new.xmin = curr_bb_edge->xmin + 1;
        } else { /* Xmin unchanged. */
            bb_coord_new.xmin = curr_bb_coord->xmin;
            bb_edge_new.xmin = curr_bb_edge->xmin;
        }
        /* End of move to left case. */

    } else if (pin_new_loc.x > pin_old_loc.x) { /* Move to right. */

        /* Update the xmin fields for coordinates and number of edges first. */

        if (pin_old_loc.x == curr_bb_coord->xmin) { /* Old position at xmin. */
            if (curr_bb_edge->xmin == 1) {
                get_bb_from_scratch(net_id, bb_coord_new, bb_edge_new, num_sink_pin_layer_new);
                bb_updated_before[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
                return;
            } else {
                bb_edge_new.xmin = curr_bb_edge->xmin - 1;
                bb_coord_new.xmin = curr_bb_coord->xmin;
            }
        } else { /* Move to right, old position was not at xmin. */
            bb_coord_new.xmin = curr_bb_coord->xmin;
            bb_edge_new.xmin = curr_bb_edge->xmin;
        }

        /* Now do the xmax fields for coordinates and number of edges. */

        if (pin_new_loc.x > curr_bb_coord->xmax) { /* Moved past xmax. */
            bb_coord_new.xmax = pin_new_loc.x;
            bb_edge_new.xmax = 1;
        } else if (pin_new_loc.x == curr_bb_coord->xmax) { /* Moved to xmax */
            bb_coord_new.xmax = pin_new_loc.x;
            bb_edge_new.xmax = curr_bb_edge->xmax + 1;
        } else { /* Xmax unchanged. */
            bb_coord_new.xmax = curr_bb_coord->xmax;
            bb_edge_new.xmax = curr_bb_edge->xmax;
        }
        /* End of move to right case. */

    } else { /* pin_new_loc.x == pin_old_loc.x -- no x motion. */
        bb_coord_new.xmin = curr_bb_coord->xmin;
        bb_coord_new.xmax = curr_bb_coord->xmax;
        bb_edge_new.xmin = curr_bb_edge->xmin;
        bb_edge_new.xmax = curr_bb_edge->xmax;
    }

    /* Now account for the y-direction motion. */

    if (pin_new_loc.y < pin_old_loc.y) { /* Move down. */

        /* Update the ymax fields for coordinates and number of edges first. */

        if (pin_old_loc.y == curr_bb_coord->ymax) { /* Old position at ymax. */
            if (curr_bb_edge->ymax == 1) {
                get_bb_from_scratch(net_id, bb_coord_new, bb_edge_new, num_sink_pin_layer_new);
                bb_updated_before[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
                return;
            } else {
                bb_edge_new.ymax = curr_bb_edge->ymax - 1;
                bb_coord_new.ymax = curr_bb_coord->ymax;
            }
        } else { /* Move down, old postion was not at ymax. */
            bb_coord_new.ymax = curr_bb_coord->ymax;
            bb_edge_new.ymax = curr_bb_edge->ymax;
        }

        /* Now do the ymin fields for coordinates and number of edges. */

        if (pin_new_loc.y < curr_bb_coord->ymin) { /* Moved past ymin */
            bb_coord_new.ymin = pin_new_loc.y;
            bb_edge_new.ymin = 1;
        } else if (pin_new_loc.y == curr_bb_coord->ymin) { /* Moved to ymin */
            bb_coord_new.ymin = pin_new_loc.y;
            bb_edge_new.ymin = curr_bb_edge->ymin + 1;
        } else { /* ymin unchanged. */
            bb_coord_new.ymin = curr_bb_coord->ymin;
            bb_edge_new.ymin = curr_bb_edge->ymin;
        }
        /* End of move down case. */

    } else if (pin_new_loc.y > pin_old_loc.y) { /* Moved up. */

        /* Update the ymin fields for coordinates and number of edges first. */

        if (pin_old_loc.y == curr_bb_coord->ymin) { /* Old position at ymin. */
            if (curr_bb_edge->ymin == 1) {
                get_bb_from_scratch(net_id, bb_coord_new, bb_edge_new, num_sink_pin_layer_new);
                bb_updated_before[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
                return;
            } else {
                bb_edge_new.ymin = curr_bb_edge->ymin - 1;
                bb_coord_new.ymin = curr_bb_coord->ymin;
            }
        } else { /* Moved up, old position was not at ymin. */
            bb_coord_new.ymin = curr_bb_coord->ymin;
            bb_edge_new.ymin = curr_bb_edge->ymin;
        }

        /* Now do the ymax fields for coordinates and number of edges. */

        if (pin_new_loc.y > curr_bb_coord->ymax) { /* Moved past ymax. */
            bb_coord_new.ymax = pin_new_loc.y;
            bb_edge_new.ymax = 1;
        } else if (pin_new_loc.y == curr_bb_coord->ymax) { /* Moved to ymax */
            bb_coord_new.ymax = pin_new_loc.y;
            bb_edge_new.ymax = curr_bb_edge->ymax + 1;
        } else { /* ymax unchanged. */
            bb_coord_new.ymax = curr_bb_coord->ymax;
            bb_edge_new.ymax = curr_bb_edge->ymax;
        }
        /* End of move up case. */

    } else { /* pin_new_loc.y == yold -- no y motion. */
        bb_coord_new.ymin = curr_bb_coord->ymin;
        bb_coord_new.ymax = curr_bb_coord->ymax;
        bb_edge_new.ymin = curr_bb_edge->ymin;
        bb_edge_new.ymax = curr_bb_edge->ymax;
    }

    /* Now account for the layer motion. */
    if (num_layers > 1) {
        /* We need to update it only if multiple layers are available */
        for (int layer_num = 0; layer_num < num_layers; layer_num++) {
            num_sink_pin_layer_new[layer_num] = curr_num_sink_pin_layer[layer_num];
        }
        if (!src_pin) {
            /* if src pin is being moved, we don't need to update this data structure */
            if (pin_old_loc.layer_num != pin_new_loc.layer_num) {
                num_sink_pin_layer_new[pin_old_loc.layer_num] = (curr_num_sink_pin_layer)[pin_old_loc.layer_num] - 1;
                num_sink_pin_layer_new[pin_new_loc.layer_num] = (curr_num_sink_pin_layer)[pin_new_loc.layer_num] + 1;
            }
        }

        if (pin_new_loc.layer_num < pin_old_loc.layer_num) {
            if (pin_old_loc.layer_num == curr_bb_coord->layer_max) {
                if (curr_bb_edge->layer_max == 1) {
                    get_bb_from_scratch(net_id, bb_coord_new, bb_edge_new, num_sink_pin_layer_new);
                    bb_updated_before[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
                    return;
                } else {
                    bb_edge_new.layer_max = curr_bb_edge->layer_max - 1;
                    bb_coord_new.layer_max = curr_bb_coord->layer_max;
                }
            } else {
                bb_coord_new.layer_max = curr_bb_coord->layer_max;
                bb_edge_new.layer_max = curr_bb_edge->layer_max;
            }


            if (pin_new_loc.layer_num < curr_bb_coord->layer_min) {
                bb_coord_new.layer_min = pin_new_loc.layer_num;
                bb_edge_new.layer_min = 1;
            } else if (pin_new_loc.layer_num == curr_bb_coord->layer_min) {
                bb_coord_new.layer_min = pin_new_loc.layer_num;
                bb_edge_new.layer_min = curr_bb_edge->layer_min + 1;
            } else {
                bb_coord_new.layer_min = curr_bb_coord->layer_min;
                bb_edge_new.layer_min = curr_bb_edge->layer_min;
            }

        } else if (pin_new_loc.layer_num > pin_old_loc.layer_num) {


            if (pin_old_loc.layer_num == curr_bb_coord->layer_min) {
                if (curr_bb_edge->layer_min == 1) {
                    get_bb_from_scratch(net_id, bb_coord_new, bb_edge_new, num_sink_pin_layer_new);
                    bb_updated_before[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
                    return;
                } else {
                    bb_edge_new.layer_min = curr_bb_edge->layer_min - 1;
                    bb_coord_new.layer_min = curr_bb_coord->layer_min;
                }
            } else {
                bb_coord_new.layer_min = curr_bb_coord->layer_min;
                bb_edge_new.layer_min = curr_bb_edge->layer_min;
            }

            if (pin_new_loc.layer_num > curr_bb_coord->layer_max) {
                bb_coord_new.layer_max = pin_new_loc.layer_num;
                bb_edge_new.layer_max = 1;
            } else if (pin_new_loc.layer_num == curr_bb_coord->layer_max) {
                bb_coord_new.layer_max = pin_new_loc.layer_num;
                bb_edge_new.layer_max = curr_bb_edge->layer_max + 1;
            } else {
                bb_coord_new.layer_max = curr_bb_coord->layer_max;
                bb_edge_new.layer_max = curr_bb_edge->layer_max;
            }


        } else {//pin_new_loc.layer_num == pin_old_loc.layer_num
            bb_coord_new.layer_min = curr_bb_coord->layer_min;
            bb_coord_new.layer_max = curr_bb_coord->layer_max;
            bb_edge_new.layer_min = curr_bb_edge->layer_min;
            bb_edge_new.layer_max = curr_bb_edge->layer_max;
        }

    } else {// num_layers == 1
        bb_coord_new.layer_min = curr_bb_coord->layer_min;
        bb_coord_new.layer_max = curr_bb_coord->layer_max;
        bb_edge_new.layer_min = curr_bb_edge->layer_min;
        bb_edge_new.layer_max = curr_bb_edge->layer_max;
    }

    if (bb_updated_before[net_id] == NetUpdateState::NOT_UPDATED_YET) {
        bb_updated_before[net_id] = NetUpdateState::UPDATED_ONCE;
    }
}

static void update_layer_bb(ClusterNetId net_id,
                            std::vector<t_2D_bb>& bb_edge_new,
                            std::vector<t_2D_bb>& bb_coord_new,
                            vtr::NdMatrixProxy<int, 1> bb_pin_sink_count_new,
                            t_physical_tile_loc pin_old_loc,
                            t_physical_tile_loc pin_new_loc,
                            bool is_output_pin) {

    auto& device_ctx = g_vpr_ctx.device();
    auto& place_move_ctx = g_placer_ctx.move();

    pin_new_loc.x = max(min<int>(pin_new_loc.x, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    pin_new_loc.y = max(min<int>(pin_new_loc.y, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
    pin_old_loc.x = max(min<int>(pin_old_loc.x, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    pin_old_loc.y = max(min<int>(pin_old_loc.y, device_ctx.grid.height() - 2), 1); //-2 for no perim channels

    /* Check if the net had been updated before. */
    if (bb_updated_before[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
        /* The net had been updated from scratch, DO NOT update again! */
        return;
    }

    const vtr::NdMatrixProxy<int, 1> curr_layer_pin_sink_count = (bb_updated_before[net_id] == NetUpdateState::NOT_UPDATED_YET) ? 
    place_move_ctx.num_sink_pin_layer[size_t(net_id)] : bb_pin_sink_count_new;

    const std::vector<t_2D_bb>*curr_bb_edge, *curr_bb_coord;
    if (bb_updated_before[net_id] == NetUpdateState::NOT_UPDATED_YET) {
        /* The net had NOT been updated before, could use the old values */
        curr_bb_edge = &place_move_ctx.layer_bb_num_on_edges[net_id];
        curr_bb_coord = &place_move_ctx.layer_bb_coords[net_id];
        bb_updated_before[net_id] = NetUpdateState::UPDATED_ONCE;
    } else {
        /* The net had been updated before, must use the new values */
        curr_bb_edge = &bb_edge_new;
        curr_bb_coord = &bb_coord_new;
    }

    /* Check if I can update the bounding box incrementally. */

    update_bb_pin_sink_count(pin_old_loc,
                             pin_new_loc,
                             curr_layer_pin_sink_count,
                             bb_pin_sink_count_new,
                             is_output_pin);

    int layer_old = pin_old_loc.layer_num;
    int layer_new = pin_new_loc.layer_num;
    bool layer_changed = (layer_old != layer_new);

    bb_edge_new = *curr_bb_edge;
    bb_coord_new = *curr_bb_coord;

    if (layer_changed) {
        update_bb_layer_changed(net_id,
                                pin_old_loc,
                                pin_new_loc,
                                *curr_bb_edge,
                                *curr_bb_coord,
                                bb_pin_sink_count_new,
                                bb_edge_new,
                                bb_coord_new);
    } else {
        update_bb_same_layer(net_id,
                             pin_old_loc,
                             pin_new_loc,
                             *curr_bb_edge,
                             *curr_bb_coord,
                             bb_pin_sink_count_new,
                             bb_edge_new,
                             bb_coord_new);
    }

    if (bb_updated_before[net_id] == NetUpdateState::NOT_UPDATED_YET) {
        bb_updated_before[net_id] = NetUpdateState::UPDATED_ONCE;
    }
}

static inline void update_bb_same_layer(ClusterNetId net_id,
                                        const t_physical_tile_loc& pin_old_loc,
                                        const t_physical_tile_loc& pin_new_loc,
                                        const std::vector<t_2D_bb>& curr_bb_edge,
                                        const std::vector<t_2D_bb>& curr_bb_coord,
                                        vtr::NdMatrixProxy<int, 1> bb_pin_sink_count_new,
                                        std::vector<t_2D_bb>& bb_edge_new,
                                        std::vector<t_2D_bb>& bb_coord_new) {
    int x_old = pin_old_loc.x;
    int x_new = pin_new_loc.x;

    int y_old = pin_old_loc.y;
    int y_new = pin_new_loc.y;

    int layer_num = pin_old_loc.layer_num;
    VTR_ASSERT_SAFE(layer_num == pin_new_loc.layer_num);

    if (x_new < x_old) {
        if (x_old == curr_bb_coord[layer_num].xmax) {
            update_bb_edge(net_id,
                           bb_edge_new,
                           bb_coord_new,
                           bb_pin_sink_count_new,
                           curr_bb_edge[layer_num].xmax,
                           curr_bb_coord[layer_num].xmax,
                           bb_edge_new[layer_num].xmax,
                           bb_coord_new[layer_num].xmax);
            if (bb_updated_before[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
                return;
            }
        }

        if (x_new < curr_bb_coord[layer_num].xmin) {
            bb_edge_new[layer_num].xmin = 1;
            bb_coord_new[layer_num].xmin = x_new;
        } else if (x_new == curr_bb_coord[layer_num].xmin) {
            bb_edge_new[layer_num].xmin = curr_bb_edge[layer_num].xmin + 1;
            bb_coord_new[layer_num].xmin = curr_bb_coord[layer_num].xmin;
        }

    } else if (x_new > x_old) {
        if (x_old == curr_bb_coord[layer_num].xmin) {
            update_bb_edge(net_id,
                           bb_edge_new,
                           bb_coord_new,
                           bb_pin_sink_count_new,
                           curr_bb_edge[layer_num].xmin,
                           curr_bb_coord[layer_num].xmin,
                           bb_edge_new[layer_num].xmin,
                           bb_coord_new[layer_num].xmin);
            if (bb_updated_before[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
                return;
            }
        }

        if (x_new > curr_bb_coord[layer_num].xmax) {
            bb_edge_new[layer_num].xmax = 1;
            bb_coord_new[layer_num].xmax = x_new;
        } else if (x_new == curr_bb_coord[layer_num].xmax) {
            bb_edge_new[layer_num].xmax = curr_bb_edge[layer_num].xmax + 1;
            bb_coord_new[layer_num].xmax = curr_bb_coord[layer_num].xmax;
        }
    }

    if (y_new < y_old) {
        if (y_old == curr_bb_coord[layer_num].ymax) {
            update_bb_edge(net_id,
                           bb_edge_new,
                           bb_coord_new,
                           bb_pin_sink_count_new,
                           curr_bb_edge[layer_num].ymax,
                           curr_bb_coord[layer_num].ymax,
                           bb_edge_new[layer_num].ymax,
                           bb_coord_new[layer_num].ymax);
            if (bb_updated_before[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
                return;
            }
        }

        if (y_new < curr_bb_coord[layer_num].ymin) {
            bb_edge_new[layer_num].ymin = 1;
            bb_coord_new[layer_num].ymin = y_new;
        } else if (y_new == curr_bb_coord[layer_num].ymin) {
            bb_edge_new[layer_num].ymin = curr_bb_edge[layer_num].ymin + 1;
            bb_coord_new[layer_num].ymin = curr_bb_coord[layer_num].ymin;
        }

    } else if (y_new > y_old) {
        if (y_old == curr_bb_coord[layer_num].ymin) {
            update_bb_edge(net_id,
                           bb_edge_new,
                           bb_coord_new,
                           bb_pin_sink_count_new,
                           curr_bb_edge[layer_num].ymin,
                           curr_bb_coord[layer_num].ymin,
                           bb_edge_new[layer_num].ymin,
                           bb_coord_new[layer_num].ymin);
            if (bb_updated_before[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
                return;
            }
        }

        if (y_new > curr_bb_coord[layer_num].ymax) {
            bb_edge_new[layer_num].ymax = 1;
            bb_coord_new[layer_num].ymax = y_new;
        } else if (y_new == curr_bb_coord[layer_num].ymax) {
            bb_edge_new[layer_num].ymax = curr_bb_edge[layer_num].ymax + 1;
            bb_coord_new[layer_num].ymax = curr_bb_coord[layer_num].ymax;
        }
    }
}

static inline void update_bb_layer_changed(ClusterNetId net_id,
                                           const t_physical_tile_loc& pin_old_loc,
                                           const t_physical_tile_loc& pin_new_loc,
                                           const std::vector<t_2D_bb>& curr_bb_edge,
                                           const std::vector<t_2D_bb>& curr_bb_coord,
                                           vtr::NdMatrixProxy<int, 1> bb_pin_sink_count_new,
                                           std::vector<t_2D_bb>& bb_edge_new,
                                           std::vector<t_2D_bb>& bb_coord_new) {
    int x_old = pin_old_loc.x;

    int y_old = pin_old_loc.y;

    int old_layer_num = pin_old_loc.layer_num;
    int new_layer_num = pin_new_loc.layer_num;
    VTR_ASSERT_SAFE(old_layer_num != new_layer_num);

    /*
    This funcitn is called when BB per layer is used and when the moving block is moving from one layer to another.
    Thus, we need to update bounding box on both "from" and "to" layer. Here, we update the bounding box on "from" or
    "old_layer". Then, "add_block_to_bb" is called to update the bounding box on the new layer.
    */
    if (x_old == curr_bb_coord[old_layer_num].xmax) {
        update_bb_edge(net_id,
                       bb_edge_new,
                       bb_coord_new,
                       bb_pin_sink_count_new,
                       curr_bb_edge[old_layer_num].xmax,
                       curr_bb_coord[old_layer_num].xmax,
                       bb_edge_new[old_layer_num].xmax,
                       bb_coord_new[old_layer_num].xmax);
        if (bb_updated_before[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
            return;
        }
    } else if (x_old == curr_bb_coord[old_layer_num].xmin) {
        update_bb_edge(net_id,
                       bb_edge_new,
                       bb_coord_new,
                       bb_pin_sink_count_new,
                       curr_bb_edge[old_layer_num].xmin,
                       curr_bb_coord[old_layer_num].xmin,
                       bb_edge_new[old_layer_num].xmin,
                       bb_coord_new[old_layer_num].xmin);
        if (bb_updated_before[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
            return;
        }
    }

    if (y_old == curr_bb_coord[old_layer_num].ymax) {
        update_bb_edge(net_id,
                       bb_edge_new,
                       bb_coord_new,
                       bb_pin_sink_count_new,
                       curr_bb_edge[old_layer_num].ymax,
                       curr_bb_coord[old_layer_num].ymax,
                       bb_edge_new[old_layer_num].ymax,
                       bb_coord_new[old_layer_num].ymax);
        if (bb_updated_before[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
            return;
        }
    } else if (y_old == curr_bb_coord[old_layer_num].ymin) {
        update_bb_edge(net_id,
                       bb_edge_new,
                       bb_coord_new,
                       bb_pin_sink_count_new,
                       curr_bb_edge[old_layer_num].ymin,
                       curr_bb_coord[old_layer_num].ymin,
                       bb_edge_new[old_layer_num].ymin,
                       bb_coord_new[old_layer_num].ymin);
        if (bb_updated_before[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
            return;
        }
    }

    add_block_to_bb(pin_new_loc,
                    curr_bb_edge[new_layer_num],
                    curr_bb_coord[new_layer_num],
                    bb_edge_new[new_layer_num],
                    bb_coord_new[new_layer_num]);
}

static void update_bb_pin_sink_count(const t_physical_tile_loc& pin_old_loc,
                                     const t_physical_tile_loc& pin_new_loc,
                                     const vtr::NdMatrixProxy<int, 1> curr_layer_pin_sink_count,
                                     vtr::NdMatrixProxy<int, 1> bb_pin_sink_count_new,
                                     bool is_output_pin) {
    VTR_ASSERT_SAFE(curr_layer_pin_sink_count[pin_old_loc.layer_num] > 0 || is_output_pin);
    for (int layer_num = 0; layer_num < g_vpr_ctx.device().grid.get_num_layers(); layer_num++) {
        bb_pin_sink_count_new[layer_num] = curr_layer_pin_sink_count[layer_num];
    }
    if (!is_output_pin) {
        bb_pin_sink_count_new[pin_old_loc.layer_num] -= 1;
        bb_pin_sink_count_new[pin_new_loc.layer_num] += 1;
    }
}

static inline void update_bb_edge(ClusterNetId net_id,
                                  std::vector<t_2D_bb>& bb_edge_new,
                                  std::vector<t_2D_bb>& bb_coord_new,
                                  vtr::NdMatrixProxy<int, 1> bb_layer_pin_sink_count,
                                  const int& old_num_block_on_edge,
                                  const int& old_edge_coord,
                                  int& new_num_block_on_edge,
                                  int& new_edge_coord) {
    if (old_num_block_on_edge == 1) {
        get_layer_bb_from_scratch(net_id,
                                  bb_edge_new,
                                  bb_coord_new,
                                  bb_layer_pin_sink_count);
        bb_updated_before[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
        return;
    } else {
        new_num_block_on_edge = old_num_block_on_edge - 1;
        new_edge_coord = old_edge_coord;
    }
}

static void add_block_to_bb(const t_physical_tile_loc& new_pin_loc,
                            const t_2D_bb& bb_edge_old,
                            const t_2D_bb& bb_coord_old,
                            t_2D_bb& bb_edge_new,
                            t_2D_bb& bb_coord_new) {
    int x_new = new_pin_loc.x;
    int y_new = new_pin_loc.y;

    /* 
    This function is called to only update the bounding box on the new layer from a block
    moving to this layer from another layer. Thus, we only need to assess the effect of this
    new block on the edges.
    */

    if (x_new > bb_coord_old.xmax) {
        bb_edge_new.xmax = 1;
        bb_coord_new.xmax = x_new;
    } else if (x_new == bb_coord_old.xmax) {
        bb_edge_new.xmax = bb_edge_old.xmax + 1;
    }

    if (x_new < bb_coord_old.xmin) {
        bb_edge_new.xmin = 1;
        bb_coord_new.xmin = x_new;
    } else if (x_new == bb_coord_old.xmin) {
        bb_edge_new.xmin = bb_edge_old.xmin + 1;
    }

    if (y_new > bb_coord_old.ymax) {
        bb_edge_new.ymax = 1;
        bb_coord_new.ymax = y_new;
    } else if (y_new == bb_coord_old.ymax) {
        bb_edge_new.ymax = bb_edge_old.ymax + 1;
    }

    if (y_new < bb_coord_old.ymin) {
        bb_edge_new.ymin = 1;
        bb_coord_new.ymin = y_new;
    } else if (y_new == bb_coord_old.ymin) {
        bb_edge_new.ymin = bb_edge_old.ymin + 1;
    }
}

static void get_bb_from_scratch(ClusterNetId net_id,
                                t_bb& coords,
                                t_bb& num_on_edges,
                                vtr::NdMatrixProxy<int, 1> num_sink_pin_layer) {
    int pnum, x, y, pin_layer, xmin, xmax, ymin, ymax, layer_min, layer_max;
    int xmin_edge, xmax_edge, ymin_edge, ymax_edge, layer_min_edge, layer_max_edge;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    ClusterBlockId bnum = cluster_ctx.clb_nlist.net_driver_block(net_id);
    pnum = net_pin_to_tile_pin_index(net_id, 0);
    VTR_ASSERT_SAFE(pnum >= 0);
    x = place_ctx.block_locs[bnum].loc.x
        + physical_tile_type(bnum)->pin_width_offset[pnum];
    y = place_ctx.block_locs[bnum].loc.y
        + physical_tile_type(bnum)->pin_height_offset[pnum];
    pin_layer = place_ctx.block_locs[bnum].loc.layer;

    x = max(min<int>(x, grid.width() - 2), 1);
    y = max(min<int>(y, grid.height() - 2), 1);
    pin_layer = max(min<int>(pin_layer, grid.get_num_layers() - 1), 0);

    xmin = x;
    ymin = y;
    layer_min = pin_layer;
    xmax = x;
    ymax = y;
    layer_max = pin_layer;

    xmin_edge = 1;
    ymin_edge = 1;
    layer_min_edge = 1;
    xmax_edge = 1;
    ymax_edge = 1;
    layer_max_edge = 1;

    for (int layer_num = 0; layer_num < grid.get_num_layers(); layer_num++) {
        num_sink_pin_layer[layer_num] = 0;
    }

    for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
        bnum = cluster_ctx.clb_nlist.pin_block(pin_id);
        pnum = tile_pin_index(pin_id);
        x = place_ctx.block_locs[bnum].loc.x
            + physical_tile_type(bnum)->pin_width_offset[pnum];
        y = place_ctx.block_locs[bnum].loc.y
            + physical_tile_type(bnum)->pin_height_offset[pnum];
        pin_layer = place_ctx.block_locs[bnum].loc.layer;

        /* Code below counts IO blocks as being within the 1..grid.width()-2, 1..grid.height()-2 clb array. *
         * This is because channels do not go out of the 0..grid.width()-2, 0..grid.height()-2 range, and   *
         * I always take all channels impinging on the bounding box to be within   *
         * that bounding box.  Hence, this "movement" of IO blocks does not affect *
         * the which channels are included within the bounding box, and it         *
         * simplifies the code a lot.                                              */

        x = max(min<int>(x, grid.width() - 2), 1);  //-2 for no perim channels
        y = max(min<int>(y, grid.height() - 2), 1); //-2 for no perim channels
        pin_layer = max(min<int>(pin_layer, grid.get_num_layers() - 1), 0);

        if (x == xmin) {
            xmin_edge++;
        }
        if (x == xmax) { /* Recall that xmin could equal xmax -- don't use else */
            xmax_edge++;
        } else if (x < xmin) {
            xmin = x;
            xmin_edge = 1;
        } else if (x > xmax) {
            xmax = x;
            xmax_edge = 1;
        }

        if (y == ymin) {
            ymin_edge++;
        }
        if (y == ymax) {
            ymax_edge++;
        } else if (y < ymin) {
            ymin = y;
            ymin_edge = 1;
        } else if (y > ymax) {
            ymax = y;
            ymax_edge = 1;
        }

        if (pin_layer == layer_min) {
            layer_min_edge++;
        }
        if (pin_layer == layer_max) {
            layer_max_edge++;
        } else if (pin_layer < layer_min) {
            layer_min = pin_layer;
            layer_min_edge = 1;
        } else if (pin_layer > layer_max) {
            layer_max = pin_layer;
            layer_max_edge = 1;
        }

        num_sink_pin_layer[pin_layer]++;
    }

    /* Copy the coordinates and number on edges information into the proper   *
     * structures.                                                            */
    coords.xmin = xmin;
    coords.xmax = xmax;
    coords.ymin = ymin;
    coords.ymax = ymax;
    coords.layer_min = layer_min;
    coords.layer_max = layer_max;
    VTR_ASSERT_DEBUG(layer_min >= 0 && layer_min < device_ctx.grid.get_num_layers());
    VTR_ASSERT_DEBUG(layer_max >= 0 && layer_max < device_ctx.grid.get_num_layers());


    num_on_edges.xmin = xmin_edge;
    num_on_edges.xmax = xmax_edge;
    num_on_edges.ymin = ymin_edge;
    num_on_edges.ymax = ymax_edge;
    num_on_edges.layer_min = layer_min_edge;
    num_on_edges.layer_max = layer_max_edge;
}

static void get_layer_bb_from_scratch(ClusterNetId net_id,
                                      std::vector<t_2D_bb>& num_on_edges,
                                      std::vector<t_2D_bb>& coords,
                                      vtr::NdMatrixProxy<int, 1> layer_pin_sink_count) {
    auto& device_ctx = g_vpr_ctx.device();
    const int num_layers = device_ctx.grid.get_num_layers();
    std::vector<int> xmin(num_layers, OPEN);
    std::vector<int> xmax(num_layers, OPEN);
    std::vector<int> ymin(num_layers, OPEN);
    std::vector<int> ymax(num_layers, OPEN);
    std::vector<int> xmin_edge(num_layers, OPEN);
    std::vector<int> xmax_edge(num_layers, OPEN);
    std::vector<int> ymin_edge(num_layers, OPEN);
    std::vector<int> ymax_edge(num_layers, OPEN);

    std::vector<int> num_sink_pin_layer(num_layers, 0);

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& grid = device_ctx.grid;

    ClusterBlockId bnum = cluster_ctx.clb_nlist.net_driver_block(net_id);
    int pnum_src = net_pin_to_tile_pin_index(net_id, 0);
    VTR_ASSERT_SAFE(pnum_src >= 0);
    int x_src = place_ctx.block_locs[bnum].loc.x
                + physical_tile_type(bnum)->pin_width_offset[pnum_src];
    int y_src = place_ctx.block_locs[bnum].loc.y
                + physical_tile_type(bnum)->pin_height_offset[pnum_src];

    x_src = max(min<int>(x_src, grid.width() - 2), 1);
    y_src = max(min<int>(y_src, grid.height() - 2), 1);

    // TODO: Currently we are assuming that crossing can only happen from OPIN. Because of that,
    // when per-layer bounding box is used, we want the bounding box on each layer to also include
    // the location of source since the connection on each layer starts from that location.
    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        xmin[layer_num] = x_src;
        ymin[layer_num] = y_src;
        xmax[layer_num] = x_src;
        ymax[layer_num] = y_src;
        xmin_edge[layer_num] = 1;
        ymin_edge[layer_num] = 1;
        xmax_edge[layer_num] = 1;
        ymax_edge[layer_num] = 1;
    }

    for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
        bnum = cluster_ctx.clb_nlist.pin_block(pin_id);
        int pnum = tile_pin_index(pin_id);
        int layer = place_ctx.block_locs[bnum].loc.layer;
        VTR_ASSERT_SAFE(layer >= 0 && layer < num_layers);
        num_sink_pin_layer[layer]++;
        int x = place_ctx.block_locs[bnum].loc.x
                + physical_tile_type(bnum)->pin_width_offset[pnum];
        int y = place_ctx.block_locs[bnum].loc.y
                + physical_tile_type(bnum)->pin_height_offset[pnum];

        /* Code below counts IO blocks as being within the 1..grid.width()-2, 1..grid.height()-2 clb array. *
         * This is because channels do not go out of the 0..grid.width()-2, 0..grid.height()-2 range, and   *
         * I always take all channels impinging on the bounding box to be within   *
         * that bounding box.  Hence, this "movement" of IO blocks does not affect *
         * the which channels are included within the bounding box, and it         *
         * simplifies the code a lot.                                              */

        x = max(min<int>(x, grid.width() - 2), 1);  //-2 for no perim channels
        y = max(min<int>(y, grid.height() - 2), 1); //-2 for no perim channels

        if (x == xmin[layer]) {
            xmin_edge[layer]++;
        }
        if (x == xmax[layer]) { /* Recall that xmin could equal xmax -- don't use else */
            xmax_edge[layer]++;
        } else if (x < xmin[layer]) {
            xmin[layer] = x;
            xmin_edge[layer] = 1;
        } else if (x > xmax[layer]) {
            xmax[layer] = x;
            xmax_edge[layer] = 1;
        }

        if (y == ymin[layer]) {
            ymin_edge[layer]++;
        }
        if (y == ymax[layer]) {
            ymax_edge[layer]++;
        } else if (y < ymin[layer]) {
            ymin[layer] = y;
            ymin_edge[layer] = 1;
        } else if (y > ymax[layer]) {
            ymax[layer] = y;
            ymax_edge[layer] = 1;
        }
    }

    /* Copy the coordinates and number on edges information into the proper   *
     * structures.                                                            */
    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        layer_pin_sink_count[layer_num] = num_sink_pin_layer[layer_num];
        coords[layer_num].xmin = xmin[layer_num];
        coords[layer_num].xmax = xmax[layer_num];
        coords[layer_num].ymin = ymin[layer_num];
        coords[layer_num].ymax = ymax[layer_num];
        coords[layer_num].layer_num = layer_num;

        num_on_edges[layer_num].xmin = xmin_edge[layer_num];
        num_on_edges[layer_num].xmax = xmax_edge[layer_num];
        num_on_edges[layer_num].ymin = ymin_edge[layer_num];
        num_on_edges[layer_num].ymax = ymax_edge[layer_num];
        num_on_edges[layer_num].layer_num = layer_num;
    }
}

static double get_net_cost(ClusterNetId net_id, const t_bb& bb) {
    /* Finds the cost due to one net by looking at its coordinate bounding  *
     * box.                                                                 */

    double ncost, crossing;
    auto& cluster_ctx = g_vpr_ctx.clustering();

    crossing = wirelength_crossing_count(
        cluster_ctx.clb_nlist.net_pins(net_id).size());

    /* Could insert a check for xmin == xmax.  In that case, assume  *
     * connection will be made with no bends and hence no x-cost.    *
     * Same thing for y-cost.                                        */

    /* Cost = wire length along channel * cross_count / average      *
     * channel capacity.   Do this for x, then y direction and add.  */

    ncost = (bb.xmax - bb.xmin + 1) * crossing
            * chanx_place_cost_fac[bb.ymax][bb.ymin - 1];

    ncost += (bb.ymax - bb.ymin + 1) * crossing
             * chany_place_cost_fac[bb.xmax][bb.xmin - 1];

    return (ncost);
}

static double get_net_layer_bb_wire_cost(ClusterNetId /* net_id */,
                                 const std::vector<t_2D_bb>& bb,
                                 const vtr::NdMatrixProxy<int, 1> layer_pin_sink_count) {
    /* Finds the cost due to one net by looking at its coordinate bounding  *
     * box.                                                                 */

    double ncost = 0.;
    double crossing = 0.;
    int num_layers = g_vpr_ctx.device().grid.get_num_layers();

    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        VTR_ASSERT(layer_pin_sink_count[layer_num] != OPEN);
        if (layer_pin_sink_count[layer_num] == 0) {
            continue;
        }
        /* 
        adjust the bounding box half perimeter by the wirelength correction 
        factor based on terminal count, which is 1 for the source + the number 
        of sinks on this layer. 
        */
        crossing = wirelength_crossing_count(layer_pin_sink_count[layer_num] + 1);

        /* Could insert a check for xmin == xmax.  In that case, assume  *
         * connection will be made with no bends and hence no x-cost.    *
         * Same thing for y-cost.                                        */

        /* Cost = wire length along channel * cross_count / average      *
         * channel capacity.   Do this for x, then y direction and add.  */

        ncost += (bb[layer_num].xmax - bb[layer_num].xmin + 1) * crossing
                 * chanx_place_cost_fac[bb[layer_num].ymax][bb[layer_num].ymin - 1];

        ncost += (bb[layer_num].ymax - bb[layer_num].ymin + 1) * crossing
                 * chany_place_cost_fac[bb[layer_num].xmax][bb[layer_num].xmin - 1];
    }

    return (ncost);
}

static double get_net_wirelength_estimate(ClusterNetId net_id, const t_bb& bb) {
    double ncost, crossing;
    auto& cluster_ctx = g_vpr_ctx.clustering();

    crossing = wirelength_crossing_count(
        cluster_ctx.clb_nlist.net_pins(net_id).size());

    /* Could insert a check for xmin == xmax.  In that case, assume  *
     * connection will be made with no bends and hence no x-cost.    *
     * Same thing for y-cost.                                        */

    /* Cost = wire length along channel * cross_count / average      *
     * channel capacity.   Do this for x, then y direction and add.  */

    ncost = (bb.xmax - bb.xmin + 1) * crossing;

    ncost += (bb.ymax - bb.ymin + 1) * crossing;

    return (ncost);
}

static double get_net_wirelength_from_layer_bb(ClusterNetId /* net_id */,
                                                const std::vector<t_2D_bb>& bb,
                                                const vtr::NdMatrixProxy<int, 1> layer_pin_sink_count) {
    /* WMF: Finds the estimate of wirelength due to one net by looking at   *
     * its coordinate bounding box.                                         */

    double ncost = 0.;
    double crossing = 0.;
    int num_layers = g_vpr_ctx.device().grid.get_num_layers();

    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        VTR_ASSERT_SAFE (layer_pin_sink_count[layer_num] != OPEN);
        if (layer_pin_sink_count[layer_num] == 0) {
            continue;
        }
        crossing = wirelength_crossing_count(layer_pin_sink_count[layer_num] + 1);

        /* Could insert a check for xmin == xmax.  In that case, assume  *
         * connection will be made with no bends and hence no x-cost.    *
         * Same thing for y-cost.                                        */

        /* Cost = wire length along channel * cross_count / average      *
         * channel capacity.   Do this for x, then y direction and add.  */

        ncost += (bb[layer_num].xmax - bb[layer_num].xmin + 1) * crossing;

        ncost += (bb[layer_num].ymax - bb[layer_num].ymin + 1) * crossing;
    }

    return (ncost);
}

static double recompute_bb_cost() {
    double cost = 0;

    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {       /* for each net ... */
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) { /* Do only if not ignored. */
            /* Bounding boxes don't have to be recomputed; they're correct. */
            cost += net_cost[net_id];
        }
    }

    return (cost);
}

static double wirelength_crossing_count(size_t fanout) {
    /* Get the expected "crossing count" of a net, based on its number *
     * of pins.  Extrapolate for very large nets.                      */

    if (fanout > MAX_FANOUT_CROSSING_COUNT) {
        return 2.7933 + 0.02616 * (fanout - MAX_FANOUT_CROSSING_COUNT);
    } else {
        return cross_count[fanout - 1];
    }
}

static void set_bb_delta_cost(const int num_affected_nets, double& bb_delta_c) {
    for (int inet_affected = 0; inet_affected < num_affected_nets;
         inet_affected++) {
        ClusterNetId net_id = ts_nets_to_update[inet_affected];
        const auto& cube_bb = g_vpr_ctx.placement().cube_bb;

        if (cube_bb) {
            proposed_net_cost[net_id] = get_net_cost(net_id,
                                                     ts_bb_coord_new[net_id]);
        } else {
            proposed_net_cost[net_id] = get_net_layer_bb_wire_cost(net_id,
                                                           layer_ts_bb_coord_new[net_id],
                                                           ts_layer_sink_pin_count[size_t(net_id)]);
        }

        bb_delta_c += proposed_net_cost[net_id] - net_cost[net_id];
    }
}

int find_affected_nets_and_update_costs(
    const t_place_algorithm& place_algorithm,
    const PlaceDelayModel* delay_model,
    const PlacerCriticalities* criticalities,
    t_pl_blocks_to_be_moved& blocks_affected,
    double& bb_delta_c,
    double& timing_delta_c) {
    VTR_ASSERT_SAFE(bb_delta_c == 0.);
    VTR_ASSERT_SAFE(timing_delta_c == 0.);
    auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    int num_affected_nets = 0;

    /* Go through all the blocks moved. */
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; iblk++) {
        const auto& moving_block_inf = blocks_affected.moved_blocks[iblk];
        auto& affected_pins = blocks_affected.affected_pins;
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        /* Go through all the pins in the moved block. */
        for (ClusterPinId blk_pin : clb_nlist.block_pins(blk)) {
            bool is_src_moving = false;
            if (clb_nlist.pin_type(blk_pin) == PinType::SINK) {
                ClusterNetId net_id = clb_nlist.pin_net(blk_pin);
                is_src_moving = driven_by_moved_block(net_id,
                                                      blocks_affected.num_moved_blocks,
                                                      blocks_affected.moved_blocks);
            }
            update_net_info_on_pin_move(place_algorithm,
                                        delay_model,
                                        criticalities,
                                        blk,
                                        blk_pin,
                                        moving_block_inf,
                                        affected_pins,
                                        timing_delta_c,
                                        num_affected_nets,
                                        is_src_moving);
        }
    }

    /* Now update the bounding box costs (since the net bounding     *
     * boxes are up-to-date). The cost is only updated once per net. */
    set_bb_delta_cost(num_affected_nets, bb_delta_c);

    return num_affected_nets;
}

double comp_bb_cost(e_cost_methods method) {
    double cost = 0;
    double expected_wirelength = 0.0;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_move_ctx = g_placer_ctx.mutable_move();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {       /* for each net ... */
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) { /* Do only if not ignored. */
            /* Small nets don't use incremental updating on their bounding boxes, *
             * so they can use a fast bounding box calculator.                    */
            if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= SMALL_NET
                && method == NORMAL) {
                get_bb_from_scratch(net_id,
                                    place_move_ctx.bb_coords[net_id],
                                    place_move_ctx.bb_num_on_edges[net_id],
                                    place_move_ctx.num_sink_pin_layer[size_t(net_id)]);
            } else {
                get_non_updatable_bb(net_id,
                                     place_move_ctx.bb_coords[net_id],
                                     place_move_ctx.num_sink_pin_layer[size_t(net_id)]);
            }

            net_cost[net_id] = get_net_cost(net_id, place_move_ctx.bb_coords[net_id]);
            cost += net_cost[net_id];
            if (method == CHECK)
                expected_wirelength += get_net_wirelength_estimate(net_id, place_move_ctx.bb_coords[net_id]);
        }
    }

    if (method == CHECK) {
        VTR_LOG("\n");
        VTR_LOG("BB estimate of min-dist (placement) wire length: %.0f\n",
                expected_wirelength);
    }
    return cost;
}

double comp_layer_bb_cost(e_cost_methods method) {
    double cost = 0;
    double expected_wirelength = 0.0;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_move_ctx = g_placer_ctx.mutable_move();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {       /* for each net ... */
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) { /* Do only if not ignored. */
            /* Small nets don't use incremental updating on their bounding boxes, *
             * so they can use a fast bounding box calculator.                    */
            if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= SMALL_NET
                && method == NORMAL) {
                get_layer_bb_from_scratch(net_id,
                                          place_move_ctx.layer_bb_num_on_edges[net_id],
                                          place_move_ctx.layer_bb_coords[net_id],
                                          place_move_ctx.num_sink_pin_layer[size_t(net_id)]);
            } else {
                get_non_updatable_layer_bb(net_id,
                                           place_move_ctx.layer_bb_coords[net_id],
                                           place_move_ctx.num_sink_pin_layer[size_t(net_id)]);
            }

            net_cost[net_id] = get_net_layer_bb_wire_cost(net_id,
                                                  place_move_ctx.layer_bb_coords[net_id],
                                                  place_move_ctx.num_sink_pin_layer[size_t(net_id)]);
            cost += net_cost[net_id];
            if (method == CHECK)
                expected_wirelength += get_net_wirelength_from_layer_bb(net_id,
                                                                         place_move_ctx.layer_bb_coords[net_id],
                                                                         place_move_ctx.num_sink_pin_layer[size_t(net_id)]);
        }
    }

    if (method == CHECK) {
        VTR_LOG("\n");
        VTR_LOG("BB estimate of min-dist (placement) wire length: %.0f\n",
                expected_wirelength);
    }
    return cost;
}

void update_move_nets(int num_nets_affected,
                      const bool cube_bb) {
    /* update net cost functions and reset flags. */
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_move_ctx = g_placer_ctx.mutable_move();

    for (int inet_affected = 0; inet_affected < num_nets_affected;
         inet_affected++) {
        ClusterNetId net_id = ts_nets_to_update[inet_affected];

        if (cube_bb) {
            place_move_ctx.bb_coords[net_id] = ts_bb_coord_new[net_id];
        } else {
            place_move_ctx.layer_bb_coords[net_id] = layer_ts_bb_coord_new[net_id];
        }

        for (int layer_num = 0; layer_num < g_vpr_ctx.device().grid.get_num_layers(); layer_num++) {
            place_move_ctx.num_sink_pin_layer[size_t(net_id)][layer_num] = ts_layer_sink_pin_count[size_t(net_id)][layer_num];
        }

        if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= SMALL_NET) {
            if (cube_bb) {
                place_move_ctx.bb_num_on_edges[net_id] = ts_bb_edge_new[net_id];
            } else {
                place_move_ctx.layer_bb_num_on_edges[net_id] = layer_ts_bb_edge_new[net_id];
            }
        }

        net_cost[net_id] = proposed_net_cost[net_id];

        /* negative proposed_net_cost value is acting as a flag to mean not computed yet. */
        proposed_net_cost[net_id] = -1;
        bb_updated_before[net_id] = NetUpdateState::NOT_UPDATED_YET;
    }
}

void reset_move_nets(int num_nets_affected) {
    /* Reset the net cost function flags first. */
    for (int inet_affected = 0; inet_affected < num_nets_affected;
         inet_affected++) {
        ClusterNetId net_id = ts_nets_to_update[inet_affected];
        proposed_net_cost[net_id] = -1;
        bb_updated_before[net_id] = NetUpdateState::NOT_UPDATED_YET;
    }
}

void recompute_costs_from_scratch(const t_placer_opts& placer_opts,
                                const t_noc_opts& noc_opts,
                                const PlaceDelayModel* delay_model,
                                const PlacerCriticalities* criticalities,
                                t_placer_costs* costs) {
    auto check_and_print_cost = [](double new_cost,
                                   double old_cost,
                                   const std::string& cost_name) {
        if (!vtr::isclose(new_cost, old_cost, ERROR_TOL, 0.)) {
            std::string msg = vtr::string_fmt(
                "in recompute_costs_from_scratch: new_%s = %g, old %s = %g, ERROR_TOL = %g\n",
                cost_name.c_str(), new_cost, cost_name.c_str(), old_cost, ERROR_TOL);
            VPR_ERROR(VPR_ERROR_PLACE, msg.c_str());
        }
    };

    double new_bb_cost = recompute_bb_cost();
    check_and_print_cost(new_bb_cost, costs->bb_cost, "bb_cost");
    costs->bb_cost = new_bb_cost;

    if (placer_opts.place_algorithm.is_timing_driven()) {
        double new_timing_cost = 0.;
        comp_td_costs(delay_model, *criticalities, &new_timing_cost);
        check_and_print_cost(new_timing_cost, costs->timing_cost, "timing_cost");
        costs->timing_cost = new_timing_cost;
    } else {
        VTR_ASSERT(placer_opts.place_algorithm == BOUNDING_BOX_PLACE);
        costs->cost = new_bb_cost * costs->bb_cost_norm;
    }

    if (noc_opts.noc) {
        NocCostTerms new_noc_cost;
        recompute_noc_costs(new_noc_cost);

        check_and_print_cost(new_noc_cost.aggregate_bandwidth,
                             costs->noc_cost_terms.aggregate_bandwidth,
                             "noc_aggregate_bandwidth");
        costs->noc_cost_terms.aggregate_bandwidth = new_noc_cost.aggregate_bandwidth;

        // only check if the recomputed cost and the current noc latency cost are within the error tolerance if the cost is above 1 picosecond.
        // Otherwise, there is no need to check (we expect the latency cost to be above the threshold of 1 picosecond)
        if (new_noc_cost.latency > MIN_EXPECTED_NOC_LATENCY_COST) {
            check_and_print_cost(new_noc_cost.latency,
                                 costs->noc_cost_terms.latency,
                                 "noc_latency_cost");
        }
        costs->noc_cost_terms.latency = new_noc_cost.latency;

        if (new_noc_cost.latency_overrun > MIN_EXPECTED_NOC_LATENCY_COST) {
            check_and_print_cost(new_noc_cost.latency_overrun,
                                 costs->noc_cost_terms.latency_overrun,
                                 "noc_latency_overrun_cost");
        }
        costs->noc_cost_terms.latency_overrun = new_noc_cost.latency_overrun;

        if (new_noc_cost.congestion > MIN_EXPECTED_NOC_CONGESTION_COST) {
            check_and_print_cost(new_noc_cost.congestion,
                                 costs->noc_cost_terms.congestion,
                                 "noc_congestion_cost");
        }
        costs->noc_cost_terms.congestion = new_noc_cost.congestion;
    }
}

void alloc_and_load_chan_w_factors_for_place_cost(float place_cost_exp) {
    /* Allocates and loads the chanx_place_cost_fac and chany_place_cost_fac *
     * arrays with the inverse of the average number of tracks per channel   *
     * between [subhigh] and [sublow].  This is only useful for the cost     *
     * function that takes the length of the net bounding box in each        *
     * dimension divided by the average number of tracks in that direction.  *
     * For other cost functions, you don't have to bother calling this       *
     * routine; when using the cost function described above, however, you   *
     * must always call this routine after you call init_chan and before     *
     * you do any placement cost determination.  The place_cost_exp factor   *
     * specifies to what power the width of the channel should be taken --   *
     * larger numbers make narrower channels more expensive.                 */

    auto& device_ctx = g_vpr_ctx.device();

    /* 
    Access arrays below as chan?_place_cost_fac[subhigh][sublow]. Since subhigh must be greater than or 
    equal to sublow, we will only access the lower half of a matrix, but we allocate the whole matrix anyway 
    for simplicity so we can use the vtr utility matrix functions.
    */

    chanx_place_cost_fac.resize({device_ctx.grid.height(), device_ctx.grid.height() + 1});
    chany_place_cost_fac.resize({device_ctx.grid.width(), device_ctx.grid.width() + 1});

    /* First compute the number of tracks between channel high and channel *
     * low, inclusive, in an efficient manner.                             */

    chanx_place_cost_fac[0][0] = device_ctx.chan_width.x_list[0];

    for (size_t high = 1; high < device_ctx.grid.height(); high++) {
        chanx_place_cost_fac[high][high] = device_ctx.chan_width.x_list[high];
        for (size_t low = 0; low < high; low++) {
            chanx_place_cost_fac[high][low] = chanx_place_cost_fac[high - 1][low]
                                              + device_ctx.chan_width.x_list[high];
        }
    }

    /* Now compute the inverse of the average number of tracks per channel *
     * between high and low.  The cost function divides by the average     *
     * number of tracks per channel, so by storing the inverse I convert   *
     * this to a faster multiplication.  Take this final number to the     *
     * place_cost_exp power -- numbers other than one mean this is no      *
     * longer a simple "average number of tracks"; it is some power of     *
     * that, allowing greater penalization of narrow channels.             */

    for (size_t high = 0; high < device_ctx.grid.height(); high++)
        for (size_t low = 0; low <= high; low++) {
            /* Since we will divide the wiring cost by the average channel *
             * capacity between high and low, having only 0 width channels *
             * will result in infinite wiring capacity normalization       *
             * factor, and extremely bad placer behaviour. Hence we change *
             * this to a small (1 track) channel capacity instead.         */
            if (chanx_place_cost_fac[high][low] == 0.0f) {
                VTR_LOG_WARN("CHANX place cost fac is 0 at %d %d\n", high, low);
                chanx_place_cost_fac[high][low] = 1.0f;
            }

            chanx_place_cost_fac[high][low] = (high - low + 1.)
                                              / chanx_place_cost_fac[high][low];
            chanx_place_cost_fac[high][low] = pow(
                (double)chanx_place_cost_fac[high][low],
                (double)place_cost_exp);
        }

    /* Now do the same thing for the y-directed channels.  First get the  *
     * number of tracks between channel high and channel low, inclusive.  */

    chany_place_cost_fac[0][0] = device_ctx.chan_width.y_list[0];

    for (size_t high = 1; high < device_ctx.grid.width(); high++) {
        chany_place_cost_fac[high][high] = device_ctx.chan_width.y_list[high];
        for (size_t low = 0; low < high; low++) {
            chany_place_cost_fac[high][low] = chany_place_cost_fac[high - 1][low]
                                              + device_ctx.chan_width.y_list[high];
        }
    }

    /* Now compute the inverse of the average number of tracks per channel *
     * between high and low.  Take to specified power.                     */

    for (size_t high = 0; high < device_ctx.grid.width(); high++)
        for (size_t low = 0; low <= high; low++) {
            /* Since we will divide the wiring cost by the average channel *
             * capacity between high and low, having only 0 width channels *
             * will result in infinite wiring capacity normalization       *
             * factor, and extremely bad placer behaviour. Hence we change *
             * this to a small (1 track) channel capacity instead.         */
            if (chany_place_cost_fac[high][low] == 0.0f) {
                VTR_LOG_WARN("CHANY place cost fac is 0 at %d %d\n", high, low);
                chany_place_cost_fac[high][low] = 1.0f;
            }

            chany_place_cost_fac[high][low] = (high - low + 1.)
                                              / chany_place_cost_fac[high][low];
            chany_place_cost_fac[high][low] = pow(
                (double)chany_place_cost_fac[high][low],
                (double)place_cost_exp);
        }
}

void free_chan_w_factors_for_place_cost () {
    chanx_place_cost_fac.clear();
    chany_place_cost_fac.clear();
}

void init_place_move_structs(size_t num_nets) {
    net_cost.resize(num_nets, -1.);
    proposed_net_cost.resize(num_nets, -1.);
    /* Used to store costs for moves not yet made and to indicate when a net's   *
     * cost has been recomputed. proposed_net_cost[inet] < 0 means net's cost hasn't *
     * been recomputed.                                                          */
    bb_updated_before.resize(num_nets, NetUpdateState::NOT_UPDATED_YET);
}

void free_place_move_structs() {
    vtr::release_memory(net_cost);
    vtr::release_memory(proposed_net_cost);
    vtr::release_memory(bb_updated_before);
}

void init_try_swap_net_cost_structs(size_t num_nets, bool cube_bb) {
    const int num_layers = g_vpr_ctx.device().grid.get_num_layers();

    if (cube_bb) {
        ts_bb_edge_new.resize(num_nets, t_bb());
        ts_bb_coord_new.resize(num_nets, t_bb());
    } else {
        VTR_ASSERT_SAFE(!cube_bb);
        layer_ts_bb_edge_new.resize(num_nets, std::vector<t_2D_bb>(num_layers, t_2D_bb()));
        layer_ts_bb_coord_new.resize(num_nets, std::vector<t_2D_bb>(num_layers, t_2D_bb()));
    }

    /*This initialize the whole matrix to OPEN which is an invalid value*/
    ts_layer_sink_pin_count.resize({num_nets, size_t(num_layers)}, OPEN);

    ts_nets_to_update.resize(num_nets, ClusterNetId::INVALID());
}

void free_try_swap_net_cost_structs() {
    vtr::release_memory(ts_bb_edge_new);
    vtr::release_memory(ts_bb_coord_new);
    vtr::release_memory(layer_ts_bb_edge_new);
    vtr::release_memory(layer_ts_bb_coord_new);
    ts_layer_sink_pin_count.clear();
    vtr::release_memory(ts_nets_to_update);
}
