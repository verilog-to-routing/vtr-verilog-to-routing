
#include "draw_noc.h"
#include "globals.h"
#include "noc_storage.h"
#include "vpr_error.h"
#include "vtr_math.h"

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
void determine_direction_to_shift_noc_links(vtr::vector<NocLinkId,NocLinkShift>& list_of_noc_link_shift_directions){

    auto& noc_ctx = g_vpr_ctx.noc();
    //const NocStorage* noc_model = &noc_ctx.noc_model;

    int number_of_links = list_of_noc_link_shift_directions.size();

    // store the parallel link of wach link we process
    NocLinkId parallel_link;

    // go through all the noc links and assign how the link should be shifted
    for (int link = 0; link < number_of_links; link++){

        // convert the link to a link id
        NocLinkId link_id(link);

        // only assign a shift direction if we havent already
        if (list_of_noc_link_shift_directions[link_id] == NocLinkShift::NO_SHIFT){

            // the current link will always have a TOP_shift
            list_of_noc_link_shift_directions[link_id] = NocLinkShift::TOP_SHIFT;

            // find the parallel link and assign it a BOTTOM_shift
            parallel_link = noc_ctx.noc_model.get_parallel_link(link_id);

            // check first if a legal link id was found
            if (parallel_link == INVALID_LINK_ID)
            {
                // if we are here, then a parallel link wasnt found, so that means there is only a single link and there is no need to perform any shifting on the single link
                list_of_noc_link_shift_directions[link_id] = NocLinkShift::NO_SHIFT;

                continue;
            }

            list_of_noc_link_shift_directions[parallel_link] = NocLinkShift::BOTTOM_SHIFT;

        }

    }

    return;
}


