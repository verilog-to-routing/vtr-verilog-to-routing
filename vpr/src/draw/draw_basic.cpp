/* draw_basic.cpp contains all functions that draw in the main graphics area
 * that aren't RR nodes or muxes (they have their own file).
 * All functions in this file contain the prefix draw_. */
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
#include "draw_color.h"
#include "draw.h"
#include "draw_rr.h"
#include "draw_rr_edges.h"
#include "draw_basic.h"
#include "draw_toggle_functions.h"
#include "draw_triangle.h"
#include "draw_searchbar.h"
#include "draw_mux.h"
#include "read_xml_arch_file.h"
#include "draw_global.h"
#include "intra_logic_block.h"
#include "move_utils.h"
#include "route_export.h"
#include "tatum/report/TimingPathCollector.hpp"

#ifdef VTR_ENABLE_DEBUG_LOGGING
#    include "move_utils.h"
#endif

#ifdef WIN32 /* For runtime tracking in WIN32. The clock() function defined in time.h will *
              * track CPU runtime.														   */
#    include <time.h>
#else /* For X11. The clock() function in time.h will not output correct time difference   *
       * for X11, because the graphics is processed by the Xserver rather than local CPU,  *
       * which means tracking CPU time will not be the same as the actual wall clock time. *
       * Thus, so use gettimeofday() in sys/time.h to track actual calendar time.          */
#    include <sys/time.h>
#endif

#ifndef NO_GRAPHICS

//To process key presses we need the X11 keysym definitions,
//which are unavailable when building with MINGW
#    if defined(X11) && !defined(__MINGW32__)
#        include <X11/keysym.h>
#    endif

#    include "rr_graph.h"
#    include "route_util.h"
#    include "place_macro.h"
#    include "buttons.h"

/****************************** Define Macros *******************************/
#    define DEFAULT_RR_NODE_COLOR ezgl::BLACK
#    define OLD_BLK_LOC_COLOR blk_GOLD
#    define NEW_BLK_LOC_COLOR blk_GREEN

constexpr float EMPTY_BLOCK_LIGHTEN_FACTOR = 0.20;

const std::vector<ezgl::color> kelly_max_contrast_colors = {
    //ezgl::color(242, 243, 244), //white: skip white since it doesn't contrast well with VPR's light background
    ezgl::color(34, 34, 34),    //black
    ezgl::color(243, 195, 0),   //yellow
    ezgl::color(135, 86, 146),  //purple
    ezgl::color(243, 132, 0),   //orange
    ezgl::color(161, 202, 241), //light blue
    ezgl::color(190, 0, 50),    //red
    ezgl::color(194, 178, 128), //buf
    ezgl::color(132, 132, 130), //gray
    ezgl::color(0, 136, 86),    //green
    ezgl::color(230, 143, 172), //purplish pink
    ezgl::color(0, 103, 165),   //blue
    ezgl::color(249, 147, 121), //yellowish pink
    ezgl::color(96, 78, 151),   //violet
    ezgl::color(246, 166, 0),   //orange yellow
    ezgl::color(179, 68, 108),  //purplish red
    ezgl::color(220, 211, 0),   //greenish yellow
    ezgl::color(136, 45, 23),   //redish brown
    ezgl::color(141, 182, 0),   //yellow green
    ezgl::color(101, 69, 34),   //yellowish brown
    ezgl::color(226, 88, 34),   //reddish orange
    ezgl::color(43, 61, 38)     //olive green
};

/* Draws the blocks placed on the proper clbs.  Occupied blocks are darker colours *
 * while empty ones are lighter colours and have a dashed border.  */
void drawplace(ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();
    t_draw_coords* draw_coords = get_draw_coords_vars();
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    ClusterBlockId bnum;
    int num_sub_tiles;

    g->set_line_width(0);
    for (size_t i = 0; i < device_ctx.grid.width(); i++) {
        for (size_t j = 0; j < device_ctx.grid.height(); j++) {
            /* Only the first block of a group should control drawing */
            if (device_ctx.grid[i][j].width_offset > 0
                || device_ctx.grid[i][j].height_offset > 0)
                continue;

            num_sub_tiles = device_ctx.grid[i][j].type->capacity;
            /* Don't draw if tile capacity is zero. eg-> corners. */
            if (num_sub_tiles == 0) {
                continue;
            }

            for (int k = 0; k < num_sub_tiles; ++k) {
                /* Look at the tile at start of large block */
                bnum = place_ctx.grid_blocks[i][j].blocks[k];
                /* Fill background for the clb. Do not fill if "show_blk_internal"
                 * is toggled.
                 */
                if (bnum == INVALID_BLOCK_ID)
                    continue;

                //Determine the block color and logical type
                ezgl::color block_color;
                t_logical_block_type_ptr logical_block_type = nullptr;

                //flag whether the current location is highlighted with a special color or not
                bool current_loc_is_highlighted = false;

                if (placer_breakpoint_reached())
                    current_loc_is_highlighted = highlight_loc_with_specific_color(int(i), int(j),
                                                                                   block_color);

                // No color specified at this location; use the block color.
                if (current_loc_is_highlighted == false) {
                    if (bnum != EMPTY_BLOCK_ID) {
                        block_color = draw_state->block_color(bnum);
                    } else {
                        block_color = get_block_type_color(
                            device_ctx.grid[i][j].type);
                        block_color = lighten_color(block_color,
                                                    EMPTY_BLOCK_LIGHTEN_FACTOR);
                    }
                }

                auto tile_type = device_ctx.grid[i][j].type;
                logical_block_type = pick_logical_type(tile_type);

                g->set_color(block_color);
                /* Get coords of current sub_tile */
                ezgl::rectangle abs_clb_bbox = draw_coords->get_absolute_clb_bbox(i, j, k,
                                                                                  logical_block_type);
                ezgl::point2d center = abs_clb_bbox.center();

                g->fill_rectangle(abs_clb_bbox);

                g->set_color(ezgl::BLACK);

                g->set_line_dash(
                    (EMPTY_BLOCK_ID == bnum) ? ezgl::line_dash::asymmetric_5_3 : ezgl::line_dash::none);
                if (draw_state->draw_block_outlines) {
                    g->draw_rectangle(abs_clb_bbox);
                }

                if (draw_state->draw_block_text) {
                    /* Draw text if the space has parts of the netlist */
                    if (bnum != EMPTY_BLOCK_ID && bnum != INVALID_BLOCK_ID) {
                        std::string name = cluster_ctx.clb_nlist.block_name(
                                               bnum)
                                           + vtr::string_fmt(" (#%zu)", size_t(bnum));

                        g->draw_text(center, name.c_str(), abs_clb_bbox.width(),
                                     abs_clb_bbox.height());
                    }
                    /* Draw text for block type so that user knows what block */
                    if (device_ctx.grid[i][j].width_offset == 0
                        && device_ctx.grid[i][j].height_offset == 0) {
                        std::string block_type_loc = device_ctx.grid[i][j].type->name;
                        block_type_loc += vtr::string_fmt(" (%d,%d)", i, j);

                        g->draw_text(
                            center
                                - ezgl::point2d(0,
                                                abs_clb_bbox.height() / 4),
                            block_type_loc.c_str(), abs_clb_bbox.width(),
                            abs_clb_bbox.height());
                    }
                }
            }
        }
    }
}

