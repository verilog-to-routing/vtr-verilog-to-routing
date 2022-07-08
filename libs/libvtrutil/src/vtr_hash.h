#ifndef VTR_HASH_H
#define VTR_HASH_H
#include <functional>

namespace vtr {

/**
 * @brief Hashes v and combines it with seed (as in boost)
 *
 * This is typically used to implement std::hash for composite types.
 */
template<class T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct hash_pair {
    template<class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& pair) const noexcept {
        auto hash1 = std::hash<T1>{}(pair.first);
        auto hash2 = std::hash<T2>{}(pair.second);

        return hash1 ^ hash2;
    }
};

} // namespace vtr

#endif
