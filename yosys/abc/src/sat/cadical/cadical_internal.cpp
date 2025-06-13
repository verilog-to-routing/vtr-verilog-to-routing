#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/
static Clause external_reason_clause;

Internal::Internal ()
    : mode (SEARCH), unsat (false), iterating (false),
      localsearching (false), lookingahead (false), preprocessing (false),
      protected_reasons (false), force_saved_phase (false),
      searching_lucky_phases (false), stable (false), reported (false),
      external_prop (false), did_external_prop (false),
      external_prop_is_lazy (true), forced_backt_allowed (false),

      private_steps (false), rephased (0), vsize (0), max_var (0),
      clause_id (0), original_id (0), reserved_ids (0), conflict_id (0),
      saved_decisions (0), concluded (false), lrat (false), frat (false),
      level (0), vals (0), score_inc (1.0), scores (this), conflict (0),
      ignore (0), external_reason (&external_reason_clause),
      newest_clause (0), force_no_backtrack (false),
      from_propagator (false), ext_clause_forgettable (false),
      tainted_literal (0), notified (0), probe_reason (0), propagated (0),
      propagated2 (0), propergated (0), best_assigned (0),
      target_assigned (0), no_conflict_until (0), unsat_constraint (false),
      marked_failed (true), sweep_incomplete (false), citten (0),
      num_assigned (0), proof (0), opts (this),
#ifndef CADICAL_QUIET
      profiles (this), force_phase_messages (false),
#endif
      arena (this), prefix ("c "), internal (this), external (0),
      termination_forced (false), vars (this->max_var),
      lits (this->max_var) {
  control.push_back (Level (0, 0));

  // The 'dummy_binary' is used in 'try_to_subsume_clause' to fake a real
  // clause, which then can be used to subsume or strengthen the given
  // clause in one routine for both binary and non binary clauses.  This
  // fake binary clause is always kept non-redundant (and not-moved etc.)
  // due to the following 'memset'.  Only literals will be changed.

  // In a previous version we used local automatic allocated 'Clause' on the
  // stack, which became incompatible with several compilers (see the
  // discussion on flexible array member in 'Clause.cpp').

  size_t bytes = Clause::bytes (2);
  dummy_binary = (Clause *) new char[bytes];
  memset (dummy_binary, 0, bytes);
  dummy_binary->size = 2;
}

Internal::~Internal () {
  // If a memory exception ocurred a profile might still be active.
#ifndef CADICAL_QUIET
#define PROFILE(NAME, LEVEL) \
  if (PROFILE_ACTIVE (NAME)) \
    STOP (NAME);
  PROFILES
#undef PROFILE
#endif
  delete[] (char *) dummy_binary;
  for (const auto &c : clauses)
    delete_clause (c);
  if (proof)
    delete proof;
  for (auto &tracer : tracers)
    delete tracer;
  for (auto &filetracer : file_tracers)
    delete filetracer;
  for (auto &stattracer : stat_tracers)
    delete stattracer;
  if (vals) {
    vals -= vsize;
    delete[] vals;
  }
}

/*------------------------------------------------------------------------*/

// Values in 'vals' can be accessed in the range '[-max_var,max_var]' that
// is directly by a literal.  This is crucial for performance.  By shifting
// the start of 'vals' appropriately, we achieve that negative offsets from
// the start of 'vals' can be used.  We also need to set both values at
// 'lit' and '-lit' during assignments.  In MiniSAT integer literals are
// encoded, using the least significant bit as negation.  This avoids taking
// the 'abs ()' (as in our solution) and thus also avoids a branch in the
// hot-spot of the solver (clause traversal in propagation).  That solution
// requires another (branch less) negation of the values though and
// debugging is harder since literals occur only encoded in clauses.
// The main draw-back of our solution is that we have to shift the memory
// and access it through negative indices, which looks less clean (but still
// as far I can tell is properly defined C / C++).   You might get a warning
// by static analyzers though.  Clang with '--analyze' thought that this
// idiom would generate a memory leak thus we use the following dummy.

static signed char *ignore_clang_analyze_memory_leak_warning;

