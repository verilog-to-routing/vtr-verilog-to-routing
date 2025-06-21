#include "global.h"

#include "internal.hpp"
#include <cstdint>

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

External::External (Internal *i)
    : internal (i), max_var (0), vsize (0), extended (false),
      concluded (false), terminator (0), learner (0), fixed_listener (0),
      propagator (0), solution (0), vars (max_var) {
  CADICAL_assert (internal);
  CADICAL_assert (!internal->external);
  internal->external = this;
}

External::~External () {
  if (solution)
    delete[] solution;
}

void External::enlarge (int new_max_var) {

  CADICAL_assert (!extended);

  size_t new_vsize = vsize ? 2 * vsize : 1 + (size_t) new_max_var;
  while (new_vsize <= (size_t) new_max_var)
    new_vsize *= 2;
  LOG ("enlarge external size from %zd to new size %zd", vsize, new_vsize);
  vsize = new_vsize;
}

void External::init (int new_max_var, bool extension) {
  CADICAL_assert (!extended);
  if (new_max_var <= max_var)
    return;
  int new_vars = new_max_var - max_var;
  int old_internal_max_var = internal->max_var;
  int new_internal_max_var = old_internal_max_var + new_vars;
  internal->init_vars (new_internal_max_var);
  if ((size_t) new_max_var >= vsize)
    enlarge (new_max_var);
  LOG ("initialized %d external variables", new_vars);
  if (!max_var) {
    CADICAL_assert (e2i.empty ());
    e2i.push_back (0);
    ext_units.push_back (0);
    ext_units.push_back (0);
    ext_flags.push_back (0);
    ervars.push_back (0);
    CADICAL_assert (internal->i2e.empty ());
    internal->i2e.push_back (0);
  } else {
    CADICAL_assert (e2i.size () == (size_t) max_var + 1);
    CADICAL_assert (internal->i2e.size () == (size_t) old_internal_max_var + 1);
  }
  unsigned iidx = old_internal_max_var + 1, eidx;
  for (eidx = max_var + 1u; eidx <= (unsigned) new_max_var;
       eidx++, iidx++) {
    LOG ("mapping external %u to internal %u", eidx, iidx);
    CADICAL_assert (e2i.size () == eidx);
    e2i.push_back (iidx);
    ext_units.push_back (0);
    ext_units.push_back (0);
    ext_flags.push_back (0);
    ervars.push_back (0);
    internal->i2e.push_back (eidx);
    CADICAL_assert (internal->i2e[iidx] == (int) eidx);
    CADICAL_assert (e2i[eidx] == (int) iidx);
  }
  if (extension)
    internal->stats.variables_extension += new_vars;
  else
    internal->stats.variables_original += new_vars;
  if (new_max_var >= (int64_t) is_observed.size ())
    is_observed.resize (1 + (size_t) new_max_var, false);
  if (internal->opts.checkfrozen)
    if (new_max_var >= (int64_t) moltentab.size ())
      moltentab.resize (1 + (size_t) new_max_var, false);
  CADICAL_assert (iidx == (size_t) new_internal_max_var + 1);
  CADICAL_assert (eidx == (size_t) new_max_var + 1);
  CADICAL_assert (ext_units.size () == (size_t) new_max_var * 2 + 2);
  max_var = new_max_var;
}

/*------------------------------------------------------------------------*/

void External::reset_assumptions () {
  assumptions.clear ();
  internal->reset_assumptions ();
}

void External::reset_concluded () {
  concluded = false;
  internal->reset_concluded ();
}

void External::reset_constraint () {
  constraint.clear ();
  internal->reset_constraint ();
}

void External::reset_extended () {
  if (!extended)
    return;
  LOG ("reset extended");
  extended = false;
}

void External::reset_limits () { internal->reset_limits (); }

/*------------------------------------------------------------------------*/

