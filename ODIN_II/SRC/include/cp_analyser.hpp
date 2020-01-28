#include "netlist_utils.h"


void dfs_to_cp(netlist_t *netlist);
void dfs_s2e_cp(nnode_t *node, int traverse_mark_number, netlist_t *netlist);
void dfs_e2s_cp(nnode_t *node, int traverse_mark_number, netlist_t *netlist);