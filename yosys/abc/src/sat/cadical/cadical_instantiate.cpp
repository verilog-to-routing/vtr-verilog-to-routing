#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// This provides an implementation of variable instantiation, a technique
// for removing literals with few occurrence (see also 'instantiate.hpp').

/*------------------------------------------------------------------------*/

// Triggered at the end of a variable elimination round ('elim_round').

void Internal::collect_instantiation_candidates (
    Instantiator &instantiator) {
  CADICAL_assert (occurring ());
  for (auto idx : vars) {
    if (frozen (idx))
      continue;
    if (!active (idx))
      continue;
    if (flags (idx).elim)
      continue; // BVE attempt pending
    for (int sign = -1; sign <= 1; sign += 2) {
      const int lit = sign * idx;
      if (noccs (lit) > opts.instantiateocclim)
        continue;
      Occs &os = occs (lit);
      for (const auto &c : os) {
        if (c->garbage)
          continue;
        if (opts.instantiateonce && c->instantiated)
          continue;
        if (c->size < opts.instantiateclslim)
          continue;
        bool satisfied = false;
        int unassigned = 0;
        for (const auto &other : *c) {
          const signed char tmp = val (other);
          if (tmp > 0)
            satisfied = true;
          if (!tmp)
            unassigned++;
        }
        if (satisfied)
          continue;
        if (unassigned < 3)
          continue; // avoid learning units
        size_t negoccs = occs (-lit).size ();
        LOG (c,
             "instantiation candidate literal %d "
             "with %zu negative occurrences in",
             lit, negoccs);
        instantiator.candidate (lit, c, c->size, negoccs);
      }
    }
  }
}

/*------------------------------------------------------------------------*/

// Specialized propagation and assignment routines for instantiation.

inline void Internal::inst_assign (int lit) {
  LOG ("instantiate assign %d", lit);
  CADICAL_assert (!val (lit));
  CADICAL_assert ((int) num_assigned < max_var);
  num_assigned++;
  set_val (lit, 1);
  trail.push_back (lit);
}

