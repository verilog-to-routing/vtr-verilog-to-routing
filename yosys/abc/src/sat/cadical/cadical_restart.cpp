#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

// As observed by Chanseok Oh and implemented in MapleSAT solvers too,
// various mostly satisfiable instances benefit from long quiet phases
// with less or almost no restarts.  We implement this idea by prohibiting
// the Glucose style restart scheme in a geometric fashion, which is very
// similar to how originally restarts were scheduled in MiniSAT and earlier
// solvers.  We start with say 1e3 = 1000 (opts.stabilizeinit) conflicts of
// Glucose restarts.  Then in a "stabilizing" phase we disable these
// until 1e4 = 2000 conflicts (if 'opts.stabilizefactor' is '200' percent)
// have passed. After that we switch back to regular Glucose style restarts
// until again 2 times more conflicts than the previous limit are reached.
// Actually, in the latest version we still restarts during stabilization
// but only in a reluctant doubling scheme with a rather high interval.

bool Internal::stabilizing () {
  if (!opts.stabilize)
    return false;
  if (stable && opts.stabilizeonly)
    return true;
  if (!inc.stabilize) {
    CADICAL_assert (!stable);
    if (stats.conflicts <= lim.stabilize)
      return false;
  } else if (stats.ticks.search[stable] <= lim.stabilize)
    return stable;
  report (stable ? ']' : '}');
  if (stable)
    STOP (stable);
  else
    STOP (unstable);
  const int64_t delta_conflicts =
      stats.conflicts - last.stabilize.conflicts;
  const int64_t delta_ticks =
      stats.ticks.search[stable] - last.stabilize.ticks;
  const char *current_mode = stable ? "stable" : "unstable";
  const char *next_mode = stable ? "unstable" : "stable";
  PHASE ("stabilizing", stats.stabphases,
         "reached %s stabilization limit %" PRId64 " after %" PRId64
         " conflicts and %" PRId64 " ticks at %" PRId64
         " conflicts and %" PRId64 " ticks",
         current_mode, lim.stabilize, delta_conflicts, delta_ticks,
         stats.conflicts, stats.ticks.search[stable]);
  if (!inc.stabilize)
    inc.stabilize = delta_ticks;
  if (!inc.stabilize) // rare occurence in incremental calls requiring no
                      // ticks
    inc.stabilize = 1;

  stable = !stable; // Switch!!!!!

  int64_t next_delta_ticks = inc.stabilize;
  int64_t stabphases = stats.stabphases + 1;
  next_delta_ticks *= stabphases * stabphases;

  lim.stabilize = stats.ticks.search[stable] + next_delta_ticks;
  if (lim.stabilize <= stats.ticks.search[stable])
    lim.stabilize = stats.ticks.search[stable] + 1;

  if (stable)
    stats.stabphases++;

  swap_averages ();
  PHASE ("stabilizing", stats.stabphases,
         "next %s stabilization limit %" PRId64
         " at ticks interval %" PRId64,
         next_mode, lim.stabilize, next_delta_ticks);
  report (stable ? '[' : '{');
  if (stable)
    START (stable);
  else
    START (unstable);
  return stable;
}

// Restarts are scheduled by a variant of the Glucose scheme as presented
// in our POS'15 paper using exponential moving averages.  There is a slow
// moving average of the average recent glucose level of learned clauses
// as well as a fast moving average of those glues.  If the end of a base
// restart conflict interval has passed and the fast moving average is
// above a certain margin over the slow moving average then we restart.

bool Internal::restarting () {
  if (!opts.restart)
    return false;
  if ((size_t) level < assumptions.size () + 2)
    return false;
  if (stabilizing ())
    return reluctant;
  if (stats.conflicts <= lim.restart)
    return false;
  double f = averages.current.glue.fast;
  double margin = (100.0 + opts.restartmargin) / 100.0;
  double s = averages.current.glue.slow, l = margin * s;
  LOG ("EMA glue slow %.2f fast %.2f limit %.2f", s, f, l);
  return l <= f;
}

// This is Marijn's reuse trail idea.  Instead of always backtracking to
// the top we figure out which decisions will be made again anyhow and
// only backtrack to the level of the last such decision or to the top if
// no such decision exists top (in which case we do not reuse any level).

int Internal::reuse_trail () {
  const int trivial_decisions =
      assumptions.size ()
      // Plus 1 if the constraint is satisfied via implications of
      // assumptions and a pseudo-decision level was introduced.
      + !control[assumptions.size () + 1].decision;
  if (!opts.restartreusetrail)
    return trivial_decisions;
  int next_decision = next_decision_variable ();
  CADICAL_assert (1 <= next_decision);
  int res = trivial_decisions;
  if (use_scores ()) {
    while (res < level) {
      int decision = control[res + 1].decision;
      if (decision && score_smaller (this) (abs (decision), next_decision))
        break;
      res++;
    }
  } else {
    int64_t limit = bumped (next_decision);
    while (res < level) {
      int decision = control[res + 1].decision;
      if (decision && bumped (decision) < limit)
        break;
      res++;
    }
  }
  int reused = res - trivial_decisions;
  if (reused > 0) {
    stats.reused++;
    stats.reusedlevels += reused;
    if (stable)
      stats.reusedstable++;
  }
  return res;
}

void Internal::restart () {
  START (restart);
  stats.restarts++;
  stats.restartlevels += level;
  if (stable)
    stats.restartstable++;
  LOG ("restart %" PRId64 "", stats.restarts);
  backtrack (reuse_trail ());

  lim.restart = stats.conflicts + opts.restartint;
  LOG ("new restart limit at %" PRId64 " conflicts", lim.restart);

  report ('R', 2);
  STOP (restart);
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
