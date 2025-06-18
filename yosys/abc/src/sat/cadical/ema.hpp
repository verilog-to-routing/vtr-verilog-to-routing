#ifndef _ema_hpp_INCLUDED
#define _ema_hpp_INCLUDED

#include "global.h"

#include <cstdint>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

struct Internal;

// This is a more complex generic exponential moving average class to
// support more robust initialization (see comments in the 'update'
// implementation).

struct EMA {

#ifdef LOGGING
  uint64_t updated;
#endif
  double value;  // unbiased (corrected) moving average
  double biased; // biased initialized moving average
  double alpha;  // input scaling with 'alpha = 1 - beta'
  double beta;   // decay of 'biased' with 'beta = 1 - alpha'
  double exp;    // 'exp = pow (beta, updated)'

  EMA ()
      :
#ifdef LOGGING
        updated (0),
#endif
        value (0), biased (0), alpha (0), beta (0), exp (0) {
  }

  EMA (double a)
      :
#ifdef LOGGING
        updated (0),
#endif
        value (0), biased (0), alpha (a), beta (1 - a), exp (!!beta) {
    CADICAL_assert (beta >= 0);
  }

  operator double () const { return value; }
  void update (Internal *, double y, const char *name);
};

} // namespace CaDiCaL

/*------------------------------------------------------------------------*/

// Compact average update and initialization macros for better logging.

#define UPDATE_AVERAGE(A, Y) \
  do { \
    A.update (internal, (Y), #A); \
  } while (0)

#define INIT_EMA(E, WINDOW) \
  do { \
    CADICAL_assert ((WINDOW) >= 1); \
    double ALPHA = 1.0 / (double) (WINDOW); \
    E = EMA (ALPHA); \
    LOG ("init " #E " EMA target alpha %g window %d", ALPHA, \
         (int) WINDOW); \
  } while (0)

/*------------------------------------------------------------------------*/

ABC_NAMESPACE_CXX_HEADER_END

#endif
