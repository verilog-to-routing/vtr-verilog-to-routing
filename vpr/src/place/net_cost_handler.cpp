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
#include "PlacerCriticalities.h"
#include "vtr_prefix_sum.h"
#include "stats.h"

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
        bb_coords_.resize(num_nets, t_bb());
        bb_num_on_edges_.resize(num_nets, t_bb());
        comp_bb_cost_functor_ = std::bind(&NetCostHandler::comp_cube_bb_cost_, this, std::placeholders::_1);
        update_bb_functor_ = std::bind(&NetCostHandler::update_bb_, this, std::placeholders::_1, std::placeholders::_2,
                                       std::placeholders::_3, std::placeholders::_4);
        get_net_bb_cost_functor_ = std::bind(&NetCostHandler::get_net_cube_bb_cost_, this, std::placeholders::_1, /*use_ts=*/true);
        get_non_updatable_bb_functor_ = std::bind(&NetCostHandler::get_non_updatable_cube_bb_, this, std::placeholders::_1, /*use_ts=*/true);
    } else {
        layer_ts_bb_edge_new_.resize(num_nets, std::vector<t_2D_bb>(num_layers, t_2D_bb()));
        layer_ts_bb_coord_new_.resize(num_nets, std::vector<t_2D_bb>(num_layers, t_2D_bb()));
        layer_bb_num_on_edges_.resize(num_nets, std::vector<t_2D_bb>(num_layers, t_2D_bb()));
        layer_bb_coords_.resize(num_nets, std::vector<t_2D_bb>(num_layers, t_2D_bb()));
        comp_bb_cost_functor_ = std::bind(&NetCostHandler::comp_per_layer_bb_cost_, this, std::placeholders::_1);
        update_bb_functor_ = std::bind(&NetCostHandler::update_layer_bb_, this, std::placeholders::_1, std::placeholders::_2,
                                       std::placeholders::_3, std::placeholders::_4);
        get_net_bb_cost_functor_ = std::bind(&NetCostHandler::get_net_per_layer_bb_cost_, this, std::placeholders::_1, /*use_ts=*/true);
        get_non_updatable_bb_functor_ = std::bind(&NetCostHandler::get_non_updatable_per_layer_bb_, this, std::placeholders::_1, /*use_ts=*/true);
    }

    /* This initializes the whole matrix to OPEN which is an invalid value*/
    ts_layer_sink_pin_count_.resize({num_nets, size_t(num_layers)}, UNDEFINED);
    num_sink_pin_layer_.resize({num_nets, size_t(num_layers)}, UNDEFINED);

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

    const size_t grid_height = device_ctx.grid.height();
    const size_t grid_width = device_ctx.grid.width();

    // These arrays contain accumulative channel width between channel zero and
    // the channel specified by the given index. The accumulated channel width
    // is inclusive, meaning that it includes both channel zero and channel `idx`.
    // To compute the total channel width between channels 'low' and 'high', use the
    // following formula:
    //      acc_chan?_width_[high] - acc_chan?_width_[low - 1]
    // This returns the total number of tracks between channels 'low' and 'high',
    // including tracks in these channels.
    acc_chanx_width_ = vtr::PrefixSum1D<int>(grid_height, [&](size_t y) noexcept {
        int chan_x_width = device_ctx.rr_chanx_width[y];

        // If the number of tracks in a channel is zero, two consecutive elements take the same
        // value. This can lead to a division by zero in get_chanxy_cost_fac_(). To avoid this
        // potential issue, we assume that the channel width is at least 1.
        if (chan_x_width == 0) {
            return 1;
        }

        return chan_x_width;
    });

    acc_chany_width_ = vtr::PrefixSum1D<int>(grid_width, [&](size_t x) noexcept {
        int chan_y_width = device_ctx.rr_chany_width[x];

        // to avoid a division by zero
        if (chan_y_width == 0) {
            return 1;
        }

        return chan_y_width;
    });

    if (is_multi_layer_) {
        alloc_and_load_for_fast_vertical_cost_update_();
    }
}

