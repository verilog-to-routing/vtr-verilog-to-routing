#ifndef VTR_ND_MATRIX_H
#define VTR_ND_MATRIX_H
#include <array>
#include <memory>

#include "vtr_assert.h"

namespace vtr {

//Proxy class for a sub-matrix of a NdMatrix class.
//This is used to allow chaining of array indexing [] operators in a natural way.
//
//Each instance of this class peels off one-dimension and returns a NdMatrixProxy representing
//the resulting sub-matrix. This is repeated recursively until we hit the 1-dimensional base-case.
//
//Since this expansion happens at compiler time all the proxy classes get optimized away,
//yielding both high performance and generality.
//
//Recursive case: N-dimensional array
template<typename T, size_t N>
class NdMatrixProxy {
  public:
    static_assert(N > 0, "Must have at least one dimension");

    //Construct a matrix proxy object
    //
    //  dim_sizes: Array of dimension sizes
    //  idim: The dimension associated with this proxy
    //  dim_stride: The stride of this dimension (i.e. how many element in memory between indicies of this dimension)
    //  start: Pointer to the start of the sub-matrix this proxy represents
    NdMatrixProxy<T, N>(const size_t* dim_sizes, const size_t* dim_strides, T* start)
        : dim_sizes_(dim_sizes)
        , dim_strides_(dim_strides)
        , start_(start) {}

    const NdMatrixProxy<T, N - 1> operator[](size_t index) const {
        VTR_ASSERT_SAFE_MSG(index >= 0, "Index out of range (below dimension minimum)");
        VTR_ASSERT_SAFE_MSG(index < dim_sizes_[0], "Index out of range (above dimension maximum)");
        VTR_ASSERT_SAFE_MSG(dim_sizes_[1] > 0, "Can not index into zero-sized dimension");

        //Strip off one dimension
        return NdMatrixProxy<T, N - 1>(
            dim_sizes_ + 1,                    //Pass the dimension information
            dim_strides_ + 1,                  //Pass the stride for the next dimension
            start_ + dim_strides_[0] * index); //Advance to index in this dimension
    }

    NdMatrixProxy<T, N - 1> operator[](size_t index) {
        //Call the const version and cast-away constness
        return const_cast<const NdMatrixProxy<T, N>*>(this)->operator[](index);
    }

  private:
    const size_t* dim_sizes_;
    const size_t* dim_strides_;
    T* start_;
};

//Base case: 1-dimensional array
template<typename T>
class NdMatrixProxy<T, 1> {
  public:
    NdMatrixProxy<T, 1>(const size_t* dim_sizes, const size_t* dim_stride, T* start)
        : dim_sizes_(dim_sizes)
        , dim_strides_(dim_stride)
        , start_(start) {}

    const T& operator[](size_t index) const {
        VTR_ASSERT_SAFE_MSG(dim_strides_[0] == 1, "Final dimension must have stride 1");
        VTR_ASSERT_SAFE_MSG(index >= 0, "Index out of range (below dimension minimum)");
        VTR_ASSERT_SAFE_MSG(index < dim_sizes_[0], "Index out of range (above dimension maximum)");

        //Base case
        return start_[index];
    }

    T& operator[](size_t index) {
        //Call the const version and cast-away constness
        return const_cast<T&>(const_cast<const NdMatrixProxy<T, 1>*>(this)->operator[](index));
    }

    //For legacy compatibility (i.e. code expecting a pointer) we allow this base dimension
    //case to retrieve a raw pointer to the last dimension elements.
    //
    //Note that it is the caller's responsibility to use this correctly; care must be taken
    //not to clobber elements in other dimensions
    const T* data() const {
        return start_;
    }

    T* data() {
        //Call the const version and cast-away constness
        return const_cast<T*>(const_cast<const NdMatrixProxy<T, 1>*>(this)->data());
    }

