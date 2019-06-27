#ifndef VPR_PRE_CLUSTER_TIMING_GRAPH_RESOLVER_H_
#define VPR_PRE_CLUSTER_TIMING_GRAPH_RESOLVER_H_

#include "tatum/TimingGraphNameResolver.hpp"
#include "atom_netlist_fwd.h"
#include "atom_lookup.h"
#include "AnalysisDelayCalculator.h"

class PreClusterTimingGraphResolver : public tatum::TimingGraphNameResolver {
  public:
    PreClusterTimingGraphResolver(
        const AtomNetlist& netlist,
        const AtomLookup& netlist_lookup,
        const tatum::TimingGraph& timing_graph,
        const tatum::DelayCalculator& delay_calc);

    std::string node_name(tatum::NodeId node) const override;
    std::string node_type_name(tatum::NodeId node) const override;

    tatum::EdgeDelayBreakdown edge_delay_breakdown(tatum::EdgeId edge, tatum::DelayType delay_type) const override;

    void set_detail_level(e_timing_report_detail report_detail);

  private:
    e_timing_report_detail detail_level() const;

    const AtomNetlist& netlist_;
    const AtomLookup& netlist_lookup_;
    const tatum::TimingGraph& timing_graph_;
    const tatum::DelayCalculator& delay_calc_;
    e_timing_report_detail detail_level_ = e_timing_report_detail::NETLIST;
};

#endif /* VPR_PRE_CLUSTER_TIMING_GRAPH_RESOLVER_H_ */
