#ifndef VTR_DYNAMIC_BITSET
#define VTR_DYNAMIC_BITSET

#include <limits>
#include <vector>

namespace vtr {

template<typename Index = size_t, typename Storage = unsigned int>
class dynamic_bitset {
  public:
    // Bits in underlying storage.
    static constexpr size_t kWidth = std::numeric_limits<Storage>::digits;
    static_assert(!std::numeric_limits<Storage>::is_signed,
                  "dynamic_bitset storage must be unsigned!");
    static_assert(std::numeric_limits<Storage>::is_integer,
                  "dynamic_bitset storage must be integer!");

    void resize(size_t size) {
        array_.resize((size + kWidth - 1) / kWidth);
    }

    void clear() {
        array_.clear();
        array_.shrink_to_fit();
    }

    size_t size() const {
        return array_.size() * kWidth;
    }

    void fill(bool set) {
        if (set) {
            std::fill(array_.begin(), array_.end(), std::numeric_limits<Storage>::max());
        } else {
            std::fill(array_.begin(), array_.end(), 0);
        }
    }

    void set(Index index, bool val) {
        size_t index_value(index);
        VTR_ASSERT_SAFE(index_value < size());
        if (val) {
            array_[index_value / kWidth] |= (1 << (index_value % kWidth));
        } else {
            array_[index_value / kWidth] &= ~(1u << (index_value % kWidth));
        }
    }

    bool get(Index index) const {
        size_t index_value(index);
        VTR_ASSERT_SAFE(index_value < size());
        return (array_[index_value / kWidth] & (1u << (index_value % kWidth))) != 0;
    }

  private:
    std::vector<Storage> array_;
};

} // namespace vtr

#endif /* VTR_DYNAMIC_BITSET */
