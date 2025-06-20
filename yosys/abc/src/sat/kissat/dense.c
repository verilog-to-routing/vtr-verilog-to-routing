#define INLINE_SORT

#include "dense.h"
#include "inline.h"
#include "proprobe.h"
#include "propsearch.h"
#include "trail.h"

#include "sort.c"

#include <string.h>

ABC_NAMESPACE_IMPL_START

static void flush_large_watches (kissat *solver, litpairs *irredundant) {
  KISSAT_assert (!solver->level);
  KISSAT_assert (solver->watching);
#ifndef LOGGING
  LOG ("flushing large watches");
  if (irredundant)
    LOG ("flushing and saving irredundant binary clauses too");
  else
    LOG ("keep watching irredundant binary clauses");
#endif
  const value *const values = solver->values;
  mark *const marks = solver->marks;
#ifndef KISSAT_NDEBUG
  for (all_literals (lit))
    KISSAT_assert (!marks[lit]);
#endif
  size_t flushed = 0, collected = 0;
#ifdef LOGGING
  size_t deduplicated = 0;
#endif
  watches *all_watches = solver->watches;
  unsigneds *marked = &solver->analyzed;
  for (all_literals (lit)) {
    const value lit_value = values[lit];
    watches *watches = all_watches + lit;
    watch *begin = BEGIN_WATCHES (*watches), *q = begin;
    const watch *const end_watches = END_WATCHES (*watches), *p = q;
    KISSAT_assert (EMPTY_STACK (*marked));
    while (p != end_watches) {
      const watch watch = *p++;
      if (watch.type.binary) {
        const unsigned other = watch.binary.lit;
        const value other_value = values[other];
        if (!lit_value && !other_value) {
          const mark mark = marks[other];
          if (mark) {
            if (lit < other) {
              kissat_delete_binary (solver, lit, other);
#ifdef LOGGING
              deduplicated++;
#endif
            }
          } else {
            marks[other] = 1;
            PUSH_STACK (*marked, other);
            if (irredundant) {
              const unsigned other = watch.binary.lit;
              if (lit < other) {
                const litpair litpair = {.lits = {lit, other}};
                PUSH_STACK (*irredundant, litpair);
              }
            } else
              *q++ = watch;
          }
        } else {
          KISSAT_assert (lit_value > 0 || other_value > 0);
          if (lit < other) {
            kissat_delete_binary (solver, lit, other);
            collected++;
          }
        }
      } else {
        flushed++;
        p++;
      }
    }
    if (irredundant)
      memset (watches, 0, sizeof *watches);
    else
      SET_END_OF_WATCHES (*watches, q);
    for (all_stack (unsigned, other, *marked))
      marks[other] = 0;
    CLEAR_ARRAY (*marked);
  }
  KISSAT_assert (EMPTY_STACK (*marked));
  LOG ("flushed %zu large watches", flushed);
  LOG ("removed %zu duplicated binary clauses", deduplicated);
  LOG ("collected %zu satisfied binary clauses", collected);
  if (irredundant) {
    LOG ("saved %zu irredundant binary clauses", SIZE_STACK (*irredundant));
    kissat_release_vectors (solver);
  }
  (void) collected;
  (void) flushed;
}

void kissat_enter_dense_mode (kissat *solver, litpairs *irredundant) {
  KISSAT_assert (!solver->level);
  KISSAT_assert (solver->watching);
  KISSAT_assert (kissat_propagated (solver));
  LOG ("entering dense mode with full occurrence lists");
  if (irredundant)
    flush_large_watches (solver, irredundant);
  else
    kissat_flush_large_watches (solver);
  LOG ("switched to full occurrence lists");
  solver->watching = false;
}

