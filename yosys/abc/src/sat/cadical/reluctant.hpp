#ifndef _reluctant_hpp_INCLUDED
#define _reluctant_hpp_INCLUDED

#include "global.h"

#include <cassert>
#include <cstdint>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

// This is Donald Knuth's version of the Luby restart sequence which he
// called 'reluctant doubling'.  His bit-twiddling formulation in line (DK)
// requires to keep two words around which are updated every time the
// reluctant doubling sequence is advanced.  The original version in the
// literature uses a complex recursive function which computes the length of
// the next inactive sub-sequence every time (but is state-less).
//
// In our code we incorporate a base interval 'period' and only after period
// many calls to 'tick' times the current sequence value we update the
// reluctant doubling sequence value.  The 'tick' call is decoupled from
// the activation signal of the sequence (the 'bool ()' operator) through
// 'trigger'.  It is also possible to set an upper limit to the length of an
// inactive sub-sequence.  If that limit is reached the whole reluctant
// doubling sequence starts over with the initial values.

class Reluctant {

  uint64_t u, v, limit;
  uint64_t period, countdown;
  bool trigger, limited;

public:
  Reluctant () : period (0), trigger (false) {}

  void enable (int p, int64_t l) {
    CADICAL_assert (p > 0);
    u = v = 1;
    period = countdown = p;
    trigger = false;
    if (l <= 0)
      limited = false;
    else
      limited = true, limit = l;
  };

  void disable () { period = 0, trigger = false; }

  // Increments the count until the 'period' is hit.  Then it performs the
  // actual increment of reluctant doubling.  This gives the common 'Luby'
  // sequence with the specified base interval period.  As soon the limit is
  // reached (countdown goes to zero) we remember this event and then
  // disable updating the reluctant sequence until the signal is delivered.

  void tick () {

    if (!period)
      return; // disabled
    if (trigger)
      return; // already triggered
    if (--countdown)
      return; // not there yet

    if ((u & -u) == v)
      u = u + 1, v = 1;
    else
      v = 2 * v; // (DK)

    if (limited && v >= limit)
      u = v = 1;
    countdown = v * period;
    trigger = true;
  }

  operator bool () {
    if (!trigger)
      return false;
    trigger = false;
    return true;
  }
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
