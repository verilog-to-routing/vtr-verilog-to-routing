#ifndef DRAW_H
#define DRAW_H

#include "easygl_constants.h"
#include "graphics.h"

void update_screen(int priority, char *msg, enum pic_type pic_on_screen_val,
		bool crit_path_button_enabled, const t_timing_inf &timing_inf);

void alloc_draw_structs(void);

void init_draw_coords(float clb_width);

void init_graphics_state(bool show_graphics_val, int gr_automode_val,
		enum e_route_type route_type);

void free_draw_structs(void);

void draw_get_rr_pin_coords(int inode, int iside, int width_offset, 
								   int height_offset, float *xcen, float *ycen);
void draw_get_rr_pin_coords(t_rr_node* node, int iside, int width_offset,
                                   int height_offset, float *xcen, float *ycen);

void draw_triangle_along_line(float distance_from_end, float x1, float x2, float y1, float y2);
void draw_triangle_along_line(float xend, float yend, float x1 ,float x2, float y1, float y2);
void draw_triangle_along_line(float xend, float yend, float x1 ,float x2, float y1, float y2, float arrow_size);

const color_types SELECTED_COLOR = GREEN;
const color_types DRIVES_IT_COLOR = RED;
const color_types DRIVEN_BY_IT_COLOR = LIGHTMEDIUMBLUE;

namespace crit_path_colors {
	namespace blk {
		const t_color HEAD        (0x99,0xFF,0x66);
		const t_color HEAD_DRIVER (0x55,0x88,0xFF);
		const t_color TAIL        (0xAA,0xCC,0xFF);
	}
	namespace net {
		const t_color HEAD (0x00,0xDD,0x00);
		const t_color TAIL (0x00,0x77,0xBB);
	}
}

const float WIRE_DRAWING_WIDTH = 0.5;

#endif /* DRAW_H */
