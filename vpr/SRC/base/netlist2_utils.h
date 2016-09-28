#ifndef NETLIST2_UTILS_H
#define NETLIST2_UTILS_H
#include <cstdio>
#include "netlist2.h"

//Print the netlist for debugging
void print_netlist(FILE* f, const AtomNetlist& netlist);

//Modify the netlist by absorbing buffer LUTs
void absorb_buffer_luts(AtomNetlist& netlist);

//Modify the netlist by absorbing constants
void absorb_constants(AtomNetlist& netlist);

//Modify the netlist by sweeping away unused nets/blocks/inputs
size_t sweep_iterative(AtomNetlist& netlist, bool sweep_ios);
size_t sweep_blocks(AtomNetlist& netlist);
size_t sweep_inputs(AtomNetlist& netlist);
size_t sweep_outputs(AtomNetlist& netlist);
size_t sweep_nets(AtomNetlist& netlist);


#endif
