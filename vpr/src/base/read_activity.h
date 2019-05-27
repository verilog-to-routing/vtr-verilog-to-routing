#ifndef READ_ACTIVITY_H
#define READ_ACTIVITY_H
#include <unordered_map>

#include "atom_netlist_fwd.h"
#include "vpr_types.h"

std::unordered_map<AtomNetId, t_net_power> read_activity(const AtomNetlist& netlist, const char* activity_file);

#endif
