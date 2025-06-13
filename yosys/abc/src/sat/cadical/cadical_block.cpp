#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// This implements an inprocessing version of blocked clause elimination and
// is assumed to be triggered just before bounded variable elimination.  It
// has a separate 'block' flag while variable elimination uses 'elim'.
// Thus it only tries to block clauses on a literal which was removed in an
// irredundant clause in negated form before and has not been tried to use
// as blocking literal since then.

/*------------------------------------------------------------------------*/

inline bool block_more_occs_size::operator() (unsigned a, unsigned b) {
  size_t s = internal->noccs (-internal->u2i (a));
  size_t t = internal->noccs (-internal->u2i (b));
  if (s > t)
    return true;
  if (s < t)
    return false;
  s = internal->noccs (internal->u2i (a));
  t = internal->noccs (internal->u2i (b));
  if (s > t)
    return true;
  if (s < t)
    return false;
  return a > b;
}

/*------------------------------------------------------------------------*/

// Determine whether 'c' is blocked on 'lit', by first marking all its
// literals and then checking all resolvents with negative clauses (with
// '-lit') are tautological.  We use a move-to-front scheme for both the
// occurrence list of negative clauses (with '-lit') and then for literals
// within each such clause.  The clause move-to-front scheme has the goal to
// find non-tautological clauses faster in the future, while the literal
// move-to-front scheme has the goal to faster find the matching literal,
// which makes the resolvent tautological (again in the future).

bool Internal::is_blocked_clause (Clause *c, int lit) {

  LOG (c, "trying to block on %d", lit);

  CADICAL_assert (c->size >= opts.blockminclslim);
  CADICAL_assert (c->size <= opts.blockmaxclslim);
  CADICAL_assert (active (lit));
  CADICAL_assert (!val (lit));
  CADICAL_assert (!c->garbage);
  CADICAL_assert (!c->redundant);
  CADICAL_assert (!level);

  mark (c); // First mark all literals in 'c'.

  Occs &os = occs (-lit);
  LOG ("resolving against at most %zd clauses with %d", os.size (), -lit);

  bool res = true; // Result is true if all resolvents tautological.

  // Can not use 'auto' here since we update 'os' during traversal.
  //
  const auto end_of_os = os.end ();
  auto i = os.begin ();

  Clause *prev_d = 0; // Previous non-tautological clause.

  for (; i != end_of_os; i++) {
    // Move the first clause with non-tautological resolvent to the front of
    // the occurrence list to improve finding it faster later.
    //
    Clause *d = *i;

    CADICAL_assert (!d->garbage);
    CADICAL_assert (!d->redundant);
    CADICAL_assert (d->size <= opts.blockmaxclslim);

    *i = prev_d; // Move previous non-tautological clause
    prev_d = d;  // backwards but remember clause at this position.

    LOG (d, "resolving on %d against", lit);
    stats.blockres++;

    int prev_other = 0; // Previous non-tautological literal.

    // No 'auto' since we update literals of 'd' during traversal.
    //
    const const_literal_iterator end_of_d = d->end ();
    literal_iterator l;

    for (l = d->begin (); l != end_of_d; l++) {
      // Same move-to-front mechanism for literals within a clause.  It
      // moves the first negatively marked literal to the front to find it
      // faster in the future.
      //
      const int other = *l;
      *l = prev_other;
      prev_other = other;
      if (other == -lit)
        continue;
      CADICAL_assert (other != lit);
      CADICAL_assert (active (other));
      CADICAL_assert (!val (other));
      if (marked (other) < 0) {
        LOG ("found tautological literal %d", other);
        d->literals[0] = other; // Move to front of 'd'.
        break;
      }
    }

    if (l == end_of_d) {
      LOG ("no tautological literal found");
      //
      // Since we did not find a tautological literal we restore the old
      // order of literals in the clause.
      //
      const const_literal_iterator begin_of_d = d->begin ();
      while (l-- != begin_of_d) {
        const int other = *l;
        *l = prev_other;
        prev_other = other;
      }
      res = false; // Now 'd' is a witness that 'c' is not blocked.
      os[0] = d;   // Move it to the front of the occurrence list.
      break;
    }
  }
  unmark (c); // ... all literals of the candidate clause.

  // If all resolvents are tautological and thus the clause is blocked we
  // restore the old order of clauses in the occurrence list of '-lit'.
  //
  if (res) {
    CADICAL_assert (i == end_of_os);
    const auto boc = os.begin ();
    while (i != boc) {
      Clause *d = *--i;
      *i = prev_d;
      prev_d = d;
    }
  }

  return res;
}

