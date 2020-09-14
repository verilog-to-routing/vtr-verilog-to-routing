#ifndef EXTENDED_MAP_LOOKAHEAD_H_
#define EXTENDED_MAP_LOOKAHEAD_H_

#include <vector>
#include "physical_types.h"
#include "router_lookahead.h"
#include "router_lookahead_map_utils.h"
#include "router_lookahead_cost_map.h"
#include "vtr_geometry.h"

// Implementation of RouterLookahead based on source segment and destination connection box types
class ExtendedMapLookahead : public RouterLookahead {
  private:
    ///<Look-up table from SOURCE/OPIN to CHANX/CHANY of various types
    util::t_src_opin_delays src_opin_delays;

    ///<Look-up table from CHANX/CHANY to SINK/IPIN of various types
    util::t_chan_ipins_delays chan_ipins_delays;

    /**
     * @brief Returns the cost to get to a input delta_x/y location when starting from a SOURCE or OPIN node
     * @param from_node starting node
     * @param delta_x x delta value that is the distance between the current node to the target node
     * @param delta_y y delta value that is the distance between the current node to the target node
     * @param criticality_fac criticality of the current connection between 0 (all congestion) and 1 (all timing)
     * @return expected cost to get to the destination
     */
    std::pair<float, float> get_src_opin_cost(RRNodeId from_node, int delta_x, int delta_y, const t_conn_cost_params& params) const;

    /**
     * @brief Returns the CHAN -> IPIN delay that gets added to the final expected delay
     * @param to_node target node for which we query the IPIN delay
     * @return IPIN delay relative to the input destination node
     *
     * The CHAN -> IPIN delay was removed from the cost map calculation, as it might tamper the addition
     * of a smaller cost to this node location. This might happen as not all the CHAN -> IPIN connection
     * have a delay, therefore, a different cost than the correct one might have been added to the cost
     * map location of the input node.
     * 
     * The CHAN -> IPIN delay gets re-added to the final calculation as it effectively is a valid delay
     * to reach the destination.
     */
    float get_chan_ipin_delays(RRNodeId to_node) const;

    template<typename Entry>
    bool add_paths(RRNodeId start_node,
                   Entry current,
                   const std::vector<util::Search_Path>& paths,
                   util::RoutingCosts* routing_costs);

    template<typename Entry>
    std::pair<float, int> run_dijkstra(RRNodeId start_node,
                                       std::vector<bool>* node_expanded,
                                       std::vector<util::Search_Path>* paths,
                                       util::RoutingCosts* routing_costs);

    CostMap cost_map_; ///<Cost map containing all data to extract the entry cost when querying the lookahead.
  public:
    /**
     * @brief Returns the expected cost to get to a destination node
     */
    float get_expected_cost(RRNodeId node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const override;

    /**
     * @brief Returns a pair of expected delay and congestion
     */
    std::pair<float, float> get_expected_delay_and_cong(RRNodeId inode, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const override;

    /**
     * @brief Computes the extended lookahead map
     */
    void compute(const std::vector<t_segment_inf>& segment_inf) override;

    /**
     * @brief Reads the extended lookahead map
     */
    void read(const std::string& file) override;

    /**
     * @brief Writes the extended lookahead map
     */
    void write(const std::string& file) const override;
};

#endif
