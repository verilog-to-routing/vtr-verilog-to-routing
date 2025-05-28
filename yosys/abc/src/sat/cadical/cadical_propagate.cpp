#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// We are using the address of 'decision_reason' as pseudo reason for
// decisions to distinguish assignment decisions from other assignments.
// Before we added chronological backtracking all learned units were
// assigned at decision level zero ('Solver.level == 0') and we just used a
// zero pointer as reason.  After allowing chronological backtracking units
// were also assigned at higher decision level (but with assignment level
// zero), and it was not possible anymore to just distinguish the case
// 'unit' versus 'decision' by just looking at the current level.  Both had
// a zero pointer as reason.  Now only units have a zero reason and
// decisions need to use the pseudo reason 'decision_reason'.

// External propagation steps use the pseudo reason 'external_reason'.
// The corresponding actual reason clauses are learned only when they are
// relevant in conflict analysis or in root-level fixing steps.

static Clause decision_reason_clause;
static Clause *decision_reason = &decision_reason_clause;

// If chronological backtracking is used the actual assignment level might
// be lower than the current decision level. In this case the assignment
// level is defined as the maximum level of the literals in the reason
// clause except the literal for which the clause is a reason.  This
// function determines this assignment level. For non-chronological
// backtracking as in classical CDCL this function always returns the
// current decision level, the concept of assignment level does not make
// sense, and accordingly this function can be skipped.

// In case of external propagation, it is implicitly assumed that the
// assignment level is the level of the literal (since the reason clause,
// i.e., the set of other literals, is unknown).

inline int Internal::assignment_level (int lit, Clause *reason) {

  CADICAL_assert (opts.chrono || external_prop);
  if (!reason || reason == external_reason)
    return level;

  int res = 0;

  for (const auto &other : *reason) {
    if (other == lit)
      continue;
    CADICAL_assert (val (other));
    int tmp = var (other).level;
    if (tmp > res)
      res = tmp;
  }

  return res;
}

// calculate lrat_chain
//
void Internal::build_chain_for_units (int lit, Clause *reason,
                                      bool forced) {
  if (!lrat)
    return;
  if (opts.chrono && assignment_level (lit, reason) && !forced)
    return;
  else if (!opts.chrono && level && !forced)
    return; // not decision level 0
  CADICAL_assert (lrat_chain.empty ());
  for (auto &reason_lit : *reason) {
    if (lit == reason_lit)
      continue;
    CADICAL_assert (val (reason_lit));
    if (!val (reason_lit))
      continue;
    const int signed_reason_lit = val (reason_lit) * reason_lit;
    int64_t id = unit_id (signed_reason_lit);
    lrat_chain.push_back (id);
  }
  lrat_chain.push_back (reason->id);
}

// same code as above but reason is assumed to be conflict and lit is not
// needed
//
void Internal::build_chain_for_empty () {
  if (!lrat || !lrat_chain.empty ())
    return;
  CADICAL_assert (!level);
  CADICAL_assert (lrat_chain.empty ());
  CADICAL_assert (conflict);
  LOG (conflict, "lrat for global empty clause with conflict");
  for (auto &lit : *conflict) {
    CADICAL_assert (val (lit) < 0);
    int64_t id = unit_id (-lit);
    lrat_chain.push_back (id);
  }
  lrat_chain.push_back (conflict->id);
}

/*------------------------------------------------------------------------*/

