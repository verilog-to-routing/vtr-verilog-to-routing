#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// Code for conflict analysis, i.e., to generate the first UIP clause.  The
// main function is 'analyze' below.  It further uses 'minimize' to minimize
// the first UIP clause, which is in 'minimize.cpp'.  An important side
// effect of conflict analysis is to update the decision queue by bumping
// variables.  Similarly analyzed clauses are bumped to mark them as active.

/*------------------------------------------------------------------------*/

void Internal::learn_empty_clause () {
  CADICAL_assert (!unsat);
  build_chain_for_empty ();
  LOG ("learned empty clause");
  external->check_learned_empty_clause ();
  int64_t id = ++clause_id;
  if (proof) {
    proof->add_derived_empty_clause (id, lrat_chain);
  }
  unsat = true;
  conflict_id = id;
  marked_failed = true;
  conclusion.push_back (id);
  lrat_chain.clear ();
}

void Internal::learn_unit_clause (int lit) {
  CADICAL_assert (!unsat);
  LOG ("learned unit clause %d, stored at position %d", lit, vlit (lit));
  external->check_learned_unit_clause (lit);
  int64_t id = ++clause_id;
  if (lrat || frat) {
    const unsigned uidx = vlit (lit);
    unit_clauses (uidx) = id;
  }
  if (proof) {
    proof->add_derived_unit_clause (id, lit, lrat_chain);
  }
  mark_fixed (lit);
}

/*------------------------------------------------------------------------*/

// Move bumped variables to the front of the (VMTF) decision queue.  The
// 'bumped' time stamp is updated accordingly.  It is used to determine
// whether the 'queue.assigned' pointer has to be moved in 'unassign'.

void Internal::bump_queue (int lit) {
  CADICAL_assert (opts.bump);
  const int idx = vidx (lit);
  if (!links[idx].next)
    return;
  queue.dequeue (links, idx);
  queue.enqueue (links, idx);
  CADICAL_assert (stats.bumped != INT64_MAX);
  btab[idx] = ++stats.bumped;
  LOG ("moved to front variable %d and bumped to %" PRId64 "", idx,
       btab[idx]);
  if (!vals[idx])
    update_queue_unassigned (idx);
}

/*------------------------------------------------------------------------*/

// It would be better to use 'isinf' but there are some historical issues
// with this function.  On some platforms it is a macro and even for C++ it
// changed the scope (in pre 5.0 gcc) from '::isinf' to 'std::isinf'.  I do
// not want to worry about these strange incompatibilities and thus use the
// same trick as in older solvers (since the MiniSAT team invented EVSIDS)
// and simply put a hard limit here.  It is less elegant but easy to port.

static inline bool evsids_limit_hit (double score) {
  CADICAL_assert (sizeof (score) == 8); // assume IEEE 754 64-bit double
  return score > 1e150;         // MAX_DOUBLE is around 1.8e308
}

/*------------------------------------------------------------------------*/

// Classical exponential VSIDS as pioneered by MiniSAT.

void Internal::rescale_variable_scores () {
  stats.rescored++;
  double divider = score_inc;
  for (auto idx : vars) {
    const double tmp = stab[idx];
    if (tmp > divider)
      divider = tmp;
  }
  PHASE ("rescore", stats.rescored, "rescoring %d variable scores by 1/%g",
         max_var, divider);
  CADICAL_assert (divider > 0);
  double factor = 1.0 / divider;
  for (auto idx : vars)
    stab[idx] *= factor;
  score_inc *= factor;
  PHASE ("rescore", stats.rescored,
         "new score increment %g after %" PRId64 " conflicts", score_inc,
         stats.conflicts);
}

void Internal::bump_variable_score (int lit) {
  CADICAL_assert (opts.bump);
  int idx = vidx (lit);
  double old_score = score (idx);
  CADICAL_assert (!evsids_limit_hit (old_score));
  double new_score = old_score + score_inc;
  if (evsids_limit_hit (new_score)) {
    LOG ("bumping %g score of %d hits EVSIDS score limit", old_score, idx);
    rescale_variable_scores ();
    old_score = score (idx);
    CADICAL_assert (!evsids_limit_hit (old_score));
    new_score = old_score + score_inc;
  }
  CADICAL_assert (!evsids_limit_hit (new_score));
  LOG ("new %g score of %d", new_score, idx);
  score (idx) = new_score;
  if (scores.contains (idx))
    scores.update (idx);
}

// Important variables recently used in conflict analysis are 'bumped',

void Internal::bump_variable (int lit) {
  if (use_scores ())
    bump_variable_score (lit);
  else
    bump_queue (lit);
}

// After every conflict the variable score increment is increased by a
// factor (if we are currently using scores).

void Internal::bump_variable_score_inc () {
  CADICAL_assert (use_scores ());
  CADICAL_assert (!evsids_limit_hit (score_inc));
  double f = 1e3 / opts.scorefactor;
  double new_score_inc = score_inc * f;
  if (evsids_limit_hit (new_score_inc)) {
    LOG ("bumping %g increment by %g hits EVSIDS score limit", score_inc,
         f);
    rescale_variable_scores ();
    new_score_inc = score_inc * f;
  }
  CADICAL_assert (!evsids_limit_hit (new_score_inc));
  LOG ("bumped score increment from %g to %g with factor %g", score_inc,
       new_score_inc, f);
  score_inc = new_score_inc;
}

