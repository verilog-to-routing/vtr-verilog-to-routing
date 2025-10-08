
#include "draw_interposer.h"

#include "draw_global.h"
#include "vtr_assert.h"
#include "globals.h"

void draw_interposer_cuts(ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const DeviceGrid& grid = device_ctx.grid;
    t_draw_coords* draw_coords = get_draw_coords_vars();

    const std::vector<std::vector<int>>& horizontal_cuts = grid.get_horizontal_interposer_cuts();
    const std::vector<std::vector<int>>& vertical_cuts = grid.get_vertical_interposer_cuts();

    std::vector<std::pair<ezgl::point2d, ezgl::point2d>> lines_to_draw;

    const ezgl::rectangle world{{draw_coords->tile_x.front() - draw_coords->get_tile_width(), draw_coords->tile_y.front() - draw_coords->get_tile_height()},
                                {draw_coords->tile_x.back() + 2 * draw_coords->get_tile_width(), draw_coords->tile_y.back() + 2 * draw_coords->get_tile_height()}};

    g->set_color(ezgl::BLACK, 255);
    g->set_line_dash(ezgl::line_dash::asymmetric_5_3);
    g->set_line_width(2);

    const float offset_factor = draw_state->pic_on_screen == PLACEMENT ? -0.5f : -0.5f / device_ctx.chan_width.max;

    for (size_t layer = 0; layer < grid.get_num_layers(); layer++) {
        if (!draw_state->draw_layer_display[layer].visible) {
            continue;
        }

        for (int cut_y : horizontal_cuts[layer]) {
            float y = draw_coords->tile_y[cut_y + 1] + offset_factor * draw_coords->get_tile_height();
            lines_to_draw.push_back({{world.left(), y}, {world.right(), y}});
        }

        for (int cut_x : vertical_cuts[layer]) {
            float x = draw_coords->tile_x[cut_x + 1] + offset_factor * draw_coords->get_tile_width();
            lines_to_draw.push_back({{x, world.bottom()}, {x, world.top()}});
        }
    }

    if (draw_state->pic_on_screen == PLACEMENT || draw_state->pic_on_screen == ROUTING) {
        for (const auto& [start, end] : lines_to_draw) {
            g->draw_line(start, end);
        }
    } else {
        VTR_ASSERT(draw_state->pic_on_screen == NO_PICTURE);
    }
}
