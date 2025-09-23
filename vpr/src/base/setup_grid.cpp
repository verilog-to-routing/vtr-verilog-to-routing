/**
 * @file
 * @author Jason Luu
 * @date   October 8, 2008
 *
 * @brief Initializes and allocates the physical logic block grid for VPR.
 */

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <limits>

#include "physical_types_util.h"
#include "vtr_assert.h"
#include "vtr_math.h"
#include "vtr_log.h"
#include "stats.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "vpr_utils.h"

#include "globals.h"
#include "setup_grid.h"
#include "vtr_expr_eval.h"

#define MAX_SIZE_FACTOR 10000

using vtr::FormulaParser;
using vtr::t_formula_data;

static DeviceGrid auto_size_device_grid(const std::vector<t_grid_def>& grid_layouts, const std::map<t_logical_block_type_ptr, size_t>& minimum_instance_counts, float maximum_device_utilization);
static std::vector<t_logical_block_type_ptr> grid_overused_resources(const DeviceGrid& grid, std::map<t_logical_block_type_ptr, size_t> instance_counts);
static bool grid_satisfies_instance_counts(const DeviceGrid& grid, const std::map<t_logical_block_type_ptr, size_t>& instance_counts, float maximum_utilization);
static DeviceGrid build_device_grid(const t_grid_def& grid_def, size_t width, size_t height, bool warn_out_of_range = true, const std::vector<t_logical_block_type_ptr>& limiting_resources = std::vector<t_logical_block_type_ptr>());

///@brief Check if grid is valid
static void check_grid(const DeviceGrid& grid);

static void set_grid_block_type(int priority,
                                const t_physical_tile_type* type,
                                size_t layer_num,
                                size_t x_root,
                                size_t y_root,
                                vtr::NdMatrix<t_grid_tile, 3>& grid,
                                vtr::NdMatrix<int, 3>& grid_priorities,
                                const t_metadata_dict* meta);

///@brief Create the device grid based on resource requirements
DeviceGrid create_device_grid(const std::string& layout_name, const std::vector<t_grid_def>& grid_layouts, const std::map<t_logical_block_type_ptr, size_t>& minimum_instance_counts, float target_device_utilization) {
    if (layout_name == "auto") {
        //Auto-size the device
        //
        //Note that we treat the target device utilization as a maximum
        return auto_size_device_grid(grid_layouts, minimum_instance_counts, target_device_utilization);
    } else {
        //Use the specified device

        //Find the matching grid definition
        auto cmp = [&](const t_grid_def& grid_def) {
            return grid_def.name == layout_name;
        };

        auto iter = std::find_if(grid_layouts.begin(), grid_layouts.end(), cmp);
        if (iter == grid_layouts.end()) {
            //Not found
            std::string valid_names;
            for (size_t i = 0; i < grid_layouts.size(); ++i) {
                if (i != 0) {
                    valid_names += ", ";
                }
                valid_names += "'" + grid_layouts[i].name + "'";
            }
            VPR_FATAL_ERROR(VPR_ERROR_ARCH, "Failed to find grid layout named '%s' (valid grid layouts: %s)\n", layout_name.c_str(), valid_names.c_str());
        }

        return build_device_grid(*iter, iter->width, iter->height);
    }
}

