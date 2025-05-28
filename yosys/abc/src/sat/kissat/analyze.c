#include "analyze.h"
#include "backtrack.h"
#include "bump.h"
#include "deduce.h"
#include "inline.h"
#include "learn.h"
#include "minimize.h"
#include "print.h"
#include "rank.h"
#include "shrink.h"
#include "sort.h"
#include "tiers.h"

#include <inttypes.h>

ABC_NAMESPACE_IMPL_START

static bool one_literal_on_conflict_level (kissat *solver, clause *conflict,
                                           unsigned *conflict_level_ptr) {
  KISSAT_assert (conflict);
  KISSAT_assert (conflict->size > 1);

  unsigned jump_level = INVALID_LEVEL;
  unsigned conflict_level = INVALID_LEVEL;
  unsigned literals_on_conflict_level = 0;
  unsigned forced_lit = INVALID_LIT;

  assigned *all_assigned = solver->assigned;

  unsigned *lits = conflict->lits;
  const unsigned conflict_size = conflict->size;
  const unsigned *const end_of_lits = lits + conflict_size;

  for (const unsigned *p = lits; p != end_of_lits; p++) {
    const unsigned lit = *p;
    KISSAT_assert (VALUE (lit) < 0);
    const unsigned idx = IDX (lit);
    const unsigned lit_level = all_assigned[idx].level;
    if (conflict_level == INVALID_LEVEL || conflict_level < lit_level) {
      literals_on_conflict_level = 1;
      jump_level = conflict_level;
      conflict_level = lit_level;
      forced_lit = lit;
    } else {
      if (jump_level == INVALID_LEVEL || jump_level < lit_level)
        jump_level = lit_level;
      if (conflict_level == lit_level)
        literals_on_conflict_level++;
    }
    if (literals_on_conflict_level > 1 && conflict_level == solver->level)
      break;
  }
  KISSAT_assert (conflict_level != INVALID_LEVEL);
  KISSAT_assert (literals_on_conflict_level);

  LOG ("found %u literals on conflict level %u", literals_on_conflict_level,
       conflict_level);
  *conflict_level_ptr = conflict_level;

  if (!conflict_level) {
    solver->inconsistent = true;
    LOG ("learned empty clause from conflict at conflict level zero");
    CHECK_AND_ADD_EMPTY ();
    ADD_EMPTY_TO_PROOF ();
    return false;
  }

  if (conflict_level < solver->level) {
    LOG ("forced backtracking due to conflict level %u < level %u",
         conflict_level, solver->level);
    kissat_backtrack_after_conflict (solver, conflict_level);
  }

  if (conflict_size > 2) {
    for (unsigned i = 0; i < 2; i++) {
      const unsigned lit = lits[i];
      const unsigned lit_idx = IDX (lit);
      unsigned highest_position = i;
      unsigned highest_literal = lit;
      unsigned highest_level = all_assigned[lit_idx].level;
      for (unsigned j = i + 1; j < conflict_size; j++) {
        const unsigned other = lits[j];
        const unsigned other_idx = IDX (other);
        const unsigned level = all_assigned[other_idx].level;
        if (highest_level >= level)
          continue;
        highest_literal = other;
        highest_position = j;
        highest_level = level;
        if (highest_level == conflict_level)
          break;
      }
      if (highest_position == i)
        continue;
      reference ref = INVALID_REF;
      if (highest_position > 1) {
        ref = kissat_reference_clause (solver, conflict);
        kissat_unwatch_blocking (solver, lit, ref);
      }
      lits[highest_position] = lit;
      lits[i] = highest_literal;
      if (highest_position > 1)
        kissat_watch_blocking (solver, lits[i], lits[!i], ref);
    }
  }

  if (literals_on_conflict_level > 1)
    return false;

  KISSAT_assert (literals_on_conflict_level == 1);
  KISSAT_assert (forced_lit != INVALID_LIT);
  KISSAT_assert (jump_level != INVALID_LEVEL);
  KISSAT_assert (jump_level < conflict_level);

  LOG ("reusing conflict as driving clause of %s", LOGLIT (forced_lit));

  unsigned new_level = kissat_determine_new_level (solver, jump_level);
  kissat_backtrack_after_conflict (solver, new_level);

  if (conflict_size == 2) {
    KISSAT_assert (conflict == &solver->conflict);
    const unsigned other = lits[0] ^ lits[1] ^ forced_lit;
    kissat_assign_binary (solver, forced_lit, other);
  } else {
    const reference ref = kissat_reference_clause (solver, conflict);
    kissat_assign_reference (solver, forced_lit, ref, conflict);
  }

  return true;
}

