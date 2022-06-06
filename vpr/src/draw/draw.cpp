/*********************************** Top-level Summary *************************************
 * This is VPR's main graphics application program. The program interacts with ezgl/graphics.hpp,
 * which provides an API for displaying graphics on both X11 and Win32. The most important
 * subroutine in this file is draw_main_canvas(), which is a callback function that will be called 
 * whenever the screen needs to be updated. Then, draw_main_canvas() will decide what
 * drawing subroutines to call depending on whether PLACEMENT or ROUTING is shown on screen.
 * The initial_setup_X() functions link the menu button signals to the corresponding drawing functions. 
 * As a note, looks into draw_global.c for understanding the data structures associated with drawing->
 *
 * Contains all functions that didn't fit in any other draw_*.cpp file.
 * Authors: Vaughn Betz, Long Yu (Mike) Wang, Dingyu (Tina) Yang
 * Last updated: June 2019
 */

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
#include "draw_basic.h"
#include "draw_rr.h"
#include "draw_rr_edges.h"
#include "draw_toggle_functions.h"
#include "draw_triangle.h"
#include "draw_mux.h"
#include "draw_searchbar.h"
#include "read_xml_arch_file.h"
#include "draw_global.h"
#include "intra_logic_block.h"
#include "atom_netlist.h"
#include "tatum/report/TimingPathCollector.hpp"
#include "hsl.h"
#include "route_export.h"
#include "search_bar.h"
#include "save_graphics.h"
#include "timing_info.h"
#include "physical_types.h"
#include "route_common.h"
#include "breakpoint.h"
#include "manual_moves.h"
#include "move_utils.h"

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
#    include "draw_rr.h"
/****************************** Define Macros *******************************/

#    define DEFAULT_RR_NODE_COLOR ezgl::BLACK
#    define OLD_BLK_LOC_COLOR blk_GOLD
#    define NEW_BLK_LOC_COLOR blk_GREEN
//#define TIME_DRAWSCREEN /* Enable if want to track runtime for drawscreen() */

/********************** Subroutines local to this module ********************/

void act_on_key_press(ezgl::application* /*app*/, GdkEventKey* /*event*/, char* key_name);
void act_on_mouse_press(ezgl::application* app, GdkEventButton* event, double x, double y);
void act_on_mouse_move(ezgl::application* app, GdkEventButton* event, double x, double y);

static void highlight_blocks(double x, double y);

static float get_router_expansion_cost(const t_rr_node_route_inf node_inf,
                                       e_draw_router_expansion_cost draw_router_expansion_cost);
static void draw_router_expansion_costs(ezgl::renderer* g);

static void draw_main_canvas(ezgl::renderer* g);
static void initial_setup_NO_PICTURE_to_PLACEMENT(ezgl::application* app,
                                                  bool is_new_window);
static void initial_setup_NO_PICTURE_to_PLACEMENT_with_crit_path(
    ezgl::application* app,
    bool is_new_window);
static void initial_setup_PLACEMENT_to_ROUTING(ezgl::application* app,
                                               bool is_new_window);
static void initial_setup_ROUTING_to_PLACEMENT(ezgl::application* app,
                                               bool is_new_window);
static void initial_setup_NO_PICTURE_to_ROUTING(ezgl::application* app,
                                                bool is_new_window);
static void initial_setup_NO_PICTURE_to_ROUTING_with_crit_path(
    ezgl::application* app,
    bool is_new_window);
static void toggle_window_mode(GtkWidget* /*widget*/,
                               ezgl::application* /*app*/);
static void setup_default_ezgl_callbacks(ezgl::application* app);
static void set_force_pause(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
static void set_block_outline(GtkWidget* widget, gint /*response_id*/, gpointer /*data*/);
static void set_block_text(GtkWidget* widget, gint /*response_id*/, gpointer /*data*/);
static void clip_routing_util(GtkWidget* widget, gint /*response_id*/, gpointer /*data*/);
static void run_graphics_commands(std::string commands);

/************************** File Scope Variables ****************************/

//The arrow head position for turning/straight-thru connections in a switch box
constexpr float SB_EDGE_TURN_ARROW_POSITION = 0.2;
constexpr float SB_EDGE_STRAIGHT_ARROW_POSITION = 0.95;
constexpr float EMPTY_BLOCK_LIGHTEN_FACTOR = 0.20;

//Kelly's maximum contrast colors are selected to be easily distinguishable as described in:
//  Kenneth Kelly, "Twenty-Two Colors of Maximum Contrast", Color Eng. 3(6), 1943
//We use these to highlight a relatively small number of things (e.g. stages in a critical path,
//a subset of selected net) where it is important for them to be visually distinct.

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

ezgl::application::settings settings("/ezgl/main.ui", "MainWindow", "MainCanvas", "org.verilogtorouting.vpr.PID" + std::to_string(vtr::get_pid()), setup_default_ezgl_callbacks);
ezgl::application application(settings);

bool window_mode = false;
bool window_point_1_collected = false;
ezgl::point2d point_1(0, 0);
ezgl::rectangle initial_world;
std::string rr_highlight_message;

#endif // NO_GRAPHICS

/********************** Subroutine definitions ******************************/

void init_graphics_state(bool show_graphics_val, int gr_automode_val, enum e_route_type route_type, bool save_graphics, std::string graphics_commands) {
#ifndef NO_GRAPHICS
    /* Call accessor functions to retrieve global variables. */
    t_draw_state* draw_state = get_draw_state_vars();

    /* Sets the static show_graphics and gr_automode variables to the    *
     * desired values.  They control if graphics are enabled and, if so, *
     * how often the user is prompted for input.                         */

    draw_state->show_graphics = show_graphics_val;
    draw_state->gr_automode = gr_automode_val;
    draw_state->draw_route_type = route_type;
    draw_state->save_graphics = save_graphics;
    draw_state->graphics_commands = graphics_commands;

#else
    //Suppress unused parameter warnings
    (void)show_graphics_val;
    (void)gr_automode_val;
    (void)route_type;
    (void)save_graphics;
    (void)graphics_commands;
#endif // NO_GRAPHICS
}

#ifndef NO_GRAPHICS
static void draw_main_canvas(ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();

    g->set_font_size(14);

    draw_block_pin_util();
    drawplace(g);
    draw_internal_draw_subblk(g);

    if (draw_state->pic_on_screen == PLACEMENT) {
        switch (draw_state->show_nets) {
            case DRAW_NETS:
                drawnets(g);
                break;
            case DRAW_LOGICAL_CONNECTIONS:
                break;
            default:
                break;
        }
    } else { /* ROUTING on screen */

        switch (draw_state->show_nets) {
            case DRAW_NETS:
                drawroute(ALL_NETS, g);
                break;
            case DRAW_LOGICAL_CONNECTIONS:
                // fall through
            default:
                draw_rr(g);
                break;
        }

        draw_congestion(g);

        draw_routing_costs(g);

        draw_router_expansion_costs(g);

        draw_routing_util(g);

        draw_routing_bb(g);
    }

    draw_placement_macros(g);

    draw_crit_path(g);

    draw_logical_connections(g);

    if (draw_state->color_map) {
        draw_color_map_legend(*draw_state->color_map, g);
        draw_state->color_map.reset(); //Free color map in preparation for next redraw
    }

    if (draw_state->auto_proceed) {
        //Automatically exit the event loop, so user's don't need to manually click proceed

        //Avoid trying to repeatedly exit (which would cause errors in GTK)
        draw_state->auto_proceed = false;

        application.quit(); //Ensure we leave the event loop
    }
}

/* function below intializes the interface window with a set of buttons and links 
 * signals to corresponding functions for situation where the window is opened from 
 * NO_PICTURE_to_PLACEMENT */
static void initial_setup_NO_PICTURE_to_PLACEMENT(ezgl::application* app,
                                                  bool is_new_window) {
    if (!is_new_window)
        return;

    //button to enter window_mode, created in main.ui
    GtkButton* window = (GtkButton*)app->get_object("Window");
    gtk_button_set_label(window, "Window");
    g_signal_connect(window, "clicked", G_CALLBACK(toggle_window_mode), app);

    //button to search, created in main.ui
    GtkButton* search = (GtkButton*)app->get_object("Search");
    gtk_button_set_label(search, "Search");
    g_signal_connect(search, "clicked", G_CALLBACK(search_and_highlight), app);

    //button for save graphcis, created in main.ui
    GtkButton* save = (GtkButton*)app->get_object("SaveGraphics");
    g_signal_connect(save, "clicked", G_CALLBACK(save_graphics_dialog_box),
                     app);

    //combo box for search type, created in main.ui
    GObject* search_type = (GObject*)app->get_object("SearchType");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_type), "Block ID"); // index 0
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_type),
                                   "Block Name");                                // index 1
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_type), "Net ID");   // index 2
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_type), "Net Name"); // index 3
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_type),
                                   "RR Node ID"); // index 4
    //Important to have default option set, or else user can search w. no selected type which can cause crash
    gtk_combo_box_set_active((GtkComboBox*)search_type, 0); // default set to Block ID which has an index 0
    g_signal_connect(search_type, "changed", G_CALLBACK(search_type_changed), app);
    load_block_names(app);
    button_for_toggle_nets();
    button_for_net_max_fanout();
    button_for_net_alpha();
    button_for_toggle_blk_internal();
    button_for_toggle_block_pin_util();
    button_for_toggle_placement_macros();
}