///@brief Create the device grid based on dimensions
DeviceGrid create_device_grid(const std::string& layout_name, const std::vector<t_grid_def>& grid_layouts, size_t width, size_t height) {
    if (layout_name == "auto") {
        VTR_ASSERT(!grid_layouts.empty());
        //Auto-size
        if (grid_layouts[0].grid_type == e_grid_def_type::AUTO) {
            //Auto layout of the specified dimensions
            return build_device_grid(grid_layouts[0], width, height);
        } else {
            //Find the fixed layout close to the target size
            std::vector<const t_grid_def*> grid_layouts_view;
            grid_layouts_view.reserve(grid_layouts.size());
            for (const auto& layout : grid_layouts) {
                grid_layouts_view.push_back(&layout);
            }
            auto sort_cmp = [](const t_grid_def* lhs, const t_grid_def* rhs) {
                return lhs->width < rhs->width || lhs->height < rhs->width;
            };
            std::stable_sort(grid_layouts_view.begin(), grid_layouts_view.end(), sort_cmp);

            auto find_cmp = [&](const t_grid_def* grid_def) {
                return grid_def->width >= int(width) && grid_def->height >= int(height);
            };
            auto iter = std::find_if(grid_layouts_view.begin(), grid_layouts_view.end(), find_cmp);

            if (iter == grid_layouts_view.end()) {
                //No device larger than specified width/height, so choose largest possible
                VTR_LOG_WARN(
                    "Specified device dimensions (%zux%zu) exceed those of the largest fixed-size device."
                    " Using the largest fixed-size device\n",
                    width, height);
                --iter;
            }

            const t_grid_def* layout = *iter;

            return build_device_grid(*layout, layout->width, layout->height);
        }
    } else {
        // Use the specified device
        auto cmp = [&](const t_grid_def& grid_def) {
            return grid_def.name == layout_name;
        };

        auto iter = std::find_if(grid_layouts.begin(), grid_layouts.end(), cmp);
        if (iter == grid_layouts.end()) {
            //Not found
            std::string valid_names;
            for (size_t i = 0; i < grid_layouts.size(); ++i) {
                if (i != 0) {
                    valid_names += ", ";
                }
                valid_names += "'" + grid_layouts[i].name + "'";
            }
            VPR_FATAL_ERROR(VPR_ERROR_ARCH, "Failed to find grid layout named '%s' (valid grid layouts: %s)\n", layout_name.c_str(), valid_names.c_str());
        }

        return build_device_grid(*iter, iter->width, iter->height);
    }
}

/**
 * @brief Create a device grid which satisfies the minimum block counts
 *
 * If a set of fixed grid layouts are specified, the smallest satisfying grid is picked
 * If an auto grid layouts are specified, the smallest dynamicly sized grid is picked
 */