static inline void mark_reason_side_literal (kissat *solver,
                                             assigned *all_assigned,
                                             unsigned lit) {
  const unsigned idx = IDX (lit);
  const assigned *a = all_assigned + idx;
  if (a->level && !a->analyzed)
    kissat_push_analyzed (solver, all_assigned, idx);
}

static inline void analyze_reason_side_literal (kissat *solver,
                                                size_t limit, ward *arena,
                                                assigned *all_assigned,
                                                unsigned lit) {
  const unsigned idx = IDX (lit);
  const assigned *a = all_assigned + idx;
  KISSAT_assert (a->level);
  KISSAT_assert (a->analyzed);
  KISSAT_assert (a->reason != UNIT_REASON);
  if (a->reason == DECISION_REASON)
    return;
  if (a->binary) {
    const unsigned other = a->reason;
    mark_reason_side_literal (solver, all_assigned, other);
  } else {
    const reference ref = a->reason;
    KISSAT_assert (ref < SIZE_STACK (solver->arena));
    clause *c = (clause *) (arena + ref);
    const unsigned not_lit = NOT (lit);
    INC (search_ticks);
    for (all_literals_in_clause (other, c))
      if (other != not_lit) {
        KISSAT_assert (other != lit);
        mark_reason_side_literal (solver, all_assigned, other);
        if (SIZE_STACK (solver->analyzed) > limit)
          break;
      }
  }
}

static void analyze_reason_side_literals (kissat *solver) {
  if (!GET_OPTION (bump))
    return;
  if (!GET_OPTION (bumpreasons))
    return;
  if (solver->probing)
    return;
  if (DELAYING (bumpreasons))
    return;
  const double decision_rate = AVERAGE (decision_rate);
  const int decision_rate_limit = GET_OPTION (bumpreasonsrate);
  if (decision_rate >= decision_rate_limit) {
    LOG ("decision rate %g >= limit %d", decision_rate,
         decision_rate_limit);
    return;
  }
  assigned *all_assigned = solver->assigned;
#ifndef KISSAT_NDEBUG
  for (all_stack (unsigned, lit, solver->clause))
    KISSAT_assert (all_assigned[IDX (lit)].analyzed);
#endif
  LOG ("trying to bump reason side literals too");
  const size_t saved = SIZE_STACK (solver->analyzed);
  const size_t limit = GET_OPTION (bumpreasonslimit) * saved;
  LOG ("analyzed already %zu literals thus limit %zu", saved, limit);
  ward *arena = BEGIN_STACK (solver->arena);
  for (all_stack (unsigned, lit, solver->clause)) {
    analyze_reason_side_literal (solver, limit, arena, all_assigned, lit);
    if (SIZE_STACK (solver->analyzed) > limit)
      break;
  }
  if (SIZE_STACK (solver->analyzed) > limit) {
    LOG ("too many additional reason side literals");
    while (SIZE_STACK (solver->analyzed) > saved) {
      const unsigned idx = POP_STACK (solver->analyzed);
      struct assigned *a = all_assigned + idx;
      LOG ("marking %s as not analyzed", LOGVAR (idx));
      KISSAT_assert (a->analyzed);
      a->analyzed = false;
    }
    BUMP_DELAY (bumpreasons);
  } else
    REDUCE_DELAY (bumpreasons);
}

#define RADIX_SORT_LEVELS_LIMIT 32

#define RANK_LEVEL(A) (A)
#define SMALLER_LEVEL(A, B) (RANK_LEVEL (A) < RANK_LEVEL (B))

static void sort_levels (kissat *solver) {
  unsigneds *levels = &solver->levels;
  size_t glue = SIZE_STACK (*levels);
  if (glue < RADIX_SORT_LEVELS_LIMIT)
    SORT_STACK (unsigned, *levels, SMALLER_LEVEL);
  else
    RADIX_STACK (unsigned, unsigned, *levels, RANK_LEVEL);
  LOG ("sorted %zu levels", glue);
}