/* This routine draws the nets on the placement.  The nets have not *
 * yet been routed, so we just draw a chain showing a possible path *
 * for each net.  This gives some idea of future congestion.        */
void drawnets(ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();
    t_draw_coords* draw_coords = get_draw_coords_vars();

    ClusterBlockId b1, b2;
    auto& cluster_ctx = g_vpr_ctx.clustering();

    float NET_ALPHA = draw_state->net_alpha;

    g->set_line_dash(ezgl::line_dash::none);
    g->set_line_width(0);

    /* Draw the net as a star from the source to each sink. Draw from centers of *
     * blocks (or sub blocks in the case of IOs).                                */

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
            continue; /* Don't draw */

        g->set_color(draw_state->net_color[net_id],
                     draw_state->net_color[net_id].alpha * NET_ALPHA);
        b1 = cluster_ctx.clb_nlist.net_driver_block(net_id);
        ezgl::point2d driver_center = draw_coords->get_absolute_clb_bbox(b1,
                                                                         cluster_ctx.clb_nlist.block_type(b1))
                                          .center();
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            b2 = cluster_ctx.clb_nlist.pin_block(pin_id);
            ezgl::point2d sink_center = draw_coords->get_absolute_clb_bbox(b2,
                                                                           cluster_ctx.clb_nlist.block_type(b2))
                                            .center();
            g->draw_line(driver_center, sink_center);
            /* Uncomment to draw a chain instead of a star. */
            /* driver_center = sink_center;  */
        }
    }
}

/* Draws all the overused routing resources (i.e. congestion) in various contrasting colors showing congestion ratio.  */
void draw_congestion(ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();

    if (draw_state->show_congestion == DRAW_NO_CONGEST) {
        return;
    }

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    //Record min/max congestion
    float min_congestion_ratio = 1.;
    float max_congestion_ratio = min_congestion_ratio;
    std::vector<int> congested_rr_nodes = collect_congested_rr_nodes();
    for (int inode : congested_rr_nodes) {
        short occ = route_ctx.rr_node_route_inf[inode].occ();
        short capacity = rr_graph.node_capacity(RRNodeId(inode));

        float congestion_ratio = float(occ) / capacity;

        max_congestion_ratio = std::max(max_congestion_ratio, congestion_ratio);
    }

    char msg[vtr::bufsize];
    if (draw_state->show_congestion == DRAW_CONGESTED) {
        sprintf(msg, "RR Node Overuse ratio range (%.2f, %.2f]", min_congestion_ratio, max_congestion_ratio);
    } else {
        VTR_ASSERT(draw_state->show_congestion == DRAW_CONGESTED_WITH_NETS);
        sprintf(msg, "RR Node Overuse ratio range (%.2f, %.2f] (and congested nets)", min_congestion_ratio, max_congestion_ratio);
    }
    application.update_message(msg);

    std::shared_ptr<vtr::ColorMap> cmap = std::make_shared<vtr::PlasmaColorMap>(min_congestion_ratio, max_congestion_ratio);

    //Sort the nodes in ascending order of value for drawing, this ensures high
    //valued nodes are not overdrawn by lower value ones (e.g-> when zoomed-out far)
    auto cmp_ascending_acc_cost = [&](int lhs_node, int rhs_node) {
        short lhs_occ = route_ctx.rr_node_route_inf[lhs_node].occ();
        short lhs_capacity = rr_graph.node_capacity(RRNodeId(lhs_node));

        short rhs_occ = route_ctx.rr_node_route_inf[rhs_node].occ();
        short rhs_capacity = rr_graph.node_capacity(RRNodeId(rhs_node));

        float lhs_cong_ratio = float(lhs_occ) / lhs_capacity;
        float rhs_cong_ratio = float(rhs_occ) / rhs_capacity;

        return lhs_cong_ratio < rhs_cong_ratio;
    };
    std::sort(congested_rr_nodes.begin(), congested_rr_nodes.end(), cmp_ascending_acc_cost);

    if (draw_state->show_congestion == DRAW_CONGESTED_WITH_NETS) {
        auto rr_node_nets = collect_rr_node_nets();

        for (int inode : congested_rr_nodes) {
            for (ClusterNetId net : rr_node_nets[inode]) {
                ezgl::color color = kelly_max_contrast_colors[size_t(net) % kelly_max_contrast_colors.size()];
                draw_state->net_color[net] = color;
            }
        }
        g->set_line_width(0);
        drawroute(HIGHLIGHTED, g);

        //Reset colors
        for (int inode : congested_rr_nodes) {
            for (ClusterNetId net : rr_node_nets[inode]) {
                draw_state->net_color[net] = DEFAULT_RR_NODE_COLOR;
            }
        }
    } else {
        g->set_line_width(2);
    }

    //Draw each congested node
    for (int inode : congested_rr_nodes) {
        short occ = route_ctx.rr_node_route_inf[inode].occ();
        short capacity = rr_graph.node_capacity(RRNodeId(inode));

        float congestion_ratio = float(occ) / capacity;

        bool node_congested = (occ > capacity);
        VTR_ASSERT(node_congested);

        ezgl::color color = to_ezgl_color(cmap->color(congestion_ratio));

        switch (rr_graph.node_type(RRNodeId(inode))) {
            case CHANX: //fallthrough
            case CHANY:
                draw_rr_chan(inode, color, g);
                break;

            case IPIN: //fallthrough
            case OPIN:
                draw_rr_pin(inode, color, g);
                break;
            default:
                break;
        }
    }

    draw_state->color_map = std::move(cmap);
}

