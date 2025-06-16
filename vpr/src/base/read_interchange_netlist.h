#pragma once

#include "atom_netlist_fwd.h"

struct t_arch;

AtomNetlist read_interchange_netlist(const char* ic_netlist_file,
                                     t_arch& arch);
