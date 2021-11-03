#ifndef SETUP_CLOCKS_H
#define SETUP_CLOCKS_H

#include <vector>
#include<rr_graph_fwd.h>
#include "physical_types.h"

void setup_clock_networks(const t_arch& Arch, vtr::vector<RRSegmentId,t_segment_inf>& segment_inf);

#endif
