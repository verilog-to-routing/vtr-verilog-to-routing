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
 * Authors: Vaughn Betz, Long Yu (Mike) Wang, Dingyu (Tina) Yang, Sebastian Lievano
 * Last updated: August 2022
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <set>
#include <vector>
#include "draw.h"

#include "draw_interposer.h"
#include "draw_types.h"
#include "rr_graph_fwd.h"
#include "rr_node_types.h"
#include "timing_info.h"
#include "physical_types.h"

#include "move_utils.h"
#include "vpr_types.h"

#ifndef NO_GRAPHICS

#include <algorithm>
#include <array>
#include <iostream>

#include "draw_debug.h"
#include "vtr_assert.h"
#include "vtr_ndoffsetmatrix.h"
#include "vtr_log.h"
#include "vtr_color_map.h"
#include "vtr_path.h"

#include "vpr_error.h"

#include "globals.h"
#include "draw_color.h"
#include "draw_basic.h"
#include "draw_rr.h"
#include "draw_searchbar.h"
#include "draw_global.h"
#include "intra_logic_block.h"
#include "hsl.h"
#include "search_bar.h"
#include "save_graphics.h"
#include "manual_moves.h"
#include "draw_noc.h"
#include "draw_floorplanning.h"
#include "draw_crit_path.h"

#include "ui_setup.h"
#include "ezgl/qt/render_backend.hpp"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QDialogButtonBox>

//To process key presses we need the X11 keysym definitions,
//which are unavailable when building with MINGW
#if defined(X11) && !defined(__MINGW32__)
#include <X11/keysym.h>
#endif

#include "place_macro.h"
#include "draw_rr.h"
/****************************** Define Macros *******************************/

static constexpr ezgl::color DEFAULT_RR_NODE_COLOR = ezgl::BLACK;
static constexpr ezgl::color OLD_BLK_LOC_COLOR = blk_GOLD;
static constexpr ezgl::color NEW_BLK_LOC_COLOR = blk_GREEN;
//#define TIME_DRAWSCREEN /* Enable if want to track runtime for drawscreen() */

void act_on_key_press(ezgl::application* /*app*/, QKeyEvent* event, const std::string& key_name);
void act_on_mouse_press(ezgl::application* app, QMouseEvent* event, double x, double y);
void act_on_mouse_move(ezgl::application* app, QMouseEvent* event, double x, double y);

static void highlight_blocks(double x, double y);

static float get_router_expansion_cost(const t_rr_node_route_inf& node_inf,
                                       e_draw_router_expansion_cost draw_router_expansion_cost);
static void draw_router_expansion_costs(ezgl::renderer* g);

static void draw_main_canvas(ezgl::renderer* g);

/**
 * @brief Generalized callback function to setup the UI when the stage changes.
 */
static void on_stage_change_setup(ezgl::application* app, bool is_new_window);

static void setup_default_ezgl_callbacks(ezgl::application* app);
static void set_force_pause();
static void set_block_outline(bool checked);
static void set_block_text(bool checked);
static void set_draw_partitions(bool checked);
static void clip_routing_util(bool checked);
static void run_graphics_commands(const std::string& commands);

/************************** File Scope Variables ****************************/

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

ezgl::application::settings settings(":/ezgl/main.ui", "MainWindow", "MainCanvas", "org.verilogtorouting.vpr.PID" + std::to_string(vtr::get_pid()), setup_default_ezgl_callbacks);
// provide fake argc and argv required for QApplication initialization
int argc = 1;
char appName[] = "vpr";
char* argv[] = {appName, nullptr};
ezgl::application* application = nullptr;

bool window_mode = false;
bool window_point_1_collected = false;
ezgl::point2d point_1(0, 0);
ezgl::point2d window_preview_cursor(0, 0); // updated by act_on_mouse_move while
                                           // window_point_1_collected is true; drawn
                                           // as a dashed rect by the regular draw flow.
ezgl::rectangle initial_world;
std::string rr_highlight_message;

// Used for scripted graphics (rendered to files via --graphics_commands).
// Stages that have hit their first update_screen() (auto-recorded by
// update_screen on pic_on_screen transitions) and stages that have been
// marked complete via notify_stage_complete(). Consulted by the
// `wait_for_stage <stage>_initial` / `<stage>_done` script barriers.
std::set<e_pic_type> initial_stages;
std::set<e_pic_type> completed_stages;

// Used for scripted graphics (rendered to files via --graphics_commands).
// `exit N` from --graphics_commands is processed deferredly: the
// interpreter sets these flags and breaks, then update_screen() honors
// the request after the current render checkpoint completes. This avoids
// terminating VPR mid-iteration. Both log lines start with the literal
// "Graphics-command 'exit" — run_vtr_flow.py keys off that prefix to
// skip downstream convergence/consistency checks for runs intentionally
// terminated by a graphics command.
static bool pending_graphics_exit = false;
static int pending_graphics_exit_code = 0;

#endif // NO_GRAPHICS

/********************** Subroutine definitions ******************************/

void init_graphics_state(bool show_graphics_val,
                         int gr_automode_val,
                         enum e_route_type route_type,
                         bool save_graphics,
                         std::string graphics_commands,
                         std::string renderer_type,
                         bool is_flat) {
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
    draw_state->renderer_type = renderer_type;
    draw_state->is_flat = is_flat;

    // When --disp is off, force Qt into offscreen mode before QApplication is
    // created so it doesn't try to connect to an X11/Wayland display.
    if (!show_graphics_val && !qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }

    // Create the application object here (not at file scope) so that the
    // QApplication lifetime is bounded by init/close_graphics, not by
    // static-object construction/destruction order.
    if (application == nullptr) {
        application = new ezgl::application(settings, argc, argv);
    }

#else
    //Suppress unused parameter warnings
    (void)show_graphics_val;
    (void)gr_automode_val;
    (void)route_type;
    (void)save_graphics;
    (void)graphics_commands;
    (void)renderer_type;
    (void)is_flat;
#endif // NO_GRAPHICS
}

void notify_stage_complete(e_pic_type stage) {
#ifndef NO_GRAPHICS
    completed_stages.insert(stage);
#else
    (void)stage;
#endif
}

#ifndef NO_GRAPHICS

static void draw_main_canvas(ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();

    g->set_font_size(14);

    if (draw_state->pic_on_screen != e_pic_type::ANALYTICAL_PLACEMENT) {
        draw_interposer_cuts(g);

        draw_block_pin_util();
        draw_place(g);
        draw_internal_draw_subblk(g);

        if (draw_state->pic_on_screen == e_pic_type::ROUTING) { // ROUTING on screen

            draw_rr(g);

            if (draw_state->show_nets && draw_state->draw_nets == DRAW_ROUTED_NETS) {
                draw_route(ALL_NETS, g);

                if (draw_state->highlight_fan_in_fan_out) {
                    draw_route(HIGHLIGHTED, g);
                }
            }

            draw_congestion(g);

            draw_routing_costs(g);

            draw_router_expansion_costs(g);

            draw_routing_util(g);

            draw_routing_bb(g);
        }

        draw_placement_macros(g);

#ifndef NO_SERVER
        if (g_vpr_ctx.server().gate_io.is_running()) {
            const ServerContext& server_ctx = g_vpr_ctx.server(); // shortcut
            draw_crit_path_elements(server_ctx.crit_paths, server_ctx.crit_path_element_indexes, server_ctx.draw_crit_path_contour, g);
        } else {
            draw_crit_path(g);
        }
#else
        draw_crit_path(g);
#endif /* NO_SERVER */

        draw_logical_connections(g);

        draw_selected_pb_flylines(g);

        draw_noc(g);

        if (draw_state->draw_partitions) {
            highlight_all_regions(g);
            draw_constrained_atoms(g);
        }

        if (draw_state->color_map) {
            draw_color_map_legend(*draw_state->color_map, g);
            draw_state->color_map.reset(); //Free color map in preparation for next redraw
        }
    } else {
        draw_analytical_place(g);
    }

    // Zoom-Select preview: while the user is in window mode and has clicked
    // the first point, paint a dashed grey rectangle from that anchor to the
    // current cursor. Drawing it here (inside the regular draw flow) instead
    // of on an animation overlay makes the preview work uniformly across all
    // backends, including RHI.
    if (window_point_1_collected) {
        g->set_line_dash(ezgl::line_dash::asymmetric_5_3);
        g->set_color(blk_GREY);
        g->set_line_width(2);
        g->draw_rectangle(point_1, window_preview_cursor);
        // Reset to defaults so subsequent overlays (e.g. legend) aren't
        // accidentally inherited.
        g->set_line_dash(ezgl::line_dash::none);
        g->set_line_width(0);
    }

    if (draw_state->auto_proceed) {
        //Automatically exit the event loop, so user's don't need to manually click proceed

        //Avoid trying to repeatedly exit (which would cause errors in GTK)
        draw_state->auto_proceed = false;

        application->quit(); //Ensure we leave the event loop
    }
}

