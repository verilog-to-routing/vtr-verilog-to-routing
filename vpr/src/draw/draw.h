#ifndef DRAW_H
#define DRAW_H

#include "easygl_constants.h"
#include "graphics.h"
#include "draw_global.h"

void update_screen(ScreenUpdatePriority priority, const char* msg, enum pic_type pic_on_screen_val, std::shared_ptr<SetupTimingInfo> timing_info);

void alloc_draw_structs(const t_arch* arch);

//Initializes the drawing locations.
//FIXME: Currently broken if no rr-graph is loaded
void init_draw_coords(float clb_width);

void init_graphics_state(bool show_graphics_val, int gr_automode_val, enum e_route_type route_type);

void free_draw_structs();

void draw_get_rr_pin_coords(int inode, float* xcen, float* ycen);
void draw_get_rr_pin_coords(const t_rr_node* node, float* xcen, float* ycen);

void draw_triangle_along_line(t_point start, t_point end, float relative_position = 1., float arrow_size = DEFAULT_ARROW_SIZE);
void draw_triangle_along_line(t_point loc, t_point start, t_point end, float arrow_size = DEFAULT_ARROW_SIZE);
void draw_triangle_along_line(float xend, float yend, float x1, float x2, float y1, float y2, float arrow_size = DEFAULT_ARROW_SIZE);

const color_types SELECTED_COLOR = GREEN;
const color_types DRIVES_IT_COLOR = RED;
const color_types DRIVEN_BY_IT_COLOR = LIGHTMEDIUMBLUE;

const float WIRE_DRAWING_WIDTH = 0.5;

//Returns the drawing coordinates of the specified pin
t_point atom_pin_draw_coord(AtomPinId pin);

//Returns the drawing coordinates of the specified tnode
t_point tnode_draw_coord(tatum::NodeId node);

void annotate_draw_rr_node_costs(ClusterNetId net, int sink_rr_node);
void clear_draw_rr_annotations();

#endif /* DRAW_H */