/*------------------------------------------------------------------------*/

void Internal::block_schedule (Blocker &blocker) {
  // Set skip flags for all literals in too large clauses.
  //
  for (const auto &c : clauses) {

    if (c->garbage)
      continue;
    if (c->redundant)
      continue;
    if (c->size <= opts.blockmaxclslim)
      continue;

    for (const auto &lit : *c)
      mark_skip (-lit);
  }

  // Connect all literal occurrences in irredundant clauses.
  //
  for (const auto &c : clauses) {

    if (c->garbage)
      continue;
    if (c->redundant)
      continue;

    for (const auto &lit : *c) {
      CADICAL_assert (active (lit));
      CADICAL_assert (!val (lit));
      occs (lit).push_back (c);
    }
  }

  // We establish the invariant that 'noccs' gives the number of actual
  // occurrences of 'lit' in non-garbage clauses,  while 'occs' might still
  // refer to garbage clauses, thus 'noccs (lit) <= occs (lit).size ()'.  It
  // is expensive to remove references to garbage clauses from 'occs' during
  // blocked clause elimination, but decrementing 'noccs' is cheap.

  for (auto lit : lits) {
    if (!active (lit))
      continue;
    CADICAL_assert (!val (lit));
    Occs &os = occs (lit);
    noccs (lit) = os.size ();
  }

  // Now we fill the schedule (priority queue) of candidate literals to be
  // tried as blocking literals.  It is probably slightly faster to do this
  // in one go after all occurrences have been determined, instead of
  // filling the priority queue during pushing occurrences.  Filling the
  // schedule can not be fused with the previous loop (easily) since we
  // first have to initialize 'noccs' for both 'lit' and '-lit'.

#ifndef CADICAL_QUIET
  int skipped = 0;
#endif

  for (auto idx : vars) {
    if (!active (idx))
      continue;
    if (frozen (idx)) {
#ifndef CADICAL_QUIET
      skipped += 2;
#endif
      continue;
    }
    CADICAL_assert (!val (idx));
    for (int sign = -1; sign <= 1; sign += 2) {
      const int lit = sign * idx;
      if (marked_skip (lit)) {
#ifndef CADICAL_QUIET
        skipped++;
#endif
        continue;
      }
      if (!marked_block (lit))
        continue;
      unmark_block (lit);
      LOG ("scheduling %d with %" PRId64 " positive and %" PRId64
           " negative occurrences",
           lit, noccs (lit), noccs (-lit));
      blocker.schedule.push_back (vlit (lit));
    }
  }

  PHASE ("block", stats.blockings,
         "scheduled %zd candidate literals %.2f%% (%d skipped %.2f%%)",
         blocker.schedule.size (),
         percent (blocker.schedule.size (), 2.0 * active ()), skipped,
         percent (skipped, 2.0 * active ()));
}

/*------------------------------------------------------------------------*/

// A literal is pure if it only occurs positive.  Then all clauses in which
// it occurs are blocked on it. This special case can be implemented faster
// than trying to block literals with at least one negative occurrence and
// is thus handled separately.  It also allows to avoid pushing blocked
// clauses onto the extension stack.

