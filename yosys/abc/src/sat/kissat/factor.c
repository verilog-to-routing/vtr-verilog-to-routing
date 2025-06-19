#include "factor.h"
#include "bump.h"
#include "clause.h"
#include "dense.h"
#include "heap.h"
#include "import.h"
#include "inline.h"
#include "inlineheap.h"
#include "inlinequeue.h"
#include "inlinevector.h"
#include "internal.h"
#include "logging.h"
#include "print.h"
#include "report.h"
#include "sort.h"
#include "terminate.h"
#include "vector.h"
#include "watch.h"

#include <string.h>

ABC_NAMESPACE_IMPL_START

#define FACTOR 1
#define QUOTIENT 2
#define NOUNTED 4

struct quotient {
  size_t id;
  struct quotient *prev, *next;
  unsigned factor;
  statches clauses;
  sizes matches;
  size_t matched;
};

typedef struct quotient quotient;

struct scores {
  double *score;
  unsigneds scored;
};

typedef struct scores scores;

struct factoring {
  kissat *solver;
  size_t size, allocated;
  unsigned initial;
  unsigned *count;
  scores *scores;
  unsigned hops;
  unsigned bound;
  unsigneds fresh;
  unsigneds counted;
  unsigneds nounted;
  references qlauses;
  uint64_t limit;
  struct {
    quotient *first, *last;
  } quotients;
  heap schedule;
};

typedef struct factoring factoring;

static void init_factoring (kissat *solver, factoring *factoring,
                            uint64_t limit) {
  memset (factoring, 0, sizeof *factoring);
  factoring->solver = solver;
  factoring->initial = factoring->allocated = factoring->size = LITS;
  factoring->limit = limit;
  factoring->bound = solver->bounds.eliminate.additional_clauses;
  if (GET_OPTION (factorstructural))
    factoring->hops = GET_OPTION (factorhops);
  const unsigned hops = factoring->hops;
  if (hops) {
    CALLOC (scores, factoring->scores, hops);
    for (unsigned i = 0; i != hops; i++) {
      scores *scores = factoring->scores + i;
      NALLOC (double, scores->score, VARS);
      double *score = scores->score;
      for (all_variables (idx))
        score[idx] = -1;
    }
  }
  CALLOC (unsigned, factoring->count, factoring->allocated);
#ifndef KISSAT_NDEBUG
  for (all_literals (lit))
    KISSAT_assert (!solver->marks[lit]);
#endif
}

static void release_quotients (factoring *factoring) {
  kissat *const solver = factoring->solver;
  mark *marks = solver->marks;
  for (quotient *q = factoring->quotients.first, *next; q; q = next) {
    next = q->next;
    unsigned factor = q->factor;
    KISSAT_assert (marks[factor] == FACTOR);
    marks[factor] = 0;
    RELEASE_STACK (q->clauses);
    RELEASE_STACK (q->matches);
    kissat_free (solver, q, sizeof *q);
  }
  const unsigned hops = factoring->hops;
  if (hops) {
    for (unsigned i = 0; i != hops; i++) {
      scores *scores = factoring->scores + i;
      unsigneds *scored = &scores->scored;
      double *score = scores->score;
      while (!EMPTY_STACK (*scored)) {
        unsigned idx = POP_STACK (*scored);
        score[idx] = -1;
      }
    }
  }
  factoring->quotients.first = factoring->quotients.last = 0;
}

static void release_factoring (factoring *factoring) {
  kissat *const solver = factoring->solver;
  KISSAT_assert (EMPTY_STACK (solver->analyzed));
  KISSAT_assert (EMPTY_STACK (factoring->counted));
  KISSAT_assert (EMPTY_STACK (factoring->nounted));
  KISSAT_assert (EMPTY_STACK (factoring->qlauses));
  DEALLOC (factoring->count, factoring->allocated);
  RELEASE_STACK (factoring->counted);
  RELEASE_STACK (factoring->nounted);
  RELEASE_STACK (factoring->fresh);
  RELEASE_STACK (factoring->qlauses);
  release_quotients (factoring);
  kissat_release_heap (solver, &factoring->schedule);
  KISSAT_assert (!(factoring->allocated & 1));
  const size_t allocated_score = factoring->allocated / 2;
  const unsigned hops = factoring->hops;
  if (hops) {
    for (unsigned i = 0; i != hops; i++) {
      scores *scores = factoring->scores + i;
      double *score = scores->score;
      DEALLOC (score, allocated_score);
      RELEASE_STACK (scores->scored);
    }
    DEALLOC (factoring->scores, hops);
  }
#ifndef KISSAT_NDEBUG
  for (all_literals (lit))
    KISSAT_assert (!solver->marks[lit]);
#endif
}

static void update_candidate (factoring *factoring, unsigned lit) {
  heap *cands = &factoring->schedule;
  kissat *const solver = factoring->solver;
  const size_t size = SIZE_WATCHES (solver->watches[lit]);
  if (size > 1) {
    kissat_adjust_heap (solver, cands, lit);
    kissat_update_heap (solver, cands, lit, size);
    if (!kissat_heap_contains (cands, lit))
      kissat_push_heap (solver, cands, lit);
  } else if (kissat_heap_contains (cands, lit))
    kissat_pop_heap (solver, cands, lit);
}

