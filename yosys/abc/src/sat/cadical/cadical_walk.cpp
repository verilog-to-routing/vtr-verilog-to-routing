#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// Random walk local search based on 'ProbSAT' ideas.

struct Walker {

  Internal *internal;

  Random random;           // local random number generator
  int64_t propagations;    // number of propagations
  int64_t limit;           // limit on number of propagations
  vector<Clause *> broken; // currently unsatisfied clauses
  double epsilon;          // smallest considered score
  vector<double> table;    // break value to score table
  vector<double> scores;   // scores of candidate literals

  double score (unsigned); // compute score from break count

  Walker (Internal *, double size, int64_t limit);
};

// These are in essence the CB values from Adrian Balint's thesis.  They
// denote the inverse 'cb' of the base 'b' of the (probability) weight
// 'b^-i' for picking a literal with the break value 'i' (first column is
// the 'size', second the 'CB' value).

static double cbvals[][2] = {
    {0.0, 2.00}, {3.0, 2.50}, {4.0, 2.85}, {5.0, 3.70},
    {6.0, 5.10}, {7.0, 7.40}, // Adrian has '5.4', but '7.4' looks better.
};

static const int ncbvals = sizeof cbvals / sizeof cbvals[0];

// We interpolate the CB values for uniform random SAT formula to the non
// integer situation of average clause size by piecewise linear functions.
//
//   y2 - y1
//   ------- * (x - x1) + y1
//   x2 - x1
//
// where 'x' is the average size of clauses and 'y' the CB value.

inline static double fitcbval (double size) {
  int i = 0;
  while (i + 2 < ncbvals &&
         (cbvals[i][0] > size || cbvals[i + 1][0] < size))
    i++;
  const double x2 = cbvals[i + 1][0], x1 = cbvals[i][0];
  const double y2 = cbvals[i + 1][1], y1 = cbvals[i][1];
  const double dx = x2 - x1, dy = y2 - y1;
  CADICAL_assert (dx);
  const double res = dy * (size - x1) / dx + y1;
  CADICAL_assert (res > 0);
  return res;
}

// Initialize the data structures for one local search round.

Walker::Walker (Internal *i, double size, int64_t l)
    : internal (i), random (internal->opts.seed), // global random seed
      propagations (0), limit (l) {
  random += internal->stats.walk.count; // different seed every time

  // This is the magic constant in ProbSAT (also called 'CB'), which we pick
  // according to the average size every second invocation and otherwise
  // just the default '2.0', which turns into the base '0.5'.
  //
  const bool use_size_based_cb = (internal->stats.walk.count & 1);
  const double cb = use_size_based_cb ? fitcbval (size) : 2.0;
  CADICAL_assert (cb);
  const double base = 1 / cb; // scores are 'base^0,base^1,base^2,...

  double next = 1;
  for (epsilon = next; next; next = epsilon * base)
    table.push_back (epsilon = next);

  PHASE ("walk", internal->stats.walk.count,
         "CB %.2f with inverse %.2f as base and table size %zd", cb, base,
         table.size ());
}

// The scores are tabulated for faster computation (to avoid 'pow').

inline double Walker::score (unsigned i) {
  const double res = (i < table.size () ? table[i] : epsilon);
  LOG ("break %u mapped to score %g", i, res);
  return res;
}

/*------------------------------------------------------------------------*/

Clause *Internal::walk_pick_clause (Walker &walker) {
  require_mode (WALK);
  CADICAL_assert (!walker.broken.empty ());
  int64_t size = walker.broken.size ();
  if (size > INT_MAX)
    size = INT_MAX;
  int pos = walker.random.pick_int (0, size - 1);
  Clause *res = walker.broken[pos];
  LOG (res, "picking random position %d", pos);
  return res;
}

/*------------------------------------------------------------------------*/

// Compute the number of clauses which would be become unsatisfied if 'lit'
// is flipped and set to false.  This is called the 'break-count' of 'lit'.

