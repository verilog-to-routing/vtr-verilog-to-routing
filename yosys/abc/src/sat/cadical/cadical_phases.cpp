#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

void Internal::copy_phases (vector<signed char> &dst) {
  START (copy);
  for (auto i : vars)
    dst[i] = phases.saved[i];
  STOP (copy);
}

void Internal::clear_phases (vector<signed char> &dst) {
  START (copy);
  for (auto i : vars)
    dst[i] = 0;
  STOP (copy);
}

void Internal::phase (int lit) {
  const int idx = vidx (lit);
  signed char old_forced_phase = phases.forced[idx];
  signed char new_forced_phase = sign (lit);
  if (old_forced_phase == new_forced_phase) {
    LOG ("forced phase remains at %d", old_forced_phase * idx);
    return;
  }
  if (old_forced_phase)
    LOG ("overwriting old forced phase %d", old_forced_phase * idx);
  LOG ("new forced phase %d", new_forced_phase * idx);
  phases.forced[idx] = new_forced_phase;
}

void Internal::unphase (int lit) {
  const int idx = vidx (lit);
  signed char old_forced_phase = phases.forced[idx];
  if (!old_forced_phase) {
    LOG ("forced phase of %d already reset", lit);
    return;
  }
  LOG ("clearing old forced phase %d", old_forced_phase * idx);
  phases.forced[idx] = 0;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
