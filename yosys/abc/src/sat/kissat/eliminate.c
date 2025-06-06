#include "eliminate.h"
#include "allocate.h"
#include "backtrack.h"
#include "collect.h"
#include "dense.h"
#include "forward.h"
#include "inline.h"
#include "inlineheap.h"
#include "kitten.h"
#include "print.h"
#include "propdense.h"
#include "report.h"
#include "resolve.h"
#include "terminate.h"
#include "trail.h"
#include "weaken.h"

#include <inttypes.h>
#include <math.h>

ABC_NAMESPACE_IMPL_START

bool kissat_eliminating (kissat *solver) {
  if (!solver->enabled.eliminate)
    return false;
  statistics *statistics = &solver->statistics_;
  if (!statistics->clauses_irredundant)
    return false;
  const uint64_t conflicts = statistics->conflicts;
  if (solver->last.conflicts.reduce == conflicts)
    return false;
  limits *limits = &solver->limits;
  if (limits->eliminate.conflicts > conflicts)
    return false;
  if (limits->eliminate.variables.eliminate <
      statistics->variables_eliminate)
    return true;
  if (limits->eliminate.variables.subsume < statistics->variables_subsume)
    return true;
  return false;
}

static inline double variable_score (kissat *solver, unsigned idx) {
  const unsigned lit = LIT (idx);
  const unsigned not_lit = NOT (lit);
  size_t occlim = GET_OPTION (eliminateocclim);
  size_t pos = SIZE_WATCHES (WATCHES (lit));
  size_t neg = SIZE_WATCHES (WATCHES (not_lit));
  if (pos > occlim)
    pos = occlim;
  if (neg > occlim)
    neg = occlim;
  double prod = pos * neg;
  double sum = pos + neg;
  double occlim2 = occlim * (double) occlim;
  KISSAT_assert (prod <= occlim2);
  double score = prod - sum;
  KISSAT_assert (score <= occlim2);
  double relevancy;
  if (solver->stable)
    relevancy = kissat_get_heap_score (&solver->scores, idx);
  else
    relevancy = LINK (idx).stamp;
  double res = relevancy + score - occlim2;
  LOG ("variable score of %s computed as "
       "%g = %g + (%zu*%zu - %zu - %zu) - %g"
       " = %g + %g - %g",
       LOGVAR (idx), res, relevancy, pos, neg, pos, neg, occlim2, relevancy,
       score, occlim2);
  return res;
}

static inline void update_variable_score (kissat *solver, heap *schedule,
                                          unsigned idx) {
  KISSAT_assert (schedule->size);
  KISSAT_assert (schedule == &solver->schedule);
  double new_score = variable_score (solver, idx);
  LOG ("new score %g for variable %s", new_score, LOGVAR (idx));
  kissat_update_heap (solver, schedule, idx, -new_score);
}

void kissat_update_variable_score (kissat *solver, unsigned idx) {
  update_variable_score (solver, &solver->schedule, idx);
}

static inline void update_after_adding_stack (kissat *solver,
                                              unsigneds *stack) {
  KISSAT_assert (!solver->probing);
  heap *schedule = &solver->schedule;
  if (!schedule->size)
    return;
  for (all_stack (unsigned, lit, *stack))
    update_variable_score (solver, schedule, IDX (lit));
}

static inline void update_after_removing_variable (kissat *solver,
                                                   unsigned idx) {
  heap *schedule = &solver->schedule;
  if (!schedule->size)
    return;
  KISSAT_assert (!solver->probing);
  flags *f = solver->flags + idx;
  if (f->fixed)
    return;
  KISSAT_assert (!f->eliminated);
  update_variable_score (solver, schedule, idx);
  if (!kissat_heap_contains (schedule, idx))
    kissat_push_heap (solver, schedule, idx);
}

static inline void update_after_removing_clause (kissat *solver, clause *c,
                                                 unsigned except) {
  if (!solver->schedule.size)
    return;
  KISSAT_assert (c->garbage);
  for (all_literals_in_clause (lit, c))
    if (lit != except)
      update_after_removing_variable (solver, IDX (lit));
}