// when extension is true, elit should be a fresh variable and
// we can set a flag that it is an extension variable.
// This is then used in the API contracts, that extension variables are
// never part of the input
int External::internalize (int elit, bool extension) {
  int ilit;
  if (elit) {
    CADICAL_assert (elit != INT_MIN);
    const int eidx = abs (elit);
    if (extension && eidx <= max_var)
      FATAL ("can not add a definition for an already used variable %d",
             eidx);
    if (eidx > max_var) {
      init (eidx, extension);
    }
    if (extension) {
      CADICAL_assert (ervars.size () > (size_t) eidx);
      ervars[eidx] = true;
    }
    ilit = e2i[eidx];
    if (elit < 0)
      ilit = -ilit;
    if (!ilit) {
      CADICAL_assert (internal->max_var < INT_MAX);
      ilit = internal->max_var + 1u;
      internal->init_vars (ilit);
      e2i[eidx] = ilit;
      LOG ("mapping external %d to internal %d", eidx, ilit);
      e2i[eidx] = ilit;
      internal->i2e.push_back (eidx);
      CADICAL_assert (internal->i2e[ilit] == eidx);
      CADICAL_assert (e2i[eidx] == ilit);
      if (elit < 0)
        ilit = -ilit;
    }
    if (internal->opts.checkfrozen) {
      CADICAL_assert (eidx < (int64_t) moltentab.size ());
      if (moltentab[eidx])
        FATAL ("can not reuse molten literal %d", eidx);
    }
    Flags &f = internal->flags (ilit);
    if (f.status == Flags::UNUSED)
      internal->mark_active (ilit);
    else if (f.status != Flags::ACTIVE && f.status != Flags::FIXED)
      internal->reactivate (ilit);
    if (!marked (tainted, elit) && marked (witness, -elit)) {
      CADICAL_assert (!internal->opts.checkfrozen);
      LOG ("marking tainted %d", elit);
      mark (tainted, elit);
    }
  } else
    ilit = 0;
  return ilit;
}

void External::add (int elit) {
  CADICAL_assert (elit != INT_MIN);
  reset_extended ();

  bool forgettable = false;

  if (internal->opts.check &&
      (internal->opts.checkwitness || internal->opts.checkfailed)) {

    forgettable =
        internal->from_propagator && internal->ext_clause_forgettable;

    // Forgettable clauses (coming from the external propagator) are not
    // saved into the external 'original' stack. They are stored separately
    // in external 'forgettable_original', from where they are deleted when
    // the corresponding clause is deleted (actually deleted, not just
    // marked as garbage).
    if (!forgettable)
      original.push_back (elit);
  }

  const int ilit = internalize (elit);
  CADICAL_assert (!elit == !ilit);

  // The external literals of the new clause must be saved for later
  // when the proof is printed during add_original_lit (0)
  if (elit && (internal->proof || forgettable)) {
    eclause.push_back (elit);
    if (internal->lrat) {
      // actually find unit of -elit (flips elit < 0)
      unsigned eidx = (elit > 0) + 2u * (unsigned) abs (elit);
      CADICAL_assert ((size_t) eidx < ext_units.size ());
      const int64_t id = ext_units[eidx];
      bool added = ext_flags[abs (elit)];
      if (id && !added) {
        ext_flags[abs (elit)] = true;
        internal->lrat_chain.push_back (id);
      }
    }
  }

  if (!elit && internal->proof && internal->lrat) {
    for (const auto &elit : eclause) {
      ext_flags[abs (elit)] = false;
    }
  }

  if (elit)
    LOG ("adding external %d as internal %d", elit, ilit);
  internal->add_original_lit (ilit);

  // Clean-up saved external literals once proof line is printed
  if (!elit && (internal->proof || forgettable))
    eclause.clear ();
}

void External::assume (int elit) {
  CADICAL_assert (elit);
  reset_extended ();
  if (internal->proof)
    internal->proof->add_assumption (elit);
  assumptions.push_back (elit);
  const int ilit = internalize (elit);
  CADICAL_assert (ilit);
  LOG ("assuming external %d as internal %d", elit, ilit);
  internal->assume (ilit);
}

