#ifndef NETLIST_VISUALIZER_H
#define NETLIST_VISUALIZER_H

#include <string>
#include "odin_types.h"

void graphVizOutputNetlist(const std::string& path, const char* name, uintptr_t marker_value, netlist_t* input_netlist);
void graphVizOutputCombinationalNet(const std::string& path, const char* name, uintptr_t marker_value, nnode_t* current_node);

#endif
