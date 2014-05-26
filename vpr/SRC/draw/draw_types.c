#include "draw_global.h"
#include "draw_types.h"
#include "globals.h"
#include "vpr_utils.h"

void t_draw_state::reset_nets_congestion_and_rr() {
	show_nets = DRAW_NO_NETS;
	draw_rr_toggle = DRAW_NO_RR;
	show_congestion = DRAW_NO_CONGEST;
}

float t_draw_bbox::get_xcenter() const {
	return (xright + xleft) / 2;
}

float t_draw_bbox::get_ycenter() const {
	return (ytop + ybottom) / 2;
}

t_draw_bbox t_draw_bbox::get_absolute_bbox(const t_block& clb, t_pb* pb) {
	t_draw_coords* draw_coords = get_draw_coords_vars();

	t_draw_bbox result = draw_coords->get_pb_bbox(clb,*pb);
	
	while (pb->parent_pb != NULL) {
		t_draw_bbox& parents_bbox = draw_coords->get_pb_bbox(clb, *pb->parent_pb);
		result.xleft   += parents_bbox.xleft;
		result.xright  += parents_bbox.xleft;
		result.ybottom += parents_bbox.ybottom;
		result.ytop    += parents_bbox.ybottom;
		pb = pb->parent_pb;
	}

	float clb_x = draw_coords->tile_x[clb.x];
	float clb_y = draw_coords->tile_y[clb.y];	

	result.xleft   += clb_x;
	result.xright  += clb_x;
	result.ybottom += clb_y;
	result.ytop    += clb_y;

	return result;
}

t_draw_bbox t_draw_bbox::get_absolute_bbox(int clb_index, t_pb* pb) {
	return get_absolute_bbox(block[clb_index], pb);
}


t_draw_bbox& t_draw_coords::get_pb_bbox(int clb_index, const t_pb& pb) {
	return get_pb_bbox(block[clb_index], pb);
}

t_draw_bbox& t_draw_coords::get_pb_bbox(const t_block& clb, const t_pb& pb) {
	const int pb_gnode_id = get_unique_pb_graph_node_id(pb.pb_graph_node);
	const int clb_type_id = clb.type->index;

	return this->blk_info.at(clb_type_id).subblk_array.at(pb_gnode_id);
}