#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    October 2024
 * @brief   The declaration of the PrimitiveVector object.
 *
 * This object is designed to store a sparse M-dimensional vector which can be
 * efficiently operated upon.
 *
 * Each dimensions of this vector is indexed using a PrimitiveVectorDim. These
 * dims are controlled outside of this class and are used to encode more complex
 * meaning based on the mass of primitives (by the Mass Calculator class).
 *
 * To keep track of the meaning of each dimension, the mass calculator also
 * has a primitive dim manager class. That class is what holds all of the
 * available dims and lookups between the models and the dims.
 */

#include <cmath>
#include <cstdlib>
#include <vector>
#include "vtr_log.h"
#include "vtr_vector.h"

#include "primitive_vector_fwd.h"

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
    /// Although it is assumed that the primitive vector will be quite sparse,
    /// found that using an unordered map was slower than just directly using
    /// a vector and leaving them empty. Instead, using a vector; but outside
    /// of this class, we are careful to try and keep the most used information
    /// in the early dimensions of this array so the vector can remain small in
    /// length and only grow if it needs to.
    vtr::vector<PrimitiveVectorDim, float> data_;

  public:
    /**
     * @brief Add the value to the given dimension.
     *
     * This is a common enough feature to use its own setter.
     */
    inline void add_val_to_dim(float val, PrimitiveVectorDim dim) {
        if ((size_t)dim >= data_.size())
            data_.resize((size_t)dim + 1, 0.0f);
        data_[dim] += val;
    }

    /**
     * @brief Subtract the value to the given dimension.
     */
    inline void subtract_val_from_dim(float val, PrimitiveVectorDim dim) {
        if ((size_t)dim >= data_.size())
            data_.resize((size_t)dim + 1, 0.0f);
        data_[dim] -= val;
    }

    /**
     * @brief Get the value at the given dimension.
     */
    inline float get_dim_val(PrimitiveVectorDim dim) const {
        if ((size_t)dim >= data_.size())
            return 0.0f;
        return data_[dim];
    }

    /**
     * @brief Set the value at the given dimension.
     */
    inline void set_dim_val(PrimitiveVectorDim dim, float val) {
        if ((size_t)dim >= data_.size())
            data_.resize((size_t)dim + 1, 0.0f);
        data_[dim] = val;
    }

    /**
     * @brief Equality operator between two Primitive Vectors.
     *
     * Returns true if the dimensions of each vector are equal.
     */
    inline bool operator==(const PrimitiveVector& rhs) const {
        size_t num_elem_to_check = std::max(rhs.data_.size(), data_.size());
        // Check if every dim in rhs matches this.
        for (size_t i = 0; i < num_elem_to_check; i++) {
            PrimitiveVectorDim dim = (PrimitiveVectorDim)i;
            if (get_dim_val(dim) != rhs.get_dim_val(dim))
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
        for (size_t i = 0; i < rhs.data_.size(); i++) {
            PrimitiveVectorDim dim = (PrimitiveVectorDim)i;
            add_val_to_dim(rhs.get_dim_val(dim), dim);
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
        for (size_t i = 0; i < rhs.data_.size(); i++) {
            PrimitiveVectorDim dim = (PrimitiveVectorDim)i;
            subtract_val_from_dim(rhs.get_dim_val(dim), dim);
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
            p *= rhs;
        }
        return *this;
    }

    /**
     * @brief Element-wise division with a scalar.
     */
    inline PrimitiveVector& operator/=(float rhs) {
        for (auto& p : data_) {
            p /= rhs;
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
        size_t num_elem_to_check = std::max(rhs.data_.size(), data_.size());
        for (size_t i = 0; i < num_elem_to_check; i++) {
            PrimitiveVectorDim dim = (PrimitiveVectorDim)i;
            if (get_dim_val(dim) < rhs.get_dim_val(dim))
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
        for (float& val : data_) {
            if (val < 0.0f)
                val = 0.0f;
        }
    }

    /**
     * @brief Returns true if all dimensions of this vector are zero.
     */
    inline bool is_zero() const {
        // NOTE: This can be made cheaper by storing this information at
        //       creation and updating it if values are added or removed.
        for (float p : data_) {
            if (p != 0.f)
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
        for (float p : data_) {
            if (p < 0.f)
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
        for (float p : data_) {
            mag += std::abs(p);
        }
        return mag;
    }

    /**
     * @brief Computes the sum across all dimensions of the vector.
     *
     * This is similar to manhattan_norm, however this does not take the
     * absolute value of each dimension.
     */
    inline float sum() const {
        float sum = 0.f;
        for (float p : data_) {
            sum += p;
        }
        return sum;
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
        size_t last_non_zero_dim = 0;
        for (size_t i = 0; i < data_.size(); i++) {
            PrimitiveVectorDim dim = (PrimitiveVectorDim)i;
            if (dir.get_dim_val(dim) == 0.0f)
                data_[dim] = 0.0f;
            else
                last_non_zero_dim = i;
        }
        // Resize the vector to the last non-zero dim. This can improve performance
        // by keeping the size of vectors as small as possible.
        data_.resize(last_non_zero_dim + 1);
    }

    /**
     * @brief Gets the non-zero dimensions of this vector.
     */
    inline std::vector<PrimitiveVectorDim> get_non_zero_dims() const {
        std::vector<PrimitiveVectorDim> non_zero_dims;
        for (size_t i = 0; i < data_.size(); i++) {
            PrimitiveVectorDim dim = (PrimitiveVectorDim)i;
            if (data_[dim] != 0.0f)
                non_zero_dims.push_back(dim);
        }
        return non_zero_dims;
    }

    /**
     * @brief Returns true if this and other do not share any non-zero dimensions.
     */
    inline bool are_dims_disjoint(const PrimitiveVector& other) const {
        size_t dims_to_check = std::min(data_.size(), other.data_.size());
        for (size_t i = 0; i < dims_to_check; i++) {
            PrimitiveVectorDim dim = (PrimitiveVectorDim)i;
            // If this and other both have a shared dimension, then they are not
            // perpendicular.
            if (other.get_dim_val(dim) != 0.0f && get_dim_val(dim) != 0.0f) {
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
        size_t num_dims = std::max(lhs.data_.size(), rhs.data_.size());
        res.data_.resize(num_dims, 0.0f);
        for (size_t i = 0; i < num_dims; i++) {
            PrimitiveVectorDim dim = (PrimitiveVectorDim)i;
            res.data_[dim] = std::max(lhs.get_dim_val(dim), rhs.get_dim_val(dim));
        }
        return res;
    }

    /**
     * @brief Debug printing method.
     */
    inline void print() const {
        for (size_t i = 0; i < data_.size(); i++) {
            PrimitiveVectorDim dim = (PrimitiveVectorDim)i;
            VTR_LOG("(%zu, %f)\n", i, get_dim_val(dim));
        }
    }
};
