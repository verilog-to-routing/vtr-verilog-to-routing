#include <cstdio>
#include <cstring>
#include <algorithm>
#include <regex>
#include <limits>

#include "vtr_assert.h"
#include "vtr_math.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "vpr_utils.h"

#include "globals.h"
#include "SetupGrid.h"
#include "SetupVibGrid.h"
#include "vtr_expr_eval.h"

using vtr::FormulaParser;
using vtr::t_formula_data;

static VibDeviceGrid build_vib_device_grid(const t_vib_grid_def& grid_def, size_t grid_width, size_t grid_height, bool warn_out_of_range = true);

static void set_vib_grid_block_type(int priority,
                                    const VibInf* type,
                                    int layer_num,
                                    size_t x_root,
                                    size_t y_root,
                                    vtr::NdMatrix<const VibInf*, 3>& vib_grid,
                                    vtr::NdMatrix<int, 3>& grid_priorities);

VibDeviceGrid create_vib_device_grid(std::string layout_name, const std::vector<t_vib_grid_def>& vib_grid_layouts) {
    if (layout_name == "auto") {
        //We do not support auto layout now
        //
        VPR_FATAL_ERROR(VPR_ERROR_ARCH, "We do not support auto layout now\n");
        
    } else {
        //Use the specified device

        //Find the matching grid definition
        auto cmp = [&](const t_vib_grid_def& grid_def) {
            return grid_def.name == layout_name;
        };

        auto iter = std::find_if(vib_grid_layouts.begin(), vib_grid_layouts.end(), cmp);
        if (iter == vib_grid_layouts.end()) {
            //Not found
            std::string valid_names;
            for (size_t i = 0; i < vib_grid_layouts.size(); ++i) {
                if (i != 0) {
                    valid_names += ", ";
                }
                valid_names += "'" + vib_grid_layouts[i].name + "'";
            }
            VPR_FATAL_ERROR(VPR_ERROR_ARCH, "Failed to find grid layout named '%s' (valid grid layouts: %s)\n", layout_name.c_str(), valid_names.c_str());
        }

        return build_vib_device_grid(*iter, iter->width, iter->height);
    }
}

