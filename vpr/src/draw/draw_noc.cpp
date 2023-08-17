
#ifndef NO_GRAPHICS

#    include "draw_noc.h"
#    include "globals.h"
#    include "noc_storage.h"
#    include "vpr_error.h"
#    include "vtr_math.h"
#    include "draw_basic.h"

void draw_noc(ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();
    auto& noc_ctx = g_vpr_ctx.noc();
    auto& device_ctx = g_vpr_ctx.device();

    // vector of routers in the NoC
    const vtr::vector<NocRouterId, NocRouter>& router_list = noc_ctx.noc_model.get_noc_routers();

    // a vector of colors to use for the NoC links, determines the colors used when drawing each link
    vtr::vector<NocLinkId, ezgl::color> noc_link_colors;

    // initialize all the link colors to black and set the vector size to the total number of links
    noc_link_colors.resize(noc_ctx.noc_model.get_noc_links().size(), ezgl::BLACK);

    // variables to keep track of how each link is drawn
    vtr::vector<NocLinkId, NocLinkShift> list_of_noc_link_shift_directions;

    // initialize all the link shift directions to no shift and set the vector size to the total number of links
    list_of_noc_link_shift_directions.resize(noc_ctx.noc_model.get_noc_links().size(), NocLinkShift::NO_SHIFT);

    // start by checking to see if the NoC display button was selected
    // if the noc display option was not selected then don't draw the noc
    if (draw_state->draw_noc == DRAW_NO_NOC) {
        return;
    }

    // check that the NoC tile has a capacity greater than 0 (can we assume it always will?) and if not then we cant draw anythign as the NoC tile wont be drawn
    /* since the vector of routers all have a reference positions on the grid to the corresponding physical tile, just use the first router in the vector and get its position, then use this to get the capcity of a noc router tile
     */
    const auto& type = device_ctx.grid.get_physical_type({router_list.begin()->get_router_grid_position_x(),
                                                          router_list.begin()->get_router_grid_position_y(),
                                                          router_list.begin()->get_router_layer_position()});
    int num_subtiles = type->capacity;

    if (num_subtiles == 0) {
        return;
    }

    // get the logical type of a noc router tile
    t_logical_block_type_ptr noc_router_logical_type = pick_logical_type(type);

    // Now construct the coordinates for the markers that represent the connections between links (relative to the noc router tile position)
    ezgl::rectangle noc_connection_marker_bbox = get_noc_connection_marker_bbox(noc_router_logical_type);

    // only draw the noc useage if the user selected the option
    if (draw_state->draw_noc == DRAW_NOC_LINK_USAGE) {
        draw_noc_usage(noc_link_colors);

        // draw the color map legend
        draw_color_map_legend(*(draw_state->noc_usage_color_map), g);
    }

    // go through all the pairs of parallel links (if there are any) and assign a shift direction to them.
    // One link will shift in one direction (knows as TOP) and the other link will shift in the opposite direction (known as BOTTOM)
    determine_direction_to_shift_noc_links(list_of_noc_link_shift_directions);

    draw_noc_links(g, noc_router_logical_type, noc_link_colors, noc_connection_marker_bbox, list_of_noc_link_shift_directions);

    draw_noc_connection_marker(g, router_list, noc_connection_marker_bbox);

    return;
}

/*
 * Go through each NoC link and assign a color based on how much the link is being used (its bandwidth). The colors are determined from the PLasma colormap.
 */