static void on_stage_change_setup(ezgl::application* app, bool is_new_window) {
    // default setup for new window
    if (is_new_window) {
        basic_button_setup(app);
        net_button_setup(app);
        block_button_setup(app);
        search_setup(app);
        routing_button_setup(app);
        view_button_setup(app);
        crit_path_button_setup(app);
    }

    t_draw_state* draw_state = get_draw_state_vars();

    if (draw_state->pic_on_screen == e_pic_type::PLACEMENT) {
        app->hide_widget("RoutingMenuButton");

        draw_state->save_graphics_file_base = "vpr_placement";

    } else if (draw_state->pic_on_screen == e_pic_type::ANALYTICAL_PLACEMENT) {
        // Routing has not started during analytical placement, so hide the
        // routing controls (otherwise the Route button/switch/checkboxes are
        // visible and interactable before routing commences).
        app->hide_widget("RoutingMenuButton");

        draw_state->save_graphics_file_base = "vpr_analytical_placement";

    } else if (draw_state->pic_on_screen == e_pic_type::ROUTING) {
        app->show_widget("RoutingMenuButton");

        draw_state->save_graphics_file_base = "vpr_routing";
    }

    // show/hide critical path routing UI elements
    hide_crit_path_routing(app);

    hide_draw_routing(app);

    app->update_message(draw_state->default_message);

    // Force a canvas redraw so the new picture type is visible immediately
    // when the event loop starts, rather than waiting for user interaction.
    app->refresh_drawing();
}

#endif //NO_GRAPHICS

void update_screen(ScreenUpdatePriority priority,
                   const char* msg,
                   e_pic_type pic_on_screen_val,
                   std::shared_ptr<const SetupTimingInfo> setup_timing_info) {
#ifndef NO_GRAPHICS

    /* Updates the screen if the user has requested graphics.  The priority  *
     * value controls whether or not the Proceed button must be clicked to   *
     * continue.  Saves the pic_on_screen_val to allow pan and zoom redraws. */
    t_draw_state* draw_state = get_draw_state_vars();

    // The application object is created lazily in vpr_init_graphics(), which is
    // called after the AP flow.  If we are called before that (e.g. from the
    // global placer's draw callbacks), bail out — there is nothing to draw.
    if (application == nullptr)
        return;

    strcpy(draw_state->default_message, msg);

    if (!draw_state->show_graphics) {
        ezgl::set_disable_event_loop(true);
    } else {
        ezgl::set_disable_event_loop(false);
    }

    bool state_change = false;

    /* If it's the type of picture displayed has changed, set up the proper  *
     * buttons.                                                              */
    if (draw_state->pic_on_screen != pic_on_screen_val) { //State changed

        state_change = true;

        if (draw_state->show_graphics) {
            if (pic_on_screen_val == e_pic_type::ANALYTICAL_PLACEMENT) {
                set_initial_world_ap();
            } else {
                set_initial_world();
            }
        }

        if (draw_state->pic_on_screen == e_pic_type::NO_PICTURE) {
            auto* canvas = application->add_canvas("MainCanvas", draw_main_canvas, initial_world);
            if (canvas != nullptr) {
                ezgl::renderer_type rt = ezgl::renderer_type::rhi;
                if (draw_state->renderer_type == "immediate")
                    rt = ezgl::renderer_type::immediate;
                else if (draw_state->renderer_type == "deferred")
                    rt = ezgl::renderer_type::deferred;

                // The QRhiWidget path (used only under --disp on) cannot
                // acquire a QRhi from QPlatformBackingStore::rhi() under the
                // offscreen QPA — the plugin returns nullptr. Headless
                // (--disp off) is fine: render_to_image() creates an
                // offscreen QRhi directly without QRhiWidget. So scope the
                // fallback to the widget case only.
                if (rt == ezgl::renderer_type::rhi
                    && draw_state->show_graphics
                    && qEnvironmentVariable("QT_QPA_PLATFORM") == "offscreen") {
                    VTR_LOG_WARN(
                        "QRhiWidget cannot run under QT_QPA_PLATFORM=offscreen "
                        "with --disp on; falling back to the immediate renderer.\n");
                    rt = ezgl::renderer_type::immediate;
                }

                canvas->set_renderer_type(rt);

                // Surface the renderer that actually got installed (which may
                // differ from --renderer after the offscreen-QPA fallback
                // above) so logs/tests can confirm the active backend.
                VTR_LOG("EZGL: active renderer backend: %s\n",
                        ezgl::renderer_type_name(rt));
            }
        } else {
            // TODO: will this ever be null?
            auto canvas = application->get_canvas(application->get_main_canvas_id());
            if (canvas != nullptr) {
                // Same coordinate frame: set_world() keeps the user's pan/zoom.
                // Changed frame (e.g. AP grid space -> tile space): reset_world().
                // set_world() does not refresh camera::m_initial_world, and the
                // RHI renderer sizes its geometry cull tile-grid from it; left
                // stale at the old frame, all geometry is culled (blank canvas,
                // only the text overlay survives). reset_world() refreshes it.
                if (initial_world == canvas->get_camera().get_initial_world()) {
                    canvas->get_camera().set_world(initial_world);
                } else {
                    canvas->get_camera().reset_world(initial_world);
                }
            }
        }

        draw_state->setup_timing_info = setup_timing_info;
        draw_state->pic_on_screen = pic_on_screen_val;
        // Record that we've now reached this stage at least once. Consumed
        // by the `wait_for_stage <stage>_initial` script barrier.
        initial_stages.insert(pic_on_screen_val);
    }

    bool should_pause = int(priority) >= draw_state->gr_automode;

    //If there was a state change, we must call ezgl::application::run() to update the buttons.
    //However, by default this causes graphics to pause for user interaction.
    //
    //If the priority is such that we shouldn't pause we need to continue automatically, so
    //the user won't need to click manually.
    draw_state->auto_proceed = (state_change && !should_pause);

    // Headless mode (save_graphics / graphics_commands without --disp): never
    // block for user interaction — there is no user at the keyboard. Always
    // auto-proceed so the event loop exits immediately after setup.
    if (!draw_state->show_graphics)
        draw_state->auto_proceed = true;

    if (state_change                   //Must update buttons
        || should_pause                //The priority means graphics should pause for user interaction
        || draw_state->forced_pause) { //The user asked to pause

        if (draw_state->forced_pause) {
            VTR_LOG("Pausing in interactive graphics (user pressed 'Pause')\n");
            draw_state->forced_pause = false; //Reset pause flag
        }

        const bool has_cmds = !draw_state->graphics_commands.empty();

        // Under --disp on, run the script inside the event loop. Without this,
        // QT_QPA_PLATFORM=offscreen delivers no input events, auto-proceed
        // never fires, and exec() sleeps forever.
        if (has_cmds && draw_state->show_graphics) {
            std::string cmds = draw_state->graphics_commands;
            application->schedule_initial_callback([cmds]() {
                run_graphics_commands(cmds);
                // If the script didn't `exit`, return from exec() so VPR can
                // continue with subsequent stages.
                QCoreApplication::quit();
            });
        }

        application->run(on_stage_change_setup, act_on_mouse_press, act_on_mouse_move,
                         act_on_key_press);

        // --disp off path: application::run() short-circuits via
        // disable_event_loop, so commands must run synchronously here.
        if (has_cmds && !draw_state->show_graphics) {
            run_graphics_commands(draw_state->graphics_commands);
        }
    }

    if (draw_state->show_graphics) {
        application->update_message(msg);
        application->refresh_drawing();
        application->flush_drawing();
    }

    if (draw_state->save_graphics) {
        std::string extension = "pdf";
        save_graphics(extension, draw_state->save_graphics_file_base);
    }

    // Honor a deferred `exit N` request from a graphics command. We are at
    // a render-safe checkpoint here (refresh/flush/save_graphics have
    // completed). Emit the sentinel so run_vtr_flow.py recognizes this as
    // an intentional early termination, then exit with the requested code.
    // exit() flushes stdio, so the sentinel reliably lands in vpr.out.
    if (pending_graphics_exit) {
        VTR_LOG("Graphics-command 'exit %d' fired: terminating now.\n",
                pending_graphics_exit_code);
        exit(pending_graphics_exit_code);
    }

#else
    (void)setup_timing_info;
    (void)priority;
    (void)msg;
    (void)pic_on_screen_val;
#endif //NO_GRAPHICS
}