void Internal::block_pure_literal (Blocker &blocker, int lit) {
  if (frozen (lit))
    return;
  CADICAL_assert (active (lit));

  Occs &pos = occs (lit);
  Occs &nos = occs (-lit);

  CADICAL_assert (!noccs (-lit));
#ifndef CADICAL_NDEBUG
  for (const auto &c : nos)
    CADICAL_assert (c->garbage);
#endif
  stats.blockpurelits++;
  LOG ("found pure literal %d", lit);
#ifdef LOGGING
  int64_t pured = 0;
#endif
  for (const auto &c : pos) {
    if (c->garbage)
      continue;
    CADICAL_assert (!c->redundant);
    LOG (c, "pure literal %d in", lit);
    blocker.reschedule.push_back (c);
    if (proof) {
      proof->weaken_minus (c);
    }
    external->push_clause_on_extension_stack (c, lit);
    stats.blockpured++;
    mark_garbage (c);
#ifdef LOGGING
    pured++;
#endif
  }

  erase_vector (pos);
  erase_vector (nos);

  mark_pure (lit);
  stats.blockpured++;
  LOG ("blocking %" PRId64 " clauses on pure literal %d", pured, lit);
}

/*------------------------------------------------------------------------*/

// If there is only one negative clause with '-lit' it is faster to mark it
// instead of marking all the positive clauses with 'lit' one after the
// other and then resolving against the negative clause.

void Internal::block_literal_with_one_negative_occ (Blocker &blocker,
                                                    int lit) {
  CADICAL_assert (active (lit));
  CADICAL_assert (!frozen (lit));
  CADICAL_assert (noccs (lit) > 0);
  CADICAL_assert (noccs (-lit) == 1);

  Occs &nos = occs (-lit);
  CADICAL_assert (nos.size () >= 1);

  Clause *d = 0;
  for (const auto &c : nos) {
    if (c->garbage)
      continue;
    CADICAL_assert (!d);
    d = c;
#ifndef CADICAL_NDEBUG
    break;
#endif
  }
  CADICAL_assert (d);
  nos.resize (1);
  nos[0] = d;

  if (d && d->size > opts.blockmaxclslim) {
    LOG (d, "skipped common antecedent");
    return;
  }

  CADICAL_assert (!d->garbage);
  CADICAL_assert (!d->redundant);
  CADICAL_assert (d->size <= opts.blockmaxclslim);

  LOG (d, "common %d antecedent", lit);
  mark (d);
  int64_t blocked = 0;
#ifdef LOGGING
  int64_t skipped = 0;
#endif
  Occs &pos = occs (lit);

  // Again no 'auto' since 'pos' is update during traversal.
  //
  const auto eop = pos.end ();
  auto j = pos.begin (), i = j;

  for (; i != eop; i++) {
    Clause *c = *j++ = *i;

    if (c->garbage) {
      j--;
      continue;
    }
    if (c->size > opts.blockmaxclslim) {
#ifdef LOGGING
      skipped++;
#endif
      continue;
    }
    if (c->size < opts.blockminclslim) {
#ifdef LOGGING
      skipped++;
#endif
      continue;
    }

    LOG (c, "trying to block on %d", lit);

    // We use the same literal move-to-front strategy as in
    // 'is_blocked_clause'.  See there for more explanations.

    int prev_other = 0; // Previous non-tautological literal.

    // No 'auto' since literals of 'c' are updated during traversal.
    //
    const const_literal_iterator end_of_c = c->end ();
    literal_iterator l;

    for (l = c->begin (); l != end_of_c; l++) {
      const int other = *l;
      *l = prev_other;
      prev_other = other;
      if (other == lit)
        continue;
      CADICAL_assert (other != -lit);
      CADICAL_assert (active (other));
      CADICAL_assert (!val (other));
      if (marked (other) < 0) {
        LOG ("found tautological literal %d", other);
        c->literals[0] = other; // Move to front of 'c'.
        break;
      }
    }

    if (l == end_of_c) {
      LOG ("no tautological literal found");

      // Restore old literal order in the clause because.

      const const_literal_iterator begin_of_c = c->begin ();
      while (l-- != begin_of_c) {
        const int other = *l;
        *l = prev_other;
        prev_other = other;
      }

      continue; // ... with next candidate 'c' in 'pos'.
    }

    blocked++;
    LOG (c, "blocked");
    if (proof) {
      proof->weaken_minus (c);
    }
    external->push_clause_on_extension_stack (c, lit);
    blocker.reschedule.push_back (c);
    mark_garbage (c);
    j--;
  }
  if (j == pos.begin ())
    erase_vector (pos);
  else
    pos.resize (j - pos.begin ());

  stats.blocked += blocked;
  LOG ("blocked %" PRId64 " clauses on %d (skipped %" PRId64 ")", blocked,
       lit, skipped);

  unmark (d);
}

