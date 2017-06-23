#include "types.h"

void graphVizOutputNetlist(char* path, const char* name, short marker_value, netlist_t *netlist); 
void graphVizOutputCombinationalNet(char* path, const char* name, short marker_value, nnode_t *current_node);