static void schedule_factorization (factoring *factoring) {
  kissat *const solver = factoring->solver;
  flags *flags = solver->flags;
  for (all_variables (idx)) {
    if (ACTIVE (idx)) {
      struct flags *f = flags + idx;
      const unsigned lit = LIT (idx);
      const unsigned not_lit = NOT (lit);
      if (f->factor & 1)
        update_candidate (factoring, lit);
      if (f->factor & 2)
        update_candidate (factoring, not_lit);
    }
  }
#ifndef KISSAT_QUIET
  heap *cands = &factoring->schedule;
  size_t size_cands = kissat_size_heap (cands);
  kissat_very_verbose (
      solver, "scheduled %zu factorization candidate literals %.0f %%",
      size_cands, kissat_percent (size_cands, LITS));
#endif
}

static quotient *new_quotient (factoring *factoring, unsigned factor) {
  kissat *const solver = factoring->solver;
  mark *marks = solver->marks;
  KISSAT_assert (!marks[factor]);
  marks[factor] = FACTOR;
  quotient *res = (quotient*)kissat_malloc (solver, sizeof *res);
  memset (res, 0, sizeof *res);
  res->factor = factor;
  quotient *last = factoring->quotients.last;
  if (last) {
    KISSAT_assert (factoring->quotients.first);
    KISSAT_assert (!last->next);
    last->next = res;
    res->id = last->id + 1;
  } else {
    KISSAT_assert (!factoring->quotients.first);
    factoring->quotients.first = res;
  }
  factoring->quotients.last = res;
  res->prev = last;
  LOG ("new quotient[%zu] with factor %s", res->id, LOGLIT (factor));
  return res;
}

static size_t first_factor (factoring *factoring, unsigned factor) {
  kissat *const solver = factoring->solver;
  watches *all_watches = solver->watches;
  watches *factor_watches = all_watches + factor;
  KISSAT_assert (!factoring->quotients.first);
  quotient *quotient = new_quotient (factoring, factor);
  statches *clauses = &quotient->clauses;
  uint64_t ticks = 0;
  for (all_binary_large_watches (watch, *factor_watches)) {
    PUSH_STACK (*clauses, watch);
#ifndef KISSAT_NDEBUG
    if (watch.type.binary)
      continue;
    const reference ref = watch.large.ref;
    clause *const c = kissat_dereference_clause (solver, ref);
    KISSAT_assert (!c->quotient);
#endif
    ticks++;
  }
  size_t res = SIZE_STACK (*clauses);
  LOG ("quotient[0] factor %s size %zu", LOGLIT (factor), res);
  KISSAT_assert (res > 1);
  ADD (factor_ticks, ticks);
  return res;
}

static void clear_nounted (kissat *solver, unsigneds *nounted) {
  mark *marks = solver->marks;
  for (all_stack (unsigned, lit, *nounted)) {
    KISSAT_assert (marks[lit] & NOUNTED);
    marks[lit] &= ~NOUNTED;
  }
  CLEAR_STACK (*nounted);
}

static void clear_qlauses (kissat *solver, references *qlauses) {
  ward *const arena = BEGIN_STACK (solver->arena);
  for (all_stack (reference, ref, *qlauses)) {
    clause *const c = (clause *) (arena + ref);
    KISSAT_assert (c->quotient);
    c->quotient = false;
  }
  CLEAR_STACK (*qlauses);
}

static double distinct_paths (factoring *factoring, unsigned src_lit,
                              unsigned dst_idx, unsigned hops) {
  kissat *const solver = factoring->solver;
  const unsigned src_idx = IDX (src_lit);
  bool matched = (src_idx == dst_idx);
  if (!hops)
    return matched;
  const unsigned next_hops = hops - 1;
  scores *scores = &factoring->scores[next_hops];
  double *score = scores->score;
  double res = score[src_idx];
  if (res >= 0)
    return res;
  res = matched;
  for (unsigned sign = 0; sign != 2; sign++) {
    const unsigned signed_src_lit = src_lit ^ sign;
    watches *const watches = &WATCHES (signed_src_lit);
    uint64_t ticks =
        1 + kissat_cache_lines (SIZE_WATCHES (*watches), sizeof (watch));
    for (all_binary_large_watches (watch, *watches)) {
      if (watch.type.binary) {
        const unsigned other = watch.binary.lit;
        res += distinct_paths (factoring, other, dst_idx, next_hops);
      } else {
        const reference ref = watch.large.ref;
        clause *c = kissat_dereference_clause (solver, ref);
        ticks++;
        for (all_literals_in_clause (other, c))
          if (other != signed_src_lit)
            res += distinct_paths (factoring, other, dst_idx, next_hops);
      }
    }
    ADD (factor_ticks, ticks);
  }
  KISSAT_assert (res >= 0);
  score[src_idx] = res;
  unsigneds *scored = &scores->scored;
  PUSH_STACK (*scored, src_idx);
  LOG ("caching %g distinct paths from %s to %s in %u hops", res,
       LOGVAR (src_idx), LOGVAR (dst_idx), hops);
  return res;
}

