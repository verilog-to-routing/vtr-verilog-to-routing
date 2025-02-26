/**
 * @file
 * @author  Alex Singer
 * @date    February 2025
 * @brief   Definition of the Prefix Sum class which enables O(1) time-complexity
 *          sums over regions of an unchanging grid of values.
 */

#pragma once

#include <functional>
#include <vector>
#include "vtr_assert.h"
#include "vtr_ndmatrix.h"

namespace vtr {

/**
 * @brief 1D Prefix Sum manager class.
 *
 * Given an array of values, it may be necessary to find the sum of values
 * within a continuous sub-section of the array. If this operation needs to be
 * performed many times, this may be expensive in runtime to calculate.
 *
 * If the array of values does not change, we can create a prefix sum which will
 * allow us to get the sum of values in some continuous sub-section of the array
 * in O(1) time, instead of O(k) time where k is the number of values in the
 * sub-section.
 *
 * This class has a space complexity of O(l) where l is the length of the array
 * of values.
 *
 *
 * Static Array of Values Example (values stored in a vector):
 *
 * std::vector<float> vals = {...};
 *
 * // Build the Prefix Sum
 * vtr::PrefixSum1D<float> prefix_sum(vals);
 *
 * // Compute the sum of the values between index 3 and 7 of the array (inclusive)
 * float sum = prefix_sum.get_sum(3, 7);
 *
 *
 * Dynamic Vector of Values Example (values derived at runtime):
 *
 * // Build the Prefix Sum using a lambda
 * vtr::PrefixSum1D<float> prefix_sum(length, [&](size_t x) {
 *      // This lambda returns the value that would be in the array at index x.
 *      return static_cast<float>(x * x);
 * });
 *
 * // Compute the sum of the values between index 0 and 5 of the array (inclusive)
 * float sum = prefix_sum.get_sum(0, 5);
 */
template<typename T>
class PrefixSum1D {
public:
    PrefixSum1D() = default;

    /**
     * @brief Construct the 1D prefix sum.
     *
     * This pre-computes the sums of values in the array, making it faster to
     * get the sum of sub-regions of the array later.
     *
     * This constructor has a time complexity of O(length)
     *
     *  @param length
     *          The length of the array to a make a prefix sum of.
     *  @param lookup
     *          A lambda function which will return the value in the array at
     *          the given x index. This is a lambda to allow a prefix sum to be
     *          created, even if the values in the array are not stored in a
     *          vector (may be computed on the spot).
     *  @param zero
     *          What is zero for this data type. For most basic data types (like
     *          int float, etc.) this parameter can be ignored; for more complex
     *          data classes (like multi-dimensional vectors) this is necessary
     *          to be passed in.
     */
    PrefixSum1D(size_t length, std::function<T(size_t)> lookup, T zero = T())
            : prefix_sum_(length + 1, zero) {
        // The first value in the prefix sum is already initialized to 0.

        // Initialize the prefix sum. The prefix sum at position x is the sum
        // of all values in the original array from 0 to x - 1.
        for (size_t x = 1; x < length + 1; x++) {
            prefix_sum_[x] = prefix_sum_[x - 1] + lookup(x - 1);
        }
    }

    /**
     * @brief Construct the 1D prefix sum from a vector.
     */
    PrefixSum1D(std::vector<T> vals, T zero = T())
            : PrefixSum1D(vals.size(),
                          [&](size_t x) noexcept {
                            return vals[x];
                          },
                          zero) {}

    /**
     * @brief Get the sum of all values in the original array of values between
     *        lower_x and upper_x (inclusive).
     *
     * Inclusive means that the sum will include the values at lower_x and
     * upper_x.
     *
     * This method has O(1) time complexity.
     */
    T get_sum(size_t lower_x, size_t upper_x) const {
        // Some safety asserts.
        VTR_ASSERT_SAFE_MSG(lower_x <= upper_x, "lower_x is larger than upper_x");
        VTR_ASSERT_SAFE_MSG(lower_x < prefix_sum_.size() - 1, "lower_x out of range");
        VTR_ASSERT_SAFE_MSG(upper_x < prefix_sum_.size() - 1, "upper_x out of range");

        // The sum of the region lower_x to upper_x inclusive is equal to
        //      - The sum from 0 to upper_x
        //      - Minus the sum from 0 to lower_x - 1
        // Note: These are all offset by 1 since the first value is zero. This
        //       saves us from having to do bound checking.
        return prefix_sum_[upper_x + 1] - prefix_sum_[lower_x];
    }

private:
    /**
     * @brief The 1D prefix sum of the original array of values.
     *
     * Index x of the prefix sum contains the sum of all values in the original
     * array from 0 to x - 1. The first value in this array is 0. By setting the
     * first value in the array to 0, we can avoid bound checking. This data
     * structure has the special property that the sum of any sub-array can be
     * computed in O(1) time.
     */
    std::vector<T> prefix_sum_;
};

/**
 * @brief 2D Prefix Sum manager class.
 *
 * Given a 2D grid of values, it may be necessary to find the sum of values
 * within some rectangular sub-region of that grid. If this operation needs to
 * be performed many times, this may be expensive in runtime to calculate.
 *
 * If the grid of values does not change, we can create a prefix sum which will
 * allow us to get the sum of values in some rectangular sub-region of the
 * grid in O(1) time, instead of O(k) time where k is the number of values
 * in the region.
 *
 * This class has a space complexity of O(w * h) where w and h are the width
 * and height of the grid of values.
 *
 *
 * Static Matrix of Values Example (values stored in a matrix):
 *
 * vtr::NdMatrix<float, 2> vals({w, h});
 *
 * // ... Initialize vals
 *
 * // Build the Prefix Sum
 * vtr::PrefixSum2D<float> prefix_sum(vals);
 *
 * // Compute the sum of the rectangular region from (1, 2) to (3, 4) inclusive.
 * float sum = prefix_sum.get_sum(1, 2, 3, 4);
 *
 *
 * Dynamic Matrix of Values Example (values derived at runtime):
 *
 * // Build the Prefix Sum using a lambda
 * vtr::PrefixSum2D<float> prefix_sum(w, h, [&](size_t x, size_t y) {
 *      // This lambda returns the value that would be in the matrix at (x, y)
 *      return (x + y) / 2.f;
 * });
 *
 * // Compute the sum of the rectangular region from (0, 4) to (3, 5) inclusive.
 * float sum = prefix_sum.get_sum(0, 4, 3, 5);
 */
template<typename T>
class PrefixSum2D {
public:
    PrefixSum2D() = default;

