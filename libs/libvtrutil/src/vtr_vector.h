#ifndef VTR_VECTOR
#define VTR_VECTOR
#include <vector>
#include <cstddef>
#include <iterator>
#include "vtr_range.h"

namespace vtr {

//A std::vector container which is indexed by K (instead of size_t).
//
//The main use of this container is to behave like a std::vector which is
//indexed by a vtr::StrongId. It assumes that K is explicitly convertable to size_t
//(i.e. via operator size_t()), and can be explicitly constructed from a size_t.
//
//If you need more std::map-like (instead of std::vector-like) behaviour see
//vtr::vector_map.
template<typename K, typename V>
class vector : private std::vector<V> {
  public:
    typedef K key_type;

    class key_iterator;
    typedef vtr::Range<key_iterator> key_range;

  public:
    //Pass through std::vector's types
    using typename std::vector<V>::value_type;
    using typename std::vector<V>::allocator_type;
    using typename std::vector<V>::reference;
    using typename std::vector<V>::const_reference;
    using typename std::vector<V>::pointer;
    using typename std::vector<V>::const_pointer;
    using typename std::vector<V>::iterator;
    using typename std::vector<V>::const_iterator;
    using typename std::vector<V>::reverse_iterator;
    using typename std::vector<V>::const_reverse_iterator;
    using typename std::vector<V>::difference_type;
    using typename std::vector<V>::size_type;

    //Pass through std::vector's methods
    using std::vector<V>::vector;

    using std::vector<V>::begin;
    using std::vector<V>::end;
    using std::vector<V>::rbegin;
    using std::vector<V>::rend;
    using std::vector<V>::cbegin;
    using std::vector<V>::cend;
    using std::vector<V>::crbegin;
    using std::vector<V>::crend;

    using std::vector<V>::size;
    using std::vector<V>::max_size;
    using std::vector<V>::resize;
    using std::vector<V>::capacity;
    using std::vector<V>::empty;
    using std::vector<V>::reserve;
    using std::vector<V>::shrink_to_fit;

    using std::vector<V>::front;
    using std::vector<V>::back;
    using std::vector<V>::data;

    using std::vector<V>::assign;
    using std::vector<V>::push_back;
    using std::vector<V>::pop_back;
    using std::vector<V>::insert;
    using std::vector<V>::erase;
    using std::vector<V>::swap;
    using std::vector<V>::clear;
    using std::vector<V>::emplace;
    using std::vector<V>::emplace_back;
    using std::vector<V>::get_allocator;

    //Don't include operator[] and at() from std::vector,
    //since we redine them to take key_type instead of size_t
    reference operator[](const key_type id) {
        auto i = size_t(id);
        return std::vector<V>::operator[](i);
    }
    const_reference operator[](const key_type id) const {
        auto i = size_t(id);
        return std::vector<V>::operator[](i);
    }
    reference at(const key_type id) {
        auto i = size_t(id);
        return std::vector<V>::at(i);
    }
    const_reference at(const key_type id) const {
        auto i = size_t(id);
        return std::vector<V>::at(i);
    }

    //Returns a range containing the keys
    key_range keys() const {
        return vtr::make_range(key_begin(), key_end());
    }

  public:
    //Iterator class which is convertable to the key_type
    //This allows end-users to call the parent class's keys() member
    //to iterate through the keys with a range-based for loop
    class key_iterator : public std::iterator<std::bidirectional_iterator_tag, key_type> {
      public:
        //We use the intermediate type my_iter to avoid a potential ambiguity for which
        //clang generates errors and warnings
        using my_iter = typename std::iterator<std::bidirectional_iterator_tag, K>;
        using typename my_iter::iterator;
        using typename my_iter::pointer;
        using typename my_iter::reference;
        using typename my_iter::value_type;

        key_iterator(key_iterator::value_type init)
            : value_(init) {}

        //vtr::vector assumes that the key time is convertable to size_t and
        //that all the underlying IDs are zero-based and contiguous. That means
        //we can just increment the underlying Id to build the next key.
        key_iterator operator++() {
            value_ = value_type(size_t(value_) + 1);
            return *this;
        }
        key_iterator operator--() {
            value_ = value_type(size_t(value_) - 1);
            return *this;
        }
        reference operator*() { return value_; }
        pointer operator->() { return &value_; }

        friend bool operator==(const key_iterator lhs, const key_iterator rhs) { return lhs.value_ == rhs.value_; }
        friend bool operator!=(const key_iterator lhs, const key_iterator rhs) { return !(lhs == rhs); }

      private:
        value_type value_;
    };

  private:
    key_iterator key_begin() const { return key_iterator(key_type(0)); }
    key_iterator key_end() const { return key_iterator(key_type(size())); }
};

} // namespace vtr
#endif