#ifndef NO_GRAPHICS
void toggle_window_mode(QWidget* /*widget*/,
                        ezgl::application* app) {
    window_mode = true;
    app->update_message("Zoom to Selection: Click on two points to define a rectangle to zoom into.");
    app->refresh_drawing();
}

#endif // NO_GRAPHICS

void alloc_draw_structs(const t_arch* arch) {
#ifndef NO_GRAPHICS
    /* Call accessor functions to retrieve global variables. */
    t_draw_coords* draw_coords = get_draw_coords_vars();
    t_draw_state* draw_state = get_draw_state_vars();
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    /* Allocate the structures needed to draw the placement and routing->  Set *
     * up the default colors for blocks and nets.                             */
    draw_coords->tile_x.resize(device_ctx.grid.width(), 0.f);
    draw_coords->tile_y.resize(device_ctx.grid.height(), 0.f);

    /* For sub-block drawings inside clbs */
    draw_internal_alloc_blk();

    if (draw_state->is_flat) {
        draw_state->net_color.resize(atom_ctx.netlist().nets().size());
    } else {
        draw_state->net_color.resize(cluster_ctx.clb_nlist.nets().size());
    }

    draw_state->block_color_.resize(cluster_ctx.clb_nlist.blocks().size());
    draw_state->use_default_block_color_.resize(
        cluster_ctx.clb_nlist.blocks().size());

    /* Space is allocated for draw_rr_node but not initialized because we do *
     * not yet know information about the routing resources.				  */
    draw_state->draw_rr_node.resize(device_ctx.rr_graph.num_nodes());

    draw_state->draw_layer_display.resize(device_ctx.grid.get_num_layers());
    //By default show the lowest layer only. This is the only die layer for 2D FPGAs
    draw_state->draw_layer_display[0].visible = true;

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
    t_draw_coords* draw_coords = get_draw_coords_vars();

    if (draw_coords != nullptr) {
        vtr::release_memory(draw_coords->tile_x);
        vtr::release_memory(draw_coords->tile_y);
    }

    delete application;
    application = nullptr;

#else
    ;
#endif /* NO_GRAPHICS */
}

void init_draw_coords(float clb_width, const BlkLocRegistry& blk_loc_registry) {
#ifndef NO_GRAPHICS
    t_draw_state* draw_state = get_draw_state_vars();
    t_draw_coords* draw_coords = get_draw_coords_vars();
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const DeviceGrid& grid = device_ctx.grid;
    const RRGraphView& rr_graph = device_ctx.rr_graph;

    // Store a reference to block location variables so that other drawing
    // functions can access block location information without accessing
    // the global placement state, which is inaccessible during placement.
    draw_state->set_graphics_blk_loc_registry_ref(blk_loc_registry);

    // do not initialize only if --disp off and --save_graphics off
    if (!draw_state->show_graphics && !draw_state->save_graphics && draw_state->graphics_commands.empty()) {
        return;
    }

    // Each time routing is on screen, need to reallocate the color of each
    // rr_node, as the number of rr_nodes may change.
    if (rr_graph.num_nodes() != 0) {
        draw_state->draw_rr_node.resize(rr_graph.num_nodes());
        for (RRNodeId inode : rr_graph.nodes()) {
            draw_state->draw_rr_node[inode].color = DEFAULT_RR_NODE_COLOR;
            draw_state->draw_rr_node[inode].node_highlighted = false;
        }
    }

    draw_coords->set_tile_width(clb_width);
    draw_coords->pin_size = 0.3;
    for (const t_physical_tile_type& type : device_ctx.physical_tile_types) {
        int num_pins = type.num_pins;
        if (num_pins > 0) {
            draw_coords->pin_size = std::min(draw_coords->pin_size,
                                             draw_coords->get_tile_width() / (4.0F * num_pins));
        }
    }

    std::vector<int> max_chanx_ptc_nums(grid.height() - 1);
    std::vector<int> max_chany_ptc_nums(grid.width() - 1);

    // Normally you could use device_ctx.rr_chany_width to get the device's channel width but in some cases
    // like devices with interposer cuts, you have situations where there are 100 segments in a channel,
    // but the maximum ptc_num is 120. If we only reserve enough space for 100 segments, the segments with
    // ptc_num > 100 would be drawn over the tiles. This loop explicitly calculates the maximum ptc_num in
    // each routing row/column to avoid that issue.
    for (size_t layer = 0; layer < grid.get_num_layers(); layer++) {
        for (size_t x_loc = 0; x_loc < grid.width() - 1; x_loc++) {
            for (size_t y_loc = 0; y_loc < grid.height() - 1; y_loc++) {

                // Get all chanx nodes at location (x_loc, y_loc), find largest ptc_num across all nodes
                std::vector<RRNodeId> chanx_nodes = rr_graph.node_lookup().find_channel_nodes(layer, x_loc, y_loc, e_rr_type::CHANX);
                int max_chanx_ptc_num = 0;
                if (!chanx_nodes.empty()) {
                    // Must explicitly check for emptiness, since std::ranges::max will crash if chanx_nodes is empty
                    RRNodeId max_chanx_ptc_node = std::ranges::max(chanx_nodes, {}, [&rr_graph](RRNodeId node) { return rr_graph.node_ptc_num(node); });
                    max_chanx_ptc_num = rr_graph.node_ptc_num(max_chanx_ptc_node);
                }

                // Do the same for chany
                std::vector<RRNodeId> chany_nodes = rr_graph.node_lookup().find_channel_nodes(layer, x_loc, y_loc, e_rr_type::CHANY);
                int max_chany_ptc_num = 0;
                if (!chany_nodes.empty()) {
                    RRNodeId max_chany_ptc_node = std::ranges::max(chany_nodes, {}, [&rr_graph](RRNodeId node) { return rr_graph.node_ptc_num(node); });
                    max_chany_ptc_num = rr_graph.node_ptc_num(max_chany_ptc_node);
                }

                // Update maximum ptc_num for each routing channel row/column
                max_chany_ptc_nums[x_loc] = std::max(max_chany_ptc_num, max_chany_ptc_nums[x_loc]);
                max_chanx_ptc_nums[y_loc] = std::max(max_chanx_ptc_num, max_chanx_ptc_nums[y_loc]);
            }
        }
    }

    size_t j = 0;
    for (size_t i = 0; i < grid.width() - 1; i++) {
        draw_coords->tile_x[i] = i * draw_coords->get_tile_width() + j;
        j += max_chany_ptc_nums[i] + 2; // N wires need N + 1 units of space, plus one more since max ptc_num of 0 means 1 wire in the channel
    }
    draw_coords->tile_x[grid.width() - 1] = (grid.width() - 1) * draw_coords->get_tile_width() + j;

    j = 0;
    for (size_t i = 0; i < grid.height() - 1; ++i) {
        draw_coords->tile_y[i] = i * draw_coords->get_tile_width() + j;
        j += max_chanx_ptc_nums[i] + 2;
    }
    draw_coords->tile_y[grid.height() - 1] = (grid.height() - 1) * draw_coords->get_tile_width() + j;

    // Load coordinates of sub-blocks inside the clbs
    draw_internal_init_blk();

    // Tile coordinates are now fully populated — set initial_world so that
    // save_graphics / graphics_commands can compute valid image dimensions
    // even in headless mode (show_graphics = false).
    const ezgl::rectangle prev_initial_world = initial_world;
    set_initial_world();

    // The router's binary search re-runs init_draw_coords() per channel-width
    // trial, which can change tile_x/tile_y (and therefore initial_world)
    // without a pic_on_screen state change — so update_screen()'s state-change
    // branch will not refresh the camera. If the user is currently at "fit"
    // (camera world == previous initial_world) push the new initial_world to
    // the camera so the next render uses the correct world rect. If the user
    // has pan/zoomed away from fit, leave their view alone.
    if (application != nullptr && initial_world != prev_initial_world) {
        ezgl::canvas* canvas = application->get_canvas(application->get_main_canvas_id());
        if (canvas != nullptr && canvas->get_camera().get_world() == prev_initial_world) {
            canvas->get_camera().set_world(initial_world);
        }
    }

#else
    (void)clb_width;
    (void)blk_loc_registry;
#endif /* NO_GRAPHICS */
}

