#include "draw_global.h"
#include "draw_types.h"
#include "globals.h"
#include "vpr_utils.h"
#include <utility>

/*******************************************
 * begin t_draw_state function definitions *
 *******************************************/

void t_draw_state::reset_nets_congestion_and_rr() {
    show_nets = DRAW_NO_NETS;
    draw_rr_toggle = DRAW_NO_RR;
    show_congestion = DRAW_NO_CONGEST;
}

bool t_draw_state::showing_sub_blocks() {
    return show_blk_internal > 0;
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
t_draw_coords::t_draw_coords() {
    tile_width = 0;
    pin_size = 0;
    tile_x = nullptr;
    tile_y = nullptr;
}

float t_draw_coords::get_tile_width() {
    return tile_width;
}

float t_draw_coords::get_tile_height() {
    //For now, same as width
    return tile_width;
}

t_bound_box t_draw_coords::get_pb_bbox(ClusterBlockId clb_index, const t_pb_graph_node& pb_gnode) {
    auto& place_ctx = g_vpr_ctx.placement();
    return get_pb_bbox(place_ctx.block_locs[clb_index].loc.x, place_ctx.block_locs[clb_index].loc.y, place_ctx.block_locs[clb_index].loc.z, pb_gnode);
}

t_bound_box t_draw_coords::get_pb_bbox(int grid_x, int grid_y, int sub_block_index, const t_pb_graph_node& pb_gnode) {
    auto& device_ctx = g_vpr_ctx.device();
    const int clb_type_id = device_ctx.grid[grid_x][grid_y].type->index;
    t_draw_pb_type_info& blk_type_info = this->blk_info.at(clb_type_id);

    t_bound_box result = blk_type_info.get_pb_bbox(pb_gnode);

    // if getting clb bbox, apply location info.
    if (pb_gnode.is_root()) {
        float sub_blk_offset = this->tile_width * (sub_block_index / (float)device_ctx.grid[grid_x][grid_y].type->capacity);

        result += t_point(this->tile_x[grid_x], this->tile_y[grid_y]);
        if (sub_block_index != 0) {
            result += t_point(sub_blk_offset, 0);
        }
    }

    return result;
}

t_bound_box t_draw_coords::get_absolute_pb_bbox(const ClusterBlockId clb_index, const t_pb_graph_node* pb_gnode) {
    t_bound_box result = this->get_pb_bbox(clb_index, *pb_gnode);

    // go up the graph, adding the parent bboxes to the result,
    // ie. make it relative to one level higher each time.
    while (pb_gnode && !pb_gnode->is_root()) {
        t_bound_box parents_bbox = this->get_pb_bbox(clb_index, *pb_gnode->parent_pb_graph_node);
        result += parents_bbox.bottom_left();
        pb_gnode = pb_gnode->parent_pb_graph_node;
    }

    return result;
}

t_bound_box t_draw_coords::get_absolute_clb_bbox(const ClusterBlockId clb_index, const t_type_ptr type) {
    return get_pb_bbox(clb_index, *type->pb_graph_head);
}

t_bound_box t_draw_coords::get_absolute_clb_bbox(int grid_x, int grid_y, int sub_block_index) {
    auto& device_ctx = g_vpr_ctx.device();
    return get_pb_bbox(grid_x, grid_y, sub_block_index, *device_ctx.grid[grid_x][grid_y].type->pb_graph_head);
}
