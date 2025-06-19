#include "transitive.h"
#include "allocate.h"
#include "analyze.h"
#include "heap.h"
#include "inline.h"
#include "inlinevector.h"
#include "logging.h"
#include "print.h"
#include "proprobe.h"
#include "report.h"
#include "sort.h"
#include "terminate.h"
#include "trail.h"

#include <stddef.h>

ABC_NAMESPACE_IMPL_START

static void transitive_assign (kissat *solver, unsigned lit) {
  LOG ("transitive assign %s", LOGLIT (lit));
  value *values = solver->values;
  const unsigned not_lit = NOT (lit);
  KISSAT_assert (!values[lit]);
  KISSAT_assert (!values[not_lit]);
  values[lit] = 1;
  values[not_lit] = -1;
  PUSH_ARRAY (solver->trail, lit);
}

static void transitive_backtrack (kissat *solver, unsigned *saved) {
  value *values = solver->values;

  unsigned *end_trail = END_ARRAY (solver->trail);
  KISSAT_assert (saved <= end_trail);

  while (end_trail != saved) {
    const unsigned lit = *--end_trail;
    LOG ("transitive unassign %s", LOGLIT (lit));
    const unsigned not_lit = NOT (lit);
    KISSAT_assert (values[lit] > 0);
    KISSAT_assert (values[not_lit] < 0);
    values[lit] = values[not_lit] = 0;
  }

  SET_END_OF_ARRAY (solver->trail, saved);
  solver->propagate = saved;
  solver->level = 0;
}

static void prioritize_binaries (kissat *solver) {
  KISSAT_assert (solver->watching);
  statches large;
  INIT_STACK (large);
  watches *all_watches = solver->watches;
  for (all_literals (lit)) {
    KISSAT_assert (EMPTY_STACK (large));
    watches *watches = all_watches + lit;
    watch *begin_watches = BEGIN_WATCHES (*watches), *q = begin_watches;
    const watch *const end_watches = END_WATCHES (*watches), *p = q;
    while (p != end_watches) {
      const watch head = *q++ = *p++;
      if (head.type.binary)
        continue;
      const watch tail = *p++;
      PUSH_STACK (large, head);
      PUSH_STACK (large, tail);
      q--;
    }
    const watch *const end_large = END_STACK (large);
    watch const *r = BEGIN_STACK (large);
    while (r != end_large)
      *q++ = *r++;
    KISSAT_assert (q == end_watches);
    CLEAR_STACK (large);
  }
  RELEASE_STACK (large);
}

