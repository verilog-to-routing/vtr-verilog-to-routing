#ifndef DRAW_NOC_H
#define DRAW_NOC_H
/*
    THis file contains functions and data types related to drawing the NoC in the canvas.

    The functions here should be used to determine how the NoC links should be drawn.

*/
#include <iostream>
#include <vector>

#include "draw.h"

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
    Since the noc links run in both directions between any two routers, we want to draw them parallel to each other instead of ovrelapping them. So the idea is to shift one link in one direction and shift the other link in the opposite direction. THe enum below defines the direction a link was shifted, so for example if we had a vertical line, top would be mean shift left and Bottom would mean shift right. SImilarily, if we had a horizontal line, top would mean shift up and bottom would mean shift down.
*/
enum NocLinkShift {
    NO_SHIFT, // initially there is no shift
    TOP_SHIFT,
    BOTTOM_SHIFT
};

void determine_direction_to_shift_noc_links(vtr::vector<NocLinkId,NocLinkShift>& list_of_noc_link_shift_directions);

NocLinkType determine_noc_link_type(ezgl::point2d link_start_point, ezgl::point2d link_end_point);

void shift_noc_link(noc_link_draw_coords& link_coords, NocLinkShift link_shift_direction, NocLinkType link_type, double noc_connection_marker_quarter_width, double noc_connection_marker_quarter_height);



























#endif