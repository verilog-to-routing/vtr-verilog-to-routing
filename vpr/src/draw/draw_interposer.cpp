
#ifndef NO_GRAPHICS

#include "draw_interposer.h"

#include "draw_global.h"
#include "vtr_assert.h"
#include "globals.h"

void draw_interposer_cuts(ezgl::renderer* g) {
    const t_draw_state* draw_state = get_draw_state_vars();
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const DeviceGrid& grid = device_ctx.grid;
    const t_draw_coords* draw_coords = get_draw_coords_vars();

    if (draw_state->pic_on_screen == e_pic_type::NO_PICTURE) {
        return;
    }
    VTR_ASSERT_SAFE(draw_state->pic_on_screen == e_pic_type::PLACEMENT || draw_state->pic_on_screen == e_pic_type::ROUTING);

    // A rectangle containing the FPGA fabric with some padding.
    // Used to determine the start and end coordinates of an interposer cut whose x or y position is fixed.
    const ezgl::rectangle world{
        {draw_coords->tile_x.front() - draw_coords->get_tile_width(),
         draw_coords->tile_y.front() - draw_coords->get_tile_height()},
        {draw_coords->tile_x.back() + 2 * draw_coords->get_tile_width(),
         draw_coords->tile_y.back() + 2 * draw_coords->get_tile_height()}
    };

    g->set_color(ezgl::BLACK, 255);
    g->set_line_dash(ezgl::line_dash::asymmetric_5_3);
    g->set_line_width(2);

    const std::vector<std::vector<int>>& horizontal_cuts = grid.get_horizontal_interposer_cuts();
    const std::vector<std::vector<int>>& vertical_cuts = grid.get_vertical_interposer_cuts();
    std::vector<std::pair<ezgl::point2d, ezgl::point2d>> lines_to_draw;

    for (size_t layer = 0; layer < grid.get_num_layers(); layer++) {
        if (!draw_state->draw_layer_display[layer].visible) {
            continue;
        }

        for (int cut_y : horizontal_cuts[layer]) {
            float y;
            if (draw_state->pic_on_screen == e_pic_type::PLACEMENT) {
                // Draw the interposer line exactly in the middle of routing channel.
                y = (draw_coords->tile_y[cut_y + 1] + draw_coords->tile_y[cut_y] + draw_coords->get_tile_height()) / 2.0f;
            } else if (draw_state->pic_on_screen == e_pic_type::ROUTING) {
                // Draw the interposer above the last track and below the next tile.
                // The row below the cut owns the channel, so the cut is drawn closer
                // to the row above the cut to make this ownership explicit.
                y = draw_coords->tile_y[cut_y + 1] - 0.5f;
            } else {
                VTR_ASSERT(false);
            }

            lines_to_draw.push_back({{world.left(), y}, {world.right(), y}});
        }

        for (int cut_x : vertical_cuts[layer]) {
            float x;
            if (draw_state->pic_on_screen == e_pic_type::PLACEMENT) {
                // Draw the interposer line exactly in the middle of routing channel.
                x = (draw_coords->tile_x[cut_x + 1] + draw_coords->tile_x[cut_x] + draw_coords->get_tile_width()) / 2.0f;
            } else if (draw_state->pic_on_screen == e_pic_type::ROUTING) {
                // Draw the interposer line between the last track and the next column.
                // The channel belongs to the column on the left of the cut, so the cut is
                // positioned closer to the column on the right to make this ownership clear.
                x = draw_coords->tile_x[cut_x + 1] - 0.5f;
            } else {
                VTR_ASSERT(false);
            }

            lines_to_draw.push_back({{x, world.bottom()}, {x, world.top()}});
        }
    }

    for (const auto& [start, end] : lines_to_draw) {
        g->draw_line(start, end);
    }
}

#endif