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

#include "clustered_netlist_fwd.h"
#include "globals.h"
#include "physical_types.h"
#include "placer_state.h"
#include "move_utils.h"
#include "place_timing_update.h"
#include "vtr_math.h"
#include "vtr_ndmatrix.h"
#include "vtr_ndoffsetmatrix.h"

#include <array>

static constexpr int MAX_FANOUT_CROSSING_COUNT = 50;

/**
 * @brief Crossing counts for nets with different #'s of pins.  From
 * ICCAD 94 pp. 690 - 695 (with linear interpolation applied by me).
 * Multiplied to bounding box of a net to better estimate wire length
 * for higher fanout nets. Each entry is the correction factor for the
 * fanout index-1
 */
constexpr std::array<float, MAX_FANOUT_CROSSING_COUNT> cross_count = {1.0000, 1.0000, 1.0000, 1.0828, 1.1536, 1.2206, 1.2823, 1.3385,
                                                                      1.3991, 1.4493, 1.4974, 1.5455, 1.5937, 1.6418, 1.6899, 1.7304,
                                                                      1.7709, 1.8114, 1.8519, 1.8924, 1.9288, 1.9652, 2.0015, 2.0379,
                                                                      2.0743, 2.1061, 2.1379, 2.1698, 2.2016, 2.2334, 2.2646, 2.2958,
                                                                      2.3271, 2.3583, 2.3895, 2.4187, 2.4479, 2.4772, 2.5064, 2.5356,
                                                                      2.5610, 2.5864, 2.6117, 2.6371, 2.6625, 2.6887, 2.7148, 2.7410,
                                                                      2.7671, 2.7933};

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
 * @brief Given the 3D BB, calculate the wire-length estimate of the net
 * @param net_id ID of the net which wirelength estimate is requested
 * @param bb Bounding box of the net
 * @return Wirelength estimate of the net
 */
static double get_net_wirelength_estimate(ClusterNetId net_id, const t_bb& bb);

/**
 * @brief To get the wirelength cost/est, BB perimeter is multiplied by a factor to approximately correct for the half-perimeter
 * bounding box wirelength's underestimate of wiring for nets with fanout greater than 2.
 * @return Multiplicative wirelength correction factor
 */
static double wirelength_crossing_count(size_t fanout);

/******************************* End of Function definitions ************************************/


NetCostHandler::NetCostHandler(const t_placer_opts& placer_opts,
                               PlacerState& placer_state,
                               bool cube_bb)
    : cube_bb_(cube_bb)
    , placer_state_(placer_state)
    , placer_opts_(placer_opts) {
    const int num_layers = g_vpr_ctx.device().grid.get_num_layers();
    const size_t num_nets = g_vpr_ctx.clustering().clb_nlist.nets().size();

    is_multi_layer_ = num_layers > 1;

    // Either 3D BB or per layer BB data structure are used, not both.
    if (cube_bb_) {
        ts_bb_edge_new_.resize(num_nets, t_bb());
        ts_bb_coord_new_.resize(num_nets, t_bb());
        comp_bb_cost_functor_ =  std::bind(&NetCostHandler::comp_cube_bb_cost_, this, std::placeholders::_1);
        update_bb_functor_ = std::bind(&NetCostHandler::update_bb_, this, std::placeholders::_1, std::placeholders::_2,
                                       std::placeholders::_3, std::placeholders::_4);
        get_net_bb_cost_functor_ = std::bind(&NetCostHandler::get_net_cube_bb_cost_, this, std::placeholders::_1, /*use_ts=*/true);
        get_non_updatable_bb_functor_ = std::bind(&NetCostHandler::get_non_updatable_cube_bb_, this, std::placeholders::_1, /*use_ts=*/true);
    } else {
        layer_ts_bb_edge_new_.resize(num_nets, std::vector<t_2D_bb>(num_layers, t_2D_bb()));
        layer_ts_bb_coord_new_.resize(num_nets, std::vector<t_2D_bb>(num_layers, t_2D_bb()));
        comp_bb_cost_functor_ =  std::bind(&NetCostHandler::comp_per_layer_bb_cost_, this, std::placeholders::_1);
        update_bb_functor_ = std::bind(&NetCostHandler::update_layer_bb_, this, std::placeholders::_1, std::placeholders::_2,
                                       std::placeholders::_3, std::placeholders::_4);
        get_net_bb_cost_functor_ = std::bind(&NetCostHandler::get_net_per_layer_bb_cost_, this, std::placeholders::_1, /*use_ts=*/true);
        get_non_updatable_bb_functor_ = std::bind(&NetCostHandler::get_non_updatable_per_layer_bb_, this, std::placeholders::_1, /*use_ts=*/true);
    }

    /* This initializes the whole matrix to OPEN which is an invalid value*/
    ts_layer_sink_pin_count_.resize({num_nets, size_t(num_layers)}, OPEN);

    ts_nets_to_update_.resize(num_nets, ClusterNetId::INVALID());

    // negative net costs mean the cost is not valid.
    net_cost_.resize(num_nets, -1.);
    proposed_net_cost_.resize(num_nets, -1.);

    /* Used to store costs for moves not yet made and to indicate when a net's
     * cost has been recomputed. proposed_net_cost[inet] < 0 means net's cost hasn't
     * been recomputed. */
    bb_update_status_.resize(num_nets, NetUpdateState::NOT_UPDATED_YET);

    alloc_and_load_chan_w_factors_for_place_cost_();
}

void NetCostHandler::alloc_and_load_chan_w_factors_for_place_cost_() {
    const auto& device_ctx = g_vpr_ctx.device();

    const int grid_height = (int)device_ctx.grid.height();
    const int grid_width = (int)device_ctx.grid.width();

    /* These arrays contain accumulative channel width between channel zero and
     * the channel specified by the given index. The accumulated channel width
     * is inclusive, meaning that it includes both channel zero and channel `idx`.
     * To compute the total channel width between channels 'low' and 'high', use the
     * following formula:
     *      acc_chan?_width_[high] - acc_chan?_width_[low - 1]
     * This returns the total number of tracks between channels 'low' and 'high',
     * including tracks in these channels.
     *
     * Channel -1 doesn't exist, so we can say it has zero tracks. We need to be able
     * to access these arrays with index -1 to handle cases where the lower channel is 0.
     */
    acc_chanx_width_ = vtr::NdOffsetMatrix<int, 1>({{{-1, grid_height}}});
    acc_chany_width_ = vtr::NdOffsetMatrix<int, 1>({{{-1, grid_width}}});

    // initialize the first element (index -1) with zero
    acc_chanx_width_[-1] = 0;
    for (int y = 0; y < grid_height; y++) {
        acc_chanx_width_[y] = acc_chanx_width_[y - 1] + device_ctx.chan_width.x_list[y];

        /* If the number of tracks in a channel is zero, two consecutive elements take the same
         * value. This can lead to a division by zero in get_chanxy_cost_fac_(). To avoid this
         * potential issue, we assume that the channel width is at least 1.
         */
        if (acc_chanx_width_[y] == acc_chanx_width_[y - 1]) {
            acc_chanx_width_[y]++;
        }
    }

    // initialize the first element (index -1) with zero
    acc_chany_width_[-1] = 0;
    for (int x = 0; x < grid_width; x++) {
        acc_chany_width_[x] = acc_chany_width_[x - 1] + device_ctx.chan_width.y_list[x];

        // to avoid a division by zero
        if (acc_chany_width_[x] == acc_chany_width_[x - 1]) {
            acc_chany_width_[x]++;
        }
    }
    
    if (is_multi_layer_) {
        alloc_and_load_for_fast_vertical_cost_update_();
    }
}