#ifndef NO_GRAPHICS

void set_initial_world() {
    t_draw_coords* draw_coords = get_draw_coords_vars();
    const DeviceGrid& grid = g_vpr_ctx.device().grid;

    // Margin beyond edge of the drawn device to extend the visible world
    // Setting this to > 0.0 means 'Zoom Fit' leave some fraction of white
    // space around the device edges
    constexpr float VISIBLE_MARGIN = 0.01;

    float draw_width = draw_coords->tile_x[grid.width() - 1] + draw_coords->get_tile_width();
    float draw_height = draw_coords->tile_y[grid.height() - 1] + draw_coords->get_tile_width();

    initial_world = ezgl::rectangle(
        {-VISIBLE_MARGIN * draw_width, -VISIBLE_MARGIN * draw_height},
        {(1. + VISIBLE_MARGIN) * draw_width, (1. + VISIBLE_MARGIN)
                                                 * draw_height});
}

void set_initial_world_ap() {
    constexpr float VISIBLE_MARGIN = 0.01f;
    const DeviceContext& device_ctx = g_vpr_ctx.device();

    const size_t grid_w = device_ctx.grid.width();
    const size_t grid_h = device_ctx.grid.height();

    float draw_width = static_cast<float>(grid_w);
    float draw_height = static_cast<float>(grid_h);

    initial_world = ezgl::rectangle(
        {-VISIBLE_MARGIN * draw_width, -VISIBLE_MARGIN * draw_height},
        {(1.f + VISIBLE_MARGIN) * draw_width, (1.f + VISIBLE_MARGIN) * draw_height});
}

double get_pixels_per_world_unit(ezgl::renderer* g) {
    double world_width = g->get_visible_world().width();
    double screen_width = g->get_visible_screen().width();
    // If using this function for decluttering purpose:
    // This ratio is sufficient for determining when decluttering should be on, and no other factors (e.g. channel width, tile width)
    // are needed, because a channel node usually occupies one world unit (in width), yet it is also always drawn in one pixel
    // in spite of the zoom level. The channel nodes are also placed contiguously and in parallel. Therefore, when the ratio
    // returned by this function is below 1, it means that the channel nodes are blended together and should be decluttered.
    return screen_width / world_width;
}

int get_track_num(int inode, const vtr::OffsetMatrix<int>& chanx_track, const vtr::OffsetMatrix<int>& chany_track) {
    /* Returns the track number of this routing resource node.   */
    e_rr_type rr_type;
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;
    RRNodeId rr_node = RRNodeId(inode);

    if (get_draw_state_vars()->draw_route_type == e_route_type::DETAILED)
        return (rr_graph.node_track_num(rr_node));

    /* GLOBAL route stuff below. */

    rr_type = rr_graph.node_type(rr_node);
    int i = rr_graph.node_xlow(rr_node); // Global rr graphs must have only unit
    int j = rr_graph.node_ylow(rr_node); // length channel segments.

    switch (rr_type) {
        case e_rr_type::CHANX:
            return (chanx_track[i][j]);

        case e_rr_type::CHANY:
            return (chany_track[i][j]);

        default:
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                      "in get_track_num: Unexpected node type %d for node %d.\n",
                      rr_type, inode);
            return UNDEFINED;
    }
}

/* This helper function determines whether a net has been highlighted. The highlighting
 * could be caused by the user clicking on a routing resource, toggled, or
 * fan-in/fan-out of a highlighted node.
 */
bool draw_if_net_highlighted(ParentNetId inet) {
    t_draw_state* draw_state = get_draw_state_vars();

    if (draw_state->net_color[inet] != DEFAULT_RR_NODE_COLOR) {
        return true;
    }
    return false;
}

/**
 * @brief cbk function for key press
 *
 * At the moment, only does something if user is currently typing in searchBar and
 * hits enter, at which point it runs autocomplete
 */
void act_on_key_press(ezgl::application* app, QKeyEvent* /*event*/, const std::string& key_name) {
    QLineEdit* searchBar = app->find_line_edit("TextInput");
    if (!searchBar) {
        return;
    }
    QString text(searchBar->text());
    t_draw_state* draw_state = get_draw_state_vars();
    if (searchBar->hasFocus()) {
        if (key_name == "Return" || key_name == "Tab") {
            enable_autocomplete(app);
            searchBar->setCursorPosition(text.length());
            return;
        }
    }
    if (draw_state->justEnabled) {
        draw_state->justEnabled = false;
    } else {
        searchBar->setCompleter(nullptr);
    }
    if (key_name == "Escape") {
        deselect_all();
    }
}

