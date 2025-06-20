#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// Failed literal probing uses its own propagation and assignment
// functions.  It further provides on-the-fly generation of hyper binary
// resolvents but only probes on roots of the binary implication graph.  The
// search for failed literals is limited, but untried roots are kept until
// the next time 'probe' is called.  Left over probes from the last attempt
// and new probes are tried until the limit is hit or all are tried.

/*------------------------------------------------------------------------*/

bool Internal::inprobing () {
  if (!opts.inprobing)
    return false;
  if (!preprocessing && !opts.inprocessing)
    return false;
  if (preprocessing)
    CADICAL_assert (lim.preprocessing);
  if (stats.inprobingphases && last.inprobe.reductions == stats.reductions)
    return false;
  return lim.inprobe <= stats.conflicts;
}

/*------------------------------------------------------------------------*/

inline int Internal::get_parent_reason_literal (int lit) {
  const int idx = vidx (lit);
  int res = parents[idx];
  if (lit < 0)
    res = -res;
  return res;
}

inline void Internal::set_parent_reason_literal (int lit, int reason) {
  const int idx = vidx (lit);
  if (lit < 0)
    reason = -reason;
  parents[idx] = reason;
}

/*-----------------------------------------------------------------------*/

// for opts.probehbr=false we need to do a lot of extra work to remember the
// correct lrat_chains... This solution is also memory intensive I think
// all corresponding functions are guarded to only work with the right
// options so they can be called without checking for options
//
// call locally after failed_literal or backtracking
//
void Internal::clean_probehbr_lrat () {
  if (!lrat || opts.probehbr)
    return;
  for (auto &field : probehbr_chains) {
    for (auto &chain : field) {
      chain.clear ();
    }
  }
}

// call globally before a probe round (or a lookahead round)
//
void Internal::init_probehbr_lrat () {
  if (!lrat || opts.probehbr)
    return;
  const size_t size = 2 * (1 + (size_t) max_var);
  probehbr_chains.resize (size);
  for (size_t i = 0; i < size; i++) {
    probehbr_chains[i].resize (size);
    // commented because not needed... should be empty already
    /*
    for (size_t j = 0; j < size; j++) {
      vector<int64_t> empty;
      probehbr_chains[i][j] = empty;
    }
    */
  }
}

// sets lrat_chain to the stored chain in probehbr_chains.
// this leads to conflict with unit reason uip
//
void Internal::get_probehbr_lrat (int lit, int uip) {
  if (!lrat || opts.probehbr)
    return;
  CADICAL_assert (lit);
  CADICAL_assert (lrat_chain.empty ());
  CADICAL_assert (val (uip) < 0);
  lrat_chain = probehbr_chains[vlit (lit)][vlit (uip)];
  int64_t id = unit_id (-uip);
  lrat_chain.push_back (id);
}

// sets the corresponding probehbr_chain to what is currently stored in
// lrat_chain. also clears lrat_chain.
//
void Internal::set_probehbr_lrat (int lit, int uip) {
  if (!lrat || opts.probehbr)
    return;
  CADICAL_assert (lit);
  CADICAL_assert (lrat_chain.size ());
  CADICAL_assert (probehbr_chains[vlit (lit)][vlit (uip)].empty ());
  probehbr_chains[vlit (lit)][vlit (uip)] = lrat_chain;
  lrat_chain.clear ();
}

// compute lrat_chain for the part of the tree from lit to dom
// use mini_chain because it needs to be reversed
//
void Internal::probe_dominator_lrat (int dom, Clause *reason) {
  if (!lrat || !dom)
    return;
  LOG (reason, "probe dominator LRAT for %d from", dom);
  for (const auto lit : *reason) {
    if (val (lit) >= 0)
      continue;
    const auto other = -lit;
    if (other == dom)
      continue;
    Flags &f = flags (other);
    if (f.seen)
      continue;
    f.seen = true;
    analyzed.push_back (other);
    Var u = var (other);
    if (u.level) {
      if (!u.reason) {
        LOG ("this may be a problem %d", other);
        continue;
      }
      probe_dominator_lrat (dom, u.reason);
      continue;
    }
    int64_t id = unit_id (other);
    lrat_chain.push_back (id);
  }
  lrat_chain.push_back (reason->id);
}