static DeviceGrid auto_size_device_grid(const std::vector<t_grid_def>& grid_layouts, const std::map<t_logical_block_type_ptr, size_t>& minimum_instance_counts, float maximum_device_utilization) {
    VTR_ASSERT(!grid_layouts.empty());

    DeviceGrid grid;

    auto is_auto_grid_def = [](const t_grid_def& grid_def) {
        return grid_def.grid_type == e_grid_def_type::AUTO;
    };

    auto auto_layout_itr = std::ranges::find_if(grid_layouts, is_auto_grid_def);
    if (auto_layout_itr != grid_layouts.end()) {
        //Automatic grid layout, find the smallest height/width
        VTR_ASSERT_SAFE_MSG(std::find_if(auto_layout_itr + 1, grid_layouts.end(), is_auto_grid_def) == grid_layouts.end(), "Only one <auto_layout>");

        //Determine maximum device size to try before concluding that the circuit cannot fit on any device
        //Calculate total number of required instances
        //Then multiply by a factor of MAX_SIZE_FACTOR as overhead
        //This is to avoid infinite loop if increasing the grid size never gets you more of the instance
        //type you need and hence never lets you fit the design
        size_t max_size;
        size_t total_minimum_instance_counts = 0;
        for (auto& inst : minimum_instance_counts) {
            size_t count = inst.second;
            total_minimum_instance_counts += count;
        }
        max_size = total_minimum_instance_counts * MAX_SIZE_FACTOR;

        const auto& grid_def = *auto_layout_itr;
        VTR_ASSERT(grid_def.aspect_ratio >= 0.);

        //Initial size is num_layers x 3 x 3, the smallest possible while avoiding
        //start before end location issues with <perimeter> location
        //specifications
        size_t width = 3;
        size_t height = 3;
        std::vector<t_logical_block_type_ptr> limiting_resources;
        size_t grid_size = 0;
        do {
            //Scale opposite dimension to match aspect ratio
            height = vtr::nint(width / grid_def.aspect_ratio);

#ifdef VERBOSE
            VTR_LOG("Grid size: %zu x %zu (AR: %.2f) \n", width, height, float(width) / height);
#endif

            //Build the device
            // Don't warn about out-of-range specifications since these can
            // occur (harmlessly) at small device dimensions
            grid = build_device_grid(grid_def, width, height, false, limiting_resources);

            //Check if it satisfies the block counts
            if (grid_satisfies_instance_counts(grid, minimum_instance_counts, maximum_device_utilization)) {
                //Re-build the grid at the final size with out-of-range
                grid = build_device_grid(grid_def, width, height, false, limiting_resources);
                return grid;
            }

            limiting_resources = grid_overused_resources(grid, minimum_instance_counts);

            //Determine grid size
            grid_size = width * height;

            //Increase the grid size
            width++;

        } while (grid_size < max_size);

        //Maximum device size reached
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Device auto-fit aborted: device size already exceeds required resources count by %d times yet still cannot fit the design. "
                        "This may be due to resources that do not grow as the grid size increases (e.g. PLLs in the Titan Stratix IV architecture capture).\n",
                        MAX_SIZE_FACTOR);

    } else {
        VTR_ASSERT(auto_layout_itr == grid_layouts.end());
        //Fixed grid layouts, find the smallest of the fixed layouts

        //Sort the grid layouts from smallest to largest
        std::vector<const t_grid_def*> grid_layouts_view;
        grid_layouts_view.reserve(grid_layouts.size());
        for (const auto& layout : grid_layouts) {
            grid_layouts_view.push_back(&layout);
        }
        auto area_cmp = [](const t_grid_def* lhs, const t_grid_def* rhs) {
            VTR_ASSERT(lhs->grid_type == e_grid_def_type::FIXED);
            VTR_ASSERT(rhs->grid_type == e_grid_def_type::FIXED);

            int lhs_area = lhs->width * lhs->height;
            int rhs_area = rhs->width * rhs->height;

            return lhs_area < rhs_area;
        };
        std::stable_sort(grid_layouts_view.begin(), grid_layouts_view.end(), area_cmp);

        std::vector<t_logical_block_type_ptr> limiting_resources;

        //Try all the fixed devices in order from smallest to largest
        for (const auto* grid_def : grid_layouts_view) {
            //Build the grid
            grid = build_device_grid(*grid_def, grid_def->width, grid_def->height, true, limiting_resources);

            if (grid_satisfies_instance_counts(grid, minimum_instance_counts, maximum_device_utilization)) {
                return grid;
            }
            limiting_resources = grid_overused_resources(grid, minimum_instance_counts);
        }
    }

    return grid; //Unreachable
}

/**
 * @brief Estimates what logical block types will be unimplementable due to resource
 *        limits in the available grid
 *
 * Performs a fast counting based estimate, allocating the least
 * flexible block types (those with the fewestequivalent tiles) first.
 */