bool External::flip (int elit) {
  CADICAL_assert (elit);
  CADICAL_assert (elit != INT_MIN);
  CADICAL_assert (!propagator);

  int eidx = abs (elit);
  if (eidx > max_var)
    return false;
  if (marked (witness, elit))
    return false;
  int ilit = e2i[eidx];
  if (!ilit)
    return false;
  bool res = internal->flip (ilit);
  if (res && extended)
    reset_extended ();
  return res;
}

bool External::flippable (int elit) {
  CADICAL_assert (elit);
  CADICAL_assert (elit != INT_MIN);
  CADICAL_assert (!propagator);

  int eidx = abs (elit);
  if (eidx > max_var)
    return false;
  if (marked (witness, elit))
    return false;
  int ilit = e2i[eidx];
  if (!ilit)
    return false;
  return internal->flippable (ilit);
}

bool External::failed (int elit) {
  CADICAL_assert (elit);
  CADICAL_assert (elit != INT_MIN);
  int eidx = abs (elit);
  if (eidx > max_var)
    return 0;
  int ilit = e2i[eidx];
  if (!ilit)
    return 0;
  if (elit < 0)
    ilit = -ilit;
  return internal->failed (ilit);
}

void External::constrain (int elit) {
  if (constraint.size () && !constraint.back ()) {
    LOG (constraint, "replacing previous constraint");
    reset_constraint ();
  }
  CADICAL_assert (elit != INT_MIN);
  reset_extended ();
  const int ilit = internalize (elit);
  CADICAL_assert (!elit == !ilit);
  if (elit)
    LOG ("adding external %d as internal %d to constraint", elit, ilit);
  else if (!elit && internal->proof) {
    internal->proof->add_constraint (constraint);
  }
  constraint.push_back (elit);
  internal->constrain (ilit);
}

bool External::failed_constraint () {
  return internal->failed_constraint ();
}

void External::phase (int elit) {
  CADICAL_assert (elit);
  CADICAL_assert (elit != INT_MIN);
  const int ilit = internalize (elit);
  internal->phase (ilit);
}

void External::unphase (int elit) {
  CADICAL_assert (elit);
  CADICAL_assert (elit != INT_MIN);
  int eidx = abs (elit);
  if (eidx > max_var) {
  UNUSED:
    LOG ("resetting forced phase of unused external %d ignored", elit);
    return;
  }
  int ilit = e2i[eidx];
  if (!ilit)
    goto UNUSED;
  if (elit < 0)
    ilit = -ilit;
  internal->unphase (ilit);
}

/*------------------------------------------------------------------------*/

// External propagation related functions
//
// Note that when an already assigned variable is added as observed, the
// solver will backtrack to undo this assignment.
//
void External::add_observed_var (int elit) {
  if (!propagator) {
    LOG ("No connected propagator that could observe the variable, "
         "observed flag is not set.");
    return;
  }

  CADICAL_assert (elit);
  CADICAL_assert (elit != INT_MIN);
  reset_extended (); // tainting!

  int eidx = abs (elit);
  if (eidx <= max_var &&
      (marked (witness, elit) || marked (witness, -elit))) {
    LOG ("Error, only clean variables are allowed to become observed.");
    CADICAL_assert (false);

    // TODO: here needs to come the taint and restore of the newly
    // observed variable. Restore_clauses must be called before continue.
    // LOG ("marking tainted %d", elit);
    // mark (tainted, elit);
    // mark (tainted, -elit);
    // restore_clauses ...
  }

  if (eidx >= (int64_t) is_observed.size ())
    is_observed.resize (1 + (size_t) eidx, false);

  if (is_observed[eidx])
    return;

  LOG ("marking %d as externally watched", eidx);

  // Will do the necessary internalization
  freeze (elit);
  is_observed[eidx] = true;

  int ilit = internalize (elit);
  // internal add-observed-var backtracks to a lower decision level to
  // unassign the variable in case it was already assigned previously (but
  // not on the current level)
  internal->add_observed_var (ilit);

  if (propagator->is_lazy)
    return;

  // In case this variable was already assigned (e.g. via unit clause) and
  // got compacted to map to another (not observed) variable, it can not be
  // unnasigned so it must be notified explicitly now. (-> Can lead to
  // repeated fixed assignment notifications, in case it was unobserved and
  // observed again. But a repeated notification is less error-prone than
  // never notifying an assignment.)
  const int tmp = fixed (elit);
  if (!tmp)
    return;
  int unit = tmp < 0 ? -elit : elit;

  LOG ("notify propagator about fixed assignment upon observe for %d",
       unit);

  // internal add-observed-var had to backtrack to root-level already
  CADICAL_assert (!internal->level);

  std::vector<int> assigned = {unit};
  propagator->notify_assignment (assigned);
}

