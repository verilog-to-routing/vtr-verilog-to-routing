#pragma once

#include "rr_node_types.h"
#include "vtr_ndmatrix.h"

#include "net_cost_handler.h"
#include "placer_state.h"

/**
 * @class RoutingChanUtilEstimator
 * @brief This class computes the net bounding boxes (cube mode) and estimates the routing channel utilization
 * for each CHANX/CHANY channel by smearing the estimated wirelength for each net across all channels
 * within its bounding box.
 */
class RoutingChanUtilEstimator {
  public:
    RoutingChanUtilEstimator(const BlkLocRegistry& blk_loc_registry);

    std::pair<vtr::NdMatrix<double, 3>, vtr::NdMatrix<double, 3>> estimate_routing_chan_util();

  private:
    std::unique_ptr<PlacerState> placer_state_;
    std::unique_ptr<NetCostHandler> net_cost_handler_;
    t_placer_opts placer_opts_;
};

vtr::Matrix<float> calculate_routing_avail(e_rr_type rr_type);

/**
 * @brief: Calculates and returns the usage over the entire grid for the specified
 * type of rr_node to the usage array. The usage is recorded at each (x,y) location.
 *
 * @param rr_type: Type of rr_node that we are calculating the usage of; can be CHANX or CHANY
 * @param is_flat: Is the flat router being used or not?
 * @param only_visible: If true, only record the usage of rr_nodes on layers that are visible according to the current
 * drawing settings.
 */
vtr::Matrix<float> calculate_routing_usage(e_rr_type rr_type, bool is_flat, bool is_print);

float routing_util(float used, float avail);