static std::vector<t_logical_block_type_ptr> grid_overused_resources(const DeviceGrid& grid, std::map<t_logical_block_type_ptr, size_t> instance_counts) {
    auto& device_ctx = g_vpr_ctx.device();

    std::vector<t_logical_block_type_ptr> overused_resources;

    std::unordered_map<t_physical_tile_type_ptr, size_t> min_count_map;
    // Initialize min_count_map
    for (const auto& tile_type : device_ctx.physical_tile_types) {
        min_count_map.insert(std::make_pair(&tile_type, size_t(0)));
    }

    //Initialize available tile counts
    std::unordered_map<t_physical_tile_type_ptr, int> avail_tiles;
    for (auto& tile_type : device_ctx.physical_tile_types) {
        avail_tiles[&tile_type] = grid.num_instances(&tile_type, -1);
    }

    //Sort so we allocate logical blocks with the fewest equivalent sites first (least flexible)
    std::vector<const t_logical_block_type*> logical_block_types;
    logical_block_types.reserve(device_ctx.logical_block_types.size());
    for (auto& block_type : device_ctx.logical_block_types) {
        logical_block_types.push_back(&block_type);
    }

    auto by_ascending_equiv_tiles = [](t_logical_block_type_ptr lhs, t_logical_block_type_ptr rhs) {
        return lhs->equivalent_tiles.size() < rhs->equivalent_tiles.size();
    };
    std::stable_sort(logical_block_types.begin(), logical_block_types.end(), by_ascending_equiv_tiles);

    //Allocate logical blocks to available tiles
    for (auto block_type : logical_block_types) {
        if (instance_counts.count(block_type)) {
            int required_blocks = instance_counts[block_type];

            for (auto tile_type : block_type->equivalent_tiles) {
                if (avail_tiles[tile_type] >= required_blocks) {
                    avail_tiles[tile_type] -= required_blocks;
                    required_blocks = 0;
                } else {
                    required_blocks -= avail_tiles[tile_type];
                    avail_tiles[tile_type] = 0;
                }

                if (required_blocks == 0) break;
            }

            if (required_blocks > 0) {
                overused_resources.push_back(block_type);
            }
        }
    }

    return overused_resources;
}

static bool grid_satisfies_instance_counts(const DeviceGrid& grid, const std::map<t_logical_block_type_ptr, size_t>& instance_counts, float maximum_utilization) {
    //Are the resources satisfied?
    auto overused_resources = grid_overused_resources(grid, instance_counts);

    if (!overused_resources.empty()) {
        return false;
    }

    //Is the utilization below the maximum?
    float utilization = calculate_device_utilization(grid, instance_counts);

    if (utilization > maximum_utilization) {
        return false;
    }

    return true; //OK
}

