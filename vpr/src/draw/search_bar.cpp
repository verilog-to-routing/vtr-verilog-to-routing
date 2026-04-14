/**
 * @file search_bar.cpp
 * @author Sebastian Lievano
 * @brief Contains search/auto-complete related functions
 * @version 0.1
 * @date 2022-07-20
 *
 * This file essentially follows the whole search process, from searching, finding the match,
 * and finally highlighting the searched for item. Additionally, auto-complete related stuff is found
 * here.
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef NO_GRAPHICS
#include <cstdio>
#include <sstream>

#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_utils.h"
#include "route_utils.h"

#include "globals.h"
#include "draw.h"
#include "draw_rr.h"
#include "draw_searchbar.h"
#include "draw_global.h"
#include "intra_logic_block.h"
#include "atom_netlist.h"
#include "search_bar.h"
#include "old_traceback.h"
#include "physical_types.h"
#include "place_macro.h"

#include "vpr_qtcompat.h"
#include <QMessageBox>

//To process key presses we need the X11 keysym definitions,
//which are unavailable when building with MINGW
#if defined(X11) && !defined(__MINGW32__)
#include <X11/keysym.h>
#endif

extern std::string rr_highlight_message;

void search_and_highlight(GtkWidget* /*widget*/, ezgl::application* app) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    // get ID from search bar
    GtkEntry* text_entry = (GtkEntry*)app->get_object("TextInput");
    const char* text = gtk_entry_get_text(text_entry);
    std::string user_input = text;
    std::stringstream ss(user_input);

    auto search_type = get_search_type(app);
    if (search_type.empty())
        return;

    // reset
    deselect_all();

    t_draw_state* draw_state = get_draw_state_vars();

    if (search_type == "RR Node ID") {
        int rr_node_id = -1;
        ss >> rr_node_id;

        // valid rr node id check
        if (rr_node_id < 0 || rr_node_id >= int(device_ctx.rr_graph.num_nodes())) {
            warning_dialog_box("Invalid RR Node ID");
            app->refresh_drawing();
            return;
        }

        highlight_rr_nodes(RRNodeId(rr_node_id));
        auto_zoom_rr_node(RRNodeId(rr_node_id));
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

        highlight_cluster_block((ClusterBlockId)block_id);
    }

    else if (search_type == "Block Name") {
        /* If the block exists in atom netlist, proceeding with highlighting process.
         * if highlight atom block fn returns false, that means that the block can't be highlighted
         * We've already confirmed the block exists in the netlist, so that means that at this zoom lvl,
         * the subblock is not shown. Therefore highlight the clb mapping.
         *
         * If the block does not exist in the atom netlist, we will check the CLB netlist to see if
         * they searched for a cluster block*/
        std::string block_name;
        ss >> block_name;

        AtomBlockId atom_blk_id = atom_ctx.netlist().find_block(block_name);
        if (atom_blk_id != AtomBlockId::INVALID()) {
            ClusterBlockId cluster_block_id = atom_ctx.lookup().atom_clb(atom_blk_id);
            if (!highlight_atom_block(atom_blk_id, cluster_block_id, app)) {
                highlight_cluster_block(cluster_block_id);
            }
            return;
        }

        //Continues if atom block not found (Checking if user searched a clb)
        ClusterBlockId cluster_block_id = ClusterBlockId::INVALID();
        cluster_block_id = cluster_ctx.clb_nlist.find_block(block_name);

        if (cluster_block_id == ClusterBlockId::INVALID()) {
            warning_dialog_box("Invalid Block Name");
            return; //name not exist
        }
        highlight_cluster_block(cluster_block_id); //found block
    }

    else if (search_type == "Net ID") {
        int net_id = -1;
        ss >> net_id;
        if (draw_state->is_flat) {
            AtomNetId atom_net_id = AtomNetId(net_id);
            if (!atom_ctx.netlist().valid_net_id(atom_net_id)) {
                warning_dialog_box("Invalid Net ID");
                app->refresh_drawing();
                return;
            }
            if (!is_net_routed(atom_net_id)) {
                warning_dialog_box("Net is unrouted");
                app->refresh_drawing();
                return;
            }
            if (is_net_fully_absorbed(atom_net_id)) {
                warning_dialog_box("Net is fully absorbed");
                app->refresh_drawing();
                return;
            }
            highlight_nets((ClusterNetId)net_id);
        } else {
            // valid net id check
            if (!cluster_ctx.clb_nlist.valid_net_id(ClusterNetId(net_id))) {
                warning_dialog_box("Invalid Net ID");
                app->refresh_drawing();
                return;
            }
            highlight_nets((ClusterNetId)net_id);
        }
    }

    else if (search_type == "Net Name") {
        //in this case, all nets (clb and non-clb) are contained in the atom netlist
        //So we only need to search this one
        std::string net_name;
        ss >> net_name;

        if (draw_state->is_flat) {
            AtomNetId atom_net_id = atom_ctx.netlist().find_net(net_name);
            if (atom_net_id == AtomNetId::INVALID()) {
                warning_dialog_box("Invalid Net Name");
                app->refresh_drawing();
                return;
            }
            if (!is_net_routed(atom_net_id)) {
                warning_dialog_box("Net is unrouted");
                app->refresh_drawing();
                return;
            }
            if (is_net_fully_absorbed(atom_net_id)) {
                warning_dialog_box("Net is fully absorbed");
                app->refresh_drawing();
                return;
            }
            highlight_nets(convert_to_cluster_net_id(atom_net_id));
        } else {
            AtomNetId atom_net_id = atom_ctx.netlist().find_net(net_name);

            if (atom_net_id == AtomNetId::INVALID()) {
                warning_dialog_box("Invalid Net Name");
                app->refresh_drawing();
                return;
            }
            auto clb_net_ids_opt = atom_ctx.lookup().clb_nets(atom_net_id);
            if (clb_net_ids_opt.has_value()) {
                for (auto clb_net_id : clb_net_ids_opt.value()) {
                    highlight_nets(clb_net_id);
                }
            }
        }
    }

    else
        return;
    app->refresh_drawing();
}

