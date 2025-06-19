#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// In incremental solving after a first call to 'solve' has finished and
// before calling the internal 'solve' again incrementally we have to
// restore clauses which have the negation of a literal as a witness literal
// on the extension stack, which was added as original literal in a new
// clause or in an assumption.  This procedure has to be applied
// recursively, i.e., the literals of restored clauses are treated in the
// same way as literals of a new original clause.
//
// To figure out whether literals are such witnesses we have a 'witness'
// bit for each external literal, which is set in 'block', 'elim', and
// 'decompose' if a clause is pushed on the extension stack.  The witness
// bits are recomputed after restoring clauses.
//
// We further mark in the external solver newly internalized external
// literals in 'add' and 'assume' since the last call to 'solve' as tainted
// if they occur negated as a witness literal on the extension stack.  Then
// we go through the extension stack and restore all clauses which have a
// tainted literal (and its negation a marked as witness).
//
// Since the API contract disallows to call 'val' and 'failed' in an
// 'UNKNOWN' state. We do not have to internalize literals there.
//
// In order to have tainted literals accepted by the internal solver they
// have to be active and thus we might need to 'reactivate' them before
// restoring clauses if they are inactive. In case they have completely
// been eliminated and removed from the internal solver in 'compact', then
// we just use a new internal variable.  This is performed in 'internalize'
// during marking external literals as tainted.
//
// To check that this approach is correct the external solver can maintain a
// stack of original clauses and current assumptions both in terms of
// external literals.  Whenever 'solve' determines that the current
// incremental call is satisfiable we check that the (extended) witness does
// satisfy the saved original clauses, as well as all the assumptions. To
// enable these checks set 'opts.check' as well as 'opts.checkwitness' and
// 'opts.checkassumptions' all to 'true'.  The model based tester actually
// prefers to enable the 'opts.check' option and the other two are 'true' by
// default anyhow.
//
// See our SAT'19 paper [FazekasBiereScholl-SAT'19] for more details.

/*------------------------------------------------------------------------*/

void External::restore_clause (const vector<int>::const_iterator &begin,
                               const vector<int>::const_iterator &end,
                               const int64_t id) {
  LOG (begin, end, "restoring external clause[%" PRId64 "]", id);
  CADICAL_assert (eclause.empty ());
  CADICAL_assert (id);
  for (auto p = begin; p != end; p++) {
    eclause.push_back (*p);
    if (internal->proof && internal->lrat) {
      const auto &elit = *p;
      unsigned eidx = (elit > 0) + 2u * (unsigned) abs (elit);
      CADICAL_assert ((size_t) eidx < ext_units.size ());
      const int64_t id = ext_units[eidx];
      bool added = ext_flags[abs (elit)];
      if (id && !added) {
        ext_flags[abs (elit)] = true;
        internal->lrat_chain.push_back (id);
      }
    }
    int ilit = internalize (*p);
    internal->add_original_lit (ilit), internal->stats.restoredlits++;
  }
  if (internal->proof && internal->lrat) {
    for (const auto &elit : eclause) {
      ext_flags[abs (elit)] = false;
    }
  }
  internal->finish_added_clause_with_id (id, true);
  eclause.clear ();
  internal->stats.restored++;
}

/*------------------------------------------------------------------------*/

