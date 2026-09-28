#pragma once

#include <cstdint>
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

/**
 * @brief Mixes a base seed and an index into a 64-bit value that is unrelated
 * to the values for nearby indices (splitmix64).
 */
inline uint64_t mix64(uint64_t seed, uint64_t index) {
    uint64_t z = seed + 0x9E3779B97F4A7C15ull * (index + 1);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
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