/*------------------------------------------------------------------------*/

struct analyze_bumped_rank {
  Internal *internal;
  analyze_bumped_rank (Internal *i) : internal (i) {}
  typedef uint64_t Type;
  Type operator() (const int &a) const { return internal->bumped (a); }
};

struct analyze_bumped_smaller {
  Internal *internal;
  analyze_bumped_smaller (Internal *i) : internal (i) {}
  bool operator() (const int &a, const int &b) const {
    const auto s = analyze_bumped_rank (internal) (a);
    const auto t = analyze_bumped_rank (internal) (b);
    return s < t;
  }
};

/*------------------------------------------------------------------------*/

void Internal::bump_variables () {

  CADICAL_assert (opts.bump);

  START (bump);

  if (!use_scores ()) {

    // Variables are bumped in the order they are in the current decision
    // queue.  This maintains relative order between bumped variables in
    // the queue and seems to work best.  We also experimented with
    // focusing on variables of the last decision level, but results were
    // mixed.

    MSORT (opts.radixsortlim, analyzed.begin (), analyzed.end (),
           analyze_bumped_rank (this), analyze_bumped_smaller (this));
  }

  for (const auto &lit : analyzed)
    bump_variable (lit);

  if (use_scores ())
    bump_variable_score_inc ();

  STOP (bump);
}

/*------------------------------------------------------------------------*/

// We use the glue time stamp table 'gtab' for fast glue computation.

int Internal::recompute_glue (Clause *c) {
  int res = 0;
  const int64_t stamp = ++stats.recomputed;
  for (const auto &lit : *c) {
    int level = var (lit).level;
    CADICAL_assert (gtab[level] <= stamp);
    if (gtab[level] == stamp)
      continue;
    gtab[level] = stamp;
    res++;
  }
  return res;
}

// Clauses resolved since the last reduction are marked as 'used', their
// glue is recomputed and they are promoted if the glue shrinks.  Note that
// promotion from 'tier3' to 'tier2' will set 'used' to '2'.

inline void Internal::bump_clause (Clause *c) {
  LOG (c, "bumping");
  c->used = max_used;
  if (c->hyper)
    return;
  if (!c->redundant)
    return;
  int new_glue = recompute_glue (c);
  if (new_glue < c->glue)
    promote_clause (c, new_glue);

  const size_t glue =
      std::min ((size_t) c->glue, stats.used[stable].size () - 1);
  ++stats.used[stable][glue];
  ++stats.bump_used[stable];
}

void Internal::bump_clause2 (Clause *c) { bump_clause (c); }
/*------------------------------------------------------------------------*/

// During conflict analysis literals not seen yet either become part of the
// first unique implication point (UIP) clause (if on lower decision level),
// are dropped (if fixed), or are resolved away (if on the current decision
// level and different from the first UIP).  At the same time we update the
// number of seen literals on a decision level.  This helps conflict clause
// minimization.  The number of seen levels is the glucose level (also
// called 'glue', or 'LBD').

inline void Internal::analyze_literal (int lit, int &open,
                                       int &resolvent_size,
                                       int &antecedent_size) {
  CADICAL_assert (lit);
  Var &v = var (lit);
  Flags &f = flags (lit);

  if (!v.level) {
    if (f.seen || !lrat)
      return;
    f.seen = true;
    unit_analyzed.push_back (lit);
    CADICAL_assert (val (lit) < 0);
    int64_t id = unit_id (-lit);
    unit_chain.push_back (id);
    return;
  }
  ++antecedent_size;
  if (f.seen)
    return;

  // before marking as seen, get reason and check for missed unit

  CADICAL_assert (val (lit) < 0);
  CADICAL_assert (v.level <= level);
  if (v.reason == external_reason) {
    CADICAL_assert (!opts.exteagerreasons);
    v.reason = learn_external_reason_clause (-lit, 0, true);
    if (!v.reason) { // actually a unit
      --antecedent_size;
      LOG ("%d unit after explanation", -lit);
      if (f.seen || !lrat)
        return;
      f.seen = true;
      unit_analyzed.push_back (lit);
      CADICAL_assert (val (lit) < 0);
      const unsigned uidx = vlit (-lit);
      int64_t id = unit_clauses (uidx);
      CADICAL_assert (id);
      unit_chain.push_back (id);
      return;
    }
  }

  f.seen = true;
  analyzed.push_back (lit);

  CADICAL_assert (v.reason != external_reason);
  if (v.level < level)
    clause.push_back (lit);
  Level &l = control[v.level];
  if (!l.seen.count++) {
    LOG ("found new level %d contributing to conflict", v.level);
    levels.push_back (v.level);
  }
  if (v.trail < l.seen.trail)
    l.seen.trail = v.trail;
  ++resolvent_size;
  LOG ("analyzed literal %d assigned at level %d", lit, v.level);
  if (v.level == level)
    open++;
}

