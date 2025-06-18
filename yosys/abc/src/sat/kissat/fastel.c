#include "fastel.h"
#include "dense.h"
#include "eliminate.h"
#include "inline.h"
#include "internal.h"
#include "print.h"
#include "rank.h"
#include "report.h"
#include "terminate.h"
#include "weaken.h"

ABC_NAMESPACE_IMPL_START

static bool fast_forward_subsumed (kissat *solver, clause *c) {
  KISSAT_assert (!c->garbage);
  KISSAT_assert (!c->redundant);
  unsigned max_occurring = INVALID_LIT;
  size_t max_occurrence = 0;
  watches *all_watches = solver->watches;
  mark *marks = solver->marks;
  value *values = solver->values;
  for (all_literals_in_clause (other, c)) {
    const unsigned other_idx = IDX (other);
    if (!ACTIVE (other_idx))
      continue;
    watches *other_watches = all_watches + other;
    size_t other_occurrence = SIZE_WATCHES (*other_watches);
    if (other_occurrence <= max_occurrence)
      continue;
    max_occurrence = other_occurrence;
    max_occurring = other;
    marks[other] = 1;
  }
  bool subsumed = false;
  const size_t fasteloccs = GET_OPTION (fasteloccs);
  for (all_literals_in_clause (other, c)) {
    if (other == max_occurring)
      continue;
    const unsigned other_idx = IDX (other);
    if (!ACTIVE (other_idx))
      continue;
    watches *other_watches = all_watches + other;
    const size_t size_other_watches = SIZE_WATCHES (*other_watches);
    if (size_other_watches > fasteloccs)
      continue;
    for (all_binary_large_watches (watch, *other_watches)) {
      if (watch.type.binary) {
        const unsigned other2 = watch.type.lit;
        if (marks[other2]) {
          LOGBINARY (other, other2, "subsuming");
          subsumed = true;
          break;
        }
      } else {
        const reference d_ref = watch.large.ref;
        clause *d = kissat_dereference_clause (solver, d_ref);
        if (d == c)
          continue;
        if (d->garbage)
          continue;
        if (d->size > c->size)
          continue;
        KISSAT_assert (!d->redundant);
        subsumed = true;
        for (all_literals_in_clause (other2, d)) {
          if (values[other2] < 0)
            continue;
          if (!marks[other2]) {
            subsumed = false;
            break;
          }
        }
        if (subsumed)
          LOGCLS (d, "subsuming");
      }
    }
    if (subsumed)
      break;
  }
  for (all_literals_in_clause (other, c))
    marks[other] = 0;
  if (subsumed) {
    LOGCLS (c, "subsumed");
    kissat_mark_clause_as_garbage (solver, c);
    INC (subsumed);
    INC (fast_subsumed);
  }
  return subsumed;
}

static size_t flush_occurrences (kissat *solver, unsigned lit) {
  const size_t fasteloccs = GET_OPTION (fasteloccs);
  const size_t fastelclslim = GET_OPTION (fastelclslim);
  const size_t fastelsub = GET_OPTION (fastelsub);
  const value *const values = solver->values;
  const flags *const all_flags = solver->flags;
  watches *watches = &WATCHES (lit);
  watch *begin = BEGIN_WATCHES (*watches);
  watch *end = END_WATCHES (*watches);
  watch *q = begin, *p = q;
  size_t res = 0;
  while (p != end) {
    const watch watch = *q++ = *p++;
    if (watch.type.binary) {
      const unsigned other = watch.binary.lit;
      if (values[other] > 0)
        continue;
      const unsigned other_idx = IDX (other);
      const flags *other_flags = all_flags + other_idx;
      if (other_flags->eliminated) {
        q--;
        continue;
      }
    } else {
      const reference ref = watch.large.ref;
      clause *c = kissat_dereference_clause (solver, ref);
      if (c->garbage) {
        q--;
        continue;
      }
      if (c->size > fastelclslim) {
        res = fasteloccs + 1;
        break;
      }
      for (all_literals_in_clause (other, c)) {
        value other_value = values[other];
        if (other_value > 0) {
          LOGCLS (c, "%s satisfied", LOGLIT (other));
          kissat_mark_clause_as_garbage (solver, c);
          q--;
          continue;
        }
      }
      if (fastelsub && fast_forward_subsumed (solver, c)) {
        q--;
        continue;
      }
    }
    if (++res > fasteloccs)
      break;
  }
  if (q < p) {
    while (p != end)
      *q++ = *p++;
    SET_END_OF_WATCHES (*watches, q);
  }
  return res;
}

