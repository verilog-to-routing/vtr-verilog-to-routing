#ifndef BUTTONS_H
#define BUTTONS_H

#ifndef NO_GRAPHICS

#    include "draw_global.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

void button_for_toggle_nets();
void button_for_toggle_blk_internal();
void button_for_toggle_block_pin_util();
void button_for_toggle_placement_macros();
void button_for_toggle_crit_path();

void button_for_toggle_rr();
void button_for_toggle_congestion();
void button_for_toggle_congestion_cost();
void button_for_toggle_routing_bounding_box();
void button_for_toggle_routing_util();
void button_for_toggle_router_rr_costs();
void delete_button(const char* button_name);
GtkWidget* find_button(const char* button_name);
#endif /* NO_GRAPHICS */

#endif /* BUTTONS_H */