void NetCostHandler::alloc_and_load_for_fast_vertical_cost_update_() {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    const size_t grid_height = device_ctx.grid.height();
    const size_t grid_width = device_ctx.grid.width();

    vtr::NdMatrix<float, 2> tile_num_inter_die_conn({grid_width, grid_height}, 0.);

    /*
     * Step 1: iterate over the rr-graph, recording how many edges go between layers at each (x,y) location
     * in the device. We count all these edges, regardless of which layers they connect. Then we divide by
     * the number of layers - 1 to get the average cross-layer edge count per (x,y) location -- this mirrors
     * what we do for the horizontal and vertical channels where we assume the channel width doesn't change
     * along the length of the channel. It lets us be more memory-efficient for 3D devices, and could be revisited
     * if someday we have architectures with widely varying connectivity between different layers in a stack.
     */

    /* To calculate the accumulative number of inter-die connections we first need to get the number of
     * inter-die connection per location. To be able to work for the cases that RR Graph is read instead
     * of being made from the architecture file, we calculate this number by iterating over the RR graph. Once
     * tile_num_inter_die_conn is populated, we can start populating acc_tile_num_inter_die_conn_.
     */

    for (const RRNodeId node : rr_graph.nodes()) {
        if (rr_graph.node_type(node) == e_rr_type::CHANZ) {
            int x = rr_graph.node_xlow(node);
            int y = rr_graph.node_ylow(node);
            VTR_ASSERT_SAFE(x == rr_graph.node_xhigh(node) && y == rr_graph.node_yhigh(node));
            tile_num_inter_die_conn[x][y]++;
        }
    }

    int num_layers = device_ctx.grid.get_num_layers();
    for (size_t x = 0; x < device_ctx.grid.width(); x++) {
        for (size_t y = 0; y < device_ctx.grid.height(); y++) {
            tile_num_inter_die_conn[x][y] /= (num_layers - 1);
        }
    }

    // Step 2: Calculate prefix sum of the inter-die connectivity up to and including the channel at (x, y).
    acc_tile_num_inter_die_conn_ = vtr::PrefixSum2D<int>(grid_width,
                                                         grid_height,
                                                         [&](size_t x, size_t y) {
                                                             return (int)tile_num_inter_die_conn[x][y];
                                                         });
}

std::pair<double, double> NetCostHandler::comp_bb_cost(e_cost_methods method) {
    return comp_bb_cost_functor_(method);
}

std::pair<double, double> NetCostHandler::comp_cube_bb_cost_(e_cost_methods method) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    double cost = 0;
    double expected_wirelength = 0.0;

    for (ClusterNetId net_id : cluster_ctx.clb_nlist.nets()) { /* for each net ... */
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) {   /* Do only if not ignored. */
            /* Small nets don't use incremental updating on their bounding boxes, *
             * so they can use a fast bounding box calculator.                    */
            if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= SMALL_NET && method == e_cost_methods::NORMAL) {
                get_bb_from_scratch_(net_id, /*use_ts=*/false);
            } else {
                get_non_updatable_cube_bb_(net_id, /*use_ts=*/false);
            }

            net_cost_[net_id] = get_net_cube_bb_cost_(net_id, /*use_ts=*/false);
            cost += net_cost_[net_id];
            if (method == e_cost_methods::CHECK) {
                expected_wirelength += get_net_wirelength_estimate_(net_id);
            }
        }
    }

    return {cost, expected_wirelength};
}

