#ifndef NO_GRAPHICS

#    include "draw_global.h"
#    include "draw_types.h"
#    include "draw_toggle_functions.h"
#    include "globals.h"
#    include "vpr_utils.h"
#    include "draw.h"
#    include <utility>

/*******************************************
 * begin t_draw_state function definitions *
 *******************************************/
ezgl::color t_draw_state::block_color(ClusterBlockId blk) const {
    if (use_default_block_color_[blk]) {
        auto& cluster_ctx = g_vpr_ctx.clustering();
        const auto& block_locs = get_graphics_blk_loc_registry_ref().block_locs();

        t_physical_tile_type_ptr tile_type = nullptr;
        if (block_locs.empty()) { //No placement, pick best match
            tile_type = pick_physical_type(cluster_ctx.clb_nlist.block_type(blk));
        } else { // Have placement, select physical tile implementing blk
            t_pl_loc block_loc = block_locs[blk].loc;
            tile_type = physical_tile_type(block_loc);
        }
        VTR_ASSERT(tile_type != nullptr);
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
    t_draw_state* draw_state = get_draw_state_vars();
    const auto& block_locs = draw_state->get_graphics_blk_loc_registry_ref().block_locs();
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    return get_pb_bbox(block_locs[clb_index].loc.layer,
                       block_locs[clb_index].loc.x,
                       block_locs[clb_index].loc.y,
                       block_locs[clb_index].loc.sub_tile,
                       cluster_ctx.clb_nlist.block_type(clb_index),
                       pb_gnode);
}

ezgl::rectangle t_draw_coords::get_pb_bbox(int grid_layer, int grid_x, int grid_y, int sub_block_index, const t_logical_block_type_ptr logical_block_type, const t_pb_graph_node& pb_gnode) {
    const auto& device_ctx = g_vpr_ctx.device();
    t_draw_pb_type_info& blk_type_info = this->blk_info.at(logical_block_type->index);

    ezgl::rectangle result = blk_type_info.get_pb_bbox(pb_gnode);

    // if getting clb bbox, apply location info.
    if (pb_gnode.is_root()) {
        const auto& type = device_ctx.grid.get_physical_type({grid_x, grid_y, grid_layer});
        float sub_blk_offset = this->tile_width * (sub_block_index / (float)type->capacity);

        result += ezgl::point2d(this->tile_x[grid_x], this->tile_y[grid_y]);
        if (sub_block_index != 0) {
            result += ezgl::point2d(sub_blk_offset, 0);
        }
    }
    return result;
}

ezgl::rectangle t_draw_coords::get_pb_bbox(int grid_layer, int grid_x, int grid_y, int sub_block_index, const t_logical_block_type_ptr logical_block_type) {
    const auto& device_ctx = g_vpr_ctx.device();
    t_draw_pb_type_info& blk_type_info = this->blk_info.at(logical_block_type->index);

    auto& pb_gnode = *logical_block_type->pb_graph_head;
    ezgl::rectangle result = blk_type_info.get_pb_bbox(pb_gnode);

    // if getting clb bbox, apply location info.
    if (pb_gnode.is_root()) {
        const auto& type = device_ctx.grid.get_physical_type({grid_x, grid_y, grid_layer});
        float sub_blk_offset = this->tile_width * (sub_block_index / (float)type->capacity);

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
    // i.e. make it relative to one level higher each time.
    while (pb_gnode && !pb_gnode->is_root()) {
        ezgl::rectangle parents_bbox = this->get_pb_bbox(clb_index, *pb_gnode->parent_pb_graph_node);
        result += parents_bbox.bottom_left();
        pb_gnode = pb_gnode->parent_pb_graph_node;
    }

    return result;
}

ezgl::rectangle t_draw_coords::get_absolute_clb_bbox(const ClusterBlockId clb_index, const t_logical_block_type_ptr block_type) {
    t_draw_state* draw_state = get_draw_state_vars();
    const auto& block_locs = draw_state->get_graphics_blk_loc_registry_ref().block_locs();

    t_pl_loc loc = block_locs[clb_index].loc;
    return get_pb_bbox(loc.layer, loc.x, loc.y, loc.sub_tile, block_type);
}

ezgl::rectangle t_draw_coords::get_absolute_clb_bbox(int grid_layer, int grid_x, int grid_y, int sub_block_index) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& type = device_ctx.grid.get_physical_type({grid_x, grid_y, grid_layer});
    return get_pb_bbox(grid_layer, grid_x, grid_y, sub_block_index, pick_logical_type(type));
}

ezgl::rectangle t_draw_coords::get_absolute_clb_bbox(int grid_layer, int grid_x, int grid_y, int sub_block_index, const t_logical_block_type_ptr logical_block_type) {
    return get_pb_bbox(grid_layer, grid_x, grid_y, sub_block_index, logical_block_type);
}

#endif // NO_GRAPHICS