/* function below intializes the interface window with a set of buttons and links 
 * signals to corresponding functions for situation where the window is opened from 
 * NO_PICTURE_to_PLACEMENT_with_crit_path */
static void initial_setup_NO_PICTURE_to_PLACEMENT_with_crit_path(
    ezgl::application* app,
    bool is_new_window) {
    initial_setup_NO_PICTURE_to_PLACEMENT(app, is_new_window);
    button_for_toggle_crit_path();
}

/* function below intializes the interface window with a set of buttons and links 
 * signals to corresponding functions for situation where the window is opened from 
 * PLACEMENT_to_ROUTING */
static void initial_setup_PLACEMENT_to_ROUTING(ezgl::application* app,
                                               bool is_new_window) {
    initial_setup_NO_PICTURE_to_PLACEMENT_with_crit_path(app, is_new_window);
    button_for_toggle_rr();
    button_for_toggle_congestion();
    button_for_toggle_congestion_cost();
    button_for_toggle_routing_bounding_box();
    button_for_toggle_routing_util();
    button_for_toggle_router_expansion_costs();
}

/* function below intializes the interface window with a set of buttons and links 
 * signals to corresponding functions for situation where the window is opened from 
 * ROUTING_to_PLACEMENT */
static void initial_setup_ROUTING_to_PLACEMENT(ezgl::application* app,
                                               bool is_new_window) {
    initial_setup_PLACEMENT_to_ROUTING(app, is_new_window);
    std::string toggle_rr = "toggle_rr";
    std::string toggle_congestion = "toggle_congestion";
    std::string toggle_routing_congestion_cost = "toggle_routing_congestion_cost";
    std::string toggle_routing_bounding_box = "toggle_routing_bounding_box";
    std::string toggle_routing_util = "toggle_rr";
    std::string toggle_router_expansion_costs = "toggle_router_expansion_costs";

    delete_button(toggle_rr.c_str());
    delete_button(toggle_congestion.c_str());
    delete_button(toggle_routing_congestion_cost.c_str());
    delete_button(toggle_routing_bounding_box.c_str());
    delete_button(toggle_routing_util.c_str());
    delete_button(toggle_router_expansion_costs.c_str());
}

/* function below intializes the interface window with a set of buttons and links 
 * signals to corresponding functions for situation where the window is opened from 
 * NO_PICTURE_to_ROUTING */
static void initial_setup_NO_PICTURE_to_ROUTING(ezgl::application* app,
                                                bool is_new_window) {
    if (!is_new_window)
        return;

    GtkButton* window = (GtkButton*)app->get_object("Window");
    gtk_button_set_label(window, "Window");
    g_signal_connect(window, "clicked", G_CALLBACK(toggle_window_mode), app);

    GtkButton* search = (GtkButton*)app->get_object("Search");
    gtk_button_set_label(search, "Search");
    g_signal_connect(search, "clicked", G_CALLBACK(search_and_highlight), app);

    GtkButton* save = (GtkButton*)app->get_object("SaveGraphics");
    g_signal_connect(save, "clicked", G_CALLBACK(save_graphics_dialog_box),
                     app);

    GObject* search_type = (GObject*)app->get_object("SearchType");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_type), "Block ID");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_type),
                                   "Block Name");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_type), "Net ID");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_type), "Net Name");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_type),
                                   "RR Node ID");
    //Important to have default option set, or else user can search w. no selected type which can cause crash
    gtk_combo_box_set_active((GtkComboBox*)search_type, 0); // default set to Block ID which has an index 0
    g_signal_connect(search_type, "changed", G_CALLBACK(search_type_changed), app);
    load_block_names(app);

    button_for_toggle_nets();
    button_for_net_max_fanout();
    button_for_net_alpha();
    button_for_toggle_blk_internal();
    button_for_toggle_block_pin_util();
    button_for_toggle_placement_macros();
    button_for_toggle_rr();
    button_for_toggle_congestion();
    button_for_toggle_congestion_cost();
    button_for_toggle_routing_bounding_box();
    button_for_toggle_routing_util();
    button_for_toggle_router_expansion_costs();
}

