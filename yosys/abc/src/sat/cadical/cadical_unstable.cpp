#include "global.h"

#ifdef PROFILE_MODE
#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

bool Internal::propagate_unstable () {
  CADICAL_assert (!stable);
  START (propunstable);
  bool res = propagate ();
  STOP (propunstable);
  return res;
}

void Internal::analyze_unstable () {
  CADICAL_assert (!stable);
  START (analyzeunstable);
  analyze ();
  STOP (analyzeunstable);
}

int Internal::decide_unstable () {
  CADICAL_assert (!stable);
  return decide ();
}

}; // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END

#else
ABC_NAMESPACE_IMPL_START
int unstable_if_no_profile_mode;
ABC_NAMESPACE_IMPL_END
#endif