/*------------------------------------------------------------------------*/

// On-the-fly (dynamic) hyper binary resolution on decision level one can
// make use of the fact that the implication graph is actually a tree.

// Compute a dominator of two literals in the binary implication tree.

int Internal::probe_dominator (int a, int b) {
  require_mode (PROBE);
  int l = a, k = b;
  Var *u = &var (l), *v = &var (k);
  CADICAL_assert (val (l) > 0), CADICAL_assert (val (k) > 0);
  CADICAL_assert (u->level == 1), CADICAL_assert (v->level == 1);
  while (l != k) {
    if (u->trail > v->trail)
      swap (l, k), swap (u, v);
    if (!get_parent_reason_literal (l))
      return l;
    int parent = get_parent_reason_literal (k);
    CADICAL_assert (parent), CADICAL_assert (val (parent) > 0);
    v = &var (k = parent);
    CADICAL_assert (v->level == 1);
  }
  LOG ("dominator %d of %d and %d", l, a, b);
  CADICAL_assert (val (l) > 0);
  return l;
}

// The idea of dynamic on-the-fly hyper-binary resolution came up in the
// PrecoSAT solver, where it originally was used on all decision levels.

// It turned out, that most of the hyper-binary resolvents were generated
// during probing on decision level one anyhow.  Thus this version is
// specialized to decision level one, where actually all long (non-binary)
// forcing clauses can be resolved to become binary.  So if we find a clause
// which would force a new assignment at decision level one during probing
// we resolve it (the 'reason' argument) to obtain a hyper binary resolvent.
// It consists of the still unassigned literal (the new unit) and the
// negation of the unique closest dominator of the negation of all (false)
// literals in the clause (which has to exist on decision level one).

// There are two special cases which should be mentioned:
//
//   (A) The reason is already a binary clause in a certain sense, since all
//   its unwatched literals are root level fixed to false.  In this
//   situation it would be better to shrink the clause immediately instead
//   of adding a new clause consisting only of the watched literals.
//   However, this would happen during the next garbage collection anyhow.
//
//   (B) The resolvent subsumes the original reason clause. This is
//   equivalent to the property that the negated dominator is contained in
//   the original reason.  Again one could in principle shrink the clause.
//
// Note that (A) is actually subsumed by (B).  The possible optimization to
// shrink the clause on-the-fly is difficult (need to update 'blit' and
// 'binary' of the other watch at least) and also not really that important.
// For (B) we simply add the new binary resolvent and mark the old subsumed
// clause as garbage instead.  And since in the situation of (A) the
// shrinking will be performed at the next  garbage collection anyhow, we
// do not change clauses in (A).

// The hyper binary resolvent clause is redundant unless it subsumes the
// original reason and that one is irredundant.

// If the option 'opts.probehbr' is 'false', we actually do not add the new
// hyper binary resolvent, but simply pretend we would have added it and
// still return the dominator as new reason / parent for the new unit.

// Finally note that adding clauses changes the watches of the propagated
// literal and thus we can not use standard iterators during probing but
// need to fall back to indices.  One watch for the hyper binary resolvent
// clause is added at the end of the currently propagated watches, but its
// watch is a binary watch and will be skipped during propagating long
// clauses anyhow.

