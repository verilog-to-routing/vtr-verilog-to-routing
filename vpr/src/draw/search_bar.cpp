#ifndef NO_GRAPHICS

#    include <cstdio>
#    include <sstream>

#    include "vtr_assert.h"
#    include "vtr_ndoffsetmatrix.h"
#    include "vtr_memory.h"
#    include "vtr_log.h"
#    include "vtr_color_map.h"

#    include "vpr_utils.h"
#    include "vpr_error.h"

#    include "globals.h"
#    include "draw_color.h"
#    include "draw.h"
#    include "read_xml_arch_file.h"
#    include "draw_global.h"
#    include "intra_logic_block.h"
#    include "atom_netlist.h"
#    include "tatum/report/TimingPathCollector.hpp"
#    include "hsl.h"
#    include "route_export.h"
#    include "search_bar.h"

#    ifdef WIN32 /* For runtime tracking in WIN32. The clock() function defined in time.h will *
                  * track CPU runtime.														   */
#        include <time.h>
#    else /* For X11. The clock() function in time.h will not output correct time difference   *
           * for X11, because the graphics is processed by the Xserver rather than local CPU,  *
           * which means tracking CPU time will not be the same as the actual wall clock time. *
           * Thus, so use gettimeofday() in sys/time.h to track actual calendar time.          */
#        include <sys/time.h>
#    endif

//To process key presses we need the X11 keysym definitions,
//which are unavailable when building with MINGW
#    if defined(X11) && !defined(__MINGW32__)
#        include <X11/keysym.h>
#    endif

#    include "rr_graph.h"
#    include "route_util.h"
#    include "place_macro.h"

extern std::string rr_highlight_message;

void search_and_highlight(GtkWidget* /*widget*/, ezgl::application* app) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    // get ID from search bar
    GtkEntry* text_entry = (GtkEntry*)app->get_object("TextInput");
    const char* text = gtk_entry_get_text(text_entry);
    std::string user_input = text;
    std::stringstream ss(user_input);

    GObject* combo_box = (GObject*)app->get_object("SearchType");
    gchar* type = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo_box));
    std::string search_type(type);

    // reset
    deselect_all();

    if (search_type == "RR Node ID") {
        int rr_node_id = -1;
        ss >> rr_node_id;

        // valid rr node id check
        if (rr_node_id < 0 || rr_node_id >= int(device_ctx.rr_nodes.size())) {
            warning_dialog_box("Invalid RR Node ID");
            app->refresh_drawing();
            return;
        }

        highlight_rr_nodes(rr_node_id);
        auto_zoom_rr_node(rr_node_id);
    }

    else if (search_type == "Block ID") {
        int block_id = -1;
        ss >> block_id;

        // valid block id check
        if (!cluster_ctx.clb_nlist.valid_block_id(ClusterBlockId(block_id))) {
            warning_dialog_box("Invalid Block ID");
            app->refresh_drawing();
            return;
        }

        highlight_blocks((ClusterBlockId)block_id);
    }

    else if (search_type == "Block Name") {
        std::string block_name = "";
        ss >> block_name;

        highlight_blocks((std::string)block_name);
    }

    else if (search_type == "Net ID") {
        int net_id = -1;
        ss >> net_id;

        // valid net id check
        if (!cluster_ctx.clb_nlist.valid_net_id(ClusterNetId(net_id))) {
            warning_dialog_box("Invalid Net ID");
            app->refresh_drawing();
            return;
        }

        highlight_nets((ClusterNetId)net_id);
    }

    else if (search_type == "Net Name") {
        std::string net_name = "";
        ss >> net_name;

        highlight_nets((std::string)net_name);
    }

    else
        return;
    app->refresh_drawing();
}

bool highlight_rr_nodes(int hit_node) {
    t_draw_state* draw_state = get_draw_state_vars();

    char message[250] = "";

    if (hit_node != OPEN) {
        auto nodes = draw_expand_non_configurable_rr_nodes(hit_node);
        for (auto node : nodes) {
            if (draw_state->draw_rr_node[node].color != ezgl::MAGENTA) {
                /* If the node hasn't been clicked on before, highlight it
                 * in magenta.
                 */
                draw_state->draw_rr_node[node].color = ezgl::MAGENTA;
                draw_state->draw_rr_node[node].node_highlighted = true;

            } else {
                //Using white color to represent de-highlighting (or
                //de-selecting) of node.
                draw_state->draw_rr_node[node].color = ezgl::WHITE;
                draw_state->draw_rr_node[node].node_highlighted = false;
            }
            //Print info about all nodes to terminal
            VTR_LOG("%s\n", describe_rr_node(node).c_str());
        }

        //Show info about *only* hit node to graphics
        std::string info = describe_rr_node(hit_node);

        sprintf(message, "Selected %s", info.c_str());
        rr_highlight_message = message;

        if (draw_state->draw_rr_toggle != DRAW_NO_RR) {
            // If rr_graph is shown, highlight the fan-in/fan-outs for
            // this node.
            draw_highlight_fan_in_fan_out(nodes);
        }
    } else {
        application.update_message(draw_state->default_message);
        rr_highlight_message = "";
        application.refresh_drawing();
        return false; //No hit
    }

    if (draw_state->show_nets)
        highlight_nets(message, hit_node);
    else
        application.update_message(message);

    application.refresh_drawing();
    return true;
}