void External::restore_clauses () {

  CADICAL_assert (internal->opts.restoreall == 2 || !tainted.empty ());

  START (restore);
  internal->stats.restorations++;

  struct {
    int64_t weakened, satisfied, restored, removed;
  } clauses;
  memset (&clauses, 0, sizeof clauses);

  if (internal->opts.restoreall && tainted.empty ())
    PHASE ("restore", internal->stats.restorations,
           "forced to restore all clauses");

#ifndef CADICAL_QUIET
  {
    unsigned numtainted = 0;
    for (const auto b : tainted)
      if (b)
        numtainted++;

    PHASE ("restore", internal->stats.restorations,
           "starting with %u tainted literals %.0f%%", numtainted,
           percent (numtainted, 2u * max_var));
  }
#endif

  auto end_of_extension = extension.end ();
  auto p = extension.begin (), q = p;

  // Go over all witness labelled clauses on the extension stack, restore
  // those necessary, remove restored and flush satisfied clauses.
  //
  while (p != end_of_extension) {

    clauses.weakened++;

    CADICAL_assert (!*p);
    const auto saved = q; // Save old start.
    *q++ = *p++;          // Copy zero '0'.

    // Copy witness part and try to find a tainted witness literal in it.
    //
    int tlit = 0; // Negation tainted.
    int elit;
    //
    CADICAL_assert (p != end_of_extension);
    //
    while ((elit = *q++ = *p++)) {

      if (marked (tainted, -elit)) {
        tlit = elit;
        LOG ("negation of witness literal %d tainted", tlit);
      }

      CADICAL_assert (p != end_of_extension);
    }

    // now copy the id of the clause
    const int64_t id = ((int64_t) (*p) << 32) + (int64_t) * (p + 1);
    LOG ("id is %" PRId64, id);
    *q++ = *p++;
    *q++ = *p++;
    CADICAL_assert (id);
    CADICAL_assert (!*p);
    *q++ = *p++;

    // Now find 'end_of_clause' (clause starts at 'p') and at the same time
    // figure out whether the clause is actually root level satisfied.
    //
    int satisfied = 0;
    auto end_of_clause = p;
    while (end_of_clause != end_of_extension && (elit = *end_of_clause)) {
      if (!satisfied && fixed (elit) > 0)
        satisfied = elit;
      end_of_clause++;
    }
    CADICAL_assert (id);

    // Do not apply our 'FLUSH' rule to remove satisfied (implied) clauses
    // if the corresponding option is set simply by resetting 'satisfied'.
    //
    if (satisfied && !internal->opts.restoreflush) {
      LOG (p, end_of_clause, "forced to not remove %d satisfied",
           satisfied);
      satisfied = 0;
    }

    if (satisfied || tlit || internal->opts.restoreall) {

      if (satisfied) {
        LOG (p, end_of_clause,
             "flushing implied clause satisfied by %d from extension stack",
             satisfied);
        clauses.satisfied++;
      } else {
        restore_clause (p, end_of_clause, id); // Might taint literals.
        clauses.restored++;
      }

      clauses.removed++;
      p = end_of_clause;
      q = saved;

    } else {

      LOG (p, end_of_clause, "keeping clause on extension stack");

      while (p != end_of_clause) // Copy clause too.
        *q++ = *p++;
    }
  }

  extension.resize (q - extension.begin ());
  shrink_vector (extension);

#ifndef CADICAL_QUIET
  if (clauses.satisfied)
    PHASE ("restore", internal->stats.restorations,
           "removed %" PRId64 " satisfied %.0f%% of %" PRId64
           " weakened clauses",
           clauses.satisfied, percent (clauses.satisfied, clauses.weakened),
           clauses.weakened);
  else
    PHASE ("restore", internal->stats.restorations,
           "no satisfied clause removed out of %" PRId64
           " weakened clauses",
           clauses.weakened);

  if (clauses.restored)
    PHASE ("restore", internal->stats.restorations,
           "restored %" PRId64 " clauses %.0f%% out of %" PRId64
           " weakened clauses",
           clauses.restored, percent (clauses.restored, clauses.weakened),
           clauses.weakened);
  else
    PHASE ("restore", internal->stats.restorations,
           "no clause restored out of %" PRId64 " weakened clauses",
           clauses.weakened);
  {
    unsigned numtainted = 0;
    for (const auto &b : tainted)
      if (b)
        numtainted++;

    PHASE ("restore", internal->stats.restorations,
           "finishing with %u tainted literals %.0f%%", numtainted,
           percent (numtainted, 2u * max_var));
  }

#endif
  LOG ("extension stack clean");
  tainted.clear ();

  // Finally recompute the witness bits.
  //
  witness.clear ();
  const auto begin_of_extension = extension.begin ();
  p = extension.end ();
  while (p != begin_of_extension) {
    while (*--p)
      CADICAL_assert (p != begin_of_extension);
    int elit;
    CADICAL_assert (p != begin_of_extension);
    --p;
    CADICAL_assert (p != begin_of_extension);
    CADICAL_assert (*p || *(p - 1));
    --p;
    CADICAL_assert (p != begin_of_extension);
    CADICAL_assert (!*p);
    --p;
    CADICAL_assert (p != begin_of_extension);
    while ((elit = *--p)) {
      mark (witness, elit);
      CADICAL_assert (p != begin_of_extension);
    }
  }

  STOP (restore);
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
