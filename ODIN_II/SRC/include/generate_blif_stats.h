#ifndef GENERATE_BLIF_STATS_H
#define GENERATE_BLIF_STATS_H

#include "odin_types.h"
#include <string>

void output_blif_stats(std::string file_name, netlist_t *netlist, bool verbose);

#endif

