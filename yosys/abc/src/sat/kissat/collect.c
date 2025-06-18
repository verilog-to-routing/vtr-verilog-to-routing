#define INLINE_SORT

#include "collect.h"
#include "allocate.h"
#include "colors.h"
#include "compact.h"
#include "inline.h"
#include "print.h"
#include "report.h"
#include "sort.c"
#include "trail.h"

#include <inttypes.h>
#include <string.h>

ABC_NAMESPACE_IMPL_START

static void flush_watched_clauses_by_literal (kissat *solver, unsigned lit,
                                              bool compact,
                                              reference start) {
  KISSAT_assert (start != INVALID_REF);

  const value *const values = solver->values;
  const assigned *const all_assigned = solver->assigned;

  const value lit_value = values[lit];
  const assigned *const lit_assigned = all_assigned + IDX (lit);
  const value lit_fixed =
      (lit_value && !lit_assigned->level) ? lit_value : 0;
  const unsigned mlit = kissat_map_literal (solver, lit, true);

  watches *lit_watches = &WATCHES (lit);
  watch *begin = BEGIN_WATCHES (*lit_watches), *q = begin;
  const watch *const end_of_watches = END_WATCHES (*lit_watches), *p = q;

  while (p != end_of_watches) {
    watch head = *p++;
    if (head.type.binary) {
      const unsigned other = head.binary.lit;
      const unsigned other_idx = IDX (other);
      const value other_value = values[other];
      const value other_fixed =
          (other_value && !all_assigned[other_idx].level) ? other_value : 0;
      const unsigned mother = kissat_map_literal (solver, other, compact);
      if (lit_fixed > 0 || other_fixed > 0 || mother == INVALID_LIT) {
        if (lit < other)
          kissat_delete_binary (solver, lit, other);
      } else {
        KISSAT_assert (!lit_fixed);
        KISSAT_assert (!other_fixed);

        {
          head.binary.lit = mother;
          *q++ = head;
#ifdef LOGGING
          if (lit < other) {
            LOGBINARY (lit, other, "SRC");
            LOGBINARY (mlit, mother, "DST");
          }
#endif
        }
      }
    } else {
      KISSAT_assert (solver->watching);
      const watch tail = *p++;
      if (!lit_fixed) {
        const reference ref = tail.large.ref;
        if (ref < start) {
          *q++ = head;
          *q++ = tail;
        }
      }
    }
  }

  KISSAT_assert (!lit_fixed || q == begin);
  SET_END_OF_WATCHES (*lit_watches, q);
#ifdef LOGGING
  const size_t size_lit_watches = SIZE_WATCHES (*lit_watches);
  LOG ("keeping %zu watches[%u]", size_lit_watches, lit);
#endif
  if (!compact)
    return;

  if (mlit == INVALID_LIT)
    return;

  watches *mlit_watches = &WATCHES (mlit);
#if defined(LOGGING) || !defined(KISSAT_NDEBUG)
  const size_t size_mlit_watches = SIZE_WATCHES (*mlit_watches);
#endif
  if (lit_fixed)
    KISSAT_assert (!size_mlit_watches);
  else if (mlit < lit) {
    KISSAT_assert (mlit != INVALID_LIT);
    KISSAT_assert (mlit < lit);
    *mlit_watches = *lit_watches;
    LOG ("copied watches[%u] = watches[%u] (size %zu)", mlit, lit,
         size_mlit_watches);
    memset (lit_watches, 0, sizeof *lit_watches);
  } else
    KISSAT_assert (mlit == lit);
}

static void flush_all_watched_clauses (kissat *solver, bool compact,
                                       reference start) {
  KISSAT_assert (solver->watching);
  LOG ("starting to flush watches at clause[%" REFERENCE_FORMAT "]", start);
  for (all_variables (idx)) {
    const unsigned lit = LIT (idx);
    flush_watched_clauses_by_literal (solver, lit, compact, start);
    const unsigned not_lit = NOT (lit);
    flush_watched_clauses_by_literal (solver, not_lit, compact, start);
  }
}