/*------------------------------------------------------------------------*/

// Determine the set of candidate clauses with 'lit', which are checked to
// be blocked by 'lit'.  Filter out too large and small clauses and which do
// not have any negated other literal in any of the clauses with '-lit'.

size_t Internal::block_candidates (Blocker &blocker, int lit) {

  CADICAL_assert (blocker.candidates.empty ());

  Occs &pos = occs (lit); // Positive occurrences of 'lit'.
  Occs &nos = occs (-lit);

  CADICAL_assert ((size_t) noccs (lit) <= pos.size ());
  CADICAL_assert ((size_t) noccs (-lit) == nos.size ()); // Already flushed.

  // Mark all literals in clauses with '-lit'.  Note that 'mark2' uses
  // separate bits for 'lit' and '-lit'.
  //
  for (const auto &c : nos)
    mark2 (c);

  const auto eop = pos.end ();
  auto j = pos.begin (), i = j;

  for (; i != eop; i++) {
    Clause *c = *j++ = *i;
    if (c->garbage) {
      j--;
      continue;
    }
    CADICAL_assert (!c->redundant);
    if (c->size > opts.blockmaxclslim)
      continue;
    if (c->size < opts.blockminclslim)
      continue;
    const const_literal_iterator eoc = c->end ();
    const_literal_iterator l;
    for (l = c->begin (); l != eoc; l++) {
      const int other = *l;
      if (other == lit)
        continue;
      CADICAL_assert (other != -lit);
      CADICAL_assert (active (other));
      CADICAL_assert (!val (other));
      if (marked2 (-other))
        break;
    }
    if (l != eoc)
      blocker.candidates.push_back (c);
  }
  if (j == pos.begin ())
    erase_vector (pos);
  else
    pos.resize (j - pos.begin ());

  CADICAL_assert (pos.size () == (size_t) noccs (lit)); // Now also flushed.

  for (const auto &c : nos)
    unmark (c);

  return blocker.candidates.size ();
}

/*------------------------------------------------------------------------*/

// Try to find a clause with '-lit' which does not have any literal in
// clauses with 'lit'.  If such a clause exists no candidate clause can be
// blocked on 'lit' since all candidates would produce a non-tautological
// resolvent with that clause.

Clause *Internal::block_impossible (Blocker &blocker, int lit) {
  CADICAL_assert (noccs (-lit) > 1);
  CADICAL_assert (blocker.candidates.size () > 1);

  for (const auto &c : blocker.candidates)
    mark2 (c);

  Occs &nos = occs (-lit);
  Clause *res = 0;

  for (const auto &c : nos) {
    CADICAL_assert (!c->garbage);
    CADICAL_assert (!c->redundant);
    CADICAL_assert (c->size <= opts.blockmaxclslim);
    const const_literal_iterator eoc = c->end ();
    const_literal_iterator l;
    for (l = c->begin (); l != eoc; l++) {
      const int other = *l;
      if (other == -lit)
        continue;
      CADICAL_assert (other != lit);
      CADICAL_assert (active (other));
      CADICAL_assert (!val (other));
      if (marked2 (-other))
        break;
    }
    if (l == eoc)
      res = c;
  }

  for (const auto &c : blocker.candidates)
    unmark (c);

  if (res) {
    LOG (res, "common non-tautological resolvent producing");
    blocker.candidates.clear ();
  }

  return res;
}

/*------------------------------------------------------------------------*/

// In the general case we have at least two negative occurrences.

