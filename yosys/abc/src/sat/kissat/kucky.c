#include "lucky.h"
#include "analyze.h"
#include "backtrack.h"
#include "decide.h"
#include "inline.h"
#include "internal.h"
#include "print.h"
#include "proprobe.h"
#include "report.h"

ABC_NAMESPACE_IMPL_START

static bool no_all_negative_clauses (struct kissat *solver) {
  clause *last_irredundant = kissat_last_irredundant_clause (solver);
  for (all_clauses (c)) {
    if (last_irredundant && last_irredundant < c)
      break;
    if (c->redundant)
      continue;
    if (c->garbage)
      continue;
    for (all_literals_in_clause (lit, c))
      if (!NEGATED (lit) && VALUE (lit) >= 0)
        goto CONTINUE_WITH_NEXT_CLAUSE;
    kissat_verbose (solver, "found all negative large clause");
    return false;
  CONTINUE_WITH_NEXT_CLAUSE:;
  }
  KISSAT_assert (solver->watching);
  for (all_variables (idx)) {
    if (!ACTIVE (idx))
      continue;
    const unsigned lit = LIT (idx);
    const unsigned not_lit = NOT (lit);
    for (all_binary_blocking_watches (watch, WATCHES (not_lit))) {
      if (!watch.type.binary)
        continue;
      const unsigned other = watch.binary.lit;
      if (NEGATED (other) && ACTIVE (IDX (other))) {
        kissat_verbose (solver, "found all negative binary clause");
        return false;
      }
    }
  }
  kissat_message (solver, "lucky no all-negative clause");
  return true;
}

static bool no_all_positive_clauses (struct kissat *solver) {
  clause *last_irredundant = kissat_last_irredundant_clause (solver);
  for (all_clauses (c)) {
    if (last_irredundant && last_irredundant < c)
      break;
    if (c->redundant)
      continue;
    if (c->garbage)
      continue;
    for (all_literals_in_clause (lit, c))
      if (NEGATED (lit) && VALUE (lit) >= 0)
        goto CONTINUE_WITH_NEXT_CLAUSE;
    kissat_verbose (solver, "found all positive large clause");
    return false;
  CONTINUE_WITH_NEXT_CLAUSE:;
  }
  KISSAT_assert (solver->watching);
  for (all_variables (idx)) {
    if (!ACTIVE (idx))
      continue;
    const unsigned lit = LIT (idx);
    for (all_binary_blocking_watches (watch, WATCHES (lit))) {
      if (!watch.type.binary)
        continue;
      const unsigned other = watch.binary.lit;
      if (!NEGATED (other) && ACTIVE (IDX (other))) {
        kissat_verbose (solver, "found all positive binary clause");
        return false;
      }
    }
  }
  kissat_message (solver, "lucky no all-positive clause");
  return true;
}

static int forward_false_satisfiable (struct kissat *solver) {
  KISSAT_assert (!solver->level);
#ifndef KISSAT_QUIET
  unsigned conflicts = 0;
#endif
  for (all_stack (import, import, solver->import)) {
    if (!import.imported)
      continue;
    if (import.eliminated)
      continue;
    const unsigned lit = import.lit;
    const unsigned idx = IDX (lit);
    if (!ACTIVE (idx))
      continue;
    if (VALUE (lit))
      continue;
    const unsigned not_lit = NOT (lit);
    kissat_internal_assume (solver, not_lit);
    clause *c = kissat_probing_propagate (solver, 0, true);
    if (!c)
      continue;
#ifndef KISSAT_QUIET
    conflicts++;
#endif
    if (solver->level > 1) {
      kissat_backtrack_without_updating_phases (solver, solver->level - 1);
      kissat_internal_assume (solver, lit);
      clause *d = kissat_probing_propagate (solver, 0, true);
      if (!d)
        continue;
      kissat_verbose (solver,
                      "inconsistency after %u conflicts "
                      "forward assigning %u variables to false",
                      conflicts, solver->level);
      kissat_backtrack_without_updating_phases (solver, 0);
      return 0;
    } else {
      LOG ("failed literal %s", LOGLIT (not_lit));
      kissat_analyze (solver, c);
      KISSAT_assert (!solver->level);
      clause *d = kissat_probing_propagate (solver, 0, true);
      if (d) {
        kissat_analyze (solver, d);
        KISSAT_assert (solver->inconsistent);
        kissat_verbose (solver,
                        "lucky inconsistency forward assigning to false");
        return 20;
      }
    }
  }

  kissat_message (solver, "lucky in forward setting literals to false");
  return 10;
}