inline int Internal::hyper_binary_resolve (Clause *reason) {
  require_mode (PROBE);
  CADICAL_assert (level == 1);
  CADICAL_assert (reason->size > 2);
  const const_literal_iterator end = reason->end ();
  const int *lits = reason->literals;
  const_literal_iterator k;
#ifndef CADICAL_NDEBUG
  // First literal unassigned, all others false.
  CADICAL_assert (!val (lits[0]));
  for (k = lits + 1; k != end; k++)
    CADICAL_assert (val (*k) < 0);
  CADICAL_assert (var (lits[1]).level == 1);
#endif
  LOG (reason, "hyper binary resolving");
  stats.hbrs++;
  stats.hbrsizes += reason->size;
  const int lit = lits[1];
  int dom = -lit, non_root_level_literals = 0;
  for (k = lits + 2; k != end; k++) {
    const int other = -*k;
    CADICAL_assert (val (other) > 0);
    if (!var (other).level)
      continue;
    dom = probe_dominator (dom, other);
    non_root_level_literals++;
  }
  probe_reason = reason;
  if (non_root_level_literals && opts.probehbr) { // !(A)
    bool contained = false;
    for (k = lits + 1; !contained && k != end; k++)
      contained = (*k == -dom);
    const bool red = !contained || reason->redundant;
    if (red)
      stats.hbreds++;
    LOG ("new %s hyper binary resolvent %d %d",
         (red ? "redundant" : "irredundant"), -dom, lits[0]);
    CADICAL_assert (clause.empty ());
    clause.push_back (-dom);
    clause.push_back (lits[0]);
    probe_dominator_lrat (dom, reason);
    if (lrat)
      clear_analyzed_literals ();
    Clause *c = new_hyper_binary_resolved_clause (red, 2);
    probe_reason = c;
    if (red)
      c->hyper = true;
    clause.clear ();
    lrat_chain.clear ();
    if (contained) {
      stats.hbrsubs++;
      LOG (reason, "subsumed original");
      mark_garbage (reason);
    }
  } else if (non_root_level_literals && lrat) {
    // still calculate LRAT and remember for later
    CADICAL_assert (!opts.probehbr);
    probe_dominator_lrat (dom, reason);
    clear_analyzed_literals ();
    set_probehbr_lrat (dom, lits[0]);
  }
  return dom;
}

/*------------------------------------------------------------------------*/

// The following functions 'probe_assign' and 'probe_propagate' are used for
// propagating during failed literal probing in simplification mode, as
// replacement of the generic propagation routine 'propagate' and
// 'search_assign'.

// The code is mostly copied from 'propagate.cpp' and specialized.  We only
// comment on the differences.  More explanations are in 'propagate.cpp'.

inline void Internal::probe_assign (int lit, int parent) {
  require_mode (PROBE);
  int idx = vidx (lit);
  CADICAL_assert (!val (idx));
  CADICAL_assert (!flags (idx).eliminated () || !parent);
  CADICAL_assert (!parent || val (parent) > 0);
  Var &v = var (idx);
  v.level = level;
  v.trail = (int) trail.size ();
  CADICAL_assert ((int) num_assigned < max_var);
  num_assigned++;
  v.reason = level ? probe_reason : 0;
  probe_reason = 0;
  set_parent_reason_literal (lit, parent);
  if (!level)
    learn_unit_clause (lit);
  else
    CADICAL_assert (level == 1);
  const signed char tmp = sign (lit);
  set_val (idx, tmp);
  CADICAL_assert (val (lit) > 0);
  CADICAL_assert (val (-lit) < 0);
  trail.push_back (lit);

  // Do not save the current phase during inprocessing but remember the
  // number of units on the trail of the last time this literal was
  // assigned.  This allows us to avoid some redundant failed literal
  // probing attempts.  Search for 'propfixed' in 'probe.cpp' for details.
  //
  if (level)
    propfixed (lit) = stats.all.fixed;

  if (parent)
    LOG ("probe assign %d parent %d", lit, parent);
  else if (level)
    LOG ("probe assign %d probe", lit);
  else
    LOG ("probe assign %d negated failed literal UIP", lit);
}

void Internal::probe_assign_decision (int lit) {
  require_mode (PROBE);
  CADICAL_assert (!level);
  CADICAL_assert (propagated == trail.size ());
  level++;
  control.push_back (Level (lit, trail.size ()));
  probe_assign (lit, 0);
}

