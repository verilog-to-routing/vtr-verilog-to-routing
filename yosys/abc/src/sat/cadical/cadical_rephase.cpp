#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// We experimented with resetting and reinitializing the saved phase with
// many solvers. Actually RSAT had already such a scheme.  Our newest
// version seems to be quite beneficial for satisfiable instances.  In an
// arithmetic increasing interval in the number of conflicts we either use
// the original phase (set by the option 'phase'), its inverted value, flip
// the current phase, pick random phases, then pick the best since the last
// time a best phase was picked and finally also use local search to find
// phases which minimize the number of falsified clauses.  During
// stabilization (see 'stabilizing' in 'restart.cpp' when 'stable' is true)
// we execute a different rephasing schedule.  The same applies if local
// search is disabled.

/*------------------------------------------------------------------------*/

bool Internal::rephasing () {
  if (!opts.rephase)
    return false;
  if (opts.forcephase)
    return false;
  return stats.conflicts > lim.rephase;
}

/*------------------------------------------------------------------------*/

// Pick the original default phase.

char Internal::rephase_original () {
  stats.rephased.original++;
  signed char val = opts.phase ? 1 : -1; // original = initial
  PHASE ("rephase", stats.rephased.total, "switching to original phase %d",
         val);
  for (auto idx : vars)
    phases.saved[idx] = val;
  return 'O';
}

// Pick the inverted original default phase.

char Internal::rephase_inverted () {
  stats.rephased.inverted++;
  signed char val = opts.phase ? -1 : 1; // original = -initial
  PHASE ("rephase", stats.rephased.total,
         "switching to inverted original phase %d", val);
  for (auto idx : vars)
    phases.saved[idx] = val;
  return 'I';
}

// Flip the current phase.

char Internal::rephase_flipping () {
  stats.rephased.flipped++;
  PHASE ("rephase", stats.rephased.total,
         "flipping all phases individually");
  for (auto idx : vars)
    phases.saved[idx] *= -1;
  return 'F';
}

// Complete random picking of phases.

char Internal::rephase_random () {
  stats.rephased.random++;
  PHASE ("rephase", stats.rephased.total, "resetting all phases randomly");
  Random random (opts.seed);       // global seed
  random += stats.rephased.random; // different every time
  for (auto idx : vars)
    phases.saved[idx] = random.generate_bool () ? -1 : 1;
  return '#';
}

// Best phases are those saved at the largest trail height without conflict.
// See code and comments in 'update_target_and_best' in 'backtrack.cpp'

char Internal::rephase_best () {
  stats.rephased.best++;
  PHASE ("rephase", stats.rephased.total,
         "overwriting saved phases by best phases");
  signed char val;
  for (auto idx : vars)
    if ((val = phases.best[idx]))
      phases.saved[idx] = val;
  return 'B';
}

// Trigger local search 'walk' in 'walk.cpp'.

char Internal::rephase_walk () {
  stats.rephased.walk++;
  PHASE ("rephase", stats.rephased.total,
         "starting local search to improve current phase");
  walk ();
  return 'W';
}

/*------------------------------------------------------------------------*/

