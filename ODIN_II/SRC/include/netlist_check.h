#ifndef NETLIST_CHECK_H
#define NETLIST_CHECK_H

void check_netlist(netlist_t* netlist);
void levelize_and_check_for_combinational_loop_and_liveness(netlist_t* netlist);

#endif
