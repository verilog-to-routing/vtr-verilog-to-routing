#include "shrink.h"
#include "allocate.h"
#include "inline.h"
#include "minimize.h"

ABC_NAMESPACE_IMPL_START

static void reset_shrinkable (kissat *solver) {
#ifdef LOGGING
  size_t reset = 0;
#endif
  while (!EMPTY_STACK (solver->shrinkable)) {
    const unsigned idx = POP_STACK (solver->shrinkable);
    assigned *a = solver->assigned + idx;
    KISSAT_assert (a->shrinkable);
    a->shrinkable = false;
#ifdef LOGGING
    reset++;
#endif
  }
  LOG ("resetting %zu shrinkable variables", reset);
}

static void mark_shrinkable_as_removable (kissat *solver) {
#ifdef LOGGING
  size_t marked = 0, reset = 0;
#endif
  struct assigned *assigned = solver->assigned;
  while (!EMPTY_STACK (solver->shrinkable)) {
    const unsigned idx = POP_STACK (solver->shrinkable);
    struct assigned *a = assigned + idx;
    KISSAT_assert (a->shrinkable);
    a->shrinkable = false;
    KISSAT_assert (!a->poisoned);
#ifdef LOGGING
    reset++;
#endif
    if (a->removable)
      continue;
    kissat_push_removable (solver, assigned, idx);
#ifdef LOGGING
    marked++;
#endif
  }
  LOG ("resetting %zu shrinkable variables", reset);
  LOG ("marked %zu removable variables", marked);
}

static inline int shrink_literal (kissat *solver, assigned *assigned,
                                  unsigned level, unsigned lit) {
  KISSAT_assert (solver->assigned == assigned);
  KISSAT_assert (VALUE (lit) < 0);

  const unsigned idx = IDX (lit);
  struct assigned *a = assigned + idx;
  KISSAT_assert (a->level <= level);
  if (!a->level) {
    LOG2 ("skipping root level assigned %s", LOGLIT (lit));
    return 0;
  }
  if (a->shrinkable) {
    LOG2 ("skipping already shrinkable literal %s", LOGLIT (lit));
    return 0;
  }
  if (a->level < level) {
    if (a->removable) {
      LOG2 ("skipping removable thus shrinkable %s", LOGLIT (lit));
      return 0;
    }
    const bool always_minimize_on_lower_level = (GET_OPTION (shrink) > 2);
    if (always_minimize_on_lower_level &&
        kissat_minimize_literal (solver, lit, false)) {
      LOG2 ("minimized thus shrinkable %s", LOGLIT (lit));
      return 0;
    }
    LOG ("literal %s on lower level %u < %u not removable/shrinkable",
         LOGLIT (lit), a->level, level);
    return -1;
  }
  LOG2 ("marking %s as shrinkable", LOGLIT (lit));
  a->shrinkable = true;
  PUSH_STACK (solver->shrinkable, idx);
  return 1;
}

static inline unsigned shrunken_block (kissat *solver, unsigned level,
                                       unsigned *begin_block,
                                       unsigned *end_block, unsigned uip) {
  KISSAT_assert (uip != INVALID_LIT);
  const unsigned not_uip = NOT (uip);
  LOG ("found unique implication point %s on level %u", LOGLIT (uip),
       level);

  KISSAT_assert (begin_block < end_block);
#if defined(LOGGING) || !defined(KISSAT_NDEBUG)
  const size_t tmp = end_block - begin_block;
  LOG ("shrinking %zu literals on level %u to single literal %s", tmp,
       level, LOGLIT (not_uip));
  KISSAT_assert (tmp > 1);
#endif

#ifdef LOGGING
  bool not_uip_was_in_clause = false;
#endif
  unsigned block_shrunken = 0;

  for (unsigned *p = begin_block; p != end_block; p++) {
    const unsigned lit = *p;
    if (lit == INVALID_LIT)
      continue;
#ifdef LOGGING
    if (lit == not_uip)
      not_uip_was_in_clause = true;
    else
      LOG ("shrunken literal %s", LOGLIT (lit));
#endif
    *p = INVALID_LIT;
    block_shrunken++;
  }
  *begin_block = not_uip;
  KISSAT_assert (block_shrunken);
  block_shrunken--;
#ifdef LOGGING
  if (not_uip_was_in_clause)
    LOG ("keeping single literal %s on level %u", LOGLIT (not_uip), level);
  else
    LOG ("shrunken all literals on level %u and added %s instead", level,
         LOGLIT (not_uip));
#endif
  const unsigned uip_idx = IDX (uip);
  assigned *assigned = solver->assigned;
  struct assigned *a = assigned + uip_idx;
  if (!a->analyzed)
    kissat_push_analyzed (solver, assigned, uip_idx);

  mark_shrinkable_as_removable (solver);
#ifndef LOGGING
  (void) level;
#endif
  return block_shrunken;
}