void NetCostHandler::alloc_and_load_for_fast_vertical_cost_update_() {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    
    const size_t grid_height = device_ctx.grid.height();
    const size_t grid_width = device_ctx.grid.width();

    acc_tile_num_inter_die_conn_ = vtr::NdMatrix<int, 2>({grid_width, grid_height}, 0);

    vtr::NdMatrix<float, 2> tile_num_inter_die_conn({grid_width, grid_height}, 0.);         

    /*
     * Step 1: iterate over the rr-graph, recording how many edges go between layers at each (x,y) location
     * in the device. We count all these edges, regardless of which layers they connect. Then we divide by
     * the number of layers - 1 to get the average cross-layer edge count per (x,y) location -- this mirrors
     * what we do for the horizontal and vertical channels where we assume the channel width doesn't change
     * along the length of the channel. It lets us be more memory-efficient for 3D devices, and could be revisited
     * if someday we have architectures with widely varying connectivity between different layers in a stack.
     */

    /*
     * To calculate the accumulative number of inter-die connections we first need to get the number of
     * inter-die connection per location. To be able to work for the cases that RR Graph is read instead
     * of being made from the architecture file, we calculate this number by iterating over the RR graph. Once
     * tile_num_inter_die_conn is populated, we can start populating acc_tile_num_inter_die_conn_. First,
     * we populate the first row and column. Then, we iterate over the rest of blocks and get the number of
     * inter-die connections by adding up the number of inter-die block at that location + the accumulation
     * for the  block below and  left to it. Then, since the accumulated number of inter-die connection to
     * the block on the lower left connection of the block is added twice, that part needs to be removed.
     */
    for (const RRNodeId src_rr_node : rr_graph.nodes()) {
        for (const t_edge_size rr_edge_idx : rr_graph.edges(src_rr_node)) {
            const RRNodeId sink_rr_node = rr_graph.edge_sink_node(src_rr_node, rr_edge_idx);
            if (rr_graph.node_layer(src_rr_node) != rr_graph.node_layer(sink_rr_node)) {
                // We assume that the nodes driving the inter-layer connection or being driven by it
                // are not stretched across multiple tiles
                int src_x = rr_graph.node_xhigh(src_rr_node);
                int src_y = rr_graph.node_yhigh(src_rr_node);
                VTR_ASSERT(rr_graph.node_xlow(src_rr_node) == src_x && rr_graph.node_ylow(src_rr_node) == src_y);

                tile_num_inter_die_conn[src_x][src_y]++;
            }
        }
    }

    int num_layers = device_ctx.grid.get_num_layers();
    for (size_t x = 0; x < device_ctx.grid.width(); x++) {
        for (size_t y = 0; y < device_ctx.grid.height(); y++) {
            tile_num_inter_die_conn[x][y] /= (num_layers-1);
        }
    }

    // Step 2: Calculate prefix sum of the inter-die connectivity up to and including the channel at (x, y).
    acc_tile_num_inter_die_conn_[0][0] = tile_num_inter_die_conn[0][0];
    // Initialize the first row and column
    for (size_t x = 1; x < device_ctx.grid.width(); x++) {
        acc_tile_num_inter_die_conn_[x][0] = acc_tile_num_inter_die_conn_[x-1][0] +
                                             tile_num_inter_die_conn[x][0];
    }

    for (size_t y = 1; y < device_ctx.grid.height(); y++) {
        acc_tile_num_inter_die_conn_[0][y] = acc_tile_num_inter_die_conn_[0][y-1] +
                                             tile_num_inter_die_conn[0][y];
    }
    
    for (size_t x_high = 1; x_high < device_ctx.grid.width(); x_high++) {
        for (size_t y_high = 1; y_high < device_ctx.grid.height(); y_high++) {
            acc_tile_num_inter_die_conn_[x_high][y_high] = acc_tile_num_inter_die_conn_[x_high-1][y_high] +
                                                           acc_tile_num_inter_die_conn_[x_high][y_high-1] +
                                                           tile_num_inter_die_conn[x_high][y_high] -
                                                           acc_tile_num_inter_die_conn_[x_high-1][y_high-1];
        }
    }
}

double NetCostHandler::comp_bb_cost(e_cost_methods method) {
    return comp_bb_cost_functor_(method);
}

double NetCostHandler::comp_cube_bb_cost_(e_cost_methods method) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_move_ctx = placer_state_.mutable_move();

    double cost = 0;
    double expected_wirelength = 0.0;

    for (ClusterNetId net_id : cluster_ctx.clb_nlist.nets()) {       /* for each net ... */
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) { /* Do only if not ignored. */
            /* Small nets don't use incremental updating on their bounding boxes, *
             * so they can use a fast bounding box calculator.                    */
            if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= SMALL_NET && method == e_cost_methods::NORMAL) {
                get_bb_from_scratch_(net_id,
                                     place_move_ctx.bb_coords[net_id],
                                     place_move_ctx.bb_num_on_edges[net_id],
                                     place_move_ctx.num_sink_pin_layer[size_t(net_id)]);
            } else {
                get_non_updatable_cube_bb_(net_id, /*use_ts=*/false);
            }

            net_cost_[net_id] = get_net_cube_bb_cost_(net_id, /*use_ts=*/false);
            cost += net_cost_[net_id];
            if (method == e_cost_methods::CHECK) {
                expected_wirelength += get_net_wirelength_estimate(net_id, place_move_ctx.bb_coords[net_id]);
            }
        }
    }

    if (method == e_cost_methods::CHECK) {
        VTR_LOG("\n");
        VTR_LOG("BB estimate of min-dist (placement) wire length: %.0f\n",
                expected_wirelength);
    }

    return cost;
}