/* Draws routing resource nodes colored according to their congestion costs */
void draw_routing_costs(ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();

    /* show_routing_costs controls whether the total/sum of the costs or individual
     * cost components (base cost, accumulated cost, present cost) are shown, and
     * whether colours are proportional to the node's cost or the logarithm of
     * it's cost.*/
    if (draw_state->show_routing_costs == DRAW_NO_ROUTING_COSTS) {
        return;
    }

    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();
    g->set_line_width(0);

    VTR_ASSERT(!route_ctx.rr_node_route_inf.empty());

    float min_cost = std::numeric_limits<float>::infinity();
    float max_cost = -min_cost;
    std::vector<float> rr_node_costs(device_ctx.rr_graph.num_nodes(), 0.);

    for (const RRNodeId& rr_id : device_ctx.rr_graph.nodes()) {
        float cost = 0.;
        if (draw_state->show_routing_costs == DRAW_TOTAL_ROUTING_COSTS
            || draw_state->show_routing_costs
                   == DRAW_LOG_TOTAL_ROUTING_COSTS) {
            cost = get_single_rr_cong_cost((size_t)rr_id,
                                           get_draw_state_vars()->pres_fac);

        } else if (draw_state->show_routing_costs == DRAW_BASE_ROUTING_COSTS) {
            cost = get_single_rr_cong_base_cost((size_t)rr_id);

        } else if (draw_state->show_routing_costs == DRAW_ACC_ROUTING_COSTS
                   || draw_state->show_routing_costs
                          == DRAW_LOG_ACC_ROUTING_COSTS) {
            cost = get_single_rr_cong_acc_cost((size_t)rr_id);

        } else {
            VTR_ASSERT(
                draw_state->show_routing_costs == DRAW_PRES_ROUTING_COSTS
                || draw_state->show_routing_costs
                       == DRAW_LOG_PRES_ROUTING_COSTS);
            cost = get_single_rr_cong_pres_cost((size_t)rr_id,
                                                get_draw_state_vars()->pres_fac);
        }

        if (draw_state->show_routing_costs == DRAW_LOG_TOTAL_ROUTING_COSTS
            || draw_state->show_routing_costs == DRAW_LOG_ACC_ROUTING_COSTS
            || draw_state->show_routing_costs
                   == DRAW_LOG_PRES_ROUTING_COSTS) {
            cost = std::log(cost);
        }
        rr_node_costs[(size_t)rr_id] = cost;
        min_cost = std::min(min_cost, cost);
        max_cost = std::max(max_cost, cost);
    }

    //Hide min value, draw_rr_costs() ignores NaN's
    for (const RRNodeId& rr_id : device_ctx.rr_graph.nodes()) {
        if (rr_node_costs[(size_t)rr_id] == min_cost) {
            rr_node_costs[(size_t)rr_id] = NAN;
        }
    }
    char msg[vtr::bufsize];
    if (draw_state->show_routing_costs == DRAW_TOTAL_ROUTING_COSTS) {
        sprintf(msg, "Total Congestion Cost Range [%g, %g]", min_cost,
                max_cost);
    } else if (draw_state->show_routing_costs == DRAW_LOG_TOTAL_ROUTING_COSTS) {
        sprintf(msg, "Log Total Congestion Cost Range [%g, %g]", min_cost,
                max_cost);
    } else if (draw_state->show_routing_costs == DRAW_BASE_ROUTING_COSTS) {
        sprintf(msg, "Base Congestion Cost Range [%g, %g]", min_cost, max_cost);
    } else if (draw_state->show_routing_costs == DRAW_ACC_ROUTING_COSTS) {
        sprintf(msg, "Accumulated (Historical) Congestion Cost Range [%g, %g]",
                min_cost, max_cost);
    } else if (draw_state->show_routing_costs == DRAW_LOG_ACC_ROUTING_COSTS) {
        sprintf(msg,
                "Log Accumulated (Historical) Congestion Cost Range [%g, %g]",
                min_cost, max_cost);
    } else if (draw_state->show_routing_costs == DRAW_PRES_ROUTING_COSTS) {
        sprintf(msg, "Present Congestion Cost Range [%g, %g]", min_cost,
                max_cost);
    } else if (draw_state->show_routing_costs == DRAW_LOG_PRES_ROUTING_COSTS) {
        sprintf(msg, "Log Present Congestion Cost Range [%g, %g]", min_cost,
                max_cost);
    } else {
        sprintf(msg, "Cost Range [%g, %g]", min_cost, max_cost);
    }
    application.update_message(msg);

    draw_rr_costs(g, rr_node_costs, true);
}

/* Draws bounding box (BB) in which legal RR node start/end points must be contained */
void draw_routing_bb(ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();

    if (draw_state->show_routing_bb == OPEN) {
        return;
    }

    auto& route_ctx = g_vpr_ctx.routing();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    VTR_ASSERT(draw_state->show_routing_bb != OPEN);
    VTR_ASSERT(draw_state->show_routing_bb < (int)route_ctx.route_bb.size());

    t_draw_coords* draw_coords = get_draw_coords_vars();

    auto net_id = ClusterNetId(draw_state->show_routing_bb);
    const t_bb* bb = &route_ctx.route_bb[net_id];

    //The router considers an RR node to be 'within' the the bounding box if it
    //is *loosely* greater (i.e. greater than or equal) the left/bottom edges, and
    //it is *loosely* less (i.e. less than or equal) the right/top edges.
    //
    //In the graphics we represent this by drawing the BB so that legal RR node start/end points
    //are contained within the drawn box. Since VPR associates each x/y channel location to
    //the right/top of the tile with the same x/y cordinates, this means we draw the box so that:
    //  * The left edge is to the left of the channel at bb xmin (including the channel at xmin)
    //  * The bottom edge is to the below of the channel at bb ymin (including the channel at ymin)
    //  * The right edge is to the right of the channel at bb xmax (including the channel at xmax)
    //  * The top edge is to the right of the channel at bb ymax (including the channel at ymax)
    //Since tile_x/tile_y correspond to the drawing coordinates the block at grid x/y's bottom-left corner
    //this means we need to shift the top/right drawn co-ordinate one tile + channel width right/up so
    //the drawn box contains the top/right channels
    double draw_xlow = draw_coords->tile_x[bb->xmin];
    double draw_ylow = draw_coords->tile_y[bb->ymin];
    double draw_xhigh = draw_coords->tile_x[bb->xmax]
                        + 2 * draw_coords->get_tile_width();
    double draw_yhigh = draw_coords->tile_y[bb->ymax]
                        + 2 * draw_coords->get_tile_height();

    g->set_color(blk_RED);
    g->draw_rectangle({draw_xlow, draw_ylow}, {draw_xhigh, draw_yhigh});

    ezgl::color fill = blk_SKYBLUE;
    fill.alpha *= 0.3;
    g->set_color(fill);
    g->fill_rectangle({draw_xlow, draw_ylow}, {draw_xhigh, draw_yhigh});

    draw_routed_net(net_id, g);

    std::string msg;
    msg += "Showing BB";
    msg += " (" + std::to_string(bb->xmin) + ", " + std::to_string(bb->ymin)
           + ", " + std::to_string(bb->xmax) + ", " + std::to_string(bb->ymax)
           + ")";
    msg += " and routing for net '" + cluster_ctx.clb_nlist.net_name(net_id)
           + "'";
    msg += " (#" + std::to_string(size_t(net_id)) + ")";
    application.update_message(msg.c_str());
}

