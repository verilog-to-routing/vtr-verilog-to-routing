#include <cstddef>

#include "vtr_random.h"
#include "vtr_util.h"
#include "vtr_error.h"

#define CHECK_RAND

namespace vtr {
/* Portable random number generator defined below.  Taken from ANSI C by  *
 * K & R.  Not a great generator, but fast, and good enough for my needs. */

constexpr size_t IA = 1103515245u;
constexpr size_t IC = 12345u;
constexpr size_t IM = 2147483648u;

static RandState random_state = 0;

void srandom(int seed) {
    random_state = (unsigned int)seed;
}

/* returns the random_state value */
RandState get_random_state() {
    return random_state;
}

int irand(int imax, RandState& state) {
    /* Creates a random integer between 0 and imax, inclusive.  i.e. [0..imax] */
    int ival;

    /* state = (state * IA + IC) % IM; */
    state = state * IA + IC; /* Use overflow to wrap */
    ival = state & (IM - 1); /* Modulus */
    ival = (int)((float)ival * (float)(imax + 0.999) / (float)IM);

#ifdef CHECK_RAND
    if ((ival < 0) || (ival > imax)) {
        if (ival == imax + 1) {
            /* Due to random floating point rounding, sometimes above calculation gives number greater than ival by 1 */
            ival = imax;
        } else {
            throw VtrError(string_fmt("Bad value in my_irand, imax = %d  ival = %d", imax, ival), __FILE__, __LINE__);
        }
    }
#endif

    return ival;
}

int irand(int imax) {
    return irand(imax, random_state);
}

float frand() {
    /* Creates a random float between 0 and 1.  i.e. [0..1).        */

    float fval;
    int ival;

    random_state = random_state * IA + IC; /* Use overflow to wrap */
    ival = random_state & (IM - 1);        /* Modulus */
    fval = (float)ival / (float)IM;

#ifdef CHECK_RAND
    if ((fval < 0) || (fval > 1.)) {
        throw VtrError(string_fmt("Bad value in my_frand, fval = %g", fval), __FILE__, __LINE__);
    }
#endif

    return (fval);
}

} // namespace vtr