unsigned Internal::walk_break_value (int lit) {

  require_mode (WALK);
  CADICAL_assert (val (lit) > 0);

  unsigned res = 0; // The computed break-count of 'lit'.

  for (auto &w : watches (lit)) {
    CADICAL_assert (w.blit != lit);
    if (val (w.blit) > 0)
      continue;
    if (w.binary ()) {
      res++;
      continue;
    }

    Clause *c = w.clause;
    CADICAL_assert (lit == c->literals[0]);

    // Now try to find a second satisfied literal starting at 'literals[1]'
    // shifting all the traversed literals to right by one position in order
    // to move such a second satisfying literal to 'literals[1]'.  This move
    // to front strategy improves the chances to find the second satisfying
    // literal earlier in subsequent break-count computations.
    //
    auto begin = c->begin () + 1;
    const auto end = c->end ();
    auto i = begin;
    int prev = 0;
    while (i != end) {
      const int other = *i;
      *i++ = prev;
      prev = other;
      if (val (other) < 0)
        continue;

      // Found 'other' as second satisfying literal.

      w.blit = other; // Update 'blit'
      *begin = other; // and move to front.

      break;
    }

    if (i != end)
      continue; // Double satisfied!

    // Otherwise restore literals (undo shift to the right).
    //
    while (i != begin) {
      const int other = *--i;
      *i = prev;
      prev = other;
    }

    res++; // Literal 'lit' single satisfies clause 'c'.
  }

  return res;
}

/*------------------------------------------------------------------------*/

// Given an unsatisfied clause 'c', in which we want to flip a literal, we
// first determine the exponential score based on the break-count of its
// literals and then sample the literals based on these scores.  The CB
// value is smaller than one and thus the score is exponentially decreasing
// with the break-count increasing.  The sampling works as in 'ProbSAT' and
// 'YalSAT' by summing up the scores and then picking a random limit in the
// range of zero to the sum, then summing up the scores again and picking
// the first literal which reaches the limit.  Note, that during incremental
// SAT solving we can not flip assumed variables.  Those are assigned at
// decision level one, while the other variables are assigned at two.

int Internal::walk_pick_lit (Walker &walker, Clause *c) {
  LOG ("picking literal by break-count");
  CADICAL_assert (walker.scores.empty ());
  double sum = 0;
  int64_t propagations = 0;
  for (const auto lit : *c) {
    CADICAL_assert (active (lit));
    if (var (lit).level == 1) {
      LOG ("skipping assumption %d for scoring", -lit);
      continue;
    }
    CADICAL_assert (active (lit));
    propagations++;
    unsigned tmp = walk_break_value (-lit);
    double score = walker.score (tmp);
    LOG ("literal %d break-count %u score %g", lit, tmp, score);
    walker.scores.push_back (score);
    sum += score;
  }
  LOG ("scored %zd literals", walker.scores.size ());
  CADICAL_assert (!walker.scores.empty ());
  walker.propagations += propagations;
  stats.propagations.walk += propagations;
  CADICAL_assert (walker.scores.size () <= (size_t) c->size);
  const double lim = sum * walker.random.generate_double ();
  LOG ("score sum %g limit %g", sum, lim);
  const auto end = c->end ();
  auto i = c->begin ();
  auto j = walker.scores.begin ();
  int res;
  for (;;) {
    CADICAL_assert (i != end);
    res = *i++;
    if (var (res).level > 1)
      break;
    LOG ("skipping assumption %d without score", -res);
  }
  sum = *j++;
  while (sum <= lim && i != end) {
    res = *i++;
    if (var (res).level == 1) {
      LOG ("skipping assumption %d without score", -res);
      continue;
    }
    sum += *j++;
  }
  walker.scores.clear ();
  LOG ("picking literal %d by break-count", res);
  return res;
}

/*------------------------------------------------------------------------*/

