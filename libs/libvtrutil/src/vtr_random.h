#ifndef VTR_RANDOM_H
#define VTR_RANDOM_H
#include <algorithm> //For std::swap

namespace vtr {
/*********************** Portable random number generators *******************/
typedef unsigned RandState;

/**
 * @brief The pseudo-random number generator is initialized using the argument passed as seed.
 */
void srandom(int seed);

///@brief Return The random number generator state
RandState get_random_state();

///@brief Return a randomly generated integer less than or equal imax
int irand(int imax);

///@brief Return a randomly generated integer less than or equal imax using the generator (rand_state)
int irand(int imax, RandState& rand_state);

///@brief Return a randomly generated float number between [0,1]
float frand();

/**
 * @brief Portable/invariant version of std::shuffle
 *
 * Note that std::shuffle relies on std::uniform_int_distribution
 * which can produce different sequences accross different
 * compilers/compiler versions.
 * 
 * This version should be deterministic/invariant. However,  since
 * it uses vtr::irand(), may not be as well distributed as std::shuffle.
 */
template<typename Iter>
void shuffle(Iter first, Iter last, RandState& rand_state) {
    for (auto i = (last - first) - 1; i > 0; --i) {
        using std::swap;
        swap(first[i], first[irand(i, rand_state)]);
    }
}

} // namespace vtr
#endif
