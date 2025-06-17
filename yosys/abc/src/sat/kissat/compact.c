#include "compact.h"
#include "inline.h"
#include "inlineheap.h"
#include "print.h"
#include "resize.h"

#include <string.h>

ABC_NAMESPACE_IMPL_START

static void reimport_literal (kissat *solver, unsigned eidx,
                              unsigned mlit) {
  import *import = &PEEK_STACK (solver->import, eidx);
  KISSAT_assert (import->imported);
  KISSAT_assert (!import->eliminated);
  LOG ("reimporting external variable %u as internal literal %u (was %u)",
       eidx, mlit, import->lit);
  import->lit = mlit;
}

unsigned kissat_compact_literals (kissat *solver, unsigned *mfixed_ptr) {
  INC (compacted);
#if !defined(KISSAT_QUIET) || !defined(KISSAT_NDEBUG)
  const unsigned active = solver->active;
#ifndef KISSAT_QUIET
  const unsigned inactive = solver->vars - active;
  kissat_phase (solver, "compact", GET (compacted),
                "compacting garbage collection "
                "(%u inactive variables %.2f%%)",
                inactive, kissat_percent (inactive, solver->vars));
#endif
#endif
#ifdef LOGGING
  KISSAT_assert (!solver->compacting);
  solver->compacting = true;
#endif
  unsigned mfixed = INVALID_LIT;
  unsigned vars = 0;
  for (all_variables (iidx)) {
    const flags *const flags = FLAGS (iidx);
    if (flags->eliminated)
      continue;
    const unsigned ilit = LIT (iidx);
    unsigned mlit;
    if (flags->fixed) {
      const value value = kissat_fixed (solver, ilit);
      KISSAT_assert (value);
      if (mfixed == INVALID_LIT) {
        mlit = mfixed = LIT (vars);
        LOG2 ("first fixed %u mapped to %u assigned to %d", ilit, mfixed,
              value);
        if (value < 0)
          mfixed = NOT (mfixed);
        LOG2 ("all other fixed mapped to %u", mfixed);
        vars++;
      } else if (value < 0) {
        mlit = NOT (mfixed);
        LOG2 ("negatively fixed %u mapped to %u", ilit, mlit);
      } else {
        mlit = mfixed;
        LOG2 ("positively fixed %u mapped to %u", ilit, mlit);
      }
    } else if (flags->active) {
      KISSAT_assert (flags->active);
      mlit = LIT (vars);
      LOG2 ("remapping %u to %u", ilit, mlit);
      vars++;
    } else {
      const int elit = PEEK_STACK (solver->export_, iidx);
      if (elit) {
        const unsigned eidx = ABS (elit);
        import *import = &PEEK_STACK (solver->import, eidx);
        KISSAT_assert (import->imported);
        KISSAT_assert (!import->eliminated);
        import->imported = false;
        LOG2 ("external variable %d not imported anymore", eidx);
        POKE_STACK (solver->export_, iidx, 0);
      } else
        LOG2 ("skipping inactive %u", ilit);
      continue;
    }
    KISSAT_assert (mlit <= ilit);
    KISSAT_assert (mlit != NOT (ilit));
    if (mlit == ilit)
      continue;
    const int elit = PEEK_STACK (solver->export_, iidx);
    const unsigned eidx = ABS (elit);
    if (elit < 0)
      mlit = NOT (mlit);
    reimport_literal (solver, eidx, mlit);
  }
  *mfixed_ptr = mfixed;
  LOG ("compacting to %u variables %.2f%% from %u", vars,
       kissat_percent (vars, solver->vars), solver->vars);
  KISSAT_assert (vars == active || vars == active + 1);
  return vars;
}

static void compact_literal (kissat *solver, unsigned dst_lit,
                             unsigned src_lit) {
  KISSAT_assert (dst_lit < src_lit);
  KISSAT_assert (dst_lit != NOT (src_lit));
  const unsigned dst_idx = IDX (dst_lit);
  const unsigned src_idx = IDX (src_lit);
  KISSAT_assert (dst_idx != src_idx);
  LOG ("mapping old internal literal %u to %u", src_lit, dst_lit);
  solver->assigned[dst_idx] = solver->assigned[src_idx];
  solver->flags[dst_idx] = solver->flags[src_idx];

  solver->phases.best[dst_idx] = solver->phases.best[src_idx];
  solver->phases.saved[dst_idx] = solver->phases.saved[src_idx];
  solver->phases.target[dst_idx] = solver->phases.target[src_idx];

  const unsigned not_src_lit = NOT (src_lit);
  const unsigned not_dst_lit = NOT (dst_lit);
  solver->values[dst_lit] = solver->values[src_lit];
  solver->values[not_dst_lit] = solver->values[not_src_lit];
}

