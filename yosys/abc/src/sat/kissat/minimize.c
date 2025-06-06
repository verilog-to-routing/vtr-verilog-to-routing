#include "minimize.h"
#include "inline.h"

ABC_NAMESPACE_IMPL_START

static inline int minimized_index (kissat *solver, bool minimizing,
                                   assigned *a, unsigned lit, unsigned idx,
                                   unsigned depth) {
#if !defined(LOGGING) && defined(KISSAT_NDEBUG)
  (void) lit;
#endif
#ifdef KISSAT_NDEBUG
  (void) idx;
#endif
  KISSAT_assert (IDX (lit) == idx);
  KISSAT_assert (solver->assigned + idx == a);
  if (!a->level) {
    LOG2 ("skipping root level literal %s", LOGLIT (lit));
    return 1;
  }
  if (a->removable && depth) {
    LOG2 ("skipping removable literal %s", LOGLIT (lit));
    return 1;
  }
  KISSAT_assert (a->reason != UNIT_REASON);
  if (a->reason == DECISION_REASON) {
    LOG2 ("can not remove decision literal %s", LOGLIT (lit));
    return -1;
  }
  if (a->poisoned) {
    LOG2 ("can not remove poisoned literal %s", LOGLIT (lit));
    return -1;
  }
  if (minimizing || !depth) {
    frame *frame = &FRAME (a->level);
    if (frame->used <= 1) {
      LOG2 ("can not remove singleton frame literal %s", LOGLIT (lit));
      return -1;
    }
  }
  return 0;
}

static bool minimize_literal (kissat *, bool, assigned *, unsigned lit,
                              unsigned depth);

static inline bool minimize_reference (kissat *solver, bool minimizing,
                                       assigned *assigned, reference ref,
                                       unsigned lit, unsigned depth) {
  const unsigned next_depth = (depth == UINT_MAX) ? depth : depth + 1;
  const unsigned not_lit = NOT (lit);
  clause *c = kissat_dereference_clause (solver, ref);
  if (GET_OPTION (minimizeticks))
    INC (search_ticks);
  for (all_literals_in_clause (other, c))
    if (other != not_lit &&
        !minimize_literal (solver, minimizing, assigned, other, next_depth))
      return false;
  return true;
}

static inline bool minimize_binary (kissat *solver, bool minimizing,
                                    assigned *assigned, unsigned lit,
                                    unsigned depth) {
  const size_t saved = SIZE_STACK (solver->minimize);
  bool res;
  for (unsigned next = lit;;) {
    const unsigned next_idx = IDX (next);
    struct assigned *a = assigned + next_idx;
    int tmp = minimized_index (solver, minimizing, a, next, next_idx, 1);
    if (tmp) {
      res = (tmp > 0);
      break;
    }
    PUSH_STACK (solver->minimize, next_idx);
    if (!a->binary) {
      const unsigned next_depth = (depth == UINT_MAX) ? depth : depth + 1;
      res = minimize_reference (solver, minimizing, assigned, a->reason,
                                next, next_depth);
      break;
    }
    next = a->reason;
  }
  unsigned *begin = BEGIN_STACK (solver->minimize) + saved;
  const unsigned *const end = END_STACK (solver->minimize);
  KISSAT_assert (begin <= end);
  if (res)
    for (const unsigned *p = begin; p != end; p++)
      kissat_push_removable (solver, assigned, *p);
  else
    for (const unsigned *p = begin; p != end; p++)
      kissat_push_poisoned (solver, assigned, *p);
  SET_END_OF_STACK (solver->minimize, begin);
  return res;
}

