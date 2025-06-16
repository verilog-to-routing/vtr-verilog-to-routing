#pragma once

#include <memory>

#include "tatum/TimingConstraintsFwd.hpp"
#include "tatum/TimingGraphFwd.hpp"

#include "atom_netlist_fwd.h"
#include "atom_lookup_fwd.h"
#include "vpr_types.h"

class LogicalModels;

std::unique_ptr<tatum::TimingConstraints> read_sdc(const t_timing_inf& timing_inf,
                                                   const AtomNetlist& netlist,
                                                   const AtomLookup& lookup,
                                                   const LogicalModels& models,
                                                   tatum::TimingGraph& timing_graph);
