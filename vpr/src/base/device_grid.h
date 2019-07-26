#ifndef DEVICE_GRID
#define DEVICE_GRID

#include "vtr_ndmatrix.h"
#include "vpr_types.h"

class DeviceGrid {
  public:
    DeviceGrid() = default;
    DeviceGrid(std::string grid_name, vtr::Matrix<t_grid_tile> grid);
    DeviceGrid(std::string grid_name, vtr::Matrix<t_grid_tile> grid, std::vector<t_type_ptr> limiting_res);

    const std::string& name() const { return name_; }

    size_t width() const { return grid_.dim_size(0); }
    size_t height() const { return grid_.dim_size(1); }

    //Note: supports 2-d indexing [0..width()-1][0..height()-1] yielding a t_grid_tile
    auto operator[](size_t index) const { return grid_[index]; }
    auto operator[](size_t index) { return grid_[index]; }

    void clear();

    size_t num_instances(t_type_ptr type) const;

    //Returns the block types which limits the device size (may be empty if
    //resource limits were not considered when selecting the device).
    std::vector<t_type_ptr> limiting_resources() const { return limiting_resources_; }

  private:
    void count_instances();

    std::string name_;

    //Note that vtr::Matrix operator[] returns and intermediate type
    //which can be used or indexing in the second dimension, allowing
    //traditional 2-d indexing to be used
    vtr::Matrix<t_grid_tile> grid_;

    std::map<t_type_ptr, size_t> instance_counts_;

    std::vector<t_type_ptr> limiting_resources_;
};

#endif
