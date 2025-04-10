/**
 * @file
 * @author  Alex Singer
 * @date    October 2024
 * @brief   The declaration of the PrimitiveVector object.
 *
 * This object is designed to store a sparse M-dimensional vector which can be
 * efficiently operated upon.
 */

#pragma once

#include <cmath>
#include <cstdlib>
#include <unordered_map>
#include <vector>
#include "vtr_log.h"

/**
 * @brief A sparse vector class to store an M-dimensional quantity of primitives
 *        in the context of a legalizer.
 *
 * This vector is used to represent the capacity of tiles for different
 * primitives in a closed form which can be manipulated with math operations.
 *
 * This vector is also used to represent the "mass" of AP blocks in primitives,
 * since an AP block may represent many primitives.
 *
 * This vector stores floats since it is expected that, due to some heuristics,
 * the mass of a block may not be a whole number.
 *
 * This class contains useful operations to operate and compare different
 * Primitive Vectors.
 */
class PrimitiveVector {
  private:
    /// @brief Storage container for the data of this primitive vector.
    ///
    /// This is stored as a map since it is assumed that the vector will be
    /// quite sparse. This is designed to be a vector which has a dimension
    /// for each t_model::index.
    ///
    /// TODO: Is there a more efficient way to store this sparse info?
    ///       Perhaps we can just waste the space and use a vector.
    std::unordered_map<size_t, float> data_;

  public:
    /**
     * @brief Add the value to the given dimension.
     *
     * This is a common enough feature to use its own setter.
     */
    inline void add_val_to_dim(float val, size_t dim) {
        auto it = data_.find(dim);
        if (it == data_.end())
            data_.insert({dim, val});
        else {
            it->second += val;
        }
    }

    /**
     * @brief Subtract the value to the given dimension.
     */
    inline void subtract_val_from_dim(float val, size_t dim) {
        auto it = data_.find(dim);
        if (it == data_.end())
            data_.insert({dim, -1.0f * val});
        else {
            it->second -= val;
        }
    }

    /**
     * @brief Get the value at the given dimension.
     */
    inline float get_dim_val(size_t dim) const {
        const auto it = data_.find(dim);
        // If there is no data in the dim, return 0. By default the vector is
        // empty.
        if (it == data_.end())
            return 0.f;
        // If there is data at this dimension, return it.
        return it->second;
    }

    /**
     * @brief Set the value at the given dimension.
     */
    inline void set_dim_val(size_t dim, float val) {
        data_[dim] = val;
    }

    /**
     * @brief Equality operator between two Primitive Vectors.
     *
     * Returns true if the dimensions of each vector are equal.
     */
    inline bool operator==(const PrimitiveVector& rhs) const {
        // Check if every dim in rhs matches this.
        for (const auto& p : rhs.data_) {
            if (get_dim_val(p.first) != p.second)
                return false;
        }
        // If there is anything in this which is not in rhs, need to check.
        for (const auto& p : data_) {
            if (rhs.get_dim_val(p.first) != p.second)
                return false;
        }
        return true;
    }

    /**
     * @brief Inequality operator between two Primitive Vectors.
     */
    inline bool operator!=(const PrimitiveVector& rhs) const {
        return !operator==(rhs);
    }

    /**
     * @brief Element-wise accumulation of rhs into this.
     */
    inline PrimitiveVector& operator+=(const PrimitiveVector& rhs) {
        for (const auto& p : rhs.data_) {
            add_val_to_dim(p.second, p.first);
        }
        return *this;
    }

    /**
     * @brief Element-wise addition of this with rhs.
     */
    inline PrimitiveVector operator+(const PrimitiveVector& rhs) const {
        PrimitiveVector res = *this;
        res += rhs;
        return res;
    }

    /**
     * @brief Element-wise de-accumulation of rhs into this.
     */
    inline PrimitiveVector& operator-=(const PrimitiveVector& rhs) {
        for (const auto& p : rhs.data_) {
            subtract_val_from_dim(p.second, p.first);
        }
        return *this;
    }

    /**
     * @brief Element-wise subtration of two Primitive Vectors.
     */
    inline PrimitiveVector operator-(const PrimitiveVector& rhs) const {
        PrimitiveVector res = *this;
        res -= rhs;
        return res;
    }

    /**
     * @brief Element-wise multiplication with a scalar.
     */
    inline PrimitiveVector& operator*=(float rhs) {
        for (auto& p : data_) {
            p.second *= rhs;
        }
        return *this;
    }

    /**
     * @brief Element-wise division with a scalar.
     */
    inline PrimitiveVector& operator/=(float rhs) {
        for (auto& p : data_) {
            p.second /= rhs;
        }
        return *this;
    }

