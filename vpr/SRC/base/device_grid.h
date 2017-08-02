#ifndef DEVICE_GRID
#define DEVICE_GRID

#include "vtr_ndmatrix.h"
#include "vpr_types.h"

class DeviceGrid {

    public:
        DeviceGrid() = default;
        DeviceGrid(vtr::Matrix<t_grid_tile> grid): grid_(grid) {}

        int width() const { return grid_.dim_size(0); }
        int height() const { return grid_.dim_size(1); }

        //TODO: eventually remove all references to nx/ny
        int nx() const { return width() - 2; }
        int ny() const { return height() - 2; }

        auto operator[](size_t index) const { return grid_[index]; }
        auto operator[](size_t index) { return grid_[index]; }

        void clear() { grid_.clear(); }

    private:
        //Note that vtr::Matrix operator[] returns and intermediate type 
        //which can be used or indexing in the second dimension, allowing
        //traditional 2-d indexing to be used
        vtr::Matrix<t_grid_tile> grid_;
};


#endif
