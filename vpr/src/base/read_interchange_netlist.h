#ifndef READ_INTERCHANGE_NETLIST_H
#define READ_INTERCHANGE_NETLIST_H
#include "logic_types.h"
#include "atom_netlist_fwd.h"
#include "read_circuit.h"

AtomNetlist read_interchange_netlist(const char* ic_netlist_file,
                                     t_arch& arch);

#endif /*READ_INTERCHANGE_NETLIST_H*/