static void update_large_reason (kissat *solver, assigned *assigned,
                                 unsigned forced, clause *dst) {
  KISSAT_assert (dst->reason);
  KISSAT_assert (forced != INVALID_LIT);
  reference dst_ref = kissat_reference_clause (solver, dst);
  const unsigned forced_idx = IDX (forced);
  struct assigned *a = assigned + forced_idx;
  KISSAT_assert (!a->binary);
  if (a->reason != dst_ref) {
    LOG ("reason reference %u of %s updated to %u", a->reason,
         LOGLIT (forced), dst_ref);
    a->reason = dst_ref;
  }
  dst->reason = false;
}

static unsigned get_forced (const value *values, clause *dst) {
  KISSAT_assert (dst->reason);
  unsigned forced = INVALID_LIT;
  for (all_literals_in_clause (lit, dst)) {
    const value value = values[lit];
    if (value <= 0)
      continue;
    forced = lit;
    break;
  }
  KISSAT_assert (forced != INVALID_LIT);
  return forced;
}

static void get_forced_and_update_large_reason (kissat *solver,
                                                assigned *assigned,
                                                const value *const values,
                                                clause *dst) {
  const unsigned forced = get_forced (values, dst);
  update_large_reason (solver, assigned, forced, dst);
}

static void update_first_reducible (kissat *solver, const clause *end,
                                    clause *first_reducible) {
  if (first_reducible >= end) {
    LOG ("first reducible after end of arena");
    solver->first_reducible = INVALID_REF;
  } else if (first_reducible) {
    LOGCLS (first_reducible, "updating first reducible clause to");
    solver->first_reducible =
        kissat_reference_clause (solver, first_reducible);
  } else {
    LOG ("first reducible clause becomes invalid");
    solver->first_reducible = INVALID_REF;
  }
}

static void update_last_irredundant (kissat *solver, const clause *end,
                                     clause *last_irredundant) {
  if (!last_irredundant) {
    LOG ("no more large irredundant clauses left");
    solver->last_irredundant = INVALID_REF;
  } else if (end <= last_irredundant) {
    LOG ("last irredundant clause after end of arena");
    solver->last_irredundant = INVALID_REF;
  } else {
    LOGCLS (last_irredundant, "updating last irredundant clause to");
    reference ref = kissat_reference_clause (solver, last_irredundant);
    solver->last_irredundant = ref;
  }
}

void kissat_update_first_reducible (kissat *solver, clause *reducible) {
  KISSAT_assert (reducible);
  KISSAT_assert (!reducible->garbage);
  KISSAT_assert (reducible->redundant);
  if (solver->first_reducible != INVALID_REF) {
    reference ref = kissat_reference_clause (solver, reducible);
    if (ref >= solver->first_reducible) {
      LOG ("no need to update larger first reducible");
      return;
    }
  }
  clause *end = (clause *) END_STACK (solver->arena);
  update_first_reducible (solver, end, reducible);
}

void kissat_update_last_irredundant (kissat *solver, clause *irredundant) {
  KISSAT_assert (irredundant);
  KISSAT_assert (!irredundant->garbage);
  KISSAT_assert (!irredundant->redundant);
  if (solver->last_irredundant != INVALID_REF) {
    reference ref = kissat_reference_clause (solver, irredundant);
    if (ref <= solver->last_irredundant) {
      LOG ("no need to update smaller last irredundant");
      return;
    }
  }
  clause *end = (clause *) END_STACK (solver->arena);
  update_last_irredundant (solver, end, irredundant);
}