/* function below intializes the interface window with a set of buttons and links 
 * signals to corresponding functions for situation where the window is opened from 
 * NO_PICTURE_to_ROUTING_with_crit_path */
static void initial_setup_NO_PICTURE_to_ROUTING_with_crit_path(
    ezgl::application* app,
    bool is_new_window) {
    initial_setup_NO_PICTURE_to_ROUTING(app, is_new_window);
    button_for_toggle_crit_path();
}
#endif //NO_GRAPHICS

void update_screen(ScreenUpdatePriority priority, const char* msg, enum pic_type pic_on_screen_val, std::shared_ptr<SetupTimingInfo> setup_timing_info) {
#ifndef NO_GRAPHICS

    /* Updates the screen if the user has requested graphics.  The priority  *
     * value controls whether or not the Proceed button must be clicked to   *
     * continue.  Saves the pic_on_screen_val to allow pan and zoom redraws. */

    t_draw_state* draw_state = get_draw_state_vars();

    if (!draw_state->show_graphics)
        ezgl::set_disable_event_loop(true);
    else
        ezgl::set_disable_event_loop(false);

    ezgl::setup_callback_fn init_setup = nullptr;

    /* If it's the type of picture displayed has changed, set up the proper  *
     * buttons.                                                              */
    if (draw_state->pic_on_screen != pic_on_screen_val) { //State changed

        if (draw_state->pic_on_screen == NO_PICTURE) {
            //Only add the canvas the first time we open graphics
            application.add_canvas("MainCanvas", draw_main_canvas,
                                   initial_world);
        }

        draw_state->setup_timing_info = setup_timing_info;

        if (pic_on_screen_val == PLACEMENT
            && draw_state->pic_on_screen == NO_PICTURE) {
            if (setup_timing_info) {
                init_setup = initial_setup_NO_PICTURE_to_PLACEMENT_with_crit_path;
            } else {
                init_setup = initial_setup_NO_PICTURE_to_PLACEMENT;
            }
            draw_state->save_graphics_file_base = "vpr_placement";

        } else if (pic_on_screen_val == ROUTING
                   && draw_state->pic_on_screen == PLACEMENT) {
            //Routing, opening after placement
            init_setup = initial_setup_PLACEMENT_to_ROUTING;
            draw_state->save_graphics_file_base = "vpr_routing";

        } else if (pic_on_screen_val == PLACEMENT
                   && draw_state->pic_on_screen == ROUTING) {
            init_setup = initial_setup_ROUTING_to_PLACEMENT;
            draw_state->save_graphics_file_base = "vpr_placement";

        } else if (pic_on_screen_val == ROUTING
                   && draw_state->pic_on_screen == NO_PICTURE) {
            //Routing opening first
            if (setup_timing_info) {
                init_setup = initial_setup_NO_PICTURE_to_ROUTING_with_crit_path;
            } else {
                init_setup = initial_setup_NO_PICTURE_to_ROUTING;
            }
            draw_state->save_graphics_file_base = "vpr_routing";
        }

        draw_state->pic_on_screen = pic_on_screen_val;

    } else {
        //No change (e.g. paused)
        init_setup = nullptr;
    }

    bool state_change = (init_setup != nullptr);
    bool should_pause = int(priority) >= draw_state->gr_automode;

    //If there was a state change, we must call ezgl::application::run() to update the buttons.
    //However, by default this causes graphics to pause for user interaction.
    //
    //If the priority is such that we shouldn't pause we need to continue automatically, so
    //the user won't need to click manually.
    draw_state->auto_proceed = (state_change && !should_pause);

    if (state_change                   //Must update buttons
        || should_pause                //The priority means graphics should pause for user interaction
        || draw_state->forced_pause) { //The user asked to pause

        if (draw_state->forced_pause) {
            VTR_LOG("Pausing in interactive graphics (user pressed 'Pause')\n");
            draw_state->forced_pause = false; //Reset pause flag
        }

        application.run(init_setup, act_on_mouse_press, act_on_mouse_move,
                        act_on_key_press);

        if (!draw_state->graphics_commands.empty()) {
            run_graphics_commands(draw_state->graphics_commands);
        }
    }

    if (draw_state->show_graphics) {
        application.update_message(msg);
        application.refresh_drawing();
        application.flush_drawing();
    }

    if (draw_state->save_graphics) {
        std::string extension = "pdf";
        save_graphics(extension, draw_state->save_graphics_file_base);
    }

#else
    (void)setup_timing_info;
    (void)priority;
    (void)msg;
    (void)pic_on_screen_val;
#endif //NO_GRAPHICS
}

#ifndef NO_GRAPHICS
static void toggle_window_mode(GtkWidget* /*widget*/,
                               ezgl::application* /*app*/) {
    window_mode = true;
}

#endif // NO_GRAPHICS

void alloc_draw_structs(const t_arch* arch) {
#ifndef NO_GRAPHICS
    /* Call accessor functions to retrieve global variables. */
    t_draw_coords* draw_coords = get_draw_coords_vars();
    t_draw_state* draw_state = get_draw_state_vars();
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    /* Allocate the structures needed to draw the placement and routing->  Set *
     * up the default colors for blocks and nets.                             */
    draw_coords->tile_x = new float[device_ctx.grid.width()];
    draw_coords->tile_y = new float[device_ctx.grid.height()];

    /* For sub-block drawings inside clbs */
    draw_internal_alloc_blk();

    draw_state->net_color.resize(cluster_ctx.clb_nlist.nets().size());
    draw_state->block_color_.resize(cluster_ctx.clb_nlist.blocks().size());
    draw_state->use_default_block_color_.resize(
        cluster_ctx.clb_nlist.blocks().size());

    /* Space is allocated for draw_rr_node but not initialized because we do *
     * not yet know information about the routing resources.				  */
    draw_state->draw_rr_node = (t_draw_rr_node*)vtr::malloc(
        device_ctx.rr_graph.num_nodes() * sizeof(t_draw_rr_node));

    draw_state->arch_info = arch;

    deselect_all(); /* Set initial colors */
#else
    (void)arch;
#endif // NO_GRAPHICS
}

