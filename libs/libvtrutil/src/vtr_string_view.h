#ifndef VTR_STRING_VIEW_H_
#define VTR_STRING_VIEW_H_

#include <cstring>
#include <ostream>
#include <string>
#include <stdexcept>

#include "vtr_hash.h"

namespace vtr {

/**
 * @brief Implements a view to a fixed length string (similar to std::basic_string_view).
 *
 * The underlying string does not need to be NULL terminated.
 */
class string_view {
  public:
    static constexpr size_t npos = size_t(-1);

    ///@brief constructor
    explicit constexpr string_view()
        : data_(nullptr)
        , size_(0) {}

    ///@brief constructor
    explicit string_view(const char* str)
        : data_(str)
        , size_(strlen(str)) {}
    ///@brief constructor
    explicit constexpr string_view(const char* str, size_t size)
        : data_(str)
        , size_(size) {}

    constexpr string_view(const string_view& other) noexcept = default;
    ///@brief copy constructor
    constexpr string_view& operator=(const string_view& view) noexcept {
        data_ = view.data_;
        size_ = view.size_;
        return *this;
    }

    ///@brief indexing [] operator (immutable)
    constexpr char operator[](size_t pos) const {
        return data_[pos];
    }

    ///@brief aT() operator (immutable)
    const char& at(size_t pos) const {
        if (pos >= size()) {
            throw std::out_of_range("Pos is out of range.");
        }

        return data_[pos];
    }

    ///@brief Returns the first character of the string
    constexpr const char& front() const {
        return data_[0];
    }

    ///@brief Returns the last character of the string
    constexpr const char& back() const {
        return data_[size() - 1];
    }

    ///@brief Returns a pointer to the string data
    constexpr const char* data() const {
        return data_;
    }

    ///@brief Returns the string size
    constexpr size_t size() const noexcept {
        return size_;
    }

    ///@brief Returns the string size
    constexpr size_t length() const noexcept {
        return size_;
    }

    ///@brief Returns true if empty
    constexpr bool empty() const noexcept {
        return size_ == 0;
    }

    ///@brief Returns a pointer to the begin of the string
    constexpr const char* begin() const noexcept {
        return data_;
    }

    ///@brief Same as begin()
    constexpr const char* cbegin() const noexcept {
        return data_;
    }

    ///@brief Returns a pointer to the end of the string
    constexpr const char* end() const noexcept {
        return data_ + size_;
    }

    ///@brief Same as end()
    constexpr const char* cend() const noexcept {
        return data_ + size_;
    }

    ///@brief Swaps two string views
    void swap(string_view& v) noexcept {
        std::swap(data_, v.data_);
        std::swap(size_, v.size_);
    }

    ///@brief Returns a newly constructed string object with its value initialized to a copy of a substring of this object.
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

///@brief == operator
inline bool operator==(string_view lhs,
                       string_view rhs) noexcept {
    return lhs.size() == rhs.size() && (lhs.empty() || rhs.empty() || (strncmp(lhs.data(), rhs.data(), std::min(lhs.size(), rhs.size())) == 0));
}

///@brief != operator
inline bool operator!=(string_view lhs,
                       string_view rhs) noexcept {
    return lhs.size() != rhs.size() || strncmp(lhs.data(), rhs.data(), std::min(lhs.size(), rhs.size())) != 0;
}

///@brief < operator
inline bool operator<(string_view lhs,
                      string_view rhs) noexcept {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

///brief >= operator
inline bool operator>=(string_view lhs,
                       string_view rhs) noexcept {
    return !std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

///@brief > operator
inline bool operator>(string_view lhs,
                      string_view rhs) noexcept {
    return rhs < lhs;
}

///@brief <= operator
inline bool operator<=(string_view lhs,
                       string_view rhs) noexcept {
    return rhs >= lhs;
}

///@brief << operator for ostream
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