static void do_fast_resolve_binary_binary (kissat *solver, unsigned pivot,
                                           unsigned clit, unsigned dlit) {
  KISSAT_assert (!FLAGS (IDX (clit))->eliminated);
  KISSAT_assert (!FLAGS (IDX (dlit))->eliminated);
  if (clit == NOT (dlit)) {
    LOG ("resolvent tautological");
    return;
  }
  value *values = solver->values;
  value cval = values[clit];
  if (cval > 0) {
    LOG ("1st antecedent satisfied");
    return;
  }
  value dval = values[dlit];
  if (dval > 0) {
    LOG ("2nd antecedent satisfied");
    return;
  }
  if (cval < 0 && dval < 0) {
    KISSAT_assert (!solver->inconsistent);
    solver->inconsistent = true;
    LOG ("resolved empty clause");
    CHECK_AND_ADD_EMPTY ();
    ADD_EMPTY_TO_PROOF ();
    return;
  }
  if (cval < 0) {
    LOG ("resolved unit clause %s", LOGLIT (dlit));
    INC (eliminate_units);
    kissat_learned_unit (solver, dlit);
    return;
  }
  if (dval < 0) {
    LOG ("resolved unit clause %s", LOGLIT (clit));
    INC (eliminate_units);
    kissat_learned_unit (solver, clit);
    return;
  }
  if (clit == dlit) {
    LOG ("resolved unit clause %s", LOGLIT (clit));
    INC (eliminate_units);
    kissat_learned_unit (solver, clit);
    return;
  }
  KISSAT_assert (!cval);
  KISSAT_assert (!dval);
  unsigneds *clause = &solver->clause;
  KISSAT_assert (EMPTY_STACK (*clause));
  PUSH_STACK (*clause, clit);
  PUSH_STACK (*clause, dlit);
  LOGTMP ("%s resolvent", LOGVAR (pivot));
#ifndef LOGGING
  (void) pivot;
#endif
  kissat_new_irredundant_clause (solver);
  CLEAR_STACK (*clause);
}

static void do_fast_resolve_binary_large (kissat *solver, unsigned pivot,
                                          unsigned lit, clause *c) {
  KISSAT_assert (!FLAGS (IDX (lit))->eliminated);
  if (c->garbage)
    return;
  KISSAT_assert (!c->redundant);
  value *values = solver->values;
  value lit_val = values[lit];
  if (lit_val > 0) {
    LOG ("binary clause antecedent satisfied");
    return;
  }
  unsigneds *clause = &solver->clause;
  KISSAT_assert (EMPTY_STACK (*clause));
  if (!lit_val)
    PUSH_STACK (*clause, lit);
  bool satisfied = false, tautological = false;
  const unsigned not_lit = NOT (lit);
  for (all_literals_in_clause (other, c)) {
    const unsigned idx_other = IDX (other);
    if (idx_other == pivot)
      continue;
    if (other == lit)
      continue;
    if (other == not_lit) {
      LOG ("resolvent tautological");
      tautological = true;
      break;
    }
    value other_val = values[other];
    if (other_val < 0)
      continue;
    if (other_val > 0) {
      LOG ("large clause antecedent satisfied");
      kissat_mark_clause_as_garbage (solver, c);
      satisfied = true;
      break;
    }
    PUSH_STACK (*clause, other);
  }
  if (satisfied || tautological) {
    CLEAR_STACK (*clause);
    return;
  }
  size_t size = SIZE_STACK (*clause);
  if (!size) {
    KISSAT_assert (!solver->inconsistent);
    solver->inconsistent = true;
    LOG ("resolved empty clause");
    CHECK_AND_ADD_EMPTY ();
    ADD_EMPTY_TO_PROOF ();
    return;
  }
  if (size == 1) {
    const unsigned unit = PEEK_STACK (*clause, 0);
    CLEAR_STACK (*clause);
    LOG ("resolved unit clause %s", LOGLIT (unit));
    INC (eliminate_units);
    kissat_learned_unit (solver, unit);
    return;
  }
  LOGTMP ("%s resolvent", LOGVAR (pivot));
  kissat_new_irredundant_clause (solver);
  CLEAR_STACK (*clause);
}