void draw_noc_usage(vtr::vector<NocLinkId, ezgl::color>& noc_link_colors) {
    t_draw_state* draw_state = get_draw_state_vars();
    auto& noc_ctx = g_vpr_ctx.noc();

    // get the maximum badnwidth per link
    double max_noc_link_bandwidth = noc_ctx.noc_model.get_noc_link_bandwidth();

    // check to see if a color map was already created previously
    if (draw_state->noc_usage_color_map == nullptr) {
        // we havent created a color map yet for the noc link usage, so create it here
        // the color map creates a color spectrum that gradually changes from a dark to light color. Where a dark color represents low noc link usage (low bandwidth) and a light color represents high noc link usage (high bandwidth)
        // The color map needs a min and max value to generate the color range.
        // The noc usage is calculated by taking the ratio of the links current bandwidth over the maximum allowable bandwidth
        // for the NoC, the min value is 0, since you cannot go lower than 0 badnwidth.
        // The max value is going to be 1 and represents the case where the link is used to full capacity on the noc link (as provided by the user)
        draw_state->noc_usage_color_map = std::make_shared<vtr::PlasmaColorMap>(0.0, 1.0);
    }

    // get the list of links in the NoC
    const vtr::vector<NocLinkId, NocLink>& noc_link_list = noc_ctx.noc_model.get_noc_links();

    // store each links bandwidth usage
    double link_bandwidth_usage;

    // represents the color to draw each noc link
    ezgl::color current_noc_link_color;

    // now we need to determine the colors for each link
    for (int link = 0; link < (int)noc_link_list.size(); link++) {
        // get the current link id
        NocLinkId link_id(link);

        // only update the color of the link if it wasnt updated previously
        if (noc_link_colors[link_id] == ezgl::BLACK) {
            // if we are here then the link was not updated previously, so assign the color here

            //get the current link bandwidth usage (ratio calculation)
            link_bandwidth_usage = (noc_link_list[link_id].get_bandwidth_usage()) / max_noc_link_bandwidth;

            // check if the link is being overused and if it is then cap it at 1.0
            if (link_bandwidth_usage > 1.0) {
                link_bandwidth_usage = 1.0;
            }

            // get the corresponding color that represents the link bandwidth usgae
            current_noc_link_color = to_ezgl_color(draw_state->noc_usage_color_map->color(link_bandwidth_usage));

            // set the colors of the link
            noc_link_colors[link_id] = current_noc_link_color;
        }
    }

    return;
}

/*
 * This function calculates the position of the marker that will be drawn inside the noc router tile on the FPGA. This marker will be located in the center of the tile and represents a connection point between links that connect to the router.
 */
ezgl::rectangle get_noc_connection_marker_bbox(const t_logical_block_type_ptr noc_router_logical_block_type) {
    t_draw_coords* draw_coords = get_draw_coords_vars();

    // get the drawing information for a noc router
    t_draw_pb_type_info& blk_type_info = draw_coords->blk_info.at(noc_router_logical_block_type->index);

    // get the drawing coordinates for the noc router tile
    auto& pb_gnode = *noc_router_logical_block_type->pb_graph_head;
    ezgl::rectangle noc_router_pb_bbox = blk_type_info.get_pb_bbox(pb_gnode);

    /*
     * The connection marker will be positioned at the center of the noc router tile. For example it will look like below:
     *
     *********************
     *                   *
     *                   *
     *       ****        *
     *       *  *        *
     *       ****        *
     *                   *
     *                   *
     *********************
     *
     * We do the following to calculate the position of the marker:
     * 1. Get the area of the larger router tile
     * 2. Calculate the area of the marker (based on a predefined percentage of the area of the larger noc tile)
     * 3. The marker is a square, so we can can calculate the lengths 
     * of the sides of the marker
     * 4. Divide the side length by 2 and subtract this from the x & y coordinates of the center of the larger noc router tile. This is the bottom left corner of the rectangle.
     * 5. Then add the side length to the x & y coordinate of the center of the larger noc router tile. THis is the top right corner of the rectangle.    
     */
    double noc_router_bbox_area = noc_router_pb_bbox.area();
    ezgl::point2d noc_router_bbox_center = noc_router_pb_bbox.center();

    double connection_marker_bbox_area = noc_router_bbox_area * SIZE_OF_NOC_MARKER;
    double connection_marker_bbox_side_length = sqrt(connection_marker_bbox_area);

    double half_of_connection_marker_bbox_side_length = connection_marker_bbox_side_length / 2;

    // calculate bottom left corner coordinate of marker
    ezgl::point2d connection_marker_origin_pt(noc_router_bbox_center.x - half_of_connection_marker_bbox_side_length, noc_router_bbox_center.y - half_of_connection_marker_bbox_side_length);
    // calculate upper right corner coordinate of marker
    ezgl::point2d connection_marker_top_right_pt(noc_router_bbox_center.x + half_of_connection_marker_bbox_side_length, noc_router_bbox_center.y + half_of_connection_marker_bbox_side_length);

    ezgl::rectangle connection_marker_bbox(connection_marker_origin_pt, connection_marker_top_right_pt);

    return connection_marker_bbox;
}

/*
 * This function draws the markers inside the noc router tiles. This marker represents a connection that is an intersection points between multiple links.
 */
