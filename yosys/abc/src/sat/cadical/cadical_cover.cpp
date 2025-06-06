#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// Covered clause elimination (CCE) is described in our short LPAR-10 paper
// and later in more detail in our JAIR'15 article.  Actually implement
// the asymmetric version which adds asymmetric literals too but still call
// it 'CCE' in the following (and not 'ACCE').  This implementation provides
// a simplified and cleaner version of the one implemented before in
// Lingeling. We still follow quite closely the original description in the
// literature, which is based on asymmetric literal addition (ALA) and
// covered literal addition (CLA).  Both can be seen as kind of propagation,
// where the literals in the original and then extended clause are assigned
// to false, and the literals on the trail (actually we use our own 'added'
// stack for that) make up the extended clause.   The ALA steps can be
// implemented by simple propagation (copied from 'propagate.cpp') using
// watches, while the CLA steps need full occurrence lists to determine the
// resolution candidate clauses.  The CCE is successful if a conflict is
// found during ALA steps or if during a CLA step all resolution candidates
// of a literal on the trail are satisfied (the extended clause is blocked).

struct Coveror {
  std::vector<int> added;        // acts as trail
  std::vector<int> extend;       // extension stack for witness
  std::vector<int> covered;      // clause literals or added through CLA
  std::vector<int> intersection; // of literals in resolution candidates

  size_t alas, clas; // actual number of ALAs and CLAs

  struct {
    size_t added, covered;
  } next; // propagate next ...

  Coveror () : alas (0), clas (0) {}
};

/*------------------------------------------------------------------------*/

// Push on the extension stack a clause made up of the given literal, the
// original clause (initially copied to 'covered') and all the added covered
// literals so far.  The given literal will act as blocking literal for that
// clause, if CCE is successful.  Only in this case, this private extension
// stack is copied to the actual extension stack of the solver.  Note, that
// even though all 'added' clauses correspond to the extended clause, we
// only need to save the original and added covered literals.

inline void Internal::cover_push_extension (int lit, Coveror &coveror) {
  coveror.extend.push_back (0);
  coveror.extend.push_back (lit); // blocking literal comes first
  bool found = false;
  for (const auto &other : coveror.covered)
    if (lit == other)
      CADICAL_assert (!found), found = true;
    else
      coveror.extend.push_back (other);
  CADICAL_assert (found);
  (void) found;
}

// Successful covered literal addition (CLA) step.

inline void Internal::covered_literal_addition (int lit, Coveror &coveror) {
  require_mode (COVER);
  CADICAL_assert (level == 1);
  cover_push_extension (lit, coveror);
  for (const auto &other : coveror.intersection) {
    LOG ("covered literal addition %d", other);
    CADICAL_assert (!vals[other]), CADICAL_assert (!vals[-other]);
    set_val (other, -1);
    coveror.covered.push_back (other);
    coveror.added.push_back (other);
    coveror.clas++;
  }
  coveror.next.covered = 0;
}

// Successful asymmetric literal addition (ALA) step.

inline void Internal::asymmetric_literal_addition (int lit,
                                                   Coveror &coveror) {
  require_mode (COVER);
  CADICAL_assert (level == 1);
  LOG ("initial asymmetric literal addition %d", lit);
  CADICAL_assert (!vals[lit]), CADICAL_assert (!vals[-lit]);
  set_val (lit, -1);
  coveror.added.push_back (lit);
  coveror.alas++;
  coveror.next.covered = 0;
}

/*------------------------------------------------------------------------*/

// In essence copied and adapted from 'propagate' in 'propagate.cpp'. Since
// this function is also a hot-spot here in 'cover' we specialize it in the
// same spirit as 'probe_propagate' and 'vivify_propagate'.  Please refer to
// the detailed comments for 'propagate' in 'propagate.cpp' for details.