inline void Internal::search_assign (int lit, Clause *reason) {

  if (level)
    require_mode (SEARCH);

  const int idx = vidx (lit);
  const bool from_external = reason == external_reason;
  CADICAL_assert (!val (idx));
  CADICAL_assert (!flags (idx).eliminated () || reason == decision_reason ||
          reason == external_reason);
  Var &v = var (idx);
  int lit_level;
  CADICAL_assert (!lrat || level || reason == external_reason ||
          reason == decision_reason || !lrat_chain.empty ());
  // The following cases are explained in the two comments above before
  // 'decision_reason' and 'assignment_level'.
  //
  // External decision reason means that the propagation was done by
  // an external propagation and the reason clause not known (yet).
  // In that case it is assumed that the propagation is NOT out of
  // order (i.e. lit_level = level), because due to lazy explanation,
  // we can not calculate the real assignment level.
  // The function assignment_level () will also assign the current level
  // to literals with external reason.
  if (!reason)
    lit_level = 0; // unit
  else if (reason == decision_reason)
    lit_level = level, reason = 0;
  else if (opts.chrono)
    lit_level = assignment_level (lit, reason);
  else
    lit_level = level;
  if (!lit_level)
    reason = 0;

  v.level = lit_level;
  v.trail = trail.size ();
  v.reason = reason;
  CADICAL_assert ((int) num_assigned < max_var);
  CADICAL_assert (num_assigned == trail.size ());
  num_assigned++;
  if (!lit_level && !from_external)
    learn_unit_clause (lit); // increases 'stats.fixed'
  CADICAL_assert (lit_level || !from_external);
  const signed char tmp = sign (lit);
  set_val (idx, tmp);
  CADICAL_assert (val (lit) > 0);  // Just a bit paranoid but useful.
  CADICAL_assert (val (-lit) < 0); // Ditto.
  if (!searching_lucky_phases)
    phases.saved[idx] = tmp; // phase saving during search
  trail.push_back (lit);
#ifdef LOGGING
  if (!lit_level)
    LOG ("root-level unit assign %d @ 0", lit);
  else
    LOG (reason, "search assign %d @ %d", lit, lit_level);
#endif

  if (watching ()) {
    const Watches &ws = watches (-lit);
    if (!ws.empty ()) {
      const Watch &w = ws[0];
#ifndef WIN32
      __builtin_prefetch (&w, 0, 1);
#endif
    }
  }
  lrat_chain.clear ();
}

/*------------------------------------------------------------------------*/

// External versions of 'search_assign' which are not inlined.  They either
// are used to assign unit clauses on the root-level, in 'decide' to assign
// a decision or in 'analyze' to assign the literal 'driven' by a learned
// clause.  This happens far less frequently than the 'search_assign' above,
// which is called directly in 'propagate' below and thus is inlined.

void Internal::assign_unit (int lit) {
  CADICAL_assert (!level);
  search_assign (lit, 0);
}

// Just assume the given literal as decision (increase decision level and
// assign it).  This is used below in 'decide'.

void Internal::search_assume_decision (int lit) {
  require_mode (SEARCH);
  CADICAL_assert (propagated == trail.size ());
  new_trail_level (lit);
  notify_decision ();
  LOG ("search decide %d", lit);
  search_assign (lit, decision_reason);
}

void Internal::search_assign_driving (int lit, Clause *c) {
  require_mode (SEARCH);
  search_assign (lit, c);
  notify_assignments ();
}

void Internal::search_assign_external (int lit) {
  require_mode (SEARCH);
  search_assign (lit, external_reason);
  notify_assignments ();
}

/*------------------------------------------------------------------------*/

// The 'propagate' function is usually the hot-spot of a CDCL SAT solver.
// The 'trail' stack saves assigned variables and is used here as BFS queue
// for checking clauses with the negation of assigned variables for being in
// conflict or whether they produce additional assignments.

// This version of 'propagate' uses lazy watches and keeps two watched
// literals at the beginning of the clause.  We also use 'blocking literals'
// to reduce the number of times clauses have to be visited (2008 JSAT paper
// by Chu, Harwood and Stuckey).  The watches know if a watched clause is
// binary, in which case it never has to be visited.  If a binary clause is
// falsified we continue propagating.

// Finally, for long clauses we save the position of the last watch
// replacement in 'pos', which in turn reduces certain quadratic accumulated
// propagation costs (2013 JAIR article by Ian Gent) at the expense of four
// more bytes for each clause.

