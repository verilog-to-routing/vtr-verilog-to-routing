#ifndef _random_h_INCLUDED
#define _random_h_INCLUDED

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

typedef uint64_t generator;

static inline uint64_t kissat_next_random64 (generator *rng) {
  *rng *= 6364136223846793005ul;
  *rng += 1442695040888963407ul;
  return *rng;
}

static inline unsigned kissat_next_random32 (generator *rng) {
  return kissat_next_random64 (rng) >> 32;
}

static inline unsigned kissat_pick_random (generator *rng, unsigned l,
                                           unsigned r) {
  KISSAT_assert (l <= r);
  if (l == r)
    return l;
  const unsigned delta = r - l;
  const unsigned tmp = kissat_next_random32 (rng);
  const double fraction = tmp / 4294967296.0;
  KISSAT_assert (0 <= fraction), KISSAT_assert (fraction < 1);
  const unsigned scaled = delta * fraction;
  KISSAT_assert (scaled < delta);
  const unsigned res = l + scaled;
  KISSAT_assert (l <= res), KISSAT_assert (res < r);
  return res;
}

static inline bool kissat_pick_bool (generator *rng) {
  return kissat_pick_random (rng, 0, 2);
}

static inline double kissat_pick_double (generator *rng) {
  return kissat_next_random32 (rng) / 4294967296.0;
}

ABC_NAMESPACE_HEADER_END

#endif