static unsigned map_idx (kissat *solver, unsigned iidx) {
  int elit = PEEK_STACK (solver->export_, iidx);
  if (!elit)
    return INVALID_IDX;
  KISSAT_assert (elit);
  const unsigned eidx = ABS (elit);
  KISSAT_assert (eidx);
  import *import = &PEEK_STACK (solver->import, eidx);
  KISSAT_assert (import->imported);
  if (import->eliminated)
    return INVALID_IDX;
  const unsigned mlit = import->lit;
  const unsigned midx = IDX (mlit);
  KISSAT_assert (midx <= iidx);
  return midx;
}

static void compact_queue (kissat *solver) {
  LOG ("compacting queue");
  links *links = solver->links, *l;
  unsigned *p = &solver->queue.first, prev = DISCONNECT;
  solver->queue.stamp = 0;
  for (unsigned idx; !DISCONNECTED (idx = *p); p = &l->next) {
    const unsigned midx = map_idx (solver, idx);
    KISSAT_assert (midx != INVALID_IDX);
    l = links + idx;
    l->prev = prev;
    l->stamp = ++solver->queue.stamp;
    if (idx == solver->queue.search.idx) {
      solver->queue.search.idx = midx;
      solver->queue.search.stamp = l->stamp;
    }
    *p = prev = midx;
  }
  solver->queue.last = prev;
  *p = DISCONNECT;
  for (all_variables (idx)) {
    const unsigned midx = map_idx (solver, idx);
    if (midx == INVALID_IDX)
      continue;
    links[midx] = links[idx];
  }
}

static void compact_stack (kissat *solver, unsigneds *stack) {
  unsigned *q = BEGIN_STACK (*stack);
  const unsigned *const end = END_STACK (*stack);
  for (const unsigned *p = q; p != end; p++) {
    const unsigned idx = *p;
    const unsigned midx = map_idx (solver, idx);
    if (midx == INVALID_IDX)
      continue;
    *q++ = midx;
  }
  SET_END_OF_STACK (*stack, q);
  SHRINK_STACK (*stack);
}

static void compact_scores (kissat *solver, heap *old_scores,
                            unsigned vars) {
  LOG ("compacting scores");

  heap new_scores;
  memset (&new_scores, 0, sizeof new_scores);
  kissat_resize_heap (solver, &new_scores, vars);

  if (old_scores->tainted) {
    LOG ("copying scores of tainted old scores heap");
    for (all_variables (idx)) {
      const unsigned midx = map_idx (solver, idx);
      if (midx == INVALID_IDX)
        continue;
      const double score = kissat_get_heap_score (old_scores, idx);
      kissat_update_heap (solver, &new_scores, midx, score);
    }
  } else
    LOG ("no need to copy scores of old untainted scores heap");

  LOG ("now pushing mapped literals onto new heap");
  for (all_stack (unsigned, idx, old_scores->stack)) {
    const unsigned midx = map_idx (solver, idx);
    if (midx == INVALID_IDX)
      continue;
    kissat_push_heap (solver, &new_scores, midx);
  }

  kissat_release_heap (solver, old_scores);
  *old_scores = new_scores;
}

static void compact_trail (kissat *solver) {
  LOG ("compacting trail");
  const size_t size = SIZE_ARRAY (solver->trail);
  for (size_t i = 0; i < size; i++) {
    const unsigned ilit = PEEK_ARRAY (solver->trail, i);
    const unsigned mlit = kissat_map_literal (solver, ilit, true);
    KISSAT_assert (mlit != INVALID_LIT);
    POKE_ARRAY (solver->trail, i, mlit);
    const unsigned idx = IDX (ilit);
    assigned *a = solver->assigned + idx;
    if (!a->binary)
      continue;
    const unsigned other = a->reason;
    KISSAT_assert (VALID_INTERNAL_LITERAL (other));
    const unsigned mother = kissat_map_literal (solver, other, true);
    KISSAT_assert (mother != INVALID_LIT);
    a->reason = mother;
  }
}

static void compact_frames (kissat *solver) {
  LOG ("compacting frames");
  const size_t size = SIZE_STACK (solver->frames);
  for (size_t level = 1; level < size; level++) {
    frame *frame = &FRAME (level);
    const unsigned ilit = frame->decision;
    const unsigned mlit = kissat_map_literal (solver, ilit, true);
    KISSAT_assert (mlit != INVALID_LIT);
    frame->decision = mlit;
  }
}

static void compact_export (kissat *solver, unsigned vars) {
  LOG ("compacting export");
  const size_t size = SIZE_STACK (solver->export_);
  KISSAT_assert (size <= UINT_MAX);
  KISSAT_assert (size == solver->vars);
  for (unsigned iidx = 0; iidx < size; iidx++) {
    const unsigned elit = PEEK_STACK (solver->export_, iidx);
    if (!elit)
      continue;
    const unsigned midx = map_idx (solver, iidx);
    if (midx == INVALID_IDX)
      continue;
    POKE_STACK (solver->export_, midx, elit);
  }
  RESIZE_STACK (solver->export_, vars);
  SHRINK_STACK (solver->export_);
#ifndef KISSAT_NDEBUG
  KISSAT_assert (SIZE_STACK (solver->export_) == vars);
  for (unsigned iidx = 0; iidx < vars; iidx++) {
    const int elit = PEEK_STACK (solver->export_, iidx);
    KISSAT_assert (VALID_EXTERNAL_LITERAL (elit));
    const unsigned eidx = ABS (elit);
    const import *const import = &PEEK_STACK (solver->import, eidx);
    KISSAT_assert (import->imported);
    if (import->eliminated)
      continue;
    unsigned mlit = import->lit;
    if (elit < 0)
      mlit = NOT (mlit);
    const unsigned ilit = LIT (iidx);
    KISSAT_assert (mlit == ilit);
  }
#endif
}

static void compact_units (kissat *solver, unsigned mfixed) {
  LOG ("compacting units (first fixed %u)", mfixed);
  KISSAT_assert (kissat_fixed (solver, mfixed) > 0);
  for (all_stack (int, elit, solver->units)) {
    const unsigned eidx = ABS (elit);
    const unsigned mlit = elit < 0 ? NOT (mfixed) : mfixed;
    const import *const import = &PEEK_STACK (solver->import, eidx);
    KISSAT_assert (import->imported);
    KISSAT_assert (!import->eliminated);
    const unsigned ilit = import->lit;
    if (mlit != ilit)
      reimport_literal (solver, eidx, mlit);
  }
}

static void compact_best_and_target_values (kissat *solver, unsigned vars) {
  const value *const best = solver->phases.best;
  const value *const target = solver->phases.target;
  const flags *const flags = solver->flags;

  unsigned best_assigned = 0;
  unsigned target_assigned = 0;

  for (unsigned idx = 0; idx < vars; idx++) {
    if (!flags[idx].active)
      continue;
    if (target[idx])
      target_assigned++;
    if (best[idx])
      best_assigned++;
  }

  if (solver->target_assigned != target_assigned) {
    LOG ("compacting target assigned from %u to %u",
         solver->target_assigned, target_assigned);
    solver->target_assigned = target_assigned;
  }

  if (solver->best_assigned != best_assigned) {
    LOG ("compacting best assigned from %u to %u", solver->best_assigned,
         best_assigned);
    solver->best_assigned = best_assigned;
  }
}

void kissat_finalize_compacting (kissat *solver, unsigned vars,
                                 unsigned mfixed) {
  LOG ("finalizing compacting");
  KISSAT_assert (vars <= solver->vars);
#ifdef LOGGING
  KISSAT_assert (solver->compacting);
#endif
  if (vars == solver->vars) {
#ifdef LOGGING
    solver->compacting = false;
    LOG ("number of variables does not change");
#endif
    return;
  }

  unsigned reduced = solver->vars - vars;
  LOG ("compacted number of variables from %u to %u", solver->vars, vars);

  bool first = true;
  for (all_variables (iidx)) {
    flags *flags = FLAGS (iidx);
    if (flags->fixed && first)
      first = false;
    else if (!flags->active)
      POKE_STACK (solver->export_, iidx, 0);
  }

  compact_trail (solver);

  for (all_variables (iidx)) {
    const unsigned ilit = LIT (iidx);
    const unsigned mlit = kissat_map_literal (solver, ilit, true);
    if (mlit != INVALID_LIT && ilit != mlit)
      compact_literal (solver, mlit, ilit);
  }

  if (mfixed != INVALID_LIT)
    compact_units (solver, mfixed);

  memset (solver->assigned + vars, 0, reduced * sizeof (assigned));
  memset (solver->flags + vars, 0, reduced * sizeof (flags));
  memset (solver->values + 2 * vars, 0, 2 * reduced * sizeof (value));
  memset (solver->watches + 2 * vars, 0, 2 * reduced * sizeof (watches));

  compact_queue (solver);
  compact_stack (solver, &solver->sweep_schedule);
  compact_scores (solver, SCORES, vars);
  compact_frames (solver);
  compact_export (solver, vars);
  compact_best_and_target_values (solver, vars);

  solver->vars = vars;
#ifdef LOGGING
  solver->compacting = false;
#endif
  kissat_decrease_size (solver);
}

ABC_NAMESPACE_IMPL_END
