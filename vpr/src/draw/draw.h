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

void init_graphics_state(bool show_graphics_val, int gr_automode_val, enum e_route_type route_type, bool save_graphics, std::string graphics_commands);

void alloc_draw_structs(const t_arch* arch);
void free_draw_structs();

#ifndef NO_GRAPHICS
const ezgl::color SELECTED_COLOR = ezgl::GREEN;
const ezgl::color DRIVES_IT_COLOR = ezgl::RED;
const ezgl::color DRIVEN_BY_IT_COLOR = ezgl::LIGHT_MEDIUM_BLUE;

const float WIRE_DRAWING_WIDTH = 0.5;

t_edge_size find_edge(int prev_inode, int inode);

int get_track_num(int inode, const vtr::OffsetMatrix<int>& chanx_track, const vtr::OffsetMatrix<int>& chany_track);

//Returns the drawing coordinates of the specified pin
ezgl::point2d atom_pin_draw_coord(AtomPinId pin);

//Returns the drawing coordinates of the specified tnode
ezgl::point2d tnode_draw_coord(tatum::NodeId node);

void annotate_draw_rr_node_costs(ClusterNetId net, int sink_rr_node);
void clear_draw_rr_annotations();

void draw_reset_blk_colors();
void draw_reset_blk_color(ClusterBlockId blk_id);

ezgl::color to_ezgl_color(vtr::Color<float> color);

bool draw_if_net_highlighted(ClusterNetId inet);
std::vector<int> trace_routed_connection_rr_nodes(
    const ClusterNetId net_id,
    const int driver_pin,
    const int sink_pin);
bool trace_routed_connection_rr_nodes_recurr(const t_rt_node* rt_node,
                                             int sink_rr_node,
                                             std::vector<int>& rr_nodes_on_path);
void draw_screen();

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

#endif /* NO_GRAPHICS */

#endif /* DRAW_H */