/* Draws an X centered at (x,y). The width and height of the X are each 2 * size. */
void draw_x(float x, float y, float size, ezgl::renderer* g) {
    g->draw_line({x - size, y + size}, {x + size, y - size});
    g->draw_line({x - size, y - size}, {x + size, y + size});
}

/* Draws the nets in the positions fixed by the router.  If draw_net_type is *
 * ALL_NETS, draw all the nets.  If it is HIGHLIGHTED, draw only the nets    *
 * that are not coloured black (useful for drawing over the rr_graph).       */
void drawroute(enum e_draw_net_type draw_net_type, ezgl::renderer* g) {
    /* Next free track in each channel segment if routing is GLOBAL */

    auto& cluster_ctx = g_vpr_ctx.clustering();

    t_draw_state* draw_state = get_draw_state_vars();

    float NET_ALPHA = draw_state->net_alpha;

    g->set_line_dash(ezgl::line_dash::none);
    g->set_color(ezgl::BLACK, ezgl::BLACK.alpha * NET_ALPHA);

    /* Now draw each net, one by one.      */

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (draw_net_type == HIGHLIGHTED
            && draw_state->net_color[net_id] == ezgl::BLACK)
            continue;

        draw_routed_net(net_id, g);
    } /* End for (each net) */
}

void draw_routed_net(ClusterNetId net_id, ezgl::renderer* g) {
    auto& route_ctx = g_vpr_ctx.routing();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    t_draw_state* draw_state = get_draw_state_vars();

    if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) /* Don't draw. */
        return;

    if (route_ctx.trace[net_id].head == nullptr) /* No routing->  Skip.  (Allows me to draw */
        return;                                  /* partially complete routes).            */

    t_trace* tptr = route_ctx.trace[net_id].head; /* SOURCE to start */
    int inode = tptr->index;

    std::vector<int> rr_nodes_to_draw;
    rr_nodes_to_draw.push_back(inode);
    for (;;) {
        tptr = tptr->next;
        inode = tptr->index;

        if (draw_if_net_highlighted(net_id)) {
            /* If a net has been highlighted, highlight the whole net in *
             * the same color.											 */
            draw_state->draw_rr_node[inode].color = draw_state->net_color[net_id];
            draw_state->draw_rr_node[inode].node_highlighted = true;
        } else {
            /* If not highlighted, draw the node in default color. */
            draw_state->draw_rr_node[inode].color = DEFAULT_RR_NODE_COLOR;
        }

        rr_nodes_to_draw.push_back(inode);

        if (tptr->iswitch == OPEN) { //End of branch
            draw_partial_route(rr_nodes_to_draw, g);
            rr_nodes_to_draw.clear();

            /* Skip the next segment */
            tptr = tptr->next;
            if (tptr == nullptr)
                break;
            inode = tptr->index;
            rr_nodes_to_draw.push_back(inode);
        }

    } /* End loop over traceback. */

    draw_partial_route(rr_nodes_to_draw, g);
}

//Draws the set of rr_nodes specified, using the colors set in draw_state
void draw_partial_route(const std::vector<int>& rr_nodes_to_draw, ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    static vtr::OffsetMatrix<int> chanx_track; /* [1..device_ctx.grid.width() - 2][0..device_ctx.grid.height() - 2] */
    static vtr::OffsetMatrix<int> chany_track; /* [0..device_ctx.grid.width() - 2][1..device_ctx.grid.height() - 2] */
    if (draw_state->draw_route_type == GLOBAL) {
        /* Allocate some temporary storage if it's not already available. */
        size_t width = device_ctx.grid.width();
        size_t height = device_ctx.grid.height();
        if (chanx_track.empty()) {
            chanx_track = vtr::OffsetMatrix<int>({{{1, width - 1}, {0, height - 1}}});
        }

        if (chany_track.empty()) {
            chany_track = vtr::OffsetMatrix<int>({{{0, width - 1}, {1, height - 1}}});
        }

        for (size_t i = 1; i < width - 1; i++)
            for (size_t j = 0; j < height - 1; j++)
                chanx_track[i][j] = (-1);

        for (size_t i = 0; i < width - 1; i++)
            for (size_t j = 1; j < height - 1; j++)
                chany_track[i][j] = (-1);
    }

    for (size_t i = 1; i < rr_nodes_to_draw.size(); ++i) {
        int inode = rr_nodes_to_draw[i];
        RRNodeId rr_node = RRNodeId(inode);
        auto rr_type = rr_graph.node_type(rr_node);

        int prev_node = rr_nodes_to_draw[i - 1];
        auto prev_type = rr_graph.node_type(RRNodeId(prev_node));

        auto iedge = find_edge(prev_node, inode);
        auto switch_type = rr_graph.edge_switch(RRNodeId(prev_node), iedge);

        switch (rr_type) {
            case OPIN: {
                draw_rr_pin(inode, draw_state->draw_rr_node[inode].color, g);
                break;
            }
            case IPIN: {
                draw_rr_pin(inode, draw_state->draw_rr_node[inode].color, g);
                if (rr_graph.node_type(RRNodeId(prev_node)) == OPIN) {
                    draw_pin_to_pin(prev_node, inode, g);
                } else {
                    draw_pin_to_chan_edge(inode, prev_node, g);
                }
                break;
            }
            case CHANX: {
                if (draw_state->draw_route_type == GLOBAL)
                    chanx_track[rr_graph.node_xlow(rr_node)][rr_graph.node_ylow(rr_node)]++;

                int itrack = get_track_num(inode, chanx_track, chany_track);
                draw_rr_chan(inode, draw_state->draw_rr_node[inode].color, g);

                switch (prev_type) {
                    case CHANX: {
                        draw_chanx_to_chanx_edge(RRNodeId(prev_node), RRNodeId(inode),
                                                 itrack, switch_type, g);
                        break;
                    }
                    case CHANY: {
                        int prev_track = get_track_num(prev_node, chanx_track,
                                                       chany_track);
                        draw_chanx_to_chany_edge(inode, itrack, prev_node,

                                                 prev_track, FROM_Y_TO_X, switch_type, g);
                        break;
                    }
                    case OPIN: {
                        draw_pin_to_chan_edge(prev_node, inode, g);
                        break;
                    }
                    default: {
                        VPR_ERROR(VPR_ERROR_OTHER,
                                  "Unexpected connection from an rr_node of type %d to one of type %d.\n",
                                  prev_type, rr_type);
                    }
                }

                break;
            }
            case CHANY: {
                if (draw_state->draw_route_type == GLOBAL)
                    chany_track[rr_graph.node_xlow(rr_node)][rr_graph.node_ylow(rr_node)]++;

                int itrack = get_track_num(inode, chanx_track, chany_track);
                draw_rr_chan(inode, draw_state->draw_rr_node[inode].color, g);

                switch (prev_type) {
                    case CHANX: {
                        int prev_track = get_track_num(prev_node, chanx_track,
                                                       chany_track);
                        draw_chanx_to_chany_edge(prev_node, prev_track, inode,
                                                 itrack, FROM_X_TO_Y, switch_type, g);
                        break;
                    }
                    case CHANY: {
                        draw_chany_to_chany_edge(RRNodeId(prev_node), RRNodeId(inode),
                                                 itrack, switch_type, g);
                        break;
                    }
                    case OPIN: {
                        draw_pin_to_chan_edge(prev_node, inode, g);

                        break;
                    }
                    default: {
                        VPR_ERROR(VPR_ERROR_OTHER,
                                  "Unexpected connection from an rr_node of type %d to one of type %d.\n",
                                  prev_type, rr_type);
                    }
                }

                break;
            }
            default: {
                break;
            }
        }
    }
}