static double structural_score (factoring *factoring, unsigned lit) {
  const quotient *first_quotient = factoring->quotients.first;
  KISSAT_assert (first_quotient);
  const unsigned first_factor = first_quotient->factor;
#ifndef KISSAT_NDEBUG
  kissat *const solver = factoring->solver;
#endif
  const unsigned first_factor_idx = IDX (first_factor);
  return distinct_paths (factoring, lit, first_factor_idx, factoring->hops);
}

static double watches_score (factoring *factoring, unsigned lit) {
  kissat *const solver = factoring->solver;
  watches *watches = solver->watches + lit;
  double res = SIZE_WATCHES (*watches);
  LOG ("watches score %g of %s", res, LOGLIT (lit));
  return res;
}

static double tied_next_factor_score (factoring *factoring, unsigned lit) {
  if (factoring->hops)
    return structural_score (factoring, lit);
  else
    return watches_score (factoring, lit);
}

static unsigned next_factor (factoring *factoring,
                             unsigned *next_count_ptr) {
  quotient *last_quotient = factoring->quotients.last;
  KISSAT_assert (last_quotient);
  statches *last_clauses = &last_quotient->clauses;
  kissat *const solver = factoring->solver;
  watches *all_watches = solver->watches;
  unsigned *count = factoring->count;
  unsigneds *counted = &factoring->counted;
  references *qlauses = &factoring->qlauses;
  KISSAT_assert (EMPTY_STACK (*counted));
  KISSAT_assert (EMPTY_STACK (*qlauses));
  ward *const arena = BEGIN_STACK (solver->arena);
  mark *marks = solver->marks;
  const unsigned initial = factoring->initial;
  uint64_t ticks =
      1 + kissat_cache_lines (SIZE_STACK (*last_clauses), sizeof (watch));
  for (all_stack (watch, quotient_watch, *last_clauses)) {
    if (quotient_watch.type.binary) {
      const unsigned q = quotient_watch.binary.lit;
      watches *q_watches = all_watches + q;
      ticks += 1 + kissat_cache_lines (SIZE_WATCHES (*q_watches),
                                       sizeof (watch));
      for (all_binary_large_watches (next_watch, *q_watches)) {
        if (!next_watch.type.binary)
          continue;
        const unsigned next = next_watch.binary.lit;
        if (next > initial)
          continue;
        if (marks[next] & FACTOR)
          continue;
        const unsigned next_idx = IDX (next);
        if (!ACTIVE (next_idx))
          continue;
        if (!count[next])
          PUSH_STACK (*counted, next);
        count[next]++;
      }
    } else {
      const reference c_ref = quotient_watch.large.ref;
      clause *const c = (clause *) (arena + c_ref);
      KISSAT_assert (!c->quotient);
      unsigned min_lit = INVALID_LIT, factors = 0;
      size_t min_size = 0;
      ticks++;
      for (all_literals_in_clause (other, c)) {
        if (marks[other] & FACTOR) {
          if (factors++)
            break;
        } else {
          KISSAT_assert (!(marks[other] & QUOTIENT));
          marks[other] |= QUOTIENT;
          watches *other_watches = all_watches + other;
          const size_t other_size = SIZE_WATCHES (*other_watches);
          if (min_lit != INVALID_LIT && min_size <= other_size)
            continue;
          min_lit = other;
          min_size = other_size;
        }
      }
      KISSAT_assert (factors);
      if (factors == 1) {
        KISSAT_assert (min_lit != INVALID_LIT);
        watches *min_watches = all_watches + min_lit;
        unsigned c_size = c->size;
        unsigneds *nounted = &factoring->nounted;
        KISSAT_assert (EMPTY_STACK (*nounted));
        ticks += 1 + kissat_cache_lines (SIZE_WATCHES (*min_watches),
                                         sizeof (watch));
        for (all_binary_large_watches (min_watch, *min_watches)) {
          if (min_watch.type.binary)
            continue;
          const reference d_ref = min_watch.large.ref;
          if (c_ref == d_ref)
            continue;
          clause *const d = (clause *) (arena + d_ref);
          ticks++;
          if (d->quotient)
            continue;
          if (d->size != c_size)
            continue;
          unsigned next = INVALID_LIT;
          int continue_with_next_min_watch = 0;
          for (all_literals_in_clause (other, d)) {
            const mark mark = marks[other];
            if (mark & QUOTIENT)
              continue;
            if (mark & FACTOR) {
              continue_with_next_min_watch = 1;
              break;
            }
            if (mark & NOUNTED) {
              continue_with_next_min_watch = 1;
              break;
            }
            if (next != INVALID_LIT) {
              continue_with_next_min_watch = 1;
              break;
            }
            next = other;
          }
          if(continue_with_next_min_watch) {
            continue;
          }
          KISSAT_assert (next != INVALID_LIT);
          if (next > initial)
            continue;
          const unsigned next_idx = IDX (next);
          if (!ACTIVE (next_idx))
            continue;
          KISSAT_assert (!(marks[next] & (FACTOR | NOUNTED)));
          marks[next] |= NOUNTED;
          PUSH_STACK (*nounted, next);
          d->quotient = true;
          PUSH_STACK (*qlauses, d_ref);
          if (!count[next])
            PUSH_STACK (*counted, next);
          count[next]++;
        }
        clear_nounted (solver, nounted);
      }
      for (all_literals_in_clause (other, c))
        marks[other] &= ~QUOTIENT;
    }
    ADD (factor_ticks, ticks);
    ticks = 0;
    if (solver->statistics_.factor_ticks > factoring->limit)
      break;
  }
  clear_qlauses (solver, qlauses);
  unsigned next_count = 0, next = INVALID_LIT;
  if (solver->statistics_.factor_ticks <= factoring->limit) {
    unsigned ties = 0;
    for (all_stack (unsigned, lit, *counted)) {
      const unsigned lit_count = count[lit];
      if (lit_count < next_count)
        continue;
      if (lit_count == next_count) {
        KISSAT_assert (lit_count);
        ties++;
      } else {
        KISSAT_assert (lit_count > next_count);
        next_count = lit_count;
        next = lit;
        ties = 1;
      }
    }
    if (next_count < 2) {
      LOG ("next factor count %u smaller than 2", next_count);
      next = INVALID_LIT;
    } else if (ties > 1) {
      LOG ("found %u tied next factor candidate literals with count %u",
           ties, next_count);
      double next_score = -1;
      for (all_stack (unsigned, lit, *counted)) {
        const unsigned lit_count = count[lit];
        if (lit_count != next_count)
          continue;
        double lit_score = tied_next_factor_score (factoring, lit);
        KISSAT_assert (lit_score >= 0);
        LOG ("score %g of next factor candidate %s", lit_score,
             LOGLIT (lit));
        if (lit_score <= next_score)
          continue;
        next_score = lit_score;
        next = lit;
      }
      KISSAT_assert (next_score >= 0);
      KISSAT_assert (next != INVALID_LIT);
      LOG ("best score %g of next factor %s", next_score, LOGLIT (next));
    } else {
      KISSAT_assert (ties == 1);
      LOG ("single next factor %s with count %u", LOGLIT (next),
           next_count);
    }
  }
  for (all_stack (unsigned, lit, *counted))
    count[lit] = 0;
  CLEAR_STACK (*counted);
  KISSAT_assert (next == INVALID_LIT || next_count > 1);
  *next_count_ptr = next_count;
  return next;
}