void External::remove_observed_var (int elit) {
  if (!propagator) {
    LOG ("No connected propagator that could have watched the variable");
    return;
  }
  int eidx = abs (elit);

  if (eidx > max_var)
    return;

  if (is_observed[eidx]) {
    // Follow opposite order of add_observed_var, first remove internal
    // is_observed
    int ilit = e2i[eidx]; // internalize (elit);
    internal->remove_observed_var (ilit);

    is_observed[eidx] = false;
    melt (elit);
    LOG ("unmarking %d as externally watched", eidx);
  }
}

void External::reset_observed_vars () {
  // Shouldn't be called if there is no connected propagator
  CADICAL_assert (propagator);
  reset_extended ();

  internal->notified = 0;
  LOG ("reset notified counter to 0");

  if (!is_observed.size ())
    return;

  CADICAL_assert (!max_var || (size_t) max_var + 1 == is_observed.size ());

  for (auto elit : vars) {
    int eidx = abs (elit);
    CADICAL_assert (eidx <= max_var);
    if (is_observed[eidx]) {
      int ilit = internalize (elit);
      internal->remove_observed_var (ilit);
      LOG ("unmarking %d as externally watched", eidx);
      is_observed[eidx] = false;
      melt (elit);
    }
  }
}

bool External::observed (int elit) {
  CADICAL_assert (elit);
  CADICAL_assert (elit != INT_MIN);
  int eidx = abs (elit);
  if (eidx > max_var)
    return false;
  if (eidx >= (int) is_observed.size ())
    return false;

  return is_observed[eidx];
}

bool External::is_witness (int elit) {
  CADICAL_assert (elit);
  CADICAL_assert (elit != INT_MIN);
  int eidx = abs (elit);
  if (eidx > max_var)
    return false;
  return (marked (witness, elit) || marked (witness, -elit));
}

bool External::is_decision (int elit) {
  CADICAL_assert (elit);
  CADICAL_assert (elit != INT_MIN);
  int eidx = abs (elit);
  if (eidx > max_var)
    return false;

  int ilit = internalize (elit);
  return internal->is_decision (ilit);
}

void External::force_backtrack (size_t new_level) {
  if (!propagator) {
    LOG ("No connected propagator that could force backtracking");
    return;
  }
  LOG ("force backtrack to level %zd", new_level);
  internal->force_backtrack (new_level);
}

/*------------------------------------------------------------------------*/

int External::propagate_assumptions () {
  int res = internal->propagate_assumptions ();
  if (res == 10 && !extended)
    extend (); // Call solution reconstruction
  check_solve_result (res);
  reset_limits ();
  return res;
}

void External::implied (std::vector<int> &trailed) {
  std::vector<int> ilit_implicants;
  internal->implied (ilit_implicants);

  // Those implied literals must be filtered out that are witnesses
  // on the reconstruction stack -> no inplace externalize is possible.
  // (Internal does not see these marks, so no earlier filter is
  // possible.)

  trailed.clear();

  for (const auto &ilit : ilit_implicants) {
    CADICAL_assert (ilit);
    const int elit = internal->externalize (ilit);
    const int eidx = abs (elit);
    const bool is_extension_var = ervars[eidx];
    if (!marked (tainted, elit) && !is_extension_var) {
      trailed.push_back (elit);
    }
  }
}