double NetCostHandler::comp_per_layer_bb_cost_(e_cost_methods method) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_move_ctx = placer_state_.mutable_move();

    double cost = 0;
    double expected_wirelength = 0.0;

    for (ClusterNetId net_id : cluster_ctx.clb_nlist.nets()) {       /* for each net ... */
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) { /* Do only if not ignored. */
            /* Small nets don't use incremental updating on their bounding boxes, *
             * so they can use a fast bounding box calculator.                    */
            if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= SMALL_NET && method == e_cost_methods::NORMAL) {
                get_layer_bb_from_scratch_(net_id,
                                           place_move_ctx.layer_bb_num_on_edges[net_id],
                                           place_move_ctx.layer_bb_coords[net_id],
                                           place_move_ctx.num_sink_pin_layer[size_t(net_id)]);
            } else {
                get_non_updatable_per_layer_bb_(net_id, /*use_ts=*/false);
            }

            net_cost_[net_id] = get_net_per_layer_bb_cost_(net_id, /*use_ts=*/false);
            cost += net_cost_[net_id];
            if (method == e_cost_methods::CHECK) {
                expected_wirelength += get_net_wirelength_from_layer_bb_(net_id);
            }
        }
    }

    if (method == e_cost_methods::CHECK) {
        VTR_LOG("\n");
        VTR_LOG("BB estimate of min-dist (placement) wire length: %.0f\n",
                expected_wirelength);
    }

    return cost;
}

void NetCostHandler::update_net_bb_(const ClusterNetId net,
                                    const ClusterBlockId blk,
                                    const ClusterPinId blk_pin,
                                    const t_pl_moved_block& pl_moved_block) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& block_locs = placer_state_.block_locs();

    if (cluster_ctx.clb_nlist.net_sinks(net).size() < SMALL_NET) {
        //For small nets brute-force bounding box update is faster

        if (bb_update_status_[net] == NetUpdateState::NOT_UPDATED_YET) { //Only once per-net
            get_non_updatable_bb_functor_(net);
        }
    } else {
        //For large nets, update bounding box incrementally
        int iblk_pin = placer_state_.blk_loc_registry().tile_pin_index(blk_pin);

        t_pl_loc block_loc = block_locs[blk].loc;
        t_physical_tile_type_ptr blk_type = physical_tile_type(block_loc);
        int pin_width_offset = blk_type->pin_width_offset[iblk_pin];
        int pin_height_offset = blk_type->pin_height_offset[iblk_pin];
        bool is_driver = cluster_ctx.clb_nlist.pin_type(blk_pin) == PinType::DRIVER;

        //Incremental bounding box update
        update_bb_functor_(net,
                           {pl_moved_block.old_loc.x + pin_width_offset,
                            pl_moved_block.old_loc.y + pin_height_offset,
                            pl_moved_block.old_loc.layer},
                           {pl_moved_block.new_loc.x + pin_width_offset,
                            pl_moved_block.new_loc.y + pin_height_offset,
                            pl_moved_block.new_loc.layer},
                           is_driver);
    }
}

void NetCostHandler::update_td_delta_costs_(const PlaceDelayModel* delay_model,
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
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& block_locs = placer_state_.block_locs();

    const auto& connection_delay = placer_state_.timing().connection_delay;
    auto& connection_timing_cost = placer_state_.mutable_timing().connection_timing_cost;
    auto& proposed_connection_delay = placer_state_.mutable_timing().proposed_connection_delay;
    auto& proposed_connection_timing_cost = placer_state_.mutable_timing().proposed_connection_timing_cost;

    if (cluster_ctx.clb_nlist.pin_type(pin) == PinType::DRIVER) {
        /* This pin is a net driver on a moved block. */
        /* Recompute all point to point connection delays for the net sinks. */
        for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net).size(); ipin++) {
            float temp_delay = comp_td_single_connection_delay(delay_model, block_locs, net, ipin);
            /* If the delay hasn't changed, do not mark this pin as affected */
            if (temp_delay == connection_delay[net][ipin]) {
                continue;
            }

            /* Calculate proposed delay and cost values */
            proposed_connection_delay[net][ipin] = temp_delay;

            proposed_connection_timing_cost[net][ipin] = criticalities.criticality(net, ipin) * temp_delay;
            delta_timing_cost += proposed_connection_timing_cost[net][ipin] - connection_timing_cost[net][ipin];

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

            float temp_delay = comp_td_single_connection_delay(delay_model, block_locs, net, ipin);
            /* If the delay hasn't changed, do not mark this pin as affected */
            if (temp_delay == connection_delay[net][ipin]) {
                return;
            }

            /* Calculate proposed delay and cost values */
            proposed_connection_delay[net][ipin] = temp_delay;

            proposed_connection_timing_cost[net][ipin] = criticalities.criticality(net, ipin) * temp_delay;
            delta_timing_cost += proposed_connection_timing_cost[net][ipin] - connection_timing_cost[net][ipin];

            /* Record this connection in blocks_affected.affected_pins */
            affected_pins.push_back(pin);
        }
    }
}

///@brief Record effected nets.
void NetCostHandler::record_affected_net_(const ClusterNetId net) {
    /* Record effected nets. */
    if (proposed_net_cost_[net] < 0.) {
        /* Net not marked yet. */
        VTR_ASSERT_SAFE(ts_nets_to_update_.size() < ts_nets_to_update_.capacity());
        ts_nets_to_update_.push_back(net);

        /* Flag to say we've marked this net. */
        proposed_net_cost_[net] = 1.;
    }
}

void NetCostHandler::update_net_info_on_pin_move_(const PlaceDelayModel* delay_model,
                                                  const PlacerCriticalities* criticalities,
                                                  const ClusterPinId pin_id,
                                                  const t_pl_moved_block& moving_blk_inf,
                                                  std::vector<ClusterPinId>& affected_pins,
                                                  double& timing_delta_c,
                                                  bool is_src_moving) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    const ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(pin_id);
    VTR_ASSERT_SAFE_MSG(net_id,
                        "Only valid nets should be found in compressed netlist block pins");

    if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
        //TODO: Do we require anything special here for global nets?
        //"Global nets are assumed to span the whole chip, and do not effect costs."
        return;
    }

    /* Record effected nets */
    record_affected_net_(net_id);

    ClusterBlockId blk_id = moving_blk_inf.block_num;
    /* Update the net bounding boxes. */
    update_net_bb_(net_id, blk_id, pin_id, moving_blk_inf);

    if (placer_opts_.place_algorithm.is_timing_driven()) {
        /* Determine the change in connection delay and timing cost. */
        update_td_delta_costs_(delay_model,
                               *criticalities,
                               net_id,
                               pin_id,
                               affected_pins,
                               timing_delta_c,
                               is_src_moving);
    }
}