static void sort_deduced_clause (kissat *solver) {
  sort_levels (solver);
#ifndef KISSAT_NDEBUG
  const size_t size_frames = SIZE_STACK (solver->frames);
#endif
  frame *frames = BEGIN_STACK (solver->frames);
  unsigned pos = 1;
  const unsigned *const begin_levels = BEGIN_STACK (solver->levels);
  const unsigned *const end_levels = END_STACK (solver->levels);
  unsigned const *p = end_levels;
  while (p != begin_levels) {
    const unsigned level = *--p;
    KISSAT_assert (level < size_frames);
    frame *f = frames + level;
    const unsigned used = f->used;
#ifndef KISSAT_NDEBUG
    f->saved = used;
#endif
    KISSAT_assert (used > 0);
    KISSAT_assert (UINT_MAX - used >= pos);
    f->used = pos;
    pos += used;
  }
  unsigneds *clause = &solver->clause;
  const size_t size_clause = SIZE_STACK (*clause);
#ifndef KISSAT_NDEBUG
  KISSAT_assert (pos == size_clause);
#endif
  unsigned const *begin_clause = BEGIN_STACK (*clause);
  const unsigned *const end_clause = END_STACK (*clause);
  KISSAT_assert (begin_clause < end_clause);

  unsigneds *shadow = &solver->shadow;
  while (SIZE_STACK (*shadow) < size_clause)
    PUSH_STACK (*shadow, INVALID_LIT);

  const unsigned not_uip = *begin_clause++;
  POKE_STACK (*shadow, 0, not_uip);

  const assigned *const assigned = solver->assigned;

  for (const unsigned *p = begin_clause; p != end_clause; p++) {
    const unsigned lit = *p;
    const unsigned idx = IDX (lit);
    const struct assigned *a = assigned + idx;
    const unsigned level = a->level;
    KISSAT_assert (level < size_frames);
    frame *f = frames + level;
    const unsigned pos = f->used++;
    POKE_STACK (*shadow, pos, lit);
  }

  KISSAT_assert (size_clause == SIZE_STACK (*shadow));
  SWAP (unsigneds, *clause, *shadow);

  pos = 1;
  p = end_levels;
  while (p != begin_levels) {
    const unsigned level = *--p;
    KISSAT_assert (level < size_frames);
    frame *f = frames + level;
    const unsigned end = f->used;
    KISSAT_assert (pos < end);
    f->used = end - pos;
    KISSAT_assert (f->used == f->saved);
    pos = end;
  }

  CLEAR_STACK (*shadow);
  LOGTMP ("level sorted deduced");

#ifndef KISSAT_NDEBUG
  unsigned prev_level = solver->level;
  for (all_stack (unsigned, lit, solver->clause)) {
    const unsigned idx = IDX (lit);
    const unsigned lit_level = assigned[idx].level;
    KISSAT_assert (prev_level >= lit_level);
    prev_level = lit_level;
  }
#endif
}

static void reset_levels (kissat *solver) {
  LOG ("reset %zu marked levels", SIZE_STACK (solver->levels));
  frame *frames = BEGIN_STACK (solver->frames);
#ifndef KISSAT_NDEBUG
  const size_t size_frames = SIZE_STACK (solver->frames);
#endif
  for (all_stack (unsigned, level, solver->levels)) {
    KISSAT_assert (level < size_frames);
    frame *f = frames + level;
    KISSAT_assert (f->used > 0);
    f->used = 0;
  }
  CLEAR_STACK (solver->levels);
}

void kissat_reset_only_analyzed_literals (kissat *solver) {
  LOG ("reset %zu analyzed variables", SIZE_STACK (solver->analyzed));
  assigned *assigned = solver->assigned;
  for (all_stack (unsigned, idx, solver->analyzed)) {
    KISSAT_assert (idx < VARS);
    struct assigned *a = assigned + idx;
    KISSAT_assert (!a->poisoned);
    KISSAT_assert (!a->removable);
    KISSAT_assert (!a->shrinkable);
    a->analyzed = false;
  }
  CLEAR_STACK (solver->analyzed);
}