void draw_noc_connection_marker(ezgl::renderer* g, const vtr::vector<NocRouterId, NocRouter>& router_list, ezgl::rectangle connection_marker_bbox) {
    t_draw_coords* draw_coords = get_draw_coords_vars();
    t_draw_state* draw_state = get_draw_state_vars();

    int router_grid_position_x = 0;
    int router_grid_position_y = 0;
    int router_grid_position_layer = 0;

    ezgl::rectangle updated_connection_marker_bbox;

    // go through the routers and create the connection marker
    for (auto router = router_list.begin(); router != router_list.end(); router++) {
        router_grid_position_layer = router->get_router_layer_position();

        t_draw_layer_display marker_box_visibility = draw_state->draw_layer_display[router_grid_position_layer];
        if (!marker_box_visibility.visible) {
            continue; /* Don't Draw marker box if not on visible layer*/
        }

        //set the color of the marker with the layer transparency
        g->set_color(ezgl::BLACK, marker_box_visibility.alpha);

        router_grid_position_x = router->get_router_grid_position_x();
        router_grid_position_y = router->get_router_grid_position_y();

        // get the coordinates to draw the marker given the current routers tile position
        updated_connection_marker_bbox = connection_marker_bbox + ezgl::point2d(draw_coords->tile_x[router_grid_position_x], draw_coords->tile_y[router_grid_position_y]);

        // draw the marker
        g->fill_rectangle(updated_connection_marker_bbox);
    }

    return;
}

/*
 * This function draws the links within the noc. So based on a given noc topology, this function draws the links that connect the routers in the noc together.
 */
void draw_noc_links(ezgl::renderer* g, t_logical_block_type_ptr noc_router_logical_block_type, vtr::vector<NocLinkId, ezgl::color>& noc_link_colors, ezgl::rectangle noc_connection_marker_bbox, const vtr::vector<NocLinkId, NocLinkShift>& list_of_noc_link_shift_directions) {
    t_draw_coords* draw_coords = get_draw_coords_vars();
    auto& noc_ctx = g_vpr_ctx.noc();

    // vector of routers in the NoC
    const vtr::vector<NocRouterId, NocRouter>& router_list = noc_ctx.noc_model.get_noc_routers();

    // get the links of the NoC
    const vtr::vector<NocLinkId, NocLink>& noc_link_list = noc_ctx.noc_model.get_noc_links();

    // set the width of the link
    g->set_line_width(2);

    // routers connecting links
    NocRouterId source_router;
    NocRouterId sink_router;

    // source router grid coordinates
    int source_router_layer_position = 0;
    int source_router_x_position = 0;
    int source_router_y_position = 0;

    // sink router grid coordinates
    int sink_router_layer_position = 0;
    int sink_router_x_position = 0;
    int sink_router_y_position = 0;

    // coordinates of source and sink routers
    ezgl::rectangle abs_source_router_bbox;
    ezgl::rectangle abs_sink_router_bbox;

    // the coordinates to draw the link
    noc_link_draw_coords link_coords;

    // the type of the noc link
    NocLinkType link_type;

    // get half the width and height of the noc connection marker
    // we will shift the links based on this parameters since the links will be drawn at the boundaries of connection marker instead of the center
    double noc_connection_marker_quarter_width = (noc_connection_marker_bbox.center().x - noc_connection_marker_bbox.bottom_left().x) / 2;
    double noc_connection_marker_quarter_height = (noc_connection_marker_bbox.center().y - noc_connection_marker_bbox.bottom_left().y) / 2;

    // loop through the links and draw them
    for (int link = 0; link < (int)noc_link_list.size(); link++) {
        // get the converted link if
        NocLinkId link_id(link);

        // get the routers
        source_router = noc_link_list[link_id].get_source_router();
        sink_router = noc_link_list[link_id].get_sink_router();

        //Calculate the layer position of the source and sink routers
        source_router_layer_position = router_list[source_router].get_router_layer_position();
        sink_router_layer_position = router_list[sink_router].get_router_layer_position();

        //Get visibility settings of the current NoC link based on the layer visibility settings set by the user
        t_draw_layer_display noc_link_visibility = get_element_visibility_and_transparency(source_router_layer_position, sink_router_layer_position);

        if (!noc_link_visibility.visible) {
            continue; /* Don't Draw link */
        }

        // calculate the grid positions of the source and sink routers
        source_router_x_position = router_list[source_router].get_router_grid_position_x();
        source_router_y_position = router_list[source_router].get_router_grid_position_y();

        sink_router_x_position = router_list[sink_router].get_router_grid_position_x();
        sink_router_y_position = router_list[sink_router].get_router_grid_position_y();

        // get the initial drawing coordinates of the noc link
        // it will be drawn from the center of two routers it connects
        link_coords.start = draw_coords->get_absolute_clb_bbox(source_router_layer_position, source_router_x_position, source_router_y_position, 0, noc_router_logical_block_type).center();
        link_coords.end = draw_coords->get_absolute_clb_bbox(sink_router_layer_position, sink_router_x_position, sink_router_y_position, 0, noc_router_logical_block_type).center();

        // determine the current noc link type
        link_type = determine_noc_link_type(link_coords.start, link_coords.end);

        shift_noc_link(link_coords, list_of_noc_link_shift_directions[link_id], link_type, noc_connection_marker_quarter_width, noc_connection_marker_quarter_height);

        // set the color to draw the current link
        g->set_color(noc_link_colors[link_id], noc_link_visibility.alpha);

        //draw a line between the center of the two routers this link connects
        g->draw_line(link_coords.start, link_coords.end);
    }

    return;
}