void Internal::enlarge_vals (size_t new_vsize) {
  signed char *new_vals;
  const size_t bytes = 2u * new_vsize;
  new_vals = new signed char[bytes]; // g++-4.8 does not like ... { 0 };
  memset (new_vals, 0, bytes);
  ignore_clang_analyze_memory_leak_warning = new_vals;
  new_vals += new_vsize;

  if (vals) {
    memcpy (new_vals - max_var, vals - max_var, 2u * max_var + 1u);
    vals -= vsize;
    delete[] vals;
  } else
    CADICAL_assert (!vsize);
  vals = new_vals;
}

/*------------------------------------------------------------------------*/

void Internal::enlarge (int new_max_var) {
  // New variables can be created that can invoke enlarge anytime (via calls
  // during ipasir-up call-backs), thus assuming (!level) is not correct
  size_t new_vsize = vsize ? 2 * vsize : 1 + (size_t) new_max_var;
  while (new_vsize <= (size_t) new_max_var)
    new_vsize *= 2;
  LOG ("enlarge internal size from %zd to new size %zd", vsize, new_vsize);
  // Ordered in the size of allocated memory (larger block first).
  if (lrat || frat)
    enlarge_zero (unit_clauses_idx, 2 * new_vsize);
  enlarge_only (wtab, 2 * new_vsize);
  enlarge_only (vtab, new_vsize);
  enlarge_zero (parents, new_vsize);
  enlarge_only (links, new_vsize);
  enlarge_zero (btab, new_vsize);
  enlarge_zero (gtab, new_vsize);
  enlarge_zero (stab, new_vsize);
  enlarge_init (ptab, 2 * new_vsize, -1);
  enlarge_only (ftab, new_vsize);
  enlarge_vals (new_vsize);
  vsize = new_vsize;
  if (external)
    enlarge_zero (relevanttab, new_vsize);
  const signed char val = opts.phase ? 1 : -1;
  enlarge_init (phases.saved, new_vsize, val);
  enlarge_zero (phases.forced, new_vsize);
  enlarge_zero (phases.target, new_vsize);
  enlarge_zero (phases.best, new_vsize);
  enlarge_zero (phases.prev, new_vsize);
  enlarge_zero (phases.min, new_vsize);
  enlarge_zero (marks, new_vsize);
}

void Internal::init_vars (int new_max_var) {
  if (new_max_var <= max_var)
    return;
  // New variables can be created that can invoke enlarge anytime (via calls
  // during ipasir-up call-backs), thus assuming (!level) is not correct
  LOG ("initializing %d internal variables from %d to %d",
       new_max_var - max_var, max_var + 1, new_max_var);
  if ((size_t) new_max_var >= vsize)
    enlarge (new_max_var);
#ifndef CADICAL_NDEBUG
  for (int64_t i = -new_max_var; i < -max_var; i++)
    CADICAL_assert (!vals[i]);
  for (unsigned i = max_var + 1; i <= (unsigned) new_max_var; i++)
    CADICAL_assert (!vals[i]), CADICAL_assert (!btab[i]), CADICAL_assert (!gtab[i]);
  for (uint64_t i = 2 * ((uint64_t) max_var + 1);
       i <= 2 * (uint64_t) new_max_var + 1; i++)
    CADICAL_assert (ptab[i] == -1);
#endif
  CADICAL_assert (!btab[0]);
  int old_max_var = max_var;
  max_var = new_max_var;
  init_queue (old_max_var, new_max_var);
  init_scores (old_max_var, new_max_var);
  int initialized = new_max_var - old_max_var;
  stats.vars += initialized;
  stats.unused += initialized;
  stats.inactive += initialized;
  LOG ("finished initializing %d internal variables", initialized);
}

void Internal::add_original_lit (int lit) {
  CADICAL_assert (abs (lit) <= max_var);
  if (lit) {
    original.push_back (lit);
  } else {
    const int64_t id =
        original_id < reserved_ids ? ++original_id : ++clause_id;
    if (proof) {
      // Use the external form of the clause for printing in proof
      // Externalize(internalized literal) != external literal
      CADICAL_assert (!original.size () || !external->eclause.empty ());
      proof->add_external_original_clause (id, false, external->eclause);
    }
    if (internal->opts.check &&
        (internal->opts.checkwitness || internal->opts.checkfailed)) {
      bool forgettable = from_propagator && ext_clause_forgettable;
      if (forgettable && opts.check) {
        CADICAL_assert (!original.size () || !external->eclause.empty ());

        // First integer is the presence-flag (even if the clause is empty)
        external->forgettable_original[id] = {1};

        for (auto const &elit : external->eclause)
          external->forgettable_original[id].push_back (elit);

        LOG (external->eclause,
             "clause added to external forgettable map:");
      }
    }

    add_new_original_clause (id);
    original.clear ();
  }
}

