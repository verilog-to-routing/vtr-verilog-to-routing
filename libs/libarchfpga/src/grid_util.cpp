#include "grid_util.h"

#include "arch_error.h"
#include "device_grid.h"
#include "physical_types.h"
#include "vtr_expr_eval.h"

int adjust_interposer_cut_location(const DeviceGrid& grid,
                                   size_t layer,
                                   e_interposer_cut_type dim,
                                   const std::string& formula_str,
                                   vtr::FormulaParser& p,
                                   vtr::t_formula_data& formula_data) {
    const size_t grid_width = grid.width();
    const size_t grid_height = grid.height();
    formula_data.clear();
    formula_data.set_var_value("W", grid_width);
    formula_data.set_var_value("H", grid_height);
    const int base_cut_loc = p.parse_formula(formula_str, formula_data);

    // Vertical cut at loc: locations to the right of the cut (column at loc+1) must be root.
    // Horizontal cut at loc: locations above the cut (row at loc+1) must be root.
    auto is_cut_through_roots_only = [&grid, layer](e_interposer_cut_type d, int loc) {
        t_physical_tile_loc tile_loc;
        tile_loc.layer_num = static_cast<int>(layer);
        if (d == e_interposer_cut_type::VERT) {
            const int right_col = loc + 1;
            if (right_col < 0 || size_t(right_col) >= grid.width()) return false;
            tile_loc.x = right_col;
            for (size_t y = 0; y < grid.height(); y++) {
                tile_loc.y = static_cast<int>(y);
                if (grid.get_width_offset(tile_loc) != 0 || grid.get_height_offset(tile_loc) != 0)
                    return false;
            }
            return true;
        } else {
            VTR_ASSERT(d == e_interposer_cut_type::HORZ);
            const int row_above = loc + 1;
            if (row_above < 0 || size_t(row_above) >= grid.height()) return false;
            tile_loc.y = row_above;
            for (size_t x = 0; x < grid.width(); x++) {
                tile_loc.x = static_cast<int>(x);
                if (grid.get_width_offset(tile_loc) != 0 || grid.get_height_offset(tile_loc) != 0)
                    return false;
            }
            return true;
        }
    };

    int cut_loc = base_cut_loc;
    for (int offset = 0;; offset++) {
        int try_pos_plus = base_cut_loc + offset;
        int try_pos_minus = base_cut_loc - offset;
        if (offset == 0) {
            if (is_cut_through_roots_only(dim, try_pos_plus)) {
                cut_loc = try_pos_plus;
                break;
            }
        } else {
            bool plus_ok = is_cut_through_roots_only(dim, try_pos_plus);
            bool minus_ok = is_cut_through_roots_only(dim, try_pos_minus);
            if (plus_ok) {
                cut_loc = try_pos_plus;
                break;
            }
            if (minus_ok) {
                cut_loc = try_pos_minus;
                break;
            }
        }
        if (offset > static_cast<int>(std::max(grid_width, grid_height))) {
            archfpga_throw(__FILE__, __LINE__,
                          "Interposer cut (dim=%s, formula=%s -> %d) does not cross root locations only; "
                          "no valid offset found within grid bounds.",
                          dim == e_interposer_cut_type::VERT ? "VERT" : "HORZ",
                          formula_str.c_str(), base_cut_loc);
        }
    }

    if (cut_loc != base_cut_loc) {
        VTR_LOG_WARN(
            "Interposer cut moved to avoid cutting through block: W=%zu H=%zu formula='%s' evaluated to %d, resolved to %d (%s)\n",
            grid_width, grid_height, formula_str.c_str(), base_cut_loc, cut_loc,
            dim == e_interposer_cut_type::VERT ? "VERT" : "HORZ");
    }

    return cut_loc;
}

std::pair<std::vector<std::vector<int>>, std::vector<std::vector<int>>>
resolve_interposer_cut_locations(const DeviceGrid& grid,
                                 const t_grid_def& grid_def,
                                 vtr::FormulaParser& p) {
    const size_t num_layers = grid_def.layers.size();
    vtr::t_formula_data formula_data;

    std::vector<std::vector<int>> horizontal(num_layers);
    std::vector<std::vector<int>> vertical(num_layers);

    for (size_t layer = 0; layer < num_layers; layer++) {
        const t_layer_def& layer_def = grid_def.layers[layer];

        for (const t_interposer_cut_inf& cut_inf : layer_def.interposer_cuts) {
            const int cut_loc = adjust_interposer_cut_location(grid, layer, cut_inf.dim, cut_inf.loc, p, formula_data);

            if (cut_inf.dim == e_interposer_cut_type::VERT) {
                vertical[layer].push_back(cut_loc);
            } else {
                VTR_ASSERT(cut_inf.dim == e_interposer_cut_type::HORZ);
                horizontal[layer].push_back(cut_loc);
            }
        }
    }
    return {std::move(horizontal), std::move(vertical)};
}
