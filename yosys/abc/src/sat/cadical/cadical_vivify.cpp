#include "global.h"

#include "vivify.hpp"
#include "internal.hpp"
#include "util.hpp"
#include <algorithm>
#include <limits>
#include <utility>

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// Vivification is a special case of asymmetric tautology elimination (ATE)
// and asymmetric literal elimination (ALE).  It strengthens and removes
// clauses proven redundant through unit propagation.
//
// The original algorithm is due to a paper by Piette, Hamadi and Sais
// published at ECAI'08.  We have an inprocessing version, e.g., it does not
// necessarily run-to-completion.  Our version also performs conflict
// analysis and uses a new heuristic for selecting clauses to vivify.

// Our idea is to focus on clauses with many occurrences of its literals in
// other clauses first.  This both complements nicely our implementation of
// subsume, which is bounded, e.g., subsumption attempts are skipped for
// very long clauses with literals with many occurrences and also is
// stronger in the sense that it enables to remove more clauses due to unit
// propagation (AT checks).

// While first focusing on irredundant clause we then added a separate phase
// upfront which focuses on strengthening also redundant clauses in spirit
// of the ideas presented in the IJCAI'17 paper by M. Luo, C.-M. Li, F.
// Xiao, F. Manya, and Z. Lu.

// There is another very similar approach called 'distilliation' published
// by Han and Somenzi in DAC'07, which reorganizes the CNF in a trie data
// structure to reuse decisions and propagations along the trie.  We used
// that as an inspiration but instead of building a trie we simple sort
// clauses and literals in such a way that we get the same effect.  If a new
// clause is 'distilled' or 'vivified' we first check how many of the
// decisions (which are only lazily undone) can be reused for that clause.
// Reusing can be improved by picking a global literal order and sorting the
// literals in all clauses with respect to that order.  We favor literals
// with more occurrences first.  Then we sort clauses lexicographically with
// respect to that literal order.

/*------------------------------------------------------------------------*/

// Candidate clause 'subsumed' is subsumed by 'subsuming'.

inline void Internal::vivify_subsume_clause (Clause *subsuming,
                                             Clause *subsumed) {
  stats.subsumed++;
  stats.vivifysubs++;
#ifndef CADICAL_NDEBUG
  CADICAL_assert (subsuming);
  CADICAL_assert (subsumed);
  CADICAL_assert (subsuming != subsumed);
  CADICAL_assert (!subsumed->garbage);
  // size after removeing units;
  int real_size_subsuming = 0, real_size_subsumed = 0;
  for (auto lit : *subsuming) {
    if (!val (lit) || var (lit).level)
      ++real_size_subsuming;
    else
      CADICAL_assert (val (lit) < 0);
  }
  for (auto lit : *subsumed) {
    if (!val (lit) || var (lit).level)
      ++real_size_subsumed;
    else
      CADICAL_assert (val (lit) < 0);
  }
  CADICAL_assert (real_size_subsuming <= real_size_subsumed);
#endif
  LOG (subsumed, "subsumed");
  if (subsumed->redundant) {
    stats.subred++;
    ++stats.vivifysubred;
  } else {
    stats.subirr++;
    ++stats.vivifysubirr;
  }
  if (subsuming->garbage) {
    CADICAL_assert (subsuming->size == 2);
    LOG (subsuming,
         "binary subsuming clause was already deleted, so undeleting");
    subsuming->garbage = false;
    subsuming->glue = 1;
    ++stats.current.total;
    if (subsuming->redundant)
      stats.current.redundant++;
    else
      stats.current.irredundant++, stats.irrlits += subsuming->size;
  }
  if (subsumed->redundant || !subsuming->redundant) {
    mark_garbage (subsumed);
    return;
  }
  LOG ("turning redundant subsuming clause into irredundant clause");
  subsuming->redundant = false;
  if (proof)
    proof->strengthen (subsuming->id);
  mark_garbage (subsumed);
  mark_added (subsuming);
  stats.current.irredundant++;
  stats.added.irredundant++;
  stats.irrlits += subsuming->size;
  CADICAL_assert (stats.current.redundant > 0);
  stats.current.redundant--;
  CADICAL_assert (stats.added.redundant > 0);
  stats.added.redundant--;
  // ... and keep 'stats.added.total'.
}

// demoting a clause (opposite is promote from subsume.cpp)

inline void Internal::demote_clause (Clause *c) {
  stats.subsumed++;
  stats.vivifydemote++;
  LOG (c, "demoting");
  CADICAL_assert (!c->redundant);
  mark_removed (c);
  c->redundant = true;
  CADICAL_assert (stats.current.irredundant > 0);
  stats.current.irredundant--;
  CADICAL_assert (stats.added.irredundant > 0);
  stats.added.irredundant--;
  stats.irrlits -= c->size;
  stats.current.redundant++;
  stats.added.redundant++;
  c->glue = c->size - 1;
  // ... and keep 'stats.added.total'.
}

/*------------------------------------------------------------------------*/
// For vivification we have a separate dedicated propagation routine, which
// prefers to propagate binary clauses first.  It also uses its own
// assignment procedure 'vivify_assign', which does not mess with phase
// saving during search nor the conflict and other statistics and further
// can be inlined separately here.  The propagation routine needs to ignore
// (large) clauses which are currently vivified.

inline void Internal::vivify_assign (int lit, Clause *reason) {
  require_mode (VIVIFY);
  const int idx = vidx (lit);
  CADICAL_assert (!vals[idx]);
  CADICAL_assert (!flags (idx).eliminated () || !reason);
  Var &v = var (idx);
  v.level = level;               // required to reuse decisions
  v.trail = (int) trail.size (); // used in 'vivify_better_watch'
  CADICAL_assert ((int) num_assigned < max_var);
  num_assigned++;
  v.reason = level ? reason : 0; // for conflict analysis
  if (!level)
    learn_unit_clause (lit);
  const signed char tmp = sign (lit);
  vals[idx] = tmp;
  vals[-idx] = -tmp;
  CADICAL_assert (val (lit) > 0);
  CADICAL_assert (val (-lit) < 0);
  trail.push_back (lit);
  LOG (reason, "vivify assign %d", lit);
}

// Assume negated literals in candidate clause.

void Internal::vivify_assume (int lit) {
  require_mode (VIVIFY);
  level++;
  control.push_back (Level (lit, trail.size ()));
  LOG ("vivify decide %d", lit);
  CADICAL_assert (level > 0);
  CADICAL_assert (propagated == trail.size ());
  vivify_assign (lit, 0);
}

// Dedicated routine similar to 'propagate' in 'propagate.cpp' and
// 'probe_propagate' with 'probe_propagate2' in 'probe.cpp'.  Please refer
// to that code for more explanation on how propagation is implemented.

