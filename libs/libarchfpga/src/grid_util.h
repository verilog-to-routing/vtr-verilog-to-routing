#include <concepts>
#include <vector>

#include "vtr_log.h"
#include "vtr_ndmatrix.h"

template<typename T>
    requires std::integral<T> || std::floating_point<T>
vtr::NdMatrix<T, 2> get_device_sized_matrix_from_reduced(size_t x_size,
                                                         size_t y_size,
                                                         const std::vector<int>& horizontal_lines,
                                                         const std::vector<int>& vertical_lines,
                                                         const vtr::NdMatrix<T, 2>& reduced_matrix) {
    vtr::NdMatrix<T, 2> device_matrix({x_size, y_size});
    device_matrix.fill(0);

    for (size_t i = 0; i < vertical_lines.size() + 1; i++) {
        for (size_t j = 0; j < horizontal_lines.size() + 1; j++) {
            // Calculate bounds for the given die region
            // Because of VPR's coordinate system, bounds are (exclusive, inclusive] except the dice
            // on the bottom and left edges of the device which do include 0, their starting point.
            // We set x_start and y_start to -1 to handle this.

            int x_start = -1;
            if (i != 0) {
                x_start = vertical_lines[i - 1];
            }

            int x_end = x_size - 1;
            if (i != vertical_lines.size()) {
                x_end = vertical_lines[i];
            }

            int y_start = -1;
            if (j != 0) {
                y_start = horizontal_lines[j - 1];
            }

            int y_end = y_size - 1;
            if (j != horizontal_lines.size()) {
                y_end = horizontal_lines[j];
            }

            for (int x = x_start + 1; x <= x_end; x++) {
                for (int y = y_start + 1; y <= y_end; y++) {
                    device_matrix[x][y] = reduced_matrix[i][j];
                }
            }
        }
    }
    return device_matrix;
}
