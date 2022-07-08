#ifndef DRAW_NOC_H
#define DRAW_NOC_H

/**
 * @file 
 * @brief This is the draw_noc header file. This file provides utilities
 * to display a user described NoC within the VTR GUI. The only function
 * that should be used externally to display the NoC is draw_noc().
 * 
 * Overview
 * ========
 * The following steps are all performd when displaying the NoC:
 * - First, the state of the NoC display button is checked. If the
 * user selected to not display the NoC, then nothing is displayed.
 * - If the user selected to display the NoC, then first the NoC routers
 * are highlighted. The highlighting is done on top of the already drawn
 * FPGA device within the canvas.
 * - Then links are drawn on top of the FPGA device, connecting the routers
 * together based on the topology. There is an option to display the "usage"
 * of each link in the NoC. If this is selected then a color map legend is drawn
 * and each link is colored based on how much of its bandwidth is being used
 * relative to its maximum capaity. 
 * 
 */
#include <iostream>
#include <vector>

#ifndef NO_GRAPHICS

#    include "draw.h"

// defines the area of the marker that represents connection points between links
// area is equivalent to the %x of the area of the router
constexpr float SIZE_OF_NOC_MARKER = 0.30;

// defines the length of a reference horizontal line that is used in a cross product to calculate the angle between this line and a noc link to be drawn
const double HORIZONTAL_LINE_LENGTH(5.0);

const double PI_RADIAN(3.141593);

// represents the coordinates needed to draw any noc link in the canvas
// contains the start and end positions of the link
struct noc_link_draw_coords {
    ezgl::point2d start;
    ezgl::point2d end;
};

// define the types of orientations we will see links draw in in the canvas
// vertical defines a vertical line
// horizontal defines a horizontal line
// positive_slope defines a line that is angled and has a positive slope
// negative_slope defines a line that is angled and has a negative slope
enum NocLinkType {
    INVALID_TYPE, // initially all links are undefined
    HORIZONTAL,
    VERTICAL,
    POSITVE_SLOPE,
    NEGATIVE_SLOPE
};

/*
 * Since the noc links run in both directions between any two routers, we want to draw them parallel to each other instead of ovrelapping them. So the idea is to shift one link in one direction and shift the other link in the opposite direction. THe enum below defines the direction a link was shifted, so for example if we had a vertical line, top would be mean shift left and Bottom would mean shift right. SImilarily, if we had a horizontal line, top would mean shift up and bottom would mean shift down.
 */
enum NocLinkShift {
    NO_SHIFT, // initially there is no shift
    TOP_SHIFT,
    BOTTOM_SHIFT
};

/**
 * @brief Displays the NoC on the FPGA device. Displaying the NoC goes through the following steps:
 * - First check to make sure that the user selected to button to display the NoC otherwise don't display it
 * - Highlight the Noc routers by drawing a marker on top of them in the canvas
 * - Draw the links that connect the routers together (based on topology)
 * - If the user selected the option to show the NoC link usage, then color the links to represent how much
 * of their capacity is being used
 * - The drawing is done on top of the FPGA device within the canvas
 *
 * Below we have an example of how the Noc is displayed for a FPGA device with 
 * 2 routers:
 *
 * Before NoC display:
 *    *********************                      *********************
 *    *                   *                      *                   *
 *    *                   *                      *                   *
 *    *                   *                      *                   *
 *    *                   *                      *                   *
 *    *                   *                      *                   *
 *    *                   *                      *                   *
 *    *                   *                      *                   *
 *    *********************                      *********************
 *
 * After NoC display:
 *
 *    *********************                      *********************
 *    *                   *                      *                   *
 *    *                   *                      *                   *
 *    *       ****<-------*----------------------*-------****        *
 *    *       *  *        *                      *       *  *        *
 *    *       ****--------*----------------------*------>****        *
 *    *                   *                      *                   *
 *    *                   *                      *                   *
 *    *********************                      *********************
 *
 * @param g canvas renderer.
 */
void draw_noc(ezgl::renderer* g);

// draw_noc helper functions

/**
 * @brief When the NoC is drawn, the routers are identified within the device
 * by drawing markers within them. The markers are small squares that are drawn
 * within the router tiles. This function determines the relative position and
 * size of the marker with respect to a router tile. This function determines
 * the coordinates to draw the marker within the center of each NoC router tile.
 * And having a predertimes size relative to the NoC router tile.
 * 
 * The connection marker will be positioned at the center of the noc router
 * tile. For example it will look like below: 
 *
 *    *********************
 *    *                   *
 *    *                   *
 *    *       ****        *
 *    *       *  *        *
 *    *       ****        *
 *    *                   *
 *    *                   *
 *    ********************* 
 * 
 * @param noc_router_logical_block_type The logical type of a a NoC router
 * block. It is used to get drawing information about the router blocks.
 * @return ezgl::rectangle Two drawing coordinates of the marker.
 */