  private:
    const size_t* dim_sizes_;
    const size_t* dim_strides_;
    T* start_;
};

//Base class for an N-dimensional matrix supporting arbitrary index ranges per dimension.
//This class implements all of the matrix handling (lifetime etc.) except for indexing
//(which is implemented in the NdMatrix class). Indexing is split out to allows specialization
//of indexing for N = 1.
//
//Implementation:
//
//This class uses a single linear array to store the matrix in c-style (row major)
//order. That is, the right-most index is laid out contiguous memory.
//
//This should improve memory usage (no extra pointers to store for each dimension),
//and cache locality (less indirection via pointers, predictable strides).
//
//The indicies are calculated based on the dimensions to access the appropriate elements.
//Since the indexing calculations are visible to the compiler at compile time they can be
//optimized to be efficient.
template<typename T, size_t N>
class NdMatrixBase {
  public:
    static_assert(N >= 1, "Minimum dimension 1");

    //An empty matrix (all dimensions size zero)
    NdMatrixBase() {
        clear();
    }

    //Specified dimension sizes:
    //      [0..dim_sizes[0])
    //      [0..dim_sizes[1])
    //      ...
    //with optional fill value
    NdMatrixBase(std::array<size_t, N> dim_sizes, T value = T()) {
        resize(dim_sizes, value);
    }

  public: //Accessors
    //Returns the size of the matrix (number of elements)
    size_t size() const {
        VTR_ASSERT_DEBUG_MSG(calc_size() == size_, "Calculated and current matrix size must be consistent");
        return size_;
    }

    //Returns true if there are no elements in the matrix
    bool empty() const {
        return size() == 0;
    }

    //Returns the number of dimensions (i.e. N)
    size_t ndims() const {
        return dim_sizes_.size();
    }

    //Returns the size of the ith dimension
    size_t dim_size(size_t i) const {
        VTR_ASSERT_SAFE(i < ndims());

        return dim_sizes_[i];
    }

    //Returns the starting index of ith dimension
    size_t begin_index(size_t i) const {
        VTR_ASSERT_SAFE(i < ndims());

        return 0;
    }

    //Returns the one-past-the-end index of the ith dimension
    size_t end_index(size_t i) const {
        VTR_ASSERT_SAFE(i < ndims());

        return dim_sizes_[i];
    }

    // Flat accessors of NdMatrix
    const T& get(size_t i) const {
        VTR_ASSERT_SAFE(i < size_);
        return data_[i];
    }

    T& get(size_t i) {
        VTR_ASSERT_SAFE(i < size_);
        return data_[i];
    }

  public: //Mutators
    //Set all elements to 'value'
    void fill(T value) {
        std::fill(data_.get(), data_.get() + size(), value);
    }

    //Resize the matrix to the specified dimension ranges
    //
    //If 'value' is specified all elements will be initialized to it,
    //otherwise they will be default constructed.
    void resize(std::array<size_t, N> dim_sizes, T value = T()) {
        dim_sizes_ = dim_sizes;
        size_ = calc_size();
        alloc();
        fill(value);
        if (size_ > 0) {
            dim_strides_[0] = size_ / dim_sizes_[0];
            for (size_t dim = 1; dim < N; ++dim) {
                dim_strides_[dim] = dim_strides_[dim - 1] / dim_sizes_[dim];
            }
        } else {
            dim_strides_.fill(0);
        }
    }

    //Reset the matrix to size zero
    void clear() {
        data_.reset(nullptr);
        dim_sizes_.fill(0);
        dim_strides_.fill(0);
        size_ = 0;
    }

  public: //Lifetime management
    //Copy constructor
    NdMatrixBase(const NdMatrixBase& other)
        : NdMatrixBase(other.dim_sizes_) {
        std::copy(other.data_.get(), other.data_.get() + other.size(), data_.get());
    }

    //Move constructor
    NdMatrixBase(NdMatrixBase&& other)
        : NdMatrixBase() {
        swap(*this, other);
    }

    //Copy/move assignment
    //
    //Note that rhs is taken by value (copy-swap idiom)
    NdMatrixBase& operator=(NdMatrixBase rhs) {
        swap(*this, rhs);
        return *this;
    }

    //Swap two NdMatrixBase objects
    friend void swap(NdMatrixBase<T, N>& m1, NdMatrixBase<T, N>& m2) {
        using std::swap;
        swap(m1.size_, m2.size_);
        swap(m1.dim_sizes_, m2.dim_sizes_);
        swap(m1.dim_strides_, m2.dim_strides_);
        swap(m1.data_, m2.data_);
    }

