#ifndef _inline_h_INCLUDED
#define _inline_h_INCLUDED

#include "inlinevector.h"
#include "logging.h"

#include "global.h"
ABC_NAMESPACE_HEADER_START

#ifdef METRICS

static inline size_t kissat_allocated (kissat *solver) {
  return solver->statistics.allocated_current;
}

#endif

static inline bool kissat_propagated (kissat *solver) {
  KISSAT_assert (BEGIN_ARRAY (solver->trail) <= solver->propagate);
  KISSAT_assert (solver->propagate <= END_ARRAY (solver->trail));
  return solver->propagate == END_ARRAY (solver->trail);
}

static inline bool kissat_trail_flushed (kissat *solver) {
  return !solver->unflushed && EMPTY_ARRAY (solver->trail);
}

static inline void kissat_reset_propagate (kissat *solver) {
  solver->propagate = BEGIN_ARRAY (solver->trail);
}

static inline value kissat_fixed (kissat *solver, unsigned lit) {
  KISSAT_assert (lit < LITS);
  const value res = solver->values[lit];
  if (!res)
    return 0;
  if (LEVEL (lit))
    return 0;
  return res;
}

static inline void kissat_mark_removed_literal (kissat *solver,
                                                unsigned lit) {
  const unsigned idx = IDX (lit);
  flags *flags = FLAGS (idx);
  if (flags->fixed)
    return;
  if (!flags->eliminate) {
    LOG ("marking %s to be eliminated", LOGVAR (idx));
    flags->eliminate = true;
    INC (variables_eliminate);
  }
}

static inline void kissat_mark_added_literal (kissat *solver,
                                              unsigned lit) {
  const unsigned idx = IDX (lit);
  flags *flags = FLAGS (idx);
  if (!flags->subsume) {
    LOG ("marking %s to forward subsume", LOGVAR (idx));
    flags->subsume = true;
    INC (variables_subsume);
  }
  const unsigned bit = 1u << NEGATED (lit);
  if (!(flags->factor & bit)) {
    flags->factor |= bit;
    LOG ("marking literal %s to factor", LOGLIT (lit));
    INC (literals_factor);
  }
}

static inline void
kissat_push_large_watch (kissat *solver, watches *watches, reference ref) {
  const watch watch = kissat_large_watch (ref);
  PUSH_WATCHES (*watches, watch);
}

static inline void kissat_push_binary_watch (kissat *solver,
                                             watches *watches,
                                             unsigned other) {
  const watch watch = kissat_binary_watch (other);
  PUSH_WATCHES (*watches, watch);
}

static inline void kissat_push_blocking_watch (kissat *solver,
                                               watches *watches,
                                               unsigned blocking,
                                               reference ref) {
  KISSAT_assert (solver->watching);
  const watch head = kissat_blocking_watch (blocking);
  PUSH_WATCHES (*watches, head);
  const watch tail = kissat_large_watch (ref);
  PUSH_WATCHES (*watches, tail);
}

static inline void kissat_watch_other (kissat *solver, unsigned lit,
                                       unsigned other) {
  LOGBINARY (lit, other, "watching %s blocking %s in", LOGLIT (lit),
             LOGLIT (other));
  watches *watches = &WATCHES (lit);
  kissat_push_binary_watch (solver, watches, other);
}

static inline void kissat_watch_binary (kissat *solver, unsigned a,
                                        unsigned b) {
  kissat_watch_other (solver, a, b);
  kissat_watch_other (solver, b, a);
}

static inline void kissat_watch_blocking (kissat *solver, unsigned lit,
                                          unsigned blocking,
                                          reference ref) {
  KISSAT_assert (solver->watching);
  LOGREF3 (ref, "watching %s blocking %s in", LOGLIT (lit),
           LOGLIT (blocking));
  watches *watches = &WATCHES (lit);
  kissat_push_blocking_watch (solver, watches, blocking, ref);
}

static inline void kissat_unwatch_blocking (kissat *solver, unsigned lit,
                                            reference ref) {
  KISSAT_assert (solver->watching);
  LOGREF3 (ref, "unwatching %s in", LOGLIT (lit));
  watches *watches = &WATCHES (lit);
  kissat_remove_blocking_watch (solver, watches, ref);
}

static inline void kissat_disconnect_binary (kissat *solver, unsigned lit,
                                             unsigned other) {
  KISSAT_assert (!solver->watching);
  watches *watches = &WATCHES (lit);
  const watch watch = kissat_binary_watch (other);
  REMOVE_WATCHES (*watches, watch);
}

static inline void
kissat_disconnect_reference (kissat *solver, unsigned lit, reference ref) {
  KISSAT_assert (!solver->watching);
  LOGREF3 (ref, "disconnecting %s in", LOGLIT (lit));
  const watch watch = kissat_large_watch (ref);
  watches *watches = &WATCHES (lit);
  REMOVE_WATCHES (*watches, watch);
}

static inline void kissat_watch_reference (kissat *solver, unsigned a,
                                           unsigned b, reference ref) {
  KISSAT_assert (solver->watching);
  kissat_watch_blocking (solver, a, b, ref);
  kissat_watch_blocking (solver, b, a, ref);
}

static inline void kissat_connect_literal (kissat *solver, unsigned lit,
                                           reference ref) {
  KISSAT_assert (!solver->watching);
  LOGREF3 (ref, "connecting %s in", LOGLIT (lit));
  watches *watches = &WATCHES (lit);
  kissat_push_large_watch (solver, watches, ref);
}

static inline clause *kissat_unchecked_dereference_clause (kissat *solver,
                                                           reference ref) {
  return (clause *) &PEEK_STACK (solver->arena, ref);
}