///@brief Build the specified device grid
static VibDeviceGrid build_vib_device_grid(const t_vib_grid_def& grid_def, size_t grid_width, size_t grid_height, bool warn_out_of_range) {
    if (grid_def.grid_type == VibGridDefType::VIB_FIXED) {
        if (grid_def.width != int(grid_width) || grid_def.height != int(grid_height)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Requested grid size (%zu%zu) does not match fixed device size (%dx%d)",
                            grid_width, grid_height, grid_def.width, grid_def.height);
        }
    }

    auto& device_ctx = g_vpr_ctx.device();

    //Initialize the grid and each location priority based on available dies in the architecture file
    vtr::NdMatrix<const VibInf*, 3> vib_grid;
    vtr::NdMatrix<int, 3> grid_priorities;
    int num_layers = (int)grid_def.layers.size();
    vib_grid.resize(std::array<size_t, 3>{(size_t)num_layers, grid_width, grid_height});
    
    //Track the current priority for each grid location
    // Note that we initialize it to the lowest (i.e. most negative) possible value, so
    // any user-specified priority will override the default empty grid
    grid_priorities.resize(std::array<size_t, 3>{(size_t)num_layers, grid_width, grid_height}, std::numeric_limits<int>::lowest());

    //Initialize the device to all empty blocks
    const VibInf* empty_type = nullptr;
    //VTR_ASSERT(empty_type != nullptr);
    for (int layer = 0; layer < num_layers; ++layer) {
        for (size_t x = 0; x < grid_width; ++x) {
            for (size_t y = 0; y < grid_height; ++y) {
                set_vib_grid_block_type(std::numeric_limits<int>::lowest() + 1, //+1 so it overrides without warning
                                        empty_type,
                                        layer, x, y,
                                        vib_grid, grid_priorities);
            }
        }
    }

    FormulaParser p;
    std::set<const VibInf*> seen_types;
    for (int layer = 0; layer < num_layers; layer++) {
        for (const auto& grid_loc_def : grid_def.layers.at(layer).loc_defs) {
            //Fill in the block types according to the specification
            //auto type = find_tile_type_by_name(grid_loc_def.block_type, device_ctx.physical_tile_types);
            const VibInf* type = nullptr;
            for (size_t vib_type = 0; vib_type < device_ctx.arch->vib_infs.size(); vib_type++) {
                if (grid_loc_def.block_type == device_ctx.arch->vib_infs[vib_type].get_name()) {
                    type = &device_ctx.arch->vib_infs[vib_type];
                    break;
                }
            }

            if (!type) {
                VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                "Failed to find block type '%s' for grid location specification",
                                grid_loc_def.block_type.c_str());
            }

            seen_types.insert(type);

            t_formula_data vars;
            vars.set_var_value("W", grid_width);
            vars.set_var_value("H", grid_height);
            vars.set_var_value("w", 1);
            vars.set_var_value("h", 1);

            //Load the x specification
            auto& xspec = grid_loc_def.x;

            VTR_ASSERT_MSG(!xspec.start_expr.empty(), "x start position must be specified");
            VTR_ASSERT_MSG(!xspec.end_expr.empty(), "x end position must be specified");
            VTR_ASSERT_MSG(!xspec.incr_expr.empty(), "x increment must be specified");
            VTR_ASSERT_MSG(!xspec.repeat_expr.empty(), "x repeat must be specified");

            size_t startx = p.parse_formula(xspec.start_expr, vars);
            size_t endx = p.parse_formula(xspec.end_expr, vars);
            size_t incrx = p.parse_formula(xspec.incr_expr, vars);
            size_t repeatx = p.parse_formula(xspec.repeat_expr, vars);

            //Load the y specification
            auto& yspec = grid_loc_def.y;

            VTR_ASSERT_MSG(!yspec.start_expr.empty(), "y start position must be specified");
            VTR_ASSERT_MSG(!yspec.end_expr.empty(), "y end position must be specified");
            VTR_ASSERT_MSG(!yspec.incr_expr.empty(), "y increment must be specified");
            VTR_ASSERT_MSG(!yspec.repeat_expr.empty(), "y repeat must be specified");

            size_t starty = p.parse_formula(yspec.start_expr, vars);
            size_t endy = p.parse_formula(yspec.end_expr, vars);
            size_t incry = p.parse_formula(yspec.incr_expr, vars);
            size_t repeaty = p.parse_formula(yspec.repeat_expr, vars);

            //Check start against the device dimensions
            // Start locations outside the device will never create block instances
            if (startx > grid_width - 1) {
                if (warn_out_of_range) {
                    VTR_LOG_WARN("Block type '%s' grid location specification startx (%s = %d) falls outside device horizontal range [%d,%d]\n",
                                 type->get_name(), xspec.start_expr.c_str(), startx, 0, grid_width - 1);
                }
                continue; //No instances will be created
            }

            if (starty > grid_height - 1) {
                if (warn_out_of_range) {
                    VTR_LOG_WARN("Block type '%s' grid location specification starty (%s = %d) falls outside device vertical range [%d,%d]\n",
                                 type->get_name(), yspec.start_expr.c_str(), starty, 0, grid_height - 1);
                }
                continue; //No instances will be created
            }

            //Check end against the device dimensions
            if (endx > grid_width - 1) {
                if (warn_out_of_range) {
                    VTR_LOG_WARN("Block type '%s' grid location specification endx (%s = %d) falls outside device horizontal range [%d,%d]\n",
                                 type->get_name(), xspec.end_expr.c_str(), endx, 0, grid_width - 1);
                }
            }

            if (endy > grid_height - 1) {
                if (warn_out_of_range) {
                    VTR_LOG_WARN("Block type '%s' grid location specification endy (%s = %d) falls outside device vertical range [%d,%d]\n",
                                 type->get_name(), yspec.end_expr.c_str(), endy, 0, grid_height - 1);
                }
            }

            //The end must fall after (or equal) to the start
            if (endx < startx) {
                VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                "Grid location specification endx (%s = %d) can not come before startx (%s = %d) for block type '%s'",
                                xspec.end_expr.c_str(), endx, xspec.start_expr.c_str(), startx, type->get_name());
            }

            if (endy < starty) {
                VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                "Grid location specification endy (%s = %d) can not come before starty (%s = %d) for block type '%s'",
                                yspec.end_expr.c_str(), endy, yspec.start_expr.c_str(), starty, type->get_name());
            }

            //The minimum increment is the block dimension
            //VTR_ASSERT(type->width > 0);
            if (incrx < 1/*size_t(type->width)*/) {
                VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                "Grid location specification incrx for block type '%s' must be at least"
                                " block width (%d) to avoid overlapping instances (was %s = %d)",
                                type->get_name(), 1, xspec.incr_expr.c_str(), incrx);
            }

            //VTR_ASSERT(type->height > 0);
            if (incry < 1/*size_t(type->height)*/) {
                VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                "Grid location specification incry for block type '%s' must be at least"
                                " block height (%d) to avoid overlapping instances (was %s = %d)",
                                type->get_name(), 1, yspec.incr_expr.c_str(), incry);
            }

            //The minimum repeat is the region dimension
            size_t region_width = endx - startx + 1; //+1 since start/end are both inclusive
            if (repeatx < region_width) {
                VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                "Grid location specification repeatx for block type '%s' must be at least"
                                " the region width (%d) to avoid overlapping instances (was %s = %d)",
                                type->get_name(), region_width, xspec.repeat_expr.c_str(), repeatx);
            }

            size_t region_height = endy - starty + 1; //+1 since start/end are both inclusive
            if (repeaty < region_height) {
                VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                "Grid location specification repeaty for block type '%s' must be at least"
                                " the region height (%d) to avoid overlapping instances (was %s = %d)",
                                type->get_name(), region_height, xspec.repeat_expr.c_str(), repeaty);
            }

            //VTR_LOG("Applying grid_loc_def for '%s' priority %d startx=%s=%zu, endx=%s=%zu, starty=%s=%zu, endx=%s=%zu,\n",
            //            type->name, grid_loc_def.priority,
            //            xspec.start_expr.c_str(), startx, xspec.end_expr.c_str(), endx,
            //            yspec.start_expr.c_str(), starty, yspec.end_expr.c_str(), endy);

            size_t x_end = 0;
            for (size_t kx = 0; x_end < grid_width; ++kx) { //Repeat in x direction
                size_t x_start = startx + kx * repeatx;
                x_end = endx + kx * repeatx;

                size_t y_end = 0;
                for (size_t ky = 0; y_end < grid_height; ++ky) { //Repeat in y direction
                    size_t y_start = starty + ky * repeaty;
                    y_end = endy + ky * repeaty;

                    size_t x_max = std::min(x_end, grid_width - 1);
                    size_t y_max = std::min(y_end, grid_height - 1);

                    //Fill in the region
                    for (size_t x = x_start; x <= x_max; x += incrx) {
                        for (size_t y = y_start; y <= y_max; y += incry) {
                            set_vib_grid_block_type(grid_loc_def.priority,
                                                    type,
                                                    layer, x, y,
                                                    vib_grid, grid_priorities);
                        }
                    }
                }
            }
        }
    }

    //Warn if any types were not specified in the grid layout
    // for (auto const& type : device_ctx.physical_tile_types) {
    //     if (&type == empty_type) continue; //Don't worry if empty hasn't been specified

    //     if (!seen_types.count(&type)) {
    //         VTR_LOG_WARN("Block type '%s' was not specified in device grid layout\n",
    //                      type.name);
    //     }
    // }

    auto vib_device_grid = VibDeviceGrid(grid_def.name, vib_grid);

    // CheckGrid(device_grid);

    return vib_device_grid;
}

