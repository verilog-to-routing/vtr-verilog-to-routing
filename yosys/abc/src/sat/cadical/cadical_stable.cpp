#include "global.h"

#ifdef PROFILE_MODE

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

bool Internal::propagate_stable () {
  CADICAL_assert (stable);
  START (propstable);
  bool res = propagate ();
  STOP (propstable);
  return res;
}

void Internal::analyze_stable () {
  CADICAL_assert (stable);
  START (analyzestable);
  analyze ();
  STOP (analyzestable);
}

int Internal::decide_stable () {
  CADICAL_assert (stable);
  return decide ();
}

}; // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END

#else
ABC_NAMESPACE_IMPL_START
int stable_if_not_profile_mode_dummy;
ABC_NAMESPACE_IMPL_END
#endif