inline void Internal::analyze_reason (int lit, Clause *reason, int &open,
                                      int &resolvent_size,
                                      int &antecedent_size) {
  CADICAL_assert (reason);
  CADICAL_assert (reason != external_reason);
  bump_clause (reason);
  if (lrat)
    lrat_chain.push_back (reason->id);
  for (const auto &other : *reason)
    if (other != lit)
      analyze_literal (other, open, resolvent_size, antecedent_size);
}

/*------------------------------------------------------------------------*/

// This is an idea which was implicit in MapleCOMSPS 2016 for 'limit = 1'.
// See also the paragraph on 'bumping reason side literals' in their SAT'16
// paper [LiangGaneshPoupartCzarnecki-SAT'16].  Reason side bumping was
// performed exactly when 'LRB' based decision heuristics was used, which in
// the original version was enabled after 10000 conflicts until a time limit
// of 2500 seconds was reached (half of the competition time limit).  The
// Maple / Glucose / MiniSAT evolution winning the SAT race in 2019 made
// the schedule of reason side bumping deterministic, i.e., avoiding a time
// limit, by switching between 'LRB' and 'VSIDS' in an interval of initially
// 30 million propagations, which then is increased geometrically by 10%.

inline bool Internal::bump_also_reason_literal (int lit) {
  CADICAL_assert (lit);
  CADICAL_assert (val (lit) < 0);
  Flags &f = flags (lit);
  if (f.seen)
    return false;
  const Var &v = var (lit);
  if (!v.level)
    return false;
  f.seen = true;
  analyzed.push_back (lit);
  LOG ("bumping also reason literal %d assigned at level %d", lit, v.level);
  return true;
}

// We experimented with deeper reason bumping without much success though.

inline void Internal::bump_also_reason_literals (int lit, int depth_limit,
                                                 size_t analyzed_limit) {
  CADICAL_assert (lit);
  CADICAL_assert (depth_limit > 0);
  const Var &v = var (lit);
  CADICAL_assert (val (lit));
  if (!v.level)
    return;
  Clause *reason = v.reason;
  if (!reason || reason == external_reason)
    return;
  stats.ticks.search[stable]++;
  for (const auto &other : *reason) {
    if (other == lit)
      continue;
    if (!bump_also_reason_literal (other))
      continue;
    if (depth_limit < 2)
      continue;
    bump_also_reason_literals (-other, depth_limit - 1, analyzed_limit);
    if (analyzed.size () > analyzed_limit)
      break;
  }
}

inline void Internal::bump_also_all_reason_literals () {
  CADICAL_assert (opts.bump);
  if (!opts.bumpreason)
    return;
  if (averages.current.decisions > opts.bumpreasonrate) {
    LOG ("decisions per conflict rate %g > limit %d",
         (double) averages.current.decisions, opts.bumpreasonrate);
    return;
  }
  if (delay[stable].bumpreasons.limit) {
    LOG ("delaying reason bumping %" PRId64 " more times",
         delay[stable].bumpreasons.limit);
    delay[stable].bumpreasons.limit--;
    return;
  }
  CADICAL_assert (opts.bumpreasondepth > 0);
  const int depth_limit = opts.bumpreasondepth + stable;
  size_t saved_analyzed = analyzed.size ();
  size_t analyzed_limit = saved_analyzed * opts.bumpreasonlimit;
  for (const auto &lit : clause)
    if (analyzed.size () <= analyzed_limit)
      bump_also_reason_literals (-lit, depth_limit, analyzed_limit);
    else
      break;
  if (analyzed.size () > analyzed_limit) {
    LOG ("not bumping reason side literals as limit exhausted");
    for (size_t i = saved_analyzed; i != analyzed.size (); i++) {
      const int lit = analyzed[i];
      Flags &f = flags (lit);
      CADICAL_assert (f.seen);
      f.seen = false;
    }
    delay[stable].bumpreasons.interval++;
    analyzed.resize (saved_analyzed);
  } else {
    LOG ("bumping reasons up to depth %d", opts.bumpreasondepth);
    delay[stable].bumpreasons.interval /= 2;
  }
  LOG ("delay internal %" PRId64, delay[stable].bumpreasons.interval);
  delay[stable].bumpreasons.limit = delay[stable].bumpreasons.interval;
}

/*------------------------------------------------------------------------*/

void Internal::clear_unit_analyzed_literals () {
  LOG ("clearing %zd unit analyzed literals", unit_analyzed.size ());
  for (const auto &lit : unit_analyzed) {
    Flags &f = flags (lit);
    CADICAL_assert (f.seen);
    CADICAL_assert (!var (lit).level);
    f.seen = false;
    CADICAL_assert (!f.keep);
    CADICAL_assert (!f.poison);
    CADICAL_assert (!f.removable);
  }
  unit_analyzed.clear ();
}