void Internal::finish_added_clause_with_id (int64_t id, bool restore) {
  if (proof) {
    // Use the external form of the clause for printing in proof
    // Externalize(internalized literal) != external literal
    CADICAL_assert (!original.size () || !external->eclause.empty ());
    proof->add_external_original_clause (id, false, external->eclause,
                                         restore);
  }
  add_new_original_clause (id);
  original.clear ();
}

/*------------------------------------------------------------------------*/

void Internal::reserve_ids (int number) {
  // return;
  LOG ("reserving %d ids", number);
  CADICAL_assert (number >= 0);
  CADICAL_assert (!clause_id && !reserved_ids && !original_id);
  clause_id = reserved_ids = number;
  if (proof)
    proof->begin_proof (reserved_ids);
}

/*------------------------------------------------------------------------*/

#ifdef PROFILE_MODE

// Separating these makes it easier to profile stable and unstable search.

bool Internal::propagate_wrapper () {
  if (stable)
    return propagate_stable ();
  else
    return propagate_unstable ();
}

void Internal::analyze_wrapper () {
  if (stable)
    analyze_stable ();
  else
    analyze_unstable ();
}

int Internal::decide_wrapper () {
  if (stable)
    return decide_stable ();
  else
    return decide_unstable ();
}

#endif

/*------------------------------------------------------------------------*/

// This is the main CDCL loop with interleaved inprocessing.

int Internal::cdcl_loop_with_inprocessing () {

  int res = 0;

  START (search);

  if (stable) {
    START (stable);
    report ('[');
  } else {
    START (unstable);
    report ('{');
  }

  while (!res) {
    if (unsat)
      res = 20;
    else if (unsat_constraint)
      res = 20;
    else if (!propagate_wrapper ())
      analyze_wrapper (); // propagate and analyze
    else if (iterating)
      iterate ();                               // report learned unit
    else if (!external_propagate () || unsat) { // external propagation
      if (unsat)
        continue;
      else
        analyze ();
    } else if (satisfied ()) { // found model
      if (!external_check_solution () || unsat) {
        if (unsat)
          continue;
        else
          analyze ();
      } else if (satisfied ())
        res = 10;
    } else if (search_limits_hit ())
      break;                               // decision or conflict limit
    else if (terminated_asynchronously ()) // externally terminated
      break;
    else if (restarting ())
      restart (); // restart by backtracking
    else if (rephasing ())
      rephase (); // reset variable phases
    else if (reducing ())
      reduce (); // collect useless clauses
    else if (inprobing ())
      inprobe (); // schedule of inprocessing
    else if (ineliminating ())
      elim (); // variable elimination
    else if (compacting ())
      compact (); // collect variables
    else if (conditioning ())
      condition (); // globally blocked clauses
    else
      res = decide (); // next decision
  }

  if (stable) {
    STOP (stable);
    report (']');
  } else {
    STOP (unstable);
    report ('}');
  }

  STOP (search);

  return res;
}

int Internal::propagate_assumptions () {
  if (proof)
    proof->solve_query ();
  if (opts.ilb) {
    if (opts.ilbassumptions)
      sort_and_reuse_assumptions ();
    stats.ilbtriggers++;
    stats.ilbsuccess += (level > 0);
    stats.levelsreused += level;
    if (level) {
      CADICAL_assert (control.size () > 1);
      stats.literalsreused += num_assigned - control[1].trail;
    }
  }
  init_search_limits ();
  init_report_limits ();

  int res = already_solved (); // root-level propagation is done here

  int last_assumption_level = assumptions.size ();
  if (constraint.size ())
    last_assumption_level++;

  if (!res) {
    restore_clauses ();
    while (!res) {
      if (unsat)
        res = 20;
      else if (unsat_constraint)
        res = 20;
      else if (!propagate ()) {
        // let analyze run to get failed assumptions
        analyze ();
      } else if (!external_propagate () || unsat) { // external propagation
        if (unsat)
          continue;
        else
          analyze ();
      } else if (satisfied ()) { // found model
        if (!external_check_solution () || unsat) {
          if (unsat)
            continue;
          else
            analyze ();
        } else if (satisfied ())
          res = 10;
      } else if (search_limits_hit ())
        break;                               // decision or conflict limit
      else if (terminated_asynchronously ()) // externally terminated
        break;
      else {
        if (level >= last_assumption_level)
          break;
        res = decide ();
      }
    }
  }

  if (unsat || unsat_constraint)
    res = 20;

  if (!res && satisfied ())
    res = 10;

  finalize (res);
  reset_solving ();
  report_solving (res);

  return res;
}

