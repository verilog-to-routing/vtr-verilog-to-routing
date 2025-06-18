#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

// Updating an exponential moving average is placed here since we want to
// log both updates and phases of initialization, thus need 'LOG'.
//
// We now use initialization bias correction as in the ADAM method
// [KingmaBa-ICLR'15] instead of our ad-hoc initialization method used
// before.  Our old variant used exponentially decreasing alphas:
//
//   1,
//   1/2, 1/2,
//   1/4, 1/4, 1/4, 1/4
//   1/8, 1/8, 1/8, 1/8, 1/8, 1/8, 1/8, 1/8,
//   ...
//   2^-n, ..., 2^-n                          'n' times
//   alpha, alpha, ...                        now 'alpha' forever.
//
//   where 2^-n is the smallest negative power of two above 'alpha'
//
// This old method is better than the initializations described in our
// [BiereFroehlich-POS'15] paper and actually faster than the ADAM method,
// but less precise.  We consider this old method obsolete now but it
// could still be useful for implementations relying on integers instead
// of floating points because it only needs shifts and integer arithmetic.
//
// Our new method for unbiased initialization of the exponential averages
// works as follows.  First the biased moving average is computed as usual.
// Note that (as already before) we use the simpler equation
//
//   new_biased = old_biased + alpha * (y - old_biased);
//
// which in principle (and thus easy to remember) can be implemented as
//
//   biased += alpha * (y - biased);
//
// The original formulation in the ADAM paper (with 'alpha = 1 - beta') is
//
//   new_biased = beta * old_biased + (1 - beta) * y
//
// To show that these are equivalent (modulo floating point issues)
// consider the following equivalent expressions:
//
//   old_biased + alpha * (y - old_biased)
//   old_biased + alpha * y - alpha * old_biased
//   (1 - alpha) * old_biased + alpha * y
//   beta * old_biased + (1 - beta) * y
//
// The real new idea taken from the ADAM paper is however to fix the biased
// moving average with a correction term '1.0 / (1.0 - pow (beta, updated))'
// by multiplication to obtain an unbiased moving average (called simply
// 'value' in our 'code').  In order to avoid computing 'pow' every time, we
// use 'exp' which is multiplied in every update with 'beta'.

void EMA::update (Internal *internal, double y, const char *name) {
#ifdef LOGGING
  updated++;
  const double old_value = value;
#endif
  const double old_biased = biased;
  const double delta = y - old_biased;
  const double scaled_delta = alpha * delta;
  const double new_biased = old_biased + scaled_delta;
  LOG ("update %" PRIu64 " of biased %s EMA %g with %g (delta %g) "
       "yields %g (scaled delta %g)",
       updated, name, old_biased, y, delta, new_biased, scaled_delta);
  biased = new_biased;
  const double old_exp = exp;
  double new_exp, div, new_value;
  if (old_exp) {
    new_exp = old_exp * beta;
    CADICAL_assert (new_exp < 1);
    exp = new_exp;
    div = 1 - new_exp;
    CADICAL_assert (div > 0);
    new_value = new_biased / div;
  } else {
    new_value = new_biased;
#ifdef LOGGING
    new_exp = 0;
    div = 1;
#endif
  }
  value = new_value;
  LOG ("update %" PRIu64 " of corrected %s EMA %g with %g (delta %g) "
       "yields %g (exponent %g, divisor %g)",
       updated, name, old_value, y, delta, new_value, new_exp, div);
#ifndef LOGGING
  (void) internal;
  (void) name;
#endif
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