static void factorize_next (factoring *factoring, unsigned next,
                            unsigned expected_next_count) {
  quotient *last_quotient = factoring->quotients.last;
  quotient *next_quotient = new_quotient (factoring, next);

  kissat *const solver = factoring->solver;
  watches *all_watches = solver->watches;
  ward *const arena = BEGIN_STACK (solver->arena);
  mark *marks = solver->marks;

  KISSAT_assert (last_quotient);
  statches *last_clauses = &last_quotient->clauses;
  statches *next_clauses = &next_quotient->clauses;
  sizes *matches = &next_quotient->matches;
  references *qlauses = &factoring->qlauses;
  KISSAT_assert (EMPTY_STACK (*qlauses));

  uint64_t ticks =
      1 + kissat_cache_lines (SIZE_STACK (*last_clauses), sizeof (watch));

  size_t i = 0;

  for (all_stack (watch, last_watch, *last_clauses)) {
    if (last_watch.type.binary) {
      const unsigned q = last_watch.binary.lit;
      watches *q_watches = all_watches + q;
      ticks += 1 + kissat_cache_lines (SIZE_WATCHES (*q_watches),
                                       sizeof (watch));
      for (all_binary_large_watches (q_watch, *q_watches))
        if (q_watch.type.binary && q_watch.binary.lit == next) {
          LOGBINARY (last_quotient->factor, q, "matched");
          LOGBINARY (next, q, "keeping");
          PUSH_STACK (*next_clauses, last_watch);
          PUSH_STACK (*matches, i);
          break;
        }
    } else {
      const reference c_ref = last_watch.large.ref;
      clause *const c = (clause *) (arena + c_ref);
      KISSAT_assert (!c->quotient);
      unsigned min_lit = INVALID_LIT, factors = 0;
      size_t min_size = 0;
      ticks++;
      for (all_literals_in_clause (other, c)) {
        if (marks[other] & FACTOR) {
          if (factors++)
            break;
        } else {
          KISSAT_assert (!(marks[other] & QUOTIENT));
          marks[other] |= QUOTIENT;
          watches *other_watches = all_watches + other;
          const size_t other_size = SIZE_WATCHES (*other_watches);
          if (min_lit != INVALID_LIT && min_size <= other_size)
            continue;
          min_lit = other;
          min_size = other_size;
        }
      }
      KISSAT_assert (factors);
      if (factors == 1) {
        KISSAT_assert (min_lit != INVALID_LIT);
        watches *min_watches = all_watches + min_lit;
        unsigned c_size = c->size;
        ticks += 1 + kissat_cache_lines (SIZE_WATCHES (*min_watches),
                                         sizeof (watch));
        for (all_binary_large_watches (min_watch, *min_watches)) {
          if (min_watch.type.binary)
            continue;
          const reference d_ref = min_watch.large.ref;
          if (c_ref == d_ref)
            continue;
          clause *const d = (clause *) (arena + d_ref);
          ticks++;
          if (d->quotient)
            continue;
          if (d->size != c_size)
            continue;
          for (all_literals_in_clause (other, d)) {
            const mark mark = marks[other];
            if (mark & QUOTIENT)
              continue;
            if (other != next)
              goto CONTINUE_WITH_NEXT_MIN_WATCH;
          }
          LOGCLS (c, "matched");
          LOGCLS (d, "keeping");
          PUSH_STACK (*next_clauses, min_watch);
          PUSH_STACK (*matches, i);
          PUSH_STACK (*qlauses, d_ref);
          d->quotient = true;
          break;
        CONTINUE_WITH_NEXT_MIN_WATCH:;
        }
      }
      for (all_literals_in_clause (other, c))
        marks[other] &= ~QUOTIENT;
    }
    i++;
  }

  clear_qlauses (solver, qlauses);
  ADD (factor_ticks, ticks);

  KISSAT_assert (expected_next_count <= SIZE_STACK (*next_clauses));
  (void) expected_next_count;
}