static void do_fast_resolve_large_large (kissat *solver, unsigned pivot,
                                         clause *c, clause *d) {
  if (c->garbage)
    return;
  if (d->garbage)
    return;
  KISSAT_assert (!c->redundant);
  KISSAT_assert (!d->redundant);
  value *values = solver->values;
  mark *marks = solver->marks;
  unsigneds *clause = &solver->clause;
  KISSAT_assert (EMPTY_STACK (*clause));
  bool satisfied = false, tautological = false;
  for (all_literals_in_clause (other, c)) {
    const unsigned idx_other = IDX (other);
    if (idx_other == pivot)
      continue;
    value other_val = values[other];
    if (other_val < 0)
      continue;
    if (other_val > 0) {
      LOG ("1st antecedent satisfied");
      satisfied = true;
      break;
    }
    PUSH_STACK (*clause, other);
    marks[other] = 1;
  }
  if (satisfied || tautological) {
    for (all_stack (unsigned, other, *clause))
      marks[other] = 0;
    CLEAR_STACK (*clause);
    return;
  }
  size_t marked = SIZE_STACK (*clause);
  for (all_literals_in_clause (other, d)) {
    const unsigned idx_other = IDX (other);
    if (idx_other == pivot)
      continue;
    value other_val = values[other];
    if (other_val < 0)
      continue;
    if (other_val > 0) {
      LOG ("2nd antecedent satisfied");
      satisfied = true;
      break;
    }
    mark mark_other = marks[other];
    if (mark_other)
      continue;
    const unsigned not_other = NOT (other);
    mark mark_not_other = marks[not_other];
    if (mark_not_other) {
      LOG ("tautological resolvent");
      tautological = true;
      break;
    }
    PUSH_STACK (*clause, other);
  }
  if (satisfied || tautological) {
    for (all_stack (unsigned, other, *clause))
      marks[other] = 0;
    CLEAR_STACK (*clause);
    return;
  }
  size_t size = SIZE_STACK (*clause);
  if (!size) {
    KISSAT_assert (!solver->inconsistent);
    solver->inconsistent = true;
    LOG ("resolved empty clause");
    CHECK_AND_ADD_EMPTY ();
    ADD_EMPTY_TO_PROOF ();
    return;
  }
  if (size == 1) {
    const unsigned unit = PEEK_STACK (*clause, 0);
    CLEAR_STACK (*clause);
    marks[unit] = 0;
    LOG ("resolved unit clause %s", LOGLIT (unit));
    INC (eliminate_units);
    kissat_learned_unit (solver, unit);
    return;
  }
  LOGTMP ("%s resolvent", LOGVAR (pivot));
  kissat_new_irredundant_clause (solver);
  RESIZE_STACK (*clause, marked);
  for (all_stack (unsigned, other, *clause))
    marks[other] = 0;
  CLEAR_STACK (*clause);
}

static void do_fast_resolve (kissat *solver, unsigned pivot, watch cwatch,
                             watch dwatch) {
  KISSAT_assert (!solver->inconsistent);
  LOGWATCH (LIT (pivot), cwatch, "1st fast %s elimination antecedent",
            LOGVAR (pivot));
  LOGWATCH (NOT (LIT (pivot)), dwatch, "1st fast %s elimination antecedent",
            LOGVAR (pivot));
  const unsigned clit = cwatch.binary.lit;
  const unsigned dlit = dwatch.binary.lit;
  const reference cref = cwatch.large.ref;
  const reference dref = dwatch.large.ref;
  const bool cbin = cwatch.type.binary;
  const bool dbin = dwatch.type.binary;
  clause *c = cbin ? 0 : kissat_dereference_clause (solver, cref);
  clause *d = dbin ? 0 : kissat_dereference_clause (solver, dref);
  if (cbin && dbin)
    do_fast_resolve_binary_binary (solver, pivot, clit, dlit);
  else if (cbin && !dbin)
    do_fast_resolve_binary_large (solver, pivot, clit, d);
  else if (!cbin && dbin)
    do_fast_resolve_binary_large (solver, pivot, dlit, c);
  else {
    KISSAT_assert (!cbin), KISSAT_assert (!dbin);
    do_fast_resolve_large_large (solver, pivot, c, d);
  }
}

