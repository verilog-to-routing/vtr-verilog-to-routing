#include "deduce.h"
#include "inline.h"
#include "promote.h"
#include "strengthen.h"

ABC_NAMESPACE_IMPL_START

static inline void recompute_and_promote (kissat *solver, clause *c) {
  KISSAT_assert (c->redundant);
  const unsigned old_glue = c->glue;
  const unsigned new_glue = kissat_recompute_glue (solver, c, old_glue);
  if (new_glue < old_glue)
    kissat_promote_clause (solver, c, new_glue);
}

static inline void mark_clause_as_used (kissat *solver, clause *c) {
  if (!c->redundant)
    return;
  INC (clauses_used);
  c->used = MAX_USED;
  LOGCLS (c, "using");
  recompute_and_promote (solver, c);
  unsigned glue = MIN (c->glue, MAX_GLUE_USED);
  solver->statistics_.used[solver->stable].glue[glue]++;
  if (solver->stable)
    INC (clauses_used_stable);
  else
    INC (clauses_used_focused);
}

bool kissat_recompute_and_promote (kissat *solver, clause *c) {
  KISSAT_assert (c->redundant);
  const unsigned old_glue = c->glue;
  const unsigned new_glue = kissat_recompute_glue (solver, c, old_glue);
  if (new_glue >= old_glue)
    return false;
  kissat_promote_clause (solver, c, new_glue);
  return true;
}

static inline bool analyze_literal (kissat *solver, assigned *all_assigned,
                                    frame *frames, unsigned lit) {
  KISSAT_assert (VALUE (lit) < 0);
  const unsigned idx = IDX (lit);
  assigned *a = all_assigned + idx;
  const unsigned level = a->level;
  if (!level)
    return false;
  solver->antecedent_size++;
  if (a->analyzed)
    return false;
  LOG ("analyzing literal %s", LOGLIT (lit));
  kissat_push_analyzed (solver, all_assigned, idx);
  KISSAT_assert (level <= solver->level);
#if defined(LOGGING) || !defined(KISSAT_NDEBUG)
  PUSH_STACK (solver->resolvent, lit);
#endif
  solver->resolvent_size++;
  if (level == solver->level)
    return true;
  KISSAT_assert (a->analyzed);
  PUSH_STACK (solver->clause, lit);
  LOG ("learned literal %s", LOGLIT (lit));
  frame *f = frames + level;
  if (f->used++)
    return false;
  LOG ("pulling in decision level %u", level);
  PUSH_STACK (solver->levels, level);
  return false;
}

clause *kissat_deduce_first_uip_clause (kissat *solver, clause *conflict) {
  START (deduce);
  KISSAT_assert (EMPTY_STACK (solver->analyzed));
  KISSAT_assert (EMPTY_STACK (solver->levels));
  KISSAT_assert (EMPTY_STACK (solver->clause));
#if defined(LOGGING) || !defined(KISSAT_NDEBUG)
  CLEAR_STACK (solver->resolvent);
#endif
  if (conflict->size > 2)
    mark_clause_as_used (solver, conflict);
  PUSH_STACK (solver->clause, INVALID_LIT);
  solver->antecedent_size = 0;
  solver->resolvent_size = 0;
  unsigned unresolved_on_current_level = 0, conflict_size = 0;
  assigned *all_assigned = solver->assigned;
  frame *frames = BEGIN_STACK (solver->frames);
  for (all_literals_in_clause (lit, conflict)) {
    KISSAT_assert (VALUE (lit) < 0);
    if (LEVEL (lit))
      conflict_size++;
    if (analyze_literal (solver, all_assigned, frames, lit))
      unresolved_on_current_level++;
  }
  KISSAT_assert (unresolved_on_current_level > 1);
  LOG ("starting with %u unresolved literals on current decision level",
       unresolved_on_current_level);
  KISSAT_assert (solver->antecedent_size == solver->resolvent_size);
  LOGRES2 ("initial");
  const bool otfs = GET_OPTION (otfs);
  unsigned const *t = END_ARRAY (solver->trail);
  unsigned uip = INVALID_LIT;
  unsigned resolved = 0;
  assigned *a = 0;
  for (;;) {
    do {
      KISSAT_assert (t > BEGIN_ARRAY (solver->trail));
      uip = *--t;
      a = ASSIGNED (uip);
    } while (!a->analyzed || a->level != solver->level);
    if (unresolved_on_current_level == 1)
      break;
    KISSAT_assert (a->reason != DECISION_REASON);
    KISSAT_assert (a->level == solver->level);
    solver->antecedent_size = 1;
    resolved++;
    if (a->binary) {
      const unsigned other = a->reason;
      LOGBINARY (uip, other, "resolving %s reason", LOGLIT (uip));
      if (analyze_literal (solver, all_assigned, frames, other))
        unresolved_on_current_level++;
    } else {
      const reference ref = a->reason;
      LOGREF (ref, "resolving %s reason", LOGLIT (uip));
      clause *reason = kissat_dereference_clause (solver, ref);
      for (all_literals_in_clause (lit, reason))
        if (lit != uip &&
            analyze_literal (solver, all_assigned, frames, lit))
          unresolved_on_current_level++;
      mark_clause_as_used (solver, reason);
    }
    KISSAT_assert (unresolved_on_current_level > 0);
    unresolved_on_current_level--;
    LOG ("after resolving %s there are %u literals left "
         "on current decision level",
         LOGLIT (uip), unresolved_on_current_level);
    KISSAT_assert (solver->resolvent_size > 0);
    solver->resolvent_size--;
#if defined(LOGGING) || !defined(KISSAT_NDEBUG)
    LOG2 ("actual antecedent size %u", solver->antecedent_size);
    REMOVE_STACK (unsigned, solver->resolvent, NOT (uip));
    KISSAT_assert (SIZE_STACK (solver->resolvent) == solver->resolvent_size);
    LOGRES2 ("new");
#endif
    if (otfs && solver->antecedent_size > 2 &&
        solver->resolvent_size < solver->antecedent_size) {
      KISSAT_assert (!a->binary);
      KISSAT_assert (solver->antecedent_size && solver->resolvent_size + 1);
      clause *reason = kissat_dereference_clause (solver, a->reason);
      KISSAT_assert (!reason->garbage);
      clause *res = kissat_on_the_fly_strengthen (solver, reason, uip);
      if (resolved == 1 && solver->resolvent_size < conflict_size) {
        KISSAT_assert (!conflict->garbage);
        KISSAT_assert (conflict_size > 2);
        kissat_on_the_fly_subsume (solver, res, conflict);
      }
      STOP (deduce);
      return res;
    }
  }
  KISSAT_assert (uip != INVALID_LIT);
  LOG ("first unique implication point %s (1st UIP)", LOGLIT (uip));
  KISSAT_assert (PEEK_STACK (solver->clause, 0) == INVALID_LIT);
  POKE_STACK (solver->clause, 0, NOT (uip));
  LOGTMP ("deduced not yet minimized 1st UIP");
  if (!solver->probing)
    ADD (literals_deduced, SIZE_STACK (solver->clause));
  STOP (deduce);
  return 0;
}

ABC_NAMESPACE_IMPL_END
