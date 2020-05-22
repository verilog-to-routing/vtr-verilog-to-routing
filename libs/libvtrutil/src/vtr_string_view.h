#ifndef VTR_STRING_VIEW_H_
#define VTR_STRING_VIEW_H_

#include <cstring>
#include <ostream>
#include <string>
#include <stdexcept>

#include "vtr_hash.h"

namespace vtr {

// Implements a view to a fixed length string.
// The underlying string does not need to be NULL terminated.
class string_view {
  public:
    static constexpr size_t npos = size_t(-1);

    explicit constexpr string_view()
        : data_(nullptr)
        , size_(0) {}
    explicit string_view(const char* str)
        : data_(str)
        , size_(strlen(str)) {}
    explicit constexpr string_view(const char* str, size_t size)
        : data_(str)
        , size_(size) {}

    constexpr string_view(const string_view& other) noexcept = default;
    constexpr string_view& operator=(const string_view& view) noexcept {
        data_ = view.data_;
        size_ = view.size_;
        return *this;
    }

    constexpr char operator[](size_t pos) const {
        return data_[pos];
    }

    const char& at(size_t pos) const {
        if (pos >= size()) {
            throw std::out_of_range("Pos is out of range.");
        }

        return data_[pos];
    }

    constexpr const char& front() const {
        return data_[0];
    }

    constexpr const char& back() const {
        return data_[size() - 1];
    }

    constexpr const char* data() const {
        return data_;
    }

    constexpr size_t size() const noexcept {
        return size_;
    }
    constexpr size_t length() const noexcept {
        return size_;
    }

    constexpr bool empty() const noexcept {
        return size_ == 0;
    }

    constexpr const char* begin() const noexcept {
        return data_;
    }
    constexpr const char* cbegin() const noexcept {
        return data_;
    }

    constexpr const char* end() const noexcept {
        return data_ + size_;
    }
    constexpr const char* cend() const noexcept {
        return data_ + size_;
    }

    void swap(string_view& v) noexcept {
        std::swap(data_, v.data_);
        std::swap(size_, v.size_);
    }

    string_view substr(size_t pos = 0, size_t count = npos) {
        if (pos > size()) {
            throw std::out_of_range("Pos is out of range.");
        }

        size_t rcount = size_ - pos;
        if (count != npos && (pos + count) < size_) {
            rcount = count;
        }

        return string_view(data_ + pos, rcount);
    }

  private:
    const char* data_;
    size_t size_;
};

inline bool operator==(string_view lhs,
                       string_view rhs) noexcept {
    return lhs.size() == rhs.size() && (lhs.empty() || rhs.empty() || (strncmp(lhs.data(), rhs.data(), std::min(lhs.size(), rhs.size())) == 0));
}
inline bool operator!=(string_view lhs,
                       string_view rhs) noexcept {
    return lhs.size() != rhs.size() || strncmp(lhs.data(), rhs.data(), std::min(lhs.size(), rhs.size())) != 0;
}
inline bool operator<(string_view lhs,
                      string_view rhs) noexcept {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}
inline bool operator>=(string_view lhs,
                       string_view rhs) noexcept {
    return !std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}
inline bool operator>(string_view lhs,
                      string_view rhs) noexcept {
    return rhs < lhs;
}
inline bool operator<=(string_view lhs,
                       string_view rhs) noexcept {
    return rhs >= lhs;
}

inline std::ostream& operator<<(std::ostream& os, string_view const& value) {
    for (const auto& c : value) {
        os << c;
    }
    return os;
}

} // namespace vtr

namespace std {
template<>
struct hash<vtr::string_view> {
    std::size_t operator()(vtr::string_view const& s) const noexcept {
        std::size_t h = 0;
        for (const auto& data : s) {
            vtr::hash_combine(h, std::hash<char>()(data));
        }
        return h;
    }
};
} // namespace std

#endif /* VTR_STRING_VIEW_H_ */