void free_draw_structs() {
#ifndef NO_GRAPHICS
    /* Free everything allocated by alloc_draw_structs. Called after close_graphics() *
     * in vpr_api.c.
     *
     * For safety, set all the array pointers to NULL in case any data
     * structure gets freed twice.													 */
    t_draw_state* draw_state = get_draw_state_vars();
    t_draw_coords* draw_coords = get_draw_coords_vars();

    if (draw_coords != nullptr) {
        delete[](draw_coords->tile_x);
        draw_coords->tile_x = nullptr;
        delete[](draw_coords->tile_y);
        draw_coords->tile_y = nullptr;
    }

    if (draw_state != nullptr) {
        free(draw_state->draw_rr_node);
        draw_state->draw_rr_node = nullptr;
    }
#else
    ;
#endif /* NO_GRAPHICS */
}

void init_draw_coords(float width_val) {
#ifndef NO_GRAPHICS
    /* Load the arrays containing the left and bottom coordinates of the clbs   *
     * forming the FPGA.  tile_width_val sets the width and height of a drawn    *
     * clb.                                                                     */
    t_draw_state* draw_state = get_draw_state_vars();
    t_draw_coords* draw_coords = get_draw_coords_vars();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    if (!draw_state->show_graphics && !draw_state->save_graphics
        && draw_state->graphics_commands.empty())
        return; //do not initialize only if --disp off and --save_graphics off
    /* Each time routing is on screen, need to reallocate the color of each *
     * rr_node, as the number of rr_nodes may change.						*/
    if (rr_graph.num_nodes() != 0) {
        draw_state->draw_rr_node = (t_draw_rr_node*)vtr::realloc(
            draw_state->draw_rr_node,
            (rr_graph.num_nodes()) * sizeof(t_draw_rr_node));
        /*FIXME: the type cast should be eliminated by making draw_rr_node adapt RRNodeId */
        for (const RRNodeId& rr_id : rr_graph.nodes()) {
            draw_state->draw_rr_node[(size_t)rr_id].color = DEFAULT_RR_NODE_COLOR;
            draw_state->draw_rr_node[(size_t)rr_id].node_highlighted = false;
        }
    }
    draw_coords->tile_width = width_val;
    draw_coords->pin_size = 0.3;
    for (const auto& type : device_ctx.physical_tile_types) {
        auto num_pins = type.num_pins;
        if (num_pins > 0) {
            draw_coords->pin_size = std::min(draw_coords->pin_size,
                                             (draw_coords->get_tile_width() / (4.0F * num_pins)));
        }
    }

    size_t j = 0;
    for (size_t i = 0; i < (device_ctx.grid.width() - 1); i++) {
        draw_coords->tile_x[i] = (i * draw_coords->get_tile_width()) + j;
        j += device_ctx.chan_width.y_list[i] + 1; /* N wires need N+1 units of space */
    }
    draw_coords->tile_x[device_ctx.grid.width() - 1] = ((device_ctx.grid.width()
                                                         - 1)
                                                        * draw_coords->get_tile_width())
                                                       + j;
    j = 0;
    for (size_t i = 0; i < (device_ctx.grid.height() - 1); ++i) {
        draw_coords->tile_y[i] = (i * draw_coords->get_tile_width()) + j;
        j += device_ctx.chan_width.x_list[i] + 1;
    }
    draw_coords->tile_y[device_ctx.grid.height() - 1] = ((device_ctx.grid.height() - 1) * draw_coords->get_tile_width())
                                                        + j;
    /* Load coordinates of sub-blocks inside the clbs */
    draw_internal_init_blk();
    //Margin beyond edge of the drawn device to extend the visible world
    //Setting this to > 0.0 means 'Zoom Fit' leave some fraction of white
    //space around the device edges
    constexpr float VISIBLE_MARGIN = 0.01;

    float draw_width = draw_coords->tile_x[device_ctx.grid.width() - 1]
                       + draw_coords->get_tile_width();
    float draw_height = draw_coords->tile_y[device_ctx.grid.height() - 1]
                        + draw_coords->get_tile_width();

    initial_world = ezgl::rectangle(
        {-VISIBLE_MARGIN * draw_width, -VISIBLE_MARGIN * draw_height},
        {(1. + VISIBLE_MARGIN) * draw_width, (1. + VISIBLE_MARGIN)
                                                 * draw_height});
#else
    (void)width_val;
#endif /* NO_GRAPHICS */
}

#ifndef NO_GRAPHICS

int get_track_num(int inode, const vtr::OffsetMatrix<int>& chanx_track, const vtr::OffsetMatrix<int>& chany_track) {
    /* Returns the track number of this routing resource node.   */

    int i, j;
    t_rr_type rr_type;
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    RRNodeId rr_node = RRNodeId(inode);

    if (get_draw_state_vars()->draw_route_type == DETAILED)
        return (rr_graph.node_track_num(rr_node));

    /* GLOBAL route stuff below. */

    rr_type = rr_graph.node_type(rr_node);
    i = rr_graph.node_xlow(rr_node); /* NB: Global rr graphs must have only unit */
    j = rr_graph.node_ylow(rr_node); /* length channel segments.                 */

    switch (rr_type) {
        case CHANX:
            return (chanx_track[i][j]);

        case CHANY:
            return (chany_track[i][j]);

        default:
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                      "in get_track_num: Unexpected node type %d for node %d.\n",
                      rr_type, inode);
            return OPEN;
    }
}

/* This helper function determines whether a net has been highlighted. The highlighting
 * could be caused by the user clicking on a routing resource, toggled, or
 * fan-in/fan-out of a highlighted node.
 */
bool draw_if_net_highlighted(ClusterNetId inet) {
    t_draw_state* draw_state = get_draw_state_vars();

    if (draw_state->net_color[inet] != DEFAULT_RR_NODE_COLOR) {
        return true;
    }

    return false;
}

#    if defined(X11) && !defined(__MINGW32__)
void act_on_key_press(ezgl::application* /*app*/, GdkEventKey* /*event*/, char* key_name) {
    //VTR_LOG("Key press %c (%d)\n", key_pressed, keysym);
    std::string key(key_name);
}
#    else
void act_on_key_press(ezgl::application* /*app*/, GdkEventKey* /*event*/, char* /*key_name*/) {
}
#    endif