bool Internal::cover_propagate_asymmetric (int lit, Clause *ignore,
                                           Coveror &coveror) {
  require_mode (COVER);
  stats.propagations.cover++;
  CADICAL_assert (val (lit) < 0);
  bool subsumed = false;
  LOG ("asymmetric literal propagation of %d", lit);
  Watches &ws = watches (lit);
  const const_watch_iterator eow = ws.end ();
  watch_iterator j = ws.begin ();
  const_watch_iterator i = j;
  while (!subsumed && i != eow) {
    const Watch w = *j++ = *i++;
    if (w.clause == ignore)
      continue; // costly but necessary here ...
    const signed char b = val (w.blit);
    if (b > 0)
      continue;
    if (w.clause->garbage)
      j--;
    else if (w.binary ()) {
      if (b < 0) {
        LOG (w.clause, "found subsuming");
        subsumed = true;
      } else
        asymmetric_literal_addition (-w.blit, coveror);
    } else {
      literal_iterator lits = w.clause->begin ();
      const int other = lits[0] ^ lits[1] ^ lit;
      lits[0] = other, lits[1] = lit;
      const signed char u = val (other);
      if (u > 0)
        j[-1].blit = other;
      else {
        const int size = w.clause->size;
        const const_literal_iterator end = lits + size;
        const literal_iterator middle = lits + w.clause->pos;
        literal_iterator k = middle;
        signed char v = -1;
        int r = 0;
        while (k != end && (v = val (r = *k)) < 0)
          k++;
        if (v < 0) {
          k = lits + 2;
          CADICAL_assert (w.clause->pos <= size);
          while (k != middle && (v = val (r = *k)) < 0)
            k++;
        }
        w.clause->pos = k - lits;
        CADICAL_assert (lits + 2 <= k), CADICAL_assert (k <= w.clause->end ());
        if (v > 0)
          j[-1].blit = r;
        else if (!v) {
          LOG (w.clause, "unwatch %d in", lit);
          lits[1] = r;
          *k = lit;
          watch_literal (r, lit, w.clause);
          j--;
        } else if (!u) {
          CADICAL_assert (v < 0);
          asymmetric_literal_addition (-other, coveror);
        } else {
          CADICAL_assert (u < 0), CADICAL_assert (v < 0);
          LOG (w.clause, "found subsuming");
          subsumed = true;
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
  return subsumed;
}

// Covered literal addition (which needs full occurrence lists).  The
// function returns 'true' if the extended clause is blocked on 'lit.'

bool Internal::cover_propagate_covered (int lit, Coveror &coveror) {
  require_mode (COVER);

  CADICAL_assert (val (lit) < 0);
  if (frozen (lit)) {
    LOG ("no covered propagation on frozen literal %d", lit);
    return false;
  }

  stats.propagations.cover++;

  LOG ("covered propagation of %d", lit);
  CADICAL_assert (coveror.intersection.empty ());

  Occs &os = occs (-lit);
  const auto end = os.end ();
  bool first = true;

  // Compute the intersection of the literals in all the clauses with
  // '-lit'.  If all these clauses are double satisfied then we know that
  // the extended clauses (in 'added') is blocked.  All literals in the
  // intersection can be added as covered literal. As soon the intersection
  // becomes empty (during traversal of clauses with '-lit') we abort.

  for (auto i = os.begin (); i != end; i++) {

    Clause *c = *i;
    if (c->garbage)
      continue;

    // First check whether clause is 'blocked', i.e., is double satisfied.

    bool blocked = false;
    for (const auto &other : *c) {
      if (other == -lit)
        continue;
      const signed char tmp = val (other);
      if (tmp < 0)
        continue;
      if (tmp > 0) {
        blocked = true;
        break;
      }
    }
    if (blocked) { // ... if 'c' is double satisfied.
      LOG (c, "blocked");
      continue; // with next clause with '-lit'.
    }

    if (first) {

      // Copy and mark literals of first clause.

      for (const auto &other : *c) {
        if (other == -lit)
          continue;
        const signed char tmp = val (other);
        if (tmp < 0)
          continue;
        CADICAL_assert (!tmp);
        coveror.intersection.push_back (other);
        mark (other);
      }

      first = false;

    } else {

      // Unmark all literals in current clause.

      for (const auto &other : *c) {
        if (other == -lit)
          continue;
        signed char tmp = val (other);
        if (tmp < 0)
          continue;
        CADICAL_assert (!tmp);
        tmp = marked (other);
        if (tmp > 0)
          unmark (other);
      }

      // Then remove from intersection all marked literals.

      const auto end = coveror.intersection.end ();
      auto j = coveror.intersection.begin ();
      for (auto k = j; k != end; k++) {
        const int other = *j++ = *k;
        const int tmp = marked (other);
        CADICAL_assert (tmp >= 0);
        if (tmp)
          j--, unmark (other); // remove marked and unmark it
        else
          mark (other); // keep unmarked and mark it
      }
      const size_t new_size = j - coveror.intersection.begin ();
      coveror.intersection.resize (new_size);

      if (!coveror.intersection.empty ())
        continue;

      // No covered literal addition candidates in the intersection left!
      // Move this clause triggering early abort to the beginning.
      // This is a common move to front strategy to minimize effort.

      auto begin = os.begin ();
      while (i != begin) {
        auto prev = i - 1;
        *i = *prev;
        i = prev;
      }
      *begin = c;

      break; // early abort ...
    }
  }

  bool res = false;
  if (first) {
    LOG ("all resolution candidates with %d blocked", -lit);
    CADICAL_assert (coveror.intersection.empty ());
    cover_push_extension (lit, coveror);
    res = true;
  } else if (coveror.intersection.empty ()) {
    LOG ("empty intersection of resolution candidate literals");
  } else {
    LOG (coveror.intersection,
         "non-empty intersection of resolution candidate literals");
    covered_literal_addition (lit, coveror);
    unmark (coveror.intersection);
    coveror.intersection.clear ();
    coveror.next.covered = 0; // Restart covering.
  }

  unmark (coveror.intersection);
  coveror.intersection.clear ();

  return res;
}

/*------------------------------------------------------------------------*/

bool Internal::cover_clause (Clause *c, Coveror &coveror) {

  require_mode (COVER);
  CADICAL_assert (!c->garbage);

  LOG (c, "trying covered clauses elimination on");
  bool satisfied = false;
  for (const auto &lit : *c)
    if (val (lit) > 0)
      satisfied = true;

  if (satisfied) {
    LOG (c, "clause already satisfied");
    mark_garbage (c);
    return false;
  }

  CADICAL_assert (coveror.added.empty ());
  CADICAL_assert (coveror.extend.empty ());
  CADICAL_assert (coveror.covered.empty ());

  CADICAL_assert (!level);
  level = 1;
  LOG ("assuming literals of candidate clause");
  for (const auto &lit : *c) {
    if (val (lit))
      continue;
    asymmetric_literal_addition (lit, coveror);
    coveror.covered.push_back (lit);
  }

  bool tautological = false;

  coveror.next.added = coveror.next.covered = 0;

  while (!tautological) {
    if (coveror.next.added < coveror.added.size ()) {
      const int lit = coveror.added[coveror.next.added++];
      tautological = cover_propagate_asymmetric (lit, c, coveror);
    } else if (coveror.next.covered < coveror.covered.size ()) {
      const int lit = coveror.covered[coveror.next.covered++];
      tautological = cover_propagate_covered (lit, coveror);
    } else
      break;
  }

  if (tautological) {
    if (coveror.extend.empty ()) {
      stats.cover.asymmetric++;
      stats.cover.total++;
      LOG (c, "asymmetric tautological");
    } else {
      stats.cover.blocked++;
      stats.cover.total++;
      // Only copy extension stack if successful.
      int prev = INT_MIN;
      bool already_pushed = false;
      int64_t last_id = 0;
      LOG (c, "covered tautological");
      CADICAL_assert (clause.empty ());
      LOG (coveror.extend, "extension = ");
      for (const auto &other : coveror.extend) {
        if (!prev) {
          // are we finishing a clause?
          if (already_pushed) {
            // add missing literals that are not needed for covering
            // but avoid RAT proofs
            for (auto i = 0, j = 0; i < c->size; ++i, ++j) {
              const int lit = c->literals[i];
              if (j >= (int) coveror.covered.size () ||
                  c->literals[i] != coveror.covered[j]) {
                --j;
                LOG ("adding lit %d not needed for ATA", lit);
                clause.push_back (lit);
                external->push_clause_literal_on_extension_stack (lit);
              }
            }
          }
          if (proof && already_pushed) {
            if (lrat)
              lrat_chain.push_back (c->id);
            LOG ("LEARNING clause with id %" PRId64, last_id);
            proof->add_derived_clause (last_id, false, clause, lrat_chain);
            proof->weaken_plus (last_id, clause);
            lrat_chain.clear ();
          }
          last_id = ++clause_id;
          external->push_zero_on_extension_stack ();
          external->push_witness_literal_on_extension_stack (other);
          external->push_zero_on_extension_stack ();
          external->push_id_on_extension_stack (last_id);
          external->push_zero_on_extension_stack ();
          clause.clear ();
          already_pushed = true;
        }
        if (other) {
          external->push_clause_literal_on_extension_stack (other);
          clause.push_back (other);
          LOG (clause, "current clause is");
        }
        prev = other;
      }

      if (proof) {
        // add missing literals that are not needed for covering
        // but avoid RAT proofs
        for (auto i = 0, j = 0; i < c->size; ++i, ++j) {
          const int lit = c->literals[i];
          if (j >= (int) coveror.covered.size () ||
              c->literals[i] != coveror.covered[j]) {
            --j;
            LOG ("adding lit %d not needed for ATA", lit);
            clause.push_back (lit);
            external->push_clause_literal_on_extension_stack (lit);
          }
        }
        if (lrat)
          lrat_chain.push_back (c->id);
        proof->add_derived_clause (last_id, false, clause, lrat_chain);
        proof->weaken_plus (last_id, clause);
        lrat_chain.clear ();
      }
      clause.clear ();

      mark_garbage (c);
    }
  }

  // Backtrack and 'unassign' all literals.

  CADICAL_assert (level == 1);
  for (const auto &lit : coveror.added)
    set_val (lit, 0);
  level = 0;

  coveror.covered.clear ();
  coveror.extend.clear ();
  coveror.added.clear ();

  return tautological;
}

/*------------------------------------------------------------------------*/

// Not yet tried and larger clauses are tried first.

struct clause_covered_or_smaller {
  bool operator() (const Clause *a, const Clause *b) {
    if (a->covered && !b->covered)
      return true;
    if (!a->covered && b->covered)
      return false;
    return a->size < b->size;
  }
};

int64_t Internal::cover_round () {

  if (unsat)
    return 0;

  init_watches ();
  connect_watches (true); // irredundant watches only is enough

  int64_t delta = stats.propagations.search;
  delta *= 1e-3 * opts.covereffort;
  if (delta < opts.covermineff)
    delta = opts.covermineff;
  if (delta > opts.covermaxeff)
    delta = opts.covermaxeff;
  delta = max (delta, ((int64_t) 2) * active ());

  PHASE ("cover", stats.cover.count,
         "covered clause elimination limit of %" PRId64 " propagations",
         delta);

  int64_t limit = stats.propagations.cover + delta;

  init_occs ();

  vector<Clause *> schedule;
  Coveror coveror;

  // First connect all clauses and find all not yet tried clauses.
  //
#ifndef CADICAL_QUIET
  int64_t untried = 0;
#endif
  //
  for (auto c : clauses) {
    CADICAL_assert (!c->frozen);
    if (c->garbage)
      continue;
    if (c->redundant)
      continue;
    bool satisfied = false, allfrozen = true;
    for (const auto &lit : *c)
      if (val (lit) > 0) {
        satisfied = true;
        break;
      } else if (allfrozen && !frozen (lit))
        allfrozen = false;
    if (satisfied) {
      mark_garbage (c);
      continue;
    }
    if (allfrozen) {
      c->frozen = true;
      continue;
    }
    for (const auto &lit : *c)
      occs (lit).push_back (c);
    if (c->size < opts.coverminclslim)
      continue;
    if (c->size > opts.covermaxclslim)
      continue;
    if (c->covered)
      continue;
    schedule.push_back (c);
#ifndef CADICAL_QUIET
    untried++;
#endif
  }

  if (schedule.empty ()) {

    PHASE ("cover", stats.cover.count, "no previously untried clause left");

    for (auto c : clauses) {
      if (c->garbage)
        continue;
      if (c->redundant)
        continue;
      if (c->frozen) {
        c->frozen = false;
        continue;
      }
      if (c->size < opts.coverminclslim)
        continue;
      if (c->size > opts.covermaxclslim)
        continue;
      CADICAL_assert (c->covered);
      c->covered = false;
      schedule.push_back (c);
    }
  } else { // Mix of tried and not tried clauses ....

    for (auto c : clauses) {
      if (c->garbage)
        continue;
      if (c->redundant)
        continue;
      if (c->frozen) {
        c->frozen = false;
        continue;
      }
      if (c->size < opts.coverminclslim)
        continue;
      if (c->size > opts.covermaxclslim)
        continue;
      if (!c->covered)
        continue;
      schedule.push_back (c);
    }
  }

  stable_sort (schedule.begin (), schedule.end (),
               clause_covered_or_smaller ());

#ifndef CADICAL_QUIET
  const size_t scheduled = schedule.size ();
  PHASE ("cover", stats.cover.count,
         "scheduled %zd clauses %.0f%% with %" PRId64 " untried %.0f%%",
         scheduled, percent (scheduled, stats.current.irredundant), untried,
         percent (untried, scheduled));
#endif

  // Heuristically it should be beneficial to intersect with smaller clauses
  // first, since then the chances are higher that the intersection of
  // resolution candidates becomes emptier earlier.

  for (auto lit : lits) {
    if (!active (lit))
      continue;
    Occs &os = occs (lit);
    stable_sort (os.begin (), os.end (), clause_smaller_size ());
  }

  // This is the main loop of trying to do CCE of candidate clauses.
  //
  int64_t covered = 0;
  //
  while (!terminated_asynchronously () && !schedule.empty () &&
         stats.propagations.cover < limit) {
    Clause *c = schedule.back ();
    schedule.pop_back ();
    c->covered = true;
    if (cover_clause (c, coveror))
      covered++;
  }

#ifndef CADICAL_QUIET
  const size_t remain = schedule.size ();
  const size_t tried = scheduled - remain;
  PHASE ("cover", stats.cover.count,
         "eliminated %" PRId64 " covered clauses out of %zd tried %.0f%%",
         covered, tried, percent (covered, tried));
  if (remain)
    PHASE ("cover", stats.cover.count,
           "remaining %zu clauses %.0f%% untried", remain,
           percent (remain, scheduled));
  else
    PHASE ("cover", stats.cover.count, "all scheduled clauses tried");
#endif
  reset_occs ();
  reset_watches ();

  return covered;
}

/*------------------------------------------------------------------------*/

bool Internal::cover () {

  if (!opts.cover)
    return false;
  if (unsat)
    return false;
  if (terminated_asynchronously ())
    return false;
  if (!stats.current.irredundant)
    return false;

  // TODO: Our current algorithm for producing the necessary clauses on the
  // reconstruction stack for extending the witness requires a covered
  // literal addition step which (empirically) conflicts with flushing
  // during restoring clauses (see 'regr00{48,51}.trace') even though
  // flushing during restore is disabled by default (as is covered clause
  // elimination).  The consequence of combining these two options
  // ('opts.cover' and 'opts.restoreflush') can thus produce incorrect
  // witness reconstruction and thus invalid witnesses.  This is quite
  // infrequent (one out of half billion mobical test cases) but as the two
  // regression traces show, does happen. Thus we disable the combination.
  //
  if (opts.restoreflush)
    return false;

  START_SIMPLIFIER (cover, COVER);

  stats.cover.count++;

  // During variable elimination unit clauses can be generated which need to
  // be propagated properly over redundant clauses too.  Since variable
  // elimination avoids to have occurrence lists and watches at the same
  // time this propagation is delayed until the end of variable elimination.
  // Since we want to interleave CCE with it, we have to propagate here.
  // Otherwise this triggers inconsistencies.
  //
  if (propagated < trail.size ()) {
    init_watches ();
    connect_watches (); // need to propagated over all clauses!
    LOG ("elimination produced %zd units",
         (size_t) (trail.size () - propagated));
    if (!propagate ()) {
      LOG ("propagating units before covered clause elimination "
           "results in empty clause");
      learn_empty_clause ();
      CADICAL_assert (unsat);
    }
    reset_watches ();
  }
  CADICAL_assert (unsat || propagated == trail.size ());

  int64_t covered = cover_round ();

  STOP_SIMPLIFIER (cover, COVER);
  report ('c', !opts.reportall && !covered);

  return covered;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