void External::conclude_unknown () {
  if (!internal->proof || concluded)
    return;
  concluded = true;

  vector<int> trail;
  implied (trail);
  internal->proof->conclude_unknown (trail);
}

/*------------------------------------------------------------------------*/

// Internal checker if 'solve' claims the formula to be satisfiable.

void External::check_satisfiable () {
  LOG ("checking satisfiable");
  if (!extended)
    extend ();
  if (internal->opts.checkwitness)
    check_assignment (&External::ival);
  if (internal->opts.checkassumptions && !assumptions.empty ())
    check_assumptions_satisfied ();
  if (internal->opts.checkconstraint && !constraint.empty ())
    check_constraint_satisfied ();
}

// Internal checker if 'solve' claims formula to be unsatisfiable.

void External::check_unsatisfiable () {
  LOG ("checking unsatisfiable");
  if (!internal->opts.checkfailed)
    return;
  if (!assumptions.empty () || !constraint.empty ())
    check_failing ();
}

// Check result of 'solve' to be correct.

void External::check_solve_result (int res) {
  if (!internal->opts.check)
    return;
  if (res == 10)
    check_satisfiable ();
  if (res == 20)
    check_unsatisfiable ();
}

// Prepare checking that completely molten literals are not used as argument
// of 'add' or 'assume', which is invalid under freezing semantics.  This
// case would be caught by our 'restore' implementation so is only needed
// for checking the deprecated 'freeze' semantics.

void External::update_molten_literals () {
  if (!internal->opts.checkfrozen)
    return;
  CADICAL_assert ((size_t) max_var + 1 == moltentab.size ());
#ifdef LOGGING
  int registered = 0, molten = 0;
#endif
  for (auto lit : vars) {
    if (moltentab[lit]) {
      LOG ("skipping already molten literal %d", lit);
#ifdef LOGGING
      molten++;
#endif
    } else if (frozen (lit))
      LOG ("skipping currently frozen literal %d", lit);
    else {
      LOG ("new molten literal %d", lit);
      moltentab[lit] = true;
#ifdef LOGGING
      registered++;
      molten++;
#endif
    }
  }
  LOG ("registered %d new molten literals", registered);
  LOG ("reached in total %d molten literals", molten);
}

int External::solve (bool preprocess_only) {
  reset_extended ();
  update_molten_literals ();
  int res = internal->solve (preprocess_only);
  check_solve_result (res);
  reset_limits ();
  return res;
}

void External::terminate () { internal->terminate (); }

int External::lookahead () {
  reset_extended ();
  update_molten_literals ();
  int ilit = internal->lookahead ();
  const int elit =
      (ilit && ilit != INT_MIN) ? internal->externalize (ilit) : 0;
  LOG ("lookahead internal %d external %d", ilit, elit);
  return elit;
}

CaDiCaL::CubesWithStatus External::generate_cubes (int depth,
                                                   int min_depth = 0) {
  reset_extended ();
  update_molten_literals ();
  reset_limits ();
  auto cubes = internal->generate_cubes (depth, min_depth);
  auto externalize = [this] (int ilit) {
    const int elit = ilit ? internal->externalize (ilit) : 0;
    MSG ("lookahead internal %d external %d", ilit, elit);
    return elit;
  };
  auto externalize_map = [this, externalize] (std::vector<int> cube) {
    (void) this;
    MSG ("Cube : ");
    std::for_each (begin (cube), end (cube), externalize);
  };
  std::for_each (begin (cubes.cubes), end (cubes.cubes), externalize_map);

  return cubes;
}

/*------------------------------------------------------------------------*/