void act_on_mouse_press(ezgl::application* app, QMouseEvent* event, double x, double y) {
    if (event->button() == Qt::LeftButton) {

        if (window_mode) {
            //click on any two points to form new window rectangle bound

            if (!window_point_1_collected) {
                //collect first point data

                window_point_1_collected = true;
                point_1 = {x, y};
                // Seed the preview cursor at the anchor so the dashed preview
                // rect has zero area until the next mouse-move event arrives.
                // Without this, a stale cursor from a previous zoom-select
                // operation would briefly produce a wrong-sized rectangle.
                window_preview_cursor = {x, y};
            } else {
                //collect second point data

                //click on any two points to form new window rectangle bound
                ezgl::point2d point_2 = {x, y};
                // get_canvas() returns nullptr if the canvas hasn't been
                // registered yet (canvas is added lazily in update_screen()
                // on the first stage transition). Bail out rather than
                // dereferencing — same pattern as update_screen() above.
                auto* canvas = app->get_canvas(app->get_main_canvas_id());
                if (canvas == nullptr) return;
                ezgl::rectangle current_window = canvas->get_camera().get_world();

                //calculate a rectangle with the same ratio based on the two clicks
                double window_ratio = current_window.height()
                                      / current_window.width();
                double new_height = abs(point_1.y - point_2.y);
                double new_width = new_height / window_ratio;

                //zoom in
                ezgl::rectangle new_window = {point_1, {point_1.x + new_width, point_2.y}};
                canvas->get_camera().set_world(new_window);

                //reset flags
                window_mode = false;
                window_point_1_collected = false;
                app->update_message(get_draw_state_vars()->default_message);
                app->refresh_drawing();
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

            if (get_draw_state_vars()->pic_on_screen == e_pic_type::ANALYTICAL_PLACEMENT) {
                // No selection in analytical placement mode yet
                return;
            }

            /* Control + mouse click to select multiple nets. */
            if (!(event->modifiers() & Qt::ControlModifier))
                deselect_all();

            //Check if we hit an rr node
            // Note that we check this before checking for a block, since pins and routing may appear overtop of a multi-width/height block
            if (highlight_rr_nodes(x, y)) {
                return; //Selected an rr node
            }

            highlight_blocks(x, y);
        }
    }
}

void act_on_mouse_move(ezgl::application* app, QMouseEvent* /* event */, double x, double y) {
    // DEF-005 symptom guard — defends against ezgl mouse-move dispatch arriving
    // outside an active VPR draw lifecycle (stale callback after teardown,
    // pre-init dispatch from offscreen platform, etc.). Root cause is tracked
    // separately under DEF-005 (ezgl mouse-dispatch); this guard keeps the VPR
    // side crash-free while that investigation continues. See
    // doc/src/dev/vpr_gui_test_implementation_plan.rst §7.6.
    if (app == nullptr) return;
    t_draw_state* draw_state = get_draw_state_vars();
    if (draw_state == nullptr) return;

    // user has clicked the window button, in window mode
    if (window_point_1_collected) {
        // Update the preview cursor position and let the regular draw flow
        // paint the dashed-rectangle preview (see draw_main_canvas). Drawing
        // it as part of the main draw works uniformly across all backends —
        // the immediate-renderer "animation" pattern used by GTK does not
        // translate cleanly to RHI's GPU-composited overlay.
        window_preview_cursor = {x, y};
        app->refresh_drawing();
        return;
    }

    // user has not clicked the window button, in regular mode
    if (!draw_state->show_rr) {
        return;
    }

    RRNodeId hit_node = draw_check_rr_node_hit(x, y);

    if (hit_node) {
        //Update message
        const DeviceContext& device_ctx = g_vpr_ctx.device();
        std::string info = describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, hit_node, draw_state->is_flat);
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

ezgl::point2d atom_pin_draw_coord(AtomPinId pin) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    AtomBlockId blk = atom_ctx.netlist().pin_block(pin);
    ClusterBlockId clb_index = atom_ctx.lookup().atom_clb(blk);
    const t_pb_graph_node* pg_gnode = atom_ctx.lookup().atom_pb_bimap().atom_pb_graph_node(blk);

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
std::vector<RRNodeId> trace_routed_connection_rr_nodes(
    ClusterNetId net_id,
    int driver_pin,
    int sink_pin) {
    const RoutingContext& route_ctx = g_vpr_ctx.routing();

    VTR_ASSERT(route_ctx.route_trees[net_id]);
    const RouteTree& tree = route_ctx.route_trees[net_id].value();

    VTR_ASSERT(tree.root().inode == route_ctx.net_rr_terminals[net_id][driver_pin]);

    RRNodeId sink_rr_node = route_ctx.net_rr_terminals[ParentNetId(size_t(net_id))][sink_pin];

    std::vector<RRNodeId> rr_nodes_on_path;

    //Collect the rr nodes
    trace_routed_connection_rr_nodes_recurr(tree.root(),
                                            sink_rr_node,
                                            rr_nodes_on_path);

    //Traced from sink to source, but we want to draw from source to sink
    std::reverse(rr_nodes_on_path.begin(), rr_nodes_on_path.end());

    return rr_nodes_on_path;
}

//Helper function for trace_routed_connection_rr_nodes
//Adds the rr nodes linking rt_node to sink_rr_node to rr_nodes_on_path
//Returns true if rt_node is on the path
bool trace_routed_connection_rr_nodes_recurr(const RouteTreeNode& rt_node,
                                             RRNodeId sink_rr_node,
                                             std::vector<RRNodeId>& rr_nodes_on_path) {
    //DFS from the current rt_node to the sink_rr_node, when the sink is found trace back the used rr nodes

    if (rt_node.inode == RRNodeId(sink_rr_node)) {
        rr_nodes_on_path.push_back(sink_rr_node);
        return true;
    }

    for (const RouteTreeNode& child_rt_node : rt_node.child_nodes()) {
        bool on_path_to_sink = trace_routed_connection_rr_nodes_recurr(
            child_rt_node, sink_rr_node, rr_nodes_on_path);
        if (on_path_to_sink) {
            rr_nodes_on_path.push_back(rt_node.inode);
            return true;
        }
    }

    return false; //Not on path to sink
}

//Find the edge between two rr nodes
t_edge_size find_edge(RRNodeId prev_inode, RRNodeId inode) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;
    for (t_edge_size iedge = 0;
         iedge < rr_graph.num_edges(prev_inode); ++iedge) {
        if (rr_graph.edge_sink_node(prev_inode, iedge) == inode) {
            return iedge;
        }
    }
    VTR_ASSERT(false);
    return UNDEFINED;
}

ezgl::color to_ezgl_color(vtr::Color<float> color) {
    return ezgl::color(color.r * 255, color.g * 255, color.b * 255);
}

static float get_router_expansion_cost(const t_rr_node_route_inf& node_inf,
                                       e_draw_router_expansion_cost draw_router_expansion_cost) {
    if (draw_router_expansion_cost == DRAW_ROUTER_EXPANSION_COST_TOTAL
        || draw_router_expansion_cost == DRAW_ROUTER_EXPANSION_COST_TOTAL_WITH_EDGES) {
        return node_inf.path_cost;
    } else if (draw_router_expansion_cost == DRAW_ROUTER_EXPANSION_COST_KNOWN
               || draw_router_expansion_cost == DRAW_ROUTER_EXPANSION_COST_KNOWN_WITH_EDGES) {
        return node_inf.backward_path_cost;
    } else if (draw_router_expansion_cost == DRAW_ROUTER_EXPANSION_COST_EXPECTED
               || draw_router_expansion_cost == DRAW_ROUTER_EXPANSION_COST_EXPECTED_WITH_EDGES) {
        return node_inf.path_cost - node_inf.backward_path_cost;
    }

    VPR_THROW(VPR_ERROR_DRAW, "Invalid Router RR cost drawing type");
}

static void draw_router_expansion_costs(ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();
    if (draw_state->show_router_expansion_cost == DRAW_NO_ROUTER_EXPANSION_COST) {
        return;
    }

    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RoutingContext& route_ctx = g_vpr_ctx.routing();

    vtr::vector<RRNodeId, float> rr_costs(device_ctx.rr_graph.num_nodes());

    for (RRNodeId inode : device_ctx.rr_graph.nodes()) {
        float cost = get_router_expansion_cost(route_ctx.rr_node_route_inf[inode],
                                               draw_state->show_router_expansion_cost);
        rr_costs[inode] = cost;
    }

    bool all_nan = true;
    for (RRNodeId inode : device_ctx.rr_graph.nodes()) {
        if (std::isinf(rr_costs[inode])) {
            rr_costs[inode] = NAN;
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
        application->update_message(
            "Routing Expected Total Cost (known + estimate)");
    } else if (draw_state->show_router_expansion_cost
                   == DRAW_ROUTER_EXPANSION_COST_KNOWN
               || draw_state->show_router_expansion_cost
                      == DRAW_ROUTER_EXPANSION_COST_KNOWN_WITH_EDGES) {
        application->update_message("Routing Known Cost (from source to node)");
    } else if (draw_state->show_router_expansion_cost
                   == DRAW_ROUTER_EXPANSION_COST_EXPECTED
               || draw_state->show_router_expansion_cost
                      == DRAW_ROUTER_EXPANSION_COST_EXPECTED_WITH_EDGES) {
        application->update_message(
            "Routing Expected Cost (from node to target)");
    } else {
        VPR_THROW(VPR_ERROR_DRAW, "Invalid Router RR cost drawing type");
    }
}

/**
 * @brief Highlights the block that was clicked on, looking from the top layer downwards for 3D devices (chooses the block on the top visible layer for overlapping blocks)
 *        It highlights the block green, as well as its fanin and fanout to blue and red respectively by updating the draw_state variables responsible for holding the
 *        color of the block as well as its fanout and fanin.
 * @param x
 * @param y
 */
static void highlight_blocks(double x, double y) {
    t_draw_coords* draw_coords = get_draw_coords_vars();
    t_draw_state* draw_state = get_draw_state_vars();

    char msg[vtr::bufsize];
    ClusterBlockId clb_index = get_cluster_block_id_from_xy_loc(x, y);
    if (clb_index == ClusterBlockId::INVALID()) {
        return; /* Nothing was found on any layer*/
    }

    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const auto& block_locs = draw_state->get_graphics_blk_loc_registry_ref().block_locs();

    VTR_ASSERT(clb_index != ClusterBlockId::INVALID());

    ezgl::rectangle clb_bbox = draw_coords->get_absolute_clb_bbox(clb_index, cluster_ctx.clb_nlist.block_type(clb_index));
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
                block_locs[clb_index].loc.x,
                block_locs[clb_index].loc.y);
    }

    //If manual moves is activated, then user can select block from the grid.
    if (draw_state->manual_moves_state.manual_move_enabled) {
        draw_state->manual_moves_state.user_highlighted_block = true;
        if (!draw_state->manual_moves_state.manual_move_window_is_open) {
            draw_manual_moves_window(std::to_string(size_t(clb_index)));
        }
    }

    application->update_message(msg);
    application->refresh_drawing();
}

