#ifndef DEVICE_GRID
#define DEVICE_GRID

#include <string>
#include <vector>
#include <cmath>
#include "vtr_ndmatrix.h"
#include "physical_types.h"
#include "vtr_geometry.h"

/**
 * @brief s_grid_tile is the minimum tile of the fpga
 * @note This struct shouldn't be directly accessed by other functions. Use the helper functions in DeviceGrid instead.
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
    DeviceGrid(std::string grid_name, vtr::NdMatrix<t_grid_tile, 3> grid);
    DeviceGrid(std::string grid_name, vtr::NdMatrix<t_grid_tile, 3> grid, std::vector<t_logical_block_type_ptr> limiting_res);

    const std::string& name() const { return name_; }

    ///@brief Return the number of layers(number of dies)
    inline int get_num_layers() const {
        return (int)grid_.dim_size(0);
    }

    ///@brief Return the width of the grid at the specified layer
    size_t width() const { return grid_.dim_size(1); }
    ///@brief Return the height of the grid at the specified layer
    size_t height() const { return grid_.dim_size(2); }

    ///@brief Return the size of the flattened grid on the given layer
    inline size_t grid_size() const {
        return grid_.size();
    }

    ///@brief deallocate members of DeviceGrid
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

    ///@brief Returns a rectangle which represents the bounding box of the tile at the given location.
    inline vtr::Rect<int> get_tile_bb(const t_physical_tile_loc& tile_loc) const {
        t_physical_tile_type_ptr tile_type = get_physical_type(tile_loc);

        int tile_xlow = tile_loc.x - get_width_offset(tile_loc);
        int tile_ylow = tile_loc.y - get_height_offset(tile_loc);
        int tile_xhigh = tile_xlow + tile_type->width - 1;
        int tile_yhigh = tile_ylow + tile_type->height - 1;

        return {{tile_xlow, tile_ylow}, {tile_xhigh, tile_yhigh}};
    }

    ///@brief Return the metadata of the tile at the specified location
    inline const t_metadata_dict* get_metadata(const t_physical_tile_loc& tile_loc) const {
        return grid_[tile_loc.layer_num][tile_loc.x][tile_loc.y].meta;
    }

    ///@brief Given t_grid_tile, return the x coordinate of the tile on the given layer - Used by serializer functions
    inline int get_grid_loc_x(const t_grid_tile*& grid_loc) const {
        int layer_num = std::floor(static_cast<int>(grid_loc - &grid_.get(0)) / (width() * height()));
        auto diff = grid_loc - &grid_.get(layer_num * height() * width());

        return diff / grid_.dim_size(2);
    }

    ///@brief Given t_grid_tile, return the y coordinate of the tile on the given layer - Used by serializer functions
    inline int get_grid_loc_y(const t_grid_tile*& grid_loc) const {
        int layer_num = std::floor(static_cast<int>(grid_loc - &grid_.get(0)) / (width() * height()));
        auto diff = grid_loc - &grid_.get(layer_num * height() * width());

        return diff % grid_.dim_size(2);
    }

    ///@brief Given t_grid_tile, return the layer number of the tile - Used by serializer functions
    inline int get_grid_loc_layer(const t_grid_tile*& grid_loc) const {
        int layer_num = std::floor(static_cast<int>(grid_loc - &grid_.get(0)) / (width() * height()));
        return layer_num;
    }

    ///@brief Return the nth t_grid_tile on the given layer of the flattened grid - Used by serializer functions
    inline const t_grid_tile* get_grid_locs_grid_loc(int n) const {
        return &grid_.get(n);
    }

  private:
    ///@brief count_instances() counts the number of each tile type on each layer and store it in instance_counts_. It is called in the constructor.
    void count_instances();

    std::string name_;

    /**
     * @brief grid_ is a 3D matrix that represents the grid of the FPGA chip.
     * @note The first dimension is the layer number (grid_[0] corresponds to the bottom layer), the second dimension is the x coordinate, and the third dimension is the y coordinate.
     * @note Note that vtr::Matrix operator[] returns and intermediate type
     * @note which can be used for indexing in the second dimension, allowing
     * @note traditional 2-d indexing to be used
     */
    vtr::NdMatrix<t_grid_tile, 3> grid_; //This stores the grid of complex blocks. It is a 3D matrix: [0..num_layers-1][0..grid.width()-1][0..grid_height()-1]

    ///@brief instance_counts_ stores the number of each tile type on each layer. It is initialized in count_instances().
    std::vector<std::map<t_physical_tile_type_ptr, size_t>> instance_counts_; /* [layer_num][physical_tile_type_ptr] */

    std::vector<t_logical_block_type_ptr> limiting_resources_;
};

#endif