ezgl::rectangle get_noc_connection_marker_bbox(const t_logical_block_type_ptr noc_router_logical_block_type);

/**
 * @brief Displays the markers described in the function above on the
 * drawing canvas. These markers are drawn on top of every NoC router
 * tile. 
 * 
 * @param g Canvas renderer. 
 * @param router_list Contains all the NoC router tiles within the FPGA. Used 
 * to get the drawing coordinates of the tiles.
 * @param connection_marker_bbox The coordinates of the marker to draw within 
 * the center of a NoC router tile.
 */
void draw_noc_connection_marker(ezgl::renderer* g, const vtr::vector<NocRouterId, NocRouter>& router_list, ezgl::rectangle connection_marker_bbox);

/**
 * @brief Displays the NoC links on the drawing canvas. The links are
 * drawn as lines between router tiles they connect together. The links
 * are also drawn such that they are connected to the markers drawn by
 * the function above. Links that are parallel to each other are drawn
 * spaced apart (so the parallelism can be seen instead of overlapping them).
 * Finally, if the user indicated that they wanted to see the link usage, the
 * links are colored to show how much of their capacity is being used. By
 * default the link color is black.
 * 
 * @param g Canvas renderer.
 * @param noc_router_logical_block_type The logical type of a a NoC router
 * block. It is used to get drawing information about the router blocks.
 * @param noc_link_colors A vector of colors for each link that determine
 * the color used to draw the links.
 * @param noc_connection_marker_bbox ezgl::rectangle Two drawing coordinates
 * that determine the relative position the marker within the router tile.
 * @param list_of_noc_link_shift_directions A vector of directions that 
 * determine how links should be moved before being drawn. For example, if
 * two links are parallel they would have the same drawing coordinates and
 * will overlap. This vector describes how the two links should be moved
 * so that they do not overlap.
 */
void draw_noc_links(ezgl::renderer* g, t_logical_block_type_ptr noc_router_logical_block_type, vtr::vector<NocLinkId, ezgl::color>& noc_link_colors, ezgl::rectangle noc_connection_marker_bbox, const vtr::vector<NocLinkId, NocLinkShift>& list_of_noc_link_shift_directions);

/**
 * @brief Goes through all the links within the NoC and updates the color that
 * will be used to draw them based on how much of the links bandwidth is being
 * used.
 * 
 * @param noc_link_colors A vector of color codes that represent each NoC
 * links bandwidth usage.
 */
void draw_noc_usage(vtr::vector<NocLinkId, ezgl::color>& noc_link_colors);

/**
 * @brief For a NoC that is undirected, each connection between routers has two
 *        parallel links. This function goes through all the links and assigns 
 *        opposite directions to shift each pair of parallel links. This
 *        information is later used to determine how each link is shifted.
 *        The reason for shifting the links is so that the two parallel links
 *        do not overlap each other when drawn. Instead they are spread apart a
 *        bit.
 * 
 * @param list_of_noc_link_shift_directions A list that stores the direction to
 *                                          shift each link in the NoC when
 *                                          drawn.
 */
void determine_direction_to_shift_noc_links(vtr::vector<NocLinkId, NocLinkShift>& list_of_noc_link_shift_directions);

/**
 * @brief The type (orientation) of a line is determined here. A second
 *        horizontal line is created that connects to the starting point of the
 *        provided line (function input). Then the angle between the two
 *        lines are calculated using the dot product. Based on this angle, the 
 *        line direction is determined (vertical, horizontal, positive sloped 
 *        or negative sloped).
 * 
 * @param link_start_point The starting point of a line that represents a NoC
 *                         link.
 * @param link_end_point THe end point of a line that represent a NoC link.
 */
NocLinkType determine_noc_link_type(ezgl::point2d link_start_point, ezgl::point2d link_end_point);

/**
 * @brief Given the starting and end coordinates of a line that represents a
 *        NoC link, the line shifted to a new set of coordinates based on its
 *        type and shift direction. THe appropriate shifting formula is applied
 *        to each line based on its type and direction to shift. The end result
 *        is that each pair of parallel links are spaces apart and not drawn on
 *        top of each other. 
 * 
 * @param link_coords The original coordinates to draw the NoC link.
 * @param link_shift_direction The direction to shift the NoC link.
 * @param link_type The type (direction) of the NoC link.
 * @param noc_connection_marker_quarter_width The distance to shift the NoC link
 *                                            in the horizontal direction.
 * @param noc_connection_marker_quarter_height The distance to shift the NoC
 *                                             link in the vertical direction.
 */
void shift_noc_link(noc_link_draw_coords& link_coords, NocLinkShift link_shift_direction, NocLinkType link_type, double noc_connection_marker_quarter_width, double noc_connection_marker_quarter_height);

#endif

#endif