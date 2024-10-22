#pragma once

#include <algorithm>
#include <vector>

template<unsigned D, class T, class Container = std::vector<T>, class Compare = std::less<typename Container::value_type>>
class customized_d_ary_priority_queue {
    static_assert(D == 2 || D == 4, "Only support binary or 4-ary priority queue");

  public:
    typedef Container container_type;
    typedef typename Container::value_type value_type;
    typedef typename Container::size_type size_type;
    typedef typename Container::reference reference;
    typedef typename Container::const_reference const_reference;

    Compare comp_;
    /**
     * @details
     * heap_ is indexed from [1..heap_size]; the 0th element is unused. This simplifies arithmetic
     * in first_child_index() and parent_index() functions.
     *
     * @todo
     * If an 8-ary heap is implemented, experiment with starting at index 0
     */
    Container heap_;

  private:
    inline size_t parent_index(const size_t i) {
        if constexpr (D == 2) {
            return i >> 1;
        } else {
            return (i + 2) >> 2;
        }
    }

    inline size_t first_child_index(const size_t i) {
        if constexpr (D == 2) {
            return i << 1;
        } else {
            return (i << 2) - 2;
        }
    }

    inline size_t largest_child_index(const size_t first_child) {
        if constexpr (D == 2) {
            return first_child + !!comp_(heap_[first_child], heap_[first_child + 1]);
        } else {
            const size_t child_1 = first_child;
            const size_t child_2 = child_1 + 1;
            const size_t child_3 = child_1 + 2;
            const size_t child_4 = child_1 + 3;
            const size_t first_half_largest = child_1 + !!comp_(heap_[child_1], heap_[child_2]);
            const size_t second_half_largest = child_3 + !!comp_(heap_[child_3], heap_[child_4]);
            return comp_(heap_[first_half_largest], heap_[second_half_largest]) ? second_half_largest : first_half_largest;
        }
    }

    inline size_t largest_child_index_partial(const size_t first_child, const size_t num_children /*must < `D`*/) {
        if constexpr (D == 2) {
            (void) num_children;
            return first_child;
        } else {
            switch (num_children) {
                case 3: {
                    const size_t child_1 = first_child;
                    const size_t child_2 = child_1 + 1;
                    const size_t child_3 = child_1 + 2;
                    const size_t first_two_children_largest = child_1 + !!comp_(heap_[child_1], heap_[child_2]);
                    return comp_(heap_[first_two_children_largest], heap_[child_3]) ? child_3 : first_two_children_largest;
                }
                case 2: {
                    return first_child + !!comp_(heap_[first_child], heap_[first_child + 1]);
                }
                default: {
                    return first_child;
                }
            }
        }
    }

    inline void pop_customized_heap() {
        size_t length = heap_.size() - 1;
        auto end = heap_.end();
        auto value = std::move(end[-1]);
        end[-1] = std::move(heap_[1]);
        size_t index = 1;
        for (;;) {
            size_t first_child = first_child_index(index);
            size_t last_child = first_child + (D - 1);
            if (last_child < length) {
                size_t largest_child = largest_child_index(first_child);
                if (!comp_(value, heap_[largest_child])) {
                    break;
                }
                heap_[index] = std::move(heap_[largest_child]);
                index = largest_child;
            } else if (first_child < length) {
                size_t largest_child = largest_child_index_partial(first_child, length - first_child);
                if (comp_(value, heap_[largest_child])) {
                    heap_[index] = std::move(heap_[largest_child]);
                    index = largest_child;
                }
                break;
            } else {
                break;
            }
        }
        heap_[index] = std::move(value);
    }

    inline void push_customized_heap() {
        auto value = std::move(heap_.back());
        size_t index = heap_.size() - 1;
        while (index > 1) {
            size_t parent = parent_index(index);
            if (!comp_(heap_[parent], value)) {
                break;
            }
            heap_[index] = std::move(heap_[parent]);
            index = parent;
        }
        heap_[index] = std::move(value);
    }

  public:
    explicit customized_d_ary_priority_queue(const Compare& compare = Compare(),
                                                const Container& cont = Container())
        : comp_(compare)
        , heap_(cont) {
        heap_.resize(1); // FIXME: currently do not support `make_heap` from cont (heap_)
    }

    inline bool empty() const {
        return heap_.size() == 1; // heap_[0] is invalid, heap is indexed from 1
    }

    inline size_type size() const {
        return heap_.size() - 1; // heap_[0] is invalid, heap is indexed from 1
    }

    inline const_reference top() const { return heap_[1]; }

    inline void pop() {
        pop_customized_heap();
        heap_.pop_back();
    }

    inline void push(const value_type& value) {
        heap_.push_back(value);
        push_customized_heap();
    }

    inline void push(value_type&& value) {
        heap_.push_back(std::move(value));
        push_customized_heap();
    }

    inline void clear() { heap_.resize(1); }

    inline void reserve(size_type new_cap) { heap_.reserve(new_cap + 1); }
};