static void fast_delete_and_weaken_clauses (kissat *solver, unsigned lit) {
  watches *all_watches = solver->watches;
  watches *lit_watches = all_watches + lit;
  value *values = solver->values;
  for (all_binary_large_watches (watch, *lit_watches)) {
    if (watch.type.binary) {
      const unsigned other = watch.binary.lit;
      const value value = values[other];
      if (value <= 0)
        kissat_weaken_binary (solver, lit, other);
      kissat_delete_binary (solver, lit, other);
    } else {
      const reference ref = watch.large.ref;
      clause *c = kissat_dereference_clause (solver, ref);
      if (!c->garbage) {
        bool satisfied = false;
        for (all_literals_in_clause (other, c))
          if (values[other] > 0) {
            satisfied = true;
            break;
          }
        if (!satisfied)
          kissat_weaken_clause (solver, lit, c);
        kissat_mark_clause_as_garbage (solver, c);
      }
    }
  }
  RELEASE_WATCHES (*lit_watches);
}

static void do_fast_eliminate (kissat *solver, unsigned pivot) {
  LOG ("fast variable elimination of %s", LOGVAR (pivot));
  const unsigned lit = LIT (pivot);
  const unsigned not_lit = NOT (lit);
  watches *all_watches = solver->watches;
  watches *lit_watches = all_watches + lit;
  watches *not_lit_watches = all_watches + not_lit;
  LOG ("occurs %zu positively", SIZE_WATCHES (*lit_watches));
  LOG ("occurs %zu negatively", SIZE_WATCHES (*not_lit_watches));
  watch *begin_lit_watches = BEGIN_WATCHES (*lit_watches);
  watch *begin_not_lit_watches = BEGIN_WATCHES (*not_lit_watches);
  watch *end_lit_watches = END_WATCHES (*lit_watches);
  watch *end_not_lit_watches = END_WATCHES (*not_lit_watches);
  for (watch *p = begin_lit_watches; p < end_lit_watches; p++)
    for (watch *q = begin_not_lit_watches; q < end_not_lit_watches; q++) {
      do_fast_resolve (solver, pivot, *p, *q);
      if (solver->inconsistent)
        return;
      watches *new_all_watches = solver->watches;
      const size_t i = p - begin_lit_watches;
      const size_t j = q - begin_not_lit_watches;
      all_watches = new_all_watches;
      lit_watches = all_watches + lit;
      not_lit_watches = all_watches + not_lit;
      begin_lit_watches = BEGIN_WATCHES (*lit_watches);
      begin_not_lit_watches = BEGIN_WATCHES (*not_lit_watches);
      end_lit_watches = END_WATCHES (*lit_watches);
      end_not_lit_watches = END_WATCHES (*not_lit_watches);
      p = begin_lit_watches + i;
      q = begin_not_lit_watches + j;
    }
  KISSAT_assert (!solver->inconsistent);
  INC (eliminated);
  INC (fast_eliminated);
  kissat_mark_eliminated_variable (solver, pivot);
  fast_delete_and_weaken_clauses (solver, lit);
  fast_delete_and_weaken_clauses (solver, not_lit);
}