void External::freeze (int elit) {
  reset_extended ();
  int ilit = internalize (elit);
  unsigned eidx = vidx (elit);
  if (eidx >= frozentab.size ())
    frozentab.resize (eidx + 1, 0);
  unsigned &ref = frozentab[eidx];
  if (ref < UINT_MAX) {
    ref++;
    LOG ("external variable %d frozen once and now frozen %u times", eidx,
         ref);
  } else
    LOG ("external variable %d frozen but remains frozen forever", eidx);
  internal->freeze (ilit);
}

void External::melt (int elit) {
  reset_extended ();
  int ilit = internalize (elit);
  unsigned eidx = vidx (elit);
  CADICAL_assert (eidx < frozentab.size ());
  unsigned &ref = frozentab[eidx];
  CADICAL_assert (ref > 0);
  if (ref < UINT_MAX) {
    if (!--ref) {
      if (observed (elit)) {
        ref++;
        LOG ("external variable %d is observed, can not be completely "
             "molten",
             eidx);
      } else
        LOG ("external variable %d melted once and now completely melted",
             eidx);
    } else
      LOG ("external variable %d melted once but remains frozen %u times",
           eidx, ref);
  } else
    LOG ("external variable %d melted but remains frozen forever", eidx);
  internal->melt (ilit);
}

/*------------------------------------------------------------------------*/

void External::check_assignment (int (External::*a) (int) const) {

  // First check all assigned and consistent.
  //
  for (auto idx : vars) {
    if (!(this->*a) (idx))
      FATAL ("unassigned variable: %d", idx);
    int value_idx = (this->*a) (idx);
    int value_neg_idx = (this->*a) (-idx);
    if (value_idx == idx)
      CADICAL_assert (value_neg_idx == idx);
    else {
      CADICAL_assert (value_idx == -idx);
      CADICAL_assert (value_neg_idx == -idx);
    }
    if (value_idx != value_neg_idx)
      FATAL ("inconsistently assigned literals %d and %d", idx, -idx);
  }

  // Then check that all (saved) original clauses are satisfied.
  //
  bool satisfied = false;
  const auto end = original.end ();
  auto start = original.begin (), i = start;
#ifndef CADICAL_QUIET
  int64_t count = 0;
#endif
  for (; i != end; i++) {
    int lit = *i;
    if (!lit) {
      if (!satisfied) {
        fatal_message_start ();
        fputs ("unsatisfied clause:\n", stderr);
        for (auto j = start; j != i; j++)
          fprintf (stderr, "%d ", *j);
        fputc ('0', stderr);
        fatal_message_end ();
      }
      satisfied = false;
      start = i + 1;
#ifndef CADICAL_QUIET
      count++;
#endif
    } else if (!satisfied && (this->*a) (lit) == lit)
      satisfied = true;
  }

  bool presence_flag;
  // Check those forgettable external clauses that are still present, but
  // only if the external propagator is still connected (otherwise solution
  // reconstruction is allowed to touch the previously observed variables so
  // there is no guarantee that the final model will satisfy these clauses.)
  for (const auto &forgettables : forgettable_original) {
    if (!propagator)
      break;
    presence_flag = true;
    satisfied = false;
#ifndef CADICAL_QUIET
    count++;
#endif
    std::vector<int> literals;
    for (const auto lit : forgettables.second) {
      if (presence_flag) {
        // First integer is a Boolean flag, not a literal
        if (!lit) {
          // Deleted clauses can be ignored, they count as satisfied
          satisfied = true;
          break;
        }
        presence_flag = false;
        continue;
      }

      if ((this->*a) (lit) == lit) {
        satisfied = true;
        break;
      }
    }

    if (!satisfied) {
      fatal_message_start ();
      fputs ("unsatisfied external forgettable clause:\n", stderr);
      for (size_t j = 1; j < forgettables.second.size (); j++)
        fprintf (stderr, "%d ", forgettables.second[j]);
      fputc ('0', stderr);
      fatal_message_end ();
    }
  }
#ifndef CADICAL_QUIET
  VERBOSE (1, "satisfying assignment checked on %" PRId64 " clauses",
           count);
#endif
}

