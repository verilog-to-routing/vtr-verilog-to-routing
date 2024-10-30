#ifndef VTR_RANDOM_H
#define VTR_RANDOM_H
#include <algorithm> //For std::swap

#define CHECK_RAND

namespace vtr {

class RandomNumberGeneratorInterface {
  public:

    /// @brief Initialize the pseudo-random number generator using the argument passed as seed.
    virtual void srandom(int seed) = 0;

    ///@brief Return a randomly generated integer less than or equal imax
    virtual int irand(int imax) = 0;

    ///@brief Return a randomly generated float number between [0,1]
    virtual float frand() = 0;
};

class RandomNumberGenerator : public RandomNumberGeneratorInterface {
  private:
    typedef unsigned state_t;

  public:
    RandomNumberGenerator(const RandomNumberGenerator&) = delete;
    RandomNumberGenerator& operator=(RandomNumberGenerator& other) = delete;

    RandomNumberGenerator();
    explicit RandomNumberGenerator(int seed);

    virtual void srandom(int seed) override;
    virtual int irand(int imax) override;
    virtual float frand() override;

  private:
    /* Portable random number generator defined below.  Taken from ANSI C by  *
     * K & R.  Not a great generator, but fast, and good enough for my needs. */
    static constexpr size_t IA = 1103515245u;
    static constexpr size_t IC = 12345u;
    static constexpr size_t IM = 2147483648u;

//#ifdef CHECK_RAND
    static constexpr bool CHECK_RAND_CONSTEXPR = true;
//#else
//    static constexpr bool CHECK_RAND_CONSTEXPR = false;
//#endif

    state_t random_state_ = 0;
};


/**
 * @brief Portable/invariant version of std::shuffle
 *
 * Note that std::shuffle relies on std::uniform_int_distribution
 * which can produce different sequences across different
 * compilers/compiler versions.
 * 
 * This version should be deterministic/invariant. However,  since
 * it uses vtr::irand(), may not be as well distributed as std::shuffle.
 */
template<typename Iter>
void shuffle(Iter first, Iter last, RandomNumberGeneratorInterface& rng) {
    for (auto i = (last - first) - 1; i > 0; --i) {
        using std::swap;
        swap(first[i], first[rng.irand(i)]);
    }
}

} // namespace vtr
#endif
