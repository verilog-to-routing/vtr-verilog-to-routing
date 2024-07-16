#ifndef VTR_DYNAMIC_BITSET
#define VTR_DYNAMIC_BITSET

#include <limits>
#include <vector>

namespace vtr {
/**
 * @brief A container to represent a set of flags either they are set or reset 
 *
 * It allocates any required length of bit at runtime. It is very useful in bit manipulation
 */
template<typename Index = size_t, typename Storage = unsigned int>
class dynamic_bitset {
  public:
    ///@brief Bits in underlying storage.
    static constexpr size_t kWidth = std::numeric_limits<Storage>::digits;
    static_assert(!std::numeric_limits<Storage>::is_signed,
                  "dynamic_bitset storage must be unsigned!");
    static_assert(std::numeric_limits<Storage>::is_integer,
                  "dynamic_bitset storage must be integer!");

    constexpr dynamic_bitset() = default;
    constexpr dynamic_bitset(Index size) {
        resize(size);
    }

    ///@brief Reize to the determined size
    void resize(size_t size) {
        array_.resize((size + kWidth - 1) / kWidth);
    }

    ///@brief Clear all the bits
    void clear() {
        array_.clear();
        array_.shrink_to_fit();
    }

    ///@brief Return the size of the bitset (total number of bits)
    size_t size() const {
        return array_.size() * kWidth;
    }

    ///@brief Fill the whole bitset with a specific value (0 or 1)
    void fill(bool set) {
        if (set) {
            std::fill(array_.begin(), array_.end(), std::numeric_limits<Storage>::max());
        } else {
            std::fill(array_.begin(), array_.end(), 0);
        }
    }

    ///@brief Set a specific bit in the bit set to a specific value (0 or 1)
    void set(Index index, bool val) {
        size_t index_value(index);
        VTR_ASSERT_SAFE(index_value < size());
        if (val) {
            array_[index_value / kWidth] |= (1 << (index_value % kWidth));
        } else {
            array_[index_value / kWidth] &= ~(1u << (index_value % kWidth));
        }
    }

    ///@brief Return the value of a specific bit in the bitset
    bool get(Index index) const {
        size_t index_value(index);
        VTR_ASSERT_SAFE(index_value < size());
        return (array_[index_value / kWidth] & (1u << (index_value % kWidth))) != 0;
    }

    ///@brief Return count of set bits.
    constexpr size_t count(void) const {
        size_t out = 0;
        for (auto x : array_)
            out += __builtin_popcount(x);
        return out;
    }

    ///@brief Bitwise OR with rhs. Truncate the operation if one operand is smaller.
    constexpr dynamic_bitset<Index, Storage>& operator|=(const dynamic_bitset<Index, Storage>& x) {
        size_t n = std::min(array_.size(), x.array_.size());
        for (size_t i = 0; i < n; i++)
            array_[i] |= x.array_[i];
        return *this;
    }

    ///@brief Bitwise AND with rhs. Truncate the operation if one operand is smaller.
    constexpr dynamic_bitset<Index, Storage>& operator&=(const dynamic_bitset<Index, Storage>& x) {
        size_t n = std::min(array_.size(), x.array_.size());
        for (size_t i = 0; i < n; i++)
            array_[i] &= x.array_[i];
        return *this;
    }

    ///@brief Return inverted bitset.
    inline dynamic_bitset<Index, Storage> operator~(void) const {
        dynamic_bitset<Index, Storage> out(size());
        size_t n = array_.size();
        for (size_t i = 0; i < n; i++)
            out.array_[i] = ~array_[i];
        return out;
    }

  private:
    std::vector<Storage> array_;
};

} // namespace vtr

#endif /* VTR_DYNAMIC_BITSET */
