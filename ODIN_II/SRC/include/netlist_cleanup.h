#ifndef NETLIST_CLEANUP_H
#define NETLIST_CLEANUP_H

// a global var to specify the need for cleanup after receiving coarsen BLIF as input.
extern bool coarsen_cleanup;

void remove_unused_logic(netlist_t* netlist);

#endif