bool highlight_rr_nodes(RRNodeId hit_node) {
    t_draw_state* draw_state = get_draw_state_vars();

    //TODO: fixed sized char array may cause overflow.
    char message[250] = "";

    if (!hit_node) {
        application.update_message(draw_state->default_message);
        rr_highlight_message = "";
        application.refresh_drawing();
        return false;
    }

    const DeviceContext& device_ctx = g_vpr_ctx.device();

    // Highlight neighboring non_configurable nodes in magenta as well
    std::set<RRNodeId> nodes = draw_expand_non_configurable_rr_nodes(hit_node);

    for (RRNodeId node : nodes) {
        if (draw_state->draw_rr_node[node].color != ezgl::MAGENTA) {
            // If the node hasn't been clicked on before, highlight it in magenta.
            draw_state->draw_rr_node[node].color = ezgl::MAGENTA;
            draw_state->draw_rr_node[node].node_highlighted = true;
            draw_state->hit_nodes.insert(node);
        } else {
            //Using white color to represent de-highlighting (or de-selecting) of node.
            draw_state->draw_rr_node[node].color = ezgl::WHITE;
            draw_state->draw_rr_node[node].node_highlighted = false;
            draw_state->hit_nodes.erase(node);
        }

        //Print info about all nodes to terminal including neighboring non-configurable nodes
        VTR_LOG("%s\n", describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, node, draw_state->is_flat).c_str());
    }

    //Show info about *only* hit node to graphics
    std::string info = describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, hit_node, draw_state->is_flat);
    sprintf(message, "Selected %s", info.c_str());
    rr_highlight_message = message;

    if (draw_state->show_nets) {
        highlight_nets(message, hit_node);
    }

    // Highlight RR Edges if the user has selected this option
    if (draw_state->show_rr && draw_state->highlight_rr_edges) {
        draw_highlight_fan_in_fan_out(nodes);
    }

    application.update_message(message);
    application.refresh_drawing();

    return true;
}

