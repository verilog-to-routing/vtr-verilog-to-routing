#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// This file implements a global forward subsumption algorithm, which is run
// frequently during search.  It works both on original (irredundant)
// clauses and on 'sticky' learned clauses which are likely to be kept.
// This is abstracted away in the 'likely_to_be_kept_clause' function, which
// implicitly relies on 'opts.reducetier1glue' (glucose level of clauses
// which are not reduced) as well as dynamically determined size and glucose
// level ('lim.keptglue' and 'lim.keptsize') of clauses kept in 'reduce'.
//
// Note, that 'forward' means that the clause from which the subsumption
// check is started is checked for being subsumed by other (smaller or equal
// size) clauses.  Since 'vivification' is an extended version of subsume,
// more powerful, but also slower, we schedule 'vivify' right after
// 'subsume', which in contrast to 'subsume' is not run until to completion.
//
// This implementation is inspired by Bayardo's SDM'11 analysis of our
// subsumption algorithm in our SATeLite preprocessor in the context of
// finding extremal sets in data mining and his suggested improvements.

// Our original subsumption algorithm in 'Quantor' and 'SATeLite' (and in
// MiniSAT and descendants) is based on backward subsumption.  It uses the
// observation that only the occurrence list of one literal of a clause has
// to be traversed in order to find all potential clauses which are subsumed
// by the candidate.  Thus the literal with the smallest number of
// occurrences is used.  However, that scheme requires to connect all
// literals of surviving clauses, while forward algorithms only need to
// connect one literal. On the other hand forward checking requires to
// traverse the occurrence lists of all literals of the candidate clause to
// find subsuming clauses.  During connecting the single literal (similar to
// the one-watch scheme by Lintao Zhang) one can connect a literal with a
// minimal number of occurrence so far, which implicitly also reduces future
// occurrence list traversal time.

// Also the actual subsumption check is cheaper since during backward
// checking the short subsuming candidate clause is marked and all the
// literals in the larger subsume candidate clauses have to be traversed,
// while for our forward approach the long subsumed candidate clause is only
// marked once, while the literals of the shorter subsuming clauses have to
// be checked.  We also use a fixed special more cache friendly data
// structure for binary clauses, to avoid traversing them directly.

// In our forward scheme it is still possible to skip occurrence lists of
// literals which were not added since the last subsumption round, since
// only those can contain subsuming candidates.  Actually, clauses which
// contain at least one literal, which was not added since the last
// subsumption round do not have to be connected at all, even though they
// might still be subsumed them self.

// Bayardo suggests to sort the literals in clauses and perform some kind of
// partial merge-sort, while we mark literals, but do sort literals during
// connecting a clause w.r.t. the number of occurrences, in order to find
// literals which do not occur in the subsumed candidate fast with high
// probability (less occurring literals have a higher chance).

// This is the actual subsumption and strengthening check.  We assume that
// all the literals of the candidate clause to be subsumed or strengthened
// are marked, so we only have to check that all the literals of the
// argument clause 'subsuming', which is checked for subsuming the candidate
// clause 'subsumed', has all its literals marked (in the correct phase).
// If exactly one is in the opposite phase we can still strengthen the
// candidate clause by this single literal which occurs in opposite phase.
//
// The result is INT_MIN if all literals are marked and thus the candidate
// clause can be subsumed.  It is zero if neither subsumption nor
// strengthening is possible.  Otherwise the candidate clause can be
// strengthened and as a result the negation of the literal which can be
// removed is returned.

inline int Internal::subsume_check (Clause *subsuming, Clause *subsumed) {
#ifdef CADICAL_NDEBUG
  (void) subsumed;
#endif
  // Only use 'subsumed' for these following CADICAL_assertion checks.  Otherwise we
  // only require that 'subsumed' has all its literals marked.
  //
  CADICAL_assert (!subsumed->garbage);
  CADICAL_assert (!subsuming->garbage);
  CADICAL_assert (subsuming != subsumed);
  CADICAL_assert (subsuming->size <= subsumed->size);

  stats.subchecks++;
  if (subsuming->size == 2)
    stats.subchecks2++;

  int flipped = 0, prev = 0;
  bool failed = false;
  const auto eoc = subsuming->end ();
  for (auto i = subsuming->begin (); !failed && i != eoc; i++) {
    int lit = *i;
    *i = prev;
    prev = lit;
    const int tmp = marked (lit);
    if (!tmp)
      failed = true;
    else if (tmp > 0)
      continue;
    else if (flipped)
      failed = true;
    else
      flipped = lit;
  }
  CADICAL_assert (prev);
  CADICAL_assert (!subsuming->literals[0]);
  subsuming->literals[0] = prev;
  if (failed)
    return 0;

  if (!flipped)
    return INT_MIN; // subsumed!!
  else if (!opts.subsumestr)
    return 0;
  else
    return flipped; // strengthen!!
}