    /**
     * @brief Element-wise division with a scalar.
     */
    inline PrimitiveVector operator/(float rhs) const {
        PrimitiveVector res = *this;
        res /= rhs;
        return res;
    }

    /**
     * @brief Returns true if any dimension of this vector is less than any
     *        dimension of rhs; false otherwise.
     */
    inline bool operator<(const PrimitiveVector& rhs) const {
        // Check for any element of this < rhs
        for (const auto& p : data_) {
            if (p.second < rhs.get_dim_val(p.first))
                return true;
        }
        // Check for any element of rhs > this.
        // NOTE: This is required since there may be elements in rhs which are
        //       not in this.
        // TODO: This is inneficient.
        for (const auto& p : rhs.data_) {
            if (p.second > get_dim_val(p.first))
                return true;
        }
        return false;
    }

    /**
     * @brief Clamps all dimension of this vector to non-negative values.
     *
     * If a dimension is negative, the dimension will become 0. If the dimension
     * is positive, it will not change.
     */
    inline void relu() {
        std::erase_if(data_, [](const std::pair<size_t, float>& p) {
            // Note: we erase the numbers from the map to improve the performance
            //       of future operations on this vector.
            return p.second <= 0.0f;
        });
    }

    /**
     * @brief Returns true if all dimensions of this vector are zero.
     */
    inline bool is_zero() const {
        // NOTE: This can be made cheaper by storing this information at
        //       creation and updating it if values are added or removed.
        for (const auto& p : data_) {
            if (p.second != 0.f)
                return false;
        }
        return true;
    }

    /**
     * @brief Returns true if any dimension of this vector is non-zero.
     */
    inline bool is_non_zero() const {
        return !is_zero();
    }

    /**
     * @brief Returns true if all dimensions of this vector are non-negative.
     */
    inline bool is_non_negative() const {
        for (const auto& p : data_) {
            if (p.second < 0.f)
                return false;
        }
        return true;
    }

    /**
     * @brief Computes the manhattan (L1) norm of this vector.
     *
     * This is the sum of the absolute value of all dimensions.
     */
    inline float manhattan_norm() const {
        // NOTE: This can be made much cheaper by storing the magnitude as part
        //       of the class and updating it whenever something is added or
        //       removed.
        float mag = 0.f;
        for (const auto& p : data_) {
            mag += std::abs(p.second);
        }
        return mag;
    }

    /**
     * @brief Project this vector onto the given vector.
     *
     * This basically just means zero-ing all dimension which are zero in the
     * given vector. The given vector does not need to be a unit vector.
     *
     * Example: Project <12, 32, 8, 2> onto <2, 0, 2, 0> = <12, 0, 8, 0>
     */
    inline void project(const PrimitiveVector& dir) {
        // For each dimension of this vector, if that dimension is zero in dir
        // set the dimension to zero.
        std::erase_if(data_, [&](const std::pair<size_t, float>& p) {
            return dir.get_dim_val(p.first) == 0.0f;
        });
    }

    /**
     * @brief Gets the non-zero dimensions of this vector.
     */
    inline std::vector<int> get_non_zero_dims() const {
        std::vector<int> non_zero_dims;
        for (auto& p : data_) {
            if (p.second != 0.0f)
                non_zero_dims.push_back(p.first);
        }
        return non_zero_dims;
    }

    /**
     * @brief Returns true if this and other do not share any non-zero dimensions.
     */
    inline bool are_dims_disjoint(const PrimitiveVector& other) const {
        for (const auto& p : other.data_) {
            // If this and other both have a shared dimension, then they are not
            // perpendicular.
            if (p.second != 0.0f && get_dim_val(p.first) != 0.0f) {
                return false;
            }
        }
        // If they do not share any dimensions, then they are perpendicular.
        return true;
    }

    /**
     * @brief Clear the sparse vector, which is equivalent to setting it to be
     *        the zero vector.
     */
    inline void clear() {
        data_.clear();
    }

    /**
     * @brief Compute the elementwise max between two primitive vectors.
     */
    static inline PrimitiveVector max(const PrimitiveVector& lhs,
                                      const PrimitiveVector& rhs) {
        PrimitiveVector res;
        // For each key in rhs, get the max(lhs, rhs)
        for (const auto& p : rhs.data_) {
            res.set_dim_val(p.first,
                            std::max(lhs.get_dim_val(p.first), p.second));
        }
        // For each key in lhs, get the max(lhs, rhs)
        for (const auto& p : lhs.data_) {
            res.set_dim_val(p.first,
                            std::max(p.second, rhs.get_dim_val(p.first)));
        }
        return res;
    }

    /**
     * @brief Debug printing method.
     */
    inline void print() const {
        for (const auto& p : data_) {
            VTR_LOG("(%zu, %f)\n", p.first, p.second);
        }
    }
};
