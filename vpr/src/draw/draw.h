/**
 * @file draw.h
 *
 * The main drawing file. Contains the setup for ezgl application, ui setup, and graphis functions
 *
 * This is VPR's main graphics application program. The program interacts with ezgl/graphics.hpp,
 * which provides an API for displaying graphics on both X11 and Win32. The most important
 * subroutine in this file is draw_main_canvas(), which is a callback function that will be called
 * whenever the screen needs to be updated. Then, draw_main_canvas() will decide what
 * drawing subroutines to call depending on whether PLACEMENT or ROUTING is shown on screen.
 * The initial_setup_X() functions link the menu button signals to the corresponding drawing functions.
 * As a note, looks into draw_global.c for understanding the data structures associated with drawing->
 *
 * Contains all functions that didn't fit in any other draw_*.cpp file.
 *
 * Authors: Vaughn Betz, Long Yu (Mike) Wang, Dingyu (Tina) Yang, Sebastian Lievano
 * Last updated: August 2022
 */

#ifndef DRAW_H
#define DRAW_H

#include "rr_graph_fwd.h"
#include "timing_info.h"
#include "physical_types.h"

#ifndef NO_GRAPHICS

#    include "draw_global.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"
#    include "draw_color.h"
#    include "search_bar.h"
#    include "draw_debug.h"
#    include "manual_moves.h"
#    include "vtr_ndoffsetmatrix.h"

extern ezgl::application::settings settings;
extern ezgl::application application;

#endif /* NO_GRAPHICS */

void update_screen(ScreenUpdatePriority priority, const char* msg, enum pic_type pic_on_screen_val, std::shared_ptr<SetupTimingInfo> timing_info);

//Initializes the drawing locations.
//FIXME: Currently broken if no rr-graph is loaded
void init_draw_coords(float clb_width);

/* Sets the static show_graphics and gr_automode variables to the    *
 * desired values.  They control if graphics are enabled and, if so, *
 * how often the user is prompted for input.                         */
void init_graphics_state(bool show_graphics_val,
                         int gr_automode_val,
                         enum e_route_type route_type,
                         bool save_graphics,
                         std::string graphics_commands,
                         bool is_flat);

/* Allocates the structures needed to draw the placement and routing.*/
void alloc_draw_structs(const t_arch* arch);

/* Free everything allocated by alloc_draw_structs. Called after close_graphics()
 * in vpr_api.c. */
void free_draw_structs();

#ifndef NO_GRAPHICS
const ezgl::color SELECTED_COLOR = ezgl::GREEN;
const ezgl::color DRIVES_IT_COLOR = ezgl::RED;
const ezgl::color DRIVEN_BY_IT_COLOR = ezgl::LIGHT_MEDIUM_BLUE;

const float WIRE_DRAWING_WIDTH = 0.5;

/* Find the edge between two rr nodes */
t_edge_size find_edge(RRNodeId prev_inode, RRNodeId inode);

/* Returns the track number of this routing resource node inode. */
int get_track_num(int inode, const vtr::OffsetMatrix<int>& chanx_track, const vtr::OffsetMatrix<int>& chany_track);

//Returns the drawing coordinates of the specified pin
ezgl::point2d atom_pin_draw_coord(AtomPinId pin);

//Returns the drawing coordinates of the specified tnode
ezgl::point2d tnode_draw_coord(tatum::NodeId node);

/* Converts a vtr Color to a ezgl Color. */
ezgl::color to_ezgl_color(vtr::Color<float> color);

/* This helper function determines whether a net has been highlighted. The highlighting
 * could be caused by the user clicking on a routing resource, toggled, or
 * fan-in/fan-out of a highlighted node. */
bool draw_if_net_highlighted(ClusterNetId inet);
std::vector<RRNodeId> trace_routed_connection_rr_nodes(
    ClusterNetId net_id,
    int driver_pin,
    int sink_pin);

/* Helper function for trace_routed_connection_rr_nodes
 * Adds the rr nodes linking rt_node to sink_rr_node to rr_nodes_on_path
 * Returns true if rt_node is on the path. */
bool trace_routed_connection_rr_nodes_recurr(const RouteTreeNode& rt_node,
                                             RRNodeId sink_rr_node,
                                             std::vector<RRNodeId>& rr_nodes_on_path);

/* This routine highlights the blocks affected in the latest move      *
 * It highlights the old and new locations of the moved blocks         *
 * It also highlights the moved block input and output terminals       *
 * Currently, it is used in placer debugger when breakpoint is reached */
void highlight_moved_block_and_its_terminals(const t_pl_blocks_to_be_moved&);

// pass in an (x,y,subtile) location and the color in which it should be drawn.
// This overrides the color of any block placed in that location, and also applies if the location is empty.
void set_draw_loc_color(t_pl_loc, ezgl::color);

// clear the colored_locations vector
void clear_colored_locations();

/**
 * @brief If the input loc is marked in colored_locations vector, the function will return true and the corresponding color is sent back in loc_color
 * otherwise, the function returns false (the location isn't among the highlighted locations)
 *
 * @param curr_loc  The current location that is being checked for whether it must be highlighted or not
 * @param loc_color The corresponding color that is to be used to highlight the block
 *
 * @return    Returns true or false depending on whether the block at the specified (x,y,layer) location needs to be highlighted by a specific color.
 *            The corresponding color is returned by reference.
 */
bool highlight_loc_with_specific_color(t_pl_loc curr_loc, ezgl::color& loc_color);

/* Because the list of possible block type colours is finite, we wrap around possible colours if there are more
 * block types than colour choices. This ensures we support any number of types, although the colours may repeat.*/
ezgl::color get_block_type_color(t_physical_tile_type_ptr type);

/* Lightens a color's luminance [0, 1] by an aboslute 'amount' */
ezgl::color lighten_color(ezgl::color color, float amount);

void toggle_window_mode(GtkWidget* /*widget*/, ezgl::application* /*app*/);

size_t get_max_fanout();

/**
 * @brief Takes in two colors and compares rgb values, ignoring transparency/alpha
 * Sets both transparencies to opaque and then compares the colors.
 */
bool rgb_is_same(ezgl::color color1, ezgl::color color2);

/**
 * @brief Takes in the layer number of the src and sink of an element(flyline, rr_node connections, etc...) and returns a t_draw_layer_display object holding the
 *        information of the visibility of the element as well as the transparency based on the setting set by the user from the view menu in the UI.
 *
 * @param src_layer
 * @param sink_layer
 * @return  Returns whether the element should be drawn (true or false) and the transparency factor (0 - transparent ,255 - opaque) as a t_draw_layer_display object
 */
t_draw_layer_display get_element_visibility_and_transparency(int src_layer, int sink_layer);

#endif /* NO_GRAPHICS */

#endif /* DRAW_H */