bool Internal::vivify_propagate (int64_t &ticks) {
  require_mode (VIVIFY);
  CADICAL_assert (!unsat);
  START (propagate);
  int64_t before = propagated2 = propagated;
  for (;;) {
    if (propagated2 != trail.size ()) {
      const int lit = -trail[propagated2++];
      LOG ("vivify propagating %d over binary clauses", -lit);
      Watches &ws = watches (lit);
      ticks +=
          1 + cache_lines (ws.size (), sizeof (const_watch_iterator *));
      for (const auto &w : ws) {
        if (!w.binary ())
          continue;
        const signed char b = val (w.blit);
        if (b > 0)
          continue;
        if (b < 0)
          conflict = w.clause; // but continue
        else {
          ticks++;
          build_chain_for_units (w.blit, w.clause, 0);
          vivify_assign (w.blit, w.clause);
          lrat_chain.clear ();
        }
      }
    } else if (!conflict && propagated != trail.size ()) {
      const int lit = -trail[propagated++];
      LOG ("vivify propagating %d over large clauses", -lit);
      Watches &ws = watches (lit);
      const const_watch_iterator eow = ws.end ();
      const_watch_iterator i = ws.begin ();
      ticks += 1 + cache_lines (ws.size (), sizeof (*i));
      watch_iterator j = ws.begin ();
      while (i != eow) {
        const Watch w = *j++ = *i++;
        if (w.binary ())
          continue;
        if (val (w.blit) > 0)
          continue;
        ticks++;
        if (w.clause->garbage) {
          j--;
          continue;
        }
        literal_iterator lits = w.clause->begin ();
        const int other = lits[0] ^ lits[1] ^ lit;
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
            LOG (w.clause, "unwatch %d in", r);
            lits[0] = other;
            lits[1] = r;
            *k = lit;
            ticks++;
            watch_literal (r, lit, w.clause);
            j--;
          } else if (!u) {
            if (w.clause == ignore) {
              LOG ("ignoring propagation due to clause to vivify");
              continue;
            }
            ticks++;
            CADICAL_assert (v < 0);
            vivify_chain_for_units (other, w.clause);
            vivify_assign (other, w.clause);
            lrat_chain.clear ();
          } else {
            if (w.clause == ignore) {
              LOG ("ignoring conflict due to clause to vivify");
              continue;
            }
            CADICAL_assert (u < 0);
            CADICAL_assert (v < 0);
            conflict = w.clause;
            break;
          }
        }
      }
      if (j != i) {
        while (i != eow)
          *j++ = *i++;
        ws.resize (j - ws.begin ());
      }
    } else
      break;
  }
  int64_t delta = propagated2 - before;
  stats.propagations.vivify += delta;
  if (conflict)
    LOG (conflict, "conflict");
  STOP (propagate);
  return !conflict;
}

/*------------------------------------------------------------------------*/

// Check whether a literal occurs less often.  In the implementation below
// (search for 'int64_t score = ...' or '@4') we actually compute a
// weighted occurrence count similar to the Jeroslow Wang heuristic.

struct vivify_more_noccs {

  Internal *internal;

  vivify_more_noccs (Internal *i) : internal (i) {}

  bool operator() (int a, int b) {
    int64_t n = internal->noccs (a);
    int64_t m = internal->noccs (b);
    if (n > m)
      return true; // larger occurrences / score first
    if (n < m)
      return false; // smaller occurrences / score last
    if (a == -b)
      return a > 0;           // positive literal first
    return abs (a) < abs (b); // smaller index first
  }
};

struct vivify_more_noccs_kissat {

  Internal *internal;

  vivify_more_noccs_kissat (Internal *i) : internal (i) {}

  bool operator() (int a, int b) {
    unsigned t = internal->noccs (a);
    unsigned s = internal->noccs (b);
    return ((t - s) | ((b - a) & ~(s - t))) >> 31;
  }
};

// Sort candidate clauses by the number of occurrences (actually by their
// score) of their literals, with clauses to be vivified first last.   We
// assume that clauses are sorted w.r.t. more occurring (higher score)
// literals first (with respect to 'vivify_more_noccs').
//
// For example if there are the following (long irredundant) clauses
//
//   1 -3 -4      (A)
//  -1 -2  3 4    (B)
//   2 -3  4      (C)
//
// then we have the following literal scores using Jeroslow Wang scores and
// normalizing it with 2^12 (which is the same as 1<<12):
//
//  nocc ( 1) = 2^12 * (2^-3       ) =  512  3.
//  nocc (-1) = 2^12 * (2^-4       ) =  256  6.
//  nocc ( 2) = 2^12 * (2^-3       ) =  512  4.
//  nocc (-2) = 2^12 * (2^-4       ) =  256  7.                 @1
//  nocc ( 3) = 2^12 * (2^-4       ) =  256  8.
//  nocc (-3) = 2^12 * (2^-3 + 2^-3) = 1024  1.
//  nocc ( 4) = 2^12 * (2^-3 + 2^-4) =  768  2.
//  nocc (-4) = 2^12 * (2^-3       ) =  512  5.
//
// which gives the literal order (according to 'vivify_more_noccs')
//
//  -3, 4, 1, 2, -4, -1, -2, 3
//
// Then sorting the literals in each clause gives
//
//  -3  1 -4     (A')
//   4 -1 -2  3  (B')                                           @2
//  -3  4  2     (C')
//
// and finally sorting those clauses lexicographically w.r.t. scores is
//
//  -3  4  2     (C')
//  -3  1 -4     (A')                                           @3
//   4 -1 -2  3  (B')
//
// This order is defined by 'vivify_clause_later' which returns 'true' if
// the first clause should be vivified later than the second.

struct vivify_clause_later {

  Internal *internal;

  vivify_clause_later (Internal *i) : internal (i) {}

  bool operator() (Clause *a, Clause *b) const {

    if (a == b)
      return false;

    // First focus on clauses scheduled in the last vivify round but not
    // checked yet since then.
    //
    if (!a->vivify && b->vivify)
      return true;
    if (a->vivify && !b->vivify)
      return false;

    // Among redundant clauses (in redundant mode) prefer small glue.
    //
    if (a->redundant) {
      CADICAL_assert (b->redundant);
      if (a->glue > b->glue)
        return true;
      if (a->glue < b->glue)
        return false;
    }

    // Then prefer shorter size.
    //
    if (a->size > b->size)
      return true;
    if (a->size < b->size)
      return false;

    // Now compare literals in the clauses lexicographically with respect to
    // the literal order 'vivify_more_noccs' assuming literals are sorted
    // decreasingly with respect to that order.
    //
    const auto eoa = a->end (), eob = b->end ();
    auto j = b->begin ();
    for (auto i = a->begin (); i != eoa && j != eob; i++, j++)
      if (*i != *j)
        return vivify_more_noccs (internal) (*j, *i);

    return j == eob; // Prefer shorter clauses to be vivified first.
  }
};

/*------------------------------------------------------------------------*/

// Attempting on-the-fly subsumption during sorting when the last line is
// reached in 'vivify_clause_later' above turned out to be trouble some for
// identical clauses.  This is the single point where 'vivify_clause_later'
// is not asymmetric and would require 'stable' sorting for determinism.  It
// can also not be made 'complete' on-the-fly.  Instead of on-the-fly
// subsumption we thus go over the sorted scheduled in a linear scan
// again and remove certain subsumed clauses (the subsuming clause is
// syntactically a prefix of the subsumed clause), which includes
// those troublesome syntactically identical clauses.

struct vivify_flush_smaller {

  bool operator() (Clause *a, Clause *b) const {

    const auto eoa = a->end (), eob = b->end ();
    auto i = a->begin (), j = b->begin ();
    for (; i != eoa && j != eob; i++, j++)
      if (*i != *j)
        return *i < *j;

    return j == eob && i != eoa;
  }
};

void Internal::flush_vivification_schedule (std::vector<Clause *> &schedule,
                                            int64_t &ticks) {
  ticks += 1 + 3 * cache_lines (schedule.size (), sizeof (Clause *));
  stable_sort (schedule.begin (), schedule.end (), vivify_flush_smaller ());

  const auto end = schedule.end ();
  auto j = schedule.begin (), i = j;

  Clause *prev = 0;
  int64_t subsumed = 0;
  for (; i != end; i++) {
    ticks++;
    Clause *c = *j++ = *i;
    if (!prev || c->size < prev->size) {
      prev = c;
      continue;
    }
    const auto eop = prev->end ();
    auto k = prev->begin ();
    for (auto l = c->begin (); k != eop; k++, l++)
      if (*k != *l)
        break;
    if (k == eop) {
      LOG (c, "found subsumed");
      LOG (prev, "subsuming");
      CADICAL_assert (!c->garbage);
      CADICAL_assert (!prev->garbage);
      CADICAL_assert (c->redundant || !prev->redundant);
      mark_garbage (c);
      subsumed++;
      j--;
    } else
      prev = c;
  }

  if (subsumed)
    PHASE ("vivify", stats.vivifications,
           "flushed %" PRId64 " subsumed scheduled clauses", subsumed);

  stats.vivifysubs += subsumed;

  if (subsumed) {
    schedule.resize (j - schedule.begin ());
    shrink_vector (schedule);
  } else
    CADICAL_assert (j == end);
}

/*------------------------------------------------------------------------*/

