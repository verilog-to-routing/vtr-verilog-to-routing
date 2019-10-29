#ifndef VTR_DYNAMIC_BITSET
#define VTR_DYNAMIC_BITSET

#include <limits>
#include <vector>

namespace vtr {

template<typename Storage = unsigned int>
class dynamic_bitset {
  public:
    // Bits in underlying storage.
    static constexpr size_t kWidth = std::numeric_limits<Storage>::digits;

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

    void set(size_t index, bool val) {
        if (val) {
            array_[index / kWidth] |= (1 << (index % kWidth));
        } else {
            array_[index / kWidth] &= ~(1u << (index % kWidth));
        }
    }

    bool get(size_t index) const {
        return (array_[index / kWidth] & (1u << (index % kWidth))) != 0;
    }

  private:
    std::vector<Storage> array_;
};

} // namespace vtr

#endif /* VTR_DYNAMIC_BITSET */