void Internal::probe_assign_unit (int lit) {
  require_mode (PROBE);
  CADICAL_assert (!level);
  CADICAL_assert (active (lit));
  probe_assign (lit, 0);
}

/*------------------------------------------------------------------------*/

// same as in propagate but inlined here
//
inline void Internal::probe_lrat_for_units (int lit) {
  if (!lrat)
    return;
  if (level)
    return; // not decision level 0
  LOG ("building chain for units");
  CADICAL_assert (lrat_chain.empty ());
  CADICAL_assert (probe_reason);
  for (auto &reason_lit : *probe_reason) {
    if (lit == reason_lit)
      continue;
    CADICAL_assert (val (reason_lit));
    if (!val (reason_lit))
      continue;
    const int signed_reason_lit = val (reason_lit) * reason_lit;
    int64_t id = unit_id (signed_reason_lit);
    lrat_chain.push_back (id);
  }
  lrat_chain.push_back (probe_reason->id);
}

/*------------------------------------------------------------------------*/

// This is essentially the same as 'propagate' except that we prioritize and
// always propagate binary clauses first (see our CPAIOR'13 paper on tree
// based look ahead), then immediately stop at a conflict and of course use
// 'probe_assign' instead of 'search_assign'.  The binary propagation part
// is factored out too.  If a new unit on decision level one is found we
// perform hyper binary resolution and thus actually build an implication
// tree instead of a DAG.  Statistics counters are also different.

inline void Internal::probe_propagate2 () {
  require_mode (PROBE);
  int64_t &ticks = stats.ticks.probe;
  while (propagated2 != trail.size ()) {
    const int lit = -trail[propagated2++];
    LOG ("probe propagating %d over binary clauses", -lit);
    Watches &ws = watches (lit);
    ticks += 1 + cache_lines (ws.size (), sizeof (const_watch_iterator *));
    for (const auto &w : ws) {
      if (!w.binary ())
        continue;
      const signed char b = val (w.blit);
      if (b > 0)
        continue;
      ticks++;
      if (b < 0)
        conflict = w.clause; // but continue
      else {
        CADICAL_assert (lrat_chain.empty ());
        CADICAL_assert (!probe_reason);
        probe_reason = w.clause;
        probe_lrat_for_units (w.blit);
        probe_assign (w.blit, -lit);
        lrat_chain.clear ();
      }
    }
  }
}

bool Internal::probe_propagate () {
  require_mode (PROBE);
  CADICAL_assert (!unsat);
  START (propagate);
  int64_t before = propagated2 = propagated;
  int64_t &ticks = stats.ticks.probe;
  while (!conflict) {
    if (propagated2 != trail.size ())
      probe_propagate2 ();
    else if (propagated != trail.size ()) {
      const int lit = -trail[propagated++];
      LOG ("probe propagating %d over large clauses", -lit);
      Watches &ws = watches (lit);
      ticks += 1 + cache_lines (ws.size (),
                                sizeof (sizeof (const_watch_iterator *)));
      size_t i = 0, j = 0;
      while (i != ws.size ()) {
        const Watch w = ws[j++] = ws[i++];
        if (w.binary ())
          continue;
        const signed char b = val (w.blit);
        if (b > 0)
          continue;
        ticks++;
        if (w.clause->garbage)
          continue;
        const literal_iterator lits = w.clause->begin ();
        const int other = lits[0] ^ lits[1] ^ lit;
        // lits[0] = other, lits[1] = lit;
        const signed char u = val (other);
        if (u > 0)
          ws[j - 1].blit = other;
        else {
          const int size = w.clause->size;
          const const_literal_iterator end = lits + size;
          const literal_iterator middle = lits + w.clause->pos;
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
          w.clause->pos = k - lits;
          CADICAL_assert (lits + 2 <= k), CADICAL_assert (k <= w.clause->end ());
          if (v > 0)
            ws[j - 1].blit = r;
          else if (!v) {
            ticks++;
            LOG (w.clause, "unwatch %d in", r);
            *k = lit;
            lits[0] = other;
            lits[1] = r;
            watch_literal (r, lit, w.clause);
            j--;
          } else if (!u) {
            ticks++;
            if (level == 1) {
              lits[0] = other, lits[1] = lit;
              CADICAL_assert (lrat_chain.empty ());
              CADICAL_assert (!probe_reason);
              int dom = hyper_binary_resolve (w.clause);
              probe_assign (other, dom);
            } else {
              ticks++;
              CADICAL_assert (lrat_chain.empty ());
              CADICAL_assert (!probe_reason);
              probe_reason = w.clause;
              probe_lrat_for_units (other);
              probe_assign_unit (other);
              lrat_chain.clear ();
            }
            probe_propagate2 ();
          } else
            conflict = w.clause;
        }
      }
      if (j != i) {
        while (i != ws.size ())
          ws[j++] = ws[i++];
        ws.resize (j);
      }
    } else
      break;
  }
  int64_t delta = propagated2 - before;
  stats.propagations.probe += delta;
  if (conflict)
    LOG (conflict, "conflict");
  STOP (propagate);
  return !conflict;
}

