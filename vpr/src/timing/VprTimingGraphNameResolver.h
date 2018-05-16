#ifndef VRP_TIMING_GRAPH_NAME_RESOLVER_H
#define VRP_TIMING_GRAPH_NAME_RESOLVER_H

#include "tatum/TimingGraphNameResolver.hpp"
#include "atom_netlist_fwd.h"
#include "atom_lookup.h"

class VprTimingGraphResolver : public tatum::TimingGraphNameResolver {
    public:
        VprTimingGraphResolver(const AtomNetlist& netlist, const AtomLookup& netlist_lookup);

        std::string node_name(tatum::NodeId node) const override;
        std::string node_type_name(tatum::NodeId node) const override;

        tatum::EdgeDelayBreakdown edge_delay_breakdown(tatum::EdgeId edge, tatum::DelayType delay_type) const override;

        void set_detailed(bool value);
    private:
        bool detailed() const;

        const AtomNetlist& netlist_;
        const AtomLookup& netlist_lookup_;
        bool detailed_ = false;
};

#endif
