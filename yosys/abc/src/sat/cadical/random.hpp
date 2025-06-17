#ifndef _random_hpp_INCLUDED
#define _random_hpp_INCLUDED

#include "global.h"

#include <cstdint>

ABC_NAMESPACE_CXX_HEADER_START

// Random number generator.

namespace CaDiCaL {

class Random {

  uint64_t state;

  void add (uint64_t a) {
    if (!(state += a))
      state = 1;
    next ();
  }

public:
  // Without argument use a machine, process and time dependent seed.
  //
  Random ();

  Random (uint64_t seed) : state (seed) {}
  void operator= (uint64_t seed) { state = seed; }
  Random (const Random &other) : state (other.seed ()) {}

  void operator+= (uint64_t a) { add (a); }
  uint64_t seed () const { return state; }

  uint64_t next () {
    state *= 6364136223846793005ul;
    state += 1442695040888963407ul;
    CADICAL_assert (state);
    return state;
  }

  uint32_t generate () {
    next ();
    return state >> 32;
  }
  int generate_int () { return (int) generate (); }
  bool generate_bool () { return generate () < 2147483648u; }

  // Generate 'double' value in the range '[0,1]' excluding '1'.
  //
  double generate_double () { return generate () / 4294967295.0; }

  // Generate 'int' value in the range '[l,r]'.
  //
  int pick_int (int l, int r) {
    CADICAL_assert (l <= r);
    const unsigned delta = 1 + r - (unsigned) l;
    unsigned tmp = generate (), scaled;
    if (delta) {
      const double fraction = tmp / 4294967296.0;
      scaled = delta * fraction;
    } else
      scaled = tmp;
    const int res = scaled + l;
    CADICAL_assert (l <= res);
    CADICAL_assert (res <= r);
    return res;
  }

  int pick_log (int l, int r) {
    CADICAL_assert (l <= r);
    const unsigned delta = 1 + r - (unsigned) l;
    int log_delta = delta ? 0 : 32;
    while (log_delta < 32 && (1u << log_delta) < delta)
      log_delta++;
    const int log_res = pick_int (0, log_delta);
    unsigned tmp = generate ();
    if (log_res < 32)
      tmp &= (1u << log_res) - 1;
    if (delta)
      tmp %= delta;
    const int res = l + tmp;
    CADICAL_assert (l <= res), CADICAL_assert (res <= r);
    return res;
  }

  // Generate 'double' value in the range '[l,r]'.
  //
  double pick_double (double l, double r) {
    CADICAL_assert (l <= r);
    double res = (r - l) * generate_double ();
    res += l;
    CADICAL_assert (l <= res);
    CADICAL_assert (res <= r);
    return res;
  }
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
