
#pragma once

#include "PartialPlacement.h"

class AnalyticalSolver {
public:
    virtual ~AnalyticalSolver() {}
    virtual void solve(PartialPlacement &p_placement) = 0;
};

// TODO: There is no difference between the hybrid, clique, and star since the
//       the threshold acts as a mixing factor. 0 = star, inf = clique
class QPHybridSolver : public AnalyticalSolver {
// To implement the global placement step of HeAP or SimPL efficeintly, we will
// need to store the matrices for the system of equations in the class.
// TODO: Store a standard VTR matrix and pass that into Eigen using Map
public:
    void solve(PartialPlacement &p_placement) final;
};