void Internal::implied (std::vector<int> &entrailed) {
  int last_assumption_level = assumptions.size ();
  if (constraint.size ())
    last_assumption_level++;
  
  size_t trail_limit = trail.size();
  if (level > last_assumption_level) 
    trail_limit = control[last_assumption_level + 1].trail;

  for (size_t i = 0; i < trail_limit; i++)
    entrailed.push_back (trail[i]);
}

/*------------------------------------------------------------------------*/

// Most of the limits are only initialized in the first 'solve' call and
// increased as in a stand-alone non-incremental SAT call except for those
// explicitly marked as being reset below.

void Internal::init_report_limits () {
  reported = false;
  lim.report = 0;
  lim.recompute_tier = 5000;
}

void Internal::init_preprocessing_limits () {

  const bool incremental = lim.initialized;
  if (incremental)
    LOG ("reinitializing preprocessing limits incrementally");
  else
    LOG ("initializing preprocessing limits and increments");

  const char *mode = 0;

  /*----------------------------------------------------------------------*/

  if (incremental)
    mode = "keeping";
  else {
    last.elim.marked = -1;
    lim.elim = stats.conflicts + scale (opts.elimint);
    mode = "initial";
  }
  (void) mode;
  LOG ("%s elim limit %" PRId64 " after %" PRId64 " conflicts", mode,
       lim.elim, lim.elim - stats.conflicts);

  // Initialize and reset elimination bounds in any case.

  lim.elimbound = opts.elimboundmin;
  LOG ("elimination bound %" PRId64 "", lim.elimbound);

  /*----------------------------------------------------------------------*/

  if (!incremental) {

    last.ternary.marked = -1; // TODO this should not be necessary...

    lim.compact = stats.conflicts + opts.compactint;
    LOG ("initial compact limit %" PRId64 " increment %" PRId64 "",
         lim.compact, lim.compact - stats.conflicts);
  }

  /*----------------------------------------------------------------------*/

  if (incremental)
    mode = "keeping";
  else {
    double delta = log10 (stats.added.irredundant);
    delta = delta * delta;
    lim.inprobe = stats.conflicts + opts.inprobeint * delta;
    mode = "initial";
  }
  (void) mode;
  LOG ("%s probe limit %" PRId64 " after %" PRId64 " conflicts", mode,
       lim.inprobe, lim.inprobe - stats.conflicts);

  /*----------------------------------------------------------------------*/

  if (incremental)
    mode = "keeping";
  else {
    lim.condition = stats.conflicts + opts.conditionint;
    mode = "initial";
  }
  LOG ("%s condition limit %" PRId64 " increment %" PRId64, mode,
       lim.condition, lim.condition - stats.conflicts);

  /*----------------------------------------------------------------------*/

  // Initial preprocessing rounds.

  if (inc.preprocessing <= 0) {
    lim.preprocessing = 0;
    LOG ("no preprocessing");
  } else {
    lim.preprocessing = inc.preprocessing;
    LOG ("limiting to %" PRId64 " preprocessing rounds", lim.preprocessing);
  }
}

