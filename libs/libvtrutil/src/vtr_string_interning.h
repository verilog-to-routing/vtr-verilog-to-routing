#ifndef VTR_STRING_INTERNING_H_
#define VTR_STRING_INTERNING_H_

/**
 * @file
 * @brief  Provides basic string interning, along with pattern splitting suitable for use with FASM.
 * 
 *  For reference, string interning refers to keeping a unique copy of a string
 *  in storage, and then handing out an id to that storage location, rather than
 *  keeping the string around.  This deduplicates memory overhead for strings.
 * 
 *  This string internment has an additional feature that is splitting the
 *  input string into "parts" based on '.', which happens to be the feature
 *  seperator for FASM.  This means the string "TILE.CLB.A" and "TILE.CLB.B"
 *  would be made up of the intern ids for {"TILE", "CLB", "A"} and
 *  {"TILE", "CLB", "B"} respectively, allowing some internal deduplication.
 * 
 *  Strings can contain up to kMaxParts, before they will be interned as their
 *  whole string.
 * 
 *  Interned strings (interned_string) that come from the same internment
 *  object (string_internment) can safely be checked for equality and hashed
 *  without touching the underlying string.  Lexigraphical comprisions (e.g. <)
 *  requires reconstructing the string.
 * 
 *  Basic usage:
 *  -# Create a string_internment
 *  -# Invoke string_internment::intern_string, which returns the
 *     interned_string object that is the interned string's unique idenfier.
 *     This idenfier can be checked for equality or hashed. If
 *     string_internment::intern_string is called with the same string, a value
 *     equivalent interned_string object will be returned.
 *  -# If the original string is required, interned_string::get can be invoked
 *     to copy the string into a std::string.
 *     interned_string also provides iteration via begin/end, however the begin
 *     method requires a pointer to original string_internment object.  This is
 *     not suitable for range iteration, so the method interned_string::bind
 *     can be used to create a bound_interned_string that can be used in a
 *     range iteration context.
 * 
 *     For reference, the reason that interned_string's does not have a
 *     reference back to the string_internment object is to keep their memory
 *     footprint lower.
 */
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <climits>
#include <algorithm>
#include <array>

#include "vtr_strong_id.h"
#include "vtr_string_view.h"
#include "vtr_vector.h"

namespace vtr {

// Forward declare classes for pointers.
class string_internment;
class interned_string;
class interned_string_less;

// StrongId for identifying unique string pieces.
struct interned_string_tag;
typedef StrongId<interned_string_tag> StringId;

/**
 * @brief Values that control the size of the used storage
 *
 * To keep interned_string memory footprint lower and flexible, these values
 * control the size of the storage used.
 *
 * Number of bytes to represent the StringId.  This implies a maximum number of unique strings available equal to (1 << (kBytesPerId*CHAR_BIT)).
 */
constexpr size_t kBytesPerId = 3;
///@brief Maximum number of splits to accomidate before just interning the entire string.
constexpr size_t kMaxParts = 3;
///@brief Number of bytes to represent the number of splits present in an interned string.
constexpr size_t kSizeSize = 1;
///@brief Which character to split the input string by.
constexpr char kSplitChar = '.';

static_assert((1 << (CHAR_BIT * kSizeSize)) > kMaxParts, "Size of size data is too small");

/**
 * @brief Iterator over interned string.
 *
 * This object is much heavier memory wise than interned_string, so do not
 * store these.
 * 
 * This iterator only accomidates the forward_iterator concept.
 * 
 * Do no construct this iterator directly.  Use either
 * bound_interned_string::begin/end or interned_string;:begin/end.
 */
class interned_string_iterator {
  public:
    interned_string_iterator(const string_internment* internment, std::array<StringId, kMaxParts> intern_ids, size_t n);

    interned_string_iterator() {
        clear();
    }

    using value_type = char;
    using difference_type = void;
    using pointer = const char*;
    using reference = const char&;
    using iterator_category = std::forward_iterator_tag;

    char operator*() const {
        if (num_parts_ == size_t(-1)) {
            throw std::out_of_range("Invalid iterator");
        }

        if (str_idx_ >= view_.size()) {
            return kSplitChar;
        } else {
            return view_.at(str_idx_);
        }
    }

