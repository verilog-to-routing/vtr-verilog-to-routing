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

extern ezgl::application::settings settings;
extern ezgl::application application;

#endif /* NO_GRAPHICS */

void update_screen(ScreenUpdatePriority priority, const char* msg, enum pic_type pic_on_screen_val, std::shared_ptr<SetupTimingInfo> timing_info);
//Initializes the drawing locations.
//FIXME: Currently broken if no rr-graph is loaded
void init_draw_coords(float clb_width);

void init_graphics_state(bool show_graphics_val, int gr_automode_val, enum e_route_type route_type, bool save_graphics);

void alloc_draw_structs(const t_arch* arch);
void free_draw_structs();

#ifndef NO_GRAPHICS

void draw_get_rr_pin_coords(int inode, float* xcen, float* ycen);
void draw_get_rr_pin_coords(const t_rr_node* node, float* xcen, float* ycen);

void draw_triangle_along_line(ezgl::renderer* g, ezgl::point2d start, ezgl::point2d end, float relative_position = 1., float arrow_size = DEFAULT_ARROW_SIZE);
void draw_triangle_along_line(ezgl::renderer* g, ezgl::point2d loc, ezgl::point2d start, ezgl::point2d end, float arrow_size = DEFAULT_ARROW_SIZE);
void draw_triangle_along_line(ezgl::renderer* g, float xend, float yend, float x1, float x2, float y1, float y2, float arrow_size = DEFAULT_ARROW_SIZE);

const ezgl::color SELECTED_COLOR = ezgl::GREEN;
const ezgl::color DRIVES_IT_COLOR = ezgl::RED;
const ezgl::color DRIVEN_BY_IT_COLOR = ezgl::LIGHT_MEDIUM_BLUE;

const float WIRE_DRAWING_WIDTH = 0.5;

//Returns the drawing coordinates of the specified pin
ezgl::point2d atom_pin_draw_coord(AtomPinId pin);

//Returns the drawing coordinates of the specified tnode
ezgl::point2d tnode_draw_coord(tatum::NodeId node);

void annotate_draw_rr_node_costs(ClusterNetId net, int sink_rr_node);
void clear_draw_rr_annotations();

ezgl::color to_ezgl_color(vtr::Color<float> color);

void draw_screen();

// search bar related functions
ezgl::rectangle draw_get_rr_chan_bbox(int inode);
void draw_highlight_blocks_color(t_logical_block_type_ptr type, ClusterBlockId blk_id);
void highlight_nets(char* message, int hit_node);
void draw_highlight_fan_in_fan_out(const std::set<int>& nodes);
std::set<int> draw_expand_non_configurable_rr_nodes(int hit_node);
void deselect_all();

// toggle functions
void toggle_nets(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_rr(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_congestion(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_routing_congestion_cost(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_routing_bounding_box(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_routing_util(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_crit_path(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_block_pin_util(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_router_rr_costs(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_placement_macros(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);

ezgl::color get_block_type_color(t_physical_tile_type_ptr type);

#endif /* NO_GRAPHICS */

#endif /* DRAW_H */