///@brief Build the specified device grid
static DeviceGrid build_device_grid(const t_grid_def& grid_def, size_t grid_width, size_t grid_height, bool warn_out_of_range, const std::vector<t_logical_block_type_ptr>& limiting_resources) {
    if (grid_def.grid_type == e_grid_def_type::FIXED) {
        if (grid_def.width != int(grid_width) || grid_def.height != int(grid_height)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Requested grid size (%zu%zu) does not match fixed device size (%dx%d)",
                            grid_width, grid_height, grid_def.width, grid_def.height);
        }
    }

    const DeviceContext& device_ctx = g_vpr_ctx.device();

    // Initialize the grid and each location priority based on available dies in the architecture file
    vtr::NdMatrix<t_grid_tile, 3> grid;
    vtr::NdMatrix<int, 3> grid_priorities;
    size_t num_layers = grid_def.layers.size();
    grid.resize({num_layers, grid_width, grid_height});
    //Track the current priority for each grid location
    // Note that we initialize it to the lowest (i.e. most negative) possible value, so
    // any user-specified priority will override the default empty grid
    grid_priorities.resize({num_layers, grid_width, grid_height}, std::numeric_limits<int>::lowest());

    // Initialize the device to all empty blocks
    t_physical_tile_type_ptr empty_type = device_ctx.EMPTY_PHYSICAL_TILE_TYPE;
    VTR_ASSERT(empty_type != nullptr);
    for (size_t layer = 0; layer < num_layers; ++layer) {
        for (size_t x = 0; x < grid_width; ++x) {
            for (size_t y = 0; y < grid_height; ++y) {
                set_grid_block_type(std::numeric_limits<int>::lowest() + 1, //+1 so it overrides without warning
                                    empty_type,
                                    layer, x, y,
                                    grid, grid_priorities,
                                    /*meta=*/nullptr);
            }
        }
    }

    FormulaParser p;
    std::set<t_physical_tile_type_ptr> seen_types;
    std::vector<std::vector<int>> horizontal_interposer_cuts(num_layers);
    std::vector<std::vector<int>> vertical_interposer_cuts(num_layers);

    for (size_t layer = 0; layer < num_layers; layer++) {
        for (const t_layer_def& layer_def : grid_def.layers[layer]) {

            for (const t_interposer_cut_inf& cut_inf : layer_def.interposer_cuts) {
                if (cut_inf.dim == e_interposer_cut_dim::X) {
                    vertical_interposer_cuts[layer].push_back(cut_inf.loc);
                } else {
                    VTR_ASSERT(cut_inf.dim == e_interposer_cut_dim::Y);
                    horizontal_interposer_cuts[layer].push_back(cut_inf.loc);
                }
            }


            for (const t_grid_loc_def& grid_loc_def : layer_def.loc_defs) {
                // Fill in the block types according to the specification
                t_physical_tile_type_ptr type = find_tile_type_by_name(grid_loc_def.block_type, device_ctx.physical_tile_types);

                if (!type) {
                    VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                    "Failed to find block type '%s' for grid location specification",
                                    grid_loc_def.block_type.c_str());
                }

                seen_types.insert(type);

                t_formula_data vars;
                vars.set_var_value("W", grid_width);
                vars.set_var_value("H", grid_height);
                vars.set_var_value("w", type->width);
                vars.set_var_value("h", type->height);

                //Load the x specification
                const t_grid_loc_spec& xspec = grid_loc_def.x;

                VTR_ASSERT_MSG(!xspec.start_expr.empty(), "x start position must be specified");
                VTR_ASSERT_MSG(!xspec.end_expr.empty(), "x end position must be specified");
                VTR_ASSERT_MSG(!xspec.incr_expr.empty(), "x increment must be specified");
                VTR_ASSERT_MSG(!xspec.repeat_expr.empty(), "x repeat must be specified");

                size_t startx = p.parse_formula(xspec.start_expr, vars);
                size_t endx = p.parse_formula(xspec.end_expr, vars);
                size_t incrx = p.parse_formula(xspec.incr_expr, vars);
                size_t repeatx = p.parse_formula(xspec.repeat_expr, vars);

                // Load the y specification
                const t_grid_loc_spec& yspec = grid_loc_def.y;

                VTR_ASSERT_MSG(!yspec.start_expr.empty(), "y start position must be specified");
                VTR_ASSERT_MSG(!yspec.end_expr.empty(), "y end position must be specified");
                VTR_ASSERT_MSG(!yspec.incr_expr.empty(), "y increment must be specified");
                VTR_ASSERT_MSG(!yspec.repeat_expr.empty(), "y repeat must be specified");

                size_t starty = p.parse_formula(yspec.start_expr, vars);
                size_t endy = p.parse_formula(yspec.end_expr, vars);
                size_t incry = p.parse_formula(yspec.incr_expr, vars);
                size_t repeaty = p.parse_formula(yspec.repeat_expr, vars);

                // Check start against the device dimensions
                // Start locations outside the device will never create block instances
                if (startx > grid_width - 1) {
                    if (warn_out_of_range) {
                        VTR_LOG_WARN("Block type '%s' grid location specification startx (%s = %d) falls outside device horizontal range [%d,%d]\n",
                                     type->name.c_str(), xspec.start_expr.c_str(), startx, 0, grid_width - 1);
                    }
                    continue; //No instances will be created
                }

                if (starty > grid_height - 1) {
                    if (warn_out_of_range) {
                        VTR_LOG_WARN("Block type '%s' grid location specification starty (%s = %d) falls outside device vertical range [%d,%d]\n",
                                     type->name.c_str(), yspec.start_expr.c_str(), starty, 0, grid_height - 1);
                    }
                    continue; //No instances will be created
                }

                // Check end against the device dimensions
                if (endx > grid_width - 1) {
                    if (warn_out_of_range) {
                        VTR_LOG_WARN("Block type '%s' grid location specification endx (%s = %d) falls outside device horizontal range [%d,%d]\n",
                                     type->name.c_str(), xspec.end_expr.c_str(), endx, 0, grid_width - 1);
                    }
                }

                if (endy > grid_height - 1) {
                    if (warn_out_of_range) {
                        VTR_LOG_WARN("Block type '%s' grid location specification endy (%s = %d) falls outside device vertical range [%d,%d]\n",
                                     type->name.c_str(), yspec.end_expr.c_str(), endy, 0, grid_height - 1);
                    }
                }

                // The end must fall after (or equal) to the start
                if (endx < startx) {
                    VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                    "Grid location specification endx (%s = %d) can not come before startx (%s = %d) for block type '%s'",
                                    xspec.end_expr.c_str(), endx, xspec.start_expr.c_str(), startx, type->name.c_str());
                }

                if (endy < starty) {
                    VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                    "Grid location specification endy (%s = %d) can not come before starty (%s = %d) for block type '%s'",
                                    yspec.end_expr.c_str(), endy, yspec.start_expr.c_str(), starty, type->name.c_str());
                }

                // The minimum increment is the block dimension
                VTR_ASSERT(type->width > 0);
                if (incrx < size_t(type->width)) {
                    VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                    "Grid location specification incrx for block type '%s' must be at least"
                                    " block width (%d) to avoid overlapping instances (was %s = %d)",
                                    type->name.c_str(), type->width, xspec.incr_expr.c_str(), incrx);
                }

                VTR_ASSERT(type->height > 0);
                if (incry < size_t(type->height)) {
                    VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                    "Grid location specification incry for block type '%s' must be at least"
                                    " block height (%d) to avoid overlapping instances (was %s = %d)",
                                    type->name.c_str(), type->height, yspec.incr_expr.c_str(), incry);
                }

                // The minimum repeat is the region dimension
                size_t region_width = endx - startx + 1; //+1 since start/end are both inclusive
                if (repeatx < region_width) {
                    VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                    "Grid location specification repeatx for block type '%s' must be at least"
                                    " the region width (%d) to avoid overlapping instances (was %s = %d)",
                                    type->name.c_str(), region_width, xspec.repeat_expr.c_str(), repeatx);
                }

                size_t region_height = endy - starty + 1; //+1 since start/end are both inclusive
                if (repeaty < region_height) {
                    VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                    "Grid location specification repeaty for block type '%s' must be at least"
                                    " the region height (%d) to avoid overlapping instances (was %s = %d)",
                                    type->name.c_str(), region_height, xspec.repeat_expr.c_str(), repeaty);
                }

                size_t x_end = 0;
                for (size_t kx = 0; x_end < grid_width; ++kx) {
                    // Repeat in x direction
                    size_t x_start = startx + kx * repeatx;
                    x_end = endx + kx * repeatx;

                    size_t y_end = 0;
                    for (size_t ky = 0; y_end < grid_height; ++ky) { // Repeat in y direction
                        size_t y_start = starty + ky * repeaty;
                        y_end = endy + ky * repeaty;

                        size_t x_max = std::min(x_end, grid_width - 1);
                        size_t y_max = std::min(y_end, grid_height - 1);

                        // Fill in the region
                        for (size_t x = x_start; x + (type->width - 1) <= x_max; x += incrx) {
                            for (size_t y = y_start; y + (type->height - 1) <= y_max; y += incry) {
                                set_grid_block_type(grid_loc_def.priority,
                                                    type,
                                                    layer, x, y,
                                                    grid, grid_priorities,
                                                    grid_loc_def.meta);
                            }
                        }
                    }
                }
            }
        }
    }

    // Warn if any types were not specified in the grid layout
    for (const t_physical_tile_type& type : device_ctx.physical_tile_types) {
        if (&type == empty_type) continue; // Don't worry if empty hasn't been specified

        if (!seen_types.count(&type)) {
            VTR_LOG_WARN("Block type '%s' was not specified in device grid layout\n",
                         type.name.c_str());
        }
    }

    DeviceGrid device_grid(grid_def.name,
                        grid,
                        limiting_resources,
                        std::move(horizontal_interposer_cuts),
                        std::move(vertical_interposer_cuts));

    check_grid(device_grid);

    return device_grid;
}