ClusterBlockId get_cluster_block_id_from_xy_loc(double x, double y) {
    t_draw_coords* draw_coords = get_draw_coords_vars();
    t_draw_state* draw_state = get_draw_state_vars();
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const auto& grid_blocks = draw_state->get_graphics_blk_loc_registry_ref().grid_blocks();

    /// determine block ///
    ezgl::rectangle clb_bbox;

    auto clb_index = ClusterBlockId::INVALID();

    //iterate over grid z (layers) first. Start search of the block at the top layer to prioritize highlighting of blocks at higher levels during overlapping of layers.
    for (int layer_num = device_ctx.grid.get_num_layers() - 1; layer_num >= 0; layer_num--) {
        if (!draw_state->draw_layer_display[layer_num].visible) {
            continue; /* Don't check for blocks on non-visible layers*/
        }
        // iterate over grid x
        for (int i = 0; i < (int)device_ctx.grid.width(); ++i) {
            if (draw_coords->tile_x[i] > x) {
                break; // we've gone too far in the x direction
            }
            // iterate over grid y
            for (int j = 0; j < (int)device_ctx.grid.height(); ++j) {
                if (draw_coords->tile_y[j] > y) {
                    break; // we've gone too far in the y direction
                }
                // iterate over sub_blocks
                const auto& type = device_ctx.grid.get_physical_type({i, j, layer_num});
                for (int k = 0; k < type->capacity; ++k) {
                    clb_index = grid_blocks.block_at_location({i, j, k, layer_num});
                    if (clb_index) {
                        clb_bbox = draw_coords->get_absolute_clb_bbox(clb_index,
                                                                      cluster_ctx.clb_nlist.block_type(clb_index));
                        if (clb_bbox.contains({x, y})) {
                            return clb_index; // we've found the clb
                        } else {
                            clb_index = ClusterBlockId::INVALID();
                        }
                    }
                }
            }
        }
    }
    // Searched all layers and found no clb at specified location, returning clb_index = ClusterBlockId::INVALID().
    return clb_index;
}

static void setup_default_ezgl_callbacks(ezgl::application* app) {
    // Connect press_proceed function to the Proceed button
    QPushButton* proceed_button = app->find_push_button("ProceedButton");
    QObject::connect(proceed_button, &QPushButton::clicked, [app]() {
        press_proceed(/*unused*/ nullptr, app);
    });

    // Connect press_zoom_fit function to the Zoom-fit button
    QPushButton* zoom_fit_button = app->find_push_button("ZoomFitButton");
    QObject::connect(zoom_fit_button, &QPushButton::clicked, [app]() {
        press_zoom_fit(/*unused*/ nullptr, app);
    });

    // Connect Pause button
    QPushButton* pause_button = app->find_push_button("PauseButton");
    QObject::connect(pause_button, &QPushButton::clicked, []() {
        set_force_pause();
    });

    // Connect Block Outline checkbox
    QCheckBox* block_outline = app->find_check_box("blockOutline");
    QObject::connect(block_outline, &QCheckBox::toggled, [](bool checked) {
        set_block_outline(checked);
    });

    // Connect Block Text checkbox
    QCheckBox* block_text = app->find_check_box("blockText");
    QObject::connect(block_text, &QCheckBox::toggled, [](bool checked) {
        set_block_text(checked);
    });

    // Connect Clip Routing Util checkbox
    QCheckBox* clip_routing = app->find_check_box("clipRoutingUtil");
    QObject::connect(clip_routing, &QCheckBox::toggled, [](bool checked) {
        clip_routing_util(checked);
    });

    // Connect Debug Button
    QPushButton* debugger = app->find_push_button("debugButton");
    QObject::connect(debugger, &QPushButton::clicked, [app]() {
        draw_debug_window();
    });

    // Connect Draw Partitions Checkbox
    QCheckBox* draw_partitions = app->find_check_box("drawPartitions");
    QObject::connect(draw_partitions, &QCheckBox::toggled, [](bool checked) {
        set_draw_partitions(checked);
    });
}

// Callback function for Block Outline checkbox
static void set_block_outline(bool checked) {
    t_draw_state* draw_state = get_draw_state_vars();

    draw_state->draw_block_outlines = checked;

    //redraw
    application->update_message(draw_state->default_message);
    application->refresh_drawing();
}

// Callback function for Block Text checkbox
static void set_block_text(bool checked) {
    t_draw_state* draw_state = get_draw_state_vars();

    draw_state->draw_block_text = checked;

    //redraw
    application->update_message(draw_state->default_message);
    application->refresh_drawing();
}

// Callback function for Clip Routing Util checkbox
static void clip_routing_util(bool checked) {
    t_draw_state* draw_state = get_draw_state_vars();

    draw_state->clip_routing_util = checked;

    //redraw
    application->update_message(draw_state->default_message);
    application->refresh_drawing();
}

