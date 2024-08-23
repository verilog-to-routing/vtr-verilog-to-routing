
#pragma once

#include <unordered_map>
#include "PartialPlacement.h"
#include "ap_netlist_fwd.h"

class APNetlist;

// TODO: Split this into two classes: PartialLegalizer and FullLegalizer
//       Partial legalizer takes a partial placement and returns a partial
//       placement.
//       Full legalizer takes a partial placement and returns a clustered
//       placement which can be fed into the Simulated Annealer.
class PlacementLegalizer {
public:
    PlacementLegalizer(const APNetlist& inetlist) : netlist(inetlist) {}
    virtual ~PlacementLegalizer() {}
    virtual void legalize(PartialPlacement &p_placement) = 0;
protected:
    const APNetlist& netlist;
};

class FlowBasedLegalizer : public PlacementLegalizer {
    using PlacementLegalizer::PlacementLegalizer;
public:
    void legalize(PartialPlacement &p_placement) final;
    std::unordered_map<const t_model*, size_t> demand_vector_index;
    vtr::vector<APBlockId, std::vector<int>> demand_vector; 
};

class FullLegalizer : public PlacementLegalizer {
    using PlacementLegalizer::PlacementLegalizer;
public:
    void legalize(PartialPlacement &p_placement) final;
};