// Depending on whether we try to vivify redundant or irredundant clauses,
// we schedule a clause to be vivified.  For redundant clauses we initially
// only try to vivify them if they are likely to survive the next 'reduce'
// operation, but this left the last schedule empty most of the time.

bool Internal::consider_to_vivify_clause (Clause *c) {
  if (c->garbage)
    return false;
  if (opts.vivifyonce >= 1 && c->redundant && c->vivified)
    return false;
  if (opts.vivifyonce >= 2 && !c->redundant && c->vivified)
    return false;
  if (!c->redundant)
    return true;
  CADICAL_assert (c->redundant);

  // likely_to_be_kept_clause is too aggressive at removing tier-3 clauses
  return true;
}

/*------------------------------------------------------------------------*/

// In a strengthened clause the idea is to move non-false literals to the
// front, followed by false literals.  Literals are further sorted by
// reverse assignment order.  The goal is to use watches which require to
// backtrack as few as possible decision levels.

struct vivify_better_watch {

  Internal *internal;

  vivify_better_watch (Internal *i) : internal (i) {}

  bool operator() (int a, int b) {

    const signed char av = internal->val (a), bv = internal->val (b);

    if (av >= 0 && bv < 0)
      return true;
    if (av < 0 && bv >= 0)
      return false;

    return internal->var (a).trail > internal->var (b).trail;
  }
};

// Common code to actually strengthen a candidate clause.  The resulting
// strengthened clause is communicated through the global 'clause'.

void Internal::vivify_strengthen (Clause *c) {

  CADICAL_assert (!clause.empty ());

  if (clause.size () == 1) {

    backtrack_without_updating_phases ();
    const int unit = clause[0];
    LOG (c, "vivification shrunken to unit %d", unit);
    CADICAL_assert (!val (unit));
    assign_unit (unit);
    // lrat_chain.clear ();   done in search_assign
    stats.vivifyunits++;

    bool ok = propagate ();
    if (!ok)
      learn_empty_clause ();

  } else {

    // See explanation before 'vivify_better_watch' above.
    //
    sort (clause.begin (), clause.end (), vivify_better_watch (this));

    int new_level = level;

    const int lit0 = clause[0];
    signed char val0 = val (lit0);
    if (val0 < 0) {
      const int level0 = var (lit0).level;
      LOG ("1st watch %d negative at level %d", lit0, level0);
      new_level = level0 - 1;
    }

    const int lit1 = clause[1];
    const signed char val1 = val (lit1);
    if (val1 < 0 && !(val0 > 0 && var (lit0).level <= var (lit1).level)) {
      const int level1 = var (lit1).level;
      LOG ("2nd watch %d negative at level %d", lit1, level1);
      new_level = level1 - 1;
    }

    CADICAL_assert (new_level >= 0);
    if (new_level < level)
      backtrack (new_level);

    CADICAL_assert (val (lit0) >= 0);
    CADICAL_assert (val (lit1) >= 0 || (val (lit0) > 0 && val (lit1) < 0 &&
                                var (lit0).level <= var (lit1).level));

    Clause *d = new_clause_as (c);
    LOG (c, "before vivification");
    LOG (d, "after vivification");
    (void) d;
  }
  clause.clear ();
  mark_garbage (c);
  lrat_chain.clear ();
  ++stats.vivifystrs;
}

void Internal::vivify_sort_watched (Clause *c) {

  sort (c->begin (), c->end (), vivify_better_watch (this));

  int new_level = level;

  const int lit0 = c->literals[0];
  signed char val0 = val (lit0);
  if (val0 < 0) {
    const int level0 = var (lit0).level;
    LOG ("1st watch %d negative at level %d", lit0, level0);
    new_level = level0 - 1;
  }

  const int lit1 = c->literals[1];
  const signed char val1 = val (lit1);
  if (val1 < 0 && !(val0 > 0 && var (lit0).level <= var (lit1).level)) {
    const int level1 = var (lit1).level;
    LOG ("2nd watch %d negative at level %d", lit1, level1);
    new_level = level1 - 1;
  }

  CADICAL_assert (new_level >= 0);
  if (new_level < level)
    backtrack (new_level);

  CADICAL_assert (val (lit0) >= 0);
  CADICAL_assert (val (lit1) >= 0 || (val (lit0) > 0 && val (lit1) < 0 &&
                              var (lit0).level <= var (lit1).level));
}
// Conflict analysis from 'start' which learns a decision only clause.
//
// We cannot use the stack-based implementation of Kissat, because we need
// to iterate over the conflict in topological ordering to produce a valid
// LRAT proof

void Internal::vivify_analyze (Clause *start, bool &subsumes,
                               Clause **subsuming,
                               const Clause *const candidate, int implied,
                               bool &redundant) {
  const auto &t = &trail; // normal trail, so next_trail is wrong
  int i = t->size ();     // Start at end-of-trail.
  Clause *reason = start;
  CADICAL_assert (reason);
  CADICAL_assert (!trail.empty ());
  int uip = trail.back ();
  bool mark_implied = (implied);

  while (i >= 0) {
    if (reason) {
      redundant = (redundant || reason->redundant);
      subsumes = (start != reason && reason->size <= start->size);
      LOG (reason, "resolving on %d with", uip);
      for (auto other : *reason) {
        const Var v = var (other);
        Flags &f = flags (other);
        if (!marked2 (other) && v.level) {
          LOG ("not subsuming due to lit %d", other);
          subsumes = false;
        }
        if (!val (other)) {
          LOG ("skipping unset lit %d", other);
          continue;
        }
        if (other == uip) {
          continue;
        }
        if (!v.level) {
          if (f.seen || !lrat || reason == start)
            continue;
          LOG ("unit reason for %d", other);
          int64_t id = unit_id (-other);
          LOG ("adding unit reason %zd for %d", id, other);
          unit_chain.push_back (id);
          f.seen = true;
          analyzed.push_back (other);
          continue;
        }
        if (mark_implied && other != implied) {
          LOG ("skipping non-implied literal %d on current level", other);
          continue;
        }

        CADICAL_assert (val (other));
        if (f.seen)
          continue;
        LOG ("pushing lit %d", other);
        analyzed.push_back (other);
        f.seen = true;
      }
      if (start->redundant) {
        const int new_glue = recompute_glue (start);
        promote_clause (start, new_glue);
      }
      if (subsumes) {
        CADICAL_assert (reason);
        LOG (reason, "clause found subsuming");
        LOG (candidate, "clause found subsumed");
        *subsuming = reason;
        return;
      }
    } else {
      LOG ("vivify analyzed decision %d", uip);
      clause.push_back (-uip);
    }
    mark_implied = false;

    uip = 0;
    while (!uip && i > 0) {
      CADICAL_assert (i > 0);
      const int lit = (*t)[--i];
      if (!var (lit).level)
        continue;
      if (flags (lit).seen)
        uip = lit;
    }
    if (!uip)
      break;
    LOG ("uip is %d", uip);
    Var &w = var (uip);
    reason = w.reason;
    if (lrat && reason)
      lrat_chain.push_back (reason->id);
  }
  (void) candidate;
}