static bool can_fast_resolve_binary_binary (kissat *solver, unsigned clit,
                                            unsigned dlit) {
  KISSAT_assert (!FLAGS (IDX (clit))->eliminated);
  KISSAT_assert (!FLAGS (IDX (dlit))->eliminated);
  if (clit == NOT (dlit))
    return false;
  value *values = solver->values;
  value cval = values[clit];
  if (cval > 0)
    return false;
  value dval = values[dlit];
  if (dval > 0)
    return false;
  if (cval < 0 && dval < 0) {
    KISSAT_assert (!solver->inconsistent);
    solver->inconsistent = true;
    LOG ("resolved empty clause");
    CHECK_AND_ADD_EMPTY ();
    ADD_EMPTY_TO_PROOF ();
    return false;
  }
  if (cval < 0) {
    LOG ("resolved unit clause %s", LOGLIT (dlit));
    INC (eliminate_units);
    kissat_learned_unit (solver, dlit);
    return false;
  }
  if (dval < 0) {
    LOG ("resolved unit clause %s", LOGLIT (clit));
    INC (eliminate_units);
    kissat_learned_unit (solver, clit);
    return false;
  }
  if (clit == dlit) {
    LOG ("resolved unit clause %s", LOGLIT (clit));
    INC (eliminate_units);
    kissat_learned_unit (solver, clit);
    return false;
  }
  KISSAT_assert (!cval);
  KISSAT_assert (!dval);
  return true;
}

static bool can_fast_resolve_binary_large (kissat *solver, unsigned pivot,
                                           unsigned lit, clause *c) {
  KISSAT_assert (!FLAGS (IDX (lit))->eliminated);
  if (c->garbage)
    return false;
  KISSAT_assert (!c->redundant);
  value *values = solver->values;
  value lit_val = values[lit];
  if (lit_val > 0)
    return false;
  const unsigned not_lit = NOT (lit);
  bool found_lit = false;
  for (all_literals_in_clause (other, c)) {
    if (other == lit)
      found_lit = true;
    if (other == not_lit)
      return false;
    value other_val = values[other];
    if (other_val > 0) {
      LOG ("large clause antecedent satisfied");
      kissat_mark_clause_as_garbage (solver, c);
      return false;
    }
  }
  if (found_lit) {
    unsigneds *clause = &solver->clause;
    KISSAT_assert (EMPTY_STACK (*clause));
    for (all_literals_in_clause (other, c)) {
      const unsigned idx = IDX (other);
      if (idx == pivot)
        continue;
      value value = values[other];
      if (value < 0)
        continue;
      KISSAT_assert (!value);
      PUSH_STACK (*clause, other);
    }
    LOGTMP ("self-subsuming resolvent");
    INC (strengthened);
    INC (fast_strengthened);
    const size_t size = SIZE_STACK (*clause);
    const reference ref = kissat_reference_clause (solver, c);
    if (!size) {
      KISSAT_assert (!solver->inconsistent);
      solver->inconsistent = true;
      LOG ("resolved empty clause");
      CHECK_AND_ADD_EMPTY ();
      ADD_EMPTY_TO_PROOF ();
    } else if (size == 1) {
      const unsigned unit = PEEK_STACK (*clause, 0);
      LOG ("resolved %s unit clause", LOGLIT (unit));
      INC (eliminate_units);
      kissat_learned_unit (solver, unit);
    } else
      kissat_new_irredundant_clause (solver);
    CLEAR_STACK (*clause);
    c = kissat_dereference_clause (solver, ref);
    LOGCLS (c, "self-subsuming antecedent");
    kissat_mark_clause_as_garbage (solver, c);
    return false;
  }
  return true;
}