/**
 * @brief The type (orientation) of a line is determine here. A second
 *        horizontal line is created that connects to the starting point of the
 *        provided line (function input). Then the angle between the two
 *        lines is calculated using the cross product. Based on this angle, the 
 *        line direction is determined (vertical, horizontal, positive sloped 
 *        or negative sloped).
 * 
 * @param link_start_point The starting point of a line that represents a NoC
 *                         link.
 * @param link_end_point THe end point of a line that represent a NoC link.
*/
NocLinkType determine_noc_link_type(ezgl::point2d link_start_point, ezgl::point2d link_end_point){

    // get the coordinates of the provided line
    double x_coord_start = link_start_point.x;
    double y_coord_start = link_start_point.y;

    double x_coord_end = link_end_point.x;
    double y_coord_end = link_end_point.y;

    // create a horizontal line that connects to the start point of the provided line and spans a predefined length
    double x_coord_horizontal_start = link_start_point.x;
    double y_coord_horizontal_start = link_start_point.y;

    double x_coord_horziontal_end = x_coord_horizontal_start + HORIZONTAL_LINE_LENGTH;
    double y_coord_horizontal_end = link_start_point.y;

    // stores the line type
    NocLinkType result = NocLinkType::INVALID_TYPE;

    // we can check quickly if the line is vertical or horizontal without calculating the cross product. If it is vertical or horizontal then we just return. Otherwise we have to calculate it.

    // check if the line is vertical by determining if there is any horizontal change
    if (vtr::isclose(x_coord_end - x_coord_start, 0.0)){
        result = NocLinkType::VERTICAL;

        return result;
    }

    // check if the line is horizontal by determinig if there is any vertical shift
    if (vtr::isclose(y_coord_end - y_coord_start, 0.0)){
        result = NocLinkType::HORIZONTAL;

        return result;
    }

    // if we are here then the line has a slope, so we use the cross product to determine the angle 
    double angle = acos(((x_coord_end - x_coord_start) * (x_coord_horziontal_end - x_coord_horizontal_start) + (y_coord_end - y_coord_start) * (y_coord_horizontal_end - y_coord_horizontal_start)) / (sqrt(pow(x_coord_end - x_coord_start, 2.0) + pow(y_coord_end - y_coord_start, 2.0)) * sqrt(pow(x_coord_horziontal_end - x_coord_horizontal_start, 2.0) + pow(y_coord_horizontal_end - y_coord_horizontal_start, 2.0))));

    // the angle is in the first or fourth quandrant of the unit circle
    if ((angle > 0) && (angle < (PI_RADIAN/2))){

        // if it is a positive sloped line, then the y coordinate of the end point is higher the y-coordinate of the horizontal line
        if (y_coord_end > y_coord_horizontal_end){

            result = NocLinkType::POSITVE_SLOPE;
        }
        else{ // if the end y-coordinate is less than the horizontal line then we have a negative sloped line

            result = NocLinkType::NEGATIVE_SLOPE;
        }

    }
    else{ // the case where the angle is in the 3rd and 4th quandrant of the unit cirle

        // if the line is positive sloped, then the y-coordinate of the end point is lower than the y-coordinate of the horizontal line
        if (y_coord_end < y_coord_horizontal_end){

            result = NocLinkType::POSITVE_SLOPE;
        }
        else{ // if the end y-coordinate of is greater than the horizontal line then we have a negative sloped line

            result = NocLinkType::NEGATIVE_SLOPE;
        }
    }

    return result;

}

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
void shift_noc_link(noc_link_draw_coords& link_coords, NocLinkShift link_shift_direction, NocLinkType link_type, double noc_connection_marker_quarter_width, double noc_connection_marker_quarter_height){

    // determine the type of link and based on that shift the link accordingly
    /*
        Vertical line: shift the link left and right and the distance is equal to half the width of the connection marker
        
        horizontal line: shift the link up and down and the distance is equal to half the width of the connection marker
        
        positive slope line: shift the link to the bottom right and top left corners of the connection marker

        negative slope line: shift the link to the bottom left and top roght corners of the connection marker

    */

   switch (link_type){
        case NocLinkType::INVALID_TYPE:
            break;
        case NocLinkType::VERTICAL:
            if (link_shift_direction == NocLinkShift::TOP_SHIFT){
                // shift the link left
                link_coords.start.x -= noc_connection_marker_quarter_width;
                link_coords.end.x -= noc_connection_marker_quarter_width;
            }
            else if (link_shift_direction == NocLinkShift::BOTTOM_SHIFT){
                // shift to the right
                link_coords.start.x += noc_connection_marker_quarter_width;
                link_coords.end.x += noc_connection_marker_quarter_width;
            }
            // dont change anything if we arent shifting at all
            break;
        case NocLinkType::HORIZONTAL:
            if (link_shift_direction == NocLinkShift::TOP_SHIFT){
                // shift the link up
                link_coords.start.y += noc_connection_marker_quarter_height;
                link_coords.end.y += noc_connection_marker_quarter_height;
            }
            else if (link_shift_direction == NocLinkShift::BOTTOM_SHIFT){
                // shift to the down
                link_coords.start.y -= noc_connection_marker_quarter_height;
                link_coords.end.y -= noc_connection_marker_quarter_height;
            }
            // dont change anything if we arent shifting at all
            break;
        case NocLinkType::POSITVE_SLOPE:
            if (link_shift_direction == NocLinkShift::TOP_SHIFT){
                // shift link up and left to the top left corner
                link_coords.start.x -= noc_connection_marker_quarter_width;
                link_coords.end.x -= noc_connection_marker_quarter_width;

                link_coords.start.y += noc_connection_marker_quarter_height;
                link_coords.end.y += noc_connection_marker_quarter_height;
            }
            else if (link_shift_direction == NocLinkShift::BOTTOM_SHIFT){
                // shift link down and right to the bottom right corner
                link_coords.start.x += noc_connection_marker_quarter_width;
                link_coords.end.x += noc_connection_marker_quarter_width;

                link_coords.start.y -= noc_connection_marker_quarter_height;
                link_coords.end.y -= noc_connection_marker_quarter_height;
            }
            // dont change anything if we arent shifting at all
            break;
        case NocLinkType::NEGATIVE_SLOPE:
            if (link_shift_direction == NocLinkShift::TOP_SHIFT){
                // shift link down and left to the bottom left corner
                link_coords.start.x -= noc_connection_marker_quarter_width;
                link_coords.end.x -= noc_connection_marker_quarter_width;

                link_coords.start.y -= noc_connection_marker_quarter_height;
                link_coords.end.y -= noc_connection_marker_quarter_height;
            }
            else if (link_shift_direction == NocLinkShift::BOTTOM_SHIFT){
                // shift link up and right to the top right corner
                link_coords.start.x += noc_connection_marker_quarter_width;
                link_coords.end.x += noc_connection_marker_quarter_width;

                link_coords.start.y += noc_connection_marker_quarter_height;
                link_coords.end.y += noc_connection_marker_quarter_height;
            }
            // dont change anything if we arent shifting at all
            break;
        default:
            break;
   }

    return;
}