static void set_grid_block_type(int priority,
                                const t_physical_tile_type* type,
                                size_t layer_num,
                                size_t x_root,
                                size_t y_root,
                                vtr::NdMatrix<t_grid_tile, 3>& grid,
                                vtr::NdMatrix<int, 3>& grid_priorities,
                                const t_metadata_dict* meta) {
    struct TypeLocation {
        TypeLocation(size_t x_val, size_t y_val, const t_physical_tile_type* type_val, int priority_val)
            : x(x_val)
            , y(y_val)
            , type(type_val)
            , priority(priority_val) {}
        size_t x;
        size_t y;
        const t_physical_tile_type* type;
        int priority;

        bool operator<(const TypeLocation& rhs) const {
            return x < rhs.x || y < rhs.y || type < rhs.type;
        }
    };

    //Collect locations effected by this block
    std::set<TypeLocation> target_locations;
    for (size_t x = x_root; x < x_root + type->width; ++x) {
        for (size_t y = y_root; y < y_root + type->height; ++y) {
            target_locations.insert(TypeLocation(x, y, grid[layer_num][x][y].type, grid_priorities[layer_num][x][y]));
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
            max_priority_type_loc.type->name.c_str(), max_priority_type_loc.x, max_priority_type_loc.y,
            priority, type->name.c_str());
    }

    // Mark all the grid tiles 'covered' by this block with the appropriate type
    // and width/height offsets
    std::set<TypeLocation> root_blocks_to_rip_up;
    auto& device_ctx = g_vpr_ctx.device();
    for (size_t x = x_root; x < x_root + type->width; ++x) {
        VTR_ASSERT(x < grid.end_index(1));

        size_t x_offset = x - x_root;
        for (size_t y = y_root; y < y_root + type->height; ++y) {
            VTR_ASSERT(y < grid.end_index(2));
            size_t y_offset = y - y_root;

            auto& grid_tile = grid[layer_num][x][y];
            VTR_ASSERT(grid_priorities[layer_num][x][y] <= priority);

            if (grid_tile.type != nullptr
                && grid_tile.type != device_ctx.EMPTY_PHYSICAL_TILE_TYPE) {
                //We are overriding a non-empty block, we need to be careful
                //to ensure we remove any blocks which will be invalidated when we
                //overwrite part of their locations

                size_t orig_root_x = x - grid[layer_num][x][y].width_offset;
                size_t orig_root_y = y - grid[layer_num][x][y].height_offset;

                root_blocks_to_rip_up.insert(TypeLocation(orig_root_x,
                                                          orig_root_y,
                                                          grid[layer_num][x][y].type,
                                                          grid_priorities[layer_num][x][y]));
            }

            grid[layer_num][x][y].type = type;
            grid[layer_num][x][y].width_offset = x_offset;
            grid[layer_num][x][y].height_offset = y_offset;
            grid[layer_num][x][y].meta = meta;

            grid_priorities[layer_num][x][y] = priority;
        }
    }

    // Rip-up any invalidated blocks
    for (auto invalidated_root : root_blocks_to_rip_up) {
        // Mark all the grid locations used by this root block as empty
        for (size_t x = invalidated_root.x; x < invalidated_root.x + invalidated_root.type->width; ++x) {
            int x_offset = x - invalidated_root.x;
            for (size_t y = invalidated_root.y; y < invalidated_root.y + invalidated_root.type->height; ++y) {
                int y_offset = y - invalidated_root.y;

                if (grid[layer_num][x][y].type == invalidated_root.type
                    && grid[layer_num][x][y].width_offset == x_offset
                    && grid[layer_num][x][y].height_offset == y_offset) {
                    //This is a left-over invalidated block, mark as empty
                    // Note: that we explicitly check the type and offsets, since the original block
                    //       may have been completely overwritten, and we don't want to change anything
                    //       in that case
                    VTR_ASSERT(device_ctx.EMPTY_PHYSICAL_TILE_TYPE->width == 1);
                    VTR_ASSERT(device_ctx.EMPTY_PHYSICAL_TILE_TYPE->height == 1);

#ifdef VERBOSE
                    VTR_LOG("Ripping up block '%s' at (%d,%d) offset (%d,%d). Overlapped by '%s' at (%d,%d)\n",
                            invalidated_root.type->name, invalidated_root.x, invalidated_root.y,
                            x_offset, y_offset,
                            type->name, x_root, y_root);
#endif

                    grid[layer_num][x][y].type = device_ctx.EMPTY_PHYSICAL_TILE_TYPE;
                    grid[layer_num][x][y].width_offset = 0;
                    grid[layer_num][x][y].height_offset = 0;

                    grid_priorities[layer_num][x][y] = std::numeric_limits<int>::lowest();
                }
            }
        }
    }
}