// Callback function for Draw Partitions checkbox
static void set_draw_partitions(bool checked) {
    t_draw_state* draw_state = get_draw_state_vars();

    if (checked) {
        QWidget* window = application->find_widget(application->get_main_window_id().c_str());

        QDialog* dialog = new QDialog(window);
        dialog->setWindowTitle("Floorplanning Legend");
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowFlag(Qt::Tool, true); // float above the main window
        dialog->resize(400, 500);

        QVBoxLayout* layout = new QVBoxLayout(dialog);

        QTreeWidget* tree = new QTreeWidget(dialog);
        setup_floorplanning_legend(tree);
        layout->addWidget(tree);

        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
        QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::close);
        layout->addWidget(buttons);

        QObject::connect(tree, &QTreeWidget::itemSelectionChanged, tree, [tree]() {
            highlight_selected_partition(tree);
        });

        // show() alone is not always enough: some window managers refuse to
        // give a freshly-created top-level dialog focus, leaving it stacked
        // behind the main window. raise() + activateWindow() force it on top.
        dialog->show();
        dialog->raise();
        dialog->activateWindow();
        draw_state->draw_partitions = true;
    } else {
        draw_state->draw_partitions = false;
    }

    application->update_message(draw_state->default_message);
    application->refresh_drawing();
}

static void set_force_pause() {
    t_draw_state* draw_state = get_draw_state_vars();

    draw_state->forced_pause = true;
}

// The enums below are the integer argument values accepted by the
// --graphics_commands graphics-script commands (e.g. `set_nets <int>`,
// `set_cpd <int>`, `set_congestion <int>`).

// Argument values for the `set_nets` graphics-script command.
enum e_set_nets_arg : int {
    SET_NETS_OFF = 0,
    SET_NETS_FLYLINES = 1,
    SET_NETS_ROUTED = 2,
};

// Argument bits for the `set_cpd` graphics-script command (a bitmask).
enum e_set_cpd_arg_bits : int {
    SET_CPD_OFF = 0,
    SET_CPD_FLYLINES = 1 << 0,
    SET_CPD_DELAYS = 1 << 1,
    SET_CPD_ROUTING = 1 << 2,
};

// Argument values for the `set_congestion` graphics-script command.
enum e_set_congestion_arg : int {
    SET_CONGESTION_OFF = 0,
    SET_CONGESTION_NODES = 1,
    SET_CONGESTION_NODES_AND_NETS = 2,
};

// Parse a `wait_for_stage` argument of the form <stage>_<initial|done>
// (e.g. "routing_initial", "placement_done") into the target stage `want`
// and whether it waits for stage completion (`wait_for_done`). Errors out on
// a malformed suffix or an unsupported stage name.
static void parse_wait_for_stage_arg(const std::string& arg,
                                     e_pic_type& want,
                                     bool& wait_for_done) {
    const std::string done_suffix = "_done";
    const std::string init_suffix = "_initial";
    std::string stage_name;
    if (arg.size() > done_suffix.size()
        && arg.compare(arg.size() - done_suffix.size(), done_suffix.size(), done_suffix) == 0) {
        wait_for_done = true;
        stage_name = arg.substr(0, arg.size() - done_suffix.size());
    } else if (arg.size() > init_suffix.size()
               && arg.compare(arg.size() - init_suffix.size(), init_suffix.size(), init_suffix) == 0) {
        wait_for_done = false;
        stage_name = arg.substr(0, arg.size() - init_suffix.size());
    } else {
        VPR_ERROR(VPR_ERROR_DRAW,
                  vtr::string_fmt("Unknown wait_for_stage argument '%s' "
                                  "(use <stage>_initial or <stage>_done)",
                                  arg.c_str())
                      .c_str());
    }

    if (stage_name == "placement") {
        want = e_pic_type::PLACEMENT;
    } else if (stage_name == "routing") {
        want = e_pic_type::ROUTING;
    } else {
        VPR_ERROR(VPR_ERROR_DRAW,
                  vtr::string_fmt("Unknown or unsupported stage '%s' in "
                                  "wait_for_stage (use placement|routing)",
                                  stage_name.c_str())
                      .c_str());
    }
}