void Internal::clear_analyzed_literals () {
  LOG ("clearing %zd analyzed literals", analyzed.size ());
  for (const auto &lit : analyzed) {
    Flags &f = flags (lit);
    CADICAL_assert (f.seen);
    f.seen = false;
    CADICAL_assert (!f.keep);
    CADICAL_assert (!f.poison);
    CADICAL_assert (!f.removable);
  }
  analyzed.clear ();
#if 0 // to expensive, even for debugging mode
  if (unit_analyzed.size ())
    return;
  for (auto idx : vars) {
    Flags &f = flags (idx);
    CADICAL_assert (!f.seen);
  }
#endif
}

void Internal::clear_analyzed_levels () {
  LOG ("clearing %zd analyzed levels", levels.size ());
  for (const auto &l : levels)
    if (l < (int) control.size ())
      control[l].reset ();
  levels.clear ();
}

/*------------------------------------------------------------------------*/

// Smaller level and trail.  Comparing literals on their level is necessary
// for chronological backtracking, since trail order might in this case not
// respect level order.

struct analyze_trail_negative_rank {
  Internal *internal;
  analyze_trail_negative_rank (Internal *s) : internal (s) {}
  typedef uint64_t Type;
  Type operator() (int a) {
    Var &v = internal->var (a);
    uint64_t res = v.level;
    res <<= 32;
    res |= v.trail;
    return ~res;
  }
};

struct analyze_trail_larger {
  Internal *internal;
  analyze_trail_larger (Internal *s) : internal (s) {}
  bool operator() (const int &a, const int &b) const {
    return analyze_trail_negative_rank (internal) (a) <
           analyze_trail_negative_rank (internal) (b);
  }
};

/*------------------------------------------------------------------------*/

// Generate new driving clause and compute jump level.

Clause *Internal::new_driving_clause (const int glue, int &jump) {

  const size_t size = clause.size ();
  Clause *res;

  if (!size) {

    jump = 0;
    res = 0;

  } else if (size == 1) {

    iterating = true;
    jump = 0;
    res = 0;

  } else {

    CADICAL_assert (clause.size () > 1);

    // We have to get the last assigned literals into the watch position.
    // Sorting all literals with respect to reverse assignment order is
    // overkill but seems to get slightly faster run-time.  For 'minimize'
    // we sort the literals too heuristically along the trail order (so in
    // the opposite order) with the hope to hit the recursion limit less
    // frequently.  Thus sorting effort is doubled here.
    //
    MSORT (opts.radixsortlim, clause.begin (), clause.end (),
           analyze_trail_negative_rank (this), analyze_trail_larger (this));

    jump = var (clause[1]).level;
    res = new_learned_redundant_clause (glue);
    res->used = 1 + (glue <= opts.reducetier2glue);
  }

  LOG ("jump level %d", jump);

  return res;
}

/*------------------------------------------------------------------------*/

// determine the OTFS level for OTFS. Unlike the find_conflict_level, we do
// not have to fix the clause

inline int Internal::otfs_find_backtrack_level (int &forced) {
  CADICAL_assert (opts.otfs);
  int res = 0;

  for (const auto &lit : *conflict) {
    const int tmp = var (lit).level;
    if (tmp == level) {
      forced = lit;
    } else if (tmp > res) {
      res = tmp;
      LOG ("bt level is now %d due to %d", res, lit);
    }
  }
  return res;
}

/*------------------------------------------------------------------------*/

// If chronological backtracking is enabled we need to find the actual
// conflict level and then potentially can also reuse the conflict clause
// as driving clause instead of deriving a redundant new driving clause
// (forcing 'forced') if the number 'count' of literals in conflict assigned
// at the conflict level is exactly one.

inline int Internal::find_conflict_level (int &forced) {

  CADICAL_assert (conflict);
  CADICAL_assert (opts.chrono || opts.otfs || external_prop);

  int res = 0, count = 0;

  forced = 0;

  for (const auto &lit : *conflict) {
    const int tmp = var (lit).level;
    if (tmp > res) {
      res = tmp;
      forced = lit;
      count = 1;
    } else if (tmp == res) {
      count++;
      if (res == level && count > 1)
        break;
    }
  }

  LOG ("%d literals on actual conflict level %d", count, res);

  const int size = conflict->size;
  int *lits = conflict->literals;

  // Move the two highest level literals to the front.
  //
  for (int i = 0; i < 2; i++) {

    const int lit = lits[i];

    int highest_position = i;
    int highest_literal = lit;
    int highest_level = var (highest_literal).level;

    for (int j = i + 1; j < size; j++) {
      const int other = lits[j];
      const int tmp = var (other).level;
      if (highest_level >= tmp)
        continue;
      highest_literal = other;
      highest_position = j;
      highest_level = tmp;
      if (highest_level == res)
        break;
    }

    // No unwatched higher assignment level literal.
    //
    if (highest_position == i)
      continue;

    if (highest_position > 1) {
      LOG (conflict, "unwatch %d in", lit);
      remove_watch (watches (lit), conflict);
    }

    lits[highest_position] = lit;
    lits[i] = highest_literal;

    if (highest_position > 1)
      watch_literal (highest_literal, lits[!i], conflict);
  }

  // Only if the number of highest level literals in the conflict is one
  // then we can reuse the conflict clause as driving clause for 'forced'.
  //
  if (count != 1)
    forced = 0;

  return res;
}

