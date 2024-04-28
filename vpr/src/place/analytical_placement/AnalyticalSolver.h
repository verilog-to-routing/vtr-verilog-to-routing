
#pragma once

#include "PartialPlacement.h"

class AnalyticalSolver {
public:
    virtual ~AnalyticalSolver() {}
    virtual void solve(PartialPlacement &p_placement) = 0;
};

class QPHybridSolver : public AnalyticalSolver {
// To implement the global placement step of HeAP or SimPL efficeintly, we will
// need to store the matrices for the system of equations in the class.
// TODO: Store a standard VTR matrix and pass that into Eigen using Map
public:
    void solve(PartialPlacement &p_placement) final;
};

