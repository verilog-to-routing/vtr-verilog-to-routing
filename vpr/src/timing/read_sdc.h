#ifndef VPR_READ_SDC_H
#define VPR_READ_SDC_H
#include <memory>

#include "tatum/TimingConstraintsFwd.hpp"
#include "tatum/TimingGraphFwd.hpp"

#include "atom_netlist_fwd.h"
#include "atom_lookup_fwd.h"
#include "vpr_types.h"

std::unique_ptr<tatum::TimingConstraints> read_sdc(const t_timing_inf& timing_inf,
                                                   const AtomNetlist& netlist,
                                                   const AtomLookup& lookup,
                                                   tatum::TimingGraph& timing_graph);

#endif
