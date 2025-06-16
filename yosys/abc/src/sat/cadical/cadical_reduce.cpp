#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// Once in a while we reduce, e.g., we remove learned clauses which are
// supposed to be less useful in the future.  This is done in increasing
// intervals, which has the effect of allowing more and more learned clause
// to be kept for a longer period.  The number of learned clauses kept
// in memory corresponds to an upper bound on the 'space' of a resolution
// proof needed to refute a formula in proof complexity sense.

bool Internal::reducing () {
  if (!opts.reduce)
    return false;
  if (!stats.current.redundant)
    return false;
  return stats.conflicts >= lim.reduce;
}

/*------------------------------------------------------------------------*/

// Even less regularly we are flushing all redundant clauses.

bool Internal::flushing () {
  if (!opts.flush)
    return false;
  return stats.conflicts >= lim.flush;
}

/*------------------------------------------------------------------------*/

void Internal::mark_clauses_to_be_flushed () {
  const int tier1limit = tier1[false];
  const int tier2limit = max (tier1limit, tier2[false]);
  for (const auto &c : clauses) {
    if (!c->redundant)
      continue; // keep irredundant
    if (c->garbage)
      continue; // already marked as garbage
    if (c->reason)
      continue; // need to keep reasons
    const unsigned used = c->used;
    if (used)
      c->used--;
    if (c->glue < tier1limit && used)
      continue;
    if (c->glue < tier2limit && used >= max_used - 1)
      continue;
    mark_garbage (c); // flush unused clauses
    if (c->hyper)
      stats.flush.hyper++;
    else
      stats.flush.learned++;
  }
  // No change to 'lim.kept{size,glue}'.
}

/*------------------------------------------------------------------------*/

// Clauses of larger glue or larger size are considered less useful.
//
// We also follow the observations made by the Glucose team in their
// IJCAI'09 paper and keep all low glue clauses limited by
// 'options.keepglue' (typically '2').
//
// In earlier versions we pre-computed a 64-bit sort key per clause and
// wrapped a pointer to the clause and the 64-bit sort key into a separate
// data structure for sorting.  This was probably faster but awkward and
// so we moved back to a simpler scheme which also uses 'stable_sort'
// instead of 'rsort' below.  Sorting here is not a hot-spot anyhow.

struct reduce_less_useful {
  bool operator() (const Clause *c, const Clause *d) const {
    if (c->glue > d->glue)
      return true;
    if (c->glue < d->glue)
      return false;
    return c->size > d->size;
  }
};

// This function implements the important reduction policy. It determines
// which redundant clauses are considered not useful and thus will be
// collected in a subsequent garbage collection phase.

void Internal::mark_useless_redundant_clauses_as_garbage () {

  // We use a separate stack for sorting candidates for removal.  This uses
  // (slightly) more memory but has the advantage to keep the relative order
  // in 'clauses' intact, which actually due to using stable sorting goes
  // into the candidate selection (more recently learned clauses are kept if
  // they otherwise have the same glue and size).

  vector<Clause *> stack;
  const int tier1limit = tier1[false];
  const int tier2limit = max (tier1limit, tier2[false]);

  stack.reserve (stats.current.redundant);

  for (const auto &c : clauses) {
    if (!c->redundant)
      continue; // Keep irredundant.
    if (c->garbage)
      continue; // Skip already marked.
    if (c->reason)
      continue; // Need to keep reasons.
    const unsigned used = c->used;
    if (used)
      c->used--;
    if (c->glue <= tier1limit && used)
      continue;
    if (c->glue <= tier2limit && used >= max_used - 1)
      continue;
    if (c->hyper) {          // Hyper binary and ternary resolvents
      CADICAL_assert (c->size <= 3); // are only kept for one reduce round
      if (!used)
        mark_garbage (c); // unless
      continue;           //  used recently.
    }
    stack.push_back (c);
  }

  stable_sort (stack.begin (), stack.end (), reduce_less_useful ());
  size_t target = 1e-2 * opts.reducetarget * stack.size ();

  // This is defensive code, which I usually consider a bug, but here I am
  // just not sure that using floating points in the line above is precise
  // in all situations and instead of figuring that out, I just use this.
  //
  if (target > stack.size ())
    target = stack.size ();

  PHASE ("reduce", stats.reductions, "reducing %zd clauses %.0f%%", target,
         percent (target, stats.current.redundant));

  auto i = stack.begin ();
  const auto t = i + target;
  while (i != t) {
    Clause *c = *i++;
    LOG (c, "marking useless to be collected");
    mark_garbage (c);
    stats.reduced++;
  }

  lim.keptsize = lim.keptglue = 0;

  const auto end = stack.end ();
  for (i = t; i != end; i++) {
    Clause *c = *i;
    LOG (c, "keeping");
    if (c->size > lim.keptsize)
      lim.keptsize = c->size;
    if (c->glue > lim.keptglue)
      lim.keptglue = c->glue;
  }

  erase_vector (stack);

  PHASE ("reduce", stats.reductions, "maximum kept size %d glue %d",
         lim.keptsize, lim.keptglue);
}