/*------------------------------------------------------------------------*/

void External::check_assumptions_satisfied () {
  for (const auto &lit : assumptions) {
    // Not 'signed char' !!!!
    const int tmp = ival (lit);
    if (tmp != lit)
      FATAL ("assumption %d falsified", lit);
    if (!tmp)
      FATAL ("assumption %d unassigned", lit);
  }
  VERBOSE (1, "checked that %zd assumptions are satisfied",
           assumptions.size ());
}

void External::check_constraint_satisfied () {
  for (const auto lit : constraint) {
    if (ival (lit) == lit) {
      VERBOSE (1, "checked that constraint is satisfied");
      return;
    }
  }
  FATAL ("constraint not satisfied");
}

void External::check_failing () {
  Solver *checker = new Solver ();
  DeferDeletePtr<Solver> delete_checker (checker);
  checker->prefix ("checker ");
#ifdef LOGGING
  if (internal->opts.log)
    checker->set ("log", true);
#endif

  for (const auto lit : assumptions) {
    if (!failed (lit))
      continue;
    LOG ("checking failed literal %d in core", lit);
    checker->add (lit);
    checker->add (0);
  }
  if (failed_constraint ()) {
    LOG (constraint, "checking failed constraint");
    for (const auto lit : constraint)
      checker->add (lit);
  } else if (constraint.size ())
    LOG (constraint, "constraint satisfied and ignored");

  // Add original clauses as last step, failing () and failed_constraint ()
  // might add more external clauses (due to lazy explanation)
  for (const auto lit : original)
    checker->add (lit);

  // Add every forgettable external clauses
  for (const auto &forgettables : forgettable_original) {
    bool presence_flag = true;
    for (const auto lit : forgettables.second) {
      if (presence_flag) {
        // First integer is a Boolean flag, not a literal, ignore it here
        presence_flag = false;
        continue;
      }
      checker->add (lit);
    }
    checker->add (0);
  }

  int res = checker->solve ();
  if (res != 20)
    FATAL ("failed assumptions do not form a core");
  delete_checker.free ();
  VERBOSE (1, "checked that %zd failing assumptions form a core",
           assumptions.size ());
}

/*------------------------------------------------------------------------*/

// Traversal of unit clauses is implemented here.

// In principle we want to traverse the clauses of the simplified formula
// only, particularly eliminated variables should be completely removed.
// This poses the question what to do with unit clauses.  Should they be
// considered part of the simplified formula or of the witness to construct
// solutions for the original formula?  Ideally they should be considered
// to be part of the witness only, i.e., as they have been simplified away.

// Therefore we distinguish frozen and non-frozen units during clause
// traversal.  Frozen units are treated as unit clauses while non-frozen
// units are treated as if they were already eliminated and put on the
// extension stack as witness clauses.

// Furthermore, eliminating units during 'compact' could be interpreted as
// variable elimination, i.e., just add the resolvents (remove falsified
// literals), then drop the clauses with the unit, and push the unit on the
// extension stack.  This is of course only OK if the user did not freeze
// that variable (even implicitly during assumptions).

// Thanks go to Fahiem Bacchus for asking why there is a necessity to
// distinguish these two cases (frozen and non-frozen units).  The answer is
// that it is not strictly necessary, and this distinction could be avoided
// by always treating units as remaining unit clauses, thus only using the
// first of the two following functions and dropping the 'if (!frozen (idx))
// continue;' check in it.  This has however the down-side that those units
// are still in the simplified formula and only as units.  I would not
// consider such a formula as really being 'simplified'. On the other hand
// if the user explicitly freezes a literal, then it should continue to be
// in the simplified formula during traversal.  So also only using the
// second function is not ideal.

