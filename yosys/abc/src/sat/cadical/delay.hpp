#ifndef _delay_hpp_INCLUDED
#define _delay_hpp_INCLUDED

#include "global.h"

#include <cstdint>
#include <limits>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {
struct Delay {
  unsigned count;
  unsigned current;

  Delay () : count (0), current (0) {}

  bool delay () {
    if (count) {
      --count;
      return true;
    } else {
      return false;
    }
  }

  void bump_delay () {
    current += current < std::numeric_limits<unsigned>::max ();
    count = current;
  }

  void reduce_delay () {
    if (!current)
      return;
    current /= 2;
    count = current;
  }
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
