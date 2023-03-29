#ifndef DEVICE_GRID
#define DEVICE_GRID

#include <string>
#include <vector>
#include "vtr_ndmatrix.h"
#include "physical_types.h"

/**
 * @brief s_grid_tile is the minimum tile of the fpga
 * @note This struct shouldn't be directly accessed by other functions. Use helper function in DeviceGrid instead.
 */
struct t_grid_tile {
    t_physical_tile_type_ptr type = nullptr; ///<Pointer to type descriptor, NULL for illegal
    int width_offset = 0;                    ///<Number of grid tiles reserved based on width (right) of a block
    int height_offset = 0;                   ///<Number of grid tiles reserved based on height (top) of a block
    const t_metadata_dict* meta = nullptr;
};

///@brief DeviceGrid represents the FPGA fabric. It is used to get information about different layers and tiles.
// TODO: All of the function that use helper functions of this class should pass the layer_num to the functions, and the default value of layer_num should be deleted eventually.
class DeviceGrid {
  public:
    DeviceGrid() = default;
    DeviceGrid(std::string grid_name, std::vector<vtr::Matrix<t_grid_tile>> grid);
    DeviceGrid(std::string grid_name, std::vector<vtr::Matrix<t_grid_tile>> grid, std::vector<t_logical_block_type_ptr> limiting_res);

    const std::string& name() const { return name_; }

    ///@brief Return the width of the grid at the specified layer
    size_t width(int layer_num = 0) const { return grid_[layer_num].dim_size(0); }
    ///@brief Return the height of the grid at the specified layer
    size_t height(int layer_num = 0) const { return grid_[layer_num].dim_size(1); }

    ///@brief Return the number of layers(number of dies)
    inline int get_num_layers() const {
        return num_layers_;
    }

    ///@brief Given t_grid_tile, return the x coordinate of the tile on the given layer
    inline int get_grid_loc_x(const t_grid_tile*& grid_loc, int layer_num = 0) const {
        auto diff = grid_loc - &grid_[layer_num].get(0);

        return diff / grid_[layer_num].dim_size(1);
    }

    ///@brief Given t_grid_tile, return the y coordinate of the tile on the given layer
    inline int get_grid_loc_y(const t_grid_tile*& grid_loc, int layer_num = 0) const {
        auto diff = grid_loc - &grid_[layer_num].get(0);

        return diff % grid_[layer_num].dim_size(1);
    }

    ///@brief Return the nth t_grid_tile on the given layer of the flattened grid
    inline const t_grid_tile* get_grid_locs_grid_loc(int n, int layer_num = 0) const {
        return &grid_[layer_num].get(n);
    }

    ///@brief Return the size of the flattened grid on the given layer
    inline size_t grid_size(int layer_num = 0) const {
        return grid_[layer_num].size();
    }

    inline size_t grid_dim_size(int dim, int layer_num = 0) const {
        return grid_[layer_num].dim_size(dim);
    }
    void clear();

    /**
     * @brief Return the number of instances of the specified tile type on the specified layer. If the layer_num is -1, return the total number of instances of the specified tile type on all layers.
     * @note This function should be used if count_instances() is called in the constructor.
     */
    size_t num_instances(t_physical_tile_type_ptr type, int layer_num) const;

    /**
     * @brief Returns the block types which limits the device size (may be empty if
     *        resource limits were not considered when selecting the device).
     */
    std::vector<t_logical_block_type_ptr> limiting_resources() const { return limiting_resources_; }

    ///@brief Return the t_physical_tile_type_ptr at the specified location
    inline t_physical_tile_type_ptr get_physical_type(const t_physical_tile_loc& tile_loc) const {
        return grid_[tile_loc.layer_num][tile_loc.x][tile_loc.y].type;
    }

    ///@brief Return the width offset of the tile at the specified location. The root location of the tile is where width_offset and height_offset are 0.
    inline int get_width_offset(const t_physical_tile_loc& tile_loc) const {
        return grid_[tile_loc.layer_num][tile_loc.x][tile_loc.y].width_offset;
    }

    ///@brief Return the height offset of the tile at the specified location. The root location of the tile is where width_offset and height_offset are 0
    inline int get_height_offset(const t_physical_tile_loc& tile_loc) const {
        return grid_[tile_loc.layer_num][tile_loc.x][tile_loc.y].height_offset;
    }

    ///@brief Return the metadata of the tile at the specified location
    inline const t_metadata_dict* get_metadata(const t_physical_tile_loc& tile_loc) const {
        return grid_[tile_loc.layer_num][tile_loc.x][tile_loc.y].meta;
    }

  private:
    ///@brief count_instances() counts the number of each tile type on each layer and store it in instance_counts_. It is called in the constructor.
    void count_instances();

    std::string name_;

    ///@brief num_layers_ is the number of layers (dies) in the FPGA chip. An FPGA chip has at least one layer.
    int num_layers_ = 1;

    /**
     * @brief grid_ is a vector of 2D grids. Each 2D grid represents a layer (die) of the FPGA chip. The layers are pushed backed to the top from the bottom (grid_[0] corresponds to the bottom layer).
     * @note The first dimension is the layer number, the second dimension is the x coordinate, and the third dimension is the y coordinate.
     * @note Note that vtr::Matrix operator[] returns and intermediate type
     * @note which can be used or indexing in the second dimension, allowing
     * @note traditional 2-d indexing to be used
     */
    std::vector<vtr::Matrix<t_grid_tile>> grid_;

    ///@brief instance_counts_ stores the number of each tile type on each layer. It is initialized in count_instances().
    std::vector<std::map<t_physical_tile_type_ptr, size_t>> instance_counts_; /* [layer_num][physical_tile_type_ptr] */

    std::vector<t_logical_block_type_ptr> limiting_resources_;
};

#endif