void Internal::rephase () {

  stats.rephased.total++;
  PHASE ("rephase", stats.rephased.total,
         "reached rephase limit %" PRId64 " after %" PRId64 " conflicts",
         lim.rephase, stats.conflicts);

  // Report current 'target' and 'best' and then set 'rephased' below, which
  // will trigger reporting the new 'target' and 'best' after updating it in
  // the next 'update_target_and_best' called from the next 'backtrack'.
  //
  report ('~', 1);

  backtrack ();
  clear_phases (phases.target);
  target_assigned = 0;

  size_t count = lim.rephased[stable]++;
  bool single;
  char type;

  if (opts.stabilize && opts.stabilizeonly)
    single = true;
  else
    single = !opts.stabilize;

  if (single && !opts.walk) {
    // (inverted,best,flipping,best,random,best,original,best)^\omega
    switch (count % 8) {
    case 0:
      type = rephase_inverted ();
      break;
    case 1:
      type = rephase_best ();
      break;
    case 2:
      type = rephase_flipping ();
      break;
    case 3:
      type = rephase_best ();
      break;
    case 4:
      type = rephase_random ();
      break;
    case 5:
      type = rephase_best ();
      break;
    case 6:
      type = rephase_original ();
      break;
    case 7:
      type = rephase_best ();
      break;
    default:
      type = 0;
      break;
    }
  } else if (single && opts.walk) {
    // (inverted,best,walk,
    //  flipping,best,walk,
    //    random,best,walk,
    //  original,best,walk)^\omega
    switch (count % 12) {
    case 0:
      type = rephase_inverted ();
      break;
    case 1:
      type = rephase_best ();
      break;
    case 2:
      type = rephase_walk ();
      break;
    case 3:
      type = rephase_flipping ();
      break;
    case 4:
      type = rephase_best ();
      break;
    case 5:
      type = rephase_walk ();
      break;
    case 6:
      type = rephase_random ();
      break;
    case 7:
      type = rephase_best ();
      break;
    case 8:
      type = rephase_walk ();
      break;
    case 9:
      type = rephase_original ();
      break;
    case 10:
      type = rephase_best ();
      break;
    case 11:
      type = rephase_walk ();
      break;
    default:
      type = 0;
      break;
    }
  } else if (stable && !opts.walk) {
    // original,inverted,(best,original,best,inverted)^\omega
    if (!count)
      type = rephase_original ();
    else if (count == 1)
      type = rephase_inverted ();
    else
      switch ((count - 2) % 4) {
      case 0:
        type = rephase_best ();
        break;
      case 1:
        type = rephase_original ();
        break;
      case 2:
        type = rephase_best ();
        break;
      case 3:
        type = rephase_inverted ();
        break;
      default:
        type = 0;
        break;
      }
  } else if (stable && opts.walk) {
    // original,inverted,(best,walk,original,best,walk,inverted)^\omega
    if (!count)
      type = rephase_original ();
    else if (count == 1)
      type = rephase_inverted ();
    else
      switch ((count - 2) % 6) {
      case 0:
        type = rephase_best ();
        break;
      case 1:
        type = rephase_walk ();
        break;
      case 2:
        type = rephase_original ();
        break;
      case 3:
        type = rephase_best ();
        break;
      case 4:
        type = rephase_walk ();
        break;
      case 5:
        type = rephase_inverted ();
        break;
      default:
        type = 0;
        break;
      }
  } else if (!stable && (!opts.walk || !opts.walknonstable)) {
    // flipping,(random,best,flipping,best)^\omega
    if (!count)
      type = rephase_flipping ();
    else
      switch ((count - 1) % 4) {
      case 0:
        type = rephase_random ();
        break;
      case 1:
        type = rephase_best ();
        break;
      case 2:
        type = rephase_flipping ();
        break;
      case 3:
        type = rephase_best ();
        break;
      default:
        type = 0;
        break;
      }
  } else {
    CADICAL_assert (!stable && opts.walk && opts.walknonstable);
    // flipping,(random,best,walk,flipping,best,walk)^\omega
    if (!count)
      type = rephase_flipping ();
    else
      switch ((count - 1) % 6) {
      case 0:
        type = rephase_random ();
        break;
      case 1:
        type = rephase_best ();
        break;
      case 2:
        type = rephase_walk ();
        break;
      case 3:
        type = rephase_flipping ();
        break;
      case 4:
        type = rephase_best ();
        break;
      case 5:
        type = rephase_walk ();
        break;
      default:
        type = 0;
        break;
      }
  }
  CADICAL_assert (type);

  int64_t delta = opts.rephaseint * (stats.rephased.total + 1);
  lim.rephase = stats.conflicts + delta;

  PHASE ("rephase", stats.rephased.total,
         "new rephase limit %" PRId64 " after %" PRId64 " conflicts",
         lim.rephase, delta);

  // This will trigger to report the effect of this new set of phases at the
  // 'backtrack' (actually 'update_target_and_best') after the next
  // conflict, as well as resetting 'best_assigned' then to allow to compute
  // a new "best" assignment at that point.
  //
  last.rephase.conflicts = stats.conflicts;
  rephased = type;

  if (stable)
    shuffle_scores ();
  else
    shuffle_queue ();
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