// Conflict analysis is only needed to do valid resolution proofs.
// We remember propagated clauses in order of assignment (in inst_chain)
// which allows us to do a variant of conflict analysis if the instantiation
// attempt succeeds.
//
bool Internal::inst_propagate () { // Adapted from 'propagate'.
  START (propagate);
  int64_t before = propagated;
  bool ok = true;
  while (ok && propagated != trail.size ()) {
    const int lit = -trail[propagated++];
    LOG ("instantiate propagating %d", -lit);
    Watches &ws = watches (lit);
    const const_watch_iterator eow = ws.end ();
    const_watch_iterator i = ws.begin ();
    watch_iterator j = ws.begin ();
    while (i != eow) {
      const Watch w = *j++ = *i++;
      const signed char b = val (w.blit);
      if (b > 0)
        continue;
      if (w.binary ()) {
        if (b < 0) {
          ok = false;
          LOG (w.clause, "conflict");
          if (lrat) {
            inst_chain.push_back (w.clause);
          }
          break;
        } else {
          if (lrat) {
            inst_chain.push_back (w.clause);
          }
          inst_assign (w.blit);
        }
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
          if (v > 0) {
            j[-1].blit = r;
          } else if (!v) {
            LOG (w.clause, "unwatch %d in", r);
            lits[1] = r;
            *k = lit;
            watch_literal (r, lit, w.clause);
            j--;
          } else if (!u) {
            CADICAL_assert (v < 0);
            if (lrat) {
              inst_chain.push_back (w.clause);
            }
            inst_assign (other);
          } else {
            CADICAL_assert (u < 0);
            CADICAL_assert (v < 0);
            if (lrat) {
              inst_chain.push_back (w.clause);
            }
            LOG (w.clause, "conflict");
            ok = false;
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
  int64_t delta = propagated - before;
  stats.propagations.instantiate += delta;
  STOP (propagate);
  return ok;
}

/*------------------------------------------------------------------------*/

// This is the instantiation attempt.

bool Internal::instantiate_candidate (int lit, Clause *c) {
  stats.instried++;
  if (c->garbage)
    return false;
  CADICAL_assert (!level);
  bool found = false, satisfied = false, inactive = false;
  int unassigned = 0;
  for (const auto &other : *c) {
    if (other == lit)
      found = true;
    const signed char tmp = val (other);
    if (tmp > 0) {
      satisfied = true;
      break;
    }
    if (!tmp && !active (other)) {
      inactive = true;
      break;
    }
    if (!tmp)
      unassigned++;
  }
  if (!found)
    return false;
  if (inactive)
    return false;
  if (satisfied)
    return false;
  if (unassigned < 3)
    return false;
  size_t before = trail.size ();
  CADICAL_assert (propagated == before);
  CADICAL_assert (active (lit));
  CADICAL_assert (inst_chain.empty ());
  LOG (c, "trying to instantiate %d in", lit);
  CADICAL_assert (!c->garbage);
  c->instantiated = true;
  CADICAL_assert (lrat_chain.empty ());
  level++;
  inst_assign (lit); // Assume 'lit' to true.
  for (const auto &other : *c) {
    if (other == lit)
      continue;
    const signed char tmp = val (other);
    if (tmp) {
      CADICAL_assert (tmp < 0);
      continue;
    }
    inst_assign (-other); // Assume other to false.
  }
  bool ok = inst_propagate ();  // Propagate.
  CADICAL_assert (lrat_chain.empty ()); // chain will be built here
  if (ok) {
    inst_chain.clear ();
  } else if (lrat) { // analyze conflict for lrat
    CADICAL_assert (inst_chain.size ());
    Clause *reason = inst_chain.back ();
    inst_chain.pop_back ();
    lrat_chain.push_back (reason->id);
    for (const auto &other : *reason) {
      Flags &f = flags (other);
      CADICAL_assert (!f.seen);
      f.seen = true;
      analyzed.push_back (other);
    }
  }
  while (trail.size () > before) { // Backtrack.
    const int other = trail.back ();
    LOG ("instantiate unassign %d", other);
    trail.pop_back ();
    CADICAL_assert (val (other) > 0);
    num_assigned--;
    set_val (other, 0);
    // this is a variant of conflict analysis which is only needed for lrat
    if (!ok && inst_chain.size () && lrat) {
      Flags &f = flags (other);
      if (f.seen) {
        Clause *reason = inst_chain.back ();
        lrat_chain.push_back (reason->id);
        for (const auto &other : *reason) {
          Flags &f = flags (other);
          if (f.seen)
            continue;
          f.seen = true;
          analyzed.push_back (other);
        }
        f.seen = false;
      }
      inst_chain.pop_back ();
    }
  }
  CADICAL_assert (inst_chain.empty ());
  // post processing step for lrat
  if (!ok && lrat) {
    if (flags (lit).seen)
      lrat_chain.push_back (c->id);
    for (const auto &other : *c) {
      Flags &f = flags (other);
      f.seen = false;
    }
    for (int other : analyzed) {
      Flags &f = flags (other);
      if (!f.seen) {
        f.seen = true;
        continue;
      }
      int64_t id = unit_id (-other);
      lrat_chain.push_back (id);
    }
    clear_analyzed_literals ();
    reverse (lrat_chain.begin (), lrat_chain.end ());
  }
  CADICAL_assert (analyzed.empty ());
  propagated = before;
  CADICAL_assert (level == 1);
  level = 0;
  if (ok) {
    CADICAL_assert (lrat_chain.empty ());
    LOG ("instantiation failed");
    return false;
  }
  unwatch_clause (c);
  LOG (lrat_chain, "instantiate proof chain");
  strengthen_clause (c, lit);
  watch_clause (c);
  lrat_chain.clear ();
  CADICAL_assert (c->size > 1);
  LOG ("instantiation succeeded");
  stats.instantiated++;
  return true;
}

/*------------------------------------------------------------------------*/

// Try to instantiate all candidates collected before through the
// 'collect_instantiation_candidates' routine.

void Internal::instantiate (Instantiator &instantiator) {
  CADICAL_assert (opts.instantiate);
  START (instantiate);
  stats.instrounds++;
#ifndef CADICAL_QUIET
  const int64_t candidates = instantiator.candidates.size ();
  int64_t tried = 0;
#endif
  int64_t instantiated = 0;
  init_watches ();
  connect_watches ();
  if (propagated < trail.size ()) {
    if (!propagate ()) {
      LOG ("propagation after connecting watches failed");
      learn_empty_clause ();
      CADICAL_assert (unsat);
    }
  }
  PHASE ("instantiate", stats.instrounds,
         "attempting to instantiate %" PRId64
         " candidate literal clause pairs",
         candidates);
  while (!unsat && !terminated_asynchronously () &&
         !instantiator.candidates.empty ()) {
    Instantiator::Candidate cand = instantiator.candidates.back ();
    instantiator.candidates.pop_back ();
#ifndef CADICAL_QUIET
    tried++;
#endif
    if (!active (cand.lit))
      continue;
    LOG (cand.clause,
         "trying to instantiate %d with "
         "%zd negative occurrences in",
         cand.lit, cand.negoccs);
    if (!instantiate_candidate (cand.lit, cand.clause))
      continue;
    instantiated++;
    VERBOSE (2,
             "instantiation %" PRId64 " (%.1f%%) succeeded "
             "(%.1f%%) with %zd negative occurrences in size %d clause",
             tried, percent (tried, candidates),
             percent (instantiated, tried), cand.negoccs, cand.size);
  }
  PHASE ("instantiate", stats.instrounds,
         "instantiated %" PRId64 " candidate successfully "
         "out of %" PRId64 " tried %.1f%%",
         instantiated, tried, percent (instantiated, tried));
  report ('I', !instantiated);
  reset_watches ();
  STOP (instantiate);
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