void Internal::block_literal_with_at_least_two_negative_occs (
    Blocker &blocker, int lit) {
  CADICAL_assert (active (lit));
  CADICAL_assert (!frozen (lit));
  CADICAL_assert (noccs (lit) > 0);
  CADICAL_assert (noccs (-lit) > 1);

  Occs &nos = occs (-lit);
  CADICAL_assert ((size_t) noccs (-lit) <= nos.size ());

  int max_size = 0;

  // Flush all garbage clauses in occurrence list 'nos' of '-lit' and
  // determine the maximum size of negative clauses (with '-lit').
  //
  const auto eon = nos.end ();
  auto j = nos.begin (), i = j;
  for (; i != eon; i++) {
    Clause *c = *j++ = *i;
    if (c->garbage)
      j--;
    else if (c->size > max_size)
      max_size = c->size;
  }
  if (j == nos.begin ())
    erase_vector (nos);
  else
    nos.resize (j - nos.begin ());

  CADICAL_assert (nos.size () == (size_t) noccs (-lit));
  CADICAL_assert (nos.size () > 1);

  // If the maximum size of a negative clause (with '-lit') exceeds the
  // maximum clause size limit ignore this candidate literal.
  //
  if (max_size > opts.blockmaxclslim) {
    LOG ("maximum size %d of clauses with %d exceeds clause size limit %d",
         max_size, -lit, opts.blockmaxclslim);
    return;
  }

  LOG ("maximum size %d of clauses with %d", max_size, -lit);

  // We filter candidate clauses with positive occurrence of 'lit' in
  // 'blocker.candidates' and return if no candidate clause remains.
  // Candidates should be small enough and should have at least one literal
  // which occurs negated in one of the clauses with '-lit'.
  //
  size_t candidates = block_candidates (blocker, lit);
  if (!candidates) {
    LOG ("no candidate clauses found");
    return;
  }

  LOG ("found %zd candidate clauses", candidates);

  // We further search for a clause with '-lit' that has no literal
  // negated in any of the candidate clauses (except 'lit').  If such a
  // clause exists, we know that none of the candidates is blocked.
  //
  if (candidates > 1 && block_impossible (blocker, lit)) {
    LOG ("impossible to block any candidate clause on %d", lit);
    CADICAL_assert (blocker.candidates.empty ());
    return;
  }

  LOG ("trying to block %zd clauses out of %" PRId64 " with literal %d",
       candidates, noccs (lit), lit);

  int64_t blocked = 0;

  // Go over all remaining candidates and try to block them on 'lit'.
  //
  for (const auto &c : blocker.candidates) {
    CADICAL_assert (!c->garbage);
    CADICAL_assert (!c->redundant);
    if (!is_blocked_clause (c, lit))
      continue;
    blocked++;
    LOG (c, "blocked");
    if (proof) {
      proof->weaken_minus (c);
    }
    external->push_clause_on_extension_stack (c, lit);
    blocker.reschedule.push_back (c);
    mark_garbage (c);
  }

  LOG ("blocked %" PRId64
       " clauses on %d out of %zd candidates in %zd occurrences",
       blocked, lit, blocker.candidates.size (), occs (lit).size ());

  blocker.candidates.clear ();
  stats.blocked += blocked;
  if (blocked)
    flush_occs (lit);
}

/*------------------------------------------------------------------------*/

// Reschedule literals in a clause (except 'lit') which was blocked.

void Internal::block_reschedule_clause (Blocker &blocker, int lit,
                                        Clause *c) {
#ifdef CADICAL_NDEBUG
  (void) lit;
#endif
  CADICAL_assert (c->garbage);

  for (const auto &other : *c) {

    int64_t &n = noccs (other);
    CADICAL_assert (n > 0);
    n--;

    LOG ("updating %d with %" PRId64 " positive and %" PRId64
         " negative occurrences",
         other, noccs (other), noccs (-other));

    if (blocker.schedule.contains (vlit (-other)))
      blocker.schedule.update (vlit (-other));
    else if (active (other) && !frozen (other) && !marked_skip (-other)) {
      LOG ("rescheduling to block clauses on %d", -other);
      blocker.schedule.push_back (vlit (-other));
    }

    if (blocker.schedule.contains (vlit (other))) {
      CADICAL_assert (other != lit);
      blocker.schedule.update (vlit (other));
    }
  }
}

// Reschedule all literals in clauses blocked by 'lit' (except 'lit').

void Internal::block_reschedule (Blocker &blocker, int lit) {
  while (!blocker.reschedule.empty ()) {
    Clause *c = blocker.reschedule.back ();
    blocker.reschedule.pop_back ();
    block_reschedule_clause (blocker, lit, c);
  }
}

