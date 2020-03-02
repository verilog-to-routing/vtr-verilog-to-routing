#ifndef _VTR_ARRAY_VIEW_H
#define _VTR_ARRAY_VIEW_H

#include <cstddef>
#include <stdexcept>

namespace vtr {

// Implements a fixed length view to an array.
template<typename T>
class array_view {
  public:
    explicit constexpr array_view()
        : data_(nullptr)
        , size_(0) {}
    explicit constexpr array_view(T* str, size_t size)
        : data_(str)
        , size_(size) {}

    constexpr array_view(const array_view& other) noexcept = default;
    constexpr array_view& operator=(const array_view& view) noexcept {
        data_ = view.data_;
        size_ = view.size_;
        return *this;
    }

    constexpr T& operator[](size_t pos) {
        return data_[pos];
    }
    constexpr const T& operator[](size_t pos) const {
        return data_[pos];
    }

    T& at(size_t pos) {
        if (pos >= size()) {
            throw std::out_of_range("Pos is out of range.");
        }

        return data_[pos];
    }

    const T& at(size_t pos) const {
        if (pos >= size()) {
            throw std::out_of_range("Pos is out of range.");
        }

        return data_[pos];
    }

    constexpr T& front() {
        return data_[0];
    }
    constexpr const T& front() const {
        return data_[0];
    }

    constexpr T& back() {
        return data_[size() - 1];
    }
    constexpr const T& back() const {
        return data_[size() - 1];
    }

    constexpr T* data() {
        return data_;
    }
    constexpr const T* data() const {
        return data_;
    }

    constexpr size_t size() const noexcept {
        return size_;
    }
    constexpr size_t length() const noexcept {
        return size_;
    }

    constexpr bool empty() const noexcept {
        return size_ != 0;
    }

    constexpr T* begin() noexcept {
        return data_;
    }
    constexpr const T* begin() const noexcept {
        return data_;
    }
    constexpr const T* cbegin() const noexcept {
        return data_;
    }

    constexpr T* end() noexcept {
        return data_ + size_;
    }
    constexpr const T* end() const noexcept {
        return data_ + size_;
    }
    constexpr const T* cend() const noexcept {
        return data_ + size_;
    }

  private:
    T* data_;
    size_t size_;
};

} // namespace vtr

#endif /* _VTR_ARRAY_VIEW_H */