bool Internal::propagate () {

  if (level)
    require_mode (SEARCH);
  CADICAL_assert (!unsat);
  LOG ("starting propagate");
  START (propagate);

  // Updating statistics counter in the propagation loops is costly so we
  // delay until propagation ran to completion.
  //
  int64_t before = propagated;
  int64_t ticks = 0;

  while (!conflict && propagated != trail.size ()) {

    const int lit = -trail[propagated++];
    LOG ("propagating %d", -lit);
    Watches &ws = watches (lit);

    const const_watch_iterator eow = ws.end ();
    watch_iterator j = ws.begin ();
    const_watch_iterator i = j;
    ticks += 1 + cache_lines (ws.size (), sizeof *i);

    while (i != eow) {

      const Watch w = *j++ = *i++;
      const signed char b = val (w.blit);
      LOG (w.clause, "checking");

      if (b > 0)
        continue; // blocking literal satisfied

      if (w.binary ()) {

        // CADICAL_assert (w.clause->redundant || !w.clause->garbage);

        // In principle we can ignore garbage binary clauses too, but that
        // would require to dereference the clause pointer all the time with
        //
        // if (w.clause->garbage) { j--; continue; } // (*)
        //
        // This is too costly.  It is however necessary to produce correct
        // proof traces if binary clauses are traced to be deleted ('d ...'
        // line) immediately as soon they are marked as garbage.  Actually
        // finding instances where this happens is pretty difficult (six
        // parallel fuzzing jobs in parallel took an hour), but it does
        // occur.  Our strategy to avoid generating incorrect proofs now is
        // to delay tracing the deletion of binary clauses marked as garbage
        // until they are really deleted from memory.  For large clauses
        // this is not necessary since we have to access the clause anyhow.
        //
        // Thanks go to Mathias Fleury, who wanted me to explain why the
        // line '(*)' above was in the code. Removing it actually really
        // improved running times and thus I tried to find concrete
        // instances where this happens (which I found), and then
        // implemented the described fix.

        // Binary clauses are treated separately since they do not require
        // to access the clause at all (only during conflict analysis, and
        // there also only to simplify the code).

        if (b < 0)
          conflict = w.clause; // but continue ...
        else {
          build_chain_for_units (w.blit, w.clause, 0);
          search_assign (w.blit, w.clause);
          // lrat_chain.clear (); done in search_assign
          ticks++;
        }

      } else {
        CADICAL_assert (w.clause->size > 2);

        if (conflict)
          break; // Stop if there was a binary conflict already.

        // The cache line with the clause data is forced to be loaded here
        // and thus this first memory access below is the real hot-spot of
        // the solver.  Note, that this check is positive very rarely and
        // thus branch prediction should be almost perfect here.

        ticks++;

        if (w.clause->garbage) {
          j--;
          continue;
        }

        literal_iterator lits = w.clause->begin ();

        // Simplify code by forcing 'lit' to be the second literal in the
        // clause.  This goes back to MiniSAT.  We use a branch-less version
        // for conditionally swapping the first two literals, since it
        // turned out to be substantially faster than this one
        //
        //  if (lits[0] == lit) swap (lits[0], lits[1]);
        //
        // which achieves the same effect, but needs a branch.
        //
        const int other = lits[0] ^ lits[1] ^ lit;
        const signed char u = val (other); // value of the other watch

        if (u > 0)
          j[-1].blit = other; // satisfied, just replace blit
        else {

          // This follows Ian Gent's (JAIR'13) idea of saving the position
          // of the last watch replacement.  In essence it needs two copies
          // of the default search for a watch replacement (in essence the
          // code in the 'if (v < 0) { ... }' block below), one starting at
          // the saved position until the end of the clause and then if that
          // one failed to find a replacement another one starting at the
          // first non-watched literal until the saved position.

          const int size = w.clause->size;
          const literal_iterator middle = lits + w.clause->pos;
          const const_literal_iterator end = lits + size;
          literal_iterator k = middle;

          // Find replacement watch 'r' at position 'k' with value 'v'.

          int r = 0;
          signed char v = -1;

          while (k != end && (v = val (r = *k)) < 0)
            k++;

          if (v < 0) { // need second search starting at the head?

            k = lits + 2;
            CADICAL_assert (w.clause->pos <= size);
            while (k != middle && (v = val (r = *k)) < 0)
              k++;
          }

          w.clause->pos = k - lits; // always save position

          CADICAL_assert (lits + 2 <= k), CADICAL_assert (k <= w.clause->end ());

          if (v > 0) {

            // Replacement satisfied, so just replace 'blit'.

            j[-1].blit = r;

          } else if (!v) {

            // Found new unassigned replacement literal to be watched.

            LOG (w.clause, "unwatch %d in", lit);

            lits[0] = other;
            lits[1] = r;
            *k = lit;

            watch_literal (r, lit, w.clause);

            j--; // Drop this watch from the watch list of 'lit'.

            ticks++;

          } else if (!u) {

            CADICAL_assert (v < 0);

            // The other watch is unassigned ('!u') and all other literals
            // assigned to false (still 'v < 0'), thus we found a unit.
            //
            build_chain_for_units (other, w.clause, 0);
            search_assign (other, w.clause);
            // lrat_chain.clear (); done in search_assign
            ticks++;

            // Similar code is in the implementation of the SAT'18 paper on
            // chronological backtracking but in our experience, this code
            // first does not really seem to be necessary for correctness,
            // and further does not improve running time either.
            //
            if (opts.chrono > 1) {

              const int other_level = var (other).level;

              if (other_level > var (lit).level) {

                // The assignment level of the new unit 'other' is larger
                // than the assignment level of 'lit'.  Thus we should find
                // another literal in the clause at that higher assignment
                // level and watch that instead of 'lit'.

                CADICAL_assert (size > 2);

                int pos, s = 0;

                for (pos = 2; pos < size; pos++)
                  if (var (s = lits[pos]).level == other_level)
                    break;

                CADICAL_assert (s);
                CADICAL_assert (pos < size);

                LOG (w.clause, "unwatch %d in", lit);
                lits[pos] = lit;
                lits[0] = other;
                lits[1] = s;
                watch_literal (s, lit, w.clause);

                j--; // Drop this watch from the watch list of 'lit'.
              }
            }
          } else {

            CADICAL_assert (u < 0);
            CADICAL_assert (v < 0);

            // The other watch is assigned false ('u < 0') and all other
            // literals as well (still 'v < 0'), thus we found a conflict.

            conflict = w.clause;
            break;
          }
        }
      }
    }

    if (j != i) {

      while (i != eow)
        *j++ = *i++;

      ws.resize (j - ws.begin ());
    }
  }

  if (searching_lucky_phases) {

    if (conflict)
      LOG (conflict, "ignoring lucky conflict");

  } else {

    // Avoid updating stats eagerly in the hot-spot of the solver.
    //
    stats.propagations.search += propagated - before;
    stats.ticks.search[stable] += ticks;

    if (!conflict)
      no_conflict_until = propagated;
    else {

      if (stable)
        stats.stabconflicts++;
      stats.conflicts++;

      LOG (conflict, "conflict");

      // The trail before the current decision level was conflict free.
      //
      no_conflict_until = control[level].trail;
    }
  }

  STOP (propagate);

  return !conflict;
}