static void reset_removable (kissat *solver) {
  LOG ("reset %zu removable variables", SIZE_STACK (solver->removable));
  assigned *assigned = solver->assigned;
#ifndef KISSAT_NDEBUG
  unsigned not_removable = 0;
#endif
  for (all_stack (unsigned, idx, solver->removable)) {
    KISSAT_assert (idx < VARS);
    struct assigned *a = assigned + idx;
    KISSAT_assert (a->removable || !not_removable++);
    a->removable = false;
  }
  CLEAR_STACK (solver->removable);
}

static void reset_analysis_but_not_analyzed_literals (kissat *solver) {
  reset_removable (solver);
  reset_levels (solver);
  LOG ("reset %zu learned literals", SIZE_STACK (solver->clause));
  CLEAR_STACK (solver->clause);
}

static void update_trail_average (kissat *solver) {
  KISSAT_assert (!solver->probing);
#if defined(LOGGING) || !defined(KISSAT_QUIET)
  const unsigned size = SIZE_ARRAY (solver->trail);
  const unsigned assigned = size - solver->unflushed;
  const unsigned active = solver->active;
  const double filled = kissat_percent (assigned, active);
#else
  (void) solver;
#endif
  LOG ("trail filled %.0f%% (size %u, unflushed %u, active %u)", filled,
       size, solver->unflushed, active);
#ifndef KISSAT_QUIET
  UPDATE_AVERAGE (trail, filled);
#endif
}

static void update_decision_rate_average (kissat *solver) {
  KISSAT_assert (!solver->probing);
  const uint64_t current = DECISIONS;
  const uint64_t previous =
      solver->averages[solver->stable].saved_decisions;
  KISSAT_assert (previous <= current);
  const uint64_t decisions = current - previous;
  solver->averages[solver->stable].saved_decisions = current;
  UPDATE_AVERAGE (decision_rate, decisions);
}

static void analyze_failed_literal (kissat *solver, clause *conflict) {
  KISSAT_assert (solver->level == 1);
  const unsigned failed = FRAME (1).decision;

  LOGCLS (conflict, "analyzing failed literal %s conflict",
          LOGLIT (failed));

  unsigneds *units = &solver->clause;
  KISSAT_assert (EMPTY_STACK (*units));
  KISSAT_assert (EMPTY_STACK (solver->analyzed));

  const unsigned not_failed = NOT (failed);
  assigned *all_assigned = solver->assigned;
#ifndef KISSAT_NDEBUG
  const value *const values = solver->values;
#endif
  unsigned const *t = END_ARRAY (solver->trail);
  unsigned unresolved = 0;
  unsigned unit = INVALID_LIT;

  for (all_literals_in_clause (lit, conflict)) {
    KISSAT_assert (lit != failed);
    if (lit == not_failed) {
      LOG ("negation %s of failed literal %s occurs in conflict",
           LOGLIT (not_failed), LOGLIT (failed));
      goto DONE;
    }
    KISSAT_assert (values[lit] < 0);
    const unsigned idx = IDX (lit);
    assigned *a = all_assigned + idx;
    if (!a->level)
      continue;
    KISSAT_assert (a->level == 1);
    LOG ("analyzing conflict literal %s", LOGLIT (lit));
    kissat_push_analyzed (solver, all_assigned, idx);
    unresolved++;
  }

  for (;;) {
    unsigned lit;
    assigned *a;
    do {
      KISSAT_assert (t > BEGIN_ARRAY (solver->trail));
      lit = *--t;
      KISSAT_assert (values[lit] > 0);
      const unsigned idx = IDX (lit);
      a = all_assigned + idx;
    } while (!a->analyzed);
    if (unresolved == 1) {
      unit = NOT (lit);
      LOG ("learning additional unit %s", LOGLIT (unit));
      PUSH_STACK (*units, unit);
    }
    if (a->binary) {
      const unsigned other = a->reason;
      LOGBINARY (lit, other, "resolving %s reason", LOGLIT (lit));
      KISSAT_assert (other != failed);
      KISSAT_assert (other != unit);
      KISSAT_assert (values[other] < 0);
      if (other == not_failed) {
        LOG ("negation %s of failed literal %s in reason",
             LOGLIT (not_failed), LOGLIT (failed));
        goto DONE;
      }
      const unsigned idx = IDX (other);
      assigned *b = all_assigned + idx;
      KISSAT_assert (b->level == 1);
      if (!b->analyzed) {
        LOG ("analyzing reason literal %s", LOGLIT (other));
        kissat_push_analyzed (solver, all_assigned, idx);
        unresolved++;
      }
    } else {
      KISSAT_assert (a->reason != UNIT_REASON);
      KISSAT_assert (a->reason != DECISION_REASON);
      const reference ref = a->reason;
      LOGREF (ref, "resolving %s reason", LOGLIT (lit));
      clause *reason = kissat_dereference_clause (solver, ref);
      for (all_literals_in_clause (other, reason)) {
        KISSAT_assert (other != NOT (lit));
        KISSAT_assert (other != failed);
        if (other == lit)
          continue;
        if (other == unit)
          continue;
        if (other == not_failed) {
          LOG ("negation %s of failed literal %s occurs in reason",
               LOGLIT (not_failed), LOGLIT (failed));
          goto DONE;
        }
        KISSAT_assert (values[other] < 0);
        const unsigned idx = IDX (other);
        assigned *b = all_assigned + idx;
        if (!b->level)
          continue;
        KISSAT_assert (b->level == 1);
        if (b->analyzed)
          continue;
        LOG ("analyzing reason literal %s", LOGLIT (other));
        kissat_push_analyzed (solver, all_assigned, idx);
        unresolved++;
      }
    }
    KISSAT_assert (unresolved > 0);
    unresolved--;
    LOG ("after resolving %s there are %u unresolved literals",
         LOGLIT (lit), unresolved);
  }
DONE:
  LOG ("learning negated failed literal %s", LOGLIT (not_failed));
  PUSH_STACK (*units, not_failed);

  if (!solver->probing)
    kissat_update_learned (solver, 0, 1);

  LOG ("failed literal %s produced %zu units", LOGLIT (failed),
       SIZE_STACK (*units));

  kissat_backtrack_without_updating_phases (solver, 0);

  for (all_stack (unsigned, lit, *units))
    kissat_learned_unit (solver, lit);
  CLEAR_STACK (*units);
  if (!solver->probing) {
    solver->iterating = true;
    INC (iterations);
  }
}