static void move_redundant_clauses_to_the_end (kissat *solver,
                                               reference ref) {
  INC (moved);
  KISSAT_assert (ref != INVALID_REF);
#ifndef KISSAT_NDEBUG
  const size_t size = SIZE_STACK (solver->arena);
  KISSAT_assert ((size_t) ref <= size);
#endif
  clause *begin = (clause *) (BEGIN_STACK (solver->arena) + ref);
  clause *end = (clause *) END_STACK (solver->arena);
  size_t bytes_redundant = (char *) end - (char *) begin;
  kissat_phase (solver, "move", GET (moved),
                "moving redundant clauses of %s to the end",
                FORMAT_BYTES (bytes_redundant));
  kissat_mark_reason_clauses (solver, ref);
  clause *redundant = (clause *) kissat_malloc (solver, bytes_redundant);
  clause *p = begin, *q = begin, *r = redundant;

  const value *const values = solver->values;
  assigned *assigned = solver->assigned;

  clause *last_irredundant = kissat_last_irredundant_clause (solver);

  while (p != end) {
    KISSAT_assert (!p->shrunken);
    size_t bytes = kissat_bytes_of_clause (p->size);
    if (p->redundant) {
      memcpy (r, p, bytes);
      r = (clause *) (bytes + (char *) r);
    } else {
      LOGCLS (p, "old DST");
      memmove (q, p, bytes);
      LOGCLS (q, "new DST");
      last_irredundant = q;
      if (q->reason)
        get_forced_and_update_large_reason (solver, assigned, values, q);
      q = (clause *) (bytes + (char *) q);
    }
    p = (clause *) (bytes + (char *) p);
  }
  r = redundant;
  clause *first_reducible = 0;
  while (q != end) {
    size_t bytes = kissat_bytes_of_clause (r->size);
    memcpy (q, r, bytes);
    LOGCLS (q, "new DST");
    if (q->reason)
      get_forced_and_update_large_reason (solver, assigned, values, q);
    KISSAT_assert (q->redundant);
    if (!first_reducible)
      first_reducible = q;
    r = (clause *) (bytes + (char *) r);
    q = (clause *) (bytes + (char *) q);
  }
  KISSAT_assert ((char *) r <= (char *) redundant + bytes_redundant);
  kissat_free (solver, redundant, bytes_redundant);

  KISSAT_assert (!first_reducible || first_reducible < q);

  update_first_reducible (solver, q, first_reducible);
  update_last_irredundant (solver, q, last_irredundant);
  kissat_reset_last_learned (solver);
}

