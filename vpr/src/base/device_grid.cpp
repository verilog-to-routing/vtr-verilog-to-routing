#include "device_grid.h"

DeviceGrid::DeviceGrid(std::string grid_name, vtr::Matrix<t_grid_tile> grid)
    : name_(grid_name)
    , grid_(grid) {
    count_instances();
}

DeviceGrid::DeviceGrid(std::string grid_name, vtr::Matrix<t_grid_tile> grid, std::vector<t_type_ptr> limiting_res)
    : DeviceGrid(grid_name, grid) {
    limiting_resources_ = limiting_res;
}

size_t DeviceGrid::num_instances(t_type_ptr type) const {
    auto iter = instance_counts_.find(type);
    if (iter != instance_counts_.end()) {
        //Return count
        return iter->second;
    }
    return 0; //None found
}

void DeviceGrid::clear() {
    grid_.clear();
    instance_counts_.clear();
}

void DeviceGrid::count_instances() {
    instance_counts_.clear();

    //Count the number of blocks in the grid
    for (size_t x = 0; x < width(); ++x) {
        for (size_t y = 0; y < height(); ++y) {
            auto type = grid_[x][y].type;

            if (grid_[x][y].width_offset == 0 && grid_[x][y].height_offset == 0) {
                //Add capacity only if this is the root location
                instance_counts_[type] += type->capacity;
            }
        }
    }
}
