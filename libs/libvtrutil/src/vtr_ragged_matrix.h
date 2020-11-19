#ifndef VTR_RAGGED_MATRIX_H
#define VTR_RAGGED_MATRIX_H
#include <vector>
#include <iterator>

#include "vtr_assert.h"
#include "vtr_array_view.h"

namespace vtr {

/**
 * @brief A 2 dimensional 'ragged' matrix with rows indexed by Index0, and each row of variable length (indexed by Index1)
 * 
 * Example:
 * 
 *       std::vector<int> row_sizes = {1, 5, 3, 10};
 *       FlatRaggedMatrix<float> matrix(row_sizes);
 * 
 *       //Fill in all entries with ascending values
 *       float value = 1.;
 *       for (size_t irow = 0; irow < row_sizes.size(); ++irow) {
 *           for (size_t icol = 0; icol < row_sizes[irow]; ++icoll) {
 *               matrix[irow][icol] = value;
 *               value += 1.;
 *           }
 *       }
 * 
 * 
 * For efficiency, this class uses a flat memory layout,
 * where all elements are laid out contiguiously (one row
 * after another).
 * 
 * Expects Index0 and Index1 to be convertable to size_t.
 */
template<typename T, typename Index0 = size_t, typename Index1 = size_t>
class FlatRaggedMatrix {
  public:
    ///@brief default constructor
    FlatRaggedMatrix() = default;

    /**
     * @brief Constructs matrix with 'nrows' rows. 
     *
     * The row length is determined by calling 
     * 'row_length_callback' with the associated row index.
     */
    template<class Callback>
    FlatRaggedMatrix(size_t nrows, Callback& row_length_callback, T default_value = T())
        : FlatRaggedMatrix(RowLengthIterator<Callback>(0, row_length_callback),
                           RowLengthIterator<Callback>(nrows, row_length_callback),
                           default_value) {}

    ///@brief Constructs matrix from a container of row lengths
    template<class Container>
    FlatRaggedMatrix(Container container, T default_value = T())
        : FlatRaggedMatrix(std::begin(container), std::end(container), default_value) {}

    /**
     * @brief Constructs matrix from an iterator range. 
     *
     * The length of the range is the number of rows, and iterator values are the row lengths. 
     */
    template<class Iter>
    FlatRaggedMatrix(Iter row_size_first, Iter row_size_last, T default_value = T()) {
        size_t nrows = std::distance(row_size_first, row_size_last);
        first_elem_.resize(nrows + 1, -1); //+1 for sentinel

        size_t nelem = 0;
        size_t irow = 0;
        for (Iter iter = row_size_first; iter != row_size_last; ++iter) {
            first_elem_[irow] = nelem;

            nelem += *iter;
            ++irow;
        }

        //Sentinel
        first_elem_[irow] = nelem;

        data_.resize(nelem + 1, default_value); //+1 for sentinel
    }

  public: //Accessors
    ///@brief Iterators to *all* elements
    auto begin() {
        return data_.begin();
    }

    ///@brief Iterator to the last element of the matrix
    auto end() {
        if (empty()) {
            return data_.end();
        }
        return data_.end() - 1;
    }

    ///@brief Iterator to the first element of the matrix (immutable)
    auto begin() const {
        return data_.begin();
    }

    ///@brief Iterator to the last element of the matrix (immutable)
    auto end() const {
        if (empty()) {
            return data_.end();
        }
        return data_.end() - 1;
    }

    ///@brief Return the size of the matrix
    size_t size() const {
        if (data_.empty()) {
            return 0;
        }
        return data_.size() - 1; //-1 for sentinel
    }

    ///@brief Return true if empty
    bool empty() const {
        return size() == 0;
    }

    ///@brief Indexing operators for the first dimension
    vtr::array_view<T> operator[](Index0 i) {
        int idx = size_t(i);
        T* first = &data_[first_elem_[idx]];
        T* last = &data_[first_elem_[idx + 1]];
        return vtr::array_view<T>(first,
                                  last - first);
    }

    ///@brief Indexing operators for the first dimension (immutable)
    vtr::array_view<const T> operator[](Index0 i) const {
        int idx = size_t(i);
        const T* first = &data_[first_elem_[idx]];
        const T* last = &data_[first_elem_[idx + 1]];
        return vtr::array_view<const T>(first,
                                        last - first);
    }

    ///@brief Clears the matrix
    void clear() {
        data_.clear();
        first_elem_.clear();
    }

    ///@brief Swaps two matrices
    void swap(FlatRaggedMatrix<T, Index0, Index1>& other) {
        std::swap(data_, other.data_);
        std::swap(first_elem_, other.first_elem_);
    }

    ///@brief Swaps two matrices
    friend void swap(FlatRaggedMatrix<T, Index0, Index1>& lhs, FlatRaggedMatrix<T, Index0, Index1>& rhs) {
        lhs.swap(rhs);
    }

  public: //Types
    ///@brief Proxy class used to represent a 'row' in the matrix
    template<typename U>
    class ProxyRow {
      public:
        ///@brief constructor
        ProxyRow(U* first, U* last)
            : first_(first)
            , last_(last) {}

        ///@brief Return iterator to the first element
        U* begin() { return first_; }
        ///@brief Return iterator to the last element
        U* end() { return last_; }

        ///@brief Return iterator to the first element (immutable)
        const U* begin() const { return first_; }
        ///@brief Return iterator to the last element (immutable)
        const U* end() const { return last_; }

        ///@brief Return the size of the row
        size_t size() const { return last_ - first_; }

        ///@brief indexing [] operator
        U& operator[](Index1 j) {
            VTR_ASSERT_SAFE(size_t(j) < size());
            return first_[size_t(j)];
        }

        ///@brief indexing [] operator (immutable)
        const U& operator[](Index1 j) const {
            VTR_ASSERT_SAFE(size_t(j) < size());
            return first_[size_t(j)];
        }

        ///@brief Return iterator to the first element
        U* data() {
            return first_;
        }

        ///@brief Return iterator to the first element (immutable)
        U* data() const {
            return first_;
        }

      private:
        U* first_;
        U* last_;
    };

  private:
    /*
     * Iterator for constructing FlatRaggedMatrix.
     *
     * uses a callback to determine row lengths.
     */
    template<class Callback>
    class RowLengthIterator : public std::iterator<std::random_access_iterator_tag, size_t> {
      public:
        RowLengthIterator(size_t irow, Callback& callback)
            : irow_(irow)
            , callback_(callback) {}

        RowLengthIterator& operator++() {
            ++irow_;
            return *this;
        }

        bool operator==(const RowLengthIterator& other) {
            return irow_ == other.irow_;
        }

        bool operator!=(const RowLengthIterator& other) {
            return !(*this == other);
        }

        int operator-(const RowLengthIterator& other) {
            return irow_ - other.irow_;
        }

        size_t operator*() {
            //Call the callback to get the row length
            return callback_(Index0(irow_));
        }

      private:
        size_t irow_;
        Callback& callback_;
    };

  private:
    std::vector<T> data_;
    std::vector<int> first_elem_;
};

} // namespace vtr

#endif