void Internal::vivify_deduce (Clause *candidate, Clause *conflict,
                              int implied, Clause **subsuming,
                              bool &redundant) {
  CADICAL_assert (lrat_chain.empty ());
  bool subsumes;
  Clause *reason;

  CADICAL_assert (clause.empty ());
  if (implied) {
    reason = candidate;
    mark2 (candidate);
    const int not_implied = -implied;
    CADICAL_assert (var (not_implied).level);
    Flags &f = flags (not_implied);
    f.seen = true;
    LOG ("pushing implied lit %d", not_implied);
    analyzed.push_back (not_implied);
    clause.push_back (implied);
  } else {
    reason = (conflict ? conflict : candidate);
    CADICAL_assert (reason);
    CADICAL_assert (!reason->garbage);
    mark2 (candidate);
    subsumes = (candidate != reason);
    redundant = reason->redundant;
    LOG (reason, "resolving with");
    if (lrat)
      lrat_chain.push_back (reason->id);
    for (auto lit : *reason) {
      const Var &v = var (lit);
      Flags &f = flags (lit);
      CADICAL_assert (val (lit) < 0);
      if (!v.level) {
        if (!lrat)
          continue;
        LOG ("adding unit %d", lit);
        if (!f.seen) {
          // nevertheless we can use var (l) as if l was still assigned
          // because var is updated lazily
          int64_t id = unit_id (-lit);
          LOG ("adding unit reason %zd for %d", id, lit);
          unit_chain.push_back (id);
        }
        f.seen = true;
        analyzed.push_back (lit);
        continue;
      }
      CADICAL_assert (v.level);
      if (!marked2 (lit)) {
        LOG ("lit %d is not marked", lit);
        subsumes = false;
      }
      LOG ("analyzing lit %d", lit);
      LOG ("pushing lit %d", lit);
      analyzed.push_back (lit);
      f.seen = true;
    }
    if (reason != candidate && reason->redundant) {
      const int new_glue = recompute_glue (reason);
      promote_clause (reason, new_glue);
    }
    if (subsumes) {
      CADICAL_assert (candidate != reason);
#ifndef CADICAL_NDEBUG
      int nonfalse_reason = 0;
      for (auto lit : *reason)
        if (!fixed (lit))
          ++nonfalse_reason;

      int nonfalse_candidate = 0;
      for (auto lit : *candidate)
        if (!fixed (lit))
          ++nonfalse_candidate;

      CADICAL_assert (nonfalse_reason <= nonfalse_candidate);
#endif
      LOG (candidate, "vivify subsumed 0");
      LOG (reason, "vivify subsuming 0");
      *subsuming = reason;
      unmark (candidate);
      if (lrat)
        lrat_chain.clear ();
      return;
    }
  }

  vivify_analyze (reason, subsumes, subsuming, candidate, implied,
                  redundant);
  unmark (candidate);
  if (subsumes) {
    CADICAL_assert (*subsuming);
    LOG (candidate, "vivify subsumed");
    LOG (*subsuming, "vivify subsuming");
    if (lrat)
      lrat_chain.clear ();
  }
}
/*------------------------------------------------------------------------*/

bool Internal::vivify_shrinkable (const std::vector<int> &sorted,
                                  Clause *conflict) {

  unsigned count_implied = 0;
  for (auto lit : sorted) {
    const signed char value = val (lit);
    if (!value) {
      LOG ("vivification unassigned %d", lit);
      return true;
    }
    if (value > 0) {
      LOG ("vivification implied satisfied %d", lit);
      if (conflict)
        return true;
      if (count_implied++) {
        LOG ("at least one implied literal with conflict thus shrinking");
        return true;
      }
    } else {
      CADICAL_assert (value < 0);
      const Var &v = var (lit);
      const Flags &f = flags (lit);
      if (!v.level)
        continue;
      if (!f.seen) {
        LOG ("vivification non-analyzed %d", lit);
        return true;
      }
      if (v.reason) {
        LOG ("vivification implied falsified %d", lit);
        return true;
      }
    }
  }
  return false;
}
/*------------------------------------------------------------------------*/

inline void Internal::vivify_increment_stats (const Vivifier &vivifier) {
  switch (vivifier.tier) {
  case Vivify_Mode::TIER1:
    ++stats.vivifystred1;
    break;
  case Vivify_Mode::TIER2:
    ++stats.vivifystred2;
    break;
  case Vivify_Mode::TIER3:
    ++stats.vivifystred3;
    break;
  default:
    CADICAL_assert (vivifier.tier == Vivify_Mode::IRREDUNDANT);
    ++stats.vivifystrirr;
    break;
  }
}
/*------------------------------------------------------------------------*/
// instantiate last literal (see the description of the hack track 2023),
// fix the watches and
//  backtrack two level back
bool Internal::vivify_instantiate (
    const std::vector<int> &sorted, Clause *c,
    std::vector<std::tuple<int, Clause *, bool>> &lrat_stack,
    int64_t &ticks) {
  LOG ("now trying instantiation");
  conflict = nullptr;
  const int lit = sorted.back ();
  LOG ("vivify instantiation");
  CADICAL_assert (!var (lit).reason);
  CADICAL_assert (var (lit).level);
  CADICAL_assert (val (lit));
  backtrack (level - 1);
  CADICAL_assert (val (lit) == 0);
  stats.vivifydecs++;
  vivify_assume (lit);
  bool ok = vivify_propagate (ticks);
  if (!ok) {
    LOG (c, "instantiate success with literal %d in", lit);
    stats.vivifyinst++;
    // strengthen clause
    if (lrat) {
      clear_analyzed_literals ();
      CADICAL_assert (lrat_chain.empty ());
      vivify_build_lrat (0, c, lrat_stack);
      vivify_build_lrat (0, conflict, lrat_stack);
      clear_analyzed_literals ();
    }
    int remove = lit;
    conflict = nullptr;
    unwatch_clause (c);
    backtrack_without_updating_phases (level - 2);
    strengthen_clause (c, remove);
    vivify_sort_watched (c);
    watch_clause (c);
    CADICAL_assert (!conflict);
    return true;
  } else {
    LOG ("vivify instantiation failed");
    return false;
  }
}

/*------------------------------------------------------------------------*/

// Main function: try to vivify this candidate clause in the given mode.