static int forward_true_satisfiable (struct kissat *solver) {
  KISSAT_assert (!solver->level);
#ifndef KISSAT_QUIET
  unsigned conflicts = 0;
#endif
  for (all_stack (import, import, solver->import)) {
    if (!import.imported)
      continue;
    if (import.eliminated)
      continue;
    const unsigned lit = import.lit;
    const unsigned idx = IDX (lit);
    if (!ACTIVE (idx))
      continue;
    if (VALUE (lit))
      continue;
    kissat_internal_assume (solver, lit);
    clause *c = kissat_probing_propagate (solver, 0, true);
    if (!c)
      continue;
#ifndef KISSAT_QUIET
    conflicts++;
#endif
    if (solver->level > 1) {
      kissat_backtrack_without_updating_phases (solver, solver->level - 1);
      const unsigned not_lit = NOT (lit);
      kissat_internal_assume (solver, not_lit);
      clause *d = kissat_probing_propagate (solver, 0, true);
      if (!d)
        continue;
      kissat_verbose (solver,
                      "inconsistency after %u conflicts "
                      "forward assigning %u variables to true",
                      conflicts, solver->level);
      kissat_backtrack_without_updating_phases (solver, 0);
      return 0;
    } else {
      LOG ("failed literal %s", LOGLIT (lit));
      kissat_analyze (solver, c);
      KISSAT_assert (!solver->level);
      clause *d = kissat_probing_propagate (solver, 0, true);
      if (d) {
        kissat_analyze (solver, d);
        KISSAT_assert (solver->inconsistent);
        kissat_verbose (solver,
                        "lucky inconsistency forward assigning to true");
        return 20;
      }
    }
  }
  kissat_message (solver, "lucky in forward setting literals to true");
  return 10;
}

static int backward_false_satisfiable (struct kissat *solver) {
  KISSAT_assert (!solver->level);
#ifndef KISSAT_QUIET
  unsigned conflicts = 0;
#endif
  import *begin = BEGIN_STACK (solver->import);
  import *end = END_STACK (solver->import);
  import *p = end;
  while (p != begin) {
    const import import = *--p;
    if (!import.imported)
      continue;
    if (import.eliminated)
      continue;
    const unsigned lit = import.lit;
    const unsigned idx = IDX (lit);
    if (!ACTIVE (idx))
      continue;
    if (VALUE (lit))
      continue;
    const unsigned not_lit = NOT (lit);
    kissat_internal_assume (solver, not_lit);
    clause *c = kissat_probing_propagate (solver, 0, true);
    if (!c)
      continue;
#ifndef KISSAT_QUIET
    conflicts++;
#endif
    if (solver->level > 1) {
      kissat_backtrack_without_updating_phases (solver, solver->level - 1);
      kissat_internal_assume (solver, lit);
      clause *d = kissat_probing_propagate (solver, 0, true);
      if (!d)
        continue;
      kissat_verbose (solver,
                      "inconsistency after %u conflicts "
                      "backward assigning %u variables to false",
                      conflicts, solver->level);
      kissat_backtrack_without_updating_phases (solver, 0);
      return 0;
    } else {
      LOG ("failed literal %s", LOGLIT (not_lit));
      kissat_analyze (solver, c);
      KISSAT_assert (!solver->level);
      clause *d = kissat_probing_propagate (solver, 0, true);
      if (d) {
        kissat_analyze (solver, d);
        KISSAT_assert (solver->inconsistent);
        kissat_verbose (solver,
                        "lucky inconsistency backward assigning to false");
        return 20;
      }
    }
  }
  kissat_message (solver, "lucky in backward setting literals to false");
  return 10;
}