/*------------------------------------------------------------------------*/

inline int Internal::determine_actual_backtrack_level (int jump) {

  int res;

  CADICAL_assert (level > jump);

  if (!opts.chrono) {
    res = jump;
    LOG ("chronological backtracking disabled using jump level %d", res);
  } else if (opts.chronoalways) {
    stats.chrono++;
    res = level - 1;
    LOG ("forced chronological backtracking to level %d", res);
  } else if (jump >= level - 1) {
    res = jump;
    LOG ("jump level identical to chronological backtrack level %d", res);
  } else if ((size_t) jump < assumptions.size ()) {
    res = jump;
    LOG ("using jump level %d since it is lower than assumption level %zd",
         res, assumptions.size ());
  } else if (level - jump > opts.chronolevelim) {
    stats.chrono++;
    res = level - 1;
    LOG ("back-jumping over %d > %d levels prohibited"
         "thus backtracking chronologically to level %d",
         level - jump, opts.chronolevelim, res);
  } else if (opts.chronoreusetrail) {
    int best_idx = 0, best_pos = 0;

    if (use_scores ()) {
      for (size_t i = control[jump + 1].trail; i < trail.size (); i++) {
        const int idx = abs (trail[i]);
        if (best_idx && !score_smaller (this) (best_idx, idx))
          continue;
        best_idx = idx;
        best_pos = i;
      }
      LOG ("best variable score %g", score (best_idx));
    } else {
      for (size_t i = control[jump + 1].trail; i < trail.size (); i++) {
        const int idx = abs (trail[i]);
        if (best_idx && bumped (best_idx) >= bumped (idx))
          continue;
        best_idx = idx;
        best_pos = i;
      }
      LOG ("best variable bumped %" PRId64 "", bumped (best_idx));
    }
    CADICAL_assert (best_idx);
    LOG ("best variable %d at trail position %d", best_idx, best_pos);

    // Now find the frame and decision level in the control stack of that
    // best variable index.  Note that, as in 'reuse_trail', the frame
    // 'control[i]' for decision level 'i' contains the trail before that
    // decision level, i.e., the decision 'control[i].decision' sits at
    // 'control[i].trail' in the trail and we thus have to check the level
    // of the control frame one higher than at the result level.
    //
    res = jump;
    while (res < level - 1 && control[res + 1].trail <= best_pos)
      res++;

    if (res == jump)
      LOG ("default non-chronological back-jumping to level %d", res);
    else {
      stats.chrono++;
      LOG ("chronological backtracking to level %d to reuse trail", res);
    }

  } else {
    res = jump;
    LOG ("non-chronological back-jumping to level %d", res);
  }

  return res;
}

/*------------------------------------------------------------------------*/

void Internal::eagerly_subsume_recently_learned_clauses (Clause *c) {
  CADICAL_assert (opts.eagersubsume);
  LOG (c, "trying eager subsumption with");
  mark (c);
  int64_t lim = stats.eagertried + opts.eagersubsumelim;
  const auto begin = clauses.begin ();
  auto it = clauses.end ();
#ifdef LOGGING
  int64_t before = stats.eagersub;
#endif
  while (it != begin && stats.eagertried++ <= lim) {
    Clause *d = *--it;
    if (c == d)
      continue;
    if (d->garbage)
      continue;
    if (!d->redundant)
      continue;
    int needed = c->size;
    for (auto &lit : *d) {
      if (marked (lit) <= 0)
        continue;
      if (!--needed)
        break;
    }
    if (needed)
      continue;
    LOG (d, "eager subsumed");
    stats.eagersub++;
    stats.subsumed++;
    mark_garbage (d);
  }
  unmark (c);
#ifdef LOGGING
  uint64_t subsumed = stats.eagersub - before;
  if (subsumed)
    LOG ("eagerly subsumed %" PRIu64 " clauses", subsumed);
#endif
}

/*------------------------------------------------------------------------*/