/*------------------------------------------------------------------------*/

void Internal::block_literal (Blocker &blocker, int lit) {
  CADICAL_assert (!marked_skip (lit));

  if (!active (lit))
    return; // Pure literal '-lit'.
  if (frozen (lit))
    return;

  CADICAL_assert (!val (lit));

  // If the maximum number of a negative clauses (with '-lit') exceeds the
  // occurrence limit ignore this candidate literal.
  //
  if (noccs (-lit) > opts.blockocclim)
    return;

  LOG ("blocking literal candidate %d "
       "with %" PRId64 " positive and %" PRId64 " negative occurrences",
       lit, noccs (lit), noccs (-lit));

  stats.blockcands++;

  CADICAL_assert (blocker.reschedule.empty ());
  CADICAL_assert (blocker.candidates.empty ());

  if (!noccs (-lit))
    block_pure_literal (blocker, lit);
  else if (!noccs (lit)) {
    // Rare situation, where the clause length limit was hit for 'lit' and
    // '-lit' is skipped and then it becomes pure.  Can be ignored.  We also
    // so it once happening for a 'elimboundmin=-1' and zero positive and
    // one negative occurrence.
  } else if (noccs (-lit) == 1)
    block_literal_with_one_negative_occ (blocker, lit);
  else
    block_literal_with_at_least_two_negative_occs (blocker, lit);

  // Done with blocked clause elimination on this literal and we do not
  // have to try blocked clause elimination on it again until irredundant
  // clauses with its negation are removed.
  //
  CADICAL_assert (!frozen (lit)); // just to be sure ...
  unmark_block (lit);
}

/*------------------------------------------------------------------------*/

bool Internal::block () {

  if (!opts.block)
    return false;
  if (unsat)
    return false;
  if (!stats.current.irredundant)
    return false;
  if (terminated_asynchronously ())
    return false;

  if (propagated < trail.size ()) {
    LOG ("need to propagate %zd units first", trail.size () - propagated);
    init_watches ();
    connect_watches ();
    if (!propagate ()) {
      LOG ("propagating units results in empty clause");
      learn_empty_clause ();
      CADICAL_assert (unsat);
    }
    clear_watches ();
    reset_watches ();
    if (unsat)
      return false;
  }

  START_SIMPLIFIER (block, BLOCK);

  stats.blockings++;

  LOG ("block-%" PRId64 "", stats.blockings);

  CADICAL_assert (!level);
  CADICAL_assert (!watching ());
  CADICAL_assert (!occurring ());

  mark_satisfied_clauses_as_garbage ();

  init_occs ();  // Occurrence lists for all literals.
  init_noccs (); // Number of occurrences to avoid flushing garbage clauses.

  Blocker blocker (this);
  block_schedule (blocker);

  int64_t blocked = stats.blocked;
  int64_t resolutions = stats.blockres;
  int64_t purelits = stats.blockpurelits;
  int64_t pured = stats.blockpured;

  while (!terminated_asynchronously () && !blocker.schedule.empty ()) {
    int lit = u2i (blocker.schedule.front ());
    blocker.schedule.pop_front ();
    block_literal (blocker, lit);
    block_reschedule (blocker, lit);
  }

  blocker.erase ();
  reset_noccs ();
  reset_occs ();

  resolutions = stats.blockres - resolutions;
  blocked = stats.blocked - blocked;

  PHASE ("block", stats.blockings,
         "blocked %" PRId64 " clauses in %" PRId64 " resolutions", blocked,
         resolutions);

  pured = stats.blockpured - pured;
  purelits = stats.blockpurelits - purelits;

  if (pured)
    mark_redundant_clauses_with_eliminated_variables_as_garbage ();

  if (purelits)
    PHASE ("block", stats.blockings,
           "found %" PRId64 " pure literals in %" PRId64 " clauses",
           purelits, pured);
  else
    PHASE ("block", stats.blockings, "no pure literals found");

  report ('b', !opts.reportall && !blocked);

  STOP_SIMPLIFIER (block, BLOCK);

  return blocked;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