void auto_zoom_rr_node(RRNodeId rr_node_id) {
    t_draw_coords* draw_coords = get_draw_coords_vars();
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;
    ezgl::rectangle rr_node;

    // find the location of the node
    switch (rr_graph.node_type(rr_node_id)) {
        case e_rr_type::IPIN:
        case e_rr_type::OPIN: {
            t_physical_tile_loc tile_loc = {
                rr_graph.node_xlow(rr_node_id),
                rr_graph.node_ylow(rr_node_id),
                rr_graph.node_layer_low(rr_node_id)};
            t_physical_tile_type_ptr type = device_ctx.grid.get_physical_type(tile_loc);
            int width_offset = device_ctx.grid.get_width_offset(tile_loc);
            int height_offset = device_ctx.grid.get_height_offset(tile_loc);

            int ipin = rr_graph.node_ptc_num(rr_node_id);
            float xcen, ycen;

            for (const e_side& iside : TOTAL_2D_SIDES) {
                if (type->pinloc[width_offset][height_offset][size_t(iside)][ipin]) {
                    draw_get_rr_pin_coords(rr_node_id, &xcen, &ycen, iside);
                    rr_node = {{xcen - draw_coords->pin_size, ycen - draw_coords->pin_size},
                               {xcen + draw_coords->pin_size, ycen + draw_coords->pin_size}};
                }
            }
            break;
        }
        case e_rr_type::CHANX:
        case e_rr_type::CHANY: {
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

/**
 * @brief Highlights the given cluster block
 *
 * @param clb_index Cluster Index to be highlighted
 */
void highlight_cluster_block(ClusterBlockId clb_index) {
    char msg[vtr::bufsize];
    t_draw_state* draw_state = get_draw_state_vars();
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const auto& block_locs = draw_state->get_graphics_blk_loc_registry_ref().block_locs();

    /// determine block ///
    ezgl::rectangle clb_bbox;

    VTR_ASSERT(clb_index != ClusterBlockId::INVALID());

    ezgl::point2d point_in_clb = clb_bbox.bottom_left();
    highlight_sub_block(point_in_clb, clb_index, cluster_ctx.clb_nlist.block_pb(clb_index));

    if (get_selected_sub_block_info().has_selection()) {
        t_pb* selected_subblock = get_selected_sub_block_info().get_selected_pb();
        sprintf(msg, "sub-block %s (a \"%s\") selected",
                selected_subblock->name, selected_subblock->pb_graph_node->pb_type->name);
    } else {
        /* Highlight block and fan-in/fan-outs. */
        draw_highlight_blocks_color(cluster_ctx.clb_nlist.block_type(clb_index), clb_index);
        sprintf(msg, "Block #%zu (%s) at (%d, %d) selected.",
                size_t(clb_index), cluster_ctx.clb_nlist.block_name(clb_index).c_str(),
                block_locs[clb_index].loc.x, block_locs[clb_index].loc.y);
    }

    application.update_message(msg);

    application.refresh_drawing();
}

/**
 * @brief Finds and highlights the atom block. Returns true if block shows, false if not
 *
 * @param atom_blk AtomBlockId being searched for
 * @param cl_blk ClusterBlock containing atom_blk
 * @param app ezgl:: application used
 * @return true | If sub-block can be highlighted
 * @return false | If sub-block not found (impossible in search case) or not shown at current zoom lvl
 */
bool highlight_atom_block(AtomBlockId atom_blk, ClusterBlockId cl_blk, ezgl::application* app) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    t_pb* pb = cluster_ctx.clb_nlist.block_pb(cl_blk);

    //Getting the pb* for the atom block
    auto atom_block_pb = find_atom_block_in_pb(atom_ctx.netlist().block_name(atom_blk), pb);
    if (!atom_block_pb) return false; //If no block found, returning false

    //Ensuring that block is drawn at current zoom lvl, returning false if not
    auto atom_block_depth = atom_block_pb->pb_graph_node->pb_type->depth;
    t_draw_state* draw_state = get_draw_state_vars();
    int max_depth = draw_state->show_blk_internal;
    if (atom_block_depth > max_depth) return false;

    //Highlighting block
    get_selected_sub_block_info().set(atom_block_pb, cl_blk);
    app->refresh_drawing();
    return true;
}

void highlight_nets(ClusterNetId net_id) {
    const RoutingContext& route_ctx = g_vpr_ctx.routing();

    t_draw_state* draw_state = get_draw_state_vars();

    //If routing does not exist return
    if (route_ctx.route_trees.empty()) return;
    draw_state->net_color[net_id] = ezgl::MAGENTA;
}

void warning_dialog_box(const char* message) {
    QWidget* main_window = application.get_widget(application.get_main_window_id().c_str());
    QMessageBox* box = new QMessageBox(QMessageBox::Warning,
                                       "Error",
                                       message,
                                       QMessageBox::NoButton,
                                       main_window);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->show();
}

/**
 * @brief manages auto-complete options when search type is changed
 *
 * Selects appropriate gtkEntryCompletion item when changed signal is sent
 * from gtkComboBox SearchType. Currently only sets a completion model for Block Name options,
 * sets null for anything else. brh
 *
 * @param self GtkComboBox that holds current Search Setting
 * @param app ezgl app used to access other objects
 */
void search_type_changed(GtkComboBox* self, ezgl::application* app) {
    if (!self) return;
    const QString searchType = self->currentText();
    if (searchType.isEmpty()) return;

    QLineEdit* searchBar = qobject_cast<QLineEdit*>(app->get_object("TextInput"));
    if (!searchBar) return;

    if (searchType == "Block Name") {
        searchBar->setCompleter(searchBar->findChild<QCompleter*>("BlockNames"));
    } else if (searchType == "Net Name") {
        searchBar->setCompleter(searchBar->findChild<QCompleter*>("NetNames"));
    } else {
        searchBar->setCompleter(nullptr);
    }
}

/**
 * @brief A non-default matching function. As opposed to simply searching for a prefix(default),
 * searches string for presence of a substring. Case-insensitive
 *
 * @param completer the GtkEntryCompletion being used
 * @param key a normalized and case-folded key representing the text
 * @param iter GtkTreeIter pointing at the current entry being compared
 * @param user_data null
 * @return true | if the string pointed to by iter contains key (case-insensitive)
 * @return false | if the string pointed to does not contain key
 */


/**
 * @brief Turns on autocomplete
 *
 * This function enables the auto-complete functionality for the search bar.
 * Normally, this is pretty simple, but the idea is to have auto-complete appear as soon
 * as the user hits the "Enter" key. To accomplish this, a fake Gdk event is created
 * to simulate the user hitting a key.
 *
 * This was done for usability reasons; if this is not done, user will need to input another key before seeing
 * autocomplete results. Considering the enter is supposed to be a search, we want to search for the users
 * key, not the key + another char
 *
 * PERFORMANCE DATA
 * Correlation between key length and time is shaky; there might be some correlation to
 * how many strings are similar to it. All tests are performed with the key "1" - pretty common
 * Tests are searched three times then average
 * MODEL 1: EArch + TSENG.BLIF
 * NETS         1483
 * NET SRCH.    19392
 * BLOCKS       1835
 * BLOCK SRCH.  21840
 * For second model (much larger, much longer CPU times) observed large dropoff in times from one char to two chars (about 2 times faster) but after stayed consistent
 * MODEL 2: Strativix arch + MES_NOC (TITAN)
 * NETS         577696
 * NET SRCH.    4.93438e+06
 * BLOCKS       572148
 * BLOCKS SRCH. 4.8654e+06
 * Obviously much slower w. more nets/blocks. However, it only performs a single search after each enter key press, pretty bearable considering its searching in strings
 * @param app ezgl app
 */
void enable_autocomplete(ezgl::application* app) {
    QLineEdit* searchBar = qobject_cast<QLineEdit*>(app->get_object("TextInput"));
    if (!searchBar) return;

    const std::string searchType = get_search_type(app);
    if (searchType.empty()) return;

    // Resolve the completer for the active search type. Completers are stored
    // as named children of the QLineEdit (set up in ui_setup.cpp).
    QCompleter* completer = nullptr;
    if (searchType == "Block Name") {
        completer = searchBar->findChild<QCompleter*>("BlockNames");
    } else if (searchType == "Net Name") {
        completer = searchBar->findChild<QCompleter*>("NetNames");
    }

    if (!completer) return;

    searchBar->setCompleter(completer);

    auto draw_state = get_draw_state_vars();
    draw_state->justEnabled = true;

    // Directly trigger the popup — replaces the GTK hack of stripping and
    // re-typing the last character to force GtkEntryCompletion to show.
    completer->complete();
}

//Returns current search type. Returns empty string if fails
std::string get_search_type(ezgl::application* app) {
    GObject* combo_box = (GObject*)app->get_object("SearchType");
    gchar* type = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo_box));
    //Checking that a type is selected
    if (!type || (type && type[0] == '\0')) {
        warning_dialog_box("Please select a search type");
        app->refresh_drawing();
        return "";
    }
    std::string searchType(type);
    return searchType;
}

#endif /* NO_GRAPHICS */
