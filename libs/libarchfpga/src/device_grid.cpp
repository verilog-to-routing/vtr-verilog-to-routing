#include "device_grid.h"

struct gridDimComp {
    // The dimension to compare
    int dim_;
    // Whether to compare in ascending or descending order
    bool is_greater_;
    gridDimComp(int dim, bool is_greater)
        : dim_(dim)
        , is_greater_(is_greater) {}

    bool operator()(const vtr::Matrix<t_grid_tile>& lhs, const vtr::Matrix<t_grid_tile>& rhs) {
        return is_greater_ ? lhs.dim_size(dim_) > rhs.dim_size(dim_) : lhs.dim_size(dim_) < rhs.dim_size(dim_);
    }
};

DeviceGrid::DeviceGrid(std::string grid_name, std::vector<vtr::Matrix<t_grid_tile>> grid)
    : name_(grid_name)
    , grid_(grid) {
    num_layers_ = grid_.size();
    count_instances();
}

DeviceGrid::DeviceGrid(std::string grid_name, std::vector<vtr::Matrix<t_grid_tile>> grid, std::vector<t_logical_block_type_ptr> limiting_res)
    : DeviceGrid(grid_name, grid) {
    num_layers_ = grid_.size();
    limiting_resources_ = limiting_res;
}

size_t DeviceGrid::num_instances(t_physical_tile_type_ptr type, int layer_num) const {
    size_t count = 0;
    if (instance_counts_.size() == 0) {
        //No instances counted
        return count;
    }

    if (layer_num == -1) {
        //Count all layers
        for (int curr_layer_num = 0; curr_layer_num < num_layers_; ++curr_layer_num) {
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
    num_layers_ = 1;
}

void DeviceGrid::count_instances() {
    instance_counts_.clear();
    instance_counts_.resize(num_layers_);

    //Count the number of blocks in the grid
    for (int layer_num = 0; layer_num < num_layers_; ++layer_num) {
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
