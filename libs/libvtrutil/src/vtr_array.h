#pragma once

#include <array>
#include <cstddef>
#include <stdexcept>

namespace vtr {

/**
 * @brief A std::array wrapper that can be indexed by a vtr::StrongId.
 *
 * @tparam K Key type (e.g., vtr::StrongId)
 * @tparam V Value type
 * @tparam N Number of elements
 */
template<typename K, typename V, std::size_t N>
class array {
  public:
    using key_type = K;
    using value_type = V;
    using size_type = std::size_t;
    using reference = V&;
    using const_reference = const V&;
    using iterator = typename std::array<V, N>::iterator;
    using const_iterator = typename std::array<V, N>::const_iterator;

    /**
     * @brief Construct a vtr::array from a list of values.
     *
     * This constructor allows direct brace-initialization of the array:
     * @code
     * vtr::array<MyId, int, 3> arr{1, 2, 3};
     * @endcode
     *
     * @tparam Args Types of the values being passed. All must be convertible to V.
     * @param args The values to initialize the array with. Must match the array size.
     *//**
    * @brief Construct a vtr::array from a list of values.
    *
    * This constructor allows direct brace-initialization of the array:
    * @code
    * vtr::array<MyId, int, 3> arr{1, 2, 3};
    * @endcode
    *
    * @tparam Args Types of the values being passed. All must be convertible to V.
    * @param args The values to initialize the array with. Must match the array size.
    */
    template<typename... Args,
             typename = std::enable_if_t<sizeof...(Args) == N &&
                                         std::conjunction_v<std::is_convertible<Args, V>...>>>
    constexpr array(Args&&... args)
        : data_{ { std::forward<Args>(args)... } } {}



    ///@brief Access element with strong ID
    reference operator[](K id) {
        return data_[static_cast<size_type>(id)];
    }

    ///@brief Access element with strong ID (const)
    const_reference operator[](K id) const {
        return data_[static_cast<size_type>(id)];
    }

    ///@brief Access element with bounds checking
    reference at(K id) {
        return data_.at(static_cast<size_type>(id));
    }

    ///@brief Access element with bounds checking (const)
    const_reference at(K id) const {
        return data_.at(static_cast<size_type>(id));
    }

    // Iterators
    iterator begin() { return data_.begin(); }
    iterator end() { return data_.end(); }
    const_iterator begin() const { return data_.begin(); }
    const_iterator end() const { return data_.end(); }
    const_iterator cbegin() const { return data_.cbegin(); }
    const_iterator cend() const { return data_.cend(); }

    // Size
    constexpr size_type size() const { return N; }
    constexpr bool empty() const { return N == 0; }

    // Data
    V* data() { return data_.data(); }
    const V* data() const { return data_.data(); }

    // Front/Back
    reference front() { return data_.front(); }
    const_reference front() const { return data_.front(); }
    reference back() { return data_.back(); }
    const_reference back() const { return data_.back(); }

    // Fill
    void fill(const V& value) { data_.fill(value); }

  private:
    std::array<V, N> data_;
};

} // namespace vtr