void NetCostHandler::get_non_updatable_cube_bb_(ClusterNetId net_id, bool use_ts) {
    //TODO: account for multiple physical pin instances per logical pin
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& blk_loc_registry = placer_state_.blk_loc_registry();
    auto& move_ctx = placer_state_.mutable_move();

    // the bounding box coordinates that is going to be updated by this function
    t_bb& bb_coord_new = use_ts ? ts_bb_coord_new_[net_id] : move_ctx.bb_coords[net_id];
    // the number of sink pins of "net_id" on each layer
    vtr::NdMatrixProxy<int, 1> num_sink_pin_layer = use_ts ? ts_layer_sink_pin_count_[size_t(net_id)] : move_ctx.num_sink_pin_layer[size_t(net_id)];

    // get the source pin's location
    ClusterPinId source_pin_id = cluster_ctx.clb_nlist.net_pin(net_id, 0);
    t_physical_tile_loc source_pin_loc = blk_loc_registry.get_coordinate_of_pin(source_pin_id);

    // initialize the bounding box coordinates with the source pin's coordinates
    bb_coord_new.xmin = source_pin_loc.x;
    bb_coord_new.ymin = source_pin_loc.y;
    bb_coord_new.layer_min = source_pin_loc.layer_num;
    bb_coord_new.xmax = source_pin_loc.x;
    bb_coord_new.ymax = source_pin_loc.y;
    bb_coord_new.layer_max = source_pin_loc.layer_num;

    for (int layer_num = 0; layer_num < device_ctx.grid.get_num_layers(); layer_num++) {
        num_sink_pin_layer[layer_num] = 0;
    }

    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
        t_physical_tile_loc pin_loc = blk_loc_registry.get_coordinate_of_pin(pin_id);

        if (pin_loc.x < bb_coord_new.xmin) {
            bb_coord_new.xmin = pin_loc.x;
        } else if (pin_loc.x > bb_coord_new.xmax) {
            bb_coord_new.xmax = pin_loc.x;
        }

        if (pin_loc.y < bb_coord_new.ymin) {
            bb_coord_new.ymin = pin_loc.y;
        } else if (pin_loc.y > bb_coord_new.ymax) {
            bb_coord_new.ymax = pin_loc.y;
        }

        if (pin_loc.layer_num < bb_coord_new.layer_min) {
            bb_coord_new.layer_min = pin_loc.layer_num;
        } else if (pin_loc.layer_num > bb_coord_new.layer_max) {
            bb_coord_new.layer_max = pin_loc.layer_num;
        }

        num_sink_pin_layer[pin_loc.layer_num]++;
    }
}

void NetCostHandler::get_non_updatable_per_layer_bb_(ClusterNetId net_id, bool use_ts) {
    //TODO: account for multiple physical pin instances per logical pin
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& blk_loc_registry = placer_state_.blk_loc_registry();
    auto& move_ctx = placer_state_.mutable_move();

    std::vector<t_2D_bb>& bb_coord_new = use_ts ? layer_ts_bb_coord_new_[net_id] : move_ctx.layer_bb_coords[net_id];
    vtr::NdMatrixProxy<int, 1> num_sink_layer = use_ts ? ts_layer_sink_pin_count_[size_t(net_id)] : move_ctx.num_sink_pin_layer[size_t(net_id)];

    const int num_layers = device_ctx.grid.get_num_layers();
    VTR_ASSERT_DEBUG(bb_coord_new.size() == (size_t)num_layers);

    // get the source pin's location
    ClusterPinId source_pin_id = cluster_ctx.clb_nlist.net_pin(net_id, 0);
    t_physical_tile_loc source_pin_loc = blk_loc_registry.get_coordinate_of_pin(source_pin_id);

    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        bb_coord_new[layer_num] = t_2D_bb{source_pin_loc.x, source_pin_loc.x, source_pin_loc.y, source_pin_loc.y, source_pin_loc.layer_num};
        num_sink_layer[layer_num] = 0;
    }

    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
        t_physical_tile_loc pin_loc = blk_loc_registry.get_coordinate_of_pin(pin_id);
        num_sink_layer[pin_loc.layer_num]++;

        if (pin_loc.x < bb_coord_new[pin_loc.layer_num].xmin) {
            bb_coord_new[pin_loc.layer_num].xmin = pin_loc.x;
        } else if (pin_loc.x > bb_coord_new[pin_loc.layer_num].xmax) {
            bb_coord_new[pin_loc.layer_num].xmax = pin_loc.x;
        }

        if (pin_loc.y < bb_coord_new[pin_loc.layer_num].ymin) {
            bb_coord_new[pin_loc.layer_num].ymin = pin_loc.y;
        } else if (pin_loc.y > bb_coord_new[pin_loc.layer_num].ymax) {
            bb_coord_new[pin_loc.layer_num].ymax = pin_loc.y;
        }
    }
}