/*------------------------------------------------------------------------*/

// Candidate clause 'subsumed' is subsumed by 'subsuming'.

inline void Internal::subsume_clause (Clause *subsuming, Clause *subsumed) {
  stats.subsumed++;
  CADICAL_assert (subsuming->size <= subsumed->size);
  LOG (subsumed, "subsumed");
  if (subsumed->redundant)
    stats.subred++;
  else
    stats.subirr++;
  if (subsumed->redundant || !subsuming->redundant) {
    mark_garbage (subsumed);
    return;
  }
  LOG ("turning redundant subsuming clause into irredundant clause");
  subsuming->redundant = false;
  if (proof)
    proof->strengthen (subsuming->id);
  mark_garbage (subsumed);
  stats.current.irredundant++;
  stats.added.irredundant++;
  stats.irrlits += subsuming->size;
  CADICAL_assert (stats.current.redundant > 0);
  stats.current.redundant--;
  CADICAL_assert (stats.added.redundant > 0);
  stats.added.redundant--;
  // ... and keep 'stats.added.total'.
}

/*------------------------------------------------------------------------*/

// Candidate clause 'c' is strengthened by removing 'lit'.

void Internal::strengthen_clause (Clause *c, int lit) {
  if (opts.check && is_external_forgettable (c->id))
    mark_garbage_external_forgettable (c->id);
  stats.strengthened++;
  CADICAL_assert (c->size > 2);
  LOG (c, "removing %d in", lit);
  if (proof) {
    LOG (lrat_chain, "strengthening clause with chain");
    proof->strengthen_clause (c, lit, lrat_chain);
  }
  if (!c->redundant)
    mark_removed (lit);
  auto new_end = remove (c->begin (), c->end (), lit);
  CADICAL_assert (new_end + 1 == c->end ()), (void) new_end;
  (void) shrink_clause (c, c->size - 1);
  // bump_clause2 (c);
  LOG (c, "strengthened");
  external->check_shrunken_clause (c);
}

/*------------------------------------------------------------------------*/

// Find clauses connected in the occurrence lists 'occs' which subsume the
// candidate clause 'c' given as first argument.  If this is the case the
// clause is subsumed and the result is positive.   If the clause was
// strengthened the result is negative.  Otherwise the candidate clause
// can not be subsumed nor strengthened and zero is returned.

