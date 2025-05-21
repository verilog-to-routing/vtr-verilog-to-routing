/**
 * @file draw_rr_edges.h
 * 
 * draw_rr_edges.cpp contains all functions that draw lines between RR nodes.
 */

#pragma once

#ifndef NO_GRAPHICS

#include <cstdio>
#include <cfloat>
#include <cstring>
#include <cmath>

#include "draw_types.h"
#include "rr_graph_fwd.h"

#include "ezgl/graphics.hpp"

void draw_chany_to_chany_edge(RRNodeId from_node, RRNodeId to_node, short switch_type, ezgl::renderer* g);
void draw_chanx_to_chanx_edge(RRNodeId from_node, RRNodeId to_node, short switch_type, ezgl::renderer* g);
void draw_chanx_to_chany_edge(RRNodeId chanx_node, RRNodeId chany_node, enum e_edge_dir edge_dir, short switch_type, ezgl::renderer* g);
void draw_pin_to_pin(RRNodeId opin, RRNodeId ipin, ezgl::renderer* g);
void draw_pin_to_sink(RRNodeId ipin_node, RRNodeId sink_node, ezgl::renderer* g);
void draw_source_to_pin(RRNodeId source_node, RRNodeId opin_node, ezgl::renderer* g);
void draw_pin_to_chan_edge(RRNodeId pin_node, RRNodeId chan_node, ezgl::renderer* g);

#endif /* NO_GRAPHICS */