void kissat_eliminate_binary (kissat *solver, unsigned lit,
                              unsigned other) {
  kissat_disconnect_binary (solver, other, lit);
  kissat_delete_binary (solver, lit, other);
  update_after_removing_variable (solver, IDX (other));
}

void kissat_eliminate_clause (kissat *solver, clause *c, unsigned lit) {
  kissat_mark_clause_as_garbage (solver, c);
  update_after_removing_clause (solver, c, lit);
}

static unsigned schedule_variables (kissat *solver) {
  LOG ("initializing variable schedule");
  KISSAT_assert (!solver->schedule.size);

  kissat_resize_heap (solver, &solver->schedule, solver->vars);

  flags *all_flags = solver->flags;

  size_t scheduled = 0;
  for (all_variables (idx)) {
    flags *flags = all_flags + idx;
    if (!flags->active)
      continue;
    if (!flags->eliminate)
      continue;
    LOG ("scheduling %s", LOGVAR (idx));
    scheduled++;
    update_after_removing_variable (solver, idx);
  }
  KISSAT_assert (scheduled == kissat_size_heap (&solver->schedule));
#ifndef KISSAT_QUIET
  size_t active = solver->active;
  kissat_phase (solver, "eliminate", GET (eliminations),
                "scheduled %zu variables %.0f%%", scheduled,
                kissat_percent (scheduled, active));
#endif
  return scheduled;
}

void kissat_flush_units_while_connected (kissat *solver) {
  const unsigned *propagate = solver->propagate;
  const unsigned *end_trail = END_ARRAY (solver->trail);
  KISSAT_assert (propagate <= end_trail);
  const size_t units = end_trail - propagate;
  if (!units)
    return;
#ifdef LOGGING
  LOG ("propagating and flushing %zu units", units);
#endif
  if (!kissat_dense_propagate (solver))
    return;
  LOG ("marking and flushing unit satisfied clauses");

  end_trail = END_ARRAY (solver->trail);
  while (propagate != end_trail) {
    const unsigned unit = *propagate++;
    watches *unit_watches = &WATCHES (unit);
    watch *begin = BEGIN_WATCHES (*unit_watches), *q = begin;
    const watch *const end = END_WATCHES (*unit_watches), *p = q;
    if (begin == end)
      continue;
    LOG ("marking %s satisfied clauses as garbage", LOGLIT (unit));
    while (p != end) {
      const watch watch = *q++ = *p++;
      if (watch.type.binary) {
        const unsigned other = watch.binary.lit;
        if (!solver->values[other])
          update_after_removing_variable (solver, IDX (other));
      } else {
        const reference ref = watch.large.ref;
        clause *c = kissat_dereference_clause (solver, ref);
        if (!c->garbage)
          kissat_eliminate_clause (solver, c, unit);
        KISSAT_assert (c->garbage);
        q--;
      }
    }
    KISSAT_assert (q <= end);
    size_t flushed = end - q;
    if (!flushed)
      continue;
    LOG ("flushing %zu references satisfied by %s", flushed, LOGLIT (unit));
    SET_END_OF_WATCHES (*unit_watches, q);
  }
}

static void connect_resolvents (kissat *solver) {
  const value *const values = solver->values;
  KISSAT_assert (EMPTY_STACK (solver->clause));
  bool satisfied = false;
#ifdef LOGGING
  uint64_t added = 0;
#endif
  for (all_stack (unsigned, other, solver->resolvents)) {
    if (other == INVALID_LIT) {
      if (satisfied)
        satisfied = false;
      else {
        LOGTMP ("temporary resolvent");
        const size_t size = SIZE_STACK (solver->clause);
        if (!size) {
          KISSAT_assert (!solver->inconsistent);
          LOG ("resolved empty clause");
          CHECK_AND_ADD_EMPTY ();
          ADD_EMPTY_TO_PROOF ();
          solver->inconsistent = true;
          break;
        } else if (size == 1) {
          const unsigned unit = PEEK_STACK (solver->clause, 0);
          LOG ("resolved unit clause %s", LOGLIT (unit));
          kissat_learned_unit (solver, unit);
        } else {
          KISSAT_assert (size > 1);
          (void) kissat_new_irredundant_clause (solver);
          update_after_adding_stack (solver, &solver->clause);
#ifdef LOGGING
          added++;
#endif
        }
      }
      CLEAR_STACK (solver->clause);
    } else if (!satisfied) {
      const value value = values[other];
      if (value > 0) {
        LOGTMP ("now %s satisfied resolvent", LOGLIT (other));
        satisfied = true;
      } else if (value < 0)
        LOG2 ("dropping now falsified literal %s", LOGLIT (other));
      else
        PUSH_STACK (solver->clause, other);
    }
  }
  LOG ("added %" PRIu64 " new clauses", added);
  CLEAR_STACK (solver->resolvents);
}

