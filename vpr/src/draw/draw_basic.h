#pragma once
/**
 * @file draw_basic.h
 * 
 * draw_basic.cpp contains all functions that draw in the main graphics area
 * that aren't RR nodes or muxes (they have their own file).
 * All functions in this file contain the prefix draw_.
 */

#ifndef NO_GRAPHICS

#include <cstdio>
#include <cstring>

#include "draw_types.h"
#include "netlist_fwd.h"
#include "rr_graph_fwd.h"
#include "tatum/TimingGraphFwd.hpp"

#include "vtr_color_map.h"

#include "ezgl/point.hpp"
#include "ezgl/graphics.hpp"

// Forward Declaration
class APBlockId;

/**
 * @brief Draws all placed blocks on the device grid across visible layers.
 *
 * Occupied blocks are darker colours while empty ones are lighter colours and have a dashed border.
 * Blocks are drawn in layer order (so that semi-transparent blocks/grids render well)
 */
void draw_place(ezgl::renderer* g);

/** This function draws the analytical placement from the PartialPlacement object, it
 *  also draws the architecture grid and the blocks from device_ctx.
 */
void draw_analytical_place(ezgl::renderer* g);

/** This routine draws the nets on the placement.  The nets have not
 * yet been routed, so we just draw a chain showing a possible path
 * for each net.  This gives some idea of future congestion. 
 * This function may be deprecated. draw_logical_connections() is preferred. */
void draw_flylines_placement(ezgl::renderer* g);

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
void draw_route(enum e_draw_net_type draw_net_type, ezgl::renderer* g);

void draw_routed_net(ParentNetId net, ezgl::renderer* g);

//Draws the set of rr_nodes specified, using the colors set in draw_state
void draw_partial_route(const std::vector<RRNodeId>& rr_nodes_to_draw,
                        ezgl::renderer* g);

/**
 * @brief Returns true if both the current_node and prev_node are on the same layer and it is visible,
 *        or they're on different layers that are both visible and cross-layer connections are visible.
 *        Otherwise returns false.
 */
bool is_edge_valid_to_draw(RRNodeId current_node, RRNodeId prev_node);

/* Draws a heat map of routing wire utilization (i.e. fraction of wires used in each channel)
 * when a routing is shown on-screen and Routing Util (on the GUI) is selected.
 * Lighter colours (e.g. yellow) correspond to highly utilized
 * channels, while darker colours (e.g. blue) correspond to lower utilization.*/
void draw_routing_util(ezgl::renderer* g);

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

/* Draws any placement macros (e.g. carry chains, which require specific relative placements
 * between some blocks) if the Placement Macros (in the GUI) is selected.
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

/**
 * @brief Returns the drawing coordinates for an analytical placement block.
 *
 * @param ap_block AP block whose current drawing location is requested.
 * @return 2D coordinates associated with the given AP block.
 */
ezgl::point2d get_ap_block_draw_coords(APBlockId ap_block);

#endif /* NO_GRAPHICS */