static void set_vib_grid_block_type(int priority,
                                    const VibInf* type,
                                    int layer_num,
                                    size_t x_root,
                                    size_t y_root,
                                    vtr::NdMatrix<const VibInf*, 3>& vib_grid,
                                    vtr::NdMatrix<int, 3>& grid_priorities) {
    struct TypeLocation {
        TypeLocation(size_t x_val, size_t y_val, const VibInf* type_val, int priority_val)
            : x(x_val)
            , y(y_val)
            , type(type_val)
            , priority(priority_val) {}
        size_t x;
        size_t y;
        const VibInf* type;
        int priority;

        bool operator<(const TypeLocation& rhs) const {
            return x < rhs.x || y < rhs.y || type < rhs.type;
        }
    };

    //Collect locations effected by this block
    std::set<TypeLocation> target_locations;
    for (size_t x = x_root; x < x_root + 1; ++x) {
        for (size_t y = y_root; y < y_root + 1; ++y) {
            target_locations.insert(TypeLocation(x, y, vib_grid[layer_num][x][y], grid_priorities[layer_num][x][y]));
        }
    }

    //Record the highest priority of all effected locations
    auto iter = target_locations.begin();
    TypeLocation max_priority_type_loc = *iter;
    for (; iter != target_locations.end(); ++iter) {
        if (iter->priority > max_priority_type_loc.priority) {
            max_priority_type_loc = *iter;
        }
    }

    if (priority < max_priority_type_loc.priority) {
        //Lower priority, do not override
#ifdef VERBOSE
        VTR_LOG("Not creating block '%s' at (%zu,%zu) since overlaps block '%s' at (%zu,%zu) with higher priority (%d > %d)\n",
                type->name, x_root, y_root, max_priority_type_loc.type->name, max_priority_type_loc.x, max_priority_type_loc.y,
                max_priority_type_loc.priority, priority);
#endif
        return;
    }

    if (priority == max_priority_type_loc.priority) {
        //Ambiguous case where current grid block and new specification have equal priority
        //
        //We arbitrarily decide to take the 'last applied' wins approach, and warn the user
        //about the potential ambiguity
        VTR_LOG_WARN(
            "Ambiguous block type specification at grid location (%zu,%zu)."
            " Existing block type '%s' at (%zu,%zu) has the same priority (%d) as new overlapping type '%s'."
            " The last specification will apply.\n",
            x_root, y_root,
            max_priority_type_loc.type->get_name(), max_priority_type_loc.x, max_priority_type_loc.y,
            priority, type->get_name());
    }

    //Mark all the grid tiles 'covered' by this block with the appropriate type
    //and width/height offsets
    std::set<TypeLocation> root_blocks_to_rip_up;
    auto& device_ctx = g_vpr_ctx.device();
    for (size_t x = x_root; x < x_root + 1; ++x) {
        VTR_ASSERT(x < vib_grid.end_index(1));

        //size_t x_offset = x - x_root;
        for (size_t y = y_root; y < y_root + 1; ++y) {
            VTR_ASSERT(y < vib_grid.end_index(2));
            //size_t y_offset = y - y_root;

            auto& grid_tile = vib_grid[layer_num][x][y];
            VTR_ASSERT(grid_priorities[layer_num][x][y] <= priority);

            if (grid_tile != nullptr
                //&& grid_tile.type != device_ctx.EMPTY_PHYSICAL_TILE_TYPE
                ) {
                //We are overriding a non-empty block, we need to be careful
                //to ensure we remove any blocks which will be invalidated when we
                //overwrite part of their locations

                size_t orig_root_x = x;
                size_t orig_root_y = y;

                root_blocks_to_rip_up.insert(TypeLocation(orig_root_x,
                                                          orig_root_y,
                                                          vib_grid[layer_num][x][y],
                                                          grid_priorities[layer_num][x][y]));
            }

            vib_grid[layer_num][x][y] = type;
            //grid[layer_num][x][y].width_offset = x_offset;
            //grid[layer_num][x][y].height_offset = y_offset;
            //grid[layer_num][x][y].meta = meta;

            grid_priorities[layer_num][x][y] = priority;
        }
    }

    //Rip-up any invalidated blocks
    for (auto invalidated_root : root_blocks_to_rip_up) {
        //Mark all the grid locations used by this root block as empty
        for (size_t x = invalidated_root.x; x < invalidated_root.x + 1; ++x) {
            int x_offset = x - invalidated_root.x;
            for (size_t y = invalidated_root.y; y < invalidated_root.y + 1; ++y) {
                int y_offset = y - invalidated_root.y;

                if (vib_grid[layer_num][x][y] == invalidated_root.type
                    && 0 == x_offset
                    && 0 == y_offset) {
                    //This is a left-over invalidated block, mark as empty
                    // Note: that we explicitly check the type and offsets, since the original block
                    //       may have been completely overwritten, and we don't want to change anything
                    //       in that case
                    //VTR_ASSERT(device_ctx.EMPTY_PHYSICAL_TILE_TYPE->width == 1);
                    //VTR_ASSERT(device_ctx.EMPTY_PHYSICAL_TILE_TYPE->height == 1);

#ifdef VERBOSE
                    VTR_LOG("Ripping up block '%s' at (%d,%d) offset (%d,%d). Overlapped by '%s' at (%d,%d)\n",
                            invalidated_root.type->name, invalidated_root.x, invalidated_root.y,
                            x_offset, y_offset,
                            type->name, x_root, y_root);
#endif

                    vib_grid[layer_num][x][y] = nullptr;
                    //grid[layer_num][x][y].width_offset = 0;
                    //grid[layer_num][x][y].height_offset = 0;

                    grid_priorities[layer_num][x][y] = std::numeric_limits<int>::lowest();
                }
            }
        }
    }
}