/* Draws any placement macros (e.g. carry chains, which require specific relative placements
 * between some blocks) if the Placement Macros (in the GUI) is seelected.
 */
void draw_placement_macros(ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();

    if (draw_state->show_placement_macros == DRAW_NO_PLACEMENT_MACROS) {
        return;
    }
    t_draw_coords* draw_coords = get_draw_coords_vars();

    auto& place_ctx = g_vpr_ctx.placement();
    for (size_t imacro = 0; imacro < place_ctx.pl_macros.size(); ++imacro) {
        const t_pl_macro* pl_macro = &place_ctx.pl_macros[imacro];

        //TODO: for now we just draw the bounding box of the macro, which is incorrect for non-rectangular macros...
        int xlow = std::numeric_limits<int>::max();
        int ylow = std::numeric_limits<int>::max();
        int xhigh = std::numeric_limits<int>::min();
        int yhigh = std::numeric_limits<int>::min();

        int x_root = OPEN;
        int y_root = OPEN;
        for (size_t imember = 0; imember < pl_macro->members.size();
             ++imember) {
            const t_pl_macro_member* member = &pl_macro->members[imember];

            ClusterBlockId blk = member->blk_index;

            if (imember == 0) {
                x_root = place_ctx.block_locs[blk].loc.x;
                y_root = place_ctx.block_locs[blk].loc.y;
            }

            int x = x_root + member->offset.x;
            int y = y_root + member->offset.y;

            xlow = std::min(xlow, x);
            ylow = std::min(ylow, y);
            xhigh = std::max(xhigh, x + physical_tile_type(blk)->width);
            yhigh = std::max(yhigh, y + physical_tile_type(blk)->height);
        }

        double draw_xlow = draw_coords->tile_x[xlow];
        double draw_ylow = draw_coords->tile_y[ylow];
        double draw_xhigh = draw_coords->tile_x[xhigh];
        double draw_yhigh = draw_coords->tile_y[yhigh];

        g->set_color(blk_RED);
        g->draw_rectangle({draw_xlow, draw_ylow},
                          {draw_xhigh, draw_yhigh});

        ezgl::color fill = blk_SKYBLUE;
        fill.alpha *= 0.3;
        g->set_color(fill);
        g->fill_rectangle({draw_xlow, draw_ylow},
                          {draw_xhigh, draw_yhigh});
    }
}

/* Draws a heat map of routing wire utilization (i.e. fraction of wires used in each channel)
 * when a routing is shown on-screen and Routing Util (on the GUI) is selected.
 * Lighter colours (e.g. yellow) correspond to highly utilized
 * channels, while darker colours (e.g. blue) correspond to lower utilization.*/