void Internal::init_search_limits () {

  const bool incremental = lim.initialized;
  if (incremental)
    LOG ("reinitializing search limits incrementally");
  else
    LOG ("initializing search limits and increments");

  const char *mode = 0;

  /*----------------------------------------------------------------------*/

  if (incremental)
    mode = "keeping";
  else {
    last.reduce.conflicts = -1;
    lim.reduce = stats.conflicts + opts.reduceinit;
    mode = "initial";
  }
  (void) mode;
  LOG ("%s reduce limit %" PRId64 " after %" PRId64 " conflicts", mode,
       lim.reduce, lim.reduce - stats.conflicts);

  /*----------------------------------------------------------------------*/

  if (incremental)
    mode = "keeping";
  else {
    lim.flush = opts.flushint;
    inc.flush = opts.flushint;
    mode = "initial";
  }
  (void) mode;
  LOG ("%s flush limit %" PRId64 " interval %" PRId64 "", mode, lim.flush,
       inc.flush);

  /*----------------------------------------------------------------------*/

  // Initialize or reset 'rephase' limits in any case.

  lim.rephase = stats.conflicts + opts.rephaseint;
  lim.rephased[0] = lim.rephased[1] = 0;
  LOG ("new rephase limit %" PRId64 " after %" PRId64 " conflicts",
       lim.rephase, lim.rephase - stats.conflicts);

  /*----------------------------------------------------------------------*/

  // Initialize or reset 'restart' limits in any case.

  lim.restart = stats.conflicts + opts.restartint;
  LOG ("new restart limit %" PRId64 " increment %" PRId64 "", lim.restart,
       lim.restart - stats.conflicts);

  /*----------------------------------------------------------------------*/

  if (!incremental) {
    stable = opts.stabilize && opts.stabilizeonly;
    if (stable)
      LOG ("starting in always forced stable phase");
    else
      LOG ("starting in default non-stable phase");
    init_averages ();
  } else if (opts.stabilize && opts.stabilizeonly) {
    LOG ("keeping always forced stable phase");
    CADICAL_assert (stable);
  } else if (stable) {
    LOG ("switching back to default non-stable phase");
    stable = false;
    swap_averages ();
  } else
    LOG ("keeping non-stable phase");

  if (!incremental) {
    inc.stabilize = 0;
    lim.stabilize = stats.conflicts + opts.stabilizeinit;
    LOG ("initial stabilize limit %" PRId64 " after %d conflicts",
         lim.stabilize, (int) opts.stabilizeinit);
  }

  if (opts.stabilize && opts.reluctant) {
    LOG ("new restart reluctant doubling sequence period %d",
         opts.reluctant);
    reluctant.enable (opts.reluctant, opts.reluctantmax);
  } else
    reluctant.disable ();

  /*----------------------------------------------------------------------*/

  // Conflict and decision limits.

  if (inc.conflicts < 0) {
    lim.conflicts = -1;
    LOG ("no limit on conflicts");
  } else {
    lim.conflicts = stats.conflicts + inc.conflicts;
    LOG ("conflict limit after %" PRId64 " conflicts at %" PRId64
         " conflicts",
         inc.conflicts, lim.conflicts);
  }

  if (inc.decisions < 0) {
    lim.decisions = -1;
    LOG ("no limit on decisions");
  } else {
    lim.decisions = stats.decisions + inc.decisions;
    LOG ("conflict limit after %" PRId64 " decisions at %" PRId64
         " decisions",
         inc.decisions, lim.decisions);
  }

  /*----------------------------------------------------------------------*/

  // Initial preprocessing rounds.

  if (inc.localsearch <= 0) {
    lim.localsearch = 0;
    LOG ("no local search");
  } else {
    lim.localsearch = inc.localsearch;
    LOG ("limiting to %" PRId64 " local search rounds", lim.localsearch);
  }

  /*----------------------------------------------------------------------*/

  lim.initialized = true;
}

/*------------------------------------------------------------------------*/

bool Internal::preprocess_round (int round) {
  (void) round;
  if (unsat)
    return false;
  if (!max_var)
    return false;
  START (preprocess);
  struct {
    int64_t vars, clauses;
  } before, after;
  before.vars = active ();
  before.clauses = stats.current.irredundant;
  stats.preprocessings++;
  CADICAL_assert (!preprocessing);
  preprocessing = true;
  PHASE ("preprocessing", stats.preprocessings,
         "starting round %d with %" PRId64 " variables and %" PRId64
         " clauses",
         round, before.vars, before.clauses);
  int old_elimbound = lim.elimbound;
  if (opts.inprobing)
    inprobe (false);
  if (opts.elim)
    elim (false);
  if (opts.condition)
    condition (false);

  after.vars = active ();
  after.clauses = stats.current.irredundant;
  CADICAL_assert (preprocessing);
  preprocessing = false;
  PHASE ("preprocessing", stats.preprocessings,
         "finished round %d with %" PRId64 " variables and %" PRId64
         " clauses",
         round, after.vars, after.clauses);
  STOP (preprocess);
  report ('P');
  if (unsat)
    return false;
  if (after.vars < before.vars)
    return true;
  if (old_elimbound < lim.elimbound)
    return true;
  return false;
}

