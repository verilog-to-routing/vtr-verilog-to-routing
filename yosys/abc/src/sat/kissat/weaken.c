#include "weaken.h"
#include "inline.h"

ABC_NAMESPACE_IMPL_START

static void push_witness_literal (kissat *solver, unsigned ilit) {
  KISSAT_assert (!VALUE (ilit));
  int elit = kissat_export_literal (solver, ilit);
  KISSAT_assert (elit);
  LOG2 ("pushing external witness literal %d on extension stack", elit);
  const extension ext = kissat_extension (true, elit);
  PUSH_STACK (solver->extend, ext);
}

static void push_clause_literal (kissat *solver, unsigned ilit) {
  const value value = VALUE (ilit);
  KISSAT_assert (value <= 0);
  if (value < 0)
    LOG ("not pushing internal falsified clause literal %s "
         "on extension stack",
         LOGLIT (ilit));
  else {
    int elit = kissat_export_literal (solver, ilit);
    KISSAT_assert (elit);
    LOG2 ("pushing external clause literal %d on extension stack", elit);
    const extension ext = kissat_extension (false, elit);
    PUSH_STACK (solver->extend, ext);
  }
}

#define LOGPUSHED(SIZE) \
  do { \
    LOGEXT ((SIZE), END_STACK (solver->extend) - (SIZE), \
            "pushed size %zu witness labelled clause at", \
            (size_t) (SIZE)); \
  } while (0)

void kissat_weaken_clause (kissat *solver, unsigned lit, clause *c) {
  INC (weakened);
  LOGCLS (c, "blocking on %s and weakening", LOGLIT (lit));
  push_witness_literal (solver, lit);
  for (all_literals_in_clause (other, c))
    if (lit != other)
      push_clause_literal (solver, other);
  LOGPUSHED (c->size);
}

void kissat_weaken_binary (kissat *solver, unsigned lit, unsigned other) {
  INC (weakened);
  LOGBINARY (lit, other, "blocking on %s and weakening", LOGLIT (lit));
  push_witness_literal (solver, lit);
  push_clause_literal (solver, other);
  LOGPUSHED (2);
}

void kissat_weaken_unit (kissat *solver, unsigned lit) {
  INC (weakened);
  LOG ("blocking and weakening unit %s", LOGLIT (lit));
  push_witness_literal (solver, lit);
  LOGEXT (1, END_STACK (solver->extend) - 1,
          "pushed witness labelled unit clause at");
}

ABC_NAMESPACE_IMPL_END