void draw_routing_util(ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();
    if (draw_state->show_routing_util == DRAW_NO_ROUTING_UTIL) {
        return;
    }

    t_draw_coords* draw_coords = get_draw_coords_vars();
    auto& device_ctx = g_vpr_ctx.device();

    auto chanx_usage = calculate_routing_usage(CHANX);
    auto chany_usage = calculate_routing_usage(CHANY);

    auto chanx_avail = calculate_routing_avail(CHANX);
    auto chany_avail = calculate_routing_avail(CHANY);

    float min_util = 0.;
    float max_util = -std::numeric_limits<float>::infinity();
    for (size_t x = 0; x < device_ctx.grid.width() - 1; ++x) {
        for (size_t y = 0; y < device_ctx.grid.height() - 1; ++y) {
            max_util = std::max(max_util,
                                routing_util(chanx_usage[x][y], chanx_avail[x][y]));
            max_util = std::max(max_util,
                                routing_util(chany_usage[x][y], chany_avail[x][y]));
        }
    }
    max_util = std::max(max_util, 1.f);

    std::unique_ptr<vtr::ColorMap> cmap;

    if (draw_state->clip_routing_util) {
        cmap = std::make_unique<vtr::PlasmaColorMap>(0., 1.);
    } else {
        cmap = std::make_unique<vtr::PlasmaColorMap>(min_util, max_util);
    }

    float tile_width = draw_coords->get_tile_width();
    float tile_height = draw_coords->get_tile_height();

    float ALPHA = 0.95;
    if (draw_state->show_routing_util == DRAW_ROUTING_UTIL_OVER_BLOCKS) {
        ALPHA = 1.;
    }

    for (size_t x = 0; x < device_ctx.grid.width() - 1; ++x) {
        for (size_t y = 0; y < device_ctx.grid.height() - 1; ++y) {
            float sb_util = 0;
            float chanx_util = 0;
            float chany_util = 0;
            int chan_count = 0;
            if (x > 0) {
                chanx_util = routing_util(chanx_usage[x][y], chanx_avail[x][y]);
                if (draw_state->clip_routing_util) {
                    chanx_util = std::min(chanx_util, 1.f);
                }
                ezgl::color chanx_color = to_ezgl_color(
                    cmap->color(chanx_util));
                chanx_color.alpha *= ALPHA;
                g->set_color(chanx_color);
                ezgl::rectangle bb(
                    {draw_coords->tile_x[x], draw_coords->tile_y[y]
                                                 + 1 * tile_height},
                    {draw_coords->tile_x[x] + 1 * tile_width,
                     draw_coords->tile_y[y + 1]});
                g->fill_rectangle(bb);

                g->set_color(ezgl::BLACK);
                if (draw_state->show_routing_util
                    == DRAW_ROUTING_UTIL_WITH_VALUE) {
                    g->draw_text(bb.center(),
                                 vtr::string_fmt("%.2f", chanx_util).c_str(),
                                 bb.width(), bb.height());
                } else if (draw_state->show_routing_util
                           == DRAW_ROUTING_UTIL_WITH_FORMULA) {
                    g->draw_text(bb.center(),
                                 vtr::string_fmt("%.2f = %.0f / %.0f", chanx_util,
                                                 chanx_usage[x][y], chanx_avail[x][y])
                                     .c_str(),
                                 bb.width(), bb.height());
                }

                sb_util += chanx_util;
                ++chan_count;
            }

            if (y > 0) {
                chany_util = routing_util(chany_usage[x][y], chany_avail[x][y]);
                if (draw_state->clip_routing_util) {
                    chany_util = std::min(chany_util, 1.f);
                }
                ezgl::color chany_color = to_ezgl_color(
                    cmap->color(chany_util));
                chany_color.alpha *= ALPHA;
                g->set_color(chany_color);
                ezgl::rectangle bb({draw_coords->tile_x[x] + 1 * tile_width,
                                    draw_coords->tile_y[y]},
                                   {draw_coords->tile_x[x + 1], draw_coords->tile_y[y]
                                                                    + 1 * tile_height});
                g->fill_rectangle(bb);

                g->set_color(ezgl::BLACK);
                if (draw_state->show_routing_util
                    == DRAW_ROUTING_UTIL_WITH_VALUE) {
                    g->draw_text(bb.center(),
                                 vtr::string_fmt("%.2f", chany_util).c_str(),
                                 bb.width(), bb.height());
                } else if (draw_state->show_routing_util
                           == DRAW_ROUTING_UTIL_WITH_FORMULA) {
                    g->draw_text(bb.center(),
                                 vtr::string_fmt("%.2f = %.0f / %.0f", chany_util,
                                                 chany_usage[x][y], chany_avail[x][y])
                                     .c_str(),
                                 bb.width(), bb.height());
                }

                sb_util += chany_util;
                ++chan_count;
            }

            //For now SB util is just average of surrounding channels
            //TODO: calculate actual usage
            sb_util += routing_util(chanx_usage[x + 1][y],
                                    chanx_avail[x + 1][y]);
            chan_count += 1;
            sb_util += routing_util(chany_usage[x][y + 1],
                                    chany_avail[x][y + 1]);
            chan_count += 1;

            VTR_ASSERT(chan_count > 0);
            sb_util /= chan_count;
            if (draw_state->clip_routing_util) {
                sb_util = std::min(sb_util, 1.f);
            }
            ezgl::color sb_color = to_ezgl_color(cmap->color(sb_util));
            sb_color.alpha *= ALPHA;
            g->set_color(sb_color);
            ezgl::rectangle bb(
                {draw_coords->tile_x[x] + 1 * tile_width,
                 draw_coords->tile_y[y] + 1 * tile_height},
                {draw_coords->tile_x[x + 1], draw_coords->tile_y[y + 1]});
            g->fill_rectangle(bb);

            //Draw over blocks
            if (draw_state->show_routing_util
                == DRAW_ROUTING_UTIL_OVER_BLOCKS) {
                if (x < device_ctx.grid.width() - 2
                    && y < device_ctx.grid.height() - 2) {
                    ezgl::rectangle bb2({draw_coords->tile_x[x + 1],
                                         draw_coords->tile_y[y + 1]},
                                        {draw_coords->tile_x[x + 1] + 1 * tile_width,
                                         draw_coords->tile_y[y + 1] + 1 * tile_width});
                    g->fill_rectangle(bb2);
                }
            }
            g->set_color(ezgl::BLACK);
            if (draw_state->show_routing_util == DRAW_ROUTING_UTIL_WITH_VALUE
                || draw_state->show_routing_util
                       == DRAW_ROUTING_UTIL_WITH_FORMULA) {
                g->draw_text(bb.center(),
                             vtr::string_fmt("%.2f", sb_util).c_str(), bb.width(),
                             bb.height());
            }
        }
    }

    draw_state->color_map = std::move(cmap);
}

/* Draws the critical path if Crit. Path (in the GUI) is selected. Each stage between primitive
 * pins is shown in a different colour.
 * User can toggle between two different visualizations:
 * a) during placement, critical path only shown as flylines
 * b) during routing, critical path is shown by both flylines and routed net connections.
 */
void draw_crit_path(ezgl::renderer* g) {
    tatum::TimingPathCollector path_collector;

    t_draw_state* draw_state = get_draw_state_vars();
    auto& timing_ctx = g_vpr_ctx.timing();

    if (draw_state->show_crit_path == DRAW_NO_CRIT_PATH) {
        return;
    }

    if (!draw_state->setup_timing_info) {
        return; //No timing to draw
    }

    //Get the worst timing path
    auto paths = path_collector.collect_worst_setup_timing_paths(
        *timing_ctx.graph,
        *(draw_state->setup_timing_info->setup_analyzer()), 1);
    tatum::TimingPath path = paths[0];

    //Walk through the timing path drawing each edge
    tatum::NodeId prev_node;
    float prev_arr_time = std::numeric_limits<float>::quiet_NaN();
    int i = 0;
    for (tatum::TimingPathElem elem : path.data_arrival_path().elements()) {
        tatum::NodeId node = elem.node();
        float arr_time = elem.tag().time();
        if (prev_node) {
            //We draw each 'edge' in a different color, this allows users to identify the stages and
            //any routing which corresponds to the edge
            //
            //We pick colors from the kelly max-contrast list, for long paths there may be repeats
            ezgl::color color = kelly_max_contrast_colors[i++
                                                          % kelly_max_contrast_colors.size()];

            float delay = arr_time - prev_arr_time;
            if (draw_state->show_crit_path == DRAW_CRIT_PATH_FLYLINES
                || draw_state->show_crit_path
                       == DRAW_CRIT_PATH_FLYLINES_DELAYS) {
                g->set_color(color);
                g->set_line_dash(ezgl::line_dash::none);
                g->set_line_width(4);
                draw_flyline_timing_edge(tnode_draw_coord(prev_node),
                                         tnode_draw_coord(node), delay, g);
            } else {
                VTR_ASSERT(draw_state->show_crit_path != DRAW_NO_CRIT_PATH);

                //Draw the routed version of the timing edge
                draw_routed_timing_edge(prev_node, node, delay, color, g);
            }
        }
        prev_node = node;
        prev_arr_time = arr_time;
    }
}