static bool transitive_reduce (kissat *solver, unsigned src, uint64_t limit,
                               uint64_t *reduced_ptr, unsigned *units) {
  bool res = false;
  KISSAT_assert (!VALUE (src));
  LOG ("transitive reduce %s", LOGLIT (src));
  watches *all_watches = solver->watches;
  watches *src_watches = all_watches + src;
  watch *end_src = END_WATCHES (*src_watches);
  watch *begin_src = BEGIN_WATCHES (*src_watches);
  const size_t size_src_watches = SIZE_WATCHES (*src_watches);
  const unsigned src_ticks =
      1 + kissat_cache_lines (size_src_watches, sizeof (watch));
  ADD (transitive_ticks, src_ticks);
  ADD (probing_ticks, src_ticks);
  ADD (ticks, src_ticks);
  INC (transitive_probes);
  const unsigned not_src = NOT (src);
  unsigned reduced = 0;
  bool failed = false;
  for (watch *p = begin_src; p != end_src; p++) {
    const watch src_watch = *p;
    if (!src_watch.type.binary)
      break;
    const unsigned dst = src_watch.binary.lit;
    if (dst < src)
      continue;
    if (VALUE (dst))
      continue;
    KISSAT_assert (kissat_propagated (solver));
    unsigned *saved = solver->propagate;
    KISSAT_assert (!solver->level);
    solver->level = 1;
    transitive_assign (solver, not_src);
    bool transitive = false;
    unsigned inner_ticks = 0;
    unsigned *propagate = solver->propagate;
    while (!transitive && !failed &&
           propagate != END_ARRAY (solver->trail)) {
      const unsigned lit = *propagate++;
      LOG ("transitive propagate %s", LOGLIT (lit));
      KISSAT_assert (VALUE (lit) > 0);
      const unsigned not_lit = NOT (lit);
      watches *lit_watches = all_watches + not_lit;
      const watch *const end_lit = END_WATCHES (*lit_watches);
      const watch *const begin_lit = BEGIN_WATCHES (*lit_watches);
      const size_t size_lit_watches = SIZE_WATCHES (*lit_watches);
      inner_ticks +=
          1 + kissat_cache_lines (size_lit_watches, sizeof (watch));
      for (const watch *q = begin_lit; q != end_lit; q++) {
        if (p == q)
          continue;
        const watch lit_watch = *q;
        if (!lit_watch.type.binary)
          break;
        if (not_lit == src && lit_watch.binary.lit == ILLEGAL_LIT)
          continue;
        const unsigned other = lit_watch.binary.lit;
        if (other == dst) {
          transitive = true;
          break;
        }
        const value value = VALUE (other);
        if (value < 0) {
          LOG ("both %s and %s reachable from %s", LOGLIT (NOT (other)),
               LOGLIT (other), LOGLIT (src));
          failed = true;
          break;
        }
        if (!value)
          transitive_assign (solver, other);
      }
    }

    KISSAT_assert (solver->probing);

    KISSAT_assert (solver->propagate <= propagate);
    const unsigned propagated = propagate - solver->propagate;

    ADD (transitive_propagations, propagated);
    ADD (probing_propagations, propagated);
    ADD (propagations, propagated);

    ADD (transitive_ticks, inner_ticks);
    ADD (probing_ticks, inner_ticks);
    ADD (ticks, inner_ticks);

    transitive_backtrack (solver, saved);

    if (transitive) {
      LOGBINARY (src, dst, "transitive reduce");
      INC (transitive_reduced);
      watches *dst_watches = all_watches + dst;
      watch dst_watch = src_watch;
      KISSAT_assert (dst_watch.binary.lit == dst);
      dst_watch.binary.lit = src;
      REMOVE_WATCHES (*dst_watches, dst_watch);
      kissat_delete_binary (solver, src, dst);
      p->binary.lit = ILLEGAL_LIT;
      reduced++;
      res = true;
    }

    if (failed)
      break;
    if (solver->statistics_.transitive_ticks > limit)
      break;
    if (TERMINATED (transitive_terminated_1))
      break;
  }

  if (reduced) {
    *reduced_ptr += reduced;
    KISSAT_assert (begin_src == BEGIN_WATCHES (WATCHES (src)));
    KISSAT_assert (end_src == END_WATCHES (WATCHES (src)));
    watch *q = begin_src;
    for (const watch *p = begin_src; p != end_src; p++) {
      const watch src_watch = *q++ = *p;
      if (!src_watch.type.binary) {
        *q++ = *++p;
        continue;
      }
      if (src_watch.binary.lit == ILLEGAL_LIT)
        q--;
    }
    KISSAT_assert (end_src - q == (ptrdiff_t) reduced);
    SET_END_OF_WATCHES (*src_watches, q);
  }

  if (failed) {
    LOG ("transitive failed literal %s", LOGLIT (not_src));
    INC (transitive_units);
    *units += 1;
    res = true;

    kissat_learned_unit (solver, src);

    KISSAT_assert (!solver->level);
    (void) kissat_probing_propagate (solver, 0, true);
  }

  return res;
}

static inline bool less_stable_transitive (kissat *solver,
                                           const flags *const flags,
                                           const heap *scores, unsigned a,
                                           unsigned b) {
#ifdef KISSAT_NDEBUG
  (void) solver;
#endif
  const unsigned i = IDX (a);
  const unsigned j = IDX (b);
  const bool p = flags[i].transitive;
  const bool q = flags[j].transitive;
  if (!p && q)
    return true;
  if (p && !q)
    return false;
  const double s = kissat_get_heap_score (scores, i);
  const double t = kissat_get_heap_score (scores, j);
  if (s < t)
    return true;
  if (s > t)
    return false;
  return i < j;
}

static inline unsigned less_focused_transitive (kissat *solver,
                                                const flags *const flags,
                                                const links *links,
                                                unsigned a, unsigned b) {
#ifdef KISSAT_NDEBUG
  (void) solver;
#endif
  const unsigned i = IDX (a);
  const unsigned j = IDX (b);
  const bool p = flags[i].transitive;
  const bool q = flags[j].transitive;
  if (!p && q)
    return true;
  if (p && !q)
    return false;
  const unsigned s = links[i].stamp;
  const unsigned t = links[j].stamp;
  return s < t;
}

#define LESS_STABLE_PROBE(A, B) \
  less_stable_transitive (solver, flags, scores, (A), (B))

#define LESS_FOCUSED_PROBE(A, B) \
  less_focused_transitive (solver, flags, links, (A), (B))

