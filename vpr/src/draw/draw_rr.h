#pragma once
/**
 * @file draw_rr.h
 * 
 * draw_rr.cpp contains all functions that relate to drawing routing resources.
 */

#ifndef NO_GRAPHICS

#include <cstdio>
#include <cfloat>
#include <cstring>
#include <cmath>

#include "rr_graph_fwd.h"
#include "rr_node.h"

#include "ezgl/graphics.hpp"

/* Draws the routing resources that exist in the FPGA, if the user wants *
 * them drawn. */
void draw_rr(ezgl::renderer* g);

/* Draws all the edges that the user wants shown between inode and what it
 * connects to.  inode is assumed to be a CHANX, CHANY, or IPIN. */
void draw_rr_edges(RRNodeId from_node, ezgl::renderer* g);

void draw_rr_chan(RRNodeId inode, const ezgl::color color, ezgl::renderer* g);

/* Draws an IPIN or OPIN rr_node.  Note that the pin can appear on more
 * than one side of a clb.  Also note that this routine can change the
 * current color to BLACK. */
void draw_rr_pin(RRNodeId inode, const ezgl::color& color, ezgl::renderer* g);

void draw_rr_src_sink(RRNodeId inode, ezgl::color color, ezgl::renderer* g);
void draw_get_rr_src_sink_coords(const t_rr_node& node, float* xcen, float* ycen);

/* Draws a buffer (triangle) or pass transistor (circle) on the edge
 * connecting from to to, depending on the status of buffered.  The drawing
 * is closest to the from_node, since it reflects the switch type of from.  */
void draw_rr_switch(float from_x, float from_y, float to_x, float to_y, bool buffered, bool switch_configurable, ezgl::renderer* g);
void draw_expand_non_configurable_rr_nodes_recurr(RRNodeId from_node,
                                                  std::set<RRNodeId>& expanded_nodes);

/* This is a helper function for highlight_rr_nodes(). It determines whether
 * a routing resource has been clicked on by computing a bounding box for that
 *  and checking if the mouse click hit inside its bounding box.
 *  It returns the hit RR node's ID (or INVALID if no hit) */
RRNodeId draw_check_rr_node_hit(float click_x, float click_y);

/* This routine is called when the routing resource graph is shown, and someone
 * clicks outside a block. That click might represent a click on a wire -- we call
 * this routine to determine which wire (if any) was clicked on.  If a wire was
 * clicked upon, we highlight it in Magenta, and its fanout in red.*/
bool highlight_rr_nodes(float x, float y);

/* Draws routing costs */
void draw_rr_costs(ezgl::renderer* g, const vtr::vector<RRNodeId, float>& rr_costs, bool lowest_cost_first = true);

/* Returns the coordinates at which the center of this pin should be drawn. *
 * inode gives the node number, and iside gives the side of the clb or pad  *
 * the physical pin is on.                                                  */
void draw_get_rr_pin_coords(RRNodeId inode, float* xcen, float* ycen, const e_side& pin_side);

/* Returns the coordinates at which the center of this pin should be drawn. *
 * node gives the node object, and iside gives the side of the clb or pad  *
 * the physical pin is on.                                                  */
void draw_get_rr_pin_coords(const t_rr_node& node, float* xcen, float* ycen, const e_side& pin_side);

/**
 * @brief returns transparency, given rr node
 * Checks the layer transparency of the given rr node and returns it
 *
 */
int get_rr_node_transparency(RRNodeId rr_node);
#endif /* NO_GRAPHICS */
