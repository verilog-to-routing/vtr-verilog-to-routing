#include "device_grid.h"

#include <cstddef>
#include <utility>
#include <vector>
#include "physical_types.h"
#include "vtr_expr_eval.h"
#include "vtr_ndmatrix.h"
#include "grid_util.h"

DeviceGrid::DeviceGrid(const t_grid_def& grid_def,
                       vtr::NdMatrix<t_grid_tile, 3> grid)
    : name_(grid_def.name)
    , grid_(std::move(grid)) {
    const size_t num_layers = grid_.dim_size(0);

    vtr::FormulaParser p;
    std::tie(horizontal_interposer_cuts_, vertical_interposer_cuts_) = resolve_interposer_cut_locations(*this, grid_def, p);

    count_instances();

    initialize_multi_die_data_structures();
}

DeviceGrid::DeviceGrid(const t_grid_def& grid_def,
                       vtr::NdMatrix<t_grid_tile, 3> grid,
                       std::vector<t_logical_block_type_ptr> limiting_res)
    : DeviceGrid(grid_def, std::move(grid)) {
    limiting_resources_ = std::move(limiting_res);
}

size_t DeviceGrid::num_instances(t_physical_tile_type_ptr type, int layer_num) const {
    size_t count = 0;
    //instance_counts_ is not initialized
    if (instance_counts_.empty()) {
        return 0;
    }

    int num_layers = (int)grid_.dim_size(0);

    if (layer_num == -1) {
        //Count all layers
        for (int curr_layer_num = 0; curr_layer_num < num_layers; ++curr_layer_num) {
            auto iter = instance_counts_[curr_layer_num].find(type);
            if (iter != instance_counts_[curr_layer_num].end()) {
                count += iter->second;
            }
        }
        return count;
    } else {
        auto iter = instance_counts_[layer_num].find(type);
        if (iter != instance_counts_[layer_num].end()) {
            //Return count
            count = iter->second;
        }
    }

    return count;
}

void DeviceGrid::clear() {
    grid_.clear();
    instance_counts_.clear();
}

void DeviceGrid::count_instances() {
    int num_layers = (int)grid_.dim_size(0);
    instance_counts_.clear();
    instance_counts_.resize(num_layers);

    //Initialize the instance counts
    for (int layer_num = 0; layer_num < num_layers; ++layer_num) {
        for (size_t x = 0; x < width(); ++x) {
            for (size_t y = 0; y < height(); ++y) {
                auto type = grid_[layer_num][x][y].type;

                if (grid_[layer_num][x][y].width_offset == 0 && grid_[layer_num][x][y].height_offset == 0) {
                    //Add capacity only if this is the root location
                    instance_counts_[layer_num][type] = 0;
                }
            }
        }
    }

    //Count the number of blocks in the grid
    for (int layer_num = 0; layer_num < num_layers; ++layer_num) {
        for (size_t x = 0; x < width(); ++x) {
            for (size_t y = 0; y < height(); ++y) {
                auto type = grid_[layer_num][x][y].type;

                if (grid_[layer_num][x][y].width_offset == 0 && grid_[layer_num][x][y].height_offset == 0) {
                    //Add capacity only if this is the root location
                    instance_counts_[layer_num][type] += type->capacity;
                }
            }
        }
    }
}

void DeviceGrid::initialize_multi_die_data_structures() {
    const size_t num_layers = grid_.dim_size(0);
    const size_t device_width = grid_.dim_size(1);
    const size_t device_height = grid_.dim_size(2);

    // Build the unique Ids for each die. In 2D architectures there's only a single die but this is not the case in 2.5D and 3D architectures.
    short die_region_counter = 0;
    for (size_t layer = 0; layer < num_layers; layer++) {
        const std::vector<int>& horizontal_interposers = horizontal_interposer_cuts_[layer];
        const std::vector<int>& vertical_interposers = vertical_interposer_cuts_[layer];

        vtr::NdMatrix<DeviceDieId, 2> layer_reduced_die_id_matrix({vertical_interposers.size() + 1, horizontal_interposers.size() + 1});

        for (size_t i = 0; i < vertical_interposers.size() + 1; i++) {
            for (size_t j = 0; j < horizontal_interposers.size() + 1; j++) {
                DeviceDieId die_id = (DeviceDieId)die_region_counter;
                layer_reduced_die_id_matrix[i][j] = die_id;

                t_die_region region = {.x_die = static_cast<short>(i), .y_die = static_cast<short>(j), .layer = static_cast<short>(layer)};
                die_region_map_.update(die_id, region);

                die_region_counter++;
            }
        }
        vtr::NdMatrix<DeviceDieId, 2> layer_die_id_matrix = get_device_sized_matrix_from_reduced(device_width,
                                                                                                 device_height,
                                                                                                 horizontal_interposers,
                                                                                                 vertical_interposers,
                                                                                                 layer_reduced_die_id_matrix);
        die_id_matrix_.push_back(std::move(layer_die_id_matrix));
    }
}
        if (!layer_h_cuts.empty()) {
            return true;
        }
    }

    for (const std::vector<int>& layer_v_cuts : vertical_interposer_cuts_) {
        if (!layer_v_cuts.empty()) {
            return true;
        }
    }

    return false;
}

bool DeviceGrid::are_locs_on_same_die(t_physical_tile_loc loc_a, t_physical_tile_loc loc_b) const {
    const DeviceDieId first_id = die_id_matrix_[loc_a.layer_num][loc_a.x][loc_a.y];
    const DeviceDieId second_id = die_id_matrix_[loc_b.layer_num][loc_b.x][loc_b.y];

    return first_id == second_id;
}

DeviceDieId DeviceGrid::get_loc_die_id(t_physical_tile_loc loc) const {
    return die_id_matrix_[loc.layer_num][loc.x][loc.y];
}

DeviceDieId DeviceGrid::get_die_region_id(t_die_region die_region) const {
    return die_region_map_[die_region];
}

size_t DeviceGrid::get_die_count() const {
    size_t die_count = 0;
    for (size_t layer_num = 0; layer_num < get_num_layers(); layer_num++) {
        die_count += (get_vertical_interposer_cuts()[layer_num].size() + 1) * (get_horizontal_interposer_cuts()[layer_num].size() + 1);
    }

    return die_count;
}
