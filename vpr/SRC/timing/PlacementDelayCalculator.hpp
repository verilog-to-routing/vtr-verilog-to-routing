#ifndef PLACEMENT_DELAY_CALCULATOR_HPP
#define PLACEMENT_DELAY_CALCULATOR_HPP
#include "Time.hpp"
#include "TimingGraph.hpp"

#include "atom_netlist.h"

class PlacementDelayCalculator {

public:
    PlacementDelayCalculator(const AtomNetlist& netlist, const AtomMap& netlist_map)
        : netlist_(netlist)
        , netlist_map_(netlist_map)
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

    const t_pb_graph_pin* find_pb_graph_pin(const AtomPinId pin) const;
private:
    const AtomNetlist& netlist_;
    const AtomMap& netlist_map_;
};

#include "PlacementDelayCalculator.tpp"
#endif
