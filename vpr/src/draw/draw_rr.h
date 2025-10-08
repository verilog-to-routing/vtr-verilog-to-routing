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

/** 
 * @brief Draws the routing resources that exist in the FPGA, if the user wants
 * them drawn. 
 */
void draw_rr(ezgl::renderer* g);

/// @brief Draws all the edges that the user wants shown between @p from_node and what it
/// connects to. @p from_node is assumed to be a CHANX, CHANY, or IPIN.
void draw_rr_edges(RRNodeId from_node, ezgl::renderer* g);

/// Maps (from_type, to_type) pairs to an edge type used for drawing or classification.
inline const std::map<std::pair<e_rr_type, e_rr_type>, e_edge_type> EDGE_TYPE_MAP = {
    // Pin to pin connections
    {{e_rr_type::IPIN, e_rr_type::IPIN}, e_edge_type::PIN_TO_IPIN},
    {{e_rr_type::OPIN, e_rr_type::IPIN}, e_edge_type::PIN_TO_IPIN},
    {{e_rr_type::OPIN, e_rr_type::OPIN}, e_edge_type::PIN_TO_OPIN},
    {{e_rr_type::IPIN, e_rr_type::OPIN}, e_edge_type::PIN_TO_OPIN},
    // Channel to pin connections
    {{e_rr_type::OPIN, e_rr_type::CHANX}, e_edge_type::OPIN_TO_CHAN},
    {{e_rr_type::OPIN, e_rr_type::CHANY}, e_edge_type::OPIN_TO_CHAN},
    {{e_rr_type::CHANX, e_rr_type::IPIN}, e_edge_type::CHAN_TO_IPIN},
    {{e_rr_type::CHANY, e_rr_type::IPIN}, e_edge_type::CHAN_TO_IPIN},
    // Channel to channel connections
    {{e_rr_type::CHANX, e_rr_type::CHANX}, e_edge_type::CHAN_TO_CHAN},
    {{e_rr_type::CHANX, e_rr_type::CHANY}, e_edge_type::CHAN_TO_CHAN},
    {{e_rr_type::CHANY, e_rr_type::CHANY}, e_edge_type::CHAN_TO_CHAN},
    {{e_rr_type::CHANY, e_rr_type::CHANX}, e_edge_type::CHAN_TO_CHAN},
};

void draw_rr_chan(RRNodeId inode, const ezgl::color color, ezgl::renderer* g);

/**
 * @brief Draws a RR node.
 *
 * @param inode The RRNodeId of the node to draw.
 * @param color The color to use for drawing the node.
 * @param g The renderer to use for drawing.
 */
void draw_rr_node(RRNodeId inode, const ezgl::color color, ezgl::renderer* g);

/**
 * @brief Draws the intra-cluster pin for a given RRNodeId when flat routing is enabled.
 */
void draw_rr_intra_cluster_pin(RRNodeId inode, const ezgl::color& color, ezgl::renderer* g);

/* Draws an IPIN or OPIN rr_node.  Note that the pin can appear on more
 * than one side of a clb.  Also note that this routine can change the
 * current color to BLACK. */
void draw_cluster_pin(RRNodeId inode, const ezgl::color& color, ezgl::renderer* g);

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
