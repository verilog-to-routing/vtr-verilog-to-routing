#ifndef VTR_HASH_H
#define VTR_HASH_H
#include <functional>

namespace vtr {

//Hashes v and combines it with seed (as in boost)
//
//This is typically used to implement std::hash for composite types.
template<class T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

} // namespace vtr

#endif