//Draws critical path shown as flylines.
void draw_flyline_timing_edge(ezgl::point2d start, ezgl::point2d end, float incr_delay, ezgl::renderer* g) {
    g->draw_line(start, end);
    draw_triangle_along_line(g, start, end, 0.95, 40 * DEFAULT_ARROW_SIZE);
    draw_triangle_along_line(g, start, end, 0.05, 40 * DEFAULT_ARROW_SIZE);

    bool draw_delays = (get_draw_state_vars()->show_crit_path
                            == DRAW_CRIT_PATH_FLYLINES_DELAYS
                        || get_draw_state_vars()->show_crit_path
                               == DRAW_CRIT_PATH_ROUTING_DELAYS);
    if (draw_delays) {
        //Determine the strict bounding box based on the lines start/end
        float min_x = std::min(start.x, end.x);
        float max_x = std::max(start.x, end.x);
        float min_y = std::min(start.y, end.y);
        float max_y = std::max(start.y, end.y);

        //If we have a nearly horizontal/vertical line the bbox is too
        //small to draw the text, so widen it by a tile (i.e. CLB) width
        float tile_width = get_draw_coords_vars()->get_tile_width();
        if (max_x - min_x < tile_width) {
            max_x += tile_width / 2;
            min_x -= tile_width / 2;
        }
        if (max_y - min_y < tile_width) {
            max_y += tile_width / 2;
            min_y -= tile_width / 2;
        }

        //TODO: draw the delays nicer
        //   * rotate to match edge
        //   * offset from line
        //   * track visible in window
        ezgl::rectangle text_bbox({min_x, min_y}, {max_x, max_y});

        std::stringstream ss;
        ss.precision(3);
        ss << 1e9 * incr_delay; //In nanoseconds
        std::string incr_delay_str = ss.str();

        // Get the angle of line, to rotate the text
        float text_angle = (180 / M_PI)
                           * atan((end.y - start.y) / (end.x - start.x));

        // Get the screen coordinates for text drawing
        ezgl::rectangle screen_coords = g->world_to_screen(text_bbox);
        g->set_text_rotation(text_angle);

        // Set the text colour to black to differentiate it from the line
        g->set_font_size(16);
        g->set_color(ezgl::color(0, 0, 0));

        g->set_coordinate_system(ezgl::SCREEN);

        // Find an offset so it is sitting on top/below of the line
        float x_offset = screen_coords.center().x
                         - 8 * sin(text_angle * (M_PI / 180));
        float y_offset = screen_coords.center().y
                         - 8 * cos(text_angle * (M_PI / 180));

        ezgl::point2d offset_text_bbox(x_offset, y_offset);
        g->draw_text(offset_text_bbox, incr_delay_str.c_str(),
                     text_bbox.width(), text_bbox.height());

        g->set_font_size(14);

        g->set_text_rotation(0);
        g->set_coordinate_system(ezgl::WORLD);
    }
}

//Draws critical path shown by both flylines and routed net connections.
void draw_routed_timing_edge(tatum::NodeId start_tnode,
                             tatum::NodeId end_tnode,
                             float incr_delay,
                             ezgl::color color,
                             ezgl::renderer* g) {
    draw_routed_timing_edge_connection(start_tnode, end_tnode, color, g);

    g->set_line_dash(ezgl::line_dash::asymmetric_5_3);
    g->set_line_width(3);
    g->set_color(color);

    draw_flyline_timing_edge((ezgl::point2d)tnode_draw_coord(start_tnode),
                             (ezgl::point2d)tnode_draw_coord(end_tnode), (float)incr_delay,
                             (ezgl::renderer*)g);

    g->set_line_width(0);
    g->set_line_dash(ezgl::line_dash::none);
}

//Collect all the drawing locations associated with the timing edge between start and end
void draw_routed_timing_edge_connection(tatum::NodeId src_tnode,
                                        tatum::NodeId sink_tnode,
                                        ezgl::color color,
                                        ezgl::renderer* g) {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& timing_ctx = g_vpr_ctx.timing();

    AtomPinId atom_src_pin = atom_ctx.lookup.tnode_atom_pin(src_tnode);
    AtomPinId atom_sink_pin = atom_ctx.lookup.tnode_atom_pin(sink_tnode);

    std::vector<ezgl::point2d> points;
    points.push_back(atom_pin_draw_coord(atom_src_pin));

    tatum::EdgeId tedge = timing_ctx.graph->find_edge(src_tnode, sink_tnode);
    tatum::EdgeType edge_type = timing_ctx.graph->edge_type(tedge);

    ClusterNetId net_id = ClusterNetId::INVALID();

    //We currently only trace interconnect edges in detail, and treat all others
    //as flylines
    if (edge_type == tatum::EdgeType::INTERCONNECT) {
        //All atom pins are implemented inside CLBs, so next hop is to the top-level CLB pins

        //TODO: most of this code is highly similar to code in PostClusterDelayCalculator, refactor
        //      into a common method for walking the clustered netlist, this would also (potentially)
        //      allow us to grab the component delays
        AtomBlockId atom_src_block = atom_ctx.nlist.pin_block(atom_src_pin);
        AtomBlockId atom_sink_block = atom_ctx.nlist.pin_block(atom_sink_pin);

        ClusterBlockId clb_src_block = atom_ctx.lookup.atom_clb(atom_src_block);
        VTR_ASSERT(clb_src_block != ClusterBlockId::INVALID());
        ClusterBlockId clb_sink_block = atom_ctx.lookup.atom_clb(
            atom_sink_block);
        VTR_ASSERT(clb_sink_block != ClusterBlockId::INVALID());

        const t_pb_graph_pin* sink_gpin = atom_ctx.lookup.atom_pin_pb_graph_pin(
            atom_sink_pin);
        VTR_ASSERT(sink_gpin);

        int sink_pb_route_id = sink_gpin->pin_count_in_cluster;

        int sink_block_pin_index = -1;
        int sink_net_pin_index = -1;

        std::tie(net_id, sink_block_pin_index, sink_net_pin_index) = find_pb_route_clb_input_net_pin(clb_sink_block,
                                                                                                     sink_pb_route_id);
        if (net_id != ClusterNetId::INVALID() && sink_block_pin_index != -1
            && sink_net_pin_index != -1) {
            //Connection leaves the CLB
            //Now that we have the CLB source and sink pins, we need to grab all the points on the routing connecting the pins
            VTR_ASSERT(
                cluster_ctx.clb_nlist.net_driver_block(net_id)
                == clb_src_block);

            std::vector<int> routed_rr_nodes = trace_routed_connection_rr_nodes(
                net_id, 0, sink_net_pin_index);

            //Mark all the nodes highlighted
            t_draw_state* draw_state = get_draw_state_vars();
            for (int inode : routed_rr_nodes) {
                draw_state->draw_rr_node[inode].color = color;
            }

            draw_partial_route((std::vector<int>)routed_rr_nodes,
                               (ezgl::renderer*)g);
        } else {
            //Connection entirely within the CLB, we don't draw the internal routing so treat it as a fly-line
            VTR_ASSERT(clb_src_block == clb_sink_block);
        }
    }

    points.push_back(atom_pin_draw_coord(atom_sink_pin));
}