/*------------------------------------------------------------------------*/

// This a specialized instance of 'analyze'.

void Internal::failed_literal (int failed) {

  LOG ("analyzing failed literal probe %d", failed);
  stats.failed++;
  stats.probefailed++;

  CADICAL_assert (!unsat);
  CADICAL_assert (conflict);
  CADICAL_assert (level == 1);
  CADICAL_assert (analyzed.empty ());
  CADICAL_assert (lrat_chain.empty ());

  START (analyze);

  LOG (conflict, "analyzing failed literal conflict");

  int uip = 0;
  for (const auto &lit : *conflict) {
    const int other = -lit;
    if (!var (other).level) {
      CADICAL_assert (val (other) > 0);
      continue;
    }
    uip = uip ? probe_dominator (uip, other) : other;
  }
  probe_dominator_lrat (uip, conflict);
  if (lrat)
    clear_analyzed_literals ();

  LOG ("found probing UIP %d", uip);
  CADICAL_assert (uip);

  vector<int> work;

  int parent = uip;
  while (parent != failed) {
    const int next = get_parent_reason_literal (parent);
    parent = next;
    CADICAL_assert (parent);
    work.push_back (parent);
  }

  backtrack ();
  conflict = 0;

  CADICAL_assert (!val (uip));
  probe_assign_unit (-uip);
  lrat_chain.clear ();

  if (!probe_propagate ())
    learn_empty_clause ();

  size_t j = 0;
  while (!unsat && j < work.size ()) {
    // CADICAL_assert (!opts.probehbr);        CADICAL_assertion fails ...
    const int parent = work[j++];
    const signed char tmp = val (parent);
    if (tmp > 0) {
      CADICAL_assert (!opts.probehbr); // ... CADICAL_assertion should hold here
      get_probehbr_lrat (parent, uip);
      LOG ("clashing failed parent %d", parent);
      learn_empty_clause ();
    } else if (tmp == 0) {
      CADICAL_assert (!opts.probehbr); // ... and here
      LOG ("found unassigned failed parent %d", parent);
      get_probehbr_lrat (parent, uip); // this is computed during
      probe_assign_unit (-parent);     // propagation and can include
      lrat_chain.clear ();             // multiple chains where only one
      if (!probe_propagate ())
        learn_empty_clause (); // is needed!
    }
    uip = parent;
  }
  work.clear ();
  erase_vector (work);

  STOP (analyze);

  CADICAL_assert (unsat || val (failed) < 0);
}

/*------------------------------------------------------------------------*/

bool Internal::is_binary_clause (Clause *c, int &a, int &b) {
  CADICAL_assert (!level);
  if (c->garbage)
    return false;
  int first = 0, second = 0;
  for (const auto &lit : *c) {
    const signed char tmp = val (lit);
    if (tmp > 0)
      return false;
    if (tmp < 0)
      continue;
    if (second)
      return false;
    if (first)
      second = lit;
    else
      first = lit;
  }
  if (!second)
    return false;
  a = first, b = second;
  return true;
}

