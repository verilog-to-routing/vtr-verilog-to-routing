#ifndef NETLIST2_UTILS_H
#define NETLIST2_UTILS_H
#include <cstdio>
#include "netlist2.h"

/*
 *
 * Useful utilities for working with the AtomNetlist class
 *
 */

/*
 * Modifies the netlist by absorbing buffer LUTs
 */
void absorb_buffer_luts(AtomNetlist& netlist);

/*
 * Modify the netlist by sweeping away unused nets/blocks/inputs
 */
//Repeatedly sweeps the netlist removing blocks and nets 
//until nothing more can be swept.
//If sweep_ios is true also sweeps primary-inputs and primary-outputs
size_t sweep_iterative(AtomNetlist& netlist, bool sweep_ios);

//Sweeps blocks that have no fanout
size_t sweep_blocks(AtomNetlist& netlist);

//Sweeps nets with no drivers and/or no sinks
size_t sweep_nets(AtomNetlist& netlist);

//Sweeps primary-inputs with no fanout
size_t sweep_inputs(AtomNetlist& netlist);

//Sweeps primary-outputs with no fanin
size_t sweep_outputs(AtomNetlist& netlist);

/*
 * Print the netlist for debugging
 */
void print_netlist(FILE* f, const AtomNetlist& netlist);
void print_netlist_as_blif(FILE* f, const AtomNetlist& netlist);


#endif