static void check_grid(const DeviceGrid& grid) {
    for (const t_physical_tile_loc tile_loc : grid.all_locations()) {
        const t_physical_tile_type_ptr type = grid.get_physical_type(tile_loc);
        int width_offset = grid.get_width_offset(tile_loc);
        int height_offset = grid.get_height_offset(tile_loc);
        if (nullptr == type) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Grid Location (%d,%d,%d) has no type.\n", tile_loc.layer_num, tile_loc.x, tile_loc.y);
        }

        if ((grid.get_width_offset(tile_loc) < 0) || (grid.get_width_offset(tile_loc) >= type->width)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Grid Location (%d,%d,%d) has invalid width offset (%d).\n",
                            tile_loc.layer_num, tile_loc.x, tile_loc.y,
                            width_offset);
        }
        if ((grid.get_height_offset(tile_loc) < 0) || (grid.get_height_offset(tile_loc) >= type->height)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Grid Location (%d,%d,%d) has invalid height offset (%d).\n",
                            tile_loc.layer_num, tile_loc.x, tile_loc.y,
                            height_offset);
        }

        // Verify that type and width/height offsets are correct (e.g. for dimension > 1 blocks)
        if (grid.get_width_offset(tile_loc) == 0 && grid.get_height_offset(tile_loc) == 0) {
            // From the root block check that all other blocks are correct
            for (int x = tile_loc.x; x < tile_loc.x + type->width; ++x) {
                int x_offset = x - tile_loc.x;
                for (int y = tile_loc.y; y < tile_loc.y + type->height; ++y) {
                    int y_offset = y - tile_loc.y;
                    const t_physical_tile_loc tile_loc_offset(x, y, tile_loc.layer_num);
                    const auto& tile_type = grid.get_physical_type(tile_loc_offset);
                    int tile_width_offset = grid.get_width_offset(tile_loc_offset);
                    int tile_height_offset = grid.get_height_offset(tile_loc_offset);
                    if (tile_type != type) {
                        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                                        "Grid Location (%d,%d,%d) should have type '%s' (based on root location) but has type '%s'\n",
                                        tile_loc.layer_num, tile_loc.x, tile_loc.y, type->name.c_str(), type->name.c_str());
                    }

                    if (tile_width_offset != x_offset) {
                        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                                        "Grid Location (%d,%d,%d) of type '%s' should have width offset '%d' (based on root location) but has '%d'\n",
                                        tile_loc.layer_num, tile_loc.x, tile_loc.y, type->name.c_str(), x_offset, tile_width_offset);
                    }

                    if (tile_height_offset != y_offset) {
                        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                                        "Grid Location (%d,%d,%d)  of type '%s' should have height offset '%d' (based on root location) but has '%d'\n",
                                        tile_loc.layer_num, tile_loc.x, tile_loc.y, type->name.c_str(), y_offset, tile_height_offset);
                    }
                }
            }
        }
    }
}

size_t count_grid_tiles(const DeviceGrid& grid) {
    return grid.get_num_layers() * grid.width() * grid.height();
}