static void run_graphics_commands(const std::string& commands) {
    // A very simple command interpreter for scripting graphics.
    //
    // The parsed command list and the script cursor live on draw_state
    // (parsed_graphics_cmds / graphics_cmd_index) so that `wait_for_stage`
    // barriers can split a script across multiple update_screen() invocations
    // (e.g. half at placement, half at routing). On each call, processing
    // resumes at the cursor and stops either at a wait-barrier whose stage
    // hasn't been reached yet, or at end-of-script.
    t_draw_state* draw_state = get_draw_state_vars();

    // Parse once: `graphics_commands` is set at init and never mutated, so
    // the empty-vector check doubles as a "first call" trigger.
    if (draw_state->parsed_graphics_cmds.empty()) {
        for (const std::string& raw_cmd : vtr::StringToken(commands).split(";")) {
            draw_state->parsed_graphics_cmds.push_back(vtr::StringToken(raw_cmd).split(" \t\n"));
        }
    }

    t_draw_state backup_draw_state = *draw_state;

    while (draw_state->graphics_cmd_index < draw_state->parsed_graphics_cmds.size()) {
        auto& cmd = draw_state->parsed_graphics_cmds[draw_state->graphics_cmd_index];
        VTR_ASSERT_MSG(cmd.size() > 0, "Expect non-empty graphics commands");

        for (auto& item : cmd) {
            VTR_LOG("%s ", item.c_str());
        }
        VTR_LOG("\n");

        if (cmd[0] == "wait_for_stage") {
            // Argument form: <stage>_<initial|done>, e.g. routing_initial
            // or placement_done. `_initial` resumes on the first
            // update_screen() at that stage; `_done` resumes only after the
            // stage has been marked complete by notify_stage_complete()
            // (i.e. on the post-stage settled checkpoint).
            VTR_ASSERT_MSG(cmd.size() == 2,
                           "Expect <stage>_initial or <stage>_done after 'wait_for_stage' "
                           "(stage = placement|routing)");
            e_pic_type want = e_pic_type::NO_PICTURE;
            bool wait_for_done = false;
            parse_wait_for_stage_arg(cmd[1], want, wait_for_done);

            const auto& flag_set = wait_for_done ? completed_stages : initial_stages;
            if (draw_state->pic_on_screen != want
                || flag_set.find(want) == flag_set.end()) {
                // Revert any draw-state mutations made by commands earlier in
                // this script run, but keep the script bookkeeping (cursor +
                // parse cache) so the next update_screen() resumes here.
                size_t saved_index = draw_state->graphics_cmd_index;
                *draw_state = backup_draw_state;
                draw_state->graphics_cmd_index = saved_index;
                return;
            }
            ++draw_state->graphics_cmd_index;
            continue;
        }

        if (cmd[0] == "save_graphics") {
            VTR_ASSERT_MSG(cmd.size() == 2,
                           "Expect filename after 'save_graphics'");

            auto name_ext = vtr::split_ext(cmd[1]);

            //Replace {i}  with the sequence number
            std::string name = vtr::replace_all(name_ext[0], "{i}",
                                                std::to_string(draw_state->sequence_number));

            save_graphics(/*extension=*/name_ext[1], /*file_name=*/name);
            VTR_LOG("Saving to %s\n", std::string(name + name_ext[1]).c_str());

        } else if (cmd[0] == "set_macros") {
            VTR_ASSERT_MSG(cmd.size() == 2,
                           "Expect net draw state after 'set_macro'");
            draw_state->show_placement_macros = (e_draw_placement_macros)vtr::atoi(cmd[1]);
            VTR_LOG("%d\n", (int)draw_state->show_placement_macros);
        } else if (cmd[0] == "set_nets") {
            // 0 = off, 1 = flylines, 2 = routed nets.
            VTR_ASSERT_MSG(cmd.size() == 2,
                           "Expect net draw state (0=off, 1=flylines, 2=routed) after 'set_nets'");
            int state = vtr::atoi(cmd[1]);
            VTR_ASSERT_MSG(state >= SET_NETS_OFF && state <= SET_NETS_ROUTED,
                           "set_nets expects a value in 0..2");
            if (state == SET_NETS_OFF) {
                draw_state->show_nets = false;
                draw_state->draw_inter_cluster_nets = false;
                draw_state->draw_intra_cluster_nets = false;
                draw_state->highlight_fan_in_fan_out = false;
            } else {
                draw_state->show_nets = true;
                draw_state->draw_inter_cluster_nets = true;
                draw_state->draw_intra_cluster_nets = true;
                draw_state->highlight_fan_in_fan_out = true;
                draw_state->draw_nets = (state == SET_NETS_ROUTED) ? DRAW_ROUTED_NETS : DRAW_FLYLINES;
            }
            VTR_LOG("%d\n", state);
        } else if (cmd[0] == "set_cpd") {
            // Bitmask: 0=off, bit0(1)=flylines, bit1(2)=delay labels,
            // bit2(4)=routed-wire highlight (only meaningful at routing stage;
            // gate with `wait_for_stage routing_done`).
            // Useful values: 1=flylines, 3=flylines+delays,
            //                4=routing only,
            //                5=flylines+routing, 7=flylines+delays+routing.
            // Degenerate (no-op): 2 and 6 — delay labels need flylines to
            // anchor to, so the delay bit alone draws nothing.
            VTR_ASSERT_MSG(cmd.size() == 2,
                           "Expect crit-path draw state (0=off, bitmask 1|2|4 = flylines|delays|routing) after 'set_cpd'");
            int state = vtr::atoi(cmd[1]);
            VTR_ASSERT_MSG(state >= SET_CPD_OFF && state <= (SET_CPD_FLYLINES | SET_CPD_DELAYS | SET_CPD_ROUTING),
                           "set_cpd expects a value in 0..7");
            if (state == SET_CPD_OFF) {
                draw_state->show_crit_path = false;
                draw_state->show_crit_path_flylines = false;
                draw_state->show_crit_path_delays = false;
                draw_state->show_crit_path_routing = false;
            } else {
                draw_state->show_crit_path = true;
                draw_state->show_crit_path_flylines = (state & SET_CPD_FLYLINES) != 0;
                draw_state->show_crit_path_delays = (state & SET_CPD_DELAYS) != 0;
                draw_state->show_crit_path_routing = (state & SET_CPD_ROUTING) != 0;
            }
            VTR_LOG("%d\n", state);
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
            // 0 = off, 1 = congested nodes, 2 = congested nodes + nets.
            VTR_ASSERT_MSG(cmd.size() == 2,
                           "Expect congestion draw state (0=off, 1=congested, 2=congested+nets) after 'set_congestion'");
            int state = vtr::atoi(cmd[1]);
            VTR_ASSERT_MSG(state >= SET_CONGESTION_OFF && state <= SET_CONGESTION_NODES_AND_NETS,
                           "set_congestion expects a value in 0..2");
            draw_state->show_congestion = (e_draw_congestion)state;
            VTR_LOG("%d\n", state);
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
            // Deferred exit: record the request and stop processing. The
            // current render checkpoint finishes (refresh/flush/save_graphics
            // below in update_screen) and then the flag is honored.
            pending_graphics_exit = true;
            pending_graphics_exit_code = vtr::atoi(cmd[1]);
            VTR_LOG("Graphics-command 'exit %d' deferred until next render checkpoint.\n",
                    pending_graphics_exit_code);
            ++draw_state->graphics_cmd_index;
            break;
        } else {
            VPR_ERROR(VPR_ERROR_DRAW,
                      vtr::string_fmt("Unrecognized graphics command '%s'",
                                      cmd[0].c_str())
                          .c_str());
        }
        ++draw_state->graphics_cmd_index;
    }

    // Restore user-controllable draw state, but keep the script cursor at
    // end-of-script so subsequent update_screen() calls are no-ops rather
    // than re-running the whole script.
    size_t saved_index = draw_state->graphics_cmd_index;
    *draw_state = backup_draw_state;
    draw_state->graphics_cmd_index = saved_index;

    //Advance the sequence number
    ++draw_state->sequence_number;
}

ezgl::point2d tnode_draw_coord(tatum::NodeId node) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    AtomPinId pin = atom_ctx.lookup().tnode_atom_pin(node);
    return atom_pin_draw_coord(pin);
}

/* This routine highlights the blocks affected in the latest move      *
 * It highlights the old and new locations of the moved blocks         *
 * It also highlights the moved block input and output terminals       *
 * Currently, it is used in placer debugger when breakpoint is reached */
void highlight_moved_block_and_its_terminals(
    const t_pl_blocks_to_be_moved& blocks_affected) {
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();

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

bool highlight_loc_with_specific_color(t_pl_loc curr_loc, ezgl::color& loc_color) {
    t_draw_state* draw_state = get_draw_state_vars();

    //search for the current location in the vector of colored locations
    auto it = std::find_if(draw_state->colored_locations.begin(),
                           draw_state->colored_locations.end(),
                           [&curr_loc](const std::pair<t_pl_loc, ezgl::color>& vec_element) {
                               return (vec_element.first.x == curr_loc.x
                                       && vec_element.first.y == curr_loc.y && vec_element.first.layer == curr_loc.layer);
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

//Lightens a color's luminance [0, 1] by an absolute 'amount'
ezgl::color lighten_color(ezgl::color color, float amount) {
    constexpr double MAX_LUMINANCE = 0.95; //Clip luminance so it doesn't go full white
    auto hsl = color2hsl(color);

    hsl.l = std::max(0., std::min(MAX_LUMINANCE, hsl.l + amount));

    return hsl2color(hsl);
}
/**
 * @brief Returns the max fanout
 *
 * @return size_t
 */
size_t get_max_fanout() {
    //find maximum fanout
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    auto& clb_nlist = cluster_ctx.clb_nlist;
    size_t max_fanout = 0;
    for (ClusterNetId net_id : clb_nlist.nets())
        max_fanout = std::max(max_fanout, clb_nlist.net_sinks(net_id).size());

    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    auto& atom_nlist = atom_ctx.netlist();
    size_t max_fanout2 = 0;
    for (AtomNetId net_id : atom_nlist.nets())
        max_fanout2 = std::max(max_fanout2, atom_nlist.net_sinks(net_id).size());

    size_t max = std::max(max_fanout2, max_fanout);
    return max;
}

bool rgb_is_same(ezgl::color color1, ezgl::color color2) {
    color1.alpha = 255;
    color2.alpha = 255;
    return (color1 == color2);
}
t_draw_layer_display get_element_visibility_and_transparency(int src_layer, int sink_layer) {
    t_draw_layer_display element_visibility;
    t_draw_state* draw_state = get_draw_state_vars();

    element_visibility.visible = true;
    bool cross_layer_enabled = draw_state->cross_layer_display.visible;

    //To only show elements (net flylines,noc links,etc...) that are connected to currently active layers on the screen
    if (!draw_state->draw_layer_display[sink_layer].visible || !draw_state->draw_layer_display[src_layer].visible || (!cross_layer_enabled && src_layer != sink_layer)) {
        element_visibility.visible = false; /* Don't Draw */
    }

    if (src_layer != sink_layer) {
        //assign transparency from cross layer option if connection is between different layers
        element_visibility.alpha = draw_state->cross_layer_display.alpha;
    } else {
        //otherwise assign transparency of current layer
        element_visibility.alpha = draw_state->draw_layer_display[src_layer].alpha;
    }

    return element_visibility;
}

#endif /* NO_GRAPHICS */
