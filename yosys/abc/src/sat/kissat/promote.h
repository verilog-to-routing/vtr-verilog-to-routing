#ifndef _promote_h_INCLUDED
#define _promote_h_INCLUDED

#include "internal.h"

#include "global.h"
ABC_NAMESPACE_HEADER_START

void kissat_promote_clause (struct kissat *, clause *, unsigned new_glue);

static inline unsigned kissat_recompute_glue (kissat *solver, clause *c,
                                              unsigned limit) {
  KISSAT_assert (limit);
  KISSAT_assert (EMPTY_STACK (solver->promote));
  unsigned res = 0;
  for (all_literals_in_clause (lit, c)) {
    KISSAT_assert (VALUE (lit));
    const unsigned level = LEVEL (lit);
    frame *frame = &FRAME (level);
    if (frame->promote)
      continue;
    if (++res == limit)
      break;
    frame->promote = true;
    PUSH_STACK (solver->promote, level);
  }
  for (all_stack (unsigned, level, solver->promote)) {
    frame *frame = &FRAME (level);
    KISSAT_assert (frame->promote);
    frame->promote = false;
  }
  CLEAR_STACK (solver->promote);
  return res;
}

ABC_NAMESPACE_HEADER_END

#endif
