#ifndef RR_GRAPH_UTILS_H
#define RR_GRAPH_UTILS_H

// Make room in a vector, with amortized O(1) time by using a pow2 growth pattern.
//
// This enables potentially random insertion into a vector with amortized O(1)
// time.
template<typename T>
void make_room_in_vector(T* vec, size_t elem_position) {
    if (elem_position < vec->size()) {
        return;
    }

    size_t capacity = std::max(vec->capacity(), size_t(16));
    while (elem_position >= capacity) {
        capacity *= 2;
    }

    if (capacity >= vec->capacity()) {
        vec->reserve(capacity);
    }

    vec->resize(elem_position + 1);
}

#endif
