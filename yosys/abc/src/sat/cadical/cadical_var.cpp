#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

void Internal::reset_subsume_bits () {
  LOG ("marking all variables as not subsume");
  for (auto idx : vars)
    flags (idx).subsume = false;
}

void Internal::check_var_stats () {
#ifndef CADICAL_NDEBUG
  int64_t fixed = 0, eliminated = 0, substituted = 0, pure = 0, unused = 0;
  for (auto idx : vars) {
    Flags &f = flags (idx);
    if (f.active ())
      continue;
    if (f.fixed ())
      fixed++;
    if (f.eliminated ())
      eliminated++;
    if (f.substituted ())
      substituted++;
    if (f.unused ())
      unused++;
    if (f.pure ())
      pure++;
  }
  CADICAL_assert (stats.now.fixed == fixed);
  CADICAL_assert (stats.now.eliminated == eliminated);
  CADICAL_assert (stats.now.substituted == substituted);
  CADICAL_assert (stats.now.pure == pure);
  int64_t inactive = unused + fixed + eliminated + substituted + pure;
  CADICAL_assert (stats.inactive == inactive);
  CADICAL_assert (max_var == stats.active + stats.inactive);
#endif
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