void auto_zoom_rr_node(int rr_node_id) {
    t_draw_coords* draw_coords = get_draw_coords_vars();
    auto& device_ctx = g_vpr_ctx.device();
    ezgl::rectangle rr_node;

    // find the location of the node
    switch (device_ctx.rr_nodes[rr_node_id].type()) {
        case IPIN:
        case OPIN: {
            int i = device_ctx.rr_nodes[rr_node_id].xlow();
            int j = device_ctx.rr_nodes[rr_node_id].ylow();
            t_physical_tile_type_ptr type = device_ctx.grid[i][j].type;
            int width_offset = device_ctx.grid[i][j].width_offset;
            int height_offset = device_ctx.grid[i][j].height_offset;
            int ipin = device_ctx.rr_nodes[rr_node_id].ptc_num();
            float xcen, ycen;

            int iside;
            for (iside = 0; iside < 4; iside++) {
                if (type->pinloc[width_offset][height_offset][iside][ipin]) {
                    draw_get_rr_pin_coords(rr_node_id, &xcen, &ycen);
                    rr_node = {{xcen - draw_coords->pin_size, ycen - draw_coords->pin_size},
                               {xcen + draw_coords->pin_size, ycen + draw_coords->pin_size}};
                }
            }
            break;
        }
        case CHANX:
        case CHANY: {
            rr_node = draw_get_rr_chan_bbox(rr_node_id);
            break;
        }
        default:
            break;
    }

    // zoom to the node
    ezgl::point2d offset = {rr_node.width() * 1.5, rr_node.height() * 1.5};
    ezgl::rectangle zoom_view = {rr_node.m_first - offset, rr_node.m_second + offset};
    (application.get_canvas(application.get_main_canvas_id()))->get_camera().set_world(zoom_view);
}

void highlight_blocks(ClusterBlockId clb_index) {
    char msg[vtr::bufsize];
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    /// determine block ///
    ezgl::rectangle clb_bbox;

    VTR_ASSERT(clb_index != EMPTY_BLOCK_ID);

    ezgl::point2d point_in_clb = clb_bbox.bottom_left();
    highlight_sub_block(point_in_clb, clb_index, cluster_ctx.clb_nlist.block_pb(clb_index));

    if (get_selected_sub_block_info().has_selection()) {
        t_pb* selected_subblock = get_selected_sub_block_info().get_selected_pb();
        sprintf(msg, "sub-block %s (a \"%s\") selected",
                selected_subblock->name, selected_subblock->pb_graph_node->pb_type->name);
    } else {
        /* Highlight block and fan-in/fan-outs. */
        draw_highlight_blocks_color(cluster_ctx.clb_nlist.block_type(clb_index), clb_index);
        sprintf(msg, "Block #%zu (%s) at (%d, %d) selected.", size_t(clb_index), cluster_ctx.clb_nlist.block_name(clb_index).c_str(), place_ctx.block_locs[clb_index].loc.x, place_ctx.block_locs[clb_index].loc.y);
    }

    application.update_message(msg);

    application.refresh_drawing();
}

void highlight_nets(ClusterNetId net_id) {
    t_trace* tptr;
    auto& route_ctx = g_vpr_ctx.routing();

    t_draw_state* draw_state = get_draw_state_vars();

    if (int(route_ctx.trace.size()) == 0) return;

    for (tptr = route_ctx.trace[net_id].head; tptr != nullptr; tptr = tptr->next) {
        draw_state->net_color[net_id] = ezgl::MAGENTA;
    }
}

void highlight_nets(std::string net_name) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    ClusterNetId net_id = ClusterNetId::INVALID();
    net_id = cluster_ctx.clb_nlist.find_net(net_name);

    if (net_id == ClusterNetId::INVALID()) {
        warning_dialog_box("Invalid Net Name");
        return; //name not exist
    }

    highlight_nets(net_id); //found net
}

void highlight_blocks(std::string block_name) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    ClusterBlockId block_id = ClusterBlockId::INVALID();
    block_id = cluster_ctx.clb_nlist.find_block(block_name);

    if (block_id == ClusterBlockId::INVALID()) {
        warning_dialog_box("Invalid Block Name");
        return; //name not exist
    }

    highlight_blocks(block_id); //found block
}

void warning_dialog_box(const char* message) {
    GObject* main_window;    // parent window over which to add the dialog
    GtkWidget* content_area; // content area of the dialog
    GtkWidget* label;        // label to display a message
    GtkWidget* dialog;
    // get a pointer to the main window
    main_window = application.get_object(application.get_main_window_id().c_str());
    // create a dialog window modal with no button
    dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_INFO,
                                    GTK_BUTTONS_NONE,
                                    "Error");
    // create a label and attach it to content area of the dialog
    content_area = gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog));
    label = gtk_label_new(message);
    gtk_container_add(GTK_CONTAINER(content_area), label);
    // show the label & child widget of the dialog
    gtk_widget_show_all(dialog);

    g_signal_connect_swapped(dialog,
                             "response",
                             G_CALLBACK(gtk_widget_destroy),
                             dialog);

    return;
}

#endif /* NO_GRAPHICS */