static reference sparse_sweep_garbage_clauses (kissat *solver, bool compact,
                                               reference start) {
  KISSAT_assert (solver->watching);
  LOG ("sparse garbage collection starting at clause[%" REFERENCE_FORMAT
       "]",
       start);
#ifdef CHECKING_OR_PROVING
  const bool checking_or_proving = kissat_checking_or_proving (solver);
#endif
  KISSAT_assert (EMPTY_STACK (solver->added));
  KISSAT_assert (EMPTY_STACK (solver->removed));

  const value *const values = solver->values;
  assigned *assigned = solver->assigned;

#ifndef KISSAT_QUIET
  size_t flushed_garbage_clauses = 0;
  size_t flushed_satisfied_clauses = 0;
#endif
  size_t flushed = 0;

  clause *begin = (clause *) BEGIN_STACK (solver->arena);
  const clause *const end = (clause *) END_STACK (solver->arena);

  clause *first, *src, *dst;
  if (start)
    first = kissat_dereference_clause (solver, start);
  else
    first = begin;
  src = dst = first;

  clause *first_redundant = 0;
  clause *first_reducible = 0;
  clause *last_irredundant;

  if (start)
    last_irredundant = kissat_last_irredundant_clause (solver);
  else
    last_irredundant = 0;
#ifdef LOGGING
  size_t redundant_bytes = 0;
#endif
  for (clause *next; src != end; src = next) {
    if (src->garbage) {
      next = kissat_delete_clause (solver, src);
#ifndef KISSAT_QUIET
      flushed_garbage_clauses++;
#endif
      if (last_irredundant == src) {
        if (first == begin)
          last_irredundant = 0;
        else
          last_irredundant = first;
      }
      continue;
    }

    KISSAT_assert (src->size > 1);
    LOGCLS (src, "SRC");
    next = kissat_next_clause (src);
#if !defined(KISSAT_NDEBUG) || defined(CHECKING_OR_PROVING)
    const unsigned old_size = src->size;
#endif
    KISSAT_assert (SIZE_OF_CLAUSE_HEADER == sizeof (unsigned));
    *(unsigned *) dst = *(unsigned *) src;

    unsigned *q = dst->lits;

    unsigned mfirst = INVALID_LIT;
    unsigned msecond = INVALID_LIT;
    unsigned forced = INVALID_LIT;
    unsigned other = INVALID_LIT;
    unsigned non_false = 0;

    bool satisfied = false;

    for (all_literals_in_clause (lit, src)) {
#ifdef CHECKING_OR_PROVING
      if (checking_or_proving)
        PUSH_STACK (solver->removed, lit);
#endif
      if (satisfied)
        continue;

      const value tmp = values[lit];
      const unsigned idx = IDX (lit);
      const unsigned level = tmp ? assigned[idx].level : INVALID_LEVEL;

      if (tmp < 0 && !level)
        flushed++;
      else if (tmp > 0 && !level) {
        KISSAT_assert (!satisfied);
        KISSAT_assert (!dst->reason);
        LOG ("SRC satisfied by %s", LOGLIT (lit));
        satisfied = true;
      } else {
        const unsigned mlit = kissat_map_literal (solver, lit, compact);

        if (tmp > 0) {
          KISSAT_assert (level);
          forced = non_false++ ? INVALID_LIT : lit;
        } else if (tmp < 0)
          other = lit;

        if (mfirst == INVALID_LIT)
          mfirst = mlit;
        else if (msecond == INVALID_LIT)
          msecond = mlit;

        *q++ = mlit;

#ifdef CHECKING_OR_PROVING
        if (checking_or_proving)
          PUSH_STACK (solver->added, lit);
#endif
      }
    }

    if (satisfied) {
      if (dst->redundant)
        DEC (clauses_redundant);
      else
        DEC (clauses_irredundant);
#ifndef KISSAT_QUIET
      flushed_satisfied_clauses++;
#endif
#ifdef CHECKING_OR_PROVING
      if (checking_or_proving) {
        REMOVE_CHECKER_STACK (solver->removed);
        DELETE_STACK_FROM_PROOF (solver->removed);
        CLEAR_STACK (solver->added);
        CLEAR_STACK (solver->removed);
      }
#endif
      if (last_irredundant == src) {
        if (first == begin)
          last_irredundant = 0;
        else
          last_irredundant = first;
      }
      continue;
    }

    const unsigned new_size = q - dst->lits;
    KISSAT_assert (new_size <= old_size);
    KISSAT_assert (1 < new_size);

    if (new_size == 2) {
      KISSAT_assert (mfirst != INVALID_LIT);
      KISSAT_assert (msecond != INVALID_LIT);

      statistics *statistics = &solver->statistics_;
      KISSAT_assert (statistics->clauses_binary < UINT64_MAX);
      statistics->clauses_binary++;
      bool redundant = dst->redundant;
      if (redundant) {
        KISSAT_assert (statistics->clauses_redundant > 0);
        statistics->clauses_redundant--;
        redundant = false;
      } else {
        KISSAT_assert (statistics->clauses_irredundant > 0);
        statistics->clauses_irredundant--;
      }
      LOGBINARY (mfirst, msecond, "DST");
      kissat_watch_binary (solver, mfirst, msecond);

      if (dst->reason) {
        KISSAT_assert (non_false == 1);
        KISSAT_assert (other != INVALID_LIT);
        KISSAT_assert (forced != INVALID_LIT);

        const unsigned forced_idx = IDX (forced);
        struct assigned *a = assigned + forced_idx;
        KISSAT_assert (!a->binary);

        LOGBINARY (mfirst, msecond,
                   "reason clause[%u] of %s updated to binary reason",
                   a->reason, LOGLIT (forced));

        a->binary = true;
        a->reason = other;
      }

      if (!redundant && last_irredundant == src) {
        if (first == begin)
          last_irredundant = 0;
        else
          last_irredundant = first;
      }
    } else {
      KISSAT_assert (2 < new_size);

      dst->size = new_size;
      dst->shrunken = false;
      dst->searched = 2;

      LOGCLS (dst, "DST");
      if (dst->reason)
        update_large_reason (solver, assigned, forced, dst);

      clause *next_dst = kissat_next_clause (dst);

      if (dst->redundant) {
        if (!first_reducible)
          first_reducible = dst;
#ifdef LOGGING
        redundant_bytes += (char *) next_dst - (char *) dst;
#endif
        if (!first_redundant)
          first_redundant = dst;
      } else
        last_irredundant = dst;

      dst = next_dst;
    }

#ifdef CHECKING_OR_PROVING
    if (!checking_or_proving)
      continue;

    if (new_size != old_size) {
      KISSAT_assert (1 < new_size);
      KISSAT_assert (new_size < old_size);

      CHECK_AND_ADD_STACK (solver->added);
      ADD_STACK_TO_PROOF (solver->added);

      REMOVE_CHECKER_STACK (solver->removed);
      DELETE_STACK_FROM_PROOF (solver->removed);
    }
    CLEAR_STACK (solver->added);
    CLEAR_STACK (solver->removed);
#endif
  }

  update_first_reducible (solver, dst, first_reducible);
  update_last_irredundant (solver, dst, last_irredundant);
  kissat_reset_last_learned (solver);

  if (first_redundant)
    LOGCLS (first_redundant, "determined first redundant clause as");

#if !defined(KISSAT_QUIET) || defined(METRICS)
  size_t bytes = (char *) END_STACK (solver->arena) - (char *) dst;
#endif
#ifndef KISSAT_QUIET
  if (flushed)
    kissat_phase (solver, "collect", GET (garbage_collections),
                  "flushed %zu falsified literals in large clauses",
                  flushed);
  size_t flushed_clauses =
      flushed_satisfied_clauses + flushed_garbage_clauses;
  if (flushed_satisfied_clauses)
    kissat_phase (
        solver, "collect", GET (garbage_collections),
        "flushed %zu satisfied large clauses %.0f%%",
        flushed_satisfied_clauses,
        kissat_percent (flushed_satisfied_clauses, flushed_clauses));
  if (flushed_garbage_clauses)
    kissat_phase (
        solver, "collect", GET (garbage_collections),
        "flushed %zu large garbage clauses %.0f%%", flushed_garbage_clauses,
        kissat_percent (flushed_garbage_clauses, flushed_clauses));
  kissat_phase (solver, "collect", GET (garbage_collections),
                "collected %s in total", FORMAT_BYTES (bytes));
#endif
  ADD (flushed, flushed);
#ifdef METRICS
  ADD (allocated_collected, bytes);
#endif

  reference res = INVALID_REF;

  if (first_redundant && last_irredundant &&
      first_redundant < last_irredundant) {
#ifdef LOGGING
    size_t move_bytes = (char *) dst - (char *) first_redundant;
    LOG ("redundant bytes %s (%.0f%%) out of %s moving bytes",
         FORMAT_BYTES (redundant_bytes),
         kissat_percent (redundant_bytes, move_bytes),
         FORMAT_BYTES (move_bytes));
#endif
    KISSAT_assert (first_redundant < dst);
    res = kissat_reference_clause (solver, first_redundant);
    KISSAT_assert (res != INVALID_REF);
  }

  SET_END_OF_STACK (solver->arena, (ward *) dst);
  kissat_shrink_arena (solver);

#ifdef METRICS
  if (solver->statistics_.arena_garbage)
    kissat_very_verbose (solver, "still %s garbage left in arena",
                         FORMAT_BYTES (solver->statistics_.arena_garbage));
  else
    kissat_very_verbose (solver, "all garbage clauses in arena collected");
#endif

  return res;
}