static void weaken_clauses (kissat *solver, unsigned lit) {
  const unsigned not_lit = NOT (lit);

  const value *const values = solver->values;
  KISSAT_assert (!values[lit]);

  watches *pos_watches = &WATCHES (lit);

  for (all_binary_large_watches (watch, *pos_watches)) {
    if (watch.type.binary) {
      const unsigned other = watch.binary.lit;
      const value value = values[other];
      if (value <= 0)
        kissat_weaken_binary (solver, lit, other);
      kissat_eliminate_binary (solver, lit, other);
    } else {
      const reference ref = watch.large.ref;
      clause *c = kissat_dereference_clause (solver, ref);
      if (c->garbage)
        continue;
      bool satisfied = false;
      for (all_literals_in_clause (other, c)) {
        const value value = values[other];
        if (value <= 0)
          continue;
        satisfied = true;
        break;
      }
      if (!satisfied)
        kissat_weaken_clause (solver, lit, c);
      LOGCLS (c, "removing %s", LOGLIT (lit));
      kissat_eliminate_clause (solver, c, lit);
    }
  }
  RELEASE_WATCHES (*pos_watches);

  watches *neg_watches = &WATCHES (not_lit);

  bool optimize = !GET_OPTION (incremental);
  for (all_binary_large_watches (watch, *neg_watches)) {
    if (watch.type.binary) {
      const unsigned other = watch.binary.lit;
      const value value = values[other];
      if (!optimize && value <= 0)
        kissat_weaken_binary (solver, not_lit, other);
      kissat_eliminate_binary (solver, not_lit, other);
    } else {
      const reference ref = watch.large.ref;
      clause *d = kissat_dereference_clause (solver, ref);
      if (d->garbage)
        continue;
      bool satisfied = false;
      for (all_literals_in_clause (other, d)) {
        const value value = values[other];
        if (value <= 0)
          continue;
        satisfied = true;
        break;
      }
      if (!optimize && !satisfied)
        kissat_weaken_clause (solver, not_lit, d);
      LOGCLS (d, "removing %s", LOGLIT (not_lit));
      kissat_eliminate_clause (solver, d, not_lit);
    }
  }
  if (optimize && !EMPTY_WATCHES (*neg_watches))
    kissat_weaken_unit (solver, not_lit);
  RELEASE_WATCHES (*neg_watches);

  kissat_flush_units_while_connected (solver);
}

static void try_to_eliminate_all_variables_again (kissat *solver) {
  LOG ("trying to elimination all variables again");
  flags *all_flags = solver->flags;
  for (all_variables (idx)) {
    flags *flags = all_flags + idx;
    flags->eliminate = true;
  }
  solver->limits.eliminate.variables.eliminate = 0;
}