static inline clause *kissat_dereference_clause (kissat *solver,
                                                 reference ref) {
  clause *res = kissat_unchecked_dereference_clause (solver, ref);
  KISSAT_assert (kissat_clause_in_arena (solver, res));
  return res;
}

static inline reference kissat_reference_clause (kissat *solver,
                                                 const clause *c) {
  KISSAT_assert (kissat_clause_in_arena (solver, c));
  return (ward *) c - BEGIN_STACK (solver->arena);
}

static inline void kissat_inlined_connect_clause (kissat *solver,
                                                  watches *all_watches,
                                                  clause *c,
                                                  reference ref) {
  KISSAT_assert (!solver->watching);
  KISSAT_assert (ref == kissat_reference_clause (solver, c));
  KISSAT_assert (c == kissat_dereference_clause (solver, ref));
  for (all_literals_in_clause (lit, c)) {
    KISSAT_assert (!solver->watching);
    LOGREF3 (ref, "connecting %s in", LOGLIT (lit));
    KISSAT_assert (lit < LITS);
    watches *lit_watches = all_watches + lit;
    kissat_push_large_watch (solver, lit_watches, ref);
  }
}

static inline void kissat_watch_clause (kissat *solver, clause *c) {
  KISSAT_assert (c->searched < c->size);
  const reference ref = kissat_reference_clause (solver, c);
  kissat_watch_reference (solver, c->lits[0], c->lits[1], ref);
}

static inline int kissat_export_literal (kissat *solver, unsigned ilit) {
  const unsigned iidx = IDX (ilit);
  KISSAT_assert (iidx < (unsigned) INT_MAX);
  int elit = PEEK_STACK (solver->export_, iidx);
  if (!elit)
    return 0;
  if (NEGATED (ilit))
    elit = -elit;
  KISSAT_assert (VALID_EXTERNAL_LITERAL (elit));
  return elit;
}

static inline unsigned kissat_map_literal (kissat *solver, unsigned ilit,
                                           bool map) {
  if (!map)
    return ilit;
  int elit = kissat_export_literal (solver, ilit);
  if (!elit)
    return INVALID_LIT;
  const unsigned eidx = ABS (elit);
  const import *const import = &PEEK_STACK (solver->import, eidx);
  if (import->eliminated)
    return INVALID_LIT;
  unsigned mlit = import->lit;
  if (elit < 0)
    mlit = NOT (mlit);
  return mlit;
}

static inline clause *kissat_last_irredundant_clause (kissat *solver) {
  return (solver->last_irredundant == INVALID_REF)
             ? 0
             : kissat_dereference_clause (solver, solver->last_irredundant);
}

static inline clause *kissat_binary_conflict (kissat *solver, unsigned a,
                                              unsigned b) {
  LOGBINARY (a, b, "conflicting");
  clause *res = &solver->conflict;
  res->size = 2;
  unsigned *lits = res->lits;
  lits[0] = a;
  lits[1] = b;
  return res;
}

static inline void kissat_push_analyzed (kissat *solver, assigned *assigned,
                                         unsigned idx) {
  KISSAT_assert (idx < VARS);
  struct assigned *a = assigned + idx;
  KISSAT_assert (!a->analyzed);
  a->analyzed = true;
  PUSH_STACK (solver->analyzed, idx);
  LOG2 ("%s analyzed", LOGVAR (idx));
}

static inline bool kissat_analyzed (kissat *solver) {
  return !EMPTY_STACK (solver->analyzed);
}

static inline void
kissat_push_removable (kissat *solver, assigned *assigned, unsigned idx) {
  KISSAT_assert (idx < VARS);
  struct assigned *a = assigned + idx;
  KISSAT_assert (!a->removable);
  a->removable = true;
  PUSH_STACK (solver->removable, idx);
  LOG2 ("%s removable", LOGVAR (idx));
}

static inline void kissat_push_poisoned (kissat *solver, assigned *assigned,
                                         unsigned idx) {
  KISSAT_assert (idx < VARS);
  struct assigned *a = assigned + idx;
  KISSAT_assert (!a->poisoned);
  a->poisoned = true;
  PUSH_STACK (solver->poisoned, idx);
  LOG2 ("%s poisoned", LOGVAR (idx));
}

static inline void
kissat_push_shrinkable (kissat *solver, assigned *assigned, unsigned idx) {
  KISSAT_assert (idx < VARS);
  struct assigned *a = assigned + idx;
  KISSAT_assert (!a->shrinkable);
  a->shrinkable = true;
  PUSH_STACK (solver->shrinkable, idx);
  LOG2 ("%s shrinkable", LOGVAR (idx));
}

static inline int kissat_checking (kissat *solver) {
#ifndef KISSAT_NDEBUG
#ifdef KISSAT_NOPTIONS
  (void) solver;
#endif
  return GET_OPTION (check);
#else
  (void) solver;
  return 0;
#endif
}

static inline bool kissat_logging (kissat *solver) {
#ifdef LOGGING
#ifdef KISSAT_NOPTIONS
  (void) solver;
#endif
  return GET_OPTION (log) > 0;
#else
  (void) solver;
  return false;
#endif
}

static inline bool kissat_proving (kissat *solver) {
#ifdef KISSAT_NPROOFS
  (void) solver;
  return false;
#else
  return solver->proof != 0;
#endif
}

static inline bool kissat_checking_or_proving (kissat *solver) {
  return kissat_checking (solver) || kissat_proving (solver);
}

#if !defined(KISSAT_NDEBUG) || !defined(KISSAT_NPROOFS)
#define CHECKING_OR_PROVING
#endif

ABC_NAMESPACE_HEADER_END

#endif
