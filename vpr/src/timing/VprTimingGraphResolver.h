#ifndef VRP_TIMING_GRAPH_NAME_RESOLVER_H
#define VRP_TIMING_GRAPH_NAME_RESOLVER_H

#include "tatum/TimingGraphNameResolver.hpp"
#include "atom_netlist_fwd.h"
#include "atom_lookup.h"
#include "AnalysisDelayCalculator.h"

class BlkLocRegistry;

class VprTimingGraphResolver : public tatum::TimingGraphNameResolver {
  public:
    VprTimingGraphResolver(const AtomNetlist& netlist,
                           const AtomLookup& netlist_lookup,
                           const tatum::TimingGraph& timing_graph,
                           const AnalysisDelayCalculator& delay_calc,
                           bool is_flat,
                           const BlkLocRegistry& blk_loc_registry);

    std::string node_name(tatum::NodeId node) const override;
    std::string node_type_name(tatum::NodeId node) const override;

    tatum::EdgeDelayBreakdown edge_delay_breakdown(tatum::EdgeId edge, tatum::DelayType delay_type) const override;

    void set_detail_level(e_timing_report_detail report_detail);

  private:
    e_timing_report_detail detail_level() const;
    std::vector<tatum::DelayComponent> interconnect_delay_breakdown(tatum::EdgeId edge, DelayType) const;

    void get_detailed_interconnect_components(std::vector<tatum::DelayComponent>& components, ParentNetId net_id, ParentPinId sink_pin) const;
    void get_detailed_interconnect_components_helper(std::vector<tatum::DelayComponent>& components, const RouteTreeNode& node) const;

    const AtomNetlist& netlist_;
    const AtomLookup& netlist_lookup_;
    const tatum::TimingGraph& timing_graph_;
    const AnalysisDelayCalculator& delay_calc_;
    e_timing_report_detail detail_level_ = e_timing_report_detail::NETLIST;
    bool is_flat_;
    ///@brief contains information about the placement of clustered blocks.
    const BlkLocRegistry& blk_loc_registry_;
};

#endif