bool Internal::vivify_clause (Vivifier &vivifier, Clause *c) {

  CADICAL_assert (c->size > 2); // see (NO-BINARY) below
  CADICAL_assert (analyzed.empty ());

  c->vivify = false;  // mark as checked / tried
  c->vivified = true; // and globally remember

  CADICAL_assert (!c->garbage);

  auto &lrat_stack = vivifier.lrat_stack;
  auto &ticks = vivifier.ticks;
  ticks++;

  // First check whether the candidate clause is already satisfied and at
  // the same time copy its non fixed literals to 'sorted'.  The literals
  // in the candidate clause might not be sorted anymore due to replacing
  // watches during propagation, even though we sorted them initially
  // while pushing the clause onto the schedule and sorting the schedule.
  //
  auto &sorted = vivifier.sorted;
  sorted.clear ();

  for (const auto &lit : *c) {
    const int tmp = fixed (lit);
    if (tmp > 0) {
      LOG (c, "satisfied by propagated unit %d", lit);
      mark_garbage (c);
      return false;
    } else if (!tmp)
      sorted.push_back (lit);
  }

  CADICAL_assert (sorted.size () > 1);
  if (sorted.size () == 2) {
    LOG ("skipping actual binary");
    return false;
  }

  sort (sorted.begin (), sorted.end (), vivify_more_noccs_kissat (this));

  // The actual vivification checking is performed here, by assuming the
  // negation of each of the remaining literals of the clause in turn and
  // propagating it.  If a conflict occurs or another literal in the
  // clause becomes assigned during propagation, we can stop.
  //
  LOG (c, "vivification checking");
  stats.vivifychecks++;

  // If the decision 'level' is non-zero, then we can reuse decisions for
  // the previous candidate, and avoid re-propagating them.  In preliminary
  // experiments this saved between 30%-50% decisions (and thus
  // propagations), which in turn lets us also vivify more clauses within
  // the same propagation bounds, or terminate earlier if vivify runs to
  // completion.
  //
  if (level) {
#ifdef LOGGING
    int orig_level = level;
#endif
    // First check whether this clause is actually a reason for forcing
    // one of its literals to true and then backtrack one level before
    // that happened.  Otherwise this clause might be incorrectly
    // considered to be redundant or if this situation is checked then
    // redundancy by other clauses using this forced literal becomes
    // impossible.
    //
    int forced = 0;

    // This search could be avoided if we would eagerly set the 'reason'
    // boolean flag of clauses, which however we do not want to do for
    // binary clauses (during propagation) and thus would still require
    // a version of 'protect_reason' for binary clauses during 'reduce'
    // (well binary clauses are not collected during 'reduce', but again
    // this exception from the exception is pretty complex and thus a
    // simply search here is probably easier to understand).

    for (const auto &lit : *c) {
      const signed char tmp = val (lit);
      if (tmp < 0)
        continue;
      if (tmp > 0 && var (lit).reason == c)
        forced = lit;
      break;
    }
    if (forced) {
      LOG ("clause is reason forcing %d", forced);
      CADICAL_assert (var (forced).level);
      backtrack_without_updating_phases (var (forced).level - 1);
    }

    // As long the (remaining) literals of the sorted clause match
    // decisions on the trail we just reuse them.
    //
    if (level) {

      int l = 1; // This is the decision level we want to reuse.

      for (const auto &lit : sorted) {
        CADICAL_assert (!fixed (lit));
        const int decision = control[l].decision;
        if (-lit == decision) {
          LOG ("reusing decision %d at decision level %d", decision, l);
          stats.vivifyreused++;
          if (++l > level)
            break;
        } else {
          LOG ("literal %d does not match decision %d at decision level %d",
               lit, decision, l);
          backtrack_without_updating_phases (l - 1);
          break;
        }
      }
    }

    LOG ("reused %d decision levels from %d", level, orig_level);
  }

  LOG (sorted, "sorted size %zd probing schedule", sorted.size ());

  // Make sure to ignore this clause during propagation.  This is not that
  // easy for binary clauses (NO-BINARY), e.g., ignoring binary clauses,
  // without changing 'propagate'. Actually, we do not want to remove binary
  // clauses which are subsumed.  Those are hyper binary resolvents and
  // should be kept as learned clauses instead, unless they are transitive
  // in the binary implication graph, which in turn is detected during
  // transitive reduction in 'transred'.
  //
  ignore = c;

  int subsume = 0; // determined to be redundant / subsumed

  // If the candidate is redundant, i.e., we are in redundant mode, the
  // clause is subsumed (in one of the two cases below where 'subsume' is
  // assigned) and further all reasons involved are only binary clauses,
  // then this redundant clause is what we once called a hidden tautology,
  // and even for redundant clauses it makes sense to remove the candidate.
  // It does not add anything to propagation power of the formula.  This is
  // the same argument as removing transitive clauses in the binary
  // implication graph during transitive reduction.
  //

  // Go over the literals in the candidate clause in sorted order.
  //
  for (const auto &lit : sorted) {

    // Exit loop as soon a literal is positively implied (case '@5' below)
    // or propagation of the negation of a literal fails ('@6').
    //
    if (subsume)
      break;

    // We keep on assigning literals, even though we know already that we
    // can remove one (was negatively implied), since we either might run
    // into the 'subsume' case above or more false literals become implied.
    // In any case this might result in stronger vivified clauses.  As a
    // consequence continue with this loop even if 'remove' is non-zero.

    const signed char tmp = val (lit);

    if (tmp) { // literal already assigned

      const Var &v = var (lit);
      CADICAL_assert (v.level);
      if (!v.reason) {
        LOG ("skipping decision %d", lit);
        continue;
      }

      if (tmp < 0) {
        CADICAL_assert (v.level);
        LOG ("literal %d is already false and can be removed", lit);
        continue;
      }

      CADICAL_assert (tmp > 0);
      LOG ("subsumed since literal %d already true", lit);
      subsume = lit; // will be able to subsume candidate '@5'
      break;
    }

    CADICAL_assert (!tmp);

    stats.vivifydecs++;
    vivify_assume (-lit);
    LOG ("negated decision %d score %" PRId64 "", lit, noccs (lit));

    if (!vivify_propagate (ticks)) {
      break; // hot-spot
    }
  }

  if (subsume) {
    int better_subsume_trail = var (subsume).trail;
    for (auto lit : sorted) {
      if (val (lit) <= 0)
        continue;
      const Var v = var (lit);
      if (v.trail < better_subsume_trail) {
        LOG ("improving subsume from %d at %d to %d at %d", subsume,
             better_subsume_trail, lit, v.trail);
        better_subsume_trail = v.trail;
        subsume = lit;
      }
    }
  }

  Clause *subsuming = nullptr;
  bool redundant = false;
  const int level_after_assumptions = level;
  CADICAL_assert (level_after_assumptions);
  vivify_deduce (c, conflict, subsume, &subsuming, redundant);

  bool res;

  // reverse lrat_chain. We could probably work with reversed iterators
  // (views) to be more efficient but we would have to distinguish in proof
  //
  if (lrat) {
    for (auto id : unit_chain)
      lrat_chain.push_back (id);
    unit_chain.clear ();
    reverse (lrat_chain.begin (), lrat_chain.end ());
  }

  if (subsuming) {
    CADICAL_assert (c != subsuming);
    LOG (c, "deleting subsumed clause");
    if (c->redundant && subsuming->redundant && c->glue < subsuming->glue) {
      promote_clause (c, c->glue);
    }
    vivify_subsume_clause (subsuming, c);
    res = false;
    // stats.vivifysubs++;  // already done in vivify_subsume_clause
  } else if (vivify_shrinkable (sorted, conflict)) {
    vivify_increment_stats (vivifier);
    LOG ("vivify succeeded, learning new clause");
    clear_analyzed_literals ();
    LOG (lrat_chain, "lrat");
    LOG (clause, "learning clause");
    conflict = nullptr; // TODO dup from below
    vivify_strengthen (c);
    res = true;
  } else if (subsume && c->redundant) {
    LOG (c, "vivification implied");
    mark_garbage (c);
    ++stats.vivifyimplied;
    res = true;
  } else if ((conflict || subsume) && !c->redundant && !redundant) {
    LOG ("demote clause from irredundant to redundant");
    if (opts.vivifydemote) {
      demote_clause (c);
      const int new_glue = recompute_glue (c);
      promote_clause (c, new_glue);
      res = false;
    } else {
      mark_garbage (c);
      ++stats.vivifyimplied;
      res = true;
    }
  } else if (subsume) {
    LOG (c, "no vivification instantiation with implied literal %d",
         subsume);
    CADICAL_assert (!c->redundant);
    CADICAL_assert (redundant);
    res = false;
    ++stats.vivifyimplied;
  } else {
    CADICAL_assert (level > 2);
    CADICAL_assert ((size_t) level == sorted.size ());
    LOG (c, "vivification failed on");
    lrat_chain.clear ();
    CADICAL_assert (!subsume);
    if (!subsume && opts.vivifyinst) {
      res = vivify_instantiate (sorted, c, lrat_stack, ticks);
      CADICAL_assert (!conflict);
    } else {
      LOG ("cannot apply instantiation");
      res = false;
    }
  }

  if (conflict && level == level_after_assumptions) {
    LOG ("forcing backtracking at least one level after conflict");
    backtrack_without_updating_phases (level - 1);
  }

  clause.clear ();
  clear_analyzed_literals (); // TODO why needed?
  lrat_chain.clear ();
  conflict = nullptr;
  return res;
}

// when we can strengthen clause c we have to build lrat.
// uses f.seen so do not forget to clear flags afterwards.
// this can happen in three cases. (1), (2) are only sound in redundant mode
// (1) literal l in c is positively implied. in this case we call the
// function with (l, l.reason). This justifies the reduction because the new
// clause c' will include l and all decisions so l.reason is a conflict
// assuming -c' (2) conflict during vivify propagation. function is called
// with (0, conflict) similar to (1) but more direct. (3) some literals in c
// are negatively implied and can therefore be removed. in this case we call
// the function with (0, c). originally we justified each literal in c on
// its own but this is not actually necessary.
//