void act_on_mouse_press(ezgl::application* app, GdkEventButton* event, double x, double y) {
    //  std::cout << "User clicked the ";

    if (event->button == 1) {
        //    std::cout << "left ";

        if (window_mode) {
            //click on any two points to form new window rectangle bound

            if (!window_point_1_collected) {
                //collect first point data

                window_point_1_collected = true;
                point_1 = {x, y};
            } else {
                //collect second point data

                //click on any two points to form new window rectangle bound
                ezgl::point2d point_2 = {x, y};
                ezgl::rectangle current_window = (app->get_canvas(
                                                      app->get_main_canvas_id()))
                                                     ->get_camera()
                                                     .get_world();

                //calculate a rectangle with the same ratio based on the two clicks
                double window_ratio = current_window.height()
                                      / current_window.width();
                double new_height = abs(point_1.y - point_2.y);
                double new_width = new_height / window_ratio;

                //zoom in
                ezgl::rectangle new_window = {point_1, {point_1.x + new_width, point_2.y}};
                (app->get_canvas(app->get_main_canvas_id()))->get_camera().set_world(new_window);

                //reset flags
                window_mode = false;
                window_point_1_collected = false;
            }
            app->refresh_drawing();
        } else {
            // regular clicking mode

            /* This routine is called when the user clicks in the graphics area. *
             * It determines if a clb was clicked on.  If one was, it is         *
             * highlighted in green, it's fanin nets and clbs are highlighted in *
             * blue and it's fanout is highlighted in red.  If no clb was        *
             * clicked on (user clicked on white space) any old highlighting is  *
             * removed.  Note that even though global nets are not drawn, their  *
             * fanins and fanouts are highlighted when you click on a block      *
             * attached to them.                                                 */

            /* Control + mouse click to select multiple nets. */
            if (!(event->state & GDK_CONTROL_MASK))
                deselect_all();

            //Check if we hit an rr node
            // Note that we check this before checking for a block, since pins and routing may appear overtop of a multi-width/height block
            if (highlight_rr_nodes(x, y)) {
                return; //Selected an rr node
            }

            highlight_blocks(x, y);
        }
    }
    //  else if (event->button == 2)
    //    std::cout << "middle ";
    //  else if (event->button == 3)
    //    std::cout << "right ";

    //  std::cout << "mouse button at coordinates (" << x << "," << y << ") " << std::endl;
}

void act_on_mouse_move(ezgl::application* app, GdkEventButton* event, double x, double y) {
    //  std::cout << "Mouse move at coordinates (" << x << "," << y << ") "<< std::endl;

    // user has clicked the window button, in window mode
    if (window_point_1_collected) {
        // draw a grey, dashed-line box to indicate the zoom-in region
        app->refresh_drawing();
        ezgl::renderer* g = app->get_renderer();
        g->set_line_dash(ezgl::line_dash::asymmetric_5_3);
        g->set_color(blk_GREY);
        g->set_line_width(2);
        g->draw_rectangle(point_1, {x, y});
        return;
    }

    // user has not clicked the window button, in regular mode
    t_draw_state* draw_state = get_draw_state_vars();

    if (draw_state->draw_rr_toggle != DRAW_NO_RR) {
        int hit_node = draw_check_rr_node_hit(x, y);

        if (hit_node != OPEN) {
            //Update message

            std::string info = describe_rr_node(hit_node);
            std::string msg = vtr::string_fmt("Moused over %s", info.c_str());
            app->update_message(msg.c_str());
        } else {
            if (!rr_highlight_message.empty()) {
                app->update_message(rr_highlight_message.c_str());
            } else {
                app->update_message(draw_state->default_message);
            }
        }
    }
    event = event; // just for hiding warning message
}

ezgl::point2d atom_pin_draw_coord(AtomPinId pin) {
    auto& atom_ctx = g_vpr_ctx.atom();

    AtomBlockId blk = atom_ctx.nlist.pin_block(pin);
    ClusterBlockId clb_index = atom_ctx.lookup.atom_clb(blk);
    const t_pb_graph_node* pg_gnode = atom_ctx.lookup.atom_pb_graph_node(blk);

    t_draw_coords* draw_coords = get_draw_coords_vars();
    ezgl::rectangle pb_bbox = draw_coords->get_absolute_pb_bbox(clb_index,
                                                                pg_gnode);

    //We place each atom pin inside it's pb bounding box
    //and distribute the pins along it's vertical centre line
    const float FRACTION_USABLE_WIDTH = 0.8;
    float width = pb_bbox.width();
    float usable_width = width * FRACTION_USABLE_WIDTH;
    float x_offset = pb_bbox.left() + width * (1 - FRACTION_USABLE_WIDTH) / 2;

    int pin_index, pin_total;
    find_pin_index_at_model_scope(pin, blk, &pin_index, &pin_total);

    const ezgl::point2d point = {x_offset + usable_width * pin_index / ((float)pin_total),
                                 pb_bbox.center_y()};

    return point;
}

//Returns the set of rr nodes which connect driver to sink
std::vector<int> trace_routed_connection_rr_nodes(
    const ClusterNetId net_id,
    const int driver_pin,
    const int sink_pin) {
    auto& route_ctx = g_vpr_ctx.routing();

    bool allocated_route_tree_structs = alloc_route_tree_timing_structs(true); //Needed for traceback_to_route_tree

    //Conver the traceback into an easily search-able
    t_rt_node* rt_root = traceback_to_route_tree(net_id);

    VTR_ASSERT(
        rt_root
        && rt_root->inode
               == route_ctx.net_rr_terminals[net_id][driver_pin]);

    int sink_rr_node = route_ctx.net_rr_terminals[net_id][sink_pin];

    std::vector<int> rr_nodes_on_path;

    //Collect the rr nodes
    trace_routed_connection_rr_nodes_recurr(rt_root, sink_rr_node,
                                            rr_nodes_on_path);

    //Traced from sink to source, but we want to draw from source to sink
    std::reverse(rr_nodes_on_path.begin(), rr_nodes_on_path.end());

    free_route_tree(rt_root);

    if (allocated_route_tree_structs) {
        free_route_tree_timing_structs();
    }
    return rr_nodes_on_path;
}

//Helper function for trace_routed_connection_rr_nodes
//Adds the rr nodes linking rt_node to sink_rr_node to rr_nodes_on_path
//Returns true if rt_node is on the path
bool trace_routed_connection_rr_nodes_recurr(const t_rt_node* rt_node,
                                             int sink_rr_node,
                                             std::vector<int>& rr_nodes_on_path) {
    //DFS from the current rt_node to the sink_rr_node, when the sink is found trace back the used rr nodes

    if (rt_node->inode == sink_rr_node) {
        rr_nodes_on_path.push_back(sink_rr_node);
        return true;
    }

    for (t_linked_rt_edge* edge = rt_node->u.child_list; edge != nullptr; edge = edge->next) {
        t_rt_node* child_rt_node = edge->child;
        VTR_ASSERT(child_rt_node);

        bool on_path_to_sink = trace_routed_connection_rr_nodes_recurr(
            child_rt_node, sink_rr_node, rr_nodes_on_path);

        if (on_path_to_sink) {
            rr_nodes_on_path.push_back(rt_node->inode);
            return true;
        }
    }

    return false; //Not on path to sink
}