static void resume_watching_irredundant_binaries (kissat *solver,
                                                  litpairs *binaries) {
  KISSAT_assert (binaries);
#ifdef LOGGING
  size_t resumed_watching = 0;
#endif
  watches *all_watches = solver->watches;
  for (all_stack (litpair, litpair, *binaries)) {
    const unsigned first = litpair.lits[0];
    const unsigned second = litpair.lits[1];

    KISSAT_assert (!ELIMINATED (IDX (first)));
    KISSAT_assert (!ELIMINATED (IDX (second)));

    watches *first_watches = all_watches + first;
    watch first_watch = kissat_binary_watch (second);
    PUSH_WATCHES (*first_watches, first_watch);

    watches *second_watches = all_watches + second;
    watch second_watch = kissat_binary_watch (first);
    PUSH_WATCHES (*second_watches, second_watch);

#ifdef LOGGING
    resumed_watching++;
#endif
  }
  LOG ("resumed watching %zu binary clauses", resumed_watching);
}

static void
resume_watching_large_clauses_after_elimination (kissat *solver) {
#ifdef LOGGING
  size_t resumed_watching_redundant = 0;
  size_t resumed_watching_irredundant = 0;
#endif
  const flags *const flags = solver->flags;
  watches *watches = solver->watches;
  const value *const values = solver->values;
  const assigned *const assigned = solver->assigned;
  ward *const arena = BEGIN_STACK (solver->arena);

  for (all_clauses (c)) {
    if (c->garbage)
      continue;
    bool collect = false;
    for (all_literals_in_clause (lit, c)) {
      if (values[lit] > 0) {
        LOGCLS (c, "%s satisfied", LOGLIT (lit));
        collect = true;
        break;
      }
      const unsigned idx = IDX (lit);
      if (flags[idx].eliminated) {
        LOGCLS (c, "containing eliminated %s", LOGLIT (lit));
        collect = true;
        break;
      }
    }
    if (collect) {
      kissat_mark_clause_as_garbage (solver, c);
      continue;
    }

    KISSAT_assert (c->size > 2);

    unsigned *lits = c->lits;
    kissat_sort_literals (solver, values, assigned, c->size, lits);
    c->searched = 2;

    const reference ref = (ward *) c - arena;
    const unsigned l0 = lits[0];
    const unsigned l1 = lits[1];

    kissat_push_blocking_watch (solver, watches + l0, l1, ref);
    kissat_push_blocking_watch (solver, watches + l1, l0, ref);

#ifdef LOGGING
    if (c->redundant)
      resumed_watching_redundant++;
    else
      resumed_watching_irredundant++;
#endif
  }
  LOG ("resumed watching %zu irredundant and %zu redundant large clauses",
       resumed_watching_irredundant, resumed_watching_redundant);
}

void kissat_resume_sparse_mode (kissat *solver, bool flush_eliminated,
                                litpairs *irredundant) {
  KISSAT_assert (!solver->level);
  KISSAT_assert (!solver->watching);
  if (solver->inconsistent)
    return;
  LOG ("resuming sparse mode watching clauses");
  kissat_flush_large_connected (solver);
  LOG ("switched to watching clauses");
  solver->watching = true;
  if (irredundant) {
    LOG ("resuming watching %zu irredundant binaries",
         SIZE_STACK (*irredundant));
    resume_watching_irredundant_binaries (solver, irredundant);
  }
  if (flush_eliminated)
    resume_watching_large_clauses_after_elimination (solver);
  else
    kissat_watch_large_clauses (solver);
  LOG ("forcing to propagate units on all clauses");
  kissat_reset_propagate (solver);

  clause *conflict;
  if (solver->probing)
    conflict = kissat_probing_propagate (solver, 0, true);
  else
    conflict = kissat_search_propagate (solver);

#ifndef KISSAT_NDEBUG
  if (conflict)
    KISSAT_assert (solver->inconsistent);
  else
    KISSAT_assert (kissat_trail_flushed (solver));
#else
  (void) conflict;
#endif
}

ABC_NAMESPACE_IMPL_END
