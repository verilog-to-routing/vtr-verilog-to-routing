#include "device_grid.h"

#include <cstddef>
#include <utility>
#include <vector>
#include "physical_types.h"
#include "vtr_log.h"
#include "vtr_ndmatrix.h"
#include "grid_util.h"

DeviceGrid::DeviceGrid(std::string_view grid_name,
                       vtr::NdMatrix<t_grid_tile, 3> grid,
                       std::vector<std::vector<int>>&& horizontal_interposer_cuts,
                       std::vector<std::vector<int>>&& vertical_interposer_cuts)
    : name_(grid_name)
    , grid_(std::move(grid))
    , horizontal_interposer_cuts_(std::move(horizontal_interposer_cuts))
    , vertical_interposer_cuts_(std::move(vertical_interposer_cuts)) {
    count_instances();

    const size_t num_layers = grid_.dim_size(0);
    const size_t x_size = grid_.dim_size(1);
    const size_t y_size = grid_.dim_size(2);

    // Build the unique Ids for each die. In 2D architectures there's only a single die but this is not the case in 2.5D and 3D architectures.
    short die_region_counter = 0;
    for (size_t layer = 0; layer < num_layers; layer++) {
        const std::vector<int>& horizontal_interposers = horizontal_interposer_cuts_[layer];
        const std::vector<int>& vertical_interposers = vertical_interposer_cuts_[layer];

        vtr::NdMatrix<DeviceDieId, 2> layer_reduced_die_id_matrix({vertical_interposers.size() + 1, horizontal_interposers.size() + 1});

        for (size_t i = 0; i < vertical_interposers.size() + 1; i++) {
            for (size_t j = 0; j < horizontal_interposers.size() + 1; j++) {
                layer_reduced_die_id_matrix[i][j] = (DeviceDieId)die_region_counter;
                die_region_counter++;
            }
        }
        vtr::NdMatrix<DeviceDieId, 2> layer_die_id_matrix = get_device_sized_matrix_from_reduced(x_size,
                                                                                                 y_size,
                                                                                                 horizontal_interposers,
                                                                                                 vertical_interposers,
                                                                                                 layer_reduced_die_id_matrix);
        die_id_matrix_.push_back(std::move(layer_die_id_matrix));
    }
}

DeviceGrid::DeviceGrid(std::string_view grid_name,
                       vtr::NdMatrix<t_grid_tile, 3> grid,
                       std::vector<t_logical_block_type_ptr> limiting_res,
                       std::vector<std::vector<int>>&& horizontal_interposer_cuts,
                       std::vector<std::vector<int>>&& vertical_interposer_cuts)
    : DeviceGrid(grid_name, std::move(grid), std::move(horizontal_interposer_cuts), std::move(vertical_interposer_cuts)) {
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

bool DeviceGrid::has_interposer_cuts() const {
    for (const std::vector<int>& layer_h_cuts : horizontal_interposer_cuts_) {
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