//Find the edge between two rr nodes
t_edge_size find_edge(int prev_inode, int inode) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    for (t_edge_size iedge = 0;
         iedge < rr_graph.num_edges(RRNodeId(prev_inode)); ++iedge) {
        if (size_t(rr_graph.edge_sink_node(RRNodeId(prev_inode), iedge)) == size_t(inode)) {
            return iedge;
        }
    }
    VTR_ASSERT(false);
    return OPEN;
}

ezgl::color to_ezgl_color(vtr::Color<float> color) {
    return ezgl::color(color.r * 255, color.g * 255, color.b * 255);
}

static float get_router_expansion_cost(const t_rr_node_route_inf node_inf,
                                       e_draw_router_expansion_cost draw_router_expansion_cost) {
    if (draw_router_expansion_cost == DRAW_ROUTER_EXPANSION_COST_TOTAL
        || draw_router_expansion_cost
               == DRAW_ROUTER_EXPANSION_COST_TOTAL_WITH_EDGES) {
        return node_inf.path_cost;
    } else if (draw_router_expansion_cost == DRAW_ROUTER_EXPANSION_COST_KNOWN
               || draw_router_expansion_cost
                      == DRAW_ROUTER_EXPANSION_COST_KNOWN_WITH_EDGES) {
        return node_inf.backward_path_cost;
    } else if (draw_router_expansion_cost == DRAW_ROUTER_EXPANSION_COST_EXPECTED
               || draw_router_expansion_cost
                      == DRAW_ROUTER_EXPANSION_COST_EXPECTED_WITH_EDGES) {
        return node_inf.path_cost - node_inf.backward_path_cost;
    }

    VPR_THROW(VPR_ERROR_DRAW, "Invalid Router RR cost drawing type");
}

static void draw_router_expansion_costs(ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();
    if (draw_state->show_router_expansion_cost
        == DRAW_NO_ROUTER_EXPANSION_COST) {
        return;
    }

    auto& device_ctx = g_vpr_ctx.device();
    auto& routing_ctx = g_vpr_ctx.routing();

    std::vector<float> rr_costs(device_ctx.rr_graph.num_nodes());

    for (const RRNodeId& rr_id : device_ctx.rr_graph.nodes()) {
        float cost = get_router_expansion_cost(
            routing_ctx.rr_node_route_inf[(size_t)rr_id],
            draw_state->show_router_expansion_cost);
        rr_costs[(size_t)rr_id] = cost;
    }

    bool all_nan = true;
    for (const RRNodeId& rr_id : device_ctx.rr_graph.nodes()) {
        if (std::isinf(rr_costs[(size_t)rr_id])) {
            rr_costs[(size_t)rr_id] = NAN;
        } else {
            all_nan = false;
        }
    }

    if (!all_nan) {
        draw_rr_costs(g, rr_costs, false);
    }
    if (draw_state->show_router_expansion_cost
            == DRAW_ROUTER_EXPANSION_COST_TOTAL
        || draw_state->show_router_expansion_cost
               == DRAW_ROUTER_EXPANSION_COST_TOTAL_WITH_EDGES) {
        application.update_message(
            "Routing Expected Total Cost (known + estimate)");
    } else if (draw_state->show_router_expansion_cost
                   == DRAW_ROUTER_EXPANSION_COST_KNOWN
               || draw_state->show_router_expansion_cost
                      == DRAW_ROUTER_EXPANSION_COST_KNOWN_WITH_EDGES) {
        application.update_message("Routing Known Cost (from source to node)");
    } else if (draw_state->show_router_expansion_cost
                   == DRAW_ROUTER_EXPANSION_COST_EXPECTED
               || draw_state->show_router_expansion_cost
                      == DRAW_ROUTER_EXPANSION_COST_EXPECTED_WITH_EDGES) {
        application.update_message(
            "Routing Expected Cost (from node to target)");
    } else {
        VPR_THROW(VPR_ERROR_DRAW, "Invalid Router RR cost drawing type");
    }
}