// There is however a catch where this solution breaks down (in the sense of
// producing less optimal results - that is keeping units in the formula
// which better would be witness clauses).  The problem is with compact
// preprocessing which removes eliminated but also fixed internal variables.
// One internal unit (fixed) variable is kept and all the other external
// literals which became unit are mapped to that internal literal (negated
// or positively).  Compact is called non-deterministically from the point
// of the user and thus there is no control on when this happens.  If
// compact happens those external units are mapped to a single internal
// literal now and then all share the same 'frozen' counter.   So if the
// user freezes one of them all in essence get frozen, which in turn then
// makes a difference in terms of traversing such a unit as unit clause or
// as unit witness.

bool External::traverse_all_frozen_units_as_clauses (ClauseIterator &it) {
  if (internal->unsat)
    return true;

  vector<int> clause;

  for (auto idx : vars) {
    if (!frozen (idx))
      continue;
    const int tmp = fixed (idx);
    if (!tmp)
      continue;
    int unit = tmp < 0 ? -idx : idx;
    clause.push_back (unit);
    if (!it.clause (clause))
      return false;
    clause.clear ();
  }

  return true;
}

bool External::traverse_all_non_frozen_units_as_witnesses (
    WitnessIterator &it) {
  if (internal->unsat)
    return true;

  vector<int> clause_and_witness;
  for (auto idx : vars) {
    if (frozen (idx))
      continue;
    const int tmp = fixed (idx);
    if (!tmp)
      continue;
    int unit = tmp < 0 ? -idx : idx;
    const int ilit = e2i[idx] * (tmp < 0 ? -1 : 1);
    // heurstically add + max_var to the id to avoid reusing ids
    const int64_t id = internal->lrat ? internal->unit_id (ilit) : 1;
    CADICAL_assert (id);
    clause_and_witness.push_back (unit);
    if (!it.witness (clause_and_witness, clause_and_witness, id + max_var))
      return false;
    clause_and_witness.clear ();
  }

  return true;
}

/*------------------------------------------------------------------------*/

void External::copy_flags (External &other) const {
  const vector<Flags> &this_ftab = internal->ftab;
  vector<Flags> &other_ftab = other.internal->ftab;
  const unsigned limit = min (max_var, other.max_var);
  for (unsigned eidx = 1; eidx <= limit; eidx++) {
    const int this_ilit = e2i[eidx];
    if (!this_ilit)
      continue;
    const int other_ilit = other.e2i[eidx];
    if (!other_ilit)
      continue;
    if (!internal->active (this_ilit))
      continue;
    if (!other.internal->active (other_ilit))
      continue;
    CADICAL_assert (this_ilit != INT_MIN);
    CADICAL_assert (other_ilit != INT_MIN);
    const Flags &this_flags = this_ftab[abs (this_ilit)];
    Flags &other_flags = other_ftab[abs (other_ilit)];
    this_flags.copy (other_flags);
  }
}

/*------------------------------------------------------------------------*/

void External::export_learned_empty_clause () {
  CADICAL_assert (learner);
  if (learner->learning (0)) {
    LOG ("exporting learned empty clause");
    learner->learn (0);
  } else
    LOG ("not exporting learned empty clause");
}

void External::export_learned_unit_clause (int ilit) {
  CADICAL_assert (learner);
  if (learner->learning (1)) {
    LOG ("exporting learned unit clause");
    const int elit = internal->externalize (ilit);
    CADICAL_assert (elit);
    learner->learn (elit);
    learner->learn (0);
  } else
    LOG ("not exporting learned unit clause");
}

void External::export_learned_large_clause (const vector<int> &clause) {
  CADICAL_assert (learner);
  size_t size = clause.size ();
  CADICAL_assert (size <= (unsigned) INT_MAX);
  if (learner->learning ((int) size)) {
    LOG ("exporting learned clause of size %zu", size);
    for (auto ilit : clause) {
      const int elit = internal->externalize (ilit);
      CADICAL_assert (elit);
      learner->learn (elit);
    }
    learner->learn (0);
  } else
    LOG ("not exporting learned clause of size %zu", size);
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