void determine_direction_to_shift_noc_links(vtr::vector<NocLinkId, NocLinkShift>& list_of_noc_link_shift_directions) {
    auto& noc_ctx = g_vpr_ctx.noc();

    int number_of_links = list_of_noc_link_shift_directions.size();

    // store the parallel link of wach link we process
    NocLinkId parallel_link;

    // go through all the noc links and assign how the link should be shifted
    for (int link = 0; link < number_of_links; link++) {
        // convert the link to a link id
        NocLinkId link_id(link);

        // only assign a shift direction if we havent already
        if (list_of_noc_link_shift_directions[link_id] == NocLinkShift::NO_SHIFT) {
            // the current link will always have a TOP_shift
            list_of_noc_link_shift_directions[link_id] = NocLinkShift::TOP_SHIFT;

            // find the parallel link and assign it a BOTTOM_shift
            parallel_link = noc_ctx.noc_model.get_parallel_link(link_id);

            // check first if a legal link id was found
            if (parallel_link == INVALID_LINK_ID) {
                // if we are here, then a parallel link wasnt found, so that means there is only a single link and there is no need to perform any shifting on the single link
                list_of_noc_link_shift_directions[link_id] = NocLinkShift::NO_SHIFT;

                continue;
            }

            list_of_noc_link_shift_directions[parallel_link] = NocLinkShift::BOTTOM_SHIFT;
        }
    }

    return;
}

NocLinkType determine_noc_link_type(ezgl::point2d link_start_point, ezgl::point2d link_end_point) {
    // get the coordinates of the provided line that represents a link
    double x_coord_start = link_start_point.x;
    double y_coord_start = link_start_point.y;

    double x_coord_end = link_end_point.x;
    double y_coord_end = link_end_point.y;

    // create a horizontal line that connects to the start point of the provided link and spans a predefined length
    double x_coord_horizontal_start = link_start_point.x;
    double y_coord_horizontal_start = link_start_point.y;

    double x_coord_horziontal_end = x_coord_horizontal_start + HORIZONTAL_LINE_LENGTH;
    double y_coord_horizontal_end = link_start_point.y;

    // stores the link type
    NocLinkType result = NocLinkType::INVALID_TYPE;

    // we can check quickly if the link is vertical or horizontal without calculating the dot product. If it is vertical or horizontal then we just return. Otherwise we have to calculate it.

    // check if the link is vertical by determining if there is any horizontal change
    if (vtr::isclose(x_coord_end - x_coord_start, 0.0)) {
        result = NocLinkType::VERTICAL;

        return result;
    }

    // check if the link is horizontal by determinig if there is any vertical shift
    if (vtr::isclose(y_coord_end - y_coord_start, 0.0)) {
        result = NocLinkType::HORIZONTAL;

        return result;
    }

    // if we are here then the line that represents a link has a slope, so we use the dot product to determine the angle between the line and the horizontal line created above that connects to the link.

    // get the magnitude of the link
    double link_magnitude = sqrt(pow(x_coord_end - x_coord_start, 2.0) + pow(y_coord_end - y_coord_start, 2.0));
    // get the dot product of the two connecting line
    double dot_product_of_link_and_horizontal_line = (x_coord_end - x_coord_start) * (x_coord_horziontal_end - x_coord_horizontal_start) + (y_coord_end - y_coord_start) * (y_coord_horizontal_end - y_coord_horizontal_start);
    // calculate the angle
    double angle = acos(dot_product_of_link_and_horizontal_line / (link_magnitude * HORIZONTAL_LINE_LENGTH));

    // the angle is in the first or fourth quandrant of the unit circle
    if ((angle > 0) && (angle < (PI_RADIAN / 2))) {
        // if the link is a positive sloped line, then its end point must be higher than its start point (must be higher than the connected horizontal line)
        if (y_coord_end > y_coord_horizontal_end) {
            result = NocLinkType::POSITVE_SLOPE;
        } else { // if the end point is lower than the start point (lower than the connected horizontal line) then it must be negatively sloped

            result = NocLinkType::NEGATIVE_SLOPE;
        }

    } else { // the case where the angle is in the 3rd and 4th quandrant of the unit cirle

        // if the link is a positive sloped line, then its end point must be lower than its start point (must be lower than the connected horizontal line)
        if (y_coord_end < y_coord_horizontal_end) {
            result = NocLinkType::POSITVE_SLOPE;
        } else { // if the end point is higher than the start point (higher than the connected horizontal line) then it must be negatively sloped

            result = NocLinkType::NEGATIVE_SLOPE;
        }
    }

    return result;
}

