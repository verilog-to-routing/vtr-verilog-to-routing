#ifndef VTR_SET_H
#define VTR_SET_H

#include <vector>

namespace vtr {

/**
 * @brief Implements a set-like interface which supports multiple operations
 *
 * The supported operations are:
 *  - insertion
 *  - iteration
 *  - membership test
 * all in constant time.
 *
 * It assumes the element type (T) is convertable to size_t.
 * Usually, elements are vtr::StrongIds.
 *
 * Iteration through the elements is not strictly ordered, usually
 * insertion order, unless sort() has been previously called.
 *
 * The underlying implementation uses a vector for element
 * storage (for iteration), and a bit-set for membership tests.
 */
template<typename T>
class vec_id_set {
  public:
    typedef typename std::vector<T>::const_iterator const_iterator;
    typedef const_iterator iterator;

    ///@brief Returns an iterator to the first element in the sequence
    auto begin() const { return vec_.begin(); }
    ///@brief Returns an iterator referring to the past-the-end element in the vector container
    auto end() const { return vec_.end(); }

    ///@brief Returns a constant iterator to the first element in the sequence
    auto cbegin() const { return vec_.cbegin(); }
    ///@brief Returns a constant iterator referring to the past-the-end element in the vector container
    auto cend() const { return vec_.cend(); }

    ///@brief Insert val in the set
    bool insert(T val) {
        if (count(val)) { //Already inserted
            return false;
        }

        vec_.push_back(val);

        //Mark this value as being contained
        if (size_t(val) >= contained_.size()) {
            //We dynamically grow contained_ based on the maximum
            //value contained. This allows us to avoid expensive
            contained_.resize(size_t(val) + 1, false);
        }
        contained_[size_t(val)] = true;

        return true;
    }

    ///@brief Iterators specifying a range of elements. Copies of the elements in the range [first,last) are inserted in the container.
    template<typename Iter>
    void insert(Iter first, Iter last) {
        size_t nelem = std::distance(first, last);
        vec_.reserve(size() + nelem);
        contained_.reserve(size() + nelem);

        for (Iter itr = first; itr != last; ++itr) {
            insert(*itr);
        }
    }

    ///@brief Count elements with a specific value
    size_t count(T val) const {
        if (size_t(val) < contained_.size()) {
            //Value is with-in range of previously inserted
            //elements, so look-up its membership
            return contained_[size_t(val)];
        }
        return 0;
    }

    ///@brief Returns the size of the container
    size_t size() const {
        return vec_.size();
    }

    ///@brief Sort elements in the container
    void sort() {
        std::sort(vec_.begin(), vec_.end());
    }

    ///@bried Clears the container
    void clear() {
        vec_.clear();
        contained_.clear();
    }

  private:
    std::vector<T> vec_;          //Elements contained
    std::vector<bool> contained_; //Bit-set for constant-time membership test
};

} // namespace vtr

#endif