void draw_color_map_legend(const vtr::ColorMap& cmap,
                           ezgl::renderer* g) {
    constexpr float LEGEND_WIDTH_FAC = 0.075;
    constexpr float LEGEND_VERT_OFFSET_FAC = 0.05;
    constexpr float TEXT_OFFSET = 10;
    constexpr size_t NUM_COLOR_POINTS = 1000;

    g->set_coordinate_system(ezgl::SCREEN);

    float screen_width = application.get_canvas(
                                        application.get_main_canvas_id())
                             ->width();
    float screen_height = application.get_canvas(
                                         application.get_main_canvas_id())
                              ->height();
    float vert_offset = screen_height * LEGEND_VERT_OFFSET_FAC;
    float legend_width = std::min<int>(LEGEND_WIDTH_FAC * screen_width, 100);

    // In SCREEN coordinate: bottom_left is (0,0), right_top is (screen_width, screen_height)
    ezgl::rectangle legend({0, vert_offset},
                           {legend_width, screen_height - vert_offset});

    float range = cmap.max() - cmap.min();
    float height_incr = legend.height() / float(NUM_COLOR_POINTS);
    for (size_t i = 0; i < NUM_COLOR_POINTS; ++i) {
        float val = cmap.min() + (float(i) / NUM_COLOR_POINTS) * range;
        ezgl::color color = to_ezgl_color(cmap.color(val));

        g->set_color(color);
        g->fill_rectangle({legend.left(), legend.top() - i * height_incr}, {legend.right(), legend.top() - (i + 1) * height_incr});
    }

    //Min mark
    g->set_color(blk_SKYBLUE); // set to skyblue so its easier to see
    std::string str = vtr::string_fmt("%.3g", cmap.min());
    g->draw_text({legend.center_x(), legend.top() - TEXT_OFFSET},
                 str.c_str());

    //Mid marker
    g->set_color(ezgl::BLACK);
    str = vtr::string_fmt("%.3g", cmap.min() + (cmap.range() / 2.));
    g->draw_text({legend.center_x(), legend.center_y()}, str.c_str());

    //Max marker
    g->set_color(ezgl::BLACK);
    str = vtr::string_fmt("%.3g", cmap.max());
    g->draw_text({legend.center_x(), legend.bottom() + TEXT_OFFSET},
                 str.c_str());

    g->set_color(ezgl::BLACK);
    g->draw_rectangle(legend);

    g->set_coordinate_system(ezgl::WORLD);
}

void draw_block_pin_util() {
    t_draw_state* draw_state = get_draw_state_vars();
    if (draw_state->show_blk_pin_util == DRAW_NO_BLOCK_PIN_UTIL)
        return;

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    std::map<t_physical_tile_type_ptr, size_t> total_input_pins;
    std::map<t_physical_tile_type_ptr, size_t> total_output_pins;
    for (const auto& type : device_ctx.physical_tile_types) {
        if (is_empty_type(&type)) {
            continue;
        }

        total_input_pins[&type] = type.num_input_pins + type.num_clock_pins;
        total_output_pins[&type] = type.num_output_pins;
    }

    auto blks = cluster_ctx.clb_nlist.blocks();
    vtr::vector<ClusterBlockId, float> pin_util(blks.size());
    for (auto blk : blks) {
        auto type = physical_tile_type(blk);

        if (draw_state->show_blk_pin_util == DRAW_BLOCK_PIN_UTIL_TOTAL) {
            pin_util[blk] = cluster_ctx.clb_nlist.block_pins(blk).size()
                            / float(total_input_pins[type] + total_output_pins[type]);
        } else if (draw_state->show_blk_pin_util
                   == DRAW_BLOCK_PIN_UTIL_INPUTS) {
            pin_util[blk] = (cluster_ctx.clb_nlist.block_input_pins(blk).size()
                             + cluster_ctx.clb_nlist.block_clock_pins(blk).size())
                            / float(total_input_pins[type]);
        } else if (draw_state->show_blk_pin_util
                   == DRAW_BLOCK_PIN_UTIL_OUTPUTS) {
            pin_util[blk] = (cluster_ctx.clb_nlist.block_output_pins(blk).size())
                            / float(total_output_pins[type]);
        } else {
            VTR_ASSERT(false);
        }
    }

    std::unique_ptr<vtr::ColorMap> cmap = std::make_unique<vtr::PlasmaColorMap>(
        0., 1.);

    for (auto blk : blks) {
        ezgl::color color = to_ezgl_color(cmap->color(pin_util[blk]));
        draw_state->set_block_color(blk, color);
    }

    draw_state->color_map = std::move(cmap);

    if (draw_state->show_blk_pin_util == DRAW_BLOCK_PIN_UTIL_TOTAL) {
        application.update_message("Block Total Pin Utilization");
    } else if (draw_state->show_blk_pin_util == DRAW_BLOCK_PIN_UTIL_INPUTS) {
        application.update_message("Block Input Pin Utilization");

    } else if (draw_state->show_blk_pin_util == DRAW_BLOCK_PIN_UTIL_OUTPUTS) {
        application.update_message("Block Output Pin Utilization");
    } else {
        VTR_ASSERT(false);
    }
}

void draw_reset_blk_colors() {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto blks = cluster_ctx.clb_nlist.blocks();
    for (auto blk : blks) {
        draw_reset_blk_color(blk);
    }
}

void draw_reset_blk_color(ClusterBlockId blk_id) {
    t_draw_state* draw_state = get_draw_state_vars();
    draw_state->reset_block_color(blk_id);
}

#endif