    interned_string_iterator& operator++();
    interned_string_iterator operator++(int);

    friend bool operator==(const interned_string_iterator& lhs, const interned_string_iterator& rhs);

  private:
    void clear() {
        internment_ = nullptr;
        num_parts_ = size_t(-1);
        std::fill(parts_.begin(), parts_.end(), StringId());
        part_idx_ = size_t(-1);
        str_idx_ = size_t(-1);
        view_ = vtr::string_view();
    }

    const string_internment* internment_;
    size_t num_parts_;
    std::array<StringId, kMaxParts> parts_;
    size_t part_idx_;
    size_t str_idx_;
    vtr::string_view view_;
};

///@brief == operator
inline bool operator==(const interned_string_iterator& lhs, const interned_string_iterator& rhs) {
    return lhs.internment_ == rhs.internment_ && lhs.num_parts_ == rhs.num_parts_ && lhs.parts_ == rhs.parts_ && lhs.part_idx_ == rhs.part_idx_ && lhs.str_idx_ == rhs.str_idx_ && lhs.view_ == rhs.view_;
}

///@brief != operator
inline bool operator!=(const interned_string_iterator& lhs, const interned_string_iterator& rhs) {
    return !(lhs == rhs);
}

/**
 * @brief A interned_string bound to it's string_internment object.
 *
 * This object is heavier than just an interned_string.
 * This object holds a pointer to interned_string, so its lifetime must be
 * shorter than the parent interned_string.
 */
class bound_interned_string {
  public:
    ///@brief constructor
    bound_interned_string(const string_internment* internment, const interned_string* str)
        : internment_(internment)
        , str_(str) {}

    ///@brief return an iterator to the first part of the interned_string
    interned_string_iterator begin() const;
    ///@brief return an iterator to the last part of the interned_string
    interned_string_iterator end() const;

  private:
    const string_internment* internment_;
    const interned_string* str_;
};

/**
 * @brief Interned string value returned from a string_internment object.
 *
 * This is a value object without allocation.  It can be checked for equality
 * and hashed safely against other interned_string's generated from the same
 * string_internment.
 */
class interned_string {
  public:
    ///@brief constructor
    interned_string(std::array<StringId, kMaxParts> intern_ids, size_t n) {
        std::fill(storage_.begin(), storage_.end(), 0);
        set_num_parts(n);
        for (size_t i = 0; i < n; ++i) {
            set_id(i, intern_ids[i]);
        }
    }

    /**
     * @brief Copy the underlying string into output.
     *
     * internment must the object that generated this interned_string.
     */
    void get(const string_internment* internment, std::string* output) const;

    /**
     * @brief Returns the underlying string as a std::string.
     *
     * This method will allocated memory.
     */
    std::string get(const string_internment* internment) const {
        std::string result;
        get(internment, &result);
        return result;
    }

    /**
     * @brief Bind the parent string_internment and return a bound_interned_string object.
     * 
     * That bound_interned_string lifetime must be shorter than this
     * interned_string object lifetime, as bound_interned_string contains
     * a reference this object, along with a reference to the internment
     * object.
     */
    bound_interned_string bind(const string_internment* internment) const {
        return bound_interned_string(internment, this);
    }

    ///@brief begin() function
    interned_string_iterator begin(const string_internment* internment) const {
        size_t n = num_parts();
        std::array<StringId, kMaxParts> intern_ids;

        for (size_t i = 0; i < n; ++i) {
            intern_ids[i] = id(i);
        }

        return interned_string_iterator(internment, intern_ids, n);
    }

    ///@brief end() function
    interned_string_iterator end() const {
        return interned_string_iterator();
    }

    ///@brief == operator
    friend bool operator==(interned_string lhs,
                           interned_string rhs) noexcept;
    ///@brief != operator
    friend bool operator!=(interned_string lhs,
                           interned_string rhs) noexcept;
    ///@brief hash function
    friend std::hash<interned_string>;
    friend interned_string_less;

  private:
    void set_num_parts(size_t n) {
        for (size_t i = 0; i < kSizeSize; ++i) {
            storage_[i] = (n >> (i * CHAR_BIT)) & UCHAR_MAX;
        }

        if (num_parts() != n) {
            throw std::runtime_error("Storage size exceeded.");
        }
    }