static void highlight_blocks(double x, double y) {
    t_draw_coords* draw_coords = get_draw_coords_vars();

    char msg[vtr::bufsize];
    ClusterBlockId clb_index = EMPTY_BLOCK_ID;
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    /// determine block ///
    ezgl::rectangle clb_bbox;

    // iterate over grid x
    for (size_t i = 0; i < device_ctx.grid.width(); ++i) {
        if (draw_coords->tile_x[i] > x) {
            break; // we've gone to far in the x direction
        }
        // iterate over grid y
        for (size_t j = 0; j < device_ctx.grid.height(); ++j) {
            if (draw_coords->tile_y[j] > y) {
                break; // we've gone to far in the y direction
            }
            // iterate over sub_blocks
            const t_grid_tile* grid_tile = &device_ctx.grid[i][j];
            for (int k = 0; k < grid_tile->type->capacity; ++k) {
                clb_index = place_ctx.grid_blocks[i][j].blocks[k];
                if (clb_index != EMPTY_BLOCK_ID) {
                    clb_bbox = draw_coords->get_absolute_clb_bbox(clb_index,
                                                                  cluster_ctx.clb_nlist.block_type(clb_index));
                    if (clb_bbox.contains({x, y})) {
                        break;
                    } else {
                        clb_index = EMPTY_BLOCK_ID;
                    }
                }
            }
            if (clb_index != EMPTY_BLOCK_ID) {
                break; // we've found something
            }
        }
        if (clb_index != EMPTY_BLOCK_ID) {
            break; // we've found something
        }
    }

    if (clb_index == EMPTY_BLOCK_ID || clb_index == ClusterBlockId::INVALID()) {
        //Nothing found
        return;
    }

    VTR_ASSERT(clb_index != EMPTY_BLOCK_ID);

    // note: this will clear the selected sub-block if show_blk_internal is 0,
    // or if it doesn't find anything
    ezgl::point2d point_in_clb = ezgl::point2d(x, y) - clb_bbox.bottom_left();
    highlight_sub_block(point_in_clb, clb_index,
                        cluster_ctx.clb_nlist.block_pb(clb_index));

    if (get_selected_sub_block_info().has_selection()) {
        t_pb* selected_subblock = get_selected_sub_block_info().get_selected_pb();
        sprintf(msg, "sub-block %s (a \"%s\") selected",
                selected_subblock->name,
                selected_subblock->pb_graph_node->pb_type->name);
    } else {
        /* Highlight block and fan-in/fan-outs. */
        draw_highlight_blocks_color(cluster_ctx.clb_nlist.block_type(clb_index),
                                    clb_index);
        sprintf(msg, "Block #%zu (%s) at (%d, %d) selected.", size_t(clb_index),
                cluster_ctx.clb_nlist.block_name(clb_index).c_str(),
                place_ctx.block_locs[clb_index].loc.x,
                place_ctx.block_locs[clb_index].loc.y);
    }

    //If manual moves is activated, then user can select block from the grid.
    t_draw_state* draw_state = get_draw_state_vars();
    if (draw_state->manual_moves_state.manual_move_enabled) {
        draw_state->manual_moves_state.user_highlighted_block = true;
        if (!draw_state->manual_moves_state.manual_move_window_is_open) {
            draw_manual_moves_window(std::to_string(size_t(clb_index)));
        }
    }

    application.update_message(msg);
    application.refresh_drawing();
}
static void setup_default_ezgl_callbacks(ezgl::application* app) {
    // Connect press_proceed function to the Proceed button
    GObject* proceed_button = app->get_object("ProceedButton");
    g_signal_connect(proceed_button, "clicked", G_CALLBACK(ezgl::press_proceed),
                     app);

    // Connect press_zoom_fit function to the Zoom-fit button
    GObject* zoom_fit_button = app->get_object("ZoomFitButton");
    g_signal_connect(zoom_fit_button, "clicked",
                     G_CALLBACK(ezgl::press_zoom_fit), app);

    // Connect Pause button
    GObject* pause_button = app->get_object("PauseButton");
    g_signal_connect(pause_button, "clicked", G_CALLBACK(set_force_pause), app);

    // Connect Block Outline checkbox
    GObject* block_outline = app->get_object("blockOutline");
    g_signal_connect(block_outline, "toggled", G_CALLBACK(set_block_outline),
                     app);

    // Connect Block Text checkbox
    GObject* block_text = app->get_object("blockText");
    g_signal_connect(block_text, "toggled", G_CALLBACK(set_block_text), app);

    // Connect Clip Routing Util checkbox
    GObject* clip_routing = app->get_object("clipRoutingUtil");
    g_signal_connect(clip_routing, "toggled", G_CALLBACK(clip_routing_util),
                     app);

    // Connect Debug Button
    GObject* debugger = app->get_object("debugButton");
    g_signal_connect(debugger, "clicked", G_CALLBACK(draw_debug_window), NULL);
}

// Callback function for Block Outline checkbox
static void set_block_outline(GtkWidget* widget, gint /*response_id*/, gpointer /*data*/) {
    t_draw_state* draw_state = get_draw_state_vars();

    // assign corresponding bool value to draw_state->draw_block_outlines
    if (gtk_toggle_button_get_active((GtkToggleButton*)widget))
        draw_state->draw_block_outlines = true;
    else
        draw_state->draw_block_outlines = false;
    //redraw
    application.update_message(draw_state->default_message);
    application.refresh_drawing();
}

// Callback function for Block Text checkbox
static void set_block_text(GtkWidget* widget, gint /*response_id*/, gpointer /*data*/) {
    t_draw_state* draw_state = get_draw_state_vars();

    // assign corresponding bool value to draw_state->draw_block_text
    if (gtk_toggle_button_get_active((GtkToggleButton*)widget))
        draw_state->draw_block_text = true;
    else
        draw_state->draw_block_text = false;

    //redraw
    application.update_message(draw_state->default_message);
    application.refresh_drawing();
}

// Callback function for Clip Routing Util checkbox
static void clip_routing_util(GtkWidget* widget, gint /*response_id*/, gpointer /*data*/) {
    t_draw_state* draw_state = get_draw_state_vars();

    // assign corresponding bool value to draw_state->clip_routing_util
    if (gtk_toggle_button_get_active((GtkToggleButton*)widget))
        draw_state->clip_routing_util = true;
    else
        draw_state->clip_routing_util = false;

    //redraw
    application.update_message(draw_state->default_message);
    application.refresh_drawing();
}

static void set_force_pause(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/) {
    t_draw_state* draw_state = get_draw_state_vars();

    draw_state->forced_pause = true;
}

