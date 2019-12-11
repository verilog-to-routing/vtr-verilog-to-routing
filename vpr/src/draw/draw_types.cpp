#ifndef NO_GRAPHICS

#    include "draw_global.h"
#    include "draw_types.h"
#    include "globals.h"
#    include "vpr_utils.h"
#    include "draw.h"
#    include <utility>

/*******************************************
 * begin t_draw_state function definitions *
 *******************************************/
ezgl::color t_draw_state::block_color(ClusterBlockId blk) const {
    if (use_default_block_color_[blk]) {
        t_physical_tile_type_ptr tile_type = get_physical_tile_type(blk);
        return get_block_type_color(tile_type);
    } else {
        return block_color_[blk];
    }
}

void t_draw_state::set_block_color(ClusterBlockId blk, ezgl::color color) {
    block_color_[blk] = color;
    use_default_block_color_[blk] = false;
}

void t_draw_state::reset_block_color(ClusterBlockId blk) {
    use_default_block_color_[blk] = true;
}
void t_draw_state::reset_block_colors() {
    std::fill(use_default_block_color_.begin(),
              use_default_block_color_.end(),
              true);
}

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

ezgl::rectangle t_draw_pb_type_info::get_pb_bbox(const t_pb_graph_node& pb_gnode) {
    return get_pb_bbox_ref(pb_gnode);
}

ezgl::rectangle& t_draw_pb_type_info::get_pb_bbox_ref(const t_pb_graph_node& pb_gnode) {
    const int pb_gnode_id = get_unique_pb_graph_node_id(&pb_gnode);
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

ezgl::rectangle t_draw_coords::get_pb_bbox(ClusterBlockId clb_index, const t_pb_graph_node& pb_gnode) {
    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    return get_pb_bbox(place_ctx.block_locs[clb_index].loc.x, place_ctx.block_locs[clb_index].loc.y, place_ctx.block_locs[clb_index].loc.z, cluster_ctx.clb_nlist.block_type(clb_index), pb_gnode);
}

ezgl::rectangle t_draw_coords::get_pb_bbox(int grid_x, int grid_y, int sub_block_index, const t_logical_block_type_ptr logical_block_type, const t_pb_graph_node& pb_gnode) {
    auto& device_ctx = g_vpr_ctx.device();
    t_draw_pb_type_info& blk_type_info = this->blk_info.at(logical_block_type->index);

    ezgl::rectangle result = blk_type_info.get_pb_bbox(pb_gnode);

    // if getting clb bbox, apply location info.
    if (pb_gnode.is_root()) {
        float sub_blk_offset = this->tile_width * (sub_block_index / (float)device_ctx.grid[grid_x][grid_y].type->capacity);

        result += ezgl::point2d(this->tile_x[grid_x], this->tile_y[grid_y]);
        if (sub_block_index != 0) {
            result += ezgl::point2d(sub_blk_offset, 0);
        }
    }
    return result;
}

ezgl::rectangle t_draw_coords::get_pb_bbox(int grid_x, int grid_y, int sub_block_index, const t_logical_block_type_ptr logical_block_type) {
    auto& device_ctx = g_vpr_ctx.device();
    t_draw_pb_type_info& blk_type_info = this->blk_info.at(logical_block_type->index);

    auto& pb_gnode = *logical_block_type->pb_graph_head;
    ezgl::rectangle result = blk_type_info.get_pb_bbox(pb_gnode);

    // if getting clb bbox, apply location info.
    if (pb_gnode.is_root()) {
        float sub_blk_offset = this->tile_width * (sub_block_index / (float)device_ctx.grid[grid_x][grid_y].type->capacity);

        result += ezgl::point2d(this->tile_x[grid_x], this->tile_y[grid_y]);
        if (sub_block_index != 0) {
            result += ezgl::point2d(sub_blk_offset, 0);
        }
    }
    return result;
}

ezgl::rectangle t_draw_coords::get_absolute_pb_bbox(const ClusterBlockId clb_index, const t_pb_graph_node* pb_gnode) {
    ezgl::rectangle result = this->get_pb_bbox(clb_index, *pb_gnode);

    // go up the graph, adding the parent bboxes to the result,
    // ie. make it relative to one level higher each time.
    while (pb_gnode && !pb_gnode->is_root()) {
        ezgl::rectangle parents_bbox = this->get_pb_bbox(clb_index, *pb_gnode->parent_pb_graph_node);
        result += parents_bbox.bottom_left();
        pb_gnode = pb_gnode->parent_pb_graph_node;
    }

    return result;
}

ezgl::rectangle t_draw_coords::get_absolute_clb_bbox(const ClusterBlockId clb_index, const t_logical_block_type_ptr block_type) {
    auto& place_ctx = g_vpr_ctx.placement();

    t_pl_loc loc = place_ctx.block_locs[clb_index].loc;
    return get_pb_bbox(loc.x, loc.y, loc.z, block_type);
}

ezgl::rectangle t_draw_coords::get_absolute_clb_bbox(int grid_x, int grid_y, int sub_block_index) {
    auto& device_ctx = g_vpr_ctx.device();
    return get_pb_bbox(grid_x, grid_y, sub_block_index, pick_best_logical_type(device_ctx.grid[grid_x][grid_y].type));
}

ezgl::rectangle t_draw_coords::get_absolute_clb_bbox(int grid_x, int grid_y, int sub_block_index, const t_logical_block_type_ptr logical_block_type) {
    return get_pb_bbox(grid_x, grid_y, sub_block_index, logical_block_type);
}

#endif // NO_GRAPHICS
