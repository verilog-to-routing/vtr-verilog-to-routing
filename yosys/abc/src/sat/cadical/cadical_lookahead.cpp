#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

struct literal_occ {
  int lit;
  int count;
  bool operator< (const literal_occ &locc) const {
    return (count > locc.count) || (count == locc.count && lit < locc.lit);
  }
  literal_occ operator++ () {
    ++count;
    return *this;
  }
};

std::vector<int> Internal::lookahead_populate_locc () {
  std::vector<literal_occ> loccs ((std::size_t) max_var + 1);
  for (std::size_t lit = 0; lit < loccs.size (); ++lit) {
    loccs[lit].lit = lit;
  }
  for (const auto &c : clauses)
    if (!c->redundant)
      for (const auto &lit : *c)
        if (active (lit))
          ++loccs[std::abs (lit)];
  std::sort (begin (loccs), end (loccs));
  std::vector<int> locc_map;
  locc_map.reserve (max_var);
  for (const auto &locc : loccs)
    locc_map.push_back (locc.lit);
  return locc_map;
}

int Internal::lookahead_locc (const std::vector<int> &loccs) {
  for (auto lit : loccs)
    if (active (abs (lit)) && !assumed (lit) && !assumed (-lit) &&
        !val (lit))
      return lit;
  return 0;
}