// for now counts as one of the preprocessing rounds TODO: change this?
void Internal::preprocess_quickly () {
  if (unsat)
    return;
  if (!max_var)
    return;
  if (!opts.preprocesslight)
    return;
  START (preprocess);
  struct {
    int64_t vars, clauses;
  } before, after;
  before.vars = active ();
  before.clauses = stats.current.irredundant;
  // stats.preprocessings++;
  CADICAL_assert (!preprocessing);
  preprocessing = true;
  PHASE ("preprocessing", stats.preprocessings,
         "starting with %" PRId64 " variables and %" PRId64 " clauses",
         before.vars, before.clauses);

  if (extract_gates ())
    decompose ();

  if (sweep ())
    decompose ();

  if (opts.factor)
    factor ();

  if (opts.fastelim)
    elimfast ();
  // if (opts.condition)
  // condition (false);
  after.vars = active ();
  after.clauses = stats.current.irredundant;
  CADICAL_assert (preprocessing);
  preprocessing = false;
  PHASE ("preprocessing", stats.preprocessings,
         "finished with %" PRId64 " variables and %" PRId64 " clauses",
         after.vars, after.clauses);
  STOP (preprocess);
  report ('P');
}

int Internal::preprocess () {
  preprocess_quickly ();
  for (int i = 0; i < lim.preprocessing; i++)
    if (!preprocess_round (i))
      break;
  if (unsat)
    return 20;
  return 0;
}

/*------------------------------------------------------------------------*/

int Internal::try_to_satisfy_formula_by_saved_phases () {
  LOG ("satisfying formula by saved phases");
  CADICAL_assert (!level);
  CADICAL_assert (!force_saved_phase);
  CADICAL_assert (propagated == trail.size ());
  force_saved_phase = true;
  if (external_prop) {
    CADICAL_assert (!level);
    LOG ("external notifications are turned off during preprocessing.");
    private_steps = true;
  }
  int res = 0;
  while (!res) {
    if (satisfied ()) {
      LOG ("formula indeed satisfied by saved phases");
      res = 10;
    } else if (decide ()) {
      LOG ("inconsistent assumptions with redundant clauses and phases");
      res = 20;
    } else if (!propagate ()) {
      LOG ("saved phases do not satisfy redundant clauses");
      CADICAL_assert (level > 0);
      backtrack ();
      conflict = 0; // ignore conflict
      CADICAL_assert (!res);
      break;
    }
  }
  CADICAL_assert (force_saved_phase);
  force_saved_phase = false;
  if (external_prop) {
    private_steps = false;
    LOG ("external notifications are turned back on.");
    if (!level)
      notify_assignments (); // In case fixed assignments were found.
    else {
      renotify_trail_after_local_search ();
    }
  }
  return res;
}

/*------------------------------------------------------------------------*/

void Internal::produce_failed_assumptions () {
  LOG ("producing failed assumptions");
  CADICAL_assert (!level);
  CADICAL_assert (!assumptions.empty ());
  while (!unsat) {
    CADICAL_assert (!satisfied ());
    notify_assignments ();
    if (decide ())
      break;
    while (!unsat && !propagate ())
      analyze ();
  }
  notify_assignments ();
  if (unsat)
    LOG ("formula is actually unsatisfiable unconditionally");
  else
    LOG ("assumptions indeed failing");
}

/*------------------------------------------------------------------------*/

int Internal::local_search_round (int round) {

  CADICAL_assert (round > 0);

  if (unsat)
    return false;
  if (!max_var)
    return false;

  START_OUTER_WALK ();
  CADICAL_assert (!localsearching);
  localsearching = true;

  // Determine propagation limit quadratically scaled with rounds.
  //
  int64_t limit = opts.walkmineff;
  limit *= round;
  if (LONG_MAX / round > limit)
    limit *= round;
  else
    limit = LONG_MAX;

  int res = walk_round (limit, true);

  CADICAL_assert (localsearching);
  localsearching = false;
  STOP_OUTER_WALK ();

  report ('L');

  return res;
}

