#ifndef VTR_CACHE_H_
#define VTR_CACHE_H_

#include <memory>

namespace vtr {

// Simple cache
template<typename CacheKey, typename CacheValue>
class Cache {
  public:
    // Clear cache.
    void clear() {
        key_ = CacheKey();
        value_.reset();
    }

    // Check if the cache is valid, and return the cached value if present and valid.
    //
    // Returns nullptr if the cache is invalid.
    const CacheValue* get(const CacheKey& key) const {
        if (key == key_ && value_) {
            return value_.get();
        } else {
            return nullptr;
        }
    }

    // Update the cache.
    const CacheValue* set(const CacheKey& key, std::unique_ptr<CacheValue> value) {
        key_ = key;
        value_ = std::move(value);

        return value_.get();
    }

  private:
    CacheKey key_;
    std::unique_ptr<CacheValue> value_;
};

} // namespace vtr

#endif