// This calculates the literal that appears the most often reusing the
// available datastructures and iterating over the clause set. This is too
// slow to be called iteratively. A faster (but inexact) version is
// lookahead_populate_loc and lookahead_loc.
int Internal::most_occurring_literal () {
  init_noccs ();
  for (const auto &c : clauses)
    if (!c->redundant)
      for (const auto &lit : *c)
        if (active (lit))
          noccs (lit)++;
  int64_t max_noccs = 0;
  int res = 0;

  if (unsat)
    return INT_MIN;

  propagate ();
  for (int idx = 1; idx <= max_var; idx++) {
    if (!active (idx) || assumed (idx) || assumed (-idx) || val (idx))
      continue;
    for (int sign = -1; sign <= 1; sign += 2) {
      const int lit = sign * idx;
      if (!active (lit))
        continue;
      int64_t tmp = noccs (lit);
      if (tmp <= max_noccs)
        continue;
      max_noccs = tmp;
      res = lit;
    }
  }
  MSG ("maximum occurrence %" PRId64 " of literal %d", max_noccs, res);
  reset_noccs ();
  return res;
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

// Follow the ideas in 'generate_probes' but flush non root probes and
// reorder remaining probes.

void Internal::lookahead_flush_probes () {

  CADICAL_assert (!probes.empty ());

  init_noccs ();
  for (const auto &c : clauses) {
    int a, b;
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
    const bool have_pos_bin_occs = noccs (lit) > 0;
    const bool have_neg_bin_occs = noccs (-lit) > 0;
    if (have_pos_bin_occs == have_neg_bin_occs)
      continue;
    if (have_pos_bin_occs)
      lit = -lit;
    CADICAL_assert (!noccs (lit)), CADICAL_assert (noccs (-lit) > 0);
    if (propfixed (lit) >= stats.all.fixed)
      continue;
    MSG ("keeping probe %d negated occs %" PRId64 "", lit, noccs (-lit));
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

void Internal::lookahead_generate_probes () {

  CADICAL_assert (probes.empty ());

  // First determine all the literals which occur in binary clauses. It is
  // way faster to go over the clauses once, instead of walking the watch
  // lists for each literal.
  //
  init_noccs ();
  for (const auto &c : clauses) {
    int a, b;
    if (!is_binary_clause (c, a, b))
      continue;
    noccs (a)++;
    noccs (b)++;
  }

  for (int idx = 1; idx <= max_var; idx++) {

    // Then focus on roots of the binary implication graph, which are
    // literals occurring negatively in a binary clause, but not positively.
    // If neither 'idx' nor '-idx' is a root it makes less sense to probe
    // this variable.

    // This argument requires that equivalent literal substitution through
    // 'decompose' is performed, because otherwise there might be 'cyclic
    // roots' which are not tried, i.e., -1 2 0, 1 -2 0, 1 2 3 0, 1 2 -3 0.

    const bool have_pos_bin_occs = noccs (idx) > 0;
    const bool have_neg_bin_occs = noccs (-idx) > 0;

    // if (have_pos_bin_occs == have_neg_bin_occs) continue;

    if (have_pos_bin_occs) {
      int probe = -idx;

      // See the discussion where 'propfixed' is used below.
      //
      if (propfixed (probe) >= stats.all.fixed)
        continue;

      MSG ("scheduling probe %d negated occs %" PRId64 "", probe,
           noccs (-probe));
      probes.push_back (probe);
    }

    if (have_neg_bin_occs) {
      int probe = idx;

      // See the discussion where 'propfixed' is used below.
      //
      if (propfixed (probe) >= stats.all.fixed)
        continue;

      MSG ("scheduling probe %d negated occs %" PRId64 "", probe,
           noccs (-probe));
      probes.push_back (probe);
    }
  }

  rsort (probes.begin (), probes.end (), probe_negated_noccs_rank (this));

  reset_noccs ();
  shrink_vector (probes);

  PHASE ("probe-round", stats.probingrounds,
         "scheduled %zd literals %.0f%%", probes.size (),
         percent (probes.size (), 2 * max_var));
}

int Internal::lookahead_next_probe () {

  int generated = 0;

  for (;;) {

    if (probes.empty ()) {
      if (generated++)
        return 0;
      lookahead_generate_probes ();
    }

    while (!probes.empty ()) {

      int probe = probes.back ();
      probes.pop_back ();

      // Eliminated or assigned.
      //
      if (!active (probe) || assumed (probe) || assumed (-probe))
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

bool non_tautological_cube (std::vector<int> cube) {
  std::sort (begin (cube), end (cube), clause_lit_less_than ());

  for (size_t i = 0, j = 1; j < cube.size (); ++i, ++j)
    if (cube[i] == cube[j])
      return false;
    else if (cube[i] == -cube[j])
      return false;
    else if (cube[i] == 0)
      return false;

  return true;
}

bool Internal::terminating_asked () {

  if (external->terminator && external->terminator->terminate ()) {
    MSG ("connected terminator forces termination");
    return true;
  }

  if (termination_forced) {
    MSG ("termination forced");
    return true;
  }
  return false;
}

// We run probing on all literals with some differences:
//
// * no limit on the number of propagations. We rely on terminating to
// stop()
// * we run only one round
//
// The run can be expensive, so we actually first run the cheaper
// occurrence version and only then run lookahead.
//
int Internal::lookahead_probing () {

  if (!active ())
    return 0;

  MSG ("lookahead-probe-round %" PRId64
       " without propagations limit and %zu assumptions",
       stats.probingrounds, assumptions.size ());

  termination_forced = false;

#ifndef CADICAL_QUIET
  int old_failed = stats.failed;
  int64_t old_probed = stats.probed;
#endif
  int64_t old_hbrs = stats.hbrs;

  if (unsat)
    return INT_MIN;
  if (level)
    backtrack ();
  if (!propagate ()) {
    MSG ("empty clause before probing");
    learn_empty_clause ();
    return INT_MIN;
  }

  if (terminating_asked ())
    return most_occurring_literal ();

  decompose ();

  if (ternary ()) // If we derived a binary clause
    decompose (); // then start another round of ELS.

  // Remove duplicated binary clauses and perform in essence hyper unary
  // resolution, i.e., derive the unit '2' from '1 2' and '-1 2'.
  //
  mark_duplicated_binary_clauses_as_garbage ();

  lim.conflicts = -1;

  if (!probes.empty ())
    lookahead_flush_probes ();

  // We reset 'propfixed' since there was at least another conflict thus
  // a new learned clause, which might produce new propagations (and hyper
  // binary resolvents).  During 'generate_probes' we keep the old value.
  //
  for (int idx = 1; idx <= max_var; idx++)
    propfixed (idx) = propfixed (-idx) = -1;

  CADICAL_assert (unsat || propagated == trail.size ());
  propagated = propagated2 = trail.size ();

  int probe;
  int res = most_occurring_literal ();
  int max_hbrs = -1;

  set_mode (PROBE);

  MSG ("unsat = %d, terminating_asked () = %d ", unsat,
       terminating_asked ());
  init_probehbr_lrat ();
  while (!unsat && !terminating_asked () &&
         (probe = lookahead_next_probe ())) {
    stats.probed++;
    int hbrs;

    probe_assign_decision (probe);
    if (probe_propagate ())
      hbrs = trail.size (), backtrack ();
    else
      hbrs = 0, failed_literal (probe);
    clean_probehbr_lrat ();
    if (max_hbrs < hbrs ||
        (max_hbrs == hbrs &&
         internal->bumped (probe) > internal->bumped (res))) {
      res = probe;
      max_hbrs = hbrs;
    }
  }

  reset_mode (PROBE);

  if (unsat) {
    MSG ("probing derived empty clause");
    res = INT_MIN;
  } else if (propagated < trail.size ()) {
    MSG ("probing produced %zd units",
         (size_t) (trail.size () - propagated));
    if (!propagate ()) {
      MSG ("propagating units after probing results in empty clause");
      learn_empty_clause ();
      res = INT_MIN;
    } else
      sort_watches ();
  }

#ifndef CADICAL_QUIET
  int failed = stats.failed - old_failed;
  int64_t probed = stats.probed - old_probed;
#endif
  int64_t hbrs = stats.hbrs - old_hbrs;

  MSG ("lookahead-probe-round %" PRId64 " probed %" PRId64
       " and found %d failed literals",
       stats.probingrounds, probed, failed);

  if (hbrs)
    PHASE ("lookahead-probe-round", stats.probingrounds,
           "found %" PRId64 " hyper binary resolvents", hbrs);

  MSG ("lookahead literal %d with %d\n", res, max_hbrs);

  return res;
}

CubesWithStatus Internal::generate_cubes (int depth, int min_depth) {
  if (!active () || depth == 0) {
    CubesWithStatus cubes;
    cubes.status = 0;
    cubes.cubes.push_back (std::vector<int> ());
    return cubes;
  }

  lookingahead = true;
  START (lookahead);
  MSG ("Generating cubes of depth %i", depth);

  // presimplify required due to assumptions

  termination_forced = false;
  int res = already_solved ();
  if (res == 0)
    res = restore_clauses ();
  if (unsat)
    res = 10;
  if (res != 0)
    res = solve (true);
  if (res != 0) {
    MSG ("Solved during preprocessing");
    CubesWithStatus cubes;
    cubes.status = res;
    lookingahead = false;
    STOP (lookahead);
    return cubes;
  }

  reset_limits ();
  MSG ("generate cubes with %zu assumptions\n", assumptions.size ());

  CADICAL_assert (ntab.empty ());
  std::vector<int> current_assumptions{assumptions};
  std::vector<std::vector<int>> cubes{{assumptions}};
  auto loccs{lookahead_populate_locc ()};
  LOG ("loccs populated\n");
  CADICAL_assert (ntab.empty ());

  for (int i = 0; i < depth; ++i) {
    LOG ("Probing at depth %i, currently %zu have been generated", i,
         cubes.size ());
    std::vector<std::vector<int>> cubes2{std::move (cubes)};
    cubes.clear ();

    for (size_t j = 0; j < cubes2.size (); ++j) {
      CADICAL_assert (ntab.empty ());
      CADICAL_assert (!unsat);
      reset_assumptions ();
      for (auto lit : cubes2[j])
        assume (lit);
      restore_clauses ();
      propagate ();
      // preprocess_round(0); //uncomment maybe

      if (unsat) {
        LOG ("current cube is unsat; skipping");
        unsat = false;
        continue;
      }

      int res = terminating_asked () ? lookahead_locc (loccs)
                                     : lookahead_probing ();
      if (unsat) {
        LOG ("current cube is unsat; skipping");
        unsat = false;
        continue;
      }

      if (res == 0) {
        LOG ("no lit to split %i", res);
        cubes.push_back (cubes2[j]);
        continue;
      }

      CADICAL_assert (res != 0);
      LOG ("splitting on lit %i", res);
      std::vector<int> cube1{cubes2[j]};
      cube1.push_back (res);
      std::vector<int> cube2{std::move (cubes2[j])};
      cube2.push_back (-res);
      cubes.push_back (cube1);
      cubes.push_back (cube2);
    }

    if (terminating_asked () && i >= min_depth)
      break;
  }

  CADICAL_assert (std::for_each (
      std::begin (cubes), std::end (cubes),
      [] (std::vector<int> cube) { return non_tautological_cube (cube); }));
  reset_assumptions ();

  for (auto lit : current_assumptions)
    assume (lit);

  STOP (lookahead);
  lookingahead = false;

  if (unsat) {
    LOG ("Solved during preprocessing");
    CubesWithStatus cubes;
    cubes.status = 20;
    return cubes;
  }

  CubesWithStatus rcubes;
  rcubes.status = 0;
  rcubes.cubes = cubes;

  return rcubes;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