// Non-recursive version, as some bugs have been found.  DFS over the
// reasons with preordering (aka we explore the entire reason before
// exploring deeper)
void Internal::vivify_build_lrat (
    int lit, Clause *reason,
    std::vector<std::tuple<int, Clause *, bool>> &stack) {
  CADICAL_assert (stack.empty ());
  stack.push_back ({lit, reason, false});
  while (!stack.empty ()) {
    int lit;
    Clause *reason;
    bool finished;
    std::tie (lit, reason, finished) = stack.back ();
    LOG ("VIVIFY LRAT justifying %d", lit);
    stack.pop_back ();
    if (lit && flags (lit).seen) {
      LOG ("skipping already justified");
      continue;
    }
    if (finished) {
      lrat_chain.push_back (reason->id);
      if (lit && reason) {
        Flags &f = flags (lit);
        f.seen = true;
        analyzed.push_back (lit); // CADICAL_assert (val (other) < 0);
        CADICAL_assert (flags (lit).seen);
      }
      continue;
    } else
      stack.push_back ({lit, reason, true});
    for (const auto &other : *reason) {
      if (other == lit)
        continue;
      Var &v = var (other);
      Flags &f = flags (other);
      if (f.seen)
        continue;
      if (!v.level) {
        const int64_t id = unit_id (-other);
        lrat_chain.push_back (id);
        f.seen = true;
        analyzed.push_back (other);
        continue;
      }
      if (v.reason) { // recursive justification
        LOG ("VIVIFY LRAT pushing %d", other);
        stack.push_back ({other, v.reason, false});
      }
    }
  }
  stack.clear ();
}

// calculate lrat_chain
//
inline void Internal::vivify_chain_for_units (int lit, Clause *reason) {
  if (!lrat)
    return;
  // LOG ("building chain for units");        bad line for debugging
  // equivalence if (opts.chrono && assignment_level (lit, reason)) return;
  if (level)
    return; // not decision level 0
  CADICAL_assert (lrat_chain.empty ());
  for (auto &reason_lit : *reason) {
    if (lit == reason_lit)
      continue;
    CADICAL_assert (val (reason_lit));
    const int signed_reason_lit = val (reason_lit) * reason_lit;
    int64_t id = unit_id (signed_reason_lit);
    lrat_chain.push_back (id);
  }
  lrat_chain.push_back (reason->id);
}

vivify_ref create_ref (Internal *internal, Clause *c) {
  LOG (c, "creating vivify_refs of clause");
  vivify_ref ref;
  ref.clause = c;
  ref.size = c->size;
  for (int i = 0; i < COUNTREF_COUNTS; ++i)
    ref.count[i] = 0;
  ref.vivify = c->vivify;
  int lits[COUNTREF_COUNTS] = {0};
  for (int i = 0; i != std::min (COUNTREF_COUNTS, c->size); ++i) {
    int best = 0;
    unsigned best_count = 0;
    for (auto lit : *c) {
      LOG ("to find best number of occurrences for literal %d, looking at "
           "literal %d",
           i, lit);
      for (int j = 0; j != i; ++j) {
        LOG ("comparing %d with literal %d", lit, lits[j]);
        if (lits[j] == lit)
          goto CONTINUE_WITH_NEXT_LITERAL;
      }
      {
        const int64_t lit_count = internal->noccs (lit);
        CADICAL_assert (lit_count);
        LOG ("checking literal %d with %zd occurrences", lit, lit_count);
        if (lit_count <= best_count)
          continue;
        best_count = lit_count;
        best = lit;
      }
    CONTINUE_WITH_NEXT_LITERAL:;
    }
    CADICAL_assert (best);
    CADICAL_assert (best_count);
    CADICAL_assert (best_count < UINT32_MAX);
    ref.count[i] =
        ((uint64_t) best_count << 32) + (uint64_t) internal->vlit (best);
    LOG ("final count at position %d is %d - %d: %lu", i, best, best_count,
         ref.count[i]);
    lits[i] = best;
  }
  return ref;
}
/*------------------------------------------------------------------------*/
inline void
Internal::vivify_prioritize_leftovers ([[maybe_unused]] char tag,
                                       size_t prioritized,
                                       std::vector<Clause *> &schedule) {
  if (prioritized) {
    PHASE ("vivify", stats.vivifications,
           "[phase %c] leftovers of %" PRId64 " clause", tag, prioritized);
  } else {
    PHASE ("vivify", stats.vivifications,
           "[phase %c] prioritizing all clause", tag);
    for (auto c : schedule)
      c->vivify = true;
  }
  const size_t max = opts.vivifyschedmax;
  if (schedule.size () > max) {
    if (prioritized) {
      std::partition (begin (schedule), end (schedule),
                      [] (Clause *c) { return c->vivify; });
    }
    schedule.resize (max);
  }
  // let's try to save a bit of memory
  shrink_vector (schedule);
}

void Internal::vivify_initialize (Vivifier &vivifier, int64_t &ticks) {

  const int tier1 = vivifier.tier1_limit;
  const int tier2 = vivifier.tier2_limit;
  // Count the number of occurrences of literals in all clauses,
  // particularly binary clauses, which are usually responsible
  // for most of the propagations.
  //
  init_noccs ();

  // Disconnect all watches since we sort literals within clauses.
  //
  CADICAL_assert (watching ());
#if 0
  clear_watches ();
#endif

  size_t prioritized_irred = 0, prioritized_tier1 = 0,
         prioritized_tier2 = 0, prioritized_tier3 = 0;
  for (const auto &c : clauses) {
    ++ticks;
    if (c->size == 2)
      continue; // see also (NO-BINARY) above
    if (!consider_to_vivify_clause (c))
      continue;

    // This computes an approximation of the Jeroslow Wang heuristic
    // score
    //
    //       nocc (L) =     sum       2^(12-|C|)
    //                   L in C in F
    //
    // but we cap the size at 12, that is all clauses of size 12 and
    // larger contribute '1' to the score, which allows us to use 'long'
    // numbers. See the example above (search for '@1').
    //
    const int shift = 12 - c->size;
    const int64_t score = shift < 1 ? 1 : (1l << shift); // @4
    for (const auto lit : *c) {
      noccs (lit) += score;
    }
    LOG (c, "putting clause in candidates");
    if (!c->redundant)
      vivifier.schedule_irred.push_back (c),
          prioritized_irred += (c->vivify);
    else if (c->glue <= tier1)
      vivifier.schedule_tier1.push_back (c),
          prioritized_tier1 += (c->vivify);
    else if (c->glue <= tier2)
      vivifier.schedule_tier2.push_back (c),
          prioritized_tier2 += (c->vivify);
    else
      vivifier.schedule_tier3.push_back (c),
          prioritized_tier3 += (c->vivify);
    ++ticks;
  }

  vivify_prioritize_leftovers ('x', prioritized_irred,
                               vivifier.schedule_irred);
  vivify_prioritize_leftovers ('u', prioritized_tier1,
                               vivifier.schedule_tier1);
  vivify_prioritize_leftovers ('v', prioritized_tier2,
                               vivifier.schedule_tier2);
  vivify_prioritize_leftovers ('w', prioritized_tier3,
                               vivifier.schedule_tier3);

  if (opts.vivifyflush) {
    clear_watches ();
    for (auto &sched : vivifier.schedules) {
      for (const auto &c : sched) {
        // Literals in scheduled clauses are sorted with their highest score
        // literals first (as explained above in the example at '@2').  This
        // is also needed in the prefix subsumption checking below. We do an
        // approximation below that is done only in the vivify_ref structure
        // below.
        //
        sort (c->begin (), c->end (), vivify_more_noccs (this));
      }
      // Flush clauses subsumed by another clause with the same prefix,
      // which also includes flushing syntactically identical clauses.
      //
      flush_vivification_schedule (sched, ticks);
    }
    connect_watches (); // watch all relevant clauses
  }
#if 0
  connect_watches (); // watch all relevant clauses
  vivify_propagate (ticks);
#endif
  vivify_propagate (ticks);
}

inline std::vector<vivify_ref> &current_refs_schedule (Vivifier &vivifier) {
  switch (vivifier.tier) {
  case Vivify_Mode::TIER1:
    return vivifier.refs_schedule_tier1;
    break;
  case Vivify_Mode::TIER2:
    return vivifier.refs_schedule_tier2;
    break;
  case Vivify_Mode::TIER3:
    return vivifier.refs_schedule_tier3;
    break;
  default:
    return vivifier.refs_schedule_irred;
    break;
  }
#if defined(WIN32) && !defined(__MINGW32__)
  __assume(false);
#else
  __builtin_unreachable ();
#endif
}

