/**
 * @file draw_basic.h
 * 
 * draw_basic.cpp contains all functions that draw in the main graphics area
 * that aren't RR nodes or muxes (they have their own file).
 * All functions in this file contain the prefix draw_.
 */

#ifndef DRAW_BASIC_H
#define DRAW_BASIC_H

#include <cstdio>
#include <cfloat>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <array>
#include <iostream>

#include "vtr_assert.h"
#include "vtr_ndoffsetmatrix.h"
#include "vtr_memory.h"
#include "vtr_log.h"
#include "vtr_color_map.h"
#include "vtr_path.h"

#include "vpr_utils.h"
#include "vpr_error.h"

#include "globals.h"

#include "move_utils.h"

#ifndef NO_GRAPHICS

#    include "draw_global.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

/* Draws the blocks placed on the proper clbs.  Occupied blocks are darker colours *
 * while empty ones are lighter colours and have a dashed border. *
 * Blocks are drawn in layer order (so that semi-transparent blocks/grids render well)*/
void drawplace(ezgl::renderer* g);

/* This routine draws the nets on the placement.  The nets have not *
 * yet been routed, so we just draw a chain showing a possible path *
 * for each net.  This gives some idea of future congestion.        */
void drawnets(ezgl::renderer* g);

/* Draws all the overused routing resources (i.e. congestion) in various contrasting colors showing congestion ratio.  */
void draw_congestion(ezgl::renderer* g);

/* Draws routing resource nodes colored according to their congestion costs */
void draw_routing_costs(ezgl::renderer* g);

/* Draws bounding box (BB) in which legal RR node start/end points must be contained */
void draw_routing_bb(ezgl::renderer* g);

/* Draws an X centered at (x,y). The width and height of the X are each 2 * size. */
void draw_x(float x, float y, float size, ezgl::renderer* g);

/* Draws the nets in the positions fixed by the router.  If draw_net_type is *
 * ALL_NETS, draw all the nets.  If it is HIGHLIGHTED, draw only the nets    *
 * that are not coloured black (useful for drawing over the rr_graph).       */
void drawroute(enum e_draw_net_type draw_net_type, ezgl::renderer* g);

void draw_routed_net(ParentNetId net, ezgl::renderer* g);

//Draws the set of rr_nodes specified, using the colors set in draw_state
void draw_partial_route(const std::vector<RRNodeId>& rr_nodes_to_draw,
                        ezgl::renderer* g);

/**
 * @brief Returns the layer number of a timing path node
 * @param node
 * @return layer number the node is situated on.
 */
int get_timing_path_node_layer_num(tatum::NodeId node);

/**
 * @brief Returns true if both the current_node and prev_node are on the same layer and it is visible,
 *        or they're on different layers that are both visible and cross-layer connections are visible.
 *        Otherwise returns false.
 *
 *  @param current_node
 *  @param prev_node
 *
 *  @return
 */
bool is_edge_valid_to_draw(RRNodeId current_node, RRNodeId prev_node);

/* Draws a heat map of routing wire utilization (i.e. fraction of wires used in each channel)
 * when a routing is shown on-screen and Routing Util (on the GUI) is selected.
 * Lighter colours (e.g. yellow) correspond to highly utilized
 * channels, while darker colours (e.g. blue) correspond to lower utilization.*/
void draw_routing_util(ezgl::renderer* g);

/* Draws the critical path if Crit. Path (in the GUI) is selected. Each stage between primitive
 * pins is shown in a different colour.
 * User can toggle between two different visualizations:
 * a) during placement, critical path only shown as flylines
 * b) during routing, critical path is shown by both flylines and routed net connections.
 */
void draw_crit_path(ezgl::renderer* g);

/**
 * @brief  Checks whether a flyline should be drawn or not based on the layer control settings in the UI
 * @param src_layer
 * @param sink_layer
 *
 * @return          If the source and sink are on the same active(visible) layer - returns true
 *                  If the source and sink are on different active layers & Cross-layer connections is toggled on - returns true
 *                  Otherwise returns false
 */
bool is_flyline_valid_to_draw(int src_layer, int sink_layer);

/* Draws critical path shown as flylines. Takes in start and end coordinates, time delay, & renderer.*/
void draw_flyline_timing_edge(ezgl::point2d start, ezgl::point2d end, float incr_delay, ezgl::renderer* g);

/* Collects all the drawing locations associated with the timing edge between start and end.
 * Only traces interconnect edges in detail, and treats all others as flylines.
 */
void draw_routed_timing_edge_connection(tatum::NodeId src_tnode,
                                        tatum::NodeId sink_tnode,
                                        ezgl::color color,
                                        ezgl::renderer* g);

/* Draws any placement macros (e.g. carry chains, which require specific relative placements
 * between some blocks) if the Placement Macros (in the GUI) is seelected.
 */
void draw_placement_macros(ezgl::renderer* g);

//Draws colour legend
void draw_color_map_legend(const vtr::ColorMap& cmap, ezgl::renderer* g);

//Draws block pin utilization
void draw_block_pin_util();

//Resets all block colours
void draw_reset_blk_colors();

//Reset a specific block's colour.
void draw_reset_blk_color(ClusterBlockId blk_id);

#endif /* NO_GRAPHICS */
#endif /* DRAW_BASIC_H */