    /**
     * @brief Construct the 2D prefix sum.
     *
     * This pre-computes the sums of values in the grid, making it faster to
     * get the sum of sub-regions of the grid later.
     *
     * This constructor has a time complexity of O(w * h).
     *
     *  @param w
     *          The width of the grid of values to make a prefix sum over.
     *  @param h
     *          The height of the grid of values to make a prefix sum over.
     *  @param lookup
     *          A lambda function which will return the value in the grid at the
     *          given x, y position. This is a lambda to allow a prefix sum to
     *          be created, even if the values in the grid are not stored in
     *          a matrix (may be computed at runtime).
     *  @param zero
     *          What is zero for this data type. For most basic data types (like
     *          int, float, etc.) this parameter can be ignored; for more complex
     *          data classes (like multi-dimensional vectors) this is necessary
     *          to be passed in.
     */
    PrefixSum2D(size_t w, size_t h, std::function<T(size_t, size_t)> lookup, T zero = T())
            : prefix_sum_({w + 1, h + 1}, zero) {
        // The first row and first column should already be initialized to zero.

        // Initialize the prefix sum. The prefix sum at position (x, y) is the
        // sum of all values in the original matrix in the rectangle from (0, 0)
        // to (x - 1, y - 1) inclusive.
        for (size_t x = 1; x < w + 1; x++) {
            for (size_t y = 1; y < h + 1; y++) {
                prefix_sum_[x][y] = prefix_sum_[x - 1][y] +
                                    prefix_sum_[x][y - 1] +
                                    lookup(x - 1, y - 1) -
                                    prefix_sum_[x - 1][y - 1];
            }
        }
    } 

    /**
     * @brief Constructs a 2D prefix sum from a 2D grid of values.
     */
    PrefixSum2D(const vtr::NdMatrix<T, 2>& vals, T zero = T())
            : PrefixSum2D(vals.dim_size(0),
                          vals.dim_size(1),
                          [&](size_t x, size_t y) {
                            return vals[x][y];
                          },
                          zero) {}

    /**
     * @brief Get the sum of all values in the original grid of values between
     *        x = [lower_x, upper_x] and y = [lower_y, upper_y].
     *
     * This sum is inclusive, so it also sums the values at (upper_x, upper_y).
     *
     * This method has O(1) time complexity.
     */
    T get_sum(size_t lower_x, size_t lower_y, size_t upper_x, size_t upper_y) const {
        // Some safety asserts.
        VTR_ASSERT_SAFE_MSG(lower_x <= upper_x, "lower_x is larger than upper_x");
        VTR_ASSERT_SAFE_MSG(lower_y <= upper_y, "lower_y is larger than upper_y");
        VTR_ASSERT_SAFE_MSG(lower_x < prefix_sum_.dim_size(0) - 1, "lower_x out of range");
        VTR_ASSERT_SAFE_MSG(upper_x < prefix_sum_.dim_size(0) - 1, "upper_x out of range");
        VTR_ASSERT_SAFE_MSG(lower_y < prefix_sum_.dim_size(1) - 1, "lower_y out of range");
        VTR_ASSERT_SAFE_MSG(upper_y < prefix_sum_.dim_size(1) - 1, "upper_y out of range");

        // The sum of the region (lower_x, lower_y) to (upper_x, upper_y)
        // inclusive is equal to:
        //      - The sum of the region (0, 0) to (upper_x, upper_y)
        //      - Minus the sum of the region (0, 0) to (lower_x - 1, upper_y)
        //          - Remove the part below the region
        //      - Minus the sum of the region (0, 0) to (upper_x, lower_y - 1)
        //          - Remove the part left of the region
        //      - Plus the sum of the region (0, 0) to (lower_x - 1, lower_y - 1)
        //          - Add back on the lower-left corner which was subtracted twice.
        // Note: all of these are offset by 1 since the first row and column
        //       are all zeros. This allows us to avoid bounds checking when
        //       lower_x or lower_y are 0.
        return prefix_sum_[upper_x + 1][upper_y + 1] - prefix_sum_[lower_x][upper_y + 1]
                                                     - prefix_sum_[upper_x + 1][lower_y]
                                                     + prefix_sum_[lower_x][lower_y];
    }

private:
    /**
     * @brief The 2D prefix sum of the original grid of values.
     *
     * Position (x, y) of the prefix sum contains the sum of all values in the
     * rectangle (0, 0) -> (x - 1, y - 1) inclusive. The first row and column
     * are all zeros. By setting these to zero, we can avoid bound checking.
     * This data structure has the special property that the sum of any
     * rectangular region can be computed in O(1) time.
     */
    vtr::NdMatrix<T, 2> prefix_sum_;
};

} // namespace vtr

