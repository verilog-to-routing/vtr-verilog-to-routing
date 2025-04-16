#ifndef VTR_ND_OFFSET_MATRIX_H
#define VTR_ND_OFFSET_MATRIX_H
#include <array>
#include <memory>
#include <algorithm>

#include "vtr_assert.h"

namespace vtr {

/**
 * @brief  A half-open range specification for a matrix dimension [begin_index, last_index)
 * 
 * It comes with valid indices from [begin_index() ... end_index()-1], provided size() > 0.
 */
class DimRange {
  public:
    ///@brief default constructor
    DimRange() = default;

    ///@brief a constructor with begin_index, end_index
    DimRange(int begin, int end)
        : begin_index_(begin)
        , end_index_(end) {}

    ///@brief Return the begin index
    int begin_index() const { return begin_index_; }

    ///@brief Return the end index
    int end_index() const { return end_index_; }

    ///@brief Return the size
    size_t size() const { return end_index_ - begin_index_; }

  private:
    int begin_index_ = 0;
    int end_index_ = 0;
};

/**
 * @brief Proxy class for a sub-matrix of a NdOffsetMatrix class.
 *
 * This is used to allow chaining of array indexing [] operators in a natural way.
 * 
 * Each instance of this class peels off one-dimension and returns a NdOffsetMatrixProxy representing
 * the resulting sub-matrix. This is repeated recursively until we hit the 1-dimensional base-case.
 * 
 * Since this expansion happens at compiler time all the proxy classes get optimized away,
 * yielding both high performance and generality.
 * 
 * Recursive case: N-dimensional array
 */
template<typename T, size_t N>
class NdOffsetMatrixProxy {
  public:
    static_assert(N > 0, "Must have at least one dimension");

    /**
     * @brief Construct a matrix proxy object
     *
     *   dim_ranges: Array of DimRange objects
     * idim: The dimension associated with this proxy
     *  dim_stride: The stride of this dimension (i.e. how many element in memory between indicies of this dimension)
     *  start: Pointer to the start of the sub-matrix this proxy represents
     */
    NdOffsetMatrixProxy(const DimRange* dim_ranges, size_t idim, size_t dim_stride, T* start)
        : dim_ranges_(dim_ranges)
        , idim_(idim)
        , dim_stride_(dim_stride)
        , start_(start) {}

    ///@brief const [] operator
    const NdOffsetMatrixProxy<T, N - 1> operator[](int index) const {
        VTR_ASSERT_SAFE_MSG(index >= dim_ranges_[idim_].begin_index(), "Index out of range (below dimension minimum)");
        VTR_ASSERT_SAFE_MSG(index < dim_ranges_[idim_].end_index(), "Index out of range (above dimension maximum)");

        /*
         * Calculate the effective index
         *
         * The elements are stored in zero-indexed form, so we need to adjust
         * for any non-zero minimum index
         */
        int effective_index = index - dim_ranges_[idim_].begin_index();

        //Determine the stride of the next dimension
        int next_dim_stride = dim_stride_ / dim_ranges_[idim_ + 1].size();

        //Strip off one dimension
        return NdOffsetMatrixProxy<T, N - 1>(dim_ranges_,                             //Pass the dimension information
                                             idim_ + 1,                               //Pass the next dimension
                                             next_dim_stride,                         //Pass the stride for the next dimension
                                             start_ + dim_stride_ * effective_index); //Advance to index in this dimension
    }

    ///@brief [] operator
    NdOffsetMatrixProxy<T, N - 1> operator[](int index) {
        //Call the const version and cast-away constness
        return const_cast<const NdOffsetMatrixProxy<T, N>*>(this)->operator[](index);
    }

  private:
    const DimRange* dim_ranges_;
    const size_t idim_;
    const size_t dim_stride_;
    T* start_;
};

///@brief Base case: 1-dimensional array
template<typename T>
class NdOffsetMatrixProxy<T, 1> {
  public:
    /**
     * @brief Construct a matrix proxy object
     *
     *     - dim_ranges: Array of DimRange objects
     *     - dim_stride: The stride of this dimension (i.e. how many element in memory between indicies of this dimension)
     *     - start: Pointer to the start of the sub-matrix this proxy represents
     */
    NdOffsetMatrixProxy(const DimRange* dim_ranges, size_t idim, size_t dim_stride, T* start)
        : dim_ranges_(dim_ranges)
        , idim_(idim)
        , dim_stride_(dim_stride)
        , start_(start) {}