static inline void push_literals_of_block (kissat *solver,
                                           assigned *assigned,
                                           unsigned *begin_block,
                                           unsigned *end_block,
                                           unsigned level) {
  KISSAT_assert (assigned == solver->assigned);

  for (const unsigned *p = begin_block; p != end_block; p++) {
    const unsigned lit = *p;
    if (lit == INVALID_LIT)
      continue;
#ifndef KISSAT_NDEBUG
    int tmp =
#endif
        shrink_literal (solver, assigned, level, lit);
    KISSAT_assert (tmp > 0);
  }
}

static inline unsigned shrink_along_binary (kissat *solver,
                                            assigned *assigned,
                                            unsigned level, unsigned uip,
                                            unsigned other) {
  KISSAT_assert (VALUE (other) < 0);
  LOGBINARY2 (uip, other, "shrinking along %s reason", LOGLIT (uip));
  int tmp = shrink_literal (solver, assigned, level, other);
#ifndef LOGGING
  (void) uip;
#endif
  return tmp > 0;
}

static inline unsigned
shrink_along_large (kissat *solver, assigned *assigned, unsigned level,
                    unsigned uip, reference ref, bool *failed_ptr) {
  unsigned open = 0;
  LOGREF2 (ref, "shrinking along %s reason", LOGLIT (uip));
  clause *c = kissat_dereference_clause (solver, ref);
  if (GET_OPTION (minimizeticks))
    INC (search_ticks);
  for (all_literals_in_clause (other, c)) {
    if (other == uip)
      continue;
    KISSAT_assert (VALUE (other) < 0);
    int tmp = shrink_literal (solver, assigned, level, other);
    if (tmp < 0) {
      *failed_ptr = true;
      break;
    }
    if (tmp > 0)
      open++;
  }
  return open;
}

static inline unsigned shrink_along_reason (kissat *solver,
                                            assigned *assigned,
                                            unsigned level, unsigned uip,
                                            bool resolve_large_clauses,
                                            bool *failed_ptr) {
  unsigned open = 0;
  const unsigned uip_idx = IDX (uip);
  struct assigned *a = assigned + uip_idx;
  KISSAT_assert (a->shrinkable);
  KISSAT_assert (a->level == level);
  KISSAT_assert (a->reason != DECISION_REASON);
  if (a->binary) {
    const unsigned other = a->reason;
    open = shrink_along_binary (solver, assigned, level, uip, other);
  } else {
    reference ref = a->reason;
    if (resolve_large_clauses)
      open = shrink_along_large (solver, assigned, level, uip, ref,
                                 failed_ptr);
    else {
      LOGREF (ref, "not shrinking %s reason", LOGLIT (uip));
      *failed_ptr = true;
    }
  }
  return open;
}

static inline unsigned shrink_block (kissat *solver, unsigned *begin_block,
                                     unsigned *end_block, unsigned level,
                                     unsigned max_trail) {
  KISSAT_assert (level < solver->level);

  unsigned open = end_block - begin_block;

  LOG ("trying to shrink %u literals on level %u", open, level);
  LOG ("maximum trail position %u on level %u", max_trail, level);

  assigned *assigned = solver->assigned;

  push_literals_of_block (solver, assigned, begin_block, end_block, level);

  KISSAT_assert (SIZE_STACK (solver->shrinkable) == open);

  const unsigned *const begin_trail = BEGIN_ARRAY (solver->trail);

  const bool resolve_large_clauses = (GET_OPTION (shrink) > 1);
  unsigned uip = INVALID_LIT;
  bool failed = false;

  const unsigned *t = begin_trail + max_trail;

  while (!failed) {
    {
      do
        KISSAT_assert (begin_trail <= t), uip = *t--;
      while (!assigned[IDX (uip)].shrinkable);
    }
    if (open == 1)
      break;
    open += shrink_along_reason (solver, assigned, level, uip,
                                 resolve_large_clauses, &failed);
    KISSAT_assert (open > 1);
    open--;
  }

  unsigned block_shrunken = 0;
  if (failed)
    reset_shrinkable (solver);
  else
    block_shrunken =
        shrunken_block (solver, level, begin_block, end_block, uip);

  return block_shrunken;
}