// We probe on literals first, which occur more often negated and thus we
// sort the 'probes' stack in such a way that literals which occur negated
// less frequently come first.  Probes are taken from the back of the stack.

struct probe_negated_noccs_rank {
  Internal *internal;
  probe_negated_noccs_rank (Internal *i) : internal (i) {}
  typedef size_t Type;
  Type operator() (int a) const { return internal->noccs (-a); }
};

// Fill the 'probes' schedule.

void Internal::generate_probes () {

  CADICAL_assert (probes.empty ());

  int64_t &ticks = stats.ticks.probe;

  // First determine all the literals which occur in binary clauses. It is
  // way faster to go over the clauses once, instead of walking the watch
  // lists for each literal.
  //
  init_noccs ();
  ticks += 1 + cache_lines (clauses.size (), sizeof (Clause *));
  for (const auto &c : clauses) {
    int a, b;
    ticks++;
    if (!is_binary_clause (c, a, b))
      continue;
    noccs (a)++;
    noccs (b)++;
  }

  for (auto idx : vars) {

    // Then focus on roots of the binary implication graph, which are
    // literals occurring negatively in a binary clause, but not positively.
    // If neither 'idx' nor '-idx' is a root it makes less sense to probe
    // this variable.

    // This argument requires that equivalent literal substitution through
    // 'decompose' is performed, because otherwise there might be 'cyclic
    // roots' which are not tried, i.e., -1 2 0, 1 -2 0, 1 2 3 0, 1 2 -3 0.

    ticks += 2;

    const bool have_pos_bin_occs = noccs (idx) > 0;
    const bool have_neg_bin_occs = noccs (-idx) > 0;

    if (have_pos_bin_occs == have_neg_bin_occs)
      continue;

    int probe = have_neg_bin_occs ? idx : -idx;

    // See the discussion where 'propfixed' is used below.
    //
    if (propfixed (probe) >= stats.all.fixed)
      continue;

    LOG ("scheduling probe %d negated occs %" PRId64 "", probe,
         noccs (-probe));
    probes.push_back (probe);
  }

  rsort (probes.begin (), probes.end (), probe_negated_noccs_rank (this));

  reset_noccs ();
  shrink_vector (probes);

  PHASE ("probe-round", stats.probingrounds,
         "scheduled %zd literals %.0f%%", probes.size (),
         percent (probes.size (), 2u * max_var));
}

// Follow the ideas in 'generate_probes' but flush non root probes and
// reorder remaining probes.

void Internal::flush_probes () {

  CADICAL_assert (!probes.empty ());
  int64_t &ticks = stats.ticks.probe;

  init_noccs ();
  ticks += 1 + cache_lines (clauses.size (), sizeof (Clause *));
  for (const auto &c : clauses) {
    int a, b;
    ticks++;
    if (!is_binary_clause (c, a, b))
      continue;
    noccs (a)++;
    noccs (b)++;
  }

  const auto eop = probes.end ();
  auto j = probes.begin ();
  for (auto i = j; i != eop; i++) {
    int lit = *i;
    if (!active (lit))
      continue;
    ticks += 2;
    const bool have_pos_bin_occs = noccs (lit) > 0;
    const bool have_neg_bin_occs = noccs (-lit) > 0;
    if (have_pos_bin_occs == have_neg_bin_occs)
      continue;
    if (have_pos_bin_occs)
      lit = -lit;
    CADICAL_assert (!noccs (lit)), CADICAL_assert (noccs (-lit) > 0);
    if (propfixed (lit) >= stats.all.fixed)
      continue;
    LOG ("keeping probe %d negated occs %" PRId64 "", lit, noccs (-lit));
    *j++ = lit;
  }
  size_t remain = j - probes.begin ();
#ifndef CADICAL_QUIET
  size_t flushed = probes.size () - remain;
#endif
  probes.resize (remain);

  rsort (probes.begin (), probes.end (), probe_negated_noccs_rank (this));

  reset_noccs ();
  shrink_vector (probes);

  PHASE ("probe-round", stats.probingrounds,
         "flushed %zd literals %.0f%% remaining %zd", flushed,
         percent (flushed, remain + flushed), remain);
}