static void run_graphics_commands(std::string commands) {
    //A very simmple command interpreter for scripting graphics
    t_draw_state* draw_state = get_draw_state_vars();

    t_draw_state backup_draw_state = *draw_state;

    std::vector<std::vector<std::string>> cmds;
    for (std::string raw_cmd : vtr::split(commands, ";")) {
        cmds.push_back(vtr::split(raw_cmd));
    }

    for (auto& cmd : cmds) {
        VTR_ASSERT_MSG(cmd.size() > 0, "Expect non-empty graphics commands");

        for (auto& item : cmd) {
            VTR_LOG("%s ", item.c_str());
        }
        VTR_LOG("\n");

        if (cmd[0] == "save_graphics") {
            VTR_ASSERT_MSG(cmd.size() == 2,
                           "Expect filename after 'save_graphics'");

            auto name_ext = vtr::split_ext(cmd[1]);

            //Replace {i}  with the sequence number
            std::string name = vtr::replace_all(name_ext[0], "{i}",
                                                std::to_string(draw_state->sequence_number));

            save_graphics(/*extension=*/name_ext[1], /*filename=*/name);
            VTR_LOG("Saving to %s\n", std::string(name + name_ext[1]).c_str());

        } else if (cmd[0] == "set_macros") {
            VTR_ASSERT_MSG(cmd.size() == 2,
                           "Expect net draw state after 'set_macro'");
            draw_state->show_placement_macros = (e_draw_placement_macros)vtr::atoi(cmd[1]);
            VTR_LOG("%d\n", (int)draw_state->show_placement_macros);
        } else if (cmd[0] == "set_nets") {
            VTR_ASSERT_MSG(cmd.size() == 2,
                           "Expect net draw state after 'set_nets'");
            draw_state->show_nets = (e_draw_nets)vtr::atoi(cmd[1]);
            VTR_LOG("%d\n", (int)draw_state->show_nets);
        } else if (cmd[0] == "set_cpd") {
            VTR_ASSERT_MSG(cmd.size() == 2,
                           "Expect cpd draw state after 'set_cpd'");
            draw_state->show_crit_path = (e_draw_crit_path)vtr::atoi(cmd[1]);
            VTR_LOG("%d\n", (int)draw_state->show_crit_path);
        } else if (cmd[0] == "set_routing_util") {
            VTR_ASSERT_MSG(cmd.size() == 2,
                           "Expect routing util draw state after 'set_routing_util'");
            draw_state->show_routing_util = (e_draw_routing_util)vtr::atoi(
                cmd[1]);
            VTR_LOG("%d\n", (int)draw_state->show_routing_util);
        } else if (cmd[0] == "set_clip_routing_util") {
            VTR_ASSERT_MSG(cmd.size() == 2,
                           "Expect routing util draw state after 'set_routing_util'");
            draw_state->clip_routing_util = (bool)vtr::atoi(cmd[1]);
            VTR_LOG("%d\n", (int)draw_state->clip_routing_util);
        } else if (cmd[0] == "set_congestion") {
            VTR_ASSERT_MSG(cmd.size() == 2,
                           "Expect congestion draw state after 'set_congestion'");
            draw_state->show_congestion = (e_draw_congestion)vtr::atoi(cmd[1]);
            VTR_LOG("%d\n", (int)draw_state->show_congestion);
        } else if (cmd[0] == "set_draw_block_outlines") {
            VTR_ASSERT_MSG(cmd.size() == 2,
                           "Expect draw block outlines state after 'set_draw_block_outlines'");
            draw_state->draw_block_outlines = vtr::atoi(cmd[1]);
            VTR_LOG("%d\n", (int)draw_state->draw_block_outlines);
        } else if (cmd[0] == "set_draw_block_text") {
            VTR_ASSERT_MSG(cmd.size() == 2,
                           "Expect draw block text state after 'set_draw_block_text'");
            draw_state->draw_block_text = vtr::atoi(cmd[1]);
            VTR_LOG("%d\n", (int)draw_state->draw_block_text);
        } else if (cmd[0] == "set_draw_block_internals") {
            VTR_ASSERT_MSG(cmd.size() == 2,
                           "Expect draw state after 'set_draw_block_internals'");
            draw_state->show_blk_internal = vtr::atoi(cmd[1]);
            VTR_LOG("%d\n", (int)draw_state->show_blk_internal);
        } else if (cmd[0] == "set_draw_net_max_fanout") {
            VTR_ASSERT_MSG(cmd.size() == 2,
                           "Expect maximum fanout after 'set_draw_net_max_fanout'");
            draw_state->draw_net_max_fanout = vtr::atoi(cmd[1]);
            VTR_LOG("%d\n", (int)draw_state->draw_net_max_fanout);
        } else if (cmd[0] == "exit") {
            VTR_ASSERT_MSG(cmd.size() == 2, "Expect exit code after 'exit'");
            exit(vtr::atoi(cmd[1]));
        } else {
            VPR_ERROR(VPR_ERROR_DRAW,
                      vtr::string_fmt("Unrecognized graphics command '%s'",
                                      cmd[0].c_str())
                          .c_str());
        }
    }

    *draw_state = backup_draw_state; //Restor original draw state

    //Advance the sequence number
    ++draw_state->sequence_number;
}

ezgl::point2d tnode_draw_coord(tatum::NodeId node) {
    auto& atom_ctx = g_vpr_ctx.atom();

    AtomPinId pin = atom_ctx.lookup.tnode_atom_pin(node);
    return atom_pin_draw_coord(pin);
}

/* This routine highlights the blocks affected in the latest move      *
 * It highlights the old and new locations of the moved blocks         *
 * It also highlights the moved block input and output terminals       *
 * Currently, it is used in placer debugger when breakpoint is reached */
void highlight_moved_block_and_its_terminals(
    const t_pl_blocks_to_be_moved& blocks_affected) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    //clear all selected blocks
    deselect_all();

    //highlight the input/output terminals of the moved block
    draw_highlight_blocks_color(
        cluster_ctx.clb_nlist.block_type(
            blocks_affected.moved_blocks[0].block_num),
        blocks_affected.moved_blocks[0].block_num);

    //highlight the old and new locations of the moved block
    clear_colored_locations();
    set_draw_loc_color(blocks_affected.moved_blocks[0].old_loc,
                       OLD_BLK_LOC_COLOR);
    set_draw_loc_color(blocks_affected.moved_blocks[0].old_loc,
                       NEW_BLK_LOC_COLOR);
}

// pass in an (x,y,subtile) location and the color in which it should be drawn.
// This overrides the color of any block placed in that location, and also applies if the location is empty.
void set_draw_loc_color(t_pl_loc loc, ezgl::color clr) {
    t_draw_state* draw_state = get_draw_state_vars();
    draw_state->colored_locations.push_back(std::make_pair(loc, clr));
}

// clear the colored_locations vector
void clear_colored_locations() {
    t_draw_state* draw_state = get_draw_state_vars();
    draw_state->colored_locations.clear();
}

// This routine takes in a (x,y) location.
// If the input loc is marked in colored_locations vector, the function will return true and the correspnding color is sent back in loc_color
// otherwise, the function returns false (the location isn't among the highlighted locations)
bool highlight_loc_with_specific_color(int x, int y, ezgl::color& loc_color) {
    t_draw_state* draw_state = get_draw_state_vars();

    //define a (x,y) location variable
    t_pl_loc curr_loc;
    curr_loc.x = x;
    curr_loc.y = y;

    //search for the current location in the vector of colored locations
    auto it = std::find_if(draw_state->colored_locations.begin(),
                           draw_state->colored_locations.end(),
                           [&curr_loc](const std::pair<t_pl_loc, ezgl::color>& vec_element) {
                               return (vec_element.first.x == curr_loc.x
                                       && vec_element.first.y == curr_loc.y);
                           });

    if (it != draw_state->colored_locations.end()) {
        /* found a colored location at the spot I am drawing *
         * (currently used for drawing the current move).    *
         * This overrides any block color.                   */
        loc_color = it->second;
        return true;
    }

    return false;
}

ezgl::color get_block_type_color(t_physical_tile_type_ptr type) {
    //Wrap around if there are too many blocks
    // This ensures we support an arbitrary number of types,
    // although the colours may repeat
    ezgl::color color = block_colors[type->index % block_colors.size()];

    return color;
}

//Lightens a color's luminance [0, 1] by an aboslute 'amount'
ezgl::color lighten_color(ezgl::color color, float amount) {
    constexpr double MAX_LUMINANCE = 0.95; //Clip luminance so it doesn't go full white
    auto hsl = color2hsl(color);

    hsl.l = std::max(0., std::min(MAX_LUMINANCE, hsl.l + amount));

    return hsl2color(hsl);
}

#endif /* NO_GRAPHICS */