std::pair<double, double> NetCostHandler::comp_per_layer_bb_cost_(e_cost_methods method) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    double cost = 0;
    double expected_wirelength = 0.0;

    for (ClusterNetId net_id : cluster_ctx.clb_nlist.nets()) { /* for each net ... */
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) {   /* Do only if not ignored. */
            /* Small nets don't use incremental updating on their bounding boxes, *
             * so they can use a fast bounding box calculator.                    */
            if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= SMALL_NET && method == e_cost_methods::NORMAL) {
                get_layer_bb_from_scratch_(net_id,
                                           layer_bb_num_on_edges_[net_id],
                                           layer_bb_coords_[net_id],
                                           num_sink_pin_layer_[size_t(net_id)]);
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

    return {cost, expected_wirelength};
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

    // Record effected nets
    record_affected_net_(net_id);

    ClusterBlockId blk_id = moving_blk_inf.block_num;
    // Update the net bounding boxes.
    update_net_bb_(net_id, blk_id, pin_id, moving_blk_inf);

    if (placer_opts_.place_algorithm.is_timing_driven()) {
        // Determine the change in connection delay and timing cost.
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

    // the bounding box coordinates that is going to be updated by this function
    t_bb& bb_coord_new = use_ts ? ts_bb_coord_new_[net_id] : bb_coords_[net_id];
    // the number of sink pins of "net_id" on each layer
    vtr::NdMatrixProxy<int, 1> num_sink_pin_layer = use_ts ? ts_layer_sink_pin_count_[size_t(net_id)] : num_sink_pin_layer_[size_t(net_id)];

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

    for (size_t layer_num = 0; layer_num < device_ctx.grid.get_num_layers(); layer_num++) {
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

    std::vector<t_2D_bb>& bb_coord_new = use_ts ? layer_ts_bb_coord_new_[net_id] : layer_bb_coords_[net_id];
    vtr::NdMatrixProxy<int, 1> num_sink_layer = use_ts ? ts_layer_sink_pin_count_[size_t(net_id)] : num_sink_pin_layer_[size_t(net_id)];

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

    const auto& device_ctx = g_vpr_ctx.device();

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

    vtr::NdMatrixProxy<int, 1> curr_num_sink_pin_layer = (bb_update_status_[net_id] == NetUpdateState::NOT_UPDATED_YET) ? num_sink_pin_layer_[size_t(net_id)] : num_sink_pin_layer_new;

    if (bb_update_status_[net_id] == NetUpdateState::NOT_UPDATED_YET) {
        /* The net had NOT been updated before, could use the old values */
        curr_bb_edge = &bb_num_on_edges_[net_id];
        curr_bb_coord = &bb_coords_[net_id];
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
                get_bb_from_scratch_(net_id, /*use_ts=*/true);
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
                get_bb_from_scratch_(net_id, /*use_ts=*/true);
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
                get_bb_from_scratch_(net_id, /*use_ts=*/true);
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
                get_bb_from_scratch_(net_id, /*use_ts=*/true);
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
                    get_bb_from_scratch_(net_id, /*use_ts=*/true);
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
                    get_bb_from_scratch_(net_id, /*use_ts=*/true);
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
    std::vector<t_2D_bb>& bb_edge_new = layer_ts_bb_edge_new_[net_id];
    std::vector<t_2D_bb>& bb_coord_new = layer_ts_bb_coord_new_[net_id];
    vtr::NdMatrixProxy<int, 1> bb_pin_sink_count_new = ts_layer_sink_pin_count_[size_t(net_id)];

    /* Check if the net had been updated before. */
    if (bb_update_status_[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
        /* The net had been updated from scratch, DO NOT update again! */
        return;
    }

    const vtr::NdMatrixProxy<int, 1> curr_layer_pin_sink_count = (bb_update_status_[net_id] == NetUpdateState::NOT_UPDATED_YET) ? num_sink_pin_layer_[size_t(net_id)] : bb_pin_sink_count_new;

    const std::vector<t_2D_bb>*curr_bb_edge, *curr_bb_coord;
    if (bb_update_status_[net_id] == NetUpdateState::NOT_UPDATED_YET) {
        /* The net had NOT been updated before, could use the old values */
        curr_bb_edge = &layer_bb_num_on_edges_[net_id];
        curr_bb_coord = &layer_bb_coords_[net_id];
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
     * This funcitn is called when BB per layer is used and when the moving block is moving from one layer to another.
     * Thus, we need to update bounding box on both "from" and "to" layer. Here, we update the bounding box on "from" or
     * "old_layer". Then, "add_block_to_bb" is called to update the bounding box on the new layer.
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

    for (size_t layer_num = 0; layer_num < g_vpr_ctx.device().grid.get_num_layers(); layer_num++) {
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
     * This function is called to only update the bounding box on the new layer from a block
     * moving to this layer from another layer. Thus, we only need to assess the effect of this
     * new block on the edges.
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

void NetCostHandler::get_bb_from_scratch_(ClusterNetId net_id, bool use_ts) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& grid = device_ctx.grid;
    const auto& blk_loc_registry = placer_state_.blk_loc_registry();

    t_bb& coords = use_ts ? ts_bb_coord_new_[net_id] : bb_coords_[net_id];
    t_bb& num_on_edges = use_ts ? ts_bb_edge_new_[net_id] : bb_num_on_edges_[net_id];
    vtr::NdMatrixProxy<int, 1> num_sink_pin_layer = use_ts ? ts_layer_sink_pin_count_[(size_t)net_id] : num_sink_pin_layer_[(size_t)net_id];

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

    for (size_t layer_num = 0; layer_num < grid.get_num_layers(); layer_num++) {
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
    VTR_ASSERT_DEBUG(layer_min >= 0 && layer_min < (int)device_ctx.grid.get_num_layers());
    VTR_ASSERT_DEBUG(layer_max >= 0 && layer_max < (int)device_ctx.grid.get_num_layers());

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

    const t_bb& bb = use_ts ? ts_bb_coord_new_[net_id] : bb_coords_[net_id];

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

double NetCostHandler::get_net_per_layer_bb_cost_(ClusterNetId net_id, bool use_ts) {
    // Per-layer bounding box of the net
    const std::vector<t_2D_bb>& bb = use_ts ? layer_ts_bb_coord_new_[net_id] : layer_bb_coords_[net_id];
    const vtr::NdMatrixProxy<int, 1> layer_pin_sink_count = use_ts ? ts_layer_sink_pin_count_[size_t(net_id)] : num_sink_pin_layer_[size_t(net_id)];

    // Finds the cost due to one net by looking at its coordinate bounding box
    double ncost = 0.;
    int num_layers = g_vpr_ctx.device().grid.get_num_layers();

    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        VTR_ASSERT(layer_pin_sink_count[layer_num] != UNDEFINED);
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

        const auto [chanx_cost_fac, chany_cost_fac] = get_chanxy_cost_fac_(bb[layer_num]);
        ncost += (bb[layer_num].xmax - bb[layer_num].xmin + 1) * chanx_cost_fac;
        ncost += (bb[layer_num].ymax - bb[layer_num].ymin + 1) * chany_cost_fac;
        ncost *= crossing;
    }

    return ncost;
}

double NetCostHandler::get_net_wirelength_estimate_(ClusterNetId net_id) const {
    const t_bb& bb = bb_coords_[net_id];
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

double NetCostHandler::get_net_wirelength_from_layer_bb_(ClusterNetId net_id) const {
    const std::vector<t_2D_bb>& bb = layer_bb_coords_[net_id];
    const vtr::NdMatrixProxy<int, 1> net_layer_pin_sink_count = num_sink_pin_layer_[size_t(net_id)];

    double ncost = 0.;
    VTR_ASSERT_SAFE(bb.size() == g_vpr_ctx.device().grid.get_num_layers());

    for (size_t layer_num = 0; layer_num < bb.size(); layer_num++) {
        VTR_ASSERT_SAFE(net_layer_pin_sink_count[layer_num] != UNDEFINED);
        if (net_layer_pin_sink_count[layer_num] == 0) {
            continue;
        }

        // The reason we add 1 to the number of sink pins is because when per-layer bounding box is used,
        // we want to get the estimated wirelength of the given layer assuming that the source pin is
        // also on that layer
        double crossing = wirelength_crossing_count(net_layer_pin_sink_count[layer_num] + 1);

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
    int num_inter_dir_conn = acc_tile_num_inter_die_conn_.get_sum(bb.xmin,
                                                                  bb.ymin,
                                                                  bb.xmax,
                                                                  bb.ymax);

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

    for (ClusterNetId net_id : cluster_ctx.clb_nlist.nets()) { /* for each net ... */
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) {   /* Do only if not ignored. */
            /* Bounding boxes don't have to be recomputed; they're correct. */
            cost += net_cost_[net_id];
        }
    }

    return cost;
}

double wirelength_crossing_count(size_t fanout) {
    /* Get the expected "crossing count" of a net, based on its number *
     * of pins.  Extrapolate for very large nets.                      */

    if (fanout > MAX_FANOUT_CROSSING_COUNT) {
        return 2.7933 + 0.02616 * (fanout - MAX_FANOUT_CROSSING_COUNT);
    } else {
        return cross_count[fanout - 1];
    }
}

void NetCostHandler::set_bb_delta_cost_(double& bb_delta_c) {
    for (const ClusterNetId ts_net : ts_nets_to_update_) {
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

    for (const ClusterNetId ts_net : ts_nets_to_update_) {
        ClusterNetId net_id = ts_net;

        set_ts_bb_coord_(net_id);

        for (size_t layer_num = 0; layer_num < g_vpr_ctx.device().grid.get_num_layers(); layer_num++) {
            num_sink_pin_layer_[size_t(net_id)][layer_num] = ts_layer_sink_pin_count_[size_t(net_id)][layer_num];
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

double NetCostHandler::get_total_wirelength_estimate() const {
    const auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    double estimated_wirelength = 0.0;
    for (ClusterNetId net_id : clb_nlist.nets()) { /* for each net ... */
        if (!clb_nlist.net_is_ignored(net_id)) {   /* Do only if not ignored. */
            if (cube_bb_) {
                estimated_wirelength += get_net_wirelength_estimate_(net_id);
            } else {
                estimated_wirelength += get_net_wirelength_from_layer_bb_(net_id);
            }
        }
    }

    return estimated_wirelength;
}

void NetCostHandler::set_ts_bb_coord_(const ClusterNetId net_id) {
    if (cube_bb_) {
        bb_coords_[net_id] = ts_bb_coord_new_[net_id];
    } else {
        layer_bb_coords_[net_id] = layer_ts_bb_coord_new_[net_id];
    }
}

void NetCostHandler::set_ts_edge_(const ClusterNetId net_id) {
    if (cube_bb_) {
        bb_num_on_edges_[net_id] = ts_bb_edge_new_[net_id];
    } else {
        layer_bb_num_on_edges_[net_id] = layer_ts_bb_edge_new_[net_id];
    }
}

t_bb NetCostHandler::union_2d_bb(ClusterNetId net_id) const {
    t_bb merged_bb;
    const std::vector<t_2D_bb>& bb_vec = layer_bb_coords_[net_id];

    // Not all 2d_bbs are valid. Thus, if one of the coordinates in the 2D_bb is not valid (equal to UNDEFINED),
    // we need to skip it.
    for (const t_2D_bb& layer_bb : bb_vec) {
        if (layer_bb.xmin == UNDEFINED) {
            VTR_ASSERT_DEBUG(layer_bb.xmax == UNDEFINED);
            VTR_ASSERT_DEBUG(layer_bb.ymin == UNDEFINED);
            VTR_ASSERT_DEBUG(layer_bb.ymax == UNDEFINED);
            VTR_ASSERT_DEBUG(layer_bb.layer_num == UNDEFINED);
            continue;
        }
        if (merged_bb.xmin == UNDEFINED || layer_bb.xmin < merged_bb.xmin) {
            merged_bb.xmin = layer_bb.xmin;
        }
        if (merged_bb.xmax == UNDEFINED || layer_bb.xmax > merged_bb.xmax) {
            merged_bb.xmax = layer_bb.xmax;
        }
        if (merged_bb.ymin == UNDEFINED || layer_bb.ymin < merged_bb.ymin) {
            merged_bb.ymin = layer_bb.ymin;
        }
        if (merged_bb.ymax == UNDEFINED || layer_bb.ymax > merged_bb.ymax) {
            merged_bb.ymax = layer_bb.ymax;
        }
        if (merged_bb.layer_min == UNDEFINED || layer_bb.layer_num < merged_bb.layer_min) {
            merged_bb.layer_min = layer_bb.layer_num;
        }
        if (merged_bb.layer_max == UNDEFINED || layer_bb.layer_num > merged_bb.layer_max) {
            merged_bb.layer_max = layer_bb.layer_num;
        }
    }

    return merged_bb;
}

std::pair<t_bb, t_bb> NetCostHandler::union_2d_bb_incr(ClusterNetId net_id) const {
    t_bb merged_num_edge;
    t_bb merged_bb;

    const std::vector<t_2D_bb>& num_edge_vec = layer_bb_num_on_edges_[net_id];
    const std::vector<t_2D_bb>& bb_vec = layer_bb_coords_[net_id];

    for (const t_2D_bb& layer_bb : bb_vec) {
        if (layer_bb.xmin == UNDEFINED) {
            VTR_ASSERT_SAFE(layer_bb.xmax == UNDEFINED);
            VTR_ASSERT_SAFE(layer_bb.ymin == UNDEFINED);
            VTR_ASSERT_SAFE(layer_bb.ymax == UNDEFINED);
            VTR_ASSERT_SAFE(layer_bb.layer_num == UNDEFINED);
            continue;
        }
        if (merged_bb.xmin == UNDEFINED || layer_bb.xmin <= merged_bb.xmin) {
            if (layer_bb.xmin == merged_bb.xmin) {
                VTR_ASSERT_SAFE(merged_num_edge.xmin != UNDEFINED);
                merged_num_edge.xmin += num_edge_vec[layer_bb.layer_num].xmin;
            } else {
                merged_num_edge.xmin = num_edge_vec[layer_bb.layer_num].xmin;
            }
            merged_bb.xmin = layer_bb.xmin;
        }
        if (merged_bb.xmax == UNDEFINED || layer_bb.xmax >= merged_bb.xmax) {
            if (layer_bb.xmax == merged_bb.xmax) {
                VTR_ASSERT_SAFE(merged_num_edge.xmax != UNDEFINED);
                merged_num_edge.xmax += num_edge_vec[layer_bb.layer_num].xmax;
            } else {
                merged_num_edge.xmax = num_edge_vec[layer_bb.layer_num].xmax;
            }
            merged_bb.xmax = layer_bb.xmax;
        }
        if (merged_bb.ymin == UNDEFINED || layer_bb.ymin <= merged_bb.ymin) {
            if (layer_bb.ymin == merged_bb.ymin) {
                VTR_ASSERT_SAFE(merged_num_edge.ymin != UNDEFINED);
                merged_num_edge.ymin += num_edge_vec[layer_bb.layer_num].ymin;
            } else {
                merged_num_edge.ymin = num_edge_vec[layer_bb.layer_num].ymin;
            }
            merged_bb.ymin = layer_bb.ymin;
        }
        if (merged_bb.ymax == UNDEFINED || layer_bb.ymax >= merged_bb.ymax) {
            if (layer_bb.ymax == merged_bb.ymax) {
                VTR_ASSERT_SAFE(merged_num_edge.ymax != UNDEFINED);
                merged_num_edge.ymax += num_edge_vec[layer_bb.layer_num].ymax;
            } else {
                merged_num_edge.ymax = num_edge_vec[layer_bb.layer_num].ymax;
            }
            merged_bb.ymax = layer_bb.ymax;
        }
        if (merged_bb.layer_min == UNDEFINED || layer_bb.layer_num <= merged_bb.layer_min) {
            if (layer_bb.layer_num == merged_bb.layer_min) {
                VTR_ASSERT_SAFE(merged_num_edge.layer_min != UNDEFINED);
                merged_num_edge.layer_min += num_edge_vec[layer_bb.layer_num].layer_num;
            } else {
                merged_num_edge.layer_min = num_edge_vec[layer_bb.layer_num].layer_num;
            }
            merged_bb.layer_min = layer_bb.layer_num;
        }
        if (merged_bb.layer_max == UNDEFINED || layer_bb.layer_num >= merged_bb.layer_max) {
            if (layer_bb.layer_num == merged_bb.layer_max) {
                VTR_ASSERT_SAFE(merged_num_edge.layer_max != UNDEFINED);
                merged_num_edge.layer_max += num_edge_vec[layer_bb.layer_num].layer_num;
            } else {
                merged_num_edge.layer_max = num_edge_vec[layer_bb.layer_num].layer_num;
            }
            merged_bb.layer_max = layer_bb.layer_num;
        }
    }

    return std::make_pair(merged_num_edge, merged_bb);
}

std::pair<vtr::NdMatrix<double, 3>, vtr::NdMatrix<double, 3>> NetCostHandler::estimate_routing_chan_util() const {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& device_ctx = g_vpr_ctx.device();

    auto chanx_util = vtr::NdMatrix<double, 3>({{(size_t)device_ctx.grid.get_num_layers(),
                                                 device_ctx.grid.width(),
                                                 device_ctx.grid.height()}},
                                               0);

    auto chany_util = vtr::NdMatrix<double, 3>({{(size_t)device_ctx.grid.get_num_layers(),
                                                 device_ctx.grid.width(),
                                                 device_ctx.grid.height()}},
                                               0);

    // For each net, this function estimates routing channel utilization by distributing
    // the net's expected wirelength across its bounding box. The expected wirelength
    // for each dimension (x, y) is computed proportionally based on the bounding box size
    // in each direction. The wirelength in each dimension is then **evenly spread** across
    // all grid locations within the bounding box, and the demand is accumulated in
    // the channel utilization matrices.

    for (ClusterNetId net_id : cluster_ctx.clb_nlist.nets()) {
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) {

            if (cube_bb_) {
                const t_bb& bb = bb_coords_[net_id];
                double expected_wirelength = get_net_wirelength_estimate_(net_id);

                int distance_x = bb.xmax - bb.xmin + 1;
                int distance_y = bb.ymax - bb.ymin + 1;
                int distance_z = bb.layer_max - bb.layer_min + 1;

                double expected_x_wl = (double)distance_x / (distance_x + distance_y) * expected_wirelength;
                double expected_y_wl = expected_wirelength - expected_x_wl;

                int total_channel_segments = distance_x * distance_y * distance_z;
                double expected_per_x_segment_wl = expected_x_wl / total_channel_segments;
                double expected_per_y_segment_wl = expected_y_wl / total_channel_segments;

                for (int layer = bb.layer_min; layer <= bb.layer_max; layer++) {
                    for (int x = bb.xmin; x <= bb.xmax; x++) {
                        for (int y = bb.ymin; y <= bb.ymax; y++) {
                            chanx_util[layer][x][y] += expected_per_x_segment_wl;
                            chany_util[layer][x][y] += expected_per_y_segment_wl;
                        }
                    }
                }
            } else {
                const std::vector<t_2D_bb>& bb = layer_bb_coords_[net_id];
                const vtr::NdMatrixProxy<int, 1> net_layer_pin_sink_count = num_sink_pin_layer_[size_t(net_id)];

                for (size_t layer = 0; layer < bb.size(); layer++) {
                    if (net_layer_pin_sink_count[layer] == 0) {
                        continue;
                    }

                    double crossing = wirelength_crossing_count(net_layer_pin_sink_count[layer] + 1);
                    double expected_wirelength = ((bb[layer].xmax - bb[layer].xmin + 1) + (bb[layer].ymax - bb[layer].ymin + 1)) * crossing;

                    int distance_x = bb[layer].xmax - bb[layer].xmin + 1;
                    int distance_y = bb[layer].ymax - bb[layer].ymin + 1;

                    double expected_x_wl = (double)distance_x / (distance_x + distance_y) * expected_wirelength;
                    double expected_y_wl = expected_wirelength - expected_x_wl;

                    int total_channel_segments = distance_x * distance_y;
                    double expected_per_x_segment_wl = expected_x_wl / total_channel_segments;
                    double expected_per_y_segment_wl = expected_y_wl / total_channel_segments;

                    for (int x = bb[layer].xmin; x <= bb[layer].xmax; x++) {
                        for (int y = bb[layer].ymin; y <= bb[layer].ymax; y++) {
                            chanx_util[layer][x][y] += expected_per_x_segment_wl;
                            chany_util[layer][x][y] += expected_per_y_segment_wl;
                        }
                    }
                }
            }
        }
    }

    const vtr::NdMatrix<int, 3>& chanx_width = device_ctx.rr_chanx_segment_width;
    const vtr::NdMatrix<int, 3>& chany_width = device_ctx.rr_chany_segment_width;

    VTR_ASSERT(chanx_util.size() == chany_util.size());
    VTR_ASSERT(chanx_util.ndims() == chany_util.ndims());
    VTR_ASSERT(chanx_util.size() == chanx_width.size());
    VTR_ASSERT(chanx_util.ndims() == chanx_width.ndims());
    VTR_ASSERT(chany_util.size() == chany_width.size());
    VTR_ASSERT(chany_util.ndims() == chany_width.ndims());

    for (size_t layer = 0; layer < chanx_util.dim_size(0); ++layer) {
        for (size_t x = 0; x < chanx_util.dim_size(1); ++x) {
            for (size_t y = 0; y < chanx_util.dim_size(2); ++y) {
                if (chanx_width[layer][x][y] > 0) {
                    chanx_util[layer][x][y] /= chanx_width[layer][x][y];
                } else {
                    VTR_ASSERT_SAFE(chanx_width[layer][x][y] == 0);
                    chanx_util[layer][x][y] = 1.;
                }

                if (chany_width[layer][x][y] > 0) {
                    chany_util[layer][x][y] /= chany_width[layer][x][y];
                } else {
                    VTR_ASSERT_SAFE(chany_width[layer][x][y] == 0);
                    chany_util[layer][x][y] = 1.;
                }
            }
        }
    }

    return {chanx_util, chany_util};
}