static quotient *best_quotient (factoring *factoring,
                                size_t *best_reduction_ptr) {
  size_t factors = 1, best_reduction = 0;
  quotient *best = 0;
  kissat *const solver = factoring->solver;
  for (quotient *q = factoring->quotients.first; q; q = q->next) {
    size_t quotients = SIZE_STACK (q->clauses);
    size_t before_factorization = quotients * factors;
    size_t after_factorization = quotients + factors;
    if (before_factorization == after_factorization)
      LOG ("quotient[%zu] factors %zu clauses into %zu thus no change",
           factors - 1, before_factorization, after_factorization);
    else if (before_factorization < after_factorization)
      LOG ("quotient[%zu] factors %zu clauses into %zu thus %zu more",
           factors - 1, before_factorization, after_factorization,
           after_factorization - before_factorization);
    else {
      size_t delta = before_factorization - after_factorization;
      LOG ("quotient[%zu] factors %zu clauses into %zu thus %zu less",
           factors - 1, before_factorization, after_factorization, delta);
      if (!best || best_reduction < delta) {
        best_reduction = delta;
        best = q;
      }
    }
    factors++;
  }
  if (!best) {
    LOG ("no decreasing quotient found");
    return 0;
  }
  LOG ("best decreasing quotient[%zu] with reduction %zu", best->id,
       best_reduction);
  *best_reduction_ptr = best_reduction;
  (void) solver;
  return best;
}

static void resize_factoring (factoring *factoring, unsigned lit) {
  kissat *const solver = factoring->solver;
  KISSAT_assert (lit > NOT (lit));
  const size_t old_size = factoring->size;
  KISSAT_assert (lit > old_size);
  const size_t old_allocated = factoring->allocated;
  size_t new_size = lit + 1;
  if (new_size > old_allocated) {
    size_t new_allocated = 2 * old_allocated;
    while (new_size > new_allocated)
      new_allocated *= 2;
    unsigned *count = factoring->count;
    count = (unsigned*)kissat_nrealloc (solver, count, old_allocated, new_allocated,
                             sizeof *count);
    const size_t delta_allocated = new_allocated - old_allocated;
    const size_t delta_bytes = delta_allocated * sizeof *count;
    memset (count + old_size, 0, delta_bytes);
    factoring->count = count;
    KISSAT_assert (!(old_allocated & 1));
    KISSAT_assert (!(new_allocated & 1));
    const size_t old_allocated_score = old_allocated / 2;
    const size_t new_allocated_score = new_allocated / 2;
    for (unsigned i = 0; i != factoring->hops; i++) {
      scores *scores = factoring->scores + i;
      double *score = scores->score;
      score = (double*)kissat_nrealloc (solver, score, old_allocated_score,
                               new_allocated_score, sizeof *score);
      for (size_t i = old_allocated_score; i != new_allocated_score; i++)
        score[i] = -1;
      scores->score = score;
    }
    factoring->allocated = new_allocated;
  }
  factoring->size = new_size;
}

static void flush_unmatched_clauses (kissat *solver, quotient *q) {
  quotient *prev = q->prev;
  sizes *q_matches = &q->matches, *prev_matches = &prev->matches;
  statches *q_clauses = &q->clauses, *prev_clauses = &prev->clauses;
  const size_t n = SIZE_STACK (*q_clauses);
  KISSAT_assert (n == SIZE_STACK (*q_matches));
  bool prev_is_first = !prev->id;
  size_t i = 0;
  while (i != n) {
    size_t j = PEEK_STACK (*q_matches, i);
    KISSAT_assert (i <= j);
    if (!prev_is_first) {
      size_t matches = PEEK_STACK (*prev_matches, j);
      POKE_STACK (*prev_matches, i, matches);
    }
    watch watch = PEEK_STACK (*prev_clauses, j);
    POKE_STACK (*prev_clauses, i, watch);
    i++;
  }
  LOG ("flushing %zu clauses of quotient[%zu]",
       SIZE_STACK (*prev_clauses) - n, prev->id);
  if (!prev_is_first)
    RESIZE_STACK (*prev_matches, n);
  RESIZE_STACK (*prev_clauses, n);
  (void) solver;
}