static void set_next_elimination_bound (kissat *solver, bool complete) {
  const unsigned max_bound = GET_OPTION (eliminatebound);
  const unsigned current_bound =
      solver->bounds.eliminate.additional_clauses;
  KISSAT_assert (current_bound <= max_bound);

  if (complete) {
    if (current_bound == max_bound) {
      kissat_phase (solver, "eliminate", GET (eliminations),
                    "completed maximum elimination bound %u",
                    current_bound);
      limits *limits = &solver->limits;
      statistics *statistics = &solver->statistics_;
      limits->eliminate.variables.eliminate =
          statistics->variables_eliminate;
      limits->eliminate.variables.subsume = statistics->variables_subsume;
#ifndef KISSAT_QUIET
      bool first = !solver->bounds.eliminate.max_bound_completed++;
      REPORT (!first, first ? '!' : ':');
#endif
    } else {
      const unsigned next_bound =
          !current_bound ? 1 : MIN (2 * current_bound, max_bound);
      kissat_phase (solver, "eliminate", GET (eliminations),
                    "completed elimination bound %u next %u", current_bound,
                    next_bound);
      solver->bounds.eliminate.additional_clauses = next_bound;
      try_to_eliminate_all_variables_again (solver);
      REPORT (0, '^');
    }
  } else
    kissat_phase (solver, "eliminate", GET (eliminations),
                  "incomplete elimination bound %u", current_bound);
}

static bool can_eliminate_variable (kissat *solver, unsigned idx) {
  flags *flags = FLAGS (idx);

  if (!flags->active)
    return false;
  if (!flags->eliminate)
    return false;

  return true;
}

static bool eliminate_variable (kissat *solver, unsigned idx) {
  LOG ("next elimination candidate %s", LOGVAR (idx));
#ifdef LOGGING
  if (GET_OPTION (log))
    (void) variable_score (solver, idx);
#endif

  KISSAT_assert (!solver->inconsistent);
  KISSAT_assert (can_eliminate_variable (solver, idx));

  LOG ("marking %s as not removed", LOGVAR (idx));
  FLAGS (idx)->eliminate = false;

  unsigned lit;
  if (!kissat_generate_resolvents (solver, idx, &lit))
    return false;
  connect_resolvents (solver);
  if (!solver->inconsistent)
    weaken_clauses (solver, lit);
  INC (eliminated);
  kissat_mark_eliminated_variable (solver, idx);
  if (solver->gate_eliminated) {
    INC (gates_eliminated);
#ifdef METRICS
    KISSAT_assert (*solver->gate_eliminated < UINT64_MAX);
    *solver->gate_eliminated += 1;
#endif
  }
  return true;
}