static unsigned *next_block (kissat *solver, unsigned *begin_lits,
                             unsigned *end_block, unsigned *level_ptr,
                             unsigned *max_trail_ptr) {
  assigned *assigned = solver->assigned;

  unsigned level = INVALID_LEVEL;
  unsigned max_trail = 0;

  unsigned *begin_block = end_block;

  while (begin_lits < begin_block) {
    const unsigned lit = begin_block[-1];
    KISSAT_assert (lit != INVALID_LIT);
    const unsigned idx = IDX (lit);
    struct assigned *a = assigned + idx;
    unsigned lit_level = a->level;
    if (level == INVALID_LEVEL) {
      level = lit_level;
      LOG ("starting to shrink level %u", level);
    } else {
      KISSAT_assert (lit_level >= level);
      if (lit_level > level)
        break;
    }
    begin_block--;
    const unsigned trail = a->trail;
    if (trail > max_trail)
      max_trail = trail;
  }

  *level_ptr = level;
  *max_trail_ptr = max_trail;

  return begin_block;
}

static unsigned minimize_block (kissat *solver, unsigned *begin_block,
                                unsigned *end_block) {
  unsigned minimized = 0;

  for (unsigned *p = begin_block; p != end_block; p++) {
    const unsigned lit = *p;
    KISSAT_assert (lit != INVALID_LIT);
    if (!kissat_minimize_literal (solver, lit, true))
      continue;
    LOG ("minimize-shrunken literal %s", LOGLIT (lit));
    *p = INVALID_LIT;
    minimized++;
  }

  return minimized;
}

static inline unsigned *
minimize_and_shrink_block (kissat *solver, unsigned *begin_lits,
                           unsigned *end_block, unsigned *total_shrunken,
                           unsigned *total_minimized) {
  KISSAT_assert (EMPTY_STACK (solver->shrinkable));

  unsigned level, max_trail;

  unsigned *begin_block =
      next_block (solver, begin_lits, end_block, &level, &max_trail);

  unsigned open = end_block - begin_block;
  KISSAT_assert (open > 0);

  unsigned block_shrunken = 0;
  unsigned block_minimized = 0;

  if (open < 2)
    LOG ("only one literal on level %u", level);
  else {
    block_shrunken =
        shrink_block (solver, begin_block, end_block, level, max_trail);
    if (!block_shrunken)
      block_minimized = minimize_block (solver, begin_block, end_block);
  }

  block_shrunken += block_minimized;
  LOG ("shrunken %u literals on level %u (including %u minimized)",
       block_shrunken, level, block_minimized);

  *total_minimized += block_minimized;
  *total_shrunken += block_shrunken;

  return begin_block;
}

void kissat_shrink_clause (kissat *solver) {
  KISSAT_assert (GET_OPTION (minimize) > 0);
  KISSAT_assert (GET_OPTION (shrink) > 0);
  KISSAT_assert (!EMPTY_STACK (solver->clause));

  START (shrink);

  unsigned total_shrunken = 0;
  unsigned total_minimized = 0;

  unsigned *begin_lits = BEGIN_STACK (solver->clause);
  unsigned *end_lits = END_STACK (solver->clause);

  unsigned *end_block = END_STACK (solver->clause);

  while (end_block != begin_lits)
    end_block = minimize_and_shrink_block (
        solver, begin_lits, end_block, &total_shrunken, &total_minimized);
  unsigned *q = begin_lits;
  for (const unsigned *p = q; p != end_lits; p++) {
    const unsigned lit = *p;
    if (lit != INVALID_LIT)
      *q++ = lit;
  }
  LOG ("clause shrunken by %u literals (including %u minimized)",
       total_shrunken, total_minimized);
  KISSAT_assert (q + total_shrunken == end_lits);
  SET_END_OF_STACK (solver->clause, q);
  ADD (literals_shrunken, total_shrunken);
  ADD (literals_minshrunken, total_minimized);

  LOGTMP ("shrunken learned");
  kissat_reset_poisoned (solver);

  STOP (shrink);
}

ABC_NAMESPACE_IMPL_END