    ///@brief const [] operator
    const T& operator[](int index) const {
        VTR_ASSERT_SAFE_MSG(dim_stride_ == 1, "Final dimension must have stride 1");
        VTR_ASSERT_SAFE_MSG(index >= dim_ranges_[idim_].begin_index(), "Index out of range (below dimension minimum)");
        VTR_ASSERT_SAFE_MSG(index < dim_ranges_[idim_].end_index(), "Index out of range (above dimension maximum)");

        //The elements are stored in zero-indexed form, so we need to adjust
        //for any non-zero minimum index
        int effective_index = index - dim_ranges_[idim_].begin_index();

        //Base case
        return start_[effective_index];
    }

    ///@brief [] operator
    T& operator[](int index) {
        //Call the const version and cast-away constness
        return const_cast<T&>(const_cast<const NdOffsetMatrixProxy<T, 1>*>(this)->operator[](index));
    }

  private:
    const DimRange* dim_ranges_;
    const size_t idim_;
    const size_t dim_stride_;
    T* start_;
};

/**
 * @brief Base class for an N-dimensional matrix supporting arbitrary index ranges per dimension.
 *
 * This class implements all of the matrix handling (lifetime etc.) except for indexing
 * (which is implemented in the NdOffsetMatrix class). Indexing is split out to allows specialization
 * of indexing for N = 1.
 * 
 * Implementation:
 * 
 * This class uses a single linear array to store the matrix in c-style (row major)
 * order. That is, the right-most index is laid out contiguous memory.
 * 
 * This should improve memory usage (no extra pointers to store for each dimension),
 * and cache locality (less indirection via pointers, predictable strides).
 * 
 * The indices are calculated based on the dimensions to access the appropriate elements.
 * Since the indexing calculations are visible to the compiler at compile time they can be
 * optimized to be efficient.
 */
template<typename T, size_t N>
class NdOffsetMatrixBase {
  public:
    static_assert(N >= 1, "Minimum dimension 1");

    ///@brief An empty matrix (all dimensions size zero)
    NdOffsetMatrixBase() {
        clear();
    }

    /** 
     * @brief Specified dimension sizes:
     *
     *      [0..dim_sizes[0])
     *      [0..dim_sizes[1])
     *      ...
     * with optional fill value
     */
    NdOffsetMatrixBase(std::array<size_t, N> dim_sizes, T value = T()) {
        resize(dim_sizes, value);
    }

    /**
     * @brief Specified dimension index ranges:
     *
     *      [dim_ranges[0].begin_index() ... dim_ranges[1].end_index())
     *      [dim_ranges[1].begin_index() ... dim_ranges[1].end_index())
     *      ...
     * with optional fill value
     */
    NdOffsetMatrixBase(std::array<DimRange, N> dim_ranges, T value = T()) {
        resize(dim_ranges, value);
    }

  public: //Accessors
    ///@brief Returns the size of the matrix (number of elements)
    size_t size() const {
        ///@brief Size is the product of all dimension sizes
        size_t cnt = dim_size(0);
        for (size_t idim = 1; idim < ndims(); ++idim) {
            cnt *= dim_size(idim);
        }
        return cnt;
    }

    ///@brief Returns true if there are no elements in the matrix
    bool empty() const {
        return size() == 0;
    }

    ///@brief Returns the number of dimensions (i.e. N)
    size_t ndims() const {
        return dim_ranges_.size();
    }

    ///@brief Returns the size of the ith dimension
    size_t dim_size(size_t i) const {
        VTR_ASSERT_SAFE(i < ndims());

        return dim_ranges_[i].size();
    }

    ///@brief Returns the starting index of ith dimension
    int begin_index(size_t i) const {
        VTR_ASSERT_SAFE(i < ndims());

        return dim_ranges_[i].begin_index();
    }