  private:
    //Allocate space for all the elements
    void alloc() {
        data_ = std::make_unique<T[]>(size());
    }

    //Returns the size of the matrix (number of elements) calucated
    //from the current dimensions
    size_t calc_size() const {
        //Size is the product of all dimension sizes
        size_t cnt = dim_size(0);
        for (size_t idim = 1; idim < ndims(); ++idim) {
            cnt *= dim_size(idim);
        }
        return cnt;
    }

  protected:
    size_t size_ = 0;
    std::array<size_t, N> dim_sizes_;
    std::array<size_t, N> dim_strides_;
    std::unique_ptr<T[]> data_ = nullptr;
};

//An N-dimensional matrix supporting arbitrary (continuous) index ranges
//per dimension.
//
//Examples:
//
//      //A 2-dimensional matrix with indicies [0..4][0..9]
//      NdMatrix<int,2> m1({5,10});
//
//      //Accessing an element
//      int i = m1[3][5];
//
//      //Setting an element
//      m1[2][8] = 0;
//
//      //A 3-dimensional matrix with indicies [0..4][0..9][0..19]
//      NdMatrix<int,3> m2({5,10,20});
//
//      //A 2-dimensional matrix with indicies [0..4][0..9], with all entries
//      //initialized to 42
//      NdMatrix<int,2> m3({5,10}, 42);
//
//      //Filling all entries with value 101
//      m3.fill(101);
//
//      //Resizing an existing matrix (all values reset to default constucted value)
//      m3.resize({5,5})
//
//      //Resizing an existing matrix (all elements set to value 88)
//      m3.resize({15,55}, 88)
template<typename T, size_t N>
class NdMatrix : public NdMatrixBase<T, N> {
    //General case
    static_assert(N >= 2, "Minimum dimension 2");

  public:
    //Use the base constructors
    using NdMatrixBase<T, N>::NdMatrixBase;

  public:
    //Access an element
    //
    //Returns a proxy-object to allow chained array-style indexing  (N >= 2 case)
    const NdMatrixProxy<T, N - 1> operator[](size_t index) const {
        VTR_ASSERT_SAFE_MSG(this->dim_size(0) > 0, "Can not index into size zero dimension");
        VTR_ASSERT_SAFE_MSG(this->dim_size(1) > 0, "Can not index into size zero dimension");
        VTR_ASSERT_SAFE_MSG(index >= 0, "Index out of range (below dimension minimum)");
        VTR_ASSERT_SAFE_MSG(index < this->dim_sizes_[0], "Index out of range (above dimension maximum)");

        //Peel off the first dimension
        return NdMatrixProxy<T, N - 1>(
            this->dim_sizes_.data() + 1,                        //Pass the dimension information
            this->dim_strides_.data() + 1,                      //Pass the stride for the next dimension
            this->data_.get() + this->dim_strides_[0] * index); //Advance to index in this dimension
    }

    //Access an element
    //
    //Returns a proxy-object to allow chained array-style indexing
    NdMatrixProxy<T, N - 1> operator[](size_t index) {
        //Call the const version, since returned by value don't need to worry about const
        return const_cast<const NdMatrix<T, N>*>(this)->operator[](index);
    }
};

template<typename T>
class NdMatrix<T, 1> : public NdMatrixBase<T, 1> {
    //Specialization for N = 1
  public:
    //Use the base constructors
    using NdMatrixBase<T, 1>::NdMatrixBase;

  public:
    //Access an element (immutable)
    const T& operator[](size_t index) const {
        VTR_ASSERT_SAFE_MSG(this->dim_size(0) > 0, "Can not index into size zero dimension");
        VTR_ASSERT_SAFE_MSG(index >= 0, "Index out of range (below dimension minimum)");
        VTR_ASSERT_SAFE_MSG(index < this->dim_sizes_[0], "Index out of range (above dimension maximum)");

        return this->data_[index];
    }

    //Access an element (mutable)
    T& operator[](size_t index) {
        //Call the const version, and cast away const-ness
        return const_cast<T&>(const_cast<const NdMatrix<T, 1>*>(this)->operator[](index));
    }
};

//Convenient short forms for common NdMatricies
template<typename T>
using Matrix = NdMatrix<T, 2>;

} // namespace vtr
#endif
