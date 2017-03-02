#ifndef VRP_TIMING_GRAPH_NAME_RESOLVER_H
#define VRP_TIMING_GRAPH_NAME_RESOLVER_H

#include "tatum/TimingGraphNameResolver.hpp"
#include "atom_netlist_fwd.h"
#include "atom_lookup.h"

class VprTimingGraphNameResolver : public tatum::TimingGraphNameResolver {
    public:
        VprTimingGraphNameResolver(const AtomNetlist& netlist, const AtomLookup& netlist_lookup);

        std::string node_name(tatum::NodeId node) const override;
        std::string node_block_type_name(tatum::NodeId node) const override;
    private:
        const AtomNetlist& netlist_;
        const AtomLookup& netlist_lookup_;
};

#endif