Clause *Internal::on_the_fly_strengthen (Clause *new_conflict, int uip) {
  CADICAL_assert (new_conflict);
  CADICAL_assert (new_conflict->size > 2);
  LOG (new_conflict, "applying OTFS on lit %d", uip);
  auto sorted = std::vector<int> ();
  sorted.reserve (new_conflict->size);
  CADICAL_assert (sorted.empty ());
  ++stats.otfs.strengthened;

  int *lits = new_conflict->literals;

  CADICAL_assert (lits[0] == uip || lits[1] == uip);
  const int other_init = lits[0] ^ lits[1] ^ uip;

  CADICAL_assert (mini_chain.empty ());

  const int old_size = new_conflict->size;
  int new_size = 0;
  for (int i = 0; i < old_size; ++i) {
    const int other = lits[i];
    sorted.push_back (other);
    if (var (other).level)
      lits[new_size++] = other;
  }

  LOG (new_conflict, "removing all units in");

  CADICAL_assert (lits[0] == uip || lits[1] == uip);
  const int other = lits[0] ^ lits[1] ^ uip;
  lits[0] = other;
  lits[1] = lits[--new_size];
  LOG (new_conflict, "putting uip at pos 1");

  if (other_init != other)
    remove_watch (watches (other_init), new_conflict);
  remove_watch (watches (uip), new_conflict);

  CADICAL_assert (!lrat || lrat_chain.back () == new_conflict->id);
  if (lrat) {
    CADICAL_assert (!lrat_chain.empty ());
    for (const auto &id : unit_chain) {
      mini_chain.push_back (id);
    }
    const auto end = lrat_chain.rend ();
    const auto begin = lrat_chain.rbegin ();
    for (auto i = begin; i != end; i++) {
      const auto id = *i;
      mini_chain.push_back (id);
    }
    lrat_chain.clear ();
    clear_unit_analyzed_literals ();
    unit_chain.clear ();
  }
  CADICAL_assert (unit_analyzed.empty ());
  // sort the clause
  {
    int highest_pos = 0;
    int highest_level = 0;
    for (int i = 1; i < new_size; i++) {
      const unsigned other = lits[i];
      CADICAL_assert (val (other) < 0);
      const int level = var (other).level;
      CADICAL_assert (level);
      LOG ("checking %d", other);
      if (level <= highest_level)
        continue;
      highest_pos = i;
      highest_level = level;
    }
    LOG ("highest lit is %d", lits[highest_pos]);
    if (highest_pos != 1)
      swap (lits[1], lits[highest_pos]);
    LOG ("removing %d literals", new_conflict->size - new_size);

    if (new_size == 1) {
      LOG (new_conflict, "new size = 1, so interrupting");
      CADICAL_assert (!opts.exteagerreasons);
      return 0;
    } else {
      otfs_strengthen_clause (new_conflict, uip, new_size, sorted);
      CADICAL_assert (new_size == new_conflict->size);
    }
  }

  if (other_init != other)
    watch_literal (other, lits[1], new_conflict);
  else {
    update_watch_size (watches (other), lits[1], new_conflict);
  }
  watch_literal (lits[1], other, new_conflict);

  LOG (new_conflict, "strengthened clause by OTFS");
  sorted.clear ();

  return new_conflict;
}

