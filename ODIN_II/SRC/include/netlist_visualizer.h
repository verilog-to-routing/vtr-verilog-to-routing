#include "odin_types.h"

void graphVizOutputNetlist(std::string path, const char* name, short marker_value, netlist_t *netlist);
void graphVizOutputCombinationalNet(std::string path, const char* name, short marker_value, nnode_t *current_node);