static void add_factored_divider (factoring *factoring, quotient *q,
                                  unsigned fresh) {
  const unsigned factor = q->factor;
  kissat *const solver = factoring->solver;
  LOGBINARY (fresh, factor, "factored %s divider", LOGLIT (factor));
  kissat_new_binary_clause (solver, fresh, factor);
  INC (clauses_factored);
  ADD (literals_factored, 2);
}

static void add_factored_quotient (factoring *factoring, quotient *q,
                                   unsigned not_fresh) {
  kissat *const solver = factoring->solver;
  LOG ("adding factored quotient[%zu] clauses", q->id);
  for (all_stack (watch, watch, q->clauses)) {
    if (watch.type.binary) {
      const unsigned other = watch.binary.lit;
      LOGBINARY (not_fresh, other, "factored quotient");
      kissat_new_binary_clause (solver, not_fresh, other);
      ADD (literals_factored, 2);
    } else {
      const reference c_ref = watch.large.ref;
      clause *const c = kissat_dereference_clause (solver, c_ref);
      unsigneds *clause = &solver->clause;
      KISSAT_assert (EMPTY_STACK (*clause));
      const unsigned factor = q->factor;
#ifndef KISSAT_NDEBUG
      bool found = false;
#endif
      PUSH_STACK (*clause, not_fresh);
      for (all_literals_in_clause (other, c)) {
        if (other == factor) {
#ifndef KISSAT_NDEBUG
          found = true;
#endif
          continue;
        }
        PUSH_STACK (*clause, other);
      }
      KISSAT_assert (found);
      ADD (literals_factored, c->size);
      kissat_new_irredundant_clause (solver);
      CLEAR_STACK (*clause);
    }
    INC (clauses_factored);
  }
}

static void eagerly_remove_watch (kissat *solver, watches *watches,
                                  watch needle) {
  watch *p = BEGIN_WATCHES (*watches);
  watch *end = END_WATCHES (*watches);
  KISSAT_assert (p != end);
  watch *last = end - 1;
  while (p->raw != needle.raw)
    p++, KISSAT_assert (p != end);
  if (p != last)
    memmove (p, p + 1, (last - p) * sizeof *p);
  SET_END_OF_WATCHES (*watches, last);
}

static void eagerly_remove_binary (kissat *solver, watches *watches,
                                   unsigned lit) {
  const watch needle = kissat_binary_watch (lit);
  eagerly_remove_watch (solver, watches, needle);
}

static void delete_unfactored (factoring *factoring, quotient *q) {
  kissat *const solver = factoring->solver;
  LOG ("deleting unfactored quotient[%zu] clauses", q->id);
  const unsigned factor = q->factor;
  for (all_stack (watch, watch, q->clauses)) {
    if (watch.type.binary) {
      const unsigned other = watch.binary.lit;
      LOGBINARY (factor, other, "deleting unfactored");
      eagerly_remove_binary (solver, &WATCHES (other), factor);
      eagerly_remove_binary (solver, &WATCHES (factor), other);
      kissat_delete_binary (solver, factor, other);
      ADD (literals_unfactored, 2);
    } else {
      const reference ref = watch.large.ref;
      clause *c = kissat_dereference_clause (solver, ref);
      LOGCLS (c, "deleting unfactored");
      for (all_literals_in_clause (lit, c))
        eagerly_remove_watch (solver, &WATCHES (lit), watch);
      kissat_mark_clause_as_garbage (solver, c);
      ADD (literals_unfactored, c->size);
    }
    INC (clauses_unfactored);
  }
}

static void update_factored (factoring *factoring, quotient *q) {
  kissat *const solver = factoring->solver;
  const unsigned factor = q->factor;
  update_candidate (factoring, factor);
  update_candidate (factoring, NOT (factor));
  for (all_stack (watch, watch, q->clauses)) {
    if (watch.type.binary) {
      const unsigned other = watch.binary.lit;
      update_candidate (factoring, other);
    } else {
      const reference ref = watch.large.ref;
      clause *c = kissat_dereference_clause (solver, ref);
      LOGCLS (c, "deleting unfactored");
      for (all_literals_in_clause (lit, c))
        if (lit != factor)
          update_candidate (factoring, lit);
    }
  }
}

static bool apply_factoring (factoring *factoring, quotient *q) {
  kissat *const solver = factoring->solver;
  const unsigned fresh = kissat_fresh_literal (solver);
  if (fresh == INVALID_LIT)
    return false;
  INC (factored);
  PUSH_STACK (factoring->fresh, fresh);
  for (quotient *p = q; p->prev; p = p->prev)
    flush_unmatched_clauses (solver, p);
  for (quotient *p = q; p; p = p->prev)
    add_factored_divider (factoring, p, fresh);
  const unsigned not_fresh = NOT (fresh);
  add_factored_quotient (factoring, q, not_fresh);
  for (quotient *p = q; p; p = p->prev)
    delete_unfactored (factoring, p);
  for (quotient *p = q; p; p = p->prev)
    update_factored (factoring, p);
  KISSAT_assert (fresh < not_fresh);
  resize_factoring (factoring, not_fresh);
  return true;
}

