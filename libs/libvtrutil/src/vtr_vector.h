#ifndef VTR_VECTOR
#define VTR_VECTOR
#include <vector>
#include <cstddef>

namespace vtr {

//A std::vector container which is indexed by K (instead of size_t).
//
//The main use of this container is to behave like a std::vector which is
//indexed by a vtr::StrongId.
//
//If you need more std::map-like (instead of std::vector-like) behaviour see
//vtr::vector_map.
template<typename K, typename V>
class vector : private std::vector<V> {
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
        //since we redine them to take K instead of size_t
        reference operator[](const K id) {
            auto i = size_t(id);
            return std::vector<V>::operator[](i);
        }
        const_reference operator[](const K id) const {
            auto i = size_t(id);
            return std::vector<V>::operator[](i);
        }
        reference at(const K id) {
            auto i = size_t(id);
            return std::vector<V>::at(i);
        }
        const_reference at(const K id) const {
            auto i = size_t(id);
            return std::vector<V>::at(i);
        }
};


} //namespace
#endif