    ///@brief Returns the one-past-the-end index of the ith dimension
    int end_index(size_t i) const {
        VTR_ASSERT_SAFE(i < ndims());

        return dim_ranges_[i].end_index();
    }

  public: //Mutators
    ///@brief Set all elements to 'value'
    void fill(T value) {
        std::fill(data_.get(), data_.get() + size(), value);
    }

    /**
     * @brief Resize the matrix to the specified dimensions
     *
     * If 'value' is specified all elements will be initialized to it,
     * otherwise they will be default constructed.
     */
    void resize(std::array<size_t, N> dim_sizes, T value = T()) {
        //Convert dimension to range [0..dim)
        for (size_t i = 0; i < dim_sizes.size(); ++i) {
            dim_ranges_[i] = {0, dim_sizes[i]};
        }
        alloc();
        fill(value);
    }

    /**
     * @brief Resize the matrix to the specified dimension ranges
     *
     * If 'value' is specified all elements will be initialized to it,
     * otherwise they will be default constructed.
     */
    void resize(std::array<DimRange, N> dim_ranges, T value = T()) {
        dim_ranges_ = dim_ranges;
        alloc();
        fill(value);
    }

    ///@brief Reset the matrix to size zero
    void clear() {
        data_.reset(nullptr);
        for (size_t i = 0; i < dim_ranges_.size(); ++i) {
            dim_ranges_[i] = {0, 0};
        }
    }

  public: //Lifetime management
    ///@brief Copy constructor
    NdOffsetMatrixBase(const NdOffsetMatrixBase& other)
        : NdOffsetMatrixBase(other.dim_ranges_) {
        std::copy(other.data_.get(), other.data_.get() + other.size(), data_.get());
    }

    ///@brief Move constructor
    NdOffsetMatrixBase(NdOffsetMatrixBase&& other)
        : NdOffsetMatrixBase() {
        swap(*this, other);
    }

    /**
     * @brief Copy/move assignment
     *
     * Note that rhs is taken by value (copy-swap idiom)
     */
    NdOffsetMatrixBase& operator=(NdOffsetMatrixBase rhs) {
        swap(*this, rhs);
        return *this;
    }

    ///@brief Swap two NdOffsetMatrixBase objects
    friend void swap(NdOffsetMatrixBase<T, N>& m1, NdOffsetMatrixBase<T, N>& m2) {
        std::swap(m1.dim_ranges_, m2.dim_ranges_);
        std::swap(m1.data_, m2.data_);
    }

  private:
    // Allocate space for all the elements
    void alloc() {
        data_ = std::make_unique<T[]>(size());
    }

  protected:
    std::array<DimRange, N> dim_ranges_;
    std::unique_ptr<T[]> data_ = nullptr;
};

/**
 * @brief An N-dimensional matrix supporting arbitrary (continuous) index ranges per dimension.
 * 
 * If no second template parameter is provided defaults to a 2-dimensional
 * matrix
 * 
 * Examples:
 * 
 *       //A 2-dimensional matrix with indices [0..4][0..9]
 *       NdOffsetMatrix<int,2> m1({5,10});
 * 
 *       //Accessing an element
 *       int i = m4[3][5];
 * 
 *       //Setting an element
 *       m4[6][20] = 0;
 * 
 *       //A 2-dimensional matrix with indices [2..6][5..9]
 *       // Note that C++ requires one more set of curly brace than you would expect
 *       NdOffsetMatrix<int,2> m2({{{2,7},{5,10}}});
 * 
 *       //A 3-dimensional matrix with indices [0..4][0..9][0..19]
 *       NdOffsetMatrix<int,3> m3({5,10,20});
 * 
 *       //A 3-dimensional matrix with indices [2..6][1..19][50..89]
 *       NdOffsetMatrix<int,3> m4({{{2,7}, {1,20}, {50,90}}});
 * 
 *       //A 2-dimensional matrix with indices [2..6][1..20], with all entries
 *       //initialized to 42
 *       NdOffsetMatrix<int,2> m4({{{2,7}, {1,21}}}, 42);
 * 
 *       //A 2-dimensional matrix with indices [0..4][0..9], with all entries
 *       //initialized to 42
 *       NdOffsetMatrix<int,2> m1({5,10}, 42);
 * 
 *       //Filling all entries with value 101
 *       m1.fill(101);
 * 
 *       //Resizing an existing matrix (all values reset to default constructed value)
 *       m1.resize({5,5})
 * 
 *       //Resizing an existing matrix (all elements set to value 88)
 *       m1.resize({15,55}, 88)
 */
template<typename T, size_t N>
class NdOffsetMatrix : public NdOffsetMatrixBase<T, N> {
    //General case
    static_assert(N >= 2, "Minimum dimension 2");

