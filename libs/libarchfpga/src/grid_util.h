#include <concepts>
#include <vector>

#include "vtr_log.h"
#include "vtr_ndmatrix.h"

/**
 * @brief Constructs a device_width*device_heigth sized matrix from a reduced m*n matrix, 'm' vertical lines and 'n' horizontal lines.
 * For example, imagine we have a 5*5 device. We have two vertical lines at x = 1 and x = 3, and a horizontal line at y = 3.
 * If the reduced matrix looks like this:
 *
 * 1, 2, 3
 * 4, 5, 6
 *
 * The output device-sized matrix would look like this:
 *
 *  1, 1 | 2, 2 | 3
 * ------------------
 *  4, 4 | 5, 5 | 5
 *  4, 4 | 5, 5 | 5
 *  4, 4 | 5, 5 | 5
 *  4, 4 | 5, 5 | 5
 * 
 * @param device_width Width of the device
 * @param device_height Height of the device
 * @param horizontal_lines Horizontal lines defining blocks in the device
 * @param vertical_lines Vertical lines defining blocks in the device
 * @param reduced_matrix Reduced matrix with values for each block
 */
template<typename T>
vtr::NdMatrix<T, 2> get_device_sized_matrix_from_reduced(size_t device_width,
                                                         size_t device_height,
                                                         const std::vector<int>& horizontal_lines,
                                                         const std::vector<int>& vertical_lines,
                                                         const vtr::NdMatrix<T, 2>& reduced_matrix) {
    vtr::NdMatrix<T, 2> device_matrix({device_width, device_height}, T());

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

            int x_end = device_width - 1;
            if (i != vertical_lines.size()) {
                x_end = vertical_lines[i];
            }

            int y_start = -1;
            if (j != 0) {
                y_start = horizontal_lines[j - 1];
            }

            int y_end = device_height - 1;
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
