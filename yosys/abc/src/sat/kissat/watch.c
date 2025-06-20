#define INLINE_SORT

#include "inline.h"
#include "sort.c"

ABC_NAMESPACE_IMPL_START

void kissat_remove_binary_watch (kissat *solver, watches *watches,
                                 unsigned lit) {
  watch *const begin = BEGIN_WATCHES (*watches);
  watch *const end = END_WATCHES (*watches);
  watch *q = begin;
  watch const *p = q;
#ifndef KISSAT_NDEBUG
  bool found = false;
#endif
  while (p != end) {
    const watch watch = *q++ = *p++;
    if (!watch.type.binary) {
      *q++ = *p++;
      continue;
    }
    const unsigned other = watch.binary.lit;
    if (other != lit)
      continue;
#ifndef KISSAT_NDEBUG
    KISSAT_assert (!found);
    found = true;
#endif
    q--;
  }
  KISSAT_assert (found);
#ifdef KISSAT_COMPACT
  watches->size -= 1;
#else
  KISSAT_assert (begin + 1 <= end);
  watches->end -= 1;
#endif
  const watch empty = {.raw = INVALID_VECTOR_ELEMENT};
  end[-1] = empty;
  KISSAT_assert (solver->vectors.usable < MAX_SECTOR - 1);
  solver->vectors.usable += 1;
  kissat_check_vectors (solver);
}

void kissat_remove_blocking_watch (kissat *solver, watches *watches,
                                   reference ref) {
  KISSAT_assert (solver->watching);
  watch *const begin = BEGIN_WATCHES (*watches);
  watch *const end = END_WATCHES (*watches);
  watch *q = begin;
  watch const *p = q;
#ifndef KISSAT_NDEBUG
  bool found = false;
#endif
  while (p != end) {
    const watch head = *q++ = *p++;
    if (head.type.binary)
      continue;
    const watch tail = *q++ = *p++;
    if (tail.raw != ref)
      continue;
#ifndef KISSAT_NDEBUG
    KISSAT_assert (!found);
    found = true;
#endif
    q -= 2;
  }
  KISSAT_assert (found);
#ifdef KISSAT_COMPACT
  watches->size -= 2;
#else
  KISSAT_assert (begin + 2 <= end);
  watches->end -= 2;
#endif
  const watch empty = {.raw = INVALID_VECTOR_ELEMENT};
  end[-2] = end[-1] = empty;
  KISSAT_assert (solver->vectors.usable < MAX_SECTOR - 2);
  solver->vectors.usable += 2;
  kissat_check_vectors (solver);
}

void kissat_substitute_large_watch (kissat *solver, watches *watches,
                                    watch src, watch dst) {
  KISSAT_assert (!solver->watching);
  watch *const begin = BEGIN_WATCHES (*watches);
  const watch *const end = END_WATCHES (*watches);
#ifndef KISSAT_NDEBUG
  bool found = false;
#endif
  for (watch *p = begin; p != end; p++) {
    const watch head = *p;
    if (head.raw != src.raw)
      continue;
#ifndef KISSAT_NDEBUG
    found = true;
#endif
    *p = dst;
    break;
  }
  KISSAT_assert (found);
}

void kissat_flush_all_connected (kissat *solver) {
  KISSAT_assert (!solver->watching);
  LOG ("flush all connected binaries and clauses");
  watches *all_watches = solver->watches;
  for (all_literals (lit))
    RELEASE_WATCHES (all_watches[lit]);
}

void kissat_flush_large_watches (kissat *solver) {
  KISSAT_assert (solver->watching);
  LOG ("flush large clause watches");
  watches *all_watches = solver->watches;
  signed char *marks = solver->marks;
  for (all_literals (lit)) {
    watches *lit_watches = all_watches + lit;
    watch *begin = BEGIN_WATCHES (*lit_watches), *q = begin;
    const watch *const end = END_WATCHES (*lit_watches), *p = q;
    while (p != end) {
      const watch watch = *q++ = *p++;
      if (!watch.type.binary)
        p++, q--;
      else {
        const unsigned other = watch.binary.lit;
        if (marks[other]) {
          if (lit < other) {
            LOGBINARY (lit, other, "flushing duplicated");
            kissat_delete_binary (solver, lit, other);
          }
          q--;
        } else
          marks[other] = 1;
      }
    }
    SET_END_OF_WATCHES (*lit_watches, q);
    for (p = begin; p != q; p++) {
      KISSAT_assert (p->type.binary);
      marks[p->binary.lit] = 0;
    }
  }
}

void kissat_watch_large_clauses (kissat *solver) {
  LOG ("watching all large clauses");
  KISSAT_assert (solver->watching);

  const value *const values = solver->values;
  const assigned *const assigned = solver->assigned;
  watches *watches = solver->watches;
  ward *const arena = BEGIN_STACK (solver->arena);

  for (all_clauses (c)) {
    if (c->garbage)
      continue;

    unsigned *lits = c->lits;
    kissat_sort_literals (solver, values, assigned, c->size, lits);
    c->searched = 2;

    const reference ref = (ward *) c - arena;
    const unsigned l0 = lits[0];
    const unsigned l1 = lits[1];

    kissat_push_blocking_watch (solver, watches + l0, l1, ref);
    kissat_push_blocking_watch (solver, watches + l1, l0, ref);
  }
}

void kissat_connect_irredundant_large_clauses (kissat *solver) {
  KISSAT_assert (!solver->watching);
  LOG ("connecting all large irredundant clauses");

  clause *last_irredundant = kissat_last_irredundant_clause (solver);

  const value *const values = solver->values;
  watches *all_watches = solver->watches;
  ward *const arena = BEGIN_STACK (solver->arena);

  for (all_clauses (c)) {
    if (last_irredundant && c > last_irredundant)
      break;
    if (c->redundant)
      continue;
    if (c->garbage)
      continue;
    bool satisfied = false;
    KISSAT_assert (!solver->level);
    for (all_literals_in_clause (lit, c)) {
      const value value = values[lit];
      if (value <= 0)
        continue;
      satisfied = true;
      break;
    }
    if (satisfied) {
      kissat_mark_clause_as_garbage (solver, c);
      continue;
    }
    const reference ref = (ward *) c - arena;
    kissat_inlined_connect_clause (solver, all_watches, c, ref);
  }
}

void kissat_flush_large_connected (kissat *solver) {
  KISSAT_assert (!solver->watching);
  LOG ("flushing large connected clause references");
  size_t flushed = 0;
  for (all_literals (lit)) {
    watches *watches = &WATCHES (lit);
    watch *begin = BEGIN_WATCHES (*watches), *q = begin;
    const watch *const end_watches = END_WATCHES (*watches), *p = q;
    while (p != end_watches) {
      const watch head = *p++;
      if (head.type.binary)
        *q++ = head;
      else
        flushed++;
    }
    SET_END_OF_WATCHES (*watches, q);
  }
  LOG ("flushed %zu large clause references", flushed);
  (void) flushed;
}

ABC_NAMESPACE_IMPL_END