static bool minimize_literal (kissat *solver, bool minimizing,
                              assigned *assigned, unsigned lit,
                              unsigned depth) {
  LOG ("trying to minimize literal %s at recursion depth %d", LOGLIT (lit),
       depth);
  KISSAT_assert (VALUE (lit) < 0);
  KISSAT_assert (depth || EMPTY_STACK (solver->minimize));
  KISSAT_assert (GET_OPTION (minimizedepth) > 0);
  if (depth >= (unsigned) GET_OPTION (minimizedepth))
    return false;
  const unsigned idx = IDX (lit);
  struct assigned *a = assigned + idx;
  int tmp = minimized_index (solver, minimizing, a, lit, idx, depth);
  if (tmp > 0)
    return true;
  if (tmp < 0)
    return false;
#ifdef LOGGING
  const unsigned not_lit = NOT (lit);
#endif
  bool res;
  if (a->binary) {
    const unsigned other = a->reason;
    LOGBINARY2 (not_lit, other, "minimizing along %s reason",
                LOGLIT (not_lit));
    res = minimize_binary (solver, minimizing, assigned, other, depth);
  } else {
    const reference ref = a->reason;
    LOGREF2 (ref, "minimizing along %s reason", LOGLIT (not_lit));
    res =
        minimize_reference (solver, minimizing, assigned, ref, lit, depth);
  }
  if (!depth)
    return res;
  if (!res)
    kissat_push_poisoned (solver, assigned, idx);
  else if (!a->removable)
    kissat_push_removable (solver, assigned, idx);
  return res;
}

bool kissat_minimize_literal (kissat *solver, unsigned lit,
                              bool lit_in_clause) {
  KISSAT_assert (EMPTY_STACK (solver->minimize));
  return minimize_literal (solver, false, solver->assigned, lit,
                           !lit_in_clause);
}

void kissat_reset_poisoned (kissat *solver) {
  LOG ("reset %zu poisoned variables", SIZE_STACK (solver->poisoned));
  assigned *assigned = solver->assigned;
  for (all_stack (unsigned, idx, solver->poisoned)) {
    KISSAT_assert (idx < VARS);
    struct assigned *a = assigned + idx;
    KISSAT_assert (a->poisoned);
    a->poisoned = false;
  }
  CLEAR_STACK (solver->poisoned);
}

void kissat_minimize_clause (kissat *solver) {
  START (minimize);

  KISSAT_assert (EMPTY_STACK (solver->minimize));
  KISSAT_assert (EMPTY_STACK (solver->removable));
  KISSAT_assert (EMPTY_STACK (solver->poisoned));
  KISSAT_assert (!EMPTY_STACK (solver->clause));

  unsigned *lits = BEGIN_STACK (solver->clause);
  unsigned *end = END_STACK (solver->clause);

  assigned *assigned = solver->assigned;
#ifndef KISSAT_NDEBUG
  KISSAT_assert (lits < end);
  const unsigned not_uip = lits[0];
  KISSAT_assert (assigned[IDX (not_uip)].level == solver->level);
#endif
  for (const unsigned *p = lits; p != end; p++)
    kissat_push_removable (solver, assigned, IDX (*p));

  if (GET_OPTION (shrink) > 2) {
    STOP (minimize);
    return;
  }

  unsigned minimized = 0;

  for (unsigned *p = end; --p > lits;) {
    const unsigned lit = *p;
    KISSAT_assert (lit != not_uip);
    if (minimize_literal (solver, true, assigned, lit, 0)) {
      LOG ("minimized literal %s", LOGLIT (lit));
      *p = INVALID_LIT;
      minimized++;
    } else
      LOG ("keeping literal %s", LOGLIT (lit));
  }

  unsigned *q = lits;
  for (const unsigned *p = lits; p != end; p++) {
    const unsigned lit = *p;
    if (lit != INVALID_LIT)
      *q++ = lit;
  }
  KISSAT_assert (q + minimized == end);
  SET_END_OF_STACK (solver->clause, q);
  LOG ("clause minimization removed %u literals", minimized);

  KISSAT_assert (!solver->probing);
  ADD (literals_minimized, minimized);

  LOGTMP ("minimized learned");

  kissat_reset_poisoned (solver);

  STOP (minimize);
}

ABC_NAMESPACE_IMPL_END
