#include "device_grid.h"

#include <utility>

DeviceGrid::DeviceGrid(std::string_view grid_name,
                       vtr::NdMatrix<t_grid_tile, 3> grid,
                       std::vector<std::vector<int>>&& horizontal_interposer_cuts,
                       std::vector<std::vector<int>>&& vertical_interposer_cuts)
    : name_(grid_name)
    , grid_(std::move(grid))
    , horizontal_interposer_cuts_(std::move(horizontal_interposer_cuts))
    , vertical_interposer_cuts_(std::move(vertical_interposer_cuts)) {
    count_instances();
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