/*------------------------------------------------------------------------*/
inline void Internal::otfs_subsume_clause (Clause *subsuming,
                                           Clause *subsumed) {
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

// Candidate clause 'c' is strengthened by removing 'lit' and units.
//
void Internal::otfs_strengthen_clause (Clause *c, int lit, int new_size,
                                       const std::vector<int> &old) {
  stats.strengthened++;
  CADICAL_assert (c->size > 2);
  (void) shrink_clause (c, new_size);
  if (proof) {
    proof->otfs_strengthen_clause (c, old, mini_chain);
  }
  if (!c->redundant) {
    mark_removed (lit);
  }
  mini_chain.clear ();
  c->used = true;
  LOG (c, "strengthened");
  external->check_shrunken_clause (c);
}

/*------------------------------------------------------------------------*/

// If the average number of decisions per conflict (analysis actually so not
// taking OTFS conflicts into account) is high we do not bump reasons. This
// is the function which updates the exponential moving decision rate
// average.

void Internal::update_decision_rate_average () {
  int64_t current = stats.decisions;
  int64_t decisions = current - saved_decisions;
  UPDATE_AVERAGE (averages.current.decisions, decisions);
  saved_decisions = current;
}

/*------------------------------------------------------------------------*/

// This is the main conflict analysis routine.  It assumes that a conflict
// was found.  Then we derive the 1st UIP clause, optionally minimize it,
// add it as learned clause, and then uses the clause for conflict directed
// back-jumping and flipping the 1st UIP literal.  In combination with
// chronological backtracking (see discussion above) the algorithm becomes
// slightly more involved.

void Internal::analyze () {

  START (analyze);

  CADICAL_assert (conflict);
  CADICAL_assert (lrat_chain.empty ());
  CADICAL_assert (unit_chain.empty ());
  CADICAL_assert (unit_analyzed.empty ());
  CADICAL_assert (clause.empty ());

  // First update moving averages of trail height at conflict.
  //
  UPDATE_AVERAGE (averages.current.trail.fast, num_assigned);
  UPDATE_AVERAGE (averages.current.trail.slow, num_assigned);
  update_decision_rate_average ();

  /*----------------------------------------------------------------------*/

  if (external_prop && !external_prop_is_lazy && opts.exteagerreasons) {
    explain_external_propagations ();
  }

  if (opts.chrono || external_prop) {

    int forced;

    const int conflict_level = find_conflict_level (forced);

    // In principle we can perform conflict analysis as in non-chronological
    // backtracking except if there is only one literal with the maximum
    // assignment level in the clause.  Then standard conflict analysis is
    // unnecessary and we can use the conflict as a driving clause.  In the
    // pseudo code of the SAT'18 paper on chronological backtracking this
    // corresponds to the situation handled in line 4-6 in Alg. 1, except
    // that the pseudo code in the paper only backtracks while we eagerly
    // assign the single literal on the highest decision level.

    if (forced) {

      CADICAL_assert (forced);
      CADICAL_assert (conflict_level > 0);
      LOG ("single highest level literal %d", forced);

      // The pseudo code in the SAT'18 paper actually backtracks to the
      // 'second highest decision' level, while their code backtracks
      // to 'conflict_level-1', which is more in the spirit of chronological
      // backtracking anyhow and thus we also do the latter.
      //
      backtrack (conflict_level - 1);

      // if we are on decision level 0 search assign will learn unit
      // so we need a valid chain here (of course if we are not on decision
      // level 0 this will not result in a valid chain).
      // we can just use build_chain_for_units in propagate
      //
      build_chain_for_units (forced, conflict, 0);

      LOG ("forcing %d", forced);
      search_assign_driving (forced, conflict);

      conflict = 0;
      STOP (analyze);
      return;
    }

    // Backtracking to the conflict level is in the pseudo code in the
    // SAT'18 chronological backtracking paper, but not in their actual
    // implementation.  In principle we do not need to backtrack here.
    // However, as a side effect of backtracking to the conflict level we
    // set 'level' to the conflict level which then allows us to reuse the
    // old 'analyze' code as is.  The alternative (which we also tried but
    // then abandoned) is to use 'conflict_level' instead of 'level' in the
    // analysis, which however requires to pass it to the 'analyze_reason'
    // and 'analyze_literal' functions.
    //
    backtrack (conflict_level);
  }

  // Actual conflict on root level, thus formula unsatisfiable.
  //
  if (!level) {
    learn_empty_clause ();
    if (external->learner)
      external->export_learned_empty_clause ();
    STOP (analyze);
    return;
  }

  /*----------------------------------------------------------------------*/

  // First derive the 1st UIP clause by going over literals assigned on the
  // current decision level.  Literals in the conflict are marked as 'seen'
  // as well as all literals in reason clauses of already 'seen' literals on
  // the current decision level.  Thus the outer loop starts with the
  // conflict clause as 'reason' and then uses the 'reason' of the next
  // seen literal on the trail assigned on the current decision level.
  // During this process maintain the number 'open' of seen literals on the
  // current decision level with not yet processed 'reason'.  As soon 'open'
  // drops to one, we have found the first unique implication point.  This
  // is sound because the topological order in which literals are processed
  // follows the assignment order and a more complex algorithm to find
  // articulation points is not necessary.
  //
  Clause *reason = conflict;
  LOG (reason, "analyzing conflict");

  CADICAL_assert (clause.empty ());
  CADICAL_assert (lrat_chain.empty ());

  const auto &t = &trail;
  int i = t->size ();      // Start at end-of-trail.
  int open = 0;            // Seen but not processed on this level.
  int uip = 0;             // The first UIP literal.
  int resolvent_size = 0;  // without the uip
  int antecedent_size = 1; // with the uip and without unit literals
  int conflict_size = 0;   // without the uip and without unit literals
  int resolved = 0;        // number of resolution (0 = clause in CNF)
  const bool otfs = opts.otfs;

  for (;;) {
    antecedent_size = 1; // for uip
    analyze_reason (uip, reason, open, resolvent_size, antecedent_size);
    if (resolved == 0)
      conflict_size = antecedent_size - 1;
    CADICAL_assert (resolvent_size == open + (int) clause.size ());

    if (otfs && resolved > 0 && antecedent_size > 2 &&
        resolvent_size < antecedent_size) {
      CADICAL_assert (reason != conflict);
      LOG (analyzed, "found candidate for OTFS conflict");
      LOG (clause, "found candidate for OTFS conflict");
      LOG (reason, "found candidate (size %d) for OTFS resolvent",
           antecedent_size);
      const int other = reason->literals[0] ^ reason->literals[1] ^ uip;
      CADICAL_assert (other != uip);
      reason = on_the_fly_strengthen (reason, uip);
      if (opts.bump)
        bump_variables ();

      CADICAL_assert (conflict_size);
      if (!reason) {
        uip = -other;
        CADICAL_assert (open == 1);
        LOG ("clause is actually unit %d, stopping", -uip);
        reverse (begin (mini_chain), end (mini_chain));
        for (auto id : mini_chain)
          lrat_chain.push_back (id);
        mini_chain.clear ();
        clear_analyzed_levels ();
        CADICAL_assert (!opts.exteagerreasons);
        clause.clear ();
        break;
      }
      CADICAL_assert (conflict_size >= 2);

      if (resolved == 1 && resolvent_size < conflict_size) {
        // here both clauses are part of the CNF, so one subsumes the other
        otfs_subsume_clause (reason, conflict);
        LOG (reason, "changing conflict to");
        --conflict_size;
        CADICAL_assert (conflict_size == reason->size);
        ++stats.otfs.subsumed;
        ++stats.subsumed;
      }

      LOG (reason, "changing conflict to");
      conflict = reason;
      if (open == 1) {
        int forced = 0;
        const int conflict_level = otfs_find_backtrack_level (forced);
        int new_level = determine_actual_backtrack_level (conflict_level);
        UPDATE_AVERAGE (averages.current.level, new_level);
        backtrack (new_level);

        LOG ("forcing %d", forced);
        search_assign_driving (forced, conflict);

        // Clean up.
        //
        conflict = 0;
        clear_analyzed_literals ();
        clear_analyzed_levels ();
        clause.clear ();
        STOP (analyze);
        return;
      }

      stats.conflicts++;

      clear_analyzed_literals ();
      clear_analyzed_levels ();
      clause.clear ();
      resolvent_size = 0;
      antecedent_size = 1;
      resolved = 0;
      open = 0;
      analyze_reason (0, reason, open, resolvent_size, antecedent_size);
      conflict_size = antecedent_size - 1;
      CADICAL_assert (open > 1);
    }

    ++resolved;

    uip = 0;
    while (!uip) {
      CADICAL_assert (i > 0);
      const int lit = (*t)[--i];
      if (!flags (lit).seen)
        continue;
      if (var (lit).level == level)
        uip = lit;
    }
    if (!--open)
      break;
    reason = var (uip).reason;
    if (reason == external_reason) {
      CADICAL_assert (!opts.exteagerreasons);
      reason = learn_external_reason_clause (-uip, 0, true);
      var (uip).reason = reason;
    }
    CADICAL_assert (reason != external_reason);
    LOG (reason, "analyzing %d reason", uip);
    CADICAL_assert (resolvent_size);
    --resolvent_size;
  }
  LOG ("first UIP %d", uip);
  clause.push_back (-uip);

  // Update glue and learned (1st UIP literals) statistics.
  //
  int size = (int) clause.size ();
  const int glue = (int) levels.size () - 1;
  LOG (clause, "1st UIP size %d and glue %d clause", size, glue);
  UPDATE_AVERAGE (averages.current.glue.fast, glue);
  UPDATE_AVERAGE (averages.current.glue.slow, glue);
  stats.learned.literals += size;
  stats.learned.clauses++;
  CADICAL_assert (glue < size);

  // up to this point lrat_chain contains the proof for current clause in
  // reversed order. in minimize and shrink the clause is changed and
  // therefore lrat_chain has to be extended. Unfortunately we cannot create
  // the chain directly during minimization (or shrinking) but afterwards we
  // can calculate it pretty easily and even better the same algorithm works
  // for both shrinking and minimization.

  // Minimize the 1st UIP clause as pioneered by Niklas Soerensson in
  // MiniSAT and described in our joint SAT'09 paper.
  //
  if (size > 1) {
    if (opts.shrink)
      shrink_and_minimize_clause ();
    else if (opts.minimize)
      minimize_clause ();

    size = (int) clause.size ();

    // Update decision heuristics.
    //
    if (opts.bump) {
      bump_also_all_reason_literals ();
      bump_variables ();
    }

    if (external->learner)
      external->export_learned_large_clause (clause);
  } else if (external->learner)
    external->export_learned_unit_clause (-uip);

  // Update actual size statistics.
  //
  stats.units += (size == 1);
  stats.binaries += (size == 2);
  UPDATE_AVERAGE (averages.current.size, size);

  // reverse lrat_chain. We could probably work with reversed iterators
  // (views) to be more efficient but we would have to distinguish in proof
  //
  if (lrat) {
    LOG (unit_chain, "unit chain: ");
    for (auto id : unit_chain)
      lrat_chain.push_back (id);
    unit_chain.clear ();
    reverse (lrat_chain.begin (), lrat_chain.end ());
  }

  // Determine back-jump level, learn driving clause, backtrack and assign
  // flipped 1st UIP literal.
  //
  int jump;
  Clause *driving_clause = new_driving_clause (glue, jump);
  UPDATE_AVERAGE (averages.current.jump, jump);

  int new_level = determine_actual_backtrack_level (jump);
  UPDATE_AVERAGE (averages.current.level, new_level);
  backtrack (new_level);

  // It should hold that (!level <=> size == 1)
  //                 and (!uip   <=> size == 0)
  // this means either we have already learned a clause => size >= 2
  // in this case we will not learn empty clause or unit here
  // or we haven't actually learned a clause in new_driving_clause
  // then lrat_chain is still valid and we will learn a unit or empty clause
  //
  if (uip) {
    search_assign_driving (-uip, driving_clause);
  } else
    learn_empty_clause ();

  if (stable)
    reluctant.tick (); // Reluctant has its own 'conflict' counter.

  // Clean up.
  //
  clear_analyzed_literals ();
  clear_unit_analyzed_literals ();
  clear_analyzed_levels ();
  clause.clear ();
  conflict = 0;

  lrat_chain.clear ();
  STOP (analyze);

  if (driving_clause && opts.eagersubsume)
    eagerly_subsume_recently_learned_clauses (driving_clause);

  if (lim.recompute_tier <= stats.conflicts)
    recompute_tier ();
}

// We wait reporting a learned unit until propagation of that unit is
// completed.  Otherwise the 'i' report gives the number of remaining
// variables before propagating the unit (and hides the actual remaining
// variables after propagating it).

void Internal::iterate () {
  iterating = false;
  report ('i');
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