void shift_noc_link(noc_link_draw_coords& link_coords, NocLinkShift link_shift_direction, NocLinkType link_type, double noc_connection_marker_quarter_width, double noc_connection_marker_quarter_height) {
    // determine the type of link and based on that shift the link accordingly
    /*
     * Vertical line: shift the link left and right and the distance is equal to half the width of the connection marker
     *
     * horizontal line: shift the link up and down and the distance is equal to half the width of the connection marker
     *
     * positive slope line: shift the link to the bottom right and top left corners of the connection marker
     *
     * negative slope line: shift the link to the bottom left and top right corners of the connection marker
     *
     */

    switch (link_type) {
        case NocLinkType::INVALID_TYPE:
            break;
        case NocLinkType::VERTICAL:
            if (link_shift_direction == NocLinkShift::TOP_SHIFT) {
                // shift the link left
                link_coords.start.x -= noc_connection_marker_quarter_width;
                link_coords.end.x -= noc_connection_marker_quarter_width;
            } else if (link_shift_direction == NocLinkShift::BOTTOM_SHIFT) {
                // shift to the right
                link_coords.start.x += noc_connection_marker_quarter_width;
                link_coords.end.x += noc_connection_marker_quarter_width;
            }
            // dont change anything if we arent shifting at all
            break;
        case NocLinkType::HORIZONTAL:
            if (link_shift_direction == NocLinkShift::TOP_SHIFT) {
                // shift the link up
                link_coords.start.y += noc_connection_marker_quarter_height;
                link_coords.end.y += noc_connection_marker_quarter_height;
            } else if (link_shift_direction == NocLinkShift::BOTTOM_SHIFT) {
                // shift to the down
                link_coords.start.y -= noc_connection_marker_quarter_height;
                link_coords.end.y -= noc_connection_marker_quarter_height;
            }
            // dont change anything if we arent shifting at all
            break;
        case NocLinkType::POSITVE_SLOPE:
            if (link_shift_direction == NocLinkShift::TOP_SHIFT) {
                // shift link up and left to the top left corner
                link_coords.start.x -= noc_connection_marker_quarter_width;
                link_coords.end.x -= noc_connection_marker_quarter_width;

                link_coords.start.y += noc_connection_marker_quarter_height;
                link_coords.end.y += noc_connection_marker_quarter_height;
            } else if (link_shift_direction == NocLinkShift::BOTTOM_SHIFT) {
                // shift link down and right to the bottom right corner
                link_coords.start.x += noc_connection_marker_quarter_width;
                link_coords.end.x += noc_connection_marker_quarter_width;

                link_coords.start.y -= noc_connection_marker_quarter_height;
                link_coords.end.y -= noc_connection_marker_quarter_height;
            }
            // dont change anything if we arent shifting at all
            break;
        case NocLinkType::NEGATIVE_SLOPE:
            if (link_shift_direction == NocLinkShift::TOP_SHIFT) {
                // shift link down and left to the bottom left corner
                link_coords.start.x -= noc_connection_marker_quarter_width;
                link_coords.end.x -= noc_connection_marker_quarter_width;

                link_coords.start.y -= noc_connection_marker_quarter_height;
                link_coords.end.y -= noc_connection_marker_quarter_height;
            } else if (link_shift_direction == NocLinkShift::BOTTOM_SHIFT) {
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

#endif