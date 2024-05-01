
#pragma once

#include "PartialPlacement.h"

// TODO: Split this into two classes: PartialLegalizer and FullLegalizer
//       Partial legalizer takes a partial placement and returns a partial
//       placement.
//       Full legalizer takes a partial placement and returns a clustered
//       placement which can be fed into the Simulated Annealer.
class PlacementLegalizer {
public:
    virtual ~PlacementLegalizer() {}
    virtual void legalize(PartialPlacement &p_placement) = 0;
};

class FlowBasedLegalizer : public PlacementLegalizer {
public:
    void legalize(PartialPlacement &p_placement) final;
};