void Internal::walk_flip_lit (Walker &walker, int lit) {

  require_mode (WALK);
  LOG ("flipping assign %d", lit);
  CADICAL_assert (val (lit) < 0);

  // First flip the literal value.
  //
  const int tmp = sign (lit);
  const int idx = abs (lit);
  set_val (idx, tmp);
  CADICAL_assert (val (lit) > 0);

  // Then remove 'c' and all other now satisfied (made) clauses.
  {
    // Simply go over all unsatisfied (broken) clauses.

    LOG ("trying to make %zd broken clauses", walker.broken.size ());

    // We need to measure (and bound) the memory accesses during traversing
    // broken clauses in terms of 'propagations'. This is tricky since we
    // are not actually propagating literals.  Instead we use the clause
    // variable 'ratio' as an approximation to the number of clauses used
    // during propagating a literal.  Note that we use a one-watch scheme.
    // Accordingly the number of broken clauses traversed divided by that
    // ratio is an approximation of the number of propagation this would
    // correspond to (in terms of memory access).  To eagerly update these
    // statistics we simply increment the propagation counter after every
    // 'ratio' traversed clause.  These propagations are particularly
    // expensive if the number of broken clauses is large which usually
    // happens initially.
    //
    const double ratio = clause_variable_ratio ();
    const auto eou = walker.broken.end ();
    auto j = walker.broken.begin (), i = j;
#ifdef LOGGING
    int64_t made = 0;
#endif
    int64_t count = 0;

    while (i != eou) {

      Clause *d = *j++ = *i++;

      int *literals = d->literals, prev = 0;

      // Find 'lit' in 'd'.
      //
      const int size = d->size;
      for (int i = 0; i < size; i++) {
        const int other = literals[i];
        CADICAL_assert (active (other));
        literals[i] = prev;
        prev = other;
        if (other == lit)
          break;
        CADICAL_assert (val (other) < 0);
      }

      // If 'lit' is in 'd' then move it to the front to watch it.
      //
      if (prev == lit) {
        literals[0] = lit;
        LOG (d, "made");
        watch_literal (literals[0], literals[1], d);
#ifdef LOGGING
        made++;
#endif
        j--;

      } else { // Otherwise the clause is not satisfied, undo shift.

        for (int i = size - 1; i >= 0; i--) {
          int other = literals[i];
          literals[i] = prev;
          prev = other;
        }
      }

      if (count--)
        continue;

      // Update these counters eagerly.  Otherwise if we delay the update
      // until all clauses are traversed, interrupting the solver has a high
      // chance of giving bogus statistics on the number of 'propagations'
      // in 'walk', if it is interrupted in this loop.

      count = ratio; // Starting counting down again.
      walker.propagations++;
      stats.propagations.walk++;
    }
    LOG ("made %" PRId64 " clauses by flipping %d", made, lit);
    walker.broken.resize (j - walker.broken.begin ());
  }

  // Finally add all new unsatisfied (broken) clauses.
  {
    walker.propagations++;     // This really corresponds now to one
    stats.propagations.walk++; // propagation (in a one-watch scheme).

#ifdef LOGGING
    int64_t broken = 0;
#endif
    Watches &ws = watches (-lit);

    LOG ("trying to break %zd watched clauses", ws.size ());

    for (const auto &w : ws) {
      Clause *d = w.clause;
      LOG (d, "unwatch %d in", -lit);
      int *literals = d->literals, replacement = 0, prev = -lit;
      CADICAL_assert (literals[0] == -lit);
      const int size = d->size;
      for (int i = 1; i < size; i++) {
        const int other = literals[i];
        CADICAL_assert (active (other));
        literals[i] = prev; // shift all to right
        prev = other;
        const signed char tmp = val (other);
        if (tmp < 0)
          continue;
        replacement = other; // satisfying literal
        break;
      }
      if (replacement) {
        literals[1] = -lit;
        literals[0] = replacement;
        CADICAL_assert (-lit != replacement);
        watch_literal (replacement, -lit, d);
      } else {
        for (int i = size - 1; i > 0; i--) { // undo shift
          const int other = literals[i];
          literals[i] = prev;
          prev = other;
        }
        CADICAL_assert (literals[0] == -lit);
        LOG (d, "broken");
        walker.broken.push_back (d);
#ifdef LOGGING
        broken++;
#endif
      }
    }
    LOG ("broken %" PRId64 " clauses by flipping %d", broken, lit);
    ws.clear ();
  }
}

/*------------------------------------------------------------------------*/

// Check whether to save the current phases as new global minimum.

inline void Internal::walk_save_minimum (Walker &walker) {
  int64_t broken = walker.broken.size ();
  if (broken >= stats.walk.minimum)
    return;
  VERBOSE (3, "new global minimum %" PRId64 "", broken);
  stats.walk.minimum = broken;
  for (auto i : vars) {
    const signed char tmp = vals[i];
    if (tmp)
      phases.min[i] = phases.saved[i] = tmp;
  }
}

/*------------------------------------------------------------------------*/