int Internal::next_probe () {

  int generated = 0;

  for (;;) {

    if (probes.empty ()) {
      if (generated++)
        return 0;
      generate_probes ();
    }

    while (!probes.empty ()) {

      int probe = probes.back ();
      probes.pop_back ();

      // Eliminated or assigned.
      //
      if (!active (probe))
        continue;

      // There is now new unit since the last time we propagated this probe,
      // thus we propagated it before without obtaining a conflict and
      // nothing changed since then.  Thus there is no need to propagate it
      // again.  This observation was independently made by Partik Simons
      // et.al. in the context of implementing 'smodels' (see for instance
      // Alg. 4 in his JAIR article from 2002) and it has also been
      // contributed to the thesis work of Yacine Boufkhad.
      //
      if (propfixed (probe) >= stats.all.fixed)
        continue;

      return probe;
    }
  }
}

bool Internal::probe () {

  if (!opts.probe)
    return false;
  if (unsat)
    return false;
  if (terminated_asynchronously ())
    return false;

  SET_EFFORT_LIMIT (limit, probe, true);

  START_SIMPLIFIER (probe, PROBE);
  stats.probingrounds++;

  // Probing is limited in terms of non-probing propagations
  // 'stats.propagations'. We allow a certain percentage 'opts.probeeffort'
  // (say %5) of probing propagations in each probing with a lower bound of
  // 'opts.probmineff'.
  //

  PHASE ("probe-round", stats.probingrounds,
         "probing limit of %" PRId64 " propagations ", limit);

  int old_failed = stats.failed;
#ifndef CADICAL_QUIET
  int64_t old_probed = stats.probed;
#endif
  int64_t old_hbrs = stats.hbrs;

  if (!probes.empty ())
    flush_probes ();

  // We reset 'propfixed' since there was at least another conflict thus
  // a new learned clause, which might produce new propagations (and hyper
  // binary resolvents).  During 'generate_probes' we keep the old value.
  //
  for (auto idx : vars)
    propfixed (idx) = propfixed (-idx) = -1;

  CADICAL_assert (unsat || propagated == trail.size ());
  propagated = propagated2 = trail.size ();

  int probe;
  init_probehbr_lrat ();
  while (!unsat && !terminated_asynchronously () &&
         stats.ticks.probe < limit && (probe = next_probe ())) {
    stats.probed++;
    LOG ("probing %d", probe);
    probe_assign_decision (probe);
    if (probe_propagate ())
      backtrack ();
    else
      failed_literal (probe);
    clean_probehbr_lrat ();
  }

  if (unsat)
    LOG ("probing derived empty clause");
  else if (propagated < trail.size ()) {
    LOG ("probing produced %zd units",
         (size_t) (trail.size () - propagated));
    if (!propagate ()) {
      LOG ("propagating units after probing results in empty clause");
      learn_empty_clause ();
    } else
      sort_watches ();
  }

  int failed = stats.failed - old_failed;
#ifndef CADICAL_QUIET
  int64_t probed = stats.probed - old_probed;
#endif
  int64_t hbrs = stats.hbrs - old_hbrs;

  PHASE ("probe-round", stats.probingrounds,
         "probed %" PRId64 " and found %d failed literals", probed, failed);

  if (hbrs)
    PHASE ("probe-round", stats.probingrounds,
           "found %" PRId64 " hyper binary resolvents", hbrs);

  STOP_SIMPLIFIER (probe, PROBE);

  report ('p', !opts.reportall && !(unsat + failed + hbrs));

  return !unsat && failed;
}