static bool can_fast_resolve_large_large (kissat *solver, unsigned pivot,
                                          clause *c, clause *d) {
  if (c->garbage)
    return false;
  if (d->garbage)
    return false;
  KISSAT_assert (!c->redundant);
  KISSAT_assert (!d->redundant);
  value *values = solver->values;
  mark *marks = solver->marks;
  bool satisfied = false;
  unsigneds *clause = &solver->clause;
  KISSAT_assert (EMPTY_STACK (*clause));
  for (all_literals_in_clause (other, c)) {
    const unsigned idx_other = IDX (other);
    if (idx_other == pivot)
      continue;
    value other_val = values[other];
    if (other_val < 0)
      continue;
    if (other_val > 0) {
      satisfied = true;
      LOGCLS (c, "%s satisfied", LOGLIT (other));
      kissat_mark_clause_as_garbage (solver, c);
      break;
    }
    KISSAT_assert (!marks[other]);
    marks[other] = 1;
    PUSH_STACK (*clause, other);
  }
  bool tautological = false;
  if (!satisfied) {
    for (all_literals_in_clause (other, d)) {
      const unsigned idx_other = IDX (other);
      if (idx_other == pivot)
        continue;
      value other_val = values[other];
      if (other_val < 0)
        continue;
      if (other_val > 0) {
        satisfied = true;
        LOGCLS (d, "%s satisfied", LOGLIT (other));
        kissat_mark_clause_as_garbage (solver, d);
        break;
      }
      const unsigned not_other = NOT (other);
      const mark mark_not_other = marks[not_other];
      if (mark_not_other) {
        tautological = true;
        break;
      }
      const mark other_mark = marks[other];
      if (other_mark)
        continue;
      PUSH_STACK (*clause, other);
    }
  }
  for (all_literals_in_clause (other, c))
    marks[other] = 0;
  bool strengthened = false;
  if (!satisfied && !tautological) {
    const size_t size = SIZE_STACK (*clause);
    if (!size) {
      KISSAT_assert (!solver->inconsistent);
      solver->inconsistent = true;
      LOG ("resolved empty clause");
      CHECK_AND_ADD_EMPTY ();
      ADD_EMPTY_TO_PROOF ();
      strengthened = true;
    } else if (size == 1) {
      const unsigned unit = PEEK_STACK (*clause, 0);
      LOG ("resolved %s unit clause", LOGLIT (unit));
      INC (eliminate_units);
      kissat_learned_unit (solver, unit);
      strengthened = true;
    } else {
      bool c_subsumed = false, d_subsumed = false;
      bool marked = false;
      if (size < c->size) {
        marked = true;
        for (all_stack (unsigned, other, *clause))
          marks[other] = 1;
        size_t count = 0;
        for (all_literals_in_clause (other, c))
          if (marks[other])
            count++;
        c_subsumed = (count >= size);
      }
      if (size < d->size) {
        if (!marked) {
          marked = true;
          for (all_stack (unsigned, other, *clause))
            marks[other] = 1;
        }
        size_t count = 0;
        for (all_literals_in_clause (other, d))
          if (marks[other])
            count++;
        d_subsumed = (count >= size);
      }
      if (marked) {
        for (all_stack (unsigned, other, *clause))
          marks[other] = 0;
      }
      if (c_subsumed || d_subsumed) {
        LOGTMP ("self-subsuming resolvent");
        INC (strengthened);
        INC (fast_strengthened);
        const reference c_ref = kissat_reference_clause (solver, c);
        const reference d_ref = kissat_reference_clause (solver, d);
        kissat_new_irredundant_clause (solver);
        strengthened = true;
        if (c_subsumed) {
          c = kissat_dereference_clause (solver, c_ref);
          LOGCLS (c, "self-subsuming antecedent");
          kissat_mark_clause_as_garbage (solver, c);
        }
        if (d_subsumed) {
          d = kissat_dereference_clause (solver, d_ref);
          LOGCLS (d, "self-subsuming antecedent");
          kissat_mark_clause_as_garbage (solver, d);
        }
        if (c_subsumed && d_subsumed)
          INC (fast_subsumed);
      }
    }
  }
  CLEAR_STACK (*clause);
  return !satisfied && !tautological && !strengthened;
}

static bool can_fast_resolve (kissat *solver, unsigned pivot, watch cwatch,
                              watch dwatch) {
  KISSAT_assert (!solver->inconsistent);
  const unsigned clit = cwatch.binary.lit;
  const unsigned dlit = dwatch.binary.lit;
  const reference cref = cwatch.large.ref;
  const reference dref = dwatch.large.ref;
  const bool cbin = cwatch.type.binary;
  const bool dbin = dwatch.type.binary;
  clause *c = cbin ? 0 : kissat_dereference_clause (solver, cref);
  clause *d = dbin ? 0 : kissat_dereference_clause (solver, dref);
  if (cbin && dbin)
    return can_fast_resolve_binary_binary (solver, clit, dlit);
  if (cbin && !dbin)
    return can_fast_resolve_binary_large (solver, pivot, clit, d);
  if (!cbin && dbin)
    return can_fast_resolve_binary_large (solver, pivot, dlit, c);
  KISSAT_assert (!cbin), KISSAT_assert (!dbin);
  return can_fast_resolve_large_large (solver, pivot, c, d);
}