static int backward_true_satisfiable (struct kissat *solver) {
  KISSAT_assert (!solver->level);
#ifndef KISSAT_QUIET
  unsigned conflicts = 0;
#endif
  import *begin = BEGIN_STACK (solver->import);
  import *end = END_STACK (solver->import);
  import *p = end;
  while (p != begin) {
    const import import = *--p;
    if (!import.imported)
      continue;
    if (import.eliminated)
      continue;
    const unsigned lit = import.lit;
    const unsigned idx = IDX (lit);
    if (!ACTIVE (idx))
      continue;
    if (VALUE (lit))
      continue;
    kissat_internal_assume (solver, lit);
    clause *c = kissat_probing_propagate (solver, 0, true);
    if (!c)
      continue;
#ifndef KISSAT_QUIET
    conflicts++;
#endif
    if (solver->level > 1) {
      kissat_backtrack_without_updating_phases (solver, solver->level - 1);
      const unsigned not_lit = NOT (lit);
      kissat_internal_assume (solver, not_lit);
      clause *d = kissat_probing_propagate (solver, 0, true);
      if (!d)
        continue;
      kissat_verbose (solver,
                      "inconsistency after %u conflicts "
                      "backward assigning %u variables to true",
                      conflicts, solver->level);
      kissat_backtrack_without_updating_phases (solver, 0);
      return 0;
    } else {
      LOG ("failed literal %s", LOGLIT (lit));
      kissat_analyze (solver, c);
      KISSAT_assert (!solver->level);
      clause *d = kissat_probing_propagate (solver, 0, true);
      if (d) {
        kissat_analyze (solver, d);
        KISSAT_assert (solver->inconsistent);
        kissat_verbose (solver,
                        "lucky inconsistency backward assigning to true");
        return 20;
      }
    }
  }
  kissat_message (solver, "lucky in backward setting literals to true");
  return 10;
}

int kissat_lucky (struct kissat *solver) {

  if (solver->inconsistent)
    return 0;

  if (!GET_OPTION (lucky))
    return 0;

  START (lucky);
  KISSAT_assert (!solver->level);
  KISSAT_assert (!solver->probing);
  solver->probing = true;
  KISSAT_assert (kissat_propagated (solver));

  int res = 0;

  if (no_all_negative_clauses (solver)) {
    for (all_variables (idx)) {
      if (!ACTIVE (idx))
        continue;
      const unsigned lit = LIT (idx);
      if (VALUE (lit))
        continue;
      kissat_internal_assume (solver, lit);
#ifndef KISSAT_NDEBUG
      clause *c =
#endif
          kissat_probing_propagate (solver, 0, true);
      KISSAT_assert (!c);
    }
    kissat_verbose (solver, "set all variables to true");
    KISSAT_assert (kissat_propagated (solver));
    KISSAT_assert (!solver->unassigned);
    res = 10;
  }

  if (!res && no_all_positive_clauses (solver)) {
    for (all_variables (idx)) {
      if (!ACTIVE (idx))
        continue;
      const unsigned lit = LIT (idx);
      if (VALUE (lit))
        continue;
      const unsigned not_lit = NOT (lit);
      kissat_internal_assume (solver, not_lit);
#ifndef KISSAT_NDEBUG
      clause *c =
#endif
          kissat_probing_propagate (solver, 0, true);
      KISSAT_assert (!c);
    }
    kissat_verbose (solver, "set all variables to false");
    KISSAT_assert (kissat_propagated (solver));
    KISSAT_assert (!solver->unassigned);
    res = 10;
  }

  const unsigned active_before = solver->active;

  if (!res)
    res = forward_false_satisfiable (solver);

  if (!res)
    res = forward_true_satisfiable (solver);

  if (!res)
    res = backward_false_satisfiable (solver);

  if (!res)
    res = backward_true_satisfiable (solver);

  const unsigned active_after = solver->active;
  const unsigned units = active_before - active_after;

  if (!res && units)
    kissat_message (solver, "lucky %u units", units);

#ifndef KISSAT_QUIET
  bool success = res || units;
#endif
  KISSAT_assert (solver->probing);
  solver->probing = false;
  REPORT (!success, 'l');
  STOP (lucky);

  return res;
}

ABC_NAMESPACE_IMPL_END