inline int Internal::try_to_subsume_clause (Clause *c,
                                            vector<Clause *> &shrunken) {

  stats.subtried++;
  CADICAL_assert (!level);
  LOG (c, "trying to subsume");

  mark (c); // signed!

  Clause *d = 0;
  int flipped = 0;

  for (const auto &lit : *c) {

    // Only clauses which have a variable which has recently been added are
    // checked for being subsumed.  The idea is that all these newly added
    // clauses are candidates for subsuming the clause.  Then we also only
    // need to check occurrences of these variables.  The occurrence lists
    // of other literal do not have to be checked.
    //
    if (!flags (lit).subsume)
      continue;

    for (int sign = -1; !d && sign <= 1; sign += 2) {

      // First we check against all binary clauses.  The other literals of
      // all binary clauses of 'sign*lit' are stored in one consecutive
      // array, which is way faster than storing clause pointers and
      // dereferencing them.  Since this binary clause array is also not
      // shrunken, we also can bail out earlier if subsumption or
      // strengthening is determined.

      // In both cases the (self-)subsuming clause is stored in 'd', which
      // makes it nonzero and forces aborting both the outer and inner loop.
      // If the binary clause can strengthen the candidate clause 'c'
      // (through self-subsuming resolution), then 'filled' is set to the
      // literal which can be removed in 'c', otherwise to 'INT_MIN' which
      // is a non-valid literal.

      for (const auto &bin : bins (sign * lit)) {
        const auto &other = bin.lit;
        const int tmp = marked (other);
        if (!tmp)
          continue;
        if (tmp < 0 && sign < 0)
          continue;
        if (tmp < 0) {
          if (sign < 0)
            continue; // tautological resolvent
          dummy_binary->literals[0] = lit;
          dummy_binary->literals[1] = other;
          flipped = other;
        } else {
          dummy_binary->literals[0] = sign * lit;
          dummy_binary->literals[1] = other;
          flipped = (sign < 0) ? -lit : INT_MIN;
        }

        // This dummy binary clauses is initialized in 'Internal::Internal'
        // and only changes it literals in the lines above.   By using such
        // a faked binary clause we can simply reuse 'subsume_clause' as
        // well as the code around 'strengthen_clause' uniform for both real
        // clauses and this special case for binary clauses

        dummy_binary->id = bin.id;
        d = dummy_binary;

        break;
      }

      if (d)
        break;

      // In this second loop we check for larger than binary clauses to
      // subsume or strengthen the candidate clause.   This is more costly,
      // and needs a call to 'subsume_check'.  Otherwise the same contract
      // as above for communicating 'subsumption' or 'strengthening' to the
      // code after the loop is used.
      //
      const Occs &os = occs (sign * lit);
      for (const auto &e : os) {
        CADICAL_assert (!e->garbage); // sanity check
        if (e->garbage)
          continue; // defensive: not needed
        flipped = subsume_check (e, c);
        if (!flipped)
          continue;
        d = e; // leave also outer loop
        break;
      }
    }

    if (d)
      break;
  }

  unmark (c);

  if (flipped == INT_MIN) {
    LOG (d, "subsuming");
    subsume_clause (d, c);
    return 1;
  }

  if (flipped) {
    LOG (d, "strengthening");
    if (lrat) {
      CADICAL_assert (lrat_chain.empty ());
      lrat_chain.push_back (c->id);
      lrat_chain.push_back (d->id);
    }
    if (d->used > c->used)
      c->used = d->used;
    strengthen_clause (c, -flipped);
    lrat_chain.clear ();
    CADICAL_assert (likely_to_be_kept_clause (c));
    shrunken.push_back (c);
    return -1;
  }

  return 0;
}


struct subsume_less_noccs {
  Internal *internal;
  subsume_less_noccs (Internal *i) : internal (i) {}
  bool operator() (int a, int b) {
    const signed char u = internal->val (a), v = internal->val (b);
    if (!u && v)
      return true;
    if (u && !v)
      return false;
    const int64_t m = internal->noccs (a), n = internal->noccs (b);
    if (m < n)
      return true;
    if (m > n)
      return false;
    return abs (a) < abs (b);
  }
};

/*------------------------------------------------------------------------*/

// Usually called from 'subsume' below if 'subsuming' triggered it.  Then
// the idea is to subsume both redundant and irredundant clauses. It is also
// called in the elimination loop in 'elim' in which case we focus on
// irredundant clauses only to help bounded variable elimination.  The
// function returns true of an irredundant clause was removed or
// strengthened, which might then in the second usage scenario trigger new
// variable eliminations.