static bool resolvents_limited (kissat *solver, unsigned pivot,
                                size_t limit) {
  const unsigned lit = LIT (pivot);
  const unsigned not_lit = NOT (lit);
  watches *all_watches = solver->watches;
  watches *lit_watches = all_watches + lit;
  watches *not_lit_watches = all_watches + not_lit;
  watch *begin_lit_watches = BEGIN_WATCHES (*lit_watches);
  watch *begin_not_lit_watches = BEGIN_WATCHES (*not_lit_watches);
  watch *end_lit_watches = END_WATCHES (*lit_watches);
  watch *end_not_lit_watches = END_WATCHES (*not_lit_watches);
  size_t resolved = 0;
  for (watch *p = begin_lit_watches; p < end_lit_watches; p++)
    for (watch *q = begin_not_lit_watches; q < end_not_lit_watches; q++) {
      if (can_fast_resolve (solver, pivot, *p, *q) && ++resolved > limit)
        return false;
      if (solver->inconsistent)
        return false;
      watches *new_all_watches = solver->watches;
      const size_t i = p - begin_lit_watches;
      const size_t j = q - begin_not_lit_watches;
      all_watches = new_all_watches;
      lit_watches = all_watches + lit;
      not_lit_watches = all_watches + not_lit;
      begin_lit_watches = BEGIN_WATCHES (*lit_watches);
      begin_not_lit_watches = BEGIN_WATCHES (*not_lit_watches);
      end_lit_watches = END_WATCHES (*lit_watches);
      end_not_lit_watches = END_WATCHES (*not_lit_watches);
      p = begin_lit_watches + i;
      q = begin_not_lit_watches + j;
    }
  return true;
}

static bool try_to_fast_eliminate (kissat *solver, unsigned pivot) {
  KISSAT_assert (!solver->inconsistent);
  if (!ACTIVE (pivot))
    return false;
  const unsigned lit = LIT (pivot);
  const unsigned not_lit = NOT (lit);
  const size_t fasteloccs = GET_OPTION (fasteloccs);
  const size_t pos = flush_occurrences (solver, lit);
  if (pos > fasteloccs)
    return false;
  const size_t neg = flush_occurrences (solver, not_lit);
  if (neg > fasteloccs)
    return false;
  const size_t sum = pos + neg;
  const size_t product = pos * neg;
  if (sum > fasteloccs)
    return false;
  const size_t fastelim = GET_OPTION (fastelim);
  if (product <= fastelim) {
    do_fast_eliminate (solver, pivot);
    return true;
  }
  if (resolvents_limited (solver, pivot, fastelim)) {
    do_fast_eliminate (solver, pivot);
    return true;
  }
  return false;
}

static void flush_eliminated_binary_clauses_of_literal (kissat *solver,
                                                        unsigned lit) {
  flags *all_flags = solver->flags;
  watches *watches = &WATCHES (lit);
  watch *begin = BEGIN_WATCHES (*watches);
  watch *end = END_WATCHES (*watches);
  watch *q = begin, *p = q;
  while (p != end) {
    watch watch = *q++ = *p++;
    if (!watch.type.binary)
      continue;
    const unsigned other = watch.binary.lit;
    const unsigned other_idx = IDX (other);
    flags *other_flags = all_flags + other_idx;
    if (other_flags->eliminated)
      q--;
  }
  SET_END_OF_WATCHES (*watches, q);
}

static void flush_eliminated_binary_clauses (kissat *solver) {
  for (all_variables (idx)) {
    const unsigned lit = LIT (idx);
    const unsigned not_lit = NOT (lit);
    flush_eliminated_binary_clauses_of_literal (solver, lit);
    flush_eliminated_binary_clauses_of_literal (solver, not_lit);
  }
}

struct candidate {
  unsigned pivot;
  unsigned score;
};

typedef struct candidate candidate;
typedef STACK (candidate) candidates;