    size_t num_parts() const {
        size_t n = 0;
        for (size_t i = 0; i < kSizeSize; ++i) {
            n |= storage_[i] << (i * CHAR_BIT);
        }

        return n;
    }

    void set_id(size_t idx, StringId id) {
        if (idx >= kMaxParts) {
            throw std::runtime_error("Storage size exceeded.");
        }

        size_t val = (size_t)id;
        for (size_t i = 0; i < kBytesPerId; ++i) {
            storage_[kSizeSize + i + idx * kBytesPerId] = (val >> (i * CHAR_BIT)) & UCHAR_MAX;
        }

        if (this->id(idx) != id) {
            throw std::runtime_error("Storage size exceeded.");
        }
    }

    StringId id(size_t idx) const {
        size_t val = 0;
        for (size_t i = 0; i < kBytesPerId; ++i) {
            val |= storage_[kSizeSize + i + idx * kBytesPerId] << (i * CHAR_BIT);
        }

        return StringId(val);
    }

    std::array<uint8_t, kSizeSize + kMaxParts * kBytesPerId> storage_;
};

///@brief == operator
inline bool operator==(interned_string lhs,
                       interned_string rhs) noexcept {
    return lhs.storage_ == rhs.storage_;
}

///@brief != operator
inline bool operator!=(interned_string lhs,
                       interned_string rhs) noexcept {
    return lhs.storage_ != rhs.storage_;
}

///@brief < operator
inline bool operator<(bound_interned_string lhs,
                      bound_interned_string rhs) noexcept {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

///@brief >= operator
inline bool operator>=(bound_interned_string lhs,
                       bound_interned_string rhs) noexcept {
    return !std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

///@brief > operator
inline bool operator>(bound_interned_string lhs,
                      bound_interned_string rhs) noexcept {
    return rhs < lhs;
}

///@brief <= operator
inline bool operator<=(bound_interned_string lhs,
                       bound_interned_string rhs) noexcept {
    return rhs >= lhs;
}

/**
 * @brief  Storage of interned string, and object capable of generating new interned_string objects.
 */
class string_internment {
  public:
    /**
     * @brief Intern a string, and return a unique identifier to that string.
     *
     * If interned_string is ever called with two strings of the same value,
     * the interned_string will be equal.
     */
    interned_string intern_string(vtr::string_view view) {
        size_t num_parts = 1;
        for (const auto& c : view) {
            if (c == kSplitChar) {
                num_parts += 1;
            }
        }

        std::array<StringId, kMaxParts> parts;
        if (num_parts == 1 || num_parts > kMaxParts) {
            // Intern entire string.
            parts[0] = intern_one_string(view);
            return interned_string(parts, 1);
        } else {
            // Implements parts = [intern_one_string(s) for s in view.split(kSplitChar)]
            size_t idx = 0;
            size_t start = 0;

            for (size_t i = 0; i < view.size(); ++i) {
                if (view[i] == kSplitChar) {
                    parts[idx++] = intern_one_string(view.substr(start, i - start));
                    start = i + 1;
                    if (idx == num_parts - 1) {
                        break;
                    }
                }
            }

            parts[idx++] = intern_one_string(view.substr(start));
            return interned_string(parts, num_parts);
        }
    }

    /**
     * @brief Retrieve a string part based on id.
     *
     * This method should not generally be called directly.
     */
    vtr::string_view get_string(StringId id) const {
        auto& str = strings_[id];
        return vtr::string_view(str.data(), str.size());
    }

    ///@brief Number of unique string parts stored.
    size_t unique_strings() const {
        return strings_.size();
    }

  private:
    StringId intern_one_string(vtr::string_view view) {
        temporary_.assign(view.begin(), view.end());
        StringId next_id(strings_.size());
        auto result = string_to_id_.insert(std::make_pair(temporary_, next_id));
        if (result.second) {
            strings_.push_back(std::move(temporary_));
        }

        return result.first->second;
    }

    // FIXME: This storage scheme does store 2x memory for the strings storage,
    // however it does avoid having to be concerned with what happens when
    // strings_ resizes, so for a simplier initial implementation, this is the
    // approach taken.
    vtr::vector<StringId, std::string> strings_;
    std::string temporary_;
    std::unordered_map<std::string, StringId> string_to_id_;
};

/**
 * @brief Copy the underlying string into output.
 *
 * internment must the object that generated this interned_string.
 */
inline void interned_string::get(const string_internment* internment, std::string* output) const {
    // Implements
    // kSplitChar.join(interned_string->get_string(id(idx)) for idx in range(num_parts())));
    size_t parts = num_parts();
    size_t storage_needed = parts - 1;
    std::array<StringId, kMaxParts> intern_ids;
    for (size_t i = 0; i < parts; ++i) {
        intern_ids[i] = id(i);
        storage_needed += internment->get_string(intern_ids[i]).size();
    }

    output->clear();
    output->reserve(storage_needed);

    for (size_t i = 0; i < parts; ++i) {
        auto view = internment->get_string(intern_ids[i]);
        std::copy(view.begin(), view.end(), std::back_inserter(*output));
        if (i + 1 < parts) {
            output->push_back(kSplitChar);
        }
    }
}

/**
 * @brief constructor for interned string iterator.
 *
 * Do no construct this iterator directly.  Use either
 * bound_interned_string::begin/end or interned_string;:begin/end.
 */
inline interned_string_iterator::interned_string_iterator(const string_internment* internment, std::array<StringId, kMaxParts> intern_ids, size_t n)
    : internment_(internment)
    , num_parts_(n)
    , parts_(intern_ids)
    , part_idx_(0)
    , str_idx_(0) {
    if (num_parts_ == 0) {
        clear();
    } else {
        view_ = internment_->get_string(parts_[0]);
    }
}

///@brief Increment operator for interned_string_iterator
inline interned_string_iterator& interned_string_iterator::operator++() {
    if (num_parts_ == size_t(-1)) {
        throw std::out_of_range("Invalid iterator");
    }

    if (str_idx_ < view_.size()) {
        // Current string has characters left, advance.
        str_idx_ += 1;
        // Normally when str_idx_ the iterator will next emit a kSplitChar,
        // but this is omitted on the last part of the string.
        if (str_idx_ == view_.size() && part_idx_ + 1 == num_parts_) {
            clear();
        }
    } else {
        // Current part of the string is out of characters, and the
        // kSplitChar has been emitted, advance to the next part.
        str_idx_ = 0;
        part_idx_ += 1;
        if (part_idx_ == num_parts_) {
            // No more parts.
            clear();
        } else {
            view_ = internment_->get_string(parts_[part_idx_]);
            if (view_.size() == 0 && part_idx_ + 1 == num_parts_) {
                // The last string part is empty, and because this is the last
                // part we don't want to emit another kSplitChar.
                clear();
            }
        }
    }

    return *this;
}

///@brief Increment operator for interned_string_iterator
inline interned_string_iterator interned_string_iterator::operator++(int) {
    interned_string_iterator prev = *this;
    ++*this;

    return prev;
}

///@brief return an iterator to the first part of the interned_string
inline interned_string_iterator bound_interned_string::begin() const {
    return str_->begin(internment_);
}

///@brief return an iterator to the last part of the interned_string
inline interned_string_iterator bound_interned_string::end() const {
    return interned_string_iterator();
}

inline std::ostream& operator<<(std::ostream& os, bound_interned_string const& value) {
    for (const auto& c : value) {
        os << c;
    }
    return os;
}

/**
 * @brief A friend class to interned_string that compares 2 interned_strings
 */
class interned_string_less {
  public:
    ///@brief Return true if the first interned string is less than the second one
    bool operator()(const vtr::interned_string& lhs, const vtr::interned_string& rhs) const {
        return lhs.storage_ < rhs.storage_;
    }
};

} // namespace vtr

namespace std {
/**
 * @brief Hash function for the interned_string 
 *
 * It is defined as a friend function to interned_string class.
 * It returns a unique hash for every interned_string.
 */
template<>
struct hash<vtr::interned_string> {
    std::size_t operator()(vtr::interned_string const& s) const noexcept {
        std::size_t h = 0;
        for (const auto& data : s.storage_) {
            vtr::hash_combine(h, std::hash<char>()(data));
        }
        return h;
    }
};
} // namespace std

#endif /* VTR_STRING_INTERNING_H_ */