int Internal::walk_round (int64_t limit, bool prev) {

  backtrack ();
  if (propagated < trail.size () && !propagate ()) {
    LOG ("empty clause after root level propagation");
    learn_empty_clause ();
    return 20;
  }

  stats.walk.count++;

  clear_watches ();

  // Remove all fixed variables first (assigned at decision level zero).
  //
  if (last.collect.fixed < stats.all.fixed)
    garbage_collection ();

#ifndef CADICAL_QUIET
  // We want to see more messages during initial local search.
  //
  if (localsearching) {
    CADICAL_assert (!force_phase_messages);
    force_phase_messages = true;
  }
#endif

  PHASE ("walk", stats.walk.count,
         "random walk limit of %" PRId64 " propagations", limit);

  // First compute the average clause size for picking the CB constant.
  //
  double size = 0;
  int64_t n = 0;
  for (const auto c : clauses) {
    if (c->garbage)
      continue;
    if (c->redundant) {
      if (!opts.walkredundant)
        continue;
      if (!likely_to_be_kept_clause (c))
        continue;
    }
    size += c->size;
    n++;
  }
  double average_size = relative (size, n);

  PHASE ("walk", stats.walk.count,
         "%" PRId64 " clauses average size %.2f over %d variables", n,
         average_size, active ());

  // Instantiate data structures for this local search round.
  //
  Walker walker (internal, average_size, limit);

  bool failed = false; // Inconsistent assumptions?

  level = 1; // Assumed variables assigned at level 1.

  if (assumptions.empty ()) {
    LOG ("no assumptions so assigning all variables to decision phase");
  } else {
    LOG ("assigning assumptions to their forced phase first");
    for (const auto lit : assumptions) {
      signed char tmp = val (lit);
      if (tmp > 0)
        continue;
      if (tmp < 0) {
        LOG ("inconsistent assumption %d", lit);
        failed = true;
        break;
      }
      if (!active (lit))
        continue;
      tmp = sign (lit);
      const int idx = abs (lit);
      LOG ("initial assign %d to assumption phase", tmp < 0 ? -idx : idx);
      set_val (idx, tmp);
      CADICAL_assert (level == 1);
      var (idx).level = 1;
    }
    if (!failed)
      LOG ("now assigning remaining variables to their decision phase");
  }

  level = 2; // All other non assumed variables assigned at level 2.

  if (!failed) {

    for (auto idx : vars) {
      if (!active (idx)) {
        LOG ("skipping inactive variable %d", idx);
        continue;
      }
      if (vals[idx]) {
        CADICAL_assert (var (idx).level == 1);
        LOG ("skipping assumed variable %d", idx);
        continue;
      }
      int tmp = 0;
      if (prev)
        tmp = phases.prev[idx];
      if (!tmp)
        tmp = sign (decide_phase (idx, true));
      CADICAL_assert (tmp == 1 || tmp == -1);
      set_val (idx, tmp);
      CADICAL_assert (level == 2);
      var (idx).level = 2;
      LOG ("initial assign %d to decision phase", tmp < 0 ? -idx : idx);
    }

    LOG ("watching satisfied and registering broken clauses");
#ifdef LOGGING
    int64_t watched = 0;
#endif
    for (const auto c : clauses) {

      if (c->garbage)
        continue;
      if (c->redundant) {
        if (!opts.walkredundant)
          continue;
        if (!likely_to_be_kept_clause (c))
          continue;
      }

      bool satisfiable = false; // contains not only assumptions
      int satisfied = 0;        // clause satisfied?

      int *lits = c->literals;
      const int size = c->size;

      // Move to front satisfied literals and determine whether there
      // is at least one (non-assumed) literal that can be flipped.
      //
      for (int i = 0; satisfied < 2 && i < size; i++) {
        const int lit = lits[i];
        CADICAL_assert (active (lit)); // Due to garbage collection.
        if (val (lit) > 0) {
          swap (lits[satisfied], lits[i]);
          if (!satisfied++)
            LOG ("first satisfying literal %d", lit);
        } else if (!satisfiable && var (lit).level > 1) {
          LOG ("non-assumption potentially satisfying literal %d", lit);
          satisfiable = true;
        }
      }

      if (!satisfied && !satisfiable) {
        LOG (c, "due to assumptions unsatisfiable");
        LOG ("stopping local search since assumptions falsify a clause");
        failed = true;
        break;
      }

      if (satisfied) {
        watch_literal (lits[0], lits[1], c);
#ifdef LOGGING
        watched++;
#endif
      } else {
        CADICAL_assert (satisfiable); // at least one non-assumed variable ...
        LOG (c, "broken");
        walker.broken.push_back (c);
      }
    }
#ifdef LOGGING
    if (!failed) {
      int64_t broken = walker.broken.size ();
      int64_t total = watched + broken;
      LOG ("watching %" PRId64 " clauses %.0f%% "
           "out of %" PRId64 " (watched and broken)",
           watched, percent (watched, total), total);
    }
#endif
  }

  int64_t old_global_minimum = stats.walk.minimum;

  int res; // Tells caller to continue with local search.

  if (!failed) {

    int64_t broken = walker.broken.size ();

    PHASE ("walk", stats.walk.count,
           "starting with %" PRId64 " unsatisfied clauses "
           "(%.0f%% out of %" PRId64 ")",
           broken, percent (broken, stats.current.irredundant),
           stats.current.irredundant);

    walk_save_minimum (walker);

    int64_t minimum = broken;
#ifndef CADICAL_QUIET
    int64_t flips = 0;
#endif
    while (!terminated_asynchronously () && !walker.broken.empty () &&
           walker.propagations < walker.limit) {
#ifndef CADICAL_QUIET
      flips++;
#endif
      stats.walk.flips++;
      stats.walk.broken += broken;
      Clause *c = walk_pick_clause (walker);
      const int lit = walk_pick_lit (walker, c);
      walk_flip_lit (walker, lit);
      broken = walker.broken.size ();
      LOG ("now have %" PRId64 " broken clauses in total", broken);
      if (broken >= minimum)
        continue;
      minimum = broken;
      VERBOSE (3, "new phase minimum %" PRId64 " after %" PRId64 " flips",
               minimum, flips);
      walk_save_minimum (walker);
    }

    if (minimum < old_global_minimum)
      PHASE ("walk", stats.walk.count,
             "%snew global minimum %" PRId64 "%s in %" PRId64 " flips and "
             "%" PRId64 " propagations",
             tout.bright_yellow_code (), minimum, tout.normal_code (),
             flips, walker.propagations);
    else
      PHASE ("walk", stats.walk.count,
             "best phase minimum %" PRId64 " in %" PRId64 " flips and "
             "%" PRId64 " propagations",
             minimum, flips, walker.propagations);

    if (opts.profile >= 2) {
      PHASE ("walk", stats.walk.count,
             "%.2f million propagations per second",
             relative (1e-6 * walker.propagations,
                       time () - profiles.walk.started));

      PHASE ("walk", stats.walk.count, "%.2f thousand flips per second",
             relative (1e-3 * flips, time () - profiles.walk.started));

    } else {
      PHASE ("walk", stats.walk.count, "%.2f million propagations",
             1e-6 * walker.propagations);

      PHASE ("walk", stats.walk.count, "%.2f thousand flips", 1e-3 * flips);
    }

    if (minimum > 0) {
      LOG ("minimum %" PRId64 " non-zero thus potentially continue",
           minimum);
      res = 0;
    } else {
      LOG ("minimum is zero thus stop local search");
      res = 10;
    }

  } else {

    res = 20;

    PHASE ("walk", stats.walk.count,
           "aborted due to inconsistent assumptions");
  }

  copy_phases (phases.prev);

  for (auto idx : vars)
    if (active (idx))
      set_val (idx, 0);

  CADICAL_assert (level == 2);
  level = 0;

  clear_watches ();
  connect_watches ();

#ifndef CADICAL_QUIET
  if (localsearching) {
    CADICAL_assert (force_phase_messages);
    force_phase_messages = false;
  }
#endif

  return res;
}

void Internal::walk () {
  START_INNER_WALK ();
  int64_t limit = stats.propagations.search;
  limit *= 1e-3 * opts.walkeffort;
  if (limit < opts.walkmineff)
    limit = opts.walkmineff;
  if (limit > opts.walkmaxeff)
    limit = opts.walkmaxeff;
  (void) walk_round (limit, false);
  STOP_INNER_WALK ();
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
