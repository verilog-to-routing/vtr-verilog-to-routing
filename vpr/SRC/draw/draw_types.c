#include "draw_global.h"
#include "draw_types.h"
#include "globals.h"
#include "vpr_utils.h"
#include <utility>

/*******************************************
 * begin t_draw_state function definitions *
 *******************************************/

t_draw_state::t_draw_state() :
	pic_on_screen(NO_PICTURE),
	show_nets(DRAW_NO_NETS),
	show_congestion(DRAW_NO_CONGEST),
	draw_rr_toggle(DRAW_NO_RR),
	max_sub_blk_lvl(0),
	show_blk_internal(0),
	show_graphics(FALSE),
	gr_automode(0),
	draw_route_type(GLOBAL),
	net_color(NULL),
	block_color(NULL),
	draw_rr_node(NULL) { 

}

void t_draw_state::reset_nets_congestion_and_rr() {
	show_nets = DRAW_NO_NETS;
	draw_rr_toggle = DRAW_NO_RR;
	show_congestion = DRAW_NO_CONGEST;
}

/**************************************************
 * begin t_draw_pb_type_info function definitions *
 **************************************************/

t_bound_box t_draw_pb_type_info::get_pb_bbox(const t_pb_graph_node& pb_graph_node) {
	return get_pb_bbox_ref(pb_graph_node);
}

t_bound_box& t_draw_pb_type_info::get_pb_bbox_ref(const t_pb_graph_node& pb_graph_node) {
	const int pb_gnode_id = get_unique_pb_graph_node_id(&pb_graph_node);
	return subblk_array.at(pb_gnode_id);
}

/********************************************
 * begin t_draw_corrds function definitions *
 ********************************************/

t_bound_box t_draw_coords::get_pb_bbox(int clb_index, const t_pb_graph_node& pb_gnode) {
	return get_pb_bbox(block[clb_index], pb_gnode);
}

t_bound_box t_draw_coords::get_pb_bbox(const t_block& clb, const t_pb_graph_node& pb_gnode) {
	const int clb_type_id = clb.type->index;
	t_draw_pb_type_info& blk_type_info = this->blk_info.at(clb_type_id);

	t_bound_box result = blk_type_info.get_pb_bbox(pb_gnode);

	// if pb is in the leftmost, rightmost, top, or bottom rows,
	// flip the bbox accordingly
	if (clb.y == 0 || clb.x == 0) {
		if (pb_gnode.parent_pb_graph_node != NULL) {
			float parent_height = blk_type_info.get_pb_bbox_ref(*pb_gnode.parent_pb_graph_node).get_height();
			float new_then_old_bottom = parent_height - result.top();

			std::swap(new_then_old_bottom, result.bottom());
			// new_then_old_bottom now contains the old bottom

			result.top() = parent_height - new_then_old_bottom;
		} // else {
			// it's a rectangle, so unless it's inside of someting else,
			// flipping it up-down does nothing...
		// }
	}

	if (clb.x == nx + 1 || clb.x == 0) {
		std::swap(result.right(), result.top());
		std::swap(result.left(), result.bottom());
	}

	// if getting clb bbox, apply location info.
	if (pb_gnode.parent_pb_graph_node == NULL) {
		float sub_blk_offset = this->tile_width * (clb.z/(float)grid[clb.x][clb.y].type->capacity);

		// blk_height = grid[i][j].type->height;
		// this->tile_x[clb.x] + this->tile_width,
		// this->tile_y[clb.y + blk_height - 1] + ((clb.z + 1) * sub_tile_offset)
		result += t_point(this->tile_x[clb.x], this->tile_y[clb.y]);
		if (clb.z != 0) {
			if (clb.x == 0 || clb.x == nx + 1) {
				result += t_point(0, sub_blk_offset);
			} else if (clb.y == 0 || clb.y == ny + 1) {
				result += t_point(sub_blk_offset, 0);
			}
		}
	}

	return result;
}

t_bound_box t_draw_coords::get_absolute_pb_bbox(const t_block& clb, t_pb_graph_node* pb_gnode) {
	t_bound_box result = this->get_pb_bbox(clb,*pb_gnode);
	

	while (pb_gnode->parent_pb_graph_node != NULL) {
		t_bound_box parents_bbox = this->get_pb_bbox(clb, *pb_gnode->parent_pb_graph_node);
		result += parents_bbox.bottom_left();
		pb_gnode = pb_gnode->parent_pb_graph_node;
	}

	return result;
}

t_bound_box t_draw_coords::get_absolute_pb_bbox(int clb_index, t_pb_graph_node* pb_gnode) {
	return get_absolute_pb_bbox(block[clb_index], pb_gnode);
}
