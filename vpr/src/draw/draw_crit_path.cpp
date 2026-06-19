#ifndef NO_GRAPHICS

#include "draw_crit_path.h"
#include "draw_basic.h"
#include "draw.h"



DelayLabelDrawer::DelayLabelDrawer(const tatum::TimingPath& path, const int num_timing_edges, ezgl::renderer* g) {
    delay_label_info.resize(num_timing_edges);
    tatum::NodeId prev_node;
    int edge_idx = 0;
    for (const tatum::TimingPathElem& elem : path.data_arrival_path().elements()) {
        tatum::NodeId node = elem.node();

        if (prev_node) {
            ezgl::point2d start = tnode_draw_coord(prev_node);
            ezgl::point2d end = tnode_draw_coord(node);

            if(start.x > end.x) {
                std::swap(start, end);
            }

            double min_y = std::min(start.y, end.y);
            double max_y = std::max(start.y, end.y);

            ezgl::rectangle edge_bbox({start.x, min_y}, {end.x, max_y});
            
            ezgl::rectangle screen_coords = g->world_to_screen(edge_bbox);
            double edge_length = std::sqrt(std::pow(screen_coords.width(), 2) + std::pow(screen_coords.height(), 2));
            delay_label_info[edge_idx].edge_length = edge_length;

            int src_block_layer = get_timing_path_node_layer_num(node);
            int sink_block_layer = get_timing_path_node_layer_num(prev_node);
            t_draw_layer_display flyline_visibility = get_element_visibility_and_transparency(src_block_layer, sink_block_layer);

            if (!flyline_visibility.visible) {
                crit_path_delay_drawing_info[edge_idx].skip_delay_text = true;
                edge_idx++;
                prev_node = node;
                continue;
            } else {
                crit_path_delay_drawing_info[edge_idx].skip_delay_text = false;
            }

            double delay_text_angle = (180 / std::numbers::pi) * atan2(end.y - start.y, end.x - start.x);
            crit_path_delay_drawing_info[edge_idx].text_angle = delay_text_angle;

            double text_bbox_width = DELAY_TEXT_WIDTH * cos(delay_text_angle * (std::numbers::pi / 180)) 
                                    + DELAY_TEXT_HEIGHT * std::abs(sin(delay_text_angle * (std::numbers::pi / 180)));
            double text_bbox_height = DELAY_TEXT_WIDTH * std::abs(sin(delay_text_angle * (std::numbers::pi / 180)))
                                    + DELAY_TEXT_HEIGHT * cos(delay_text_angle * (std::numbers::pi / 180));

            ezgl::point2d bottom_left = screen_coords.center() - ezgl::point2d(text_bbox_width / 2, text_bbox_height / 2);
            crit_path_delay_drawing_info[edge_idx].virtual_centered_text_bbox = ezgl::rectangle(bottom_left, text_bbox_width, text_bbox_height);

            update_text_bbox_from_relative_pos(crit_path_delay_drawing_info[edge_idx], e_delay_text_relative_pos::CENTER_ABOVE);

            edge_idx++;
        }
        prev_node = node;
    }
}

#endif /* NO_GRAPHICS */