static void sort_stable_transitive (kissat *solver, unsigneds *probes) {
  const flags *const flags = solver->flags;
  const heap *const scores = SCORES;
  SORT_STACK (unsigned, *probes, LESS_STABLE_PROBE);
}

static void sort_focused_transitive (kissat *solver, unsigneds *probes) {
  const flags *const flags = solver->flags;
  const links *const links = solver->links;
  SORT_STACK (unsigned, *probes, LESS_FOCUSED_PROBE);
}

static void sort_transitive (kissat *solver, unsigneds *probes) {
  if (solver->stable)
    sort_stable_transitive (solver, probes);
  else
    sort_focused_transitive (solver, probes);
}

static void schedule_transitive (kissat *solver, unsigneds *probes) {
  KISSAT_assert (EMPTY_STACK (*probes));
  for (all_variables (idx))
    if (ACTIVE (idx))
      PUSH_STACK (*probes, idx);
  sort_transitive (solver, probes);
  kissat_very_verbose (solver, "scheduled %zu transitive probes",
                       SIZE_STACK (*probes));
}

void kissat_transitive_reduction (kissat *solver) {
  if (solver->inconsistent)
    return;
  KISSAT_assert (solver->watching);
  KISSAT_assert (solver->probing);
  KISSAT_assert (!solver->level);
  if (!GET_OPTION (transitive))
    return;
  if (TERMINATED (transitive_terminated_2))
    return;
  START (transitive);
  INC (transitive_reductions);
#if !defined(KISSAT_NDEBUG) || defined(METRICS)
  KISSAT_assert (!solver->transitive_reducing);
  solver->transitive_reducing = true;
#endif
  prioritize_binaries (solver);
  bool success = false;
  uint64_t reduced = 0;
  unsigned units = 0;

  SET_EFFORT_LIMIT (limit, transitive, transitive_ticks);

#ifndef KISSAT_QUIET
  const unsigned active = solver->active;
  const uint64_t old_ticks = solver->statistics_.transitive_ticks;
  kissat_extremely_verbose (
      solver, "starting with %" PRIu64 " transitive ticks", old_ticks);
  unsigned probed = 0;
#endif
  unsigneds probes;
  INIT_STACK (probes);
  schedule_transitive (solver, &probes);
  bool terminate = false;
  while (!terminate && !EMPTY_STACK (probes)) {
    const unsigned idx = POP_STACK (probes);
    solver->flags[idx].transitive = false;
    if (!ACTIVE (idx))
      continue;
    for (unsigned sign = 0; !terminate && sign < 2; sign++) {
      const unsigned lit = 2 * idx + sign;
      if (solver->values[lit])
        continue;
#ifndef KISSAT_QUIET
      probed++;
#endif
      if (transitive_reduce (solver, lit, limit, &reduced, &units))
        success = true;
      if (solver->inconsistent)
        terminate = true;
      else if (solver->statistics_.transitive_ticks > limit)
        terminate = true;
      else if (TERMINATED (transitive_terminated_3))
        terminate = true;
    }
  }
  const size_t remain = SIZE_STACK (probes);
  if (remain) {
    if (!GET_OPTION (transitivekeep)) {
      kissat_very_verbose (
          solver, "dropping remaining %zu transitive candidates", remain);
      while (!EMPTY_STACK (probes)) {
        const unsigned idx = POP_STACK (probes);
        solver->flags[idx].transitive = false;
      }
    }
  } else
    kissat_very_verbose (solver, "transitive reduction complete");
  RELEASE_STACK (probes);

#ifndef KISSAT_QUIET
  const uint64_t new_ticks = solver->statistics_.transitive_ticks;
  const uint64_t delta_ticks = new_ticks - old_ticks;
  kissat_extremely_verbose (
      solver, "finished at %" PRIu64 " after %" PRIu64 " transitive ticks",
      new_ticks, delta_ticks);
#endif
  kissat_phase (solver, "transitive", GET (probings),
                "probed %u (%.0f%%): reduced %" PRIu64 ", units %u", probed,
                kissat_percent (probed, 2 * active), reduced, units);

#if !defined(KISSAT_NDEBUG) || defined(METRICS)
  KISSAT_assert (solver->transitive_reducing);
  solver->transitive_reducing = false;
#endif
  REPORT (!success, 't');
  STOP (transitive);
#ifdef KISSAT_QUIET
  (void) success;
#endif
}

ABC_NAMESPACE_IMPL_END