  public:
    ///@brief Use the base constructors
    using NdOffsetMatrixBase<T, N>::NdOffsetMatrixBase;

  public:
    /**
     * @brief Access an element
     *
     * Returns a proxy-object to allow chained array-style indexing  (N >= 2 case)
     * template<typename = typename std::enable_if<N >= 2>::type, typename T1=T>
     */
    const NdOffsetMatrixProxy<T, N - 1> operator[](int index) const {
        VTR_ASSERT_SAFE_MSG(this->dim_size(0) > 0, "Can not index into size zero dimension");
        VTR_ASSERT_SAFE_MSG(this->dim_size(1) > 0, "Can not index into size zero dimension");
        VTR_ASSERT_SAFE_MSG(index >= this->dim_ranges_[0].begin_index(), "Index out of range (below dimension minimum)");
        VTR_ASSERT_SAFE_MSG(index < this->dim_ranges_[0].end_index(), "Index out of range (above dimension maximum)");

        /*
         * Calculate the effective index
         * 
         * The elements are stored in zero-indexed form, so adjust for any
         * non-zero minimum index in this dimension
         */
        int effective_index = index - this->dim_ranges_[0].begin_index();

        //Calculate the stride for the current dimension
        int dim_stride = this->size() / this->dim_size(0);

        //Calculate the stride for the next dimension
        int next_dim_stride = dim_stride / this->dim_size(1);

        //Peel off the first dimension
        return NdOffsetMatrixProxy<T, N - 1>(this->dim_ranges_.data(),                          //Pass the dimension information
                                             1,                                                 //Pass the next dimension
                                             next_dim_stride,                                   //Pass the stride for the next dimension
                                             this->data_.get() + dim_stride * effective_index); //Advance to index in this dimension
    }

    /**
     * @brief Access an element
     *
     * Returns a proxy-object to allow chained array-style indexing
     */
    NdOffsetMatrixProxy<T, N - 1> operator[](int index) {
        //Call the const version, since returned by value don't need to worry about const
        return const_cast<const NdOffsetMatrix<T, N>*>(this)->operator[](index);
    }
};

/**
 * @brief A 1-dimensional matrix supporting arbitrary (continuous) index ranges per dimension.
 *
 * This is considered a specialization for N=1
 */
template<typename T>
class NdOffsetMatrix<T, 1> : public NdOffsetMatrixBase<T, 1> {
  public:
    ///@brief Use the base constructors
    using NdOffsetMatrixBase<T, 1>::NdOffsetMatrixBase;

  public:
    ///@brief Access an element (immutable)
    const T& operator[](int index) const {
        VTR_ASSERT_SAFE_MSG(this->dim_size(0) > 0, "Can not index into size zero dimension");
        VTR_ASSERT_SAFE_MSG(index >= this->dim_ranges_[0].begin_index(), "Index out of range (below dimension minimum)");
        VTR_ASSERT_SAFE_MSG(index < this->dim_ranges_[0].end_index(), "Index out of range (above dimension maximum)");

        int effective_index = index - this->dim_ranges_[0].begin_index();

        return this->data_[effective_index];
    }

    ///@brief Access an element (mutable)
    T& operator[](int index) {
        //Call the const version, and cast away const-ness
        return const_cast<T&>(const_cast<const NdOffsetMatrix<T, 1>*>(this)->operator[](index));
    }
};

///@brief Convenient short forms for common NdMatricies
template<typename T>
using OffsetMatrix = NdOffsetMatrix<T, 2>;

} // namespace vtr
#endif
