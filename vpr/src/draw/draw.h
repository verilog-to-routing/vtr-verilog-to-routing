#ifndef DRAW_H
#define DRAW_H

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
void init_graphics_state(bool show_graphics_val, int gr_automode_val, enum e_route_type route_type, bool save_graphics, std::string graphics_commands);

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
t_edge_size find_edge(int prev_inode, int inode);

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
std::vector<int> trace_routed_connection_rr_nodes(
    const ClusterNetId net_id,
    const int driver_pin,
    const int sink_pin);

/* Helper function for trace_routed_connection_rr_nodes
 * Adds the rr nodes linking rt_node to sink_rr_node to rr_nodes_on_path
 * Returns true if rt_node is on the path. */
bool trace_routed_connection_rr_nodes_recurr(const t_rt_node* rt_node,
                                             int sink_rr_node,
                                             std::vector<int>& rr_nodes_on_path);

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

// This routine takes in a (x,y) location.
// If the input loc is marked in colored_locations vector, the function will return true and the correspnding color is sent back in loc_color
// otherwise, the function returns false (the location isn't among the highlighted locations)
bool highlight_loc_with_specific_color(int x, int y, ezgl::color& loc_color);

/* Because the list of possible block type colours is finite, we wrap around possible colours if there are more
 * block types than colour choices. This ensures we support any number of types, although the colours may repeat.*/
ezgl::color get_block_type_color(t_physical_tile_type_ptr type);

/* Lightens a color's luminance [0, 1] by an aboslute 'amount' */
ezgl::color lighten_color(ezgl::color color, float amount);

#endif /* NO_GRAPHICS */

#endif /* DRAW_H */
