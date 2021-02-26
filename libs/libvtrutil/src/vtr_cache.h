#ifndef VTR_CACHE_H_
#define VTR_CACHE_H_

#include <memory>

namespace vtr {

///@brief An implementation of a simple cache
template<typename CacheKey, typename CacheValue>
class Cache {
  public:
    ///@brief Clear cache.
    void clear() {
        key_ = CacheKey();
        value_.reset();
    }
    /**
     * @brief Check if the cache is valid.
     * 
     * Returns the cached value if present and valid.
     * Returns nullptr if the cache is invalid.
     */
    const CacheValue* get(const CacheKey& key) const {
        if (key == key_ && value_) {
            return value_.get();
        } else {
            return nullptr;
        }
    }

    ///@brief Update the cache.
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