int Internal::local_search () {

  if (unsat)
    return 0;
  if (!max_var)
    return 0;
  if (!opts.walk)
    return 0;
  if (constraint.size ())
    return 0;

  int res = 0;

  for (int i = 1; !res && i <= lim.localsearch; i++)
    res = local_search_round (i);

  if (res == 10) {
    LOG ("local search determined formula to be satisfiable");
    CADICAL_assert (!stats.walk.minimum);
    res = try_to_satisfy_formula_by_saved_phases ();
  } else if (res == 20) {
    LOG ("local search determined assumptions to be inconsistent");
    CADICAL_assert (!assumptions.empty ());
    produce_failed_assumptions ();
  }

  return res;
}

/*------------------------------------------------------------------------*/

// if preprocess_only is false and opts.ilb is true we do not preprocess
// such that we do not have to backtrack to level 0.
//
int Internal::solve (bool preprocess_only) {
  CADICAL_assert (clause.empty ());
  START (solve);
  if (proof)
    proof->solve_query ();
  if (opts.ilb) {
    if (opts.ilbassumptions)
      sort_and_reuse_assumptions ();
    stats.ilbtriggers++;
    stats.ilbsuccess += (level > 0);
    stats.levelsreused += level;
    if (level) {
      CADICAL_assert (control.size () > 1);
      stats.literalsreused += num_assigned - control[1].trail;
    }
    if (external->propagator)
      renotify_trail_after_ilb ();
  }
  if (preprocess_only)
    LOG ("internal solving in preprocessing only mode");
  else
    LOG ("internal solving in full mode");
  init_report_limits ();
  int res = already_solved ();
  if (!res && preprocess_only && level)
    backtrack ();
  if (!res)
    res = restore_clauses ();
  if (!res || (res == 10 && external_prop)) {
    init_preprocessing_limits ();
    if (!preprocess_only)
      init_search_limits ();
  }
  if (!preprocess_only) {
    if (!res && !level)
      res = local_search ();
  }
  if (!res && !level)
    res = preprocess ();
  if (!preprocess_only) {
    if (!res && !level)
      res = local_search ();
    if (!res && !level)
      res = lucky_phases ();
    if (!res || (res == 10 && external_prop)) {
      if (res == 10 && external_prop && level)
        backtrack ();
      res = cdcl_loop_with_inprocessing ();
    }
  }
  finalize (res);
  reset_solving ();
  report_solving (res);
  STOP (solve);
  return res;
}

int Internal::already_solved () {
  int res = 0;
  if (unsat || unsat_constraint) {
    LOG ("already inconsistent");
    res = 20;
  } else {
    if (level && !opts.ilb)
      backtrack ();
    if (!level && !propagate ()) {
      LOG ("root level propagation produces conflict");
      learn_empty_clause ();
      res = 20;
    }
    if (max_var == 0 && res == 0)
      res = 10;
  }
  return res;
}
void Internal::report_solving (int res) {
  if (res == 10)
    report ('1');
  else if (res == 20)
    report ('0');
  else
    report ('?');
}

void Internal::reset_solving () {
  if (termination_forced) {

    // TODO this leads potentially to a data race if the external
    // user is calling 'terminate' twice within one 'solve' call.
    // A proper solution would be to guard / protect setting the
    // 'termination_forced' flag and only allow it during solving and
    // ignore it otherwise thus also the second time it is called during a
    // 'solve' call.  We could move resetting it also the start of
    // 'solve'.
    //
    termination_forced = false;

    LOG ("reset forced termination");
  }
}

int Internal::restore_clauses () {
  int res = 0;
  if (opts.restoreall <= 1 && external->tainted.empty ()) {
    LOG ("no tainted literals and nothing to restore");
    report ('*');
  } else {
    report ('+');
    // remove_garbage_binaries ();
    external->restore_clauses ();
    internal->report ('r');
    if (!unsat && !level && !propagate ()) {
      LOG ("root level propagation after restore produces conflict");
      learn_empty_clause ();
      res = 20;
    }
  }
  return res;
}

int Internal::lookahead () {
  CADICAL_assert (clause.empty ());
  START (lookahead);
  CADICAL_assert (!lookingahead);
  lookingahead = true;
  if (external_prop) {
    if (level) {
      // Combining lookahead with external propagator is limited
      // Note that lookahead_probing (); would also force backtrack anyway
      backtrack ();
    }
    LOG ("external notifications are turned off during preprocessing.");
    private_steps = true;
  }
  int tmp = already_solved ();
  if (!tmp)
    tmp = restore_clauses ();
  int res = 0;
  if (!tmp)
    res = lookahead_probing ();
  if (res == INT_MIN)
    res = 0;
  reset_solving ();
  report_solving (tmp);
  CADICAL_assert (lookingahead);
  lookingahead = false;
  STOP (lookahead);
  if (external_prop) {
    private_steps = false;
    LOG ("external notifications are turned back on.");
    notify_assignments (); // In case fixed assignments were found.
  }
  return res;
}

