#ifndef _fastassign_h_INCLUDED
#define _fastassign_h_INCLUDED

#define FAST_ASSIGN

#include "inline.h"
#include "inlineassign.h"

#include "global.h"
ABC_NAMESPACE_HEADER_START

static inline void kissat_fast_binary_assign (
    kissat *solver, const bool probing, const unsigned level, value *values,
    assigned *assigned, unsigned lit, unsigned other) {
  if (GET_OPTION (jumpreasons) && level && solver->classification.bigbig) {
    unsigned other_idx = IDX (other);
    struct assigned *a = assigned + other_idx;
    if (a->binary) {
      LOGBINARY (lit, other, "jumping %s reason", LOGLIT (lit));
      INC (jumped_reasons);
      other = a->reason;
    }
  }
  kissat_fast_assign (solver, probing, level, values, assigned, true, lit,
                      other);
  LOGBINARY (lit, other, "assign %s reason", LOGLIT (lit));
}

static inline void
kissat_fast_assign_reference (kissat *solver, value *values,
                              assigned *assigned, unsigned lit,
                              reference ref, clause *reason) {
  KISSAT_assert (reason == kissat_dereference_clause (solver, ref));
  const unsigned level =
      kissat_assignment_level (solver, values, assigned, lit, reason);
  KISSAT_assert (level <= solver->level);
  KISSAT_assert (ref != DECISION_REASON);
  KISSAT_assert (ref != UNIT_REASON);
  kissat_fast_assign (solver, solver->probing, level, values, assigned,
                      false, lit, ref);
  LOGREF (ref, "assign %s reason", LOGLIT (lit));
}

ABC_NAMESPACE_HEADER_END

#endif