static void rewatch_clauses (kissat *solver, reference start) {
  LOG ("rewatching clause[%" REFERENCE_FORMAT "] and following clauses",
       start);
  KISSAT_assert (solver->watching);

  const value *const values = solver->values;
  const assigned *const assigned = solver->assigned;
  watches *watches = solver->watches;
  ward *const arena = BEGIN_STACK (solver->arena);

  clause *end = (clause *) END_STACK (solver->arena);
  clause *c = (clause *) (BEGIN_STACK (solver->arena) + start);
  KISSAT_assert (c <= end);

  for (clause *next; c != end; c = next) {
    next = kissat_next_clause (c);

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

void kissat_sparse_collect (kissat *solver, bool compact, reference start) {
  KISSAT_assert (solver->watching);
  START (collect);
  INC (garbage_collections);
  INC (sparse_gcs);
  REPORT (1, 'G');
  unsigned vars, mfixed;
  if (compact)
    vars = kissat_compact_literals (solver, &mfixed);
  else {
    vars = solver->vars;
    mfixed = INVALID_LIT;
  }
  flush_all_watched_clauses (solver, compact, start);
  reference move = sparse_sweep_garbage_clauses (solver, compact, start);
  if (compact)
    kissat_finalize_compacting (solver, vars, mfixed);
  if (move != INVALID_REF)
    move_redundant_clauses_to_the_end (solver, move);
  rewatch_clauses (solver, start);
  REPORT (1, 'C');
  kissat_check_statistics (solver);
  STOP (collect);
}

bool kissat_compacting (kissat *solver) {
  if (!GET_OPTION (compact))
    return false;
  unsigned inactive = solver->vars - solver->active;
  unsigned limit = GET_OPTION (compactlim) / 1e2 * solver->vars;
  bool compact = (inactive > limit);
  LOG ("%u inactive variables %.0f%% <= limit %u %.0f%%", inactive,
       kissat_percent (inactive, solver->vars), limit,
       kissat_percent (limit, solver->vars));
  return compact;
}

void kissat_initial_sparse_collect (kissat *solver) {
  KISSAT_assert (!solver->level);
  KISSAT_assert (!solver->inconsistent);
  KISSAT_assert (solver->watching);
  KISSAT_assert (kissat_trail_flushed (solver));
  if (solver->statistics_.units) {
    bool compact = GET_OPTION (compact);
    kissat_sparse_collect (solver, compact, 0);
  }
  REPORT (0, '.');
}

static void dense_sweep_garbage_clauses (kissat *solver) {
  KISSAT_assert (!solver->level);
  KISSAT_assert (!solver->watching);

  LOG ("dense garbage collection");

#ifndef KISSAT_QUIET
  size_t flushed_garbage_clauses = 0;
#endif
  clause *first_reducible = 0;
  clause *last_irredundant = 0;

  clause *begin = (clause *) BEGIN_STACK (solver->arena);
  const clause *const end = (clause *) END_STACK (solver->arena);

  clause *src = begin;
  clause *dst = src;

  for (clause *next; src != end; src = next) {
    if (src->garbage) {
      next = kissat_delete_clause (solver, src);
#ifndef KISSAT_QUIET
      flushed_garbage_clauses++;
#endif
      continue;
    }
    KISSAT_assert (src->size > 1);
    LOGCLS (src, "SRC");
    next = kissat_next_clause (src);
    KISSAT_assert (SIZE_OF_CLAUSE_HEADER == sizeof (unsigned));
    *(unsigned *) dst = *(unsigned *) src;
    dst->searched = src->searched;
    dst->size = src->size;
    dst->shrunken = false;
    memmove (dst->lits, src->lits, src->size * sizeof (unsigned));
    LOGCLS (dst, "DST");
    if (!dst->redundant)
      last_irredundant = dst;
    else if (!first_reducible)
      first_reducible = dst;
    dst = kissat_next_clause (dst);
  }

  update_first_reducible (solver, dst, first_reducible);
  update_last_irredundant (solver, dst, last_irredundant);
  kissat_reset_last_learned (solver);

#if !defined(KISSAT_QUIET) || defined(METRICS)
  size_t bytes = (char *) END_STACK (solver->arena) - (char *) dst;
#endif
  kissat_phase (solver, "collect", GET (garbage_collections),
                "flushed %zu large garbage clauses",
                flushed_garbage_clauses);
  kissat_phase (solver, "collect", GET (garbage_collections),
                "collected %s in total", FORMAT_BYTES (bytes));
#ifdef METRICS
  ADD (allocated_collected, bytes);
#endif

  SET_END_OF_STACK (solver->arena, (ward *) dst);
  kissat_shrink_arena (solver);

#ifdef METRICS
  if (solver->statistics_.arena_garbage)
    kissat_very_verbose (solver, "still %s garbage left in arena",
                         FORMAT_BYTES (solver->statistics_.arena_garbage));
  else
    kissat_very_verbose (solver, "all garbage clauses in arena collected");
#endif
}

void kissat_dense_collect (kissat *solver) {
  KISSAT_assert (!solver->watching);
  KISSAT_assert (!solver->level);
  START (collect);
  INC (garbage_collections);
  INC (dense_garbage_collections);
  REPORT (1, 'G');
  dense_sweep_garbage_clauses (solver);
  REPORT (1, 'C');
  STOP (collect);
}

ABC_NAMESPACE_IMPL_END
