
#include "vtr_random.h"
#include "vtr_util.h"
#include "vtr_error.h"

namespace vtr {

RandomNumberGenerator::RandomNumberGenerator(int seed) {
    random_state_ = (unsigned int)seed;
}

RandomNumberGenerator::RandomNumberGenerator()
    : RandomNumberGenerator(0) {}

void RandomNumberGenerator::srandom(int seed) {
    random_state_ = (unsigned int)seed;
}

int RandomNumberGenerator::irand(int imax) {
    // Creates a random integer between 0 and imax, inclusive.  i.e. [0..imax]
    int ival;

    // state = (state * IA + IC) % IM;
    random_state_ = random_state_ * IA + IC; // Use overflow to wrap
    ival = random_state_ & (IM - 1); // Modulus
    ival = (int)((float)ival * (float)(imax + 0.999) / (float)IM);

    if constexpr (CHECK_RAND_CONSTEXPR) {
        if ((ival < 0) || (ival > imax)) {
            if (ival == imax + 1) {
                // Due to random floating point rounding, sometimes above calculation gives number greater than ival by 1
                ival = imax;
            } else {
                throw VtrError(string_fmt("Bad value in my_irand, imax = %d  ival = %d", imax, ival), __FILE__, __LINE__);
            }
        }
    }

    return ival;
}

float RandomNumberGenerator::frand() {
    random_state_ = random_state_ * IA + IC; /* Use overflow to wrap */
    int ival = random_state_ & (IM - 1);        /* Modulus */
    float fval = (float)ival / (float)IM;

    if constexpr (CHECK_RAND_CONSTEXPR) {
        if (fval < 0 || fval > 1.) {
            throw VtrError(string_fmt("Bad value in my_frand, fval = %g", fval), __FILE__, __LINE__);
        }
    }

    return fval;
}

} // namespace vtr