/*------------------------------------------------------------------------*/

void Internal::finalize (int res) {
  if (!proof)
    return;
  LOG ("finalizing");
  // finalize external units
  if (frat) {
    for (const auto &evar : external->vars) {
      CADICAL_assert (evar > 0);
      const auto eidx = 2 * evar;
      int sign = 1;
      int64_t id = external->ext_units[eidx];
      if (!id) {
        sign = -1;
        id = external->ext_units[eidx + 1];
      }
      if (id) {
        proof->finalize_external_unit (id, evar * sign);
      }
    }
    // finalize internal units
    for (const auto &lit : lits) {
      const auto elit = externalize (lit);
      if (elit) {
        const unsigned eidx = (elit < 0) + 2u * (unsigned) abs (elit);
        const int64_t id = external->ext_units[eidx];
        if (id) {
          CADICAL_assert (unit_clauses (vlit (lit)) == id);
          continue;
        }
      }
      const int64_t id = unit_clauses (vlit (lit));
      if (!id)
        continue;
      proof->finalize_unit (id, lit);
    }
    // See the discussion in 'propagate' on why garbage binary clauses stick
    // around.
    for (const auto &c : clauses)
      if (!c->garbage || (c->size == 2 && !c->flushed))
        proof->finalize_clause (c);

    // finalize conflict and proof
    if (conflict_id) {
      proof->finalize_clause (conflict_id, {});
    }
  }
  proof->report_status (res, conflict_id);
  if (res == 10)
    external->conclude_sat ();
  else if (res == 20)
    conclude_unsat ();
  else if (!res)
    external->conclude_unknown ();
}

/*------------------------------------------------------------------------*/

void Internal::print_statistics () {
  stats.print (this);
  for (auto &st : stat_tracers)
    st->print_stats ();
}

/*------------------------------------------------------------------------*/

// Only useful for debugging purposes.

void Internal::dump (Clause *c) {
  for (const auto &lit : *c)
    printf ("%d ", lit);
  printf ("0\n");
}

void Internal::dump () {
  int64_t m = assumptions.size ();
  for (auto idx : vars)
    if (fixed (idx))
      m++;
  for (const auto &c : clauses)
    if (!c->garbage)
      m++;
  printf ("p cnf %d %" PRId64 "\n", max_var, m);
  for (auto idx : vars) {
    const int tmp = fixed (idx);
    if (tmp)
      printf ("%d 0\n", tmp < 0 ? -idx : idx);
  }
  for (const auto &c : clauses)
    if (!c->garbage)
      dump (c);
  for (const auto &lit : assumptions)
    printf ("%d 0\n", lit);
  fflush (stdout);
}

/*------------------------------------------------------------------------*/

bool Internal::traverse_constraint (ClauseIterator &it) {
  if (constraint.empty () && !unsat_constraint)
    return true;

  vector<int> eclause;
  if (unsat)
    return it.clause (eclause);

  LOG (constraint, "traversing constraint");
  bool satisfied = false;
  for (auto ilit : constraint) {
    const int tmp = fixed (ilit);
    if (tmp > 0) {
      satisfied = true;
      break;
    }
    if (tmp < 0)
      continue;
    const int elit = externalize (ilit);
    eclause.push_back (elit);
  }
  if (!satisfied && !it.clause (eclause))
    return false;

  return true;
}
/*------------------------------------------------------------------------*/

bool Internal::traverse_clauses (ClauseIterator &it) {
  vector<int> eclause;
  if (unsat)
    return it.clause (eclause);
  for (const auto &c : clauses) {
    if (c->garbage)
      continue;
    if (c->redundant)
      continue;
    bool satisfied = false;
    for (const auto &ilit : *c) {
      const int tmp = fixed (ilit);
      if (tmp > 0) {
        satisfied = true;
        break;
      }
      if (tmp < 0)
        continue;
      const int elit = externalize (ilit);
      eclause.push_back (elit);
    }
    if (!satisfied && !it.clause (eclause))
      return false;
    eclause.clear ();
  }
  return true;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
