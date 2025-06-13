#include "global.h"

#ifndef CADICAL_QUIET

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

// Initialize all profile counters with constant name and profiling level.

Profiles::Profiles (Internal *s)
    : internal (s)
#define PROFILE(NAME, LEVEL) , NAME (#NAME, LEVEL)
          PROFILES
#undef PROFILE
{
}

void Internal::start_profiling (Profile &profile, double s) {
  CADICAL_assert (profile.level <= opts.profile);
  CADICAL_assert (!profile.active);
  profile.started = s;
  profile.active = true;
}

void Internal::stop_profiling (Profile &profile, double s) {
  CADICAL_assert (profile.level <= opts.profile);
  CADICAL_assert (profile.active);
  profile.value += s - profile.started;
  profile.active = false;
}

double Internal::update_profiles () {
  double now = time ();
#define PROFILE(NAME, LEVEL) \
  do { \
    Profile &profile = profiles.NAME; \
    if (profile.active) { \
      CADICAL_assert (profile.level <= opts.profile); \
      profile.value += now - profile.started; \
      profile.started = now; \
    } \
  } while (0);
  PROFILES
#undef PROFILE
  return now;
}

double Internal::solve_time () {
  (void) update_profiles ();
  return profiles.solve.value;
}

#define PRT(S, T) \
  MSG ("%s" S "%s", tout.magenta_code (), T, tout.normal_code ())

void Internal::print_profile () {
  double now = update_profiles ();
  const char *time_type = opts.realtime ? "real" : "process";
  SECTION ("run-time profiling");
  PRT ("%s time taken by individual solving procedures", time_type);
  PRT ("(percentage relative to %s time for solving)", time_type);
  LINE ();
  const size_t size = sizeof profiles / sizeof (Profile);
  struct Profile *profs[size];
  size_t n = 0;
#define PROFILE(NAME, LEVEL) \
  do { \
    if (LEVEL > opts.profile) \
      break; \
    Profile *p = &profiles.NAME; \
    if (p == &profiles.solve) \
      break; \
    if (!profiles.NAME.value && p != &profiles.parse && \
        p != &profiles.search && p != &profiles.simplify) \
      break; \
    profs[n++] = p; \
  } while (0);
  PROFILES
#undef PROFILE

  CADICAL_assert (n <= size);

  // Explicit bubble sort to avoid heap allocation since 'print_profile'
  // is also called during catching a signal after out of heap memory.
  // This only makes sense if 'profs' is allocated on the stack, and
  // not the heap, which should be the case.

  double solve = profiles.solve.value;

  for (size_t i = 0; i < n; i++) {
    for (size_t j = i + 1; j < n; j++)
      if (profs[j]->value > profs[i]->value)
        swap (profs[i], profs[j]);
    MSG ("%12.2f %7.2f%% %s", profs[i]->value,
         percent (profs[i]->value, solve), profs[i]->name);
  }

  MSG ("  =================================");
  MSG ("%12.2f %7.2f%% solve", solve, percent (solve, now));

  LINE ();
  PRT ("last line shows %s time for solving", time_type);
  PRT ("(percentage relative to total %s time)", time_type);
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END

#endif // ifndef CADICAL_QUIET