bool Internal::subsume_round () {

  if (!opts.subsume)
    return false;
  if (unsat)
    return false;
  if (terminated_asynchronously ())
    return false;
  if (!stats.current.redundant && !stats.current.irredundant)
    return false;

  START_SIMPLIFIER (subsume, SUBSUME);
  stats.subsumerounds++;

  int64_t check_limit;
  if (opts.subsumelimited) {
    int64_t delta = stats.propagations.search;
    delta *= 1e-3 * opts.subsumeeffort;
    if (delta < opts.subsumemineff)
      delta = opts.subsumemineff;
    if (delta > opts.subsumemaxeff)
      delta = opts.subsumemaxeff;
    delta = max (delta, (int64_t) 2l * active ());

    PHASE ("subsume-round", stats.subsumerounds,
           "limit of %" PRId64 " subsumption checks", delta);

    check_limit = stats.subchecks + delta;
  } else {
    PHASE ("subsume-round", stats.subsumerounds,
           "unlimited subsumption checks");
    check_limit = LONG_MAX;
  }

  int old_marked_candidate_variables_for_elimination = stats.mark.elim;

  CADICAL_assert (!level);

  // Allocate schedule and occurrence lists.
  //
  vector<ClauseSize> schedule;
  init_noccs ();

  // Determine candidate clauses and sort them by size.
  //
  int64_t left_over_from_last_subsumption_round = 0;

  for (auto c : clauses) {

    if (c->garbage)
      continue;
    if (c->size > opts.subsumeclslim)
      continue;
    if (!likely_to_be_kept_clause (c))
      continue;

    bool fixed = false;
    int subsume = 0;
    for (const auto &lit : *c)
      if (val (lit))
        fixed = true;
      else if (flags (lit).subsume)
        subsume++;

    // If the clause contains a root level assigned (fixed) literal we will
    // not work on it.  This simplifies the code substantially since we do
    // not have to care about assignments at all.  Strengthening becomes
    // much simpler too.
    //
    if (fixed) {
      LOG (c, "skipping (fixed literal)");
      continue;
    }

    // Further, if less than two variables in the clause were added since
    // the last subsumption round, the clause is ignored too.
    //
    if (subsume < 2) {
      LOG (c, "skipping (only %d added literals)", subsume);
      continue;
    }

    if (c->subsume)
      left_over_from_last_subsumption_round++;
    schedule.push_back (ClauseSize (c->size, c));
    for (const auto &lit : *c)
      noccs (lit)++;
  }
  shrink_vector (schedule);

  // Smaller clauses are checked and connected first.
  //
  rsort (schedule.begin (), schedule.end (), smaller_clause_size_rank ());

  if (!left_over_from_last_subsumption_round)
    for (auto cs : schedule)
      if (cs.clause->size > 2)
        cs.clause->subsume = true;

#ifndef CADICAL_QUIET
  int64_t scheduled = schedule.size ();
  int64_t total = stats.current.irredundant + stats.current.redundant;
  PHASE ("subsume-round", stats.subsumerounds,
         "scheduled %" PRId64 " clauses %.0f%% out of %" PRId64 " clauses",
         scheduled, percent (scheduled, total), total);
#endif

  // Now go over the scheduled clauses in the order of increasing size and
  // try to forward subsume and strengthen them. Forward subsumption tries
  // to find smaller or same size clauses which subsume or might strengthen
  // the candidate.  After the candidate has been processed connect one
  // of its literals (with smallest number of occurrences at this point) in
  // a one-watched scheme.

  int64_t subsumed = 0, strengthened = 0, checked = 0;

  vector<Clause *> shrunken;
  init_occs ();
  init_bins ();

  for (const auto &s : schedule) {

    if (terminated_asynchronously ())
      break;
    if (stats.subchecks >= check_limit)
      break;

    Clause *c = s.clause;
    CADICAL_assert (!c->garbage);

    checked++;

    // First try to subsume or strengthen this candidate clause.  For binary
    // clauses this could be done much faster by hashing and is costly due
    // to a usually large number of binary clauses.  There is further the
    // issue, that strengthening binary clauses (through double
    // self-subsuming resolution) would produce units, which needs much more
    // care. In the same (lazy) spirit we also ignore clauses with fixed
    // literals (false or true).
    //
    if (c->size > 2 && c->subsume) {
      c->subsume = false;
      const int tmp = try_to_subsume_clause (c, shrunken);
      if (tmp > 0) {
        subsumed++;
        continue;
      }
      if (tmp < 0)
        strengthened++;
    }

    // If not subsumed connect smallest occurring literal, where occurring
    // means the number of times it was used to connect (as a one-watch) a
    // previous smaller or equal sized clause.  This minimizes the length of
    // the occurrence lists traversed during 'try_to_subsume_clause'. Also
    // note that this number is usually way smaller than the number of
    // occurrences computed before and stored in 'noccs'.
    //
    int minlit = 0;
    int64_t minoccs = 0;
    size_t minsize = 0;
    bool subsume = true;
    bool binary = (c->size == 2 && !c->redundant);

    for (const auto &lit : *c) {

      if (!flags (lit).subsume)
        subsume = false;
      const size_t size = binary ? bins (lit).size () : occs (lit).size ();
      if (minlit && minsize <= size)
        continue;
      const int64_t tmp = noccs (lit);
      if (minlit && minsize == size && tmp <= minoccs)
        continue;
      minlit = lit, minsize = size, minoccs = tmp;
    }

    // If there is a variable in a clause different from is not 'subsume'
    // (has been added since the last subsumption round), then this clause
    // can not serve to strengthen or subsume another clause, since all
    // shrunken or added clauses mark all their variables as 'subsume'.
    //
    if (!subsume)
      continue;

    if (!binary) {

      // If smallest occurring literal occurs too often do not connect.
      //
      if (minsize > (size_t) opts.subsumeocclim)
        continue;

      LOG (c,
           "watching %d with %zd current and total %" PRId64 " occurrences",
           minlit, minsize, minoccs);

      occs (minlit).push_back (c);

      // This sorting should give faster failures for assumption checks
      // since the less occurring variables are put first in a clause and
      // thus will make it more likely to be found as witness for a clause
      // not to be subsuming.  One could in principle (see also the
      // discussion on 'subsumption' in our 'Splatz' solver) replace marking
      // by a kind of merge sort, as also suggested by Bayardo.  It would
      // avoid 'marked' calls and thus might be slightly faster but could
      // not take benefit of this sorting optimization.
      //
      sort (c->begin (), c->end (), subsume_less_noccs (this));

    } else {

      // If smallest occurring literal occurs too often do not connect.
      //
      if (minsize > (size_t) opts.subsumebinlim)
        continue;

      LOG (c,
           "watching %d with %zd current binary and total %" PRId64
           " occurrences",
           minlit, minsize, minoccs);

      const int minlit_pos = (c->literals[1] == minlit);
      const int other = c->literals[!minlit_pos];
      bins (minlit).push_back (Bin{other, c->id});
    }
  }

  PHASE ("subsume-round", stats.subsumerounds,
         "subsumed %" PRId64 " and strengthened %" PRId64 " out of %" PRId64
         " clauses %.0f%%",
         subsumed, strengthened, scheduled,
         percent (subsumed + strengthened, scheduled));

  const int64_t remain = schedule.size () - checked;
  const bool completed = !remain;

  if (completed)
    PHASE ("subsume-round", stats.subsumerounds,
           "checked all %" PRId64 " scheduled clauses", checked);
  else
    PHASE ("subsume-round", stats.subsumerounds,
           "checked %" PRId64 " clauses %.0f%% of scheduled (%" PRId64
           " remain)",
           checked, percent (checked, scheduled), remain);

  // Release occurrence lists and schedule.
  //
  erase_vector (schedule);
  reset_noccs ();
  reset_occs ();
  reset_bins ();

  // Reset all old 'added' flags and mark variables in shrunken
  // clauses as 'added' for the next subsumption round.
  //
  if (completed)
    reset_subsume_bits ();

  for (const auto &c : shrunken)
    mark_added (c);
  erase_vector (shrunken);

  report ('s', !opts.reportall && !(subsumed + strengthened));

  STOP_SIMPLIFIER (subsume, SUBSUME);

  return old_marked_candidate_variables_for_elimination < stats.mark.elim;
}

/*------------------------------------------------------------------------*/

void Internal::subsume () {

  if (!stats.current.redundant && !stats.current.irredundant)
    return;

  if (unsat)
    return;

  backtrack ();
  if (!propagate ()) {
    learn_empty_clause ();
    return;
  }

  stats.subsumephases++;

  if (external_prop) {
    CADICAL_assert (!level);
    private_steps = true;
  }

  if (opts.subsume) {
    reset_watches ();
    subsume_round ();
    init_watches ();
    connect_watches ();
    if (!unsat && !propagate ()) {
      LOG ("propagation after subsume rounds results in inconsistency");
      learn_empty_clause ();
    }
  }

  transred ();
  if (external_prop) {
    CADICAL_assert (!level);
    private_steps = false;
  }
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