void NetCostHandler::update_bb_(ClusterNetId net_id,
                                t_physical_tile_loc pin_old_loc,
                                t_physical_tile_loc pin_new_loc,
                                bool src_pin) {
    //TODO: account for multiple physical pin instances per logical pin
    const t_bb *curr_bb_edge, *curr_bb_coord;

    auto& device_ctx = g_vpr_ctx.device();
    auto& place_move_ctx = placer_state_.move();

    const int num_layers = device_ctx.grid.get_num_layers();

    // Number of blocks on the edges of the bounding box
    t_bb& bb_edge_new = ts_bb_edge_new_[net_id];
    // Coordinates of the bounding box
    t_bb& bb_coord_new = ts_bb_coord_new_[net_id];
    // Number of sinks of the given net on each layer
    vtr::NdMatrixProxy<int, 1> num_sink_pin_layer_new = ts_layer_sink_pin_count_[size_t(net_id)];

    /* Check if the net had been updated before. */
    if (bb_update_status_[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
        /* The net had been updated from scratch, DO NOT update again! */
        return;
    }

    vtr::NdMatrixProxy<int, 1> curr_num_sink_pin_layer = (bb_update_status_[net_id] == NetUpdateState::NOT_UPDATED_YET) ? place_move_ctx.num_sink_pin_layer[size_t(net_id)] : num_sink_pin_layer_new;

    if (bb_update_status_[net_id] == NetUpdateState::NOT_UPDATED_YET) {
        /* The net had NOT been updated before, could use the old values */
        curr_bb_edge = &place_move_ctx.bb_num_on_edges[net_id];
        curr_bb_coord = &place_move_ctx.bb_coords[net_id];
        bb_update_status_[net_id] = NetUpdateState::UPDATED_ONCE;
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
                get_bb_from_scratch_(net_id, bb_coord_new, bb_edge_new, num_sink_pin_layer_new);
                bb_update_status_[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
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
                get_bb_from_scratch_(net_id, bb_coord_new, bb_edge_new, num_sink_pin_layer_new);
                bb_update_status_[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
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
                get_bb_from_scratch_(net_id, bb_coord_new, bb_edge_new, num_sink_pin_layer_new);
                bb_update_status_[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
                return;
            } else {
                bb_edge_new.ymax = curr_bb_edge->ymax - 1;
                bb_coord_new.ymax = curr_bb_coord->ymax;
            }
        } else { /* Move down, old position was not at ymax. */
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
                get_bb_from_scratch_(net_id, bb_coord_new, bb_edge_new, num_sink_pin_layer_new);
                bb_update_status_[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
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
    if (is_multi_layer_) {
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
                    get_bb_from_scratch_(net_id, bb_coord_new, bb_edge_new, num_sink_pin_layer_new);
                    bb_update_status_[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
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
                    get_bb_from_scratch_(net_id, bb_coord_new, bb_edge_new, num_sink_pin_layer_new);
                    bb_update_status_[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
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

        } else { //pin_new_loc.layer_num == pin_old_loc.layer_num
            bb_coord_new.layer_min = curr_bb_coord->layer_min;
            bb_coord_new.layer_max = curr_bb_coord->layer_max;
            bb_edge_new.layer_min = curr_bb_edge->layer_min;
            bb_edge_new.layer_max = curr_bb_edge->layer_max;
        }

    } else { // num_layers == 1
        bb_coord_new.layer_min = curr_bb_coord->layer_min;
        bb_coord_new.layer_max = curr_bb_coord->layer_max;
        bb_edge_new.layer_min = curr_bb_edge->layer_min;
        bb_edge_new.layer_max = curr_bb_edge->layer_max;
    }

    if (bb_update_status_[net_id] == NetUpdateState::NOT_UPDATED_YET) {
        bb_update_status_[net_id] = NetUpdateState::UPDATED_ONCE;
    }
}

void NetCostHandler::update_layer_bb_(ClusterNetId net_id,
                                      t_physical_tile_loc pin_old_loc,
                                      t_physical_tile_loc pin_new_loc,
                                      bool is_output_pin) {
    auto& place_move_ctx = placer_state_.move();

    std::vector<t_2D_bb>& bb_edge_new = layer_ts_bb_edge_new_[net_id];
    std::vector<t_2D_bb>& bb_coord_new = layer_ts_bb_coord_new_[net_id];
    vtr::NdMatrixProxy<int, 1> bb_pin_sink_count_new = ts_layer_sink_pin_count_[size_t(net_id)];

    /* Check if the net had been updated before. */
    if (bb_update_status_[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
        /* The net had been updated from scratch, DO NOT update again! */
        return;
    }

    const vtr::NdMatrixProxy<int, 1> curr_layer_pin_sink_count = (bb_update_status_[net_id] == NetUpdateState::NOT_UPDATED_YET) ? place_move_ctx.num_sink_pin_layer[size_t(net_id)] : bb_pin_sink_count_new;

    const std::vector<t_2D_bb>*curr_bb_edge, *curr_bb_coord;
    if (bb_update_status_[net_id] == NetUpdateState::NOT_UPDATED_YET) {
        /* The net had NOT been updated before, could use the old values */
        curr_bb_edge = &place_move_ctx.layer_bb_num_on_edges[net_id];
        curr_bb_coord = &place_move_ctx.layer_bb_coords[net_id];
        bb_update_status_[net_id] = NetUpdateState::UPDATED_ONCE;
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
        update_bb_layer_changed_(net_id,
                                 pin_old_loc,
                                 pin_new_loc,
                                 *curr_bb_edge,
                                 *curr_bb_coord,
                                 bb_pin_sink_count_new,
                                 bb_edge_new,
                                 bb_coord_new);
    } else {
        update_bb_same_layer_(net_id,
                              pin_old_loc,
                              pin_new_loc,
                              *curr_bb_edge,
                              *curr_bb_coord,
                              bb_pin_sink_count_new,
                              bb_edge_new,
                              bb_coord_new);
    }

    if (bb_update_status_[net_id] == NetUpdateState::NOT_UPDATED_YET) {
        bb_update_status_[net_id] = NetUpdateState::UPDATED_ONCE;
    }
}

inline void NetCostHandler::update_bb_same_layer_(ClusterNetId net_id,
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
            update_bb_edge_(net_id,
                           bb_edge_new,
                           bb_coord_new,
                           bb_pin_sink_count_new,
                           curr_bb_edge[layer_num].xmax,
                           curr_bb_coord[layer_num].xmax,
                           bb_edge_new[layer_num].xmax,
                           bb_coord_new[layer_num].xmax);
            if (bb_update_status_[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
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
            update_bb_edge_(net_id,
                           bb_edge_new,
                           bb_coord_new,
                           bb_pin_sink_count_new,
                           curr_bb_edge[layer_num].xmin,
                           curr_bb_coord[layer_num].xmin,
                           bb_edge_new[layer_num].xmin,
                           bb_coord_new[layer_num].xmin);
            if (bb_update_status_[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
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
            update_bb_edge_(net_id,
                           bb_edge_new,
                           bb_coord_new,
                           bb_pin_sink_count_new,
                           curr_bb_edge[layer_num].ymax,
                           curr_bb_coord[layer_num].ymax,
                           bb_edge_new[layer_num].ymax,
                           bb_coord_new[layer_num].ymax);
            if (bb_update_status_[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
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
            update_bb_edge_(net_id,
                           bb_edge_new,
                           bb_coord_new,
                           bb_pin_sink_count_new,
                           curr_bb_edge[layer_num].ymin,
                           curr_bb_coord[layer_num].ymin,
                           bb_edge_new[layer_num].ymin,
                           bb_coord_new[layer_num].ymin);
            if (bb_update_status_[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
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

inline void NetCostHandler::update_bb_layer_changed_(ClusterNetId net_id,
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
        update_bb_edge_(net_id,
                       bb_edge_new,
                       bb_coord_new,
                       bb_pin_sink_count_new,
                       curr_bb_edge[old_layer_num].xmax,
                       curr_bb_coord[old_layer_num].xmax,
                       bb_edge_new[old_layer_num].xmax,
                       bb_coord_new[old_layer_num].xmax);
        if (bb_update_status_[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
            return;
        }
    } else if (x_old == curr_bb_coord[old_layer_num].xmin) {
        update_bb_edge_(net_id,
                       bb_edge_new,
                       bb_coord_new,
                       bb_pin_sink_count_new,
                       curr_bb_edge[old_layer_num].xmin,
                       curr_bb_coord[old_layer_num].xmin,
                       bb_edge_new[old_layer_num].xmin,
                       bb_coord_new[old_layer_num].xmin);
        if (bb_update_status_[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
            return;
        }
    }

    if (y_old == curr_bb_coord[old_layer_num].ymax) {
        update_bb_edge_(net_id,
                       bb_edge_new,
                       bb_coord_new,
                       bb_pin_sink_count_new,
                       curr_bb_edge[old_layer_num].ymax,
                       curr_bb_coord[old_layer_num].ymax,
                       bb_edge_new[old_layer_num].ymax,
                       bb_coord_new[old_layer_num].ymax);
        if (bb_update_status_[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
            return;
        }
    } else if (y_old == curr_bb_coord[old_layer_num].ymin) {
        update_bb_edge_(net_id,
                       bb_edge_new,
                       bb_coord_new,
                       bb_pin_sink_count_new,
                       curr_bb_edge[old_layer_num].ymin,
                       curr_bb_coord[old_layer_num].ymin,
                       bb_edge_new[old_layer_num].ymin,
                       bb_coord_new[old_layer_num].ymin);
        if (bb_update_status_[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
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

inline void NetCostHandler::update_bb_edge_(ClusterNetId net_id,
                                  std::vector<t_2D_bb>& bb_edge_new,
                                  std::vector<t_2D_bb>& bb_coord_new,
                                  vtr::NdMatrixProxy<int, 1> bb_layer_pin_sink_count,
                                  const int& old_num_block_on_edge,
                                  const int& old_edge_coord,
                                  int& new_num_block_on_edge,
                                  int& new_edge_coord) {
    if (old_num_block_on_edge == 1) {
        get_layer_bb_from_scratch_(net_id,
                                  bb_edge_new,
                                  bb_coord_new,
                                  bb_layer_pin_sink_count);
        bb_update_status_[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
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

void NetCostHandler::get_bb_from_scratch_(ClusterNetId net_id,
                                          t_bb& coords,
                                          t_bb& num_on_edges,
                                          vtr::NdMatrixProxy<int, 1> num_sink_pin_layer) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& grid = device_ctx.grid;
    const auto& blk_loc_registry = placer_state_.blk_loc_registry();

    // get the source pin's location
    ClusterPinId source_pin_id = cluster_ctx.clb_nlist.net_pin(net_id, 0);
    t_physical_tile_loc source_pin_loc = blk_loc_registry.get_coordinate_of_pin(source_pin_id);

    int xmin = source_pin_loc.x;
    int ymin = source_pin_loc.y;
    int layer_min = source_pin_loc.layer_num;
    int xmax = source_pin_loc.x;
    int ymax = source_pin_loc.y;
    int layer_max = source_pin_loc.layer_num;

    int xmin_edge = 1;
    int ymin_edge = 1;
    int layer_min_edge = 1;
    int xmax_edge = 1;
    int ymax_edge = 1;
    int layer_max_edge = 1;

    for (int layer_num = 0; layer_num < grid.get_num_layers(); layer_num++) {
        num_sink_pin_layer[layer_num] = 0;
    }

    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
        t_physical_tile_loc pin_loc = blk_loc_registry.get_coordinate_of_pin(pin_id);

        if (pin_loc.x == xmin) {
            xmin_edge++;
        }
        if (pin_loc.x == xmax) { /* Recall that xmin could equal xmax -- don't use else */
            xmax_edge++;
        } else if (pin_loc.x < xmin) {
            xmin = pin_loc.x;
            xmin_edge = 1;
        } else if (pin_loc.x > xmax) {
            xmax = pin_loc.x;
            xmax_edge = 1;
        }

        if (pin_loc.y == ymin) {
            ymin_edge++;
        }
        if (pin_loc.y == ymax) {
            ymax_edge++;
        } else if (pin_loc.y < ymin) {
            ymin = pin_loc.y;
            ymin_edge = 1;
        } else if (pin_loc.y > ymax) {
            ymax = pin_loc.y;
            ymax_edge = 1;
        }

        if (pin_loc.layer_num == layer_min) {
            layer_min_edge++;
        }
        if (pin_loc.layer_num == layer_max) {
            layer_max_edge++;
        } else if (pin_loc.layer_num < layer_min) {
            layer_min = pin_loc.layer_num;
            layer_min_edge = 1;
        } else if (pin_loc.layer_num > layer_max) {
            layer_max = pin_loc.layer_num;
            layer_max_edge = 1;
        }

        num_sink_pin_layer[pin_loc.layer_num]++;
    }

    // Copy the coordinates and number on edges information into the proper structures.
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

void NetCostHandler::get_layer_bb_from_scratch_(ClusterNetId net_id,
                                                std::vector<t_2D_bb>& num_on_edges,
                                                std::vector<t_2D_bb>& coords,
                                                vtr::NdMatrixProxy<int, 1> layer_pin_sink_count) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& blk_loc_registry = placer_state_.blk_loc_registry();

    const int num_layers = device_ctx.grid.get_num_layers();
    VTR_ASSERT_DEBUG(coords.size() == (size_t)num_layers);
    VTR_ASSERT_DEBUG(num_on_edges.size() == (size_t)num_layers);

    // get the source pin's location
    ClusterPinId source_pin_id = cluster_ctx.clb_nlist.net_pin(net_id, 0);
    t_physical_tile_loc source_pin_loc = blk_loc_registry.get_coordinate_of_pin(source_pin_id);

    // TODO: Currently we are assuming that crossing can only happen from OPIN. Because of that,
    // when per-layer bounding box is used, we want the bounding box on each layer to also include
    // the location of source since the connection on each layer starts from that location.
    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        coords[layer_num] = t_2D_bb{source_pin_loc.x, source_pin_loc.x, source_pin_loc.y, source_pin_loc.y, source_pin_loc.layer_num};
        num_on_edges[layer_num] = t_2D_bb{1, 1, 1, 1, layer_num};
        layer_pin_sink_count[layer_num] = 0;
    }

    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
        t_physical_tile_loc pin_loc = blk_loc_registry.get_coordinate_of_pin(pin_id);
        VTR_ASSERT_SAFE(pin_loc.layer_num >= 0 && pin_loc.layer_num < num_layers);
        layer_pin_sink_count[pin_loc.layer_num]++;

        if (pin_loc.x == coords[pin_loc.layer_num].xmin) {
            num_on_edges[pin_loc.layer_num].xmin++;
        }
        if (pin_loc.x == coords[pin_loc.layer_num].xmax) { /* Recall that xmin could equal xmax -- don't use else */
            num_on_edges[pin_loc.layer_num].xmax++;
        } else if (pin_loc.x < coords[pin_loc.layer_num].xmin) {
            coords[pin_loc.layer_num].xmin = pin_loc.x;
            num_on_edges[pin_loc.layer_num].xmin = 1;
        } else if (pin_loc.x > coords[pin_loc.layer_num].xmax) {
            coords[pin_loc.layer_num].xmax = pin_loc.x;
            num_on_edges[pin_loc.layer_num].xmax = 1;
        }

        if (pin_loc.y == coords[pin_loc.layer_num].ymin) {
            num_on_edges[pin_loc.layer_num].ymin++;
        }
        if (pin_loc.y == coords[pin_loc.layer_num].ymax) {
            num_on_edges[pin_loc.layer_num].ymax++;
        } else if (pin_loc.y < coords[pin_loc.layer_num].ymin) {
            coords[pin_loc.layer_num].ymin = pin_loc.y;
            num_on_edges[pin_loc.layer_num].ymin = 1;
        } else if (pin_loc.y > coords[pin_loc.layer_num].ymax) {
            coords[pin_loc.layer_num].ymax = pin_loc.y;
            num_on_edges[pin_loc.layer_num].ymax = 1;
        }
    }
}


double NetCostHandler::get_net_cube_bb_cost_(ClusterNetId net_id, bool use_ts) {
    // Finds the cost due to one net by looking at its coordinate bounding box.
    auto& cluster_ctx = g_vpr_ctx.clustering();

    const t_bb& bb = use_ts ? ts_bb_coord_new_[net_id] : placer_state_.move().bb_coords[net_id];

    const double crossing = wirelength_crossing_count(cluster_ctx.clb_nlist.net_pins(net_id).size());

    /* Could insert a check for xmin == xmax.  In that case, assume  *
     * connection will be made with no bends and hence no x-cost.    *
     * Same thing for y-cost.                                        */

    /* Cost = wire length along channel * cross_count / average      *
     * channel capacity.   Do this for x, then y direction and add.  */

    /* For average channel width factor, I'll always include the channel immediately
     * below and the channel immediately to the left of the bounding box, so both bb.ymin
     * and bb.xmin are subtracted by 1 before being used as indices of chan?_place_cost_fac_.
     * chan?_place_cost_fac_ objects can handle -1 indices internally.
     */

    double ncost;
    const auto [chanx_cost_fac, chany_cost_fac] = get_chanxy_cost_fac_(bb);
    ncost = (bb.xmax - bb.xmin + 1) * chanx_cost_fac;
    ncost += (bb.ymax - bb.ymin + 1) * chany_cost_fac;
    if (is_multi_layer_) {
        ncost += (bb.layer_max - bb.layer_min) * get_chanz_cost_factor_(bb);
    }

    ncost *= crossing;

    return ncost;
}


double NetCostHandler::get_net_per_layer_bb_cost_(ClusterNetId net_id , bool use_ts) {
    const auto& move_ctx = placer_state_.move();

    // Per-layer bounding box of the net
    const std::vector<t_2D_bb>& bb = use_ts ? layer_ts_bb_coord_new_[net_id] : move_ctx.layer_bb_coords[net_id];
    const vtr::NdMatrixProxy<int, 1> layer_pin_sink_count = use_ts ? ts_layer_sink_pin_count_[size_t(net_id)] : move_ctx.num_sink_pin_layer[size_t(net_id)];

    // Finds the cost due to one net by looking at its coordinate bounding box
    double ncost = 0.;
    int num_layers = g_vpr_ctx.device().grid.get_num_layers();



    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        VTR_ASSERT(layer_pin_sink_count[layer_num] != OPEN);
        if (layer_pin_sink_count[layer_num] == 0) {
            continue;
        }
        /* Adjust the bounding box half perimeter by the wirelength correction
         * factor based on terminal count, which is 1 for the source + the number
         * of sinks on this layer. */
        const double crossing = wirelength_crossing_count(layer_pin_sink_count[layer_num] + 1);

        /* Could insert a check for xmin == xmax.  In that case, assume  *
         * connection will be made with no bends and hence no x-cost.    *
         * Same thing for y-cost.                                        */

        /* Cost = wire length along channel * cross_count / average      *
         * channel capacity.   Do this for x, then y direction and add.  */

        /* For average channel width factor, I'll always include the channel immediately
         * below and the channel immediately to the left of the bounding box, so both bb.ymin
         * and bb.xmin are subtracted by 1 before being used as indices of chan?_place_cost_fac_.
         * chan?_place_cost_fac_ objects can handle -1 indices internally.
         */

        const auto[chanx_cost_fac, chany_cost_fac] = get_chanxy_cost_fac_(bb[layer_num]);
        ncost += (bb[layer_num].xmax - bb[layer_num].xmin + 1) * chanx_cost_fac;
        ncost += (bb[layer_num].ymax - bb[layer_num].ymin + 1) * chany_cost_fac;
        ncost *= crossing;
    }

    return ncost;
}

static double get_net_wirelength_estimate(ClusterNetId net_id, const t_bb& bb) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    double crossing = wirelength_crossing_count(cluster_ctx.clb_nlist.net_pins(net_id).size());

    /* Could insert a check for xmin == xmax.  In that case, assume  *
     * connection will be made with no bends and hence no x-cost.    *
     * Same thing for y-cost.                                        */

    /* Cost = wire length along channel * cross_count / average      *
     * channel capacity.   Do this for x, then y direction and add.  */

    double ncost;
    ncost = (bb.xmax - bb.xmin + 1) * crossing;
    ncost += (bb.ymax - bb.ymin + 1) * crossing;

    return ncost;
}

double NetCostHandler::get_net_wirelength_from_layer_bb_(ClusterNetId net_id) {
    /* WMF: Finds the estimate of wirelength due to one net by looking at   *
     * its coordinate bounding box.                                         */

    const auto& move_ctx = placer_state_.move();
    const std::vector<t_2D_bb>& bb = move_ctx.layer_bb_coords[net_id];
    const auto& layer_pin_sink_count = move_ctx.num_sink_pin_layer[size_t(net_id)];

    double ncost = 0.;
    const int num_layers = g_vpr_ctx.device().grid.get_num_layers();

    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        VTR_ASSERT_SAFE(layer_pin_sink_count[layer_num] != OPEN);
        if (layer_pin_sink_count[layer_num] == 0) {
            continue;
        }
        double crossing = wirelength_crossing_count(layer_pin_sink_count[layer_num] + 1);

        /* Could insert a check for xmin == xmax.  In that case, assume  *
         * connection will be made with no bends and hence no x-cost.    *
         * Same thing for y-cost.                                        */

        /* Cost = wire length along channel * cross_count / average      *
         * channel capacity.   Do this for x, then y direction and add.  */

        ncost += (bb[layer_num].xmax - bb[layer_num].xmin + 1) * crossing;
        ncost += (bb[layer_num].ymax - bb[layer_num].ymin + 1) * crossing;
    }

    return ncost;
}

float NetCostHandler::get_chanz_cost_factor_(const t_bb& bb) {
    int num_inter_dir_conn;

    if (bb.xmin == 0 && bb.ymin == 0) {
        num_inter_dir_conn = acc_tile_num_inter_die_conn_[bb.xmax][bb.ymax];
    } else if (bb.xmin == 0) {
        num_inter_dir_conn = acc_tile_num_inter_die_conn_[bb.xmax][bb.ymax] -
                             acc_tile_num_inter_die_conn_[bb.xmax][bb.ymin-1];
    } else if (bb.ymin == 0) {
        num_inter_dir_conn = acc_tile_num_inter_die_conn_[bb.xmax][bb.ymax] -
                             acc_tile_num_inter_die_conn_[bb.xmin-1][bb.ymax];
    } else {
        num_inter_dir_conn = acc_tile_num_inter_die_conn_[bb.xmax][bb.ymax] -
                             acc_tile_num_inter_die_conn_[bb.xmin-1][bb.ymax] -
                             acc_tile_num_inter_die_conn_[bb.xmax][bb.ymin-1] +
                             acc_tile_num_inter_die_conn_[bb.xmin-1][bb.ymin-1];
    }
    
    float z_cost_factor;
    if (num_inter_dir_conn == 0) {
        return 1.0f;
    } else {
        int bb_num_tiles = (bb.xmax - bb.xmin + 1) * (bb.ymax - bb.ymin + 1);
        z_cost_factor = bb_num_tiles / static_cast<float>(num_inter_dir_conn);
    }

    return z_cost_factor;

}

double NetCostHandler::recompute_bb_cost_() {
    double cost = 0;

    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (ClusterNetId net_id : cluster_ctx.clb_nlist.nets()) {       /* for each net ... */
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) { /* Do only if not ignored. */
            /* Bounding boxes don't have to be recomputed; they're correct. */
            cost += net_cost_[net_id];
        }
    }

    return cost;
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

void NetCostHandler::set_bb_delta_cost_(double& bb_delta_c) {
    for (const ClusterNetId ts_net: ts_nets_to_update_) {
        ClusterNetId net_id = ts_net;

        proposed_net_cost_[net_id] = get_net_bb_cost_functor_(net_id);

        bb_delta_c += proposed_net_cost_[net_id] - net_cost_[net_id];
    }
}

void NetCostHandler::find_affected_nets_and_update_costs(const PlaceDelayModel* delay_model,
                                                         const PlacerCriticalities* criticalities,
                                                         t_pl_blocks_to_be_moved& blocks_affected,
                                                         double& bb_delta_c,
                                                         double& timing_delta_c) {
    VTR_ASSERT_SAFE(bb_delta_c == 0.);
    VTR_ASSERT_SAFE(timing_delta_c == 0.);
    auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    ts_nets_to_update_.resize(0);

    /* Go through all the blocks moved. */
    for (const t_pl_moved_block& moving_block : blocks_affected.moved_blocks) {
        auto& affected_pins = blocks_affected.affected_pins;
        ClusterBlockId blk_id = moving_block.block_num;

        /* Go through all the pins in the moved block. */
        for (ClusterPinId blk_pin : clb_nlist.block_pins(blk_id)) {
            bool is_src_moving = false;
            if (clb_nlist.pin_type(blk_pin) == PinType::SINK) {
                ClusterNetId net_id = clb_nlist.pin_net(blk_pin);
                is_src_moving = blocks_affected.driven_by_moved_block(net_id);
            }
            update_net_info_on_pin_move_(delay_model,
                                         criticalities,
                                         blk_pin,
                                         moving_block,
                                         affected_pins,
                                         timing_delta_c,
                                         is_src_moving);
        }
    }

    /* Now update the bounding box costs (since the net bounding     *
     * boxes are up-to-date). The cost is only updated once per net. */
    set_bb_delta_cost_(bb_delta_c);
}

void NetCostHandler::update_move_nets() {
    /* update net cost functions and reset flags. */
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_move_ctx = placer_state_.mutable_move();

    for (const ClusterNetId ts_net : ts_nets_to_update_) {
        ClusterNetId net_id = ts_net;

        set_ts_bb_coord_(net_id);

        for (int layer_num = 0; layer_num < g_vpr_ctx.device().grid.get_num_layers(); layer_num++) {
            place_move_ctx.num_sink_pin_layer[size_t(net_id)][layer_num] = ts_layer_sink_pin_count_[size_t(net_id)][layer_num];
        }

        if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= SMALL_NET) {
            set_ts_edge_(net_id);
        }

        net_cost_[net_id] = proposed_net_cost_[net_id];

        /* negative proposed_net_cost value is acting as a flag to mean not computed yet. */
        proposed_net_cost_[net_id] = -1;
        bb_update_status_[net_id] = NetUpdateState::NOT_UPDATED_YET;
    }
}

void NetCostHandler::reset_move_nets() {
    /* Reset the net cost function flags first. */
    for (const ClusterNetId ts_net : ts_nets_to_update_) {
        ClusterNetId net_id = ts_net;
        proposed_net_cost_[net_id] = -1;
        bb_update_status_[net_id] = NetUpdateState::NOT_UPDATED_YET;
    }
}

void NetCostHandler::recompute_costs_from_scratch(const PlaceDelayModel* delay_model,
                                                  const PlacerCriticalities* criticalities,
                                                  t_placer_costs& costs) {
    auto check_and_print_cost = [](double new_cost,
                                   double old_cost,
                                   const std::string& cost_name) -> void {
        if (!vtr::isclose(new_cost, old_cost, PL_INCREMENTAL_COST_TOLERANCE, 0.)) {
            std::string msg = vtr::string_fmt(
                "in recompute_costs_from_scratch: new_%s = %g, old %s = %g, ERROR_TOL = %g\n",
                cost_name.c_str(), new_cost, cost_name.c_str(), old_cost, PL_INCREMENTAL_COST_TOLERANCE);
            VPR_ERROR(VPR_ERROR_PLACE, msg.c_str());
        }
    };

    double new_bb_cost = recompute_bb_cost_();
    check_and_print_cost(new_bb_cost, costs.bb_cost, "bb_cost");
    costs.bb_cost = new_bb_cost;

    if (placer_opts_.place_algorithm.is_timing_driven()) {
        double new_timing_cost = 0.;
        comp_td_costs(delay_model, *criticalities, placer_state_, &new_timing_cost);
        check_and_print_cost(new_timing_cost, costs.timing_cost, "timing_cost");
        costs.timing_cost = new_timing_cost;
    } else {
        VTR_ASSERT(placer_opts_.place_algorithm == e_place_algorithm::BOUNDING_BOX_PLACE);
        costs.cost = new_bb_cost * costs.bb_cost_norm;
    }
}

void NetCostHandler::set_ts_bb_coord_(const ClusterNetId net_id) {
    auto& place_move_ctx = placer_state_.mutable_move();
    if (cube_bb_) {
        place_move_ctx.bb_coords[net_id] = ts_bb_coord_new_[net_id];
    } else {
        place_move_ctx.layer_bb_coords[net_id] = layer_ts_bb_coord_new_[net_id];
    }
}

void NetCostHandler::set_ts_edge_(const ClusterNetId net_id) {
    auto& place_move_ctx = placer_state_.mutable_move();
    if (cube_bb_) {
        place_move_ctx.bb_num_on_edges[net_id] = ts_bb_edge_new_[net_id];
    } else {
        place_move_ctx.layer_bb_num_on_edges[net_id] = layer_ts_bb_edge_new_[net_id];
    }
}