/*------------------------------------------------------------------------*/

void Internal::propergate () {

  CADICAL_assert (!conflict);
  CADICAL_assert (propagated == trail.size ());

  while (propergated != trail.size ()) {

    const int lit = -trail[propergated++];
    LOG ("propergating %d", -lit);
    Watches &ws = watches (lit);

    const const_watch_iterator eow = ws.end ();
    watch_iterator j = ws.begin ();
    const_watch_iterator i = j;

    while (i != eow) {

      const Watch w = *j++ = *i++;

      if (w.binary ()) {
        CADICAL_assert (val (w.blit) > 0);
        continue;
      }
      if (w.clause->garbage) {
        j--;
        continue;
      }

      literal_iterator lits = w.clause->begin ();

      const int other = lits[0] ^ lits[1] ^ lit;
      const signed char u = val (other);

      // TODO: check if u == 0 can happen.
      if (u > 0)
        continue;
      CADICAL_assert (u < 0);

      const int size = w.clause->size;
      const literal_iterator middle = lits + w.clause->pos;
      const const_literal_iterator end = lits + size;
      literal_iterator k = middle;

      int r = 0;
      signed char v = -1;

      while (k != end && (v = val (r = *k)) < 0)
        k++;

      if (v < 0) {
        k = lits + 2;
        CADICAL_assert (w.clause->pos <= size);
        while (k != middle && (v = val (r = *k)) < 0)
          k++;
      }

      CADICAL_assert (lits + 2 <= k), CADICAL_assert (k <= w.clause->end ());
      w.clause->pos = k - lits;

      CADICAL_assert (v > 0);

      LOG (w.clause, "unwatch %d in", lit);

      lits[0] = other;
      lits[1] = r;
      *k = lit;

      watch_literal (r, lit, w.clause);

      j--;
    }

    if (j != i) {

      while (i != eow)
        *j++ = *i++;

      ws.resize (j - ws.begin ());
    }
  }
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