/*------------------------------------------------------------------------*/

// If chronological backtracking produces out-of-order assigned units, then
// it is necessary to completely propagate them at the root level in order
// to derive all implied units.  Otherwise the blocking literals in
// 'flush_watches' are messed up and CADICAL_assertion 'FW1' fails.

bool Internal::propagate_out_of_order_units () {
  if (!level)
    return true;
  int oou = 0;
  for (size_t i = control[1].trail; !oou && i < trail.size (); i++) {
    const int lit = trail[i];
    CADICAL_assert (val (lit) > 0);
    if (var (lit).level)
      continue;
    LOG ("found out-of-order assigned unit %d", oou);
    oou = lit;
  }
  if (!oou)
    return true;
  CADICAL_assert (opts.chrono || external_prop);
  backtrack (0);
  if (propagate ())
    return true;
  learn_empty_clause ();
  return false;
}

/*------------------------------------------------------------------------*/

// reduction is scheduled with reduceint, reducetarget and reduceopt.
// with reduceopt=1 the number of learnt clauses scale with
// sqrt of conflicts times reduceint
// the scaling is the same as with reduceopt=0 (the classical default)
// however, the constants are different. To avoid this (and get roughly the
// same behaviour with reduceopt=0 and reduceopt=1) we need to scale the
// interval, namely (reduceint^2/2)
// Lastly, reduceopt=2 just replaces sqrt conflicts with log conflicts.
// The learnt clauses should not be bigger than
// 1/reducetarget * reduceint * function (conflicts)
// for function being log if reduceint=2 an sqrt otherwise.
// This is however only the theoretical target and second chance for
// tier2 clauses and very long lifespan of tier1 clauses (through used flag)
// make this behave differently.
// reduceinit shifts the curve to the right, increasing the number of
// clauses in the solver. This impact will decrease over time.

void Internal::reduce () {
  START (reduce);

  stats.reductions++;
  report ('.', 1);

  bool flush = flushing ();
  if (flush)
    stats.flush.count++;

  if (!propagate_out_of_order_units ())
    goto DONE;

  mark_satisfied_clauses_as_garbage ();
  protect_reasons ();
  if (flush)
    mark_clauses_to_be_flushed ();
  else
    mark_useless_redundant_clauses_as_garbage ();
  garbage_collection ();

  {
    int64_t delta = opts.reduceint;
    double factor = stats.reductions + 1;
    if (opts.reduceopt ==
        0) // adjust delta such this is the same as reduceopt=1
      delta = delta * delta / 2;
    else if (opts.reduceopt == 1) {
      // this is the same as reduceopt=0 if reduceint = sqrt (reduceint) =
      // 17
      factor = sqrt ((double) stats.conflicts);
    } else if (opts.reduceopt == 2)
      // log scaling instead
      factor = log ((double) stats.conflicts);
    if (factor < 1)
      factor = 1;
    delta = delta * factor;
    if (irredundant () > 1e5) {
      delta *= log (irredundant () / 1e4) / log (10);
    }
    if (delta < 1)
      delta = 1;
    lim.reduce = stats.conflicts + delta;
    PHASE ("reduce", stats.reductions,
           "new reduce limit %" PRId64 " after %" PRId64 " conflicts",
           lim.reduce, delta);
  }

  if (flush) {
    PHASE ("flush", stats.flush.count, "new flush increment %" PRId64 "",
           inc.flush);
    inc.flush *= opts.flushfactor;
    lim.flush = stats.conflicts + inc.flush;
    PHASE ("flush", stats.flush.count, "new flush limit %" PRId64 "",
           lim.flush);
  }

  last.reduce.conflicts = stats.conflicts;

DONE:

  report (flush ? 'f' : '-');
  STOP (reduce);
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