static void eliminate_variables (kissat *solver) {
  kissat_very_verbose (solver,
                       "trying to eliminate variables with bound %u",
                       solver->bounds.eliminate.additional_clauses);
  KISSAT_assert (!solver->inconsistent);
#ifndef KISSAT_QUIET
  unsigned before = solver->active;
  unsigned eliminated = 0;
  uint64_t tried = 0;
#endif
  unsigned last_round_eliminated = 0;

  SET_EFFORT_LIMIT (resolution_limit, eliminate, eliminate_resolutions);

  bool complete;
  int round = 0;

  const bool forward = GET_OPTION (forward);

  for (;;) {
    round++;
    LOG ("starting new elimination round %d", round);

    if (forward) {
      unsigned *propagate = solver->propagate;
      complete = kissat_forward_subsume_during_elimination (solver);
      if (solver->inconsistent)
        break;
      kissat_flush_large_connected (solver);
      kissat_connect_irredundant_large_clauses (solver);
      solver->propagate = propagate;
      kissat_flush_units_while_connected (solver);
      if (solver->inconsistent)
        break;
    } else {
      kissat_connect_irredundant_large_clauses (solver);
      complete = true;
    }

#ifndef KISSAT_QUIET
    const unsigned last_round_scheduled =
#endif
        schedule_variables (solver);
    kissat_very_verbose (
        solver,
        "scheduled %u variables %.0f%% to eliminate "
        "in round %d",
        last_round_scheduled,
        kissat_percent (last_round_scheduled, solver->active), round);

    unsigned last_round_eliminated = 0;

    while (!solver->inconsistent &&
           !kissat_empty_heap (&solver->schedule)) {
      if (TERMINATED (eliminate_terminated_1)) {
        complete = false;
        break;
      }
      unsigned idx = kissat_pop_max_heap (solver, &solver->schedule);
      if (!can_eliminate_variable (solver, idx))
        continue;
      statistics *s = &solver->statistics_;
      if (s->eliminate_resolutions > resolution_limit) {
        kissat_extremely_verbose (
            solver,
            "eliminate round %u hits "
            "resolution limit %" PRIu64 " at %" PRIu64 " resolutions",
            round, resolution_limit, s->eliminate_resolutions);
        complete = false;
        break;
      }
#ifndef KISSAT_QUIET
      tried++;
#endif
      if (eliminate_variable (solver, idx))
        last_round_eliminated++;
      if (solver->inconsistent)
        break;
      kissat_flush_units_while_connected (solver);
    }

    if (last_round_eliminated) {
      complete = false;
#ifndef KISSAT_QUIET
      eliminated += last_round_eliminated;
#endif
    }

    if (!solver->inconsistent) {
      kissat_flush_large_connected (solver);
      kissat_dense_collect (solver);
    }

    kissat_phase (
        solver, "eliminate", GET (eliminations),
        "eliminated %u variables %.0f%% in round %u", last_round_eliminated,
        kissat_percent (last_round_eliminated, last_round_scheduled),
        round);
    REPORT (!last_round_eliminated, 'e');

    if (solver->inconsistent)
      break;
    kissat_release_heap (solver, &solver->schedule);
    if (complete)
      break;
    if (round == GET_OPTION (eliminaterounds))
      break;
    if (solver->statistics_.eliminate_resolutions > resolution_limit)
      break;
    if (TERMINATED (eliminate_terminated_2))
      break;
  }

  const unsigned remain = kissat_size_heap (&solver->schedule);
  kissat_release_heap (solver, &solver->schedule);
#ifndef KISSAT_QUIET
  kissat_very_verbose (solver,
                       "eliminated %u variables %.0f%% of %" PRIu64 " tried"
                       " (%u remain %.0f%%)",
                       eliminated, kissat_percent (eliminated, tried),
                       tried, remain,
                       kissat_percent (remain, solver->active));
  kissat_phase (solver, "eliminate", GET (eliminations),
                "eliminated %u variables %.0f%% out of %u in %d rounds",
                eliminated, kissat_percent (eliminated, before), before,
                round);
#endif
  if (!solver->inconsistent) {
    const bool complete = !remain && !last_round_eliminated;
    set_next_elimination_bound (solver, complete);
    if (!complete) {
      const flags *end = solver->flags + VARS;
#ifndef KISSAT_QUIET
      unsigned dropped = 0;
#endif
      for (struct flags *f = solver->flags; f != end; f++)
        if (f->eliminate) {
          f->eliminate = false;
#ifndef KISSAT_QUIET
          dropped++;
#endif
        }

      kissat_very_verbose (solver, "dropping %u eliminate candidates",
                           dropped);
    }
  }
}

static void init_map_and_kitten (kissat *solver) {
  if (!GET_OPTION (definitions))
    return;
  KISSAT_assert (!solver->kitten);
  solver->kitten = kitten_embedded (solver);
}

static void reset_map_and_kitten (kissat *solver) {
  if (solver->kitten) {
    kitten_release (solver->kitten);
    solver->kitten = 0;
  }
}

static void eliminate (kissat *solver) {
  kissat_backtrack_propagate_and_flush_trail (solver);
  KISSAT_assert (!solver->inconsistent);
  STOP_SEARCH_AND_START_SIMPLIFIER (eliminate);
  kissat_phase (solver, "eliminate", GET (eliminations),
                "elimination limit of %" PRIu64 " conflicts hit",
                solver->limits.eliminate.conflicts);
  init_map_and_kitten (solver);
  kissat_enter_dense_mode (solver, 0);
  eliminate_variables (solver);
  kissat_resume_sparse_mode (solver, true, 0);
  reset_map_and_kitten (solver);
  kissat_check_statistics (solver);
  STOP_SIMPLIFIER_AND_RESUME_SEARCH (eliminate);
}

int kissat_eliminate (kissat *solver) {
  KISSAT_assert (!solver->inconsistent);
  INC (eliminations);
  eliminate (solver);
  kissat_classify (solver);
  UPDATE_CONFLICT_LIMIT (eliminate, eliminations, NLOG2N, true);
  solver->last.ticks.eliminate = solver->statistics_.search_ticks;
  return solver->inconsistent ? 20 : 0;
}

ABC_NAMESPACE_IMPL_END
