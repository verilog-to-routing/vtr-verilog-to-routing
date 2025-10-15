# pragma once

#include "vpr_types.h"
#include "rr_graph.h"
#include "rr_graph_view.h"
#include "rr_graph_builder.h"

void build_crr_graph_edges(const RRGraphView& rr_graph,
                           RRGraphBuilder& rr_graph_builder,
                           const t_crr_opts& crr_opts);