#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// Binary implication graph lists.

void Internal::init_bins () {
  CADICAL_assert (big.empty ());
  if (big.size () < 2 * vsize)
    big.resize (2 * vsize, Bins ());
  LOG ("initialized binary implication graph");
}

void Internal::reset_bins () {
  CADICAL_assert (!big.empty ());
  erase_vector (big);
  LOG ("reset binary implication graph");
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