#define RANK_CANDIDATE(CANDIDATE) ((CANDIDATE).score)

void kissat_fast_variable_elimination (kissat *solver) {
  if (solver->inconsistent)
    return;
  if (!GET_OPTION (fastel))
    return;
#ifndef KISSAT_QUIET
  const unsigned variables_before = solver->active;
#endif
  KISSAT_assert (!solver->level);
  START (fastel);
  kissat_enter_dense_mode (solver, 0);
  kissat_connect_irredundant_large_clauses (solver);
  const unsigned fastelrounds = GET_OPTION (fastelrounds);
  const size_t fasteloccs = GET_OPTION (fasteloccs);
#ifndef KISSAT_QUIET
  unsigned eliminated = 0;
#endif
  unsigned round = 0;
  candidates candidates;
  INIT_STACK (candidates);
  bool done = false;
  do {
    if (round++ >= fastelrounds)
      break;
    kissat_extremely_verbose (
        solver, "gathering candidates for fast elimination round %u",
        round);
    KISSAT_assert (EMPTY_STACK (candidates));
    flags *all_flags = solver->flags;
    for (all_variables (pivot)) {
      flags *pivot_flags = all_flags + pivot;
      if (!pivot_flags->active)
        continue;
      if (!pivot_flags->eliminate)
        continue;
      const unsigned lit = LIT (pivot);
      const size_t pos = flush_occurrences (solver, lit);
      if (pos > fasteloccs)
        continue;
      const unsigned not_lit = LIT (pivot);
      const size_t neg = flush_occurrences (solver, not_lit);
      if (neg > fasteloccs)
        continue;
      const unsigned score = pos + neg;
      if (score > fasteloccs)
        continue;
      candidate candidate = {pivot, score};
      PUSH_STACK (candidates, candidate);
    }
#ifndef KISSAT_QUIET
    const size_t size_candidates = SIZE_STACK (candidates);
    const size_t active_variables = solver->active;
    kissat_extremely_verbose (
        solver, "gathered %zu candidates %.0f%% in elimination round %u",
        size_candidates, kissat_percent (size_candidates, active_variables),
        round);
#endif
    RADIX_STACK (candidate, unsigned, candidates, RANK_CANDIDATE);
    unsigned eliminated_this_round = 0;
    for (all_stack (candidate, candidate, candidates)) {
      const unsigned pivot = candidate.pivot;
      flags *pivot_flags = all_flags + pivot;
      if (!pivot_flags->active)
        continue;
      if (!pivot_flags->eliminate)
        continue;
      if (TERMINATED (fastel_terminated_1)) {
        done = true;
        break;
      }
      if (try_to_fast_eliminate (solver, pivot))
        eliminated_this_round++;
      if (solver->inconsistent) {
        done = true;
        break;
      }
      pivot_flags->eliminate = false;
      kissat_flush_units_while_connected (solver);
      if (solver->inconsistent) {
        done = true;
        break;
      }
    }
    CLEAR_STACK (candidates);
#ifndef KISSAT_QUIET
    eliminated += eliminated_this_round;
    kissat_very_verbose (
        solver, "fast eliminated %u of %zu candidates %.0f%% in round %u",
        eliminated_this_round, size_candidates,
        kissat_percent (eliminated_this_round, size_candidates), round);
#endif
    if (!eliminated_this_round)
      done = true;
  } while (!done);
  RELEASE_STACK (candidates);
  for (all_variables (idx))
    FLAGS (idx)->eliminate = true;
  flush_eliminated_binary_clauses (solver);
  kissat_resume_sparse_mode (solver, true, 0);
#ifndef KISSAT_QUIET
  const unsigned original_variables = solver->statistics.variables_original;
  const unsigned variables_after = solver->active;
  kissat_verbose (
      solver,
      "[fastel] "
      "fast elimination of %u variables %.0f%% (%u remain %.0f%%)",
      eliminated, kissat_percent (eliminated, variables_before),
      variables_after,
      kissat_percent (variables_after, original_variables));
#endif
  STOP (fastel);
  REPORT (!eliminated, 'e');
}

ABC_NAMESPACE_IMPL_END