static void update_tier_limits (kissat *solver) {
  INC (retiered);
  kissat_compute_and_set_tier_limits (solver);
  if (solver->limits.glue.interval < (1u << 16))
    solver->limits.glue.interval *= 2;
  solver->limits.glue.conflicts = CONFLICTS + solver->limits.glue.interval;
}

int kissat_analyze (kissat *solver, clause *conflict) {
  if (solver->inconsistent) {
    KISSAT_assert (!solver->level);
    return 20;
  }

  START (analyze);
  if (!solver->probing) {
    update_trail_average (solver);
    update_decision_rate_average (solver);
#ifndef KISSAT_QUIET
    UPDATE_AVERAGE (level, solver->level);
#endif
  }
  int res;
  do {
    LOGCLS (conflict, "analyzing conflict %" PRIu64, CONFLICTS);
    unsigned conflict_level;
    if (one_literal_on_conflict_level (solver, conflict, &conflict_level))
      res = 1;
    else if (!conflict_level)
      res = -1;
    else if (conflict_level == 1) {
      analyze_failed_literal (solver, conflict);
      res = 1;
    } else if ((conflict =
                    kissat_deduce_first_uip_clause (solver, conflict))) {
      reset_analysis_but_not_analyzed_literals (solver);
      INC (conflicts);
      if (CONFLICTS > solver->limits.glue.conflicts)
        update_tier_limits (solver);
      res = 0; // And continue with new conflict analysis.
    } else {
      if (GET_OPTION (minimize)) {
        sort_deduced_clause (solver);
        kissat_minimize_clause (solver);
        if (GET_OPTION (shrink))
          kissat_shrink_clause (solver);
      }
      analyze_reason_side_literals (solver);
      kissat_learn_clause (solver);
      reset_analysis_but_not_analyzed_literals (solver);
      res = 1;
    }
    if (!EMPTY_STACK (solver->analyzed)) {
      if (!solver->probing && GET_OPTION (bump))
        kissat_bump_analyzed (solver);
      kissat_reset_only_analyzed_literals (solver);
    }
  } while (!res);
  STOP (analyze);
  return res > 0 ? 0 : 20;
}

ABC_NAMESPACE_IMPL_END