/*------------------------------------------------------------------------*/

// This schedules a number of inprocessing techniques.
// These range from very cheap and beneficial (decompose) to
// more expensive and sometimes less beneficial. We want to limit
// expensive techniques to some fraction of total time or search time.
// this is done using 'ticks'.
// Generally, there are options for each of the techniques to set the
// efficiency, i.e., the fraction of ticks they are allowed as budget.
// Whenever e.g. vivify is called, the budget is calculated from the
// search ticks that have passed since the last vivify round and this
// efficiency.
// We want to be able to run inprocessing frequently, without it dominating
// runtimes. This entire inprocessing scheme is scheduled after a certain
// amount of conflicts were found, the gap between two inprocessing rounds
// increasing by a constant number each time. In effect, the number of
// inprocessing rounds is allways the square root of the number of conflicts
// with some constant factor.
// This factor can also be with the option 'inprobeint'
// Some of the techniques are not run always, for different reasons.
// 'factor' or BVA depends on certain structures of the irredundant clauses
// and as such will only be run when new irredundant clauses are derived or
// it was not able to finish with the entire search space.
// 'sweeping' is especially usefull on certain classes of formulas, and uses
// a increasing or decreasing delay that depends on how usefull it was.
// In cases where it is less usefull, we obviously want to reset the budged,
// even if the routine was delayed.
// Additionally 'vivify', 'sweep' and 'factor' can also have a big initial
// overhead in setting up the datastructures. This has to be accounted for
// with the 'ticks', however, since inprocessing is done frequently, this
// overhead is too expensive to pay. So instead, we accumulate the budget
// of 'ticks' and delay the technique until it passes a certain threshhold,
// which depends on the the cost of initialization. Note that in the case of
// sweeping, we have two different delays, one which resets the budged, and
// one which passes it to the next round. In this case the former takes
// precendent, until we would run sweeping once, at which point the focus
// switches to the latter delay until the budget is big enough, such that
// sweeping can be run. Then we switch back to the other delay.

void CaDiCaL::Internal::inprobe (bool update_limits) {

  if (unsat)
    return;
  if (level)
    backtrack ();
  if (!propagate ()) {
    learn_empty_clause ();
    return;
  }

  stats.inprobingphases++;
  if (external_prop) {
    CADICAL_assert (!level);
    private_steps = true;
  }
  const int before = active ();
  const int before_extended = stats.variables_extension;

  // schedule of inprobing techniques.
  //
  {
    mark_duplicated_binary_clauses_as_garbage ();
    decompose ();
    if (ternary ())
      decompose (); // If we derived a binary clause
    if (probe ())
      decompose ();

    if (extract_gates ())
      decompose ();
    if (sweep ())     // full occurrence list
      decompose ();   // ... and (ELS) afterwards.
    (void) vivify (); // resets watches
    transred ();      // builds big.
    factor ();   // resets watches, partial occurrence list
  }

  if (external_prop) {
    CADICAL_assert (!level);
    private_steps = false;
  }

  if (!update_limits)
    return;

  const int after = active ();
  const int after_extended = stats.variables_extension;
  const int diff_extended = after_extended - before_extended;
  CADICAL_assert (diff_extended >= 0);
  const int removed = before - after + diff_extended;
  CADICAL_assert (removed >= 0);

  if (removed) {
    stats.inprobesuccess++;
    PHASE ("probe-phase", stats.inprobingphases,
           "successfully removed %d active variables %.0f%%", removed,
           percent (removed, before));
  } else
    PHASE ("probe-phase", stats.inprobingphases,
           "could not remove any active variable");

  const int64_t delta =
      25 * opts.inprobeint * log10 (stats.inprobingphases + 9);
  lim.inprobe = stats.conflicts + delta;

  PHASE ("probe-phase", stats.inprobingphases,
         "new limit at %" PRId64 " conflicts after %" PRId64 " conflicts",
         lim.inprobe, delta);

  last.inprobe.reductions = stats.reductions;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
