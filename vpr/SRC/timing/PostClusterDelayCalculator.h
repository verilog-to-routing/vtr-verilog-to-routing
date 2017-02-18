#ifndef POST_CLUSTER_DELAY_CALCULATOR_H
#define POST_CLUSTER_DELAY_CALCULATOR_H
#include "vtr_linear_map.h"

#include "Time.hpp"
#include "TimingGraph.hpp"

#include "atom_netlist.h"
#include "vpr_utils.h"

#include "atom_delay_calc.h"
#include "clb_delay_calc.h"

class PostClusterDelayCalculator {

public:
    PostClusterDelayCalculator(const AtomNetlist& netlist, const AtomMap& netlist_map, float** net_delay);

    tatum::Time max_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;
    tatum::Time setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;

    tatum::Time min_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;
    tatum::Time hold_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;

private:

    tatum::Time atom_combinational_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;
    tatum::Time atom_setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;
    tatum::Time atom_clock_to_q_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;
    tatum::Time atom_net_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;

    float inter_cluster_delay(const t_net_pin* driver_clb_pin, const t_net_pin* sink_clb_pin) const;

private:
    const AtomNetlist& netlist_;
    const AtomMap& netlist_map_;
    float** net_delay_;

    ClbDelayCalc clb_delay_calc_;
    AtomDelayCalc atom_delay_calc_;

    mutable vtr::vector_map<tatum::EdgeId,tatum::Time> edge_delay_cache_;
    mutable vtr::vector_map<tatum::EdgeId,tatum::Time> driver_clb_delay_cache_;
    mutable vtr::vector_map<tatum::EdgeId,tatum::Time> sink_clb_delay_cache_;
    mutable vtr::vector_map<tatum::EdgeId,std::pair<const t_net_pin*,const t_net_pin*>> net_pin_cache_;
};

#include "PostClusterDelayCalculator.tpp"
#endif