static void
adjust_scores_and_phases_of_fresh_varaibles (factoring *factoring) {
  const unsigned *begin = BEGIN_STACK (factoring->fresh);
  const unsigned *end = END_STACK (factoring->fresh);
  kissat *const solver = factoring->solver;
  {
    const unsigned *p = begin;
    while (p != end) {
      const unsigned lit = *p++;
      const unsigned idx = IDX (lit);
      LOG ("unbumping fresh[%zu] %s", (size_t) (p - begin - 1),
           LOGVAR (idx));
      const double score = 0;
      kissat_update_heap (solver, &solver->scores, idx, score);
    }
  }
  {
    const unsigned *p = end;
    links *links = solver->links;
    queue *queue = &solver->queue;
    while (p != begin) {
      const unsigned lit = *--p;
      const unsigned idx = IDX (lit);
      kissat_dequeue_links (idx, links, queue);
    }
    queue->stamp = 0;
    unsigned rest = queue->first;
    p = end;
    while (p != begin) {
      const unsigned lit = *--p;
      const unsigned idx = IDX (lit);
      struct links *l = links + idx;
      if (DISCONNECTED (queue->first)) {
        KISSAT_assert (DISCONNECTED (queue->last));
        queue->last = idx;
      } else {
        struct links *first = links + queue->first;
        KISSAT_assert (DISCONNECTED (first->prev));
        first->prev = idx;
      }
      l->next = queue->first;
      queue->first = idx;
      KISSAT_assert (DISCONNECTED (l->prev));
      l->stamp = ++queue->stamp;
    }
    while (!DISCONNECTED (rest)) {
      struct links *l = links + rest;
      l->stamp = ++queue->stamp;
      rest = l->next;
    }
    solver->queue.search.idx = queue->last;
    solver->queue.search.stamp = queue->stamp;
  }
}

static bool run_factorization (kissat *solver, uint64_t limit) {
  factoring factoring;
  init_factoring (solver, &factoring, limit);
  schedule_factorization (&factoring);
  bool done = false;
#ifndef KISSAT_QUIET
  unsigned factored = 0;
#endif
  uint64_t *ticks = &solver->statistics_.factor_ticks;
  kissat_extremely_verbose (
      solver, "factorization limit of %" PRIu64 " ticks", limit - *ticks);
  while (!done && !kissat_empty_heap (&factoring.schedule)) {
    const unsigned first =
        kissat_pop_max_heap (solver, &factoring.schedule);
    const unsigned first_idx = IDX (first);
    if (!ACTIVE (first_idx))
      continue;
    if (*ticks > limit) {
      kissat_very_verbose (solver, "factorization ticks limit hit");
      break;
    }
    if (TERMINATED (factor_terminated_1))
      break;
    struct flags *f = solver->flags + first_idx;
    const unsigned bit = 1u << NEGATED (first);
    if (!(f->factor & bit))
      continue;
    f->factor &= ~bit;
    const size_t first_count = first_factor (&factoring, first);
    if (first_count > 1) {
      for (;;) {
        unsigned next_count;
        const unsigned next = next_factor (&factoring, &next_count);
        if (next == INVALID_LIT)
          break;
        KISSAT_assert (next_count > 1);
        if (next_count < 2)
          break;
        factorize_next (&factoring, next, next_count);
      }
      size_t reduction;
      quotient *q = best_quotient (&factoring, &reduction);
      if (q && reduction > factoring.bound) {
        if (apply_factoring (&factoring, q)) {
#ifndef KISSAT_QUIET
          factored++;
#endif
        } else
          done = true;
      }
    }
    release_quotients (&factoring);
  }
  bool completed = kissat_empty_heap (&factoring.schedule);
  adjust_scores_and_phases_of_fresh_varaibles (&factoring);
  release_factoring (&factoring);
  REPORT (!factored, 'f');
  return completed;
}