inline std::vector<Clause *> &current_schedule (Vivifier &vivifier) {
  switch (vivifier.tier) {
  case Vivify_Mode::TIER1:
    return vivifier.schedule_tier1;
    break;
  case Vivify_Mode::TIER2:
    return vivifier.schedule_tier2;
    break;
  case Vivify_Mode::TIER3:
    return vivifier.schedule_tier3;
    break;
  default:
    return vivifier.schedule_irred;
    break;
  }
#if defined(WIN32) && !defined(__MINGW32__)
  __assume(false);
#else
  __builtin_unreachable ();
#endif
}

struct vivify_refcount_rank {
  int offset;
  vivify_refcount_rank (int j) : offset (j) {
    CADICAL_assert (offset < COUNTREF_COUNTS);
  }
  typedef uint64_t Type;
  Type operator() (const vivify_ref &a) const { return a.count[offset]; }
};

struct vivify_refcount_smaller {
  int offset;
  vivify_refcount_smaller (int j) : offset (j) {
    CADICAL_assert (offset < COUNTREF_COUNTS);
  }
  bool operator() (const vivify_ref &a, const vivify_ref &b) const {
    const auto s = vivify_refcount_rank (offset) (a);
    const auto t = vivify_refcount_rank (offset) (b);
    return s < t;
  }
};

struct vivify_inversesize_rank {
  vivify_inversesize_rank () {}
  typedef uint64_t Type;
  Type operator() (const vivify_ref &a) const { return ~a.size; }
};

struct vivify_inversesize_smaller {
  vivify_inversesize_smaller () {}
  bool operator() (const vivify_ref &a, const vivify_ref &b) const {
    const auto s = vivify_inversesize_rank () (a);
    const auto t = vivify_inversesize_rank () (b);
    return s < t;
  }
};

/*------------------------------------------------------------------------*/
// There are two modes of vivification, one using all clauses and one
// focusing on irredundant clauses only.  The latter variant working on
// irredundant clauses only can also remove irredundant asymmetric
// tautologies (clauses subsumed through unit propagation), which in
// redundant mode is incorrect (due to propagating over redundant clauses).

void Internal::vivify_round (Vivifier &vivifier, int64_t ticks_limit) {

  if (unsat)
    return;
  if (terminated_asynchronously ())
    return;

  PHASE ("vivify", stats.vivifications,
         "starting %c vivification round ticks limit %" PRId64 "",
         vivifier.tag, ticks_limit);

  PHASE ("vivify", stats.vivifications,
         "starting %c vivification round ticks limit %" PRId64 "",
         vivifier.tag, ticks_limit);

  CADICAL_assert (watching ());

  auto &refs_schedule = current_refs_schedule (vivifier);
  auto &schedule = current_schedule (vivifier);

  int64_t ticks = 1 + schedule.size ();

  // Sort candidates, with first to be tried candidate clause last, i.e.,
  // many occurrences and high score literals) as in the example explained
  // above (search for '@3').
  //
  if (vivifier.tier != Vivify_Mode::IRREDUNDANT ||
      irredundant () / 10 < redundant ()) {
    // Literals in scheduled clauses are sorted with their highest score
    // literals first (as explained above in the example at '@2').  This is
    // also needed in the prefix subsumption checking below. We do an
    // approximation below that is done only in the vivify_ref structure
    // below.
    //

    // first build the schedule with vivifier_refs
    auto end_schedule = end (schedule);
    refs_schedule.resize (schedule.size ());
    std::transform (begin (schedule), end_schedule, begin (refs_schedule),
                    [&] (Clause *c) { return create_ref (this, c); });
    // now sort by size
    MSORT (opts.radixsortlim, refs_schedule.begin (), refs_schedule.end (),
           vivify_inversesize_rank (), vivify_inversesize_smaller ());
    // now (stable) sort by number of occurrences
    for (int i = 0; i < COUNTREF_COUNTS; ++i) {
      const int offset = COUNTREF_COUNTS - 1 - i;
      MSORT (opts.radixsortlim, refs_schedule.begin (),
             refs_schedule.end (), vivify_refcount_rank (offset),
             vivify_refcount_smaller (offset));
    }
    // force left-overs at the end
    std::stable_partition (begin (refs_schedule), end (refs_schedule),
                           [] (vivify_ref c) { return !c.vivify; });
    std::transform (begin (refs_schedule), end (refs_schedule),
                    begin (schedule),
                    [] (vivify_ref c) { return c.clause; });
    erase_vector (refs_schedule);
    LOG ("clause after sorting final:");
  } else {
    // skip sorting but still put clauses with the vivify tag at the end to
    // be done first Kissat does this implicitely by going twice over all
    // clauses
    std::stable_partition (begin (schedule), end (schedule),
                           [] (Clause *c) { return !c->vivify; });
  }

  // Remember old values of counters to summarize after each round with
  // verbose messages what happened in that round.
  //
  int64_t checked = stats.vivifychecks;
  int64_t subsumed = stats.vivifysubs;
  int64_t strengthened = stats.vivifystrs;
  int64_t units = stats.vivifyunits;

  int64_t scheduled = schedule.size ();
  stats.vivifysched += scheduled;

  PHASE ("vivify", stats.vivifications,
         "scheduled %" PRId64 " clauses to be vivified %.0f%%", scheduled,
         percent (scheduled, stats.current.irredundant));

  // Limit the number of propagations during vivification as in 'probe'.
  //
  const int64_t limit = ticks_limit - stats.ticks.vivify;
  CADICAL_assert (limit >= 0);

  // the clauses might still contain set literals, so propagation since the
  // beginning
  propagated2 = propagated = 0;

  if (!unsat && !propagate ()) {
    LOG ("propagation after connecting watches in inconsistency");
    learn_empty_clause ();
  }

  vivifier.ticks = ticks;
  int retry = 0;
  while (!unsat && !terminated_asynchronously () && !schedule.empty () &&
         vivifier.ticks < limit) {
    Clause *c = schedule.back (); // Next candidate.
    schedule.pop_back ();
    if (vivify_clause (vivifier, c) && !c->garbage && c->size > 2 &&
        retry < opts.vivifyretry) {
      ++retry;
      schedule.push_back (c);
    } else
      retry = 0;
  }

  if (level)
    backtrack_without_updating_phases ();

  if (!unsat) {
    int64_t still_need_to_be_vivified = schedule.size ();
#if 0
    // in the current round we have new_clauses_to_vivify @ leftovers from previous round There are
    // now two possibilities: (i) we consider all clauses as leftovers, or (ii) only the leftovers
    // from previous round are considered leftovers.
    //
    // CaDiCaL had the first version before. If
    // commented out we go to the second version.
    for (auto c : schedule)
      c->vivify = true;
#elif 1
    // if we have gone through all the leftovers, the current clauses are
    // leftovers for the next round
    if (!schedule.empty () && !schedule.front ()->vivify &&
        schedule.back ()->vivify)
      for (auto c : schedule)
        c->vivify = true;
#else
    // do nothing like in kissat and use the candidates for next time.
#endif
    // Preference clauses scheduled but not vivified yet next time.
    //
    if (still_need_to_be_vivified)
      PHASE ("vivify", stats.vivifications,
             "still need to vivify %" PRId64 " clauses %.02f%% of %" PRId64
             " scheduled",
             still_need_to_be_vivified,
             percent (still_need_to_be_vivified, scheduled), scheduled);
    else {
      PHASE ("vivify", stats.vivifications,
             "no previously not yet vivified clause left");
    }

    erase_vector (schedule); // Reclaim  memory early.
  }

  if (!unsat) {

    // Since redundant clause were disconnected during propagating vivified
    // units in redundant mode, and further irredundant clauses are
    // arbitrarily sorted, we have to propagate all literals again after
    // connecting the first two literals in the clauses, in order to
    // reestablish the watching invariant.
    //
    propagated2 = propagated = 0;

    if (!propagate ()) {
      LOG ("propagating vivified units leads to conflict");
      learn_empty_clause ();
    }
  }

  checked = stats.vivifychecks - checked;
  subsumed = stats.vivifysubs - subsumed;
  strengthened = stats.vivifystrs - strengthened;
  units = stats.vivifyunits - units;

  PHASE ("vivify", stats.vivifications,
         "checked %" PRId64 " clauses %.02f%% of %" PRId64
         " scheduled using %" PRIu64 " ticks",
         checked, percent (checked, scheduled), scheduled, vivifier.ticks);
  if (units)
    PHASE ("vivify", stats.vivifications,
           "found %" PRId64 " units %.02f%% of %" PRId64 " checked", units,
           percent (units, checked), checked);
  if (subsumed)
    PHASE ("vivify", stats.vivifications,
           "subsumed %" PRId64 " clauses %.02f%% of %" PRId64 " checked",
           subsumed, percent (subsumed, checked), checked);
  if (strengthened)
    PHASE ("vivify", stats.vivifications,
           "strengthened %" PRId64 " clauses %.02f%% of %" PRId64
           " checked",
           strengthened, percent (strengthened, checked), checked);

  stats.subsumed += subsumed;
  stats.strengthened += strengthened;
  stats.ticks.vivify += vivifier.ticks;

  bool unsuccessful = !(subsumed + strengthened + units);
  report (vivifier.tag, unsuccessful);
}

