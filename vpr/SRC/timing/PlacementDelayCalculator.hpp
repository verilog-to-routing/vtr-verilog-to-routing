#ifndef PLACEMENT_DELAY_CALCULATOR_HPP
#define PLACEMENT_DELAY_CALCULATOR_HPP
#include "Time.hpp"
#include "TimingGraph.hpp"

#include "atom_netlist.h"
#include "vpr_utils.h"

class PlacementDelayCalculator {

public:
    PlacementDelayCalculator(const AtomNetlist& netlist, const AtomMap& netlist_map, t_type_descriptor* types, int num_types, float** net_delay)
        : netlist_(netlist)
        , netlist_map_(netlist_map)
        , net_delay_(net_delay)
        , intra_lb_pb_pin_lookup_(types, num_types)
        {}

    tatum::Time max_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;
    tatum::Time setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;

    tatum::Time min_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;
    tatum::Time hold_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;

private:

    tatum::Time atom_combinational_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;
    tatum::Time atom_setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;
    tatum::Time atom_clock_to_q_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;
    tatum::Time atom_net_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;

    std::tuple<float,t_net_pin> trace_capture_cluster_delay(int clb_sink_block, const t_pb_graph_pin* sink_gpin, const AtomNetId atom_net) const;
    std::tuple<float,t_net_pin> trace_inter_cluster_delay(t_net_pin clb_sink_input_pin) const;
    std::tuple<float> trace_launch_cluster_delay(t_net_pin clb_driver_output_pin, const AtomNetId atom_net) const;

    const t_pb_graph_pin* find_pb_graph_pin(const AtomPinId pin) const;
    bool is_clb_io_pin(int clb_block, int clb_pb_route_idx) const;
    float pb_route_max_delay(int clb_block, int pb_route_idx) const;
    const t_pb_graph_edge* find_pb_graph_edge(int clb_block, int pb_route_idx) const;
    const t_pb_graph_edge* find_pb_graph_edge(const t_pb_graph_pin* driver, const t_pb_graph_pin* sink) const;
private:
    const AtomNetlist& netlist_;
    const AtomMap& netlist_map_;
    float** net_delay_;

    IntraLbPbPinLookup intra_lb_pb_pin_lookup_;
};

#include "PlacementDelayCalculator.tpp"
#endif
