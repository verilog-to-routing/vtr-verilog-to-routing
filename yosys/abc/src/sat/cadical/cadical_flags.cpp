#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

void Internal::mark_fixed (int lit) {
  if (external->fixed_listener) {
    int elit = externalize (lit);
    CADICAL_assert (elit);
    const int eidx = abs (elit);
    if (!external->ervars[eidx])
      external->fixed_listener->notify_fixed_assignment (elit);
  }
  Flags &f = flags (lit);
  CADICAL_assert (f.status == Flags::ACTIVE);
  f.status = Flags::FIXED;
  LOG ("fixed %d", abs (lit));
  stats.all.fixed++;
  stats.now.fixed++;
  stats.inactive++;
  CADICAL_assert (stats.active);
  stats.active--;
  CADICAL_assert (!active (lit));
  CADICAL_assert (f.fixed ());

  if (external_prop && private_steps) {
    // If pre/inprocessing found a fixed assignment, we want the propagator
    // to know about it.
    // But at that point it is not guaranteed to be already on the trail, so
    // notification will happen only later.
    CADICAL_assert (!level);
  }
}

void Internal::mark_eliminated (int lit) {
  Flags &f = flags (lit);
  CADICAL_assert (f.status == Flags::ACTIVE);
  f.status = Flags::ELIMINATED;
  LOG ("eliminated %d", abs (lit));
  stats.all.eliminated++;
  stats.now.eliminated++;
  stats.inactive++;
  CADICAL_assert (stats.active);
  stats.active--;
  CADICAL_assert (!active (lit));
  CADICAL_assert (f.eliminated ());
}

void Internal::mark_pure (int lit) {
  Flags &f = flags (lit);
  CADICAL_assert (f.status == Flags::ACTIVE);
  f.status = Flags::PURE;
  LOG ("pure %d", abs (lit));
  stats.all.pure++;
  stats.now.pure++;
  stats.inactive++;
  CADICAL_assert (stats.active);
  stats.active--;
  CADICAL_assert (!active (lit));
  CADICAL_assert (f.pure ());
}

void Internal::mark_substituted (int lit) {
  Flags &f = flags (lit);
  CADICAL_assert (f.status == Flags::ACTIVE);
  f.status = Flags::SUBSTITUTED;
  LOG ("substituted %d", abs (lit));
  stats.all.substituted++;
  stats.now.substituted++;
  stats.inactive++;
  CADICAL_assert (stats.active);
  stats.active--;
  CADICAL_assert (!active (lit));
  CADICAL_assert (f.substituted ());
}

void Internal::mark_active (int lit) {
  Flags &f = flags (lit);
  CADICAL_assert (f.status == Flags::UNUSED);
  f.status = Flags::ACTIVE;
  LOG ("activate %d previously unused", abs (lit));
  CADICAL_assert (stats.inactive);
  stats.inactive--;
  CADICAL_assert (stats.unused);
  stats.unused--;
  stats.active++;
  CADICAL_assert (active (lit));
}

void Internal::reactivate (int lit) {
  CADICAL_assert (!active (lit));
  Flags &f = flags (lit);
  CADICAL_assert (f.status != Flags::FIXED);
  CADICAL_assert (f.status != Flags::UNUSED);
#ifdef LOGGING
  const char *msg = 0;
#endif
  switch (f.status) {
  default:
  case Flags::ELIMINATED:
    CADICAL_assert (f.status == Flags::ELIMINATED);
    CADICAL_assert (stats.now.eliminated > 0);
    stats.now.eliminated--;
#ifdef LOGGING
    msg = "eliminated";
#endif
    break;
  case Flags::SUBSTITUTED:
#ifdef LOGGING
    msg = "substituted";
#endif
    CADICAL_assert (stats.now.substituted > 0);
    stats.now.substituted--;
    break;
  case Flags::PURE:
#ifdef LOGGING
    msg = "pure literal";
#endif
    CADICAL_assert (stats.now.pure > 0);
    stats.now.pure--;
    break;
  }
#ifdef LOGGING
  CADICAL_assert (msg);
  LOG ("reactivate previously %s %d", msg, abs (lit));
#endif
  f.status = Flags::ACTIVE;
  f.sweep = false;
  CADICAL_assert (active (lit));
  stats.reactivated++;
  CADICAL_assert (stats.inactive > 0);
  stats.inactive--;
  stats.active++;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
