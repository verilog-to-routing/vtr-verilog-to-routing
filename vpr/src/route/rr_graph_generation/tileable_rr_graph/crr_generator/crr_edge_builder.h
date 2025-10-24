#pragma once

#include <vector>

#include "vtr_vector.h"
#include "vtr_geometry.h"

#include "physical_types.h"
#include "device_grid.h"

#include "rr_gsb.h"
#include "rr_graph_obj.h"
#include "rr_graph.h"
#include "rr_graph_view.h"
#include "rr_graph_builder.h"

#include "crr_connection_builder.h"

void build_gsb_track_to_track_edges(RRGraphBuilder& rr_graph_builder,
                                    const RRGSB& rr_gsb,
                                    const crrgenerator::CRRConnectionBuilder& connection_builder);
