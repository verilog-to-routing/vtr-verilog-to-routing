#include "grid_util.h"

#include "device_grid.h"
#include "physical_types.h"
#include "vtr_expr_eval.h"

int adjust_interposer_cut_location(const DeviceGrid& grid,
                                   int layer,
                                   e_interposer_cut_type dim,
                                   const std::string& formula_str,
                                   vtr::FormulaParser& p,
                                   vtr::t_formula_data& formula_data) {
    // Cuts must not go through tiles. If formula position doesn't satisfy this, try base +/-1, +/-2, ...

    const size_t grid_width = grid.width();
    const size_t grid_height = grid.height();
    formula_data.clear();
    formula_data.set_var_value("W", grid_width);
    formula_data.set_var_value("H", grid_height);
    const int base_cut_loc = p.parse_formula(formula_str, formula_data);

    const int grid_w = grid.width();
    const int grid_h = grid.height();

    auto is_cut_through_roots_only = [&grid, layer, grid_w, grid_h](e_interposer_cut_type d, int loc) {
        if (d == e_interposer_cut_type::VERT) {
            const int right_col = loc + 1;
            if (right_col < 0 || right_col >= grid_w) return false;
            for (int y = 0; y < grid_h; y++) {
                if (grid.get_width_offset({right_col, y, layer}) != 0)
                    return false;
            }
            return true;
        } else {
            VTR_ASSERT(d == e_interposer_cut_type::HORZ);
            const int row_above = loc + 1;
            if (row_above < 0 || row_above >= grid_h) return false;
            for (int x = 0; x < grid_w; x++) {
                if (grid.get_height_offset({x, row_above, layer}) != 0)
                    return false;
            }
            return true;
        }
    };

    int cut_loc = base_cut_loc;
    if (!is_cut_through_roots_only(dim, base_cut_loc)) {
        for (int offset = 1;; offset++) {
            int try_pos_plus = base_cut_loc + offset;
            int try_pos_minus = base_cut_loc - offset;
            
            bool plus_ok = is_cut_through_roots_only(dim, try_pos_plus);
            if (plus_ok) {
                cut_loc = try_pos_plus;
                break;
            }

            bool minus_ok = is_cut_through_roots_only(dim, try_pos_minus);
            if (minus_ok) {
                cut_loc = try_pos_minus;
                break;
            }
            int max_offset = (dim == e_interposer_cut_type::VERT) ? grid_w : grid_h;
            VTR_ASSERT(offset <= max_offset);
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