static void connect_clauses_to_factor (kissat *solver) {
  const unsigned size_limit = GET_OPTION (factorsize);
  if (size_limit < 3) {
    kissat_extremely_verbose (solver, "only factorizing binary clauses");
    return;
  }
  kissat_very_verbose (solver, "factorizing clauses of maximum size %u",
                       size_limit);
  clause *last_irredundant = kissat_last_irredundant_clause (solver);
  ward *const arena = BEGIN_STACK (solver->arena);
  watches *all_watches = solver->watches;
  unsigned *bincount, *largecount;
  CALLOC (unsigned, bincount, LITS);
  for (all_literals (lit)) {
    if (!ACTIVE (IDX (lit)))
      continue;
    for (all_binary_large_watches (watch, WATCHES (lit))) {
      KISSAT_assert (watch.type.binary);
      const unsigned other = watch.type.lit;
      if (lit > other)
        continue;
      bincount[lit]++;
      bincount[other]++;
    }
  }
  CALLOC (unsigned, largecount, LITS);
  size_t initial_candidates = 0;
  for (all_clauses (c)) {
    if (c->garbage)
      continue;
    if (last_irredundant && last_irredundant < c)
      break;
    if (c->redundant)
      continue;
    if (c->size > size_limit)
      continue;
    for (all_literals_in_clause (lit, c))
      largecount[lit]++;
    initial_candidates++;
  }
  kissat_very_verbose (solver,
                       "initially found %zu large clause candidates",
                       initial_candidates);
  size_t candidates = initial_candidates;
  const unsigned rounds = GET_OPTION (factorcandrounds);
  for (unsigned round = 1; round <= rounds; round++) {
    size_t new_candidates = 0;
    unsigned *newlargecount;
    CALLOC (unsigned, newlargecount, LITS);
    for (all_clauses (c)) {
      if (c->garbage)
        continue;
      if (last_irredundant && last_irredundant < c)
        break;
      if (c->redundant)
        continue;
      if (c->size > size_limit)
        continue;
      for (all_literals_in_clause (lit, c))
        if (bincount[lit] + largecount[lit] < 2)
          goto CONTINUE_WITH_NEXT_CLAUSE1;
      for (all_literals_in_clause (lit, c))
        newlargecount[lit]++;
      new_candidates++;
    CONTINUE_WITH_NEXT_CLAUSE1:;
    }
    DEALLOC (largecount, LITS);
    largecount = newlargecount;
    if (candidates == new_candidates) {
      kissat_very_verbose (solver,
                           "no large factorization candidate clauses "
                           "reduction in round %u",
                           round);
      break;
    }
    candidates = new_candidates;
    kissat_very_verbose (
        solver,
        "reduced to %zu large factorization candidate clauses %.0f%% in "
        "round %u",
        candidates, kissat_percent (candidates, initial_candidates), round);
  }
#ifndef KISSAT_QUIET
  size_t connected = 0;
#endif
  for (all_clauses (c)) {
    if (c->garbage)
      continue;
    if (last_irredundant && last_irredundant < c)
      break;
    if (c->redundant)
      continue;
    if (c->size > size_limit)
      continue;
    int continue_with_next_clause2 = 0;
    for (all_literals_in_clause (lit, c))
      if (bincount[lit] + largecount[lit] < 2) {
        continue_with_next_clause2 = 1;
        break;
      }
    if(continue_with_next_clause2) {
      continue;
    }
    const reference ref = (ward *) c - arena;
    kissat_inlined_connect_clause (solver, all_watches, c, ref);
#ifndef KISSAT_QUIET
    connected++;
#endif
  }
  DEALLOC (largecount, LITS);
  DEALLOC (bincount, LITS);
  kissat_very_verbose (
      solver, "connected %zu large factorization candidate clauses %.0f%%",
      connected, kissat_percent (candidates, initial_candidates));
}

void kissat_factor (kissat *solver) {
  KISSAT_assert (!solver->level);
  if (solver->inconsistent)
    return;
  if (!GET_OPTION (factor))
    return;
  statistics *s = &solver->statistics_;
  if (solver->limits.factor.marked >= s->literals_factor) {
    kissat_extremely_verbose (
        solver,
        "factorization skipped as no literals have been marked to be added "
        "(%" PRIu64 " < %" PRIu64,
        solver->limits.factor.marked, s->literals_factor);
    return;
  }
  START (factor);
  INC (factorizations);
  kissat_phase (solver, "factorization", GET (factorizations),
                "binary clause bounded variable addition");
  uint64_t limit = GET_OPTION (factoriniticks);
  if (s->factorizations > 1) {
    SET_EFFORT_LIMIT (tmp, factor, factor_ticks);
    limit = tmp;
  } else {
    kissat_very_verbose (solver,
                         "initially limiting to %" PRIu64
                         " million factorization ticks",
                         limit);
    limit *= 1e6;
    limit += s->factor_ticks;
  }
#ifndef KISSAT_QUIET
  struct {
    int64_t variables, binary, clauses, ticks;
  } before, after, delta;
  before.variables = s->variables_extension + s->variables_original;
  before.binary = BINARY_CLAUSES;
  before.clauses = IRREDUNDANT_CLAUSES;
  before.ticks = s->factor_ticks;
#endif
  kissat_enter_dense_mode (solver, 0);
  connect_clauses_to_factor (solver);
  bool completed = run_factorization (solver, limit);
  kissat_resume_sparse_mode (solver, false, 0);
#ifndef KISSAT_QUIET
  after.variables = s->variables_extension + s->variables_original;
  after.binary = BINARY_CLAUSES;
  after.clauses = IRREDUNDANT_CLAUSES;
  after.ticks = s->factor_ticks;
  delta.variables = after.variables - before.variables;
  delta.binary = before.binary - after.binary;
  delta.clauses = before.clauses - after.clauses;
  delta.ticks = after.ticks - before.ticks;
  kissat_very_verbose (solver, "used %f million factorization ticks",
                       delta.ticks * 1e-6);
  kissat_phase (solver, "factorization", GET (factorizations),
                "introduced %" PRId64 " extension variables %.0f%%",
                delta.variables,
                kissat_percent (delta.variables, before.variables));
  kissat_phase (solver, "factorization", GET (factorizations),
                "removed %" PRId64 " binary clauses %.0f%%", delta.binary,
                kissat_percent (delta.binary, before.binary));
  kissat_phase (solver, "factorization", GET (factorizations),
                "removed %" PRId64 " large clauses %.0f%%", delta.clauses,
                kissat_percent (delta.clauses, before.clauses));
#endif
  if (completed)
    solver->limits.factor.marked = s->literals_factor;
  STOP (factor);
}

ABC_NAMESPACE_IMPL_END