void set_vivifier_mode (Vivifier &vivifier, Vivify_Mode tier) {
  vivifier.tier = tier;
  switch (tier) {
  case Vivify_Mode::TIER1:
    vivifier.tag = 'u';
    break;
  case Vivify_Mode::TIER2:
    vivifier.tag = 'v';
    break;
  case Vivify_Mode::TIER3:
    vivifier.tag = 'w';
    break;
  default:
    CADICAL_assert (tier == Vivify_Mode::IRREDUNDANT);
    vivifier.tag = 'x';
    break;
  }
}
/*------------------------------------------------------------------------*/

void Internal::compute_tier_limits (Vivifier &vivifier) {
  if (!opts.vivifycalctier) {
    vivifier.tier1_limit = 2;
    vivifier.tier2_limit = 6;
    return;
  }
  vivifier.tier1_limit = tier1[false];
  vivifier.tier2_limit = tier2[false];
}

/*------------------------------------------------------------------------*/

bool Internal::vivify () {

  if (unsat)
    return false;
  if (terminated_asynchronously ())
    return false;
  if (!opts.vivify)
    return false;
  if (!stats.current.irredundant)
    return false;
  if (level)
    backtrack ();
  CADICAL_assert (opts.vivify);
  CADICAL_assert (!level);

  SET_EFFORT_LIMIT (totallimit, vivify, true);

  private_steps = true;

  START_SIMPLIFIER (vivify, VIVIFY);
  stats.vivifications++;

  // the effort is normalized by dividing by sumeffort below, hence no need
  // to multiply by 1e-3 (also making the precision better)
  double tier1effort = !opts.vivifytier1 ? 0 : (double) opts.vivifytier1eff;
  double tier2effort = !opts.vivifytier2 ? 0 : (double) opts.vivifytier2eff;
  double tier3effort = !opts.vivifytier3 ? 0 : (double) opts.vivifytier3eff;
  double irreffort =
      delaying_vivify_irredundant.bumpreasons.delay () || !opts.vivifyirred
          ? 0
          : (double) opts.vivifyirredeff;
  double sumeffort = tier1effort + tier2effort + tier3effort + irreffort;
  if (!stats.current.redundant)
    tier1effort = tier2effort = tier3effort = 0;
  if (!sumeffort)
    sumeffort = irreffort = 1;
  int64_t total = totallimit - stats.ticks.vivify;

  PHASE ("vivify", stats.vivifications,
         "vivification limit of %" PRId64 " ticks", total);
  Vivifier vivifier (Vivify_Mode::TIER1);
  compute_tier_limits (vivifier);

  if (vivifier.tier1_limit == vivifier.tier2_limit) {
    tier1effort += tier2effort;
    tier2effort = 0;
    LOG ("vivification tier1 matches tier2 "
         "thus using tier2 budget for tier1");
  }
  int64_t init_ticks = 0;

  // Refill the schedule every time.  Unchecked clauses are 'saved' by
  // setting their 'vivify' bit, such that they can be tried next time.
  //
  // TODO: count against ticks.vivify directly instead of this unholy
  // shifting.
  vivify_initialize (vivifier, init_ticks);
  stats.ticks.vivify += init_ticks;
  int64_t limit = stats.ticks.vivify;
  const double shared_effort = (double) init_ticks / 4.0;
  if (opts.vivifytier1) {
    set_vivifier_mode (vivifier, Vivify_Mode::TIER1);
    if (limit < stats.ticks.vivify)
      limit = stats.ticks.vivify;
    const double effort = (total * tier1effort) / sumeffort;
    CADICAL_assert (std::numeric_limits<int64_t>::max () - (int64_t) effort >=
            limit);
    limit += effort;
    if (limit - shared_effort > stats.ticks.vivify) {
      limit -= shared_effort;
      CADICAL_assert (limit >= 0);
      vivify_round (vivifier, limit);
    } else {
      LOG ("building the schedule already used our entire ticks budget for "
           "tier1");
    }
  }

  if (!unsat && tier2effort) {
    erase_vector (
        vivifier.schedule_tier1); // save memory (well, not really as we
                                  // already reached the peak memory)
    if (limit < stats.ticks.vivify)
      limit = stats.ticks.vivify;
    const double effort = (total * tier2effort) / sumeffort;
    CADICAL_assert (std::numeric_limits<int64_t>::max () - (int64_t) effort >=
            limit);
    limit += effort;
    if (limit - shared_effort > stats.ticks.vivify) {
      limit -= shared_effort;
      CADICAL_assert (limit >= 0);
      set_vivifier_mode (vivifier, Vivify_Mode::TIER2);
      vivify_round (vivifier, limit);
    } else {
      LOG ("building the schedule already used our entire ticks budget for "
           "tier2");
    }
  }

  if (!unsat && tier3effort) {
    erase_vector (vivifier.schedule_tier2);
    if (limit < stats.ticks.vivify)
      limit = stats.ticks.vivify;
    const double effort = (total * tier3effort) / sumeffort;
    CADICAL_assert (std::numeric_limits<int64_t>::max () - (int64_t) effort >=
            limit);
    limit += effort;
    if (limit - shared_effort > stats.ticks.vivify) {
      limit -= shared_effort;
      CADICAL_assert (limit >= 0);
      set_vivifier_mode (vivifier, Vivify_Mode::TIER3);
      vivify_round (vivifier, limit);
    } else {
      LOG ("building the schedule already used our entire ticks budget for "
           "tier3");
    }
  }

  if (!unsat && irreffort) {
    erase_vector (vivifier.schedule_tier3);
    if (limit < stats.ticks.vivify)
      limit = stats.ticks.vivify;
    const double effort = (total * irreffort) / sumeffort;
    CADICAL_assert (std::numeric_limits<int64_t>::max () - (int64_t) effort >=
            limit);
    limit += effort;
    if (limit - shared_effort > stats.ticks.vivify) {
      limit -= shared_effort;
      CADICAL_assert (limit >= 0);
      set_vivifier_mode (vivifier, Vivify_Mode::IRREDUNDANT);
      const int old = stats.vivifystrirr;
      const int old_tried = stats.vivifychecks;
      vivify_round (vivifier, limit);
      if (stats.vivifychecks - old_tried == 0 ||
          (float) (stats.vivifystrirr - old) /
                  (float) (stats.vivifychecks - old_tried) <
              0.01) {
        delaying_vivify_irredundant.bumpreasons.bump_delay ();
      } else {
        delaying_vivify_irredundant.bumpreasons.reduce_delay ();
      }
    } else {
      delaying_vivify_irredundant.bumpreasons.bump_delay ();
      LOG ("building the schedule already used our entire ticks budget for "
           "irredundant");
    }
  }

  reset_noccs ();
  STOP_SIMPLIFIER (vivify, VIVIFY);

  private_steps = false;

  return true;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
