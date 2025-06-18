#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// Occurrence lists.

void Internal::init_occs () {
  if (otab.size () < 2 * vsize)
    otab.resize (2 * vsize, Occs ());
  LOG ("initialized occurrence lists");
}

void Internal::reset_occs () {
  CADICAL_assert (occurring ());
  erase_vector (otab);
  LOG ("reset occurrence lists");
}

void Internal::clear_occs () {
  CADICAL_assert (occurring ());
  for (auto &occ : otab)
    occ.clear ();
  LOG ("clear occurrence lists");
}

/*------------------------------------------------------------------------*/

// One-sided occurrence counter (each literal has its own counter).

void Internal::init_noccs () {
  CADICAL_assert (ntab.empty ());
  if (ntab.size () < 2 * vsize)
    ntab.resize (2 * vsize, 0);
  LOG ("initialized two-sided occurrence counters");
}

void Internal::clear_noccs () {
  CADICAL_assert (!ntab.empty ());
  for (auto &nt : ntab)
    nt = 0;
  LOG ("clear two-sided occurrence counters");
}

void Internal::reset_noccs () {
  CADICAL_assert (!max_var || !ntab.empty ());
  erase_vector (ntab);
  LOG ("reset two-sided occurrence counters");
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
