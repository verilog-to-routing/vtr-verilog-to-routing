#include "bump.h"
#include "analyze.h"
#include "inlineheap.h"
#include "inlinequeue.h"
#include "inlinevector.h"
#include "internal.h"
#include "logging.h"
#include "print.h"
#include "rank.h"
#include "sort.h"

ABC_NAMESPACE_IMPL_START

#define RANK(A) ((A).rank)
#define SMALLER(A, B) (RANK (A) < RANK (B))

#define RADIX_SORT_BUMP_LIMIT 32

static void sort_bump (kissat *solver) {
  const size_t size = SIZE_STACK (solver->analyzed);
  if (size < RADIX_SORT_BUMP_LIMIT) {
    LOG ("quick sorting %zu analyzed variables", size);
    SORT_STACK (datarank, solver->ranks, SMALLER);
  } else {
    LOG ("radix sorting %zu analyzed variables", size);
    RADIX_STACK (datarank, unsigned, solver->ranks, RANK);
  }
}

void kissat_rescale_scores (kissat *solver) {
  INC (rescaled);
  heap *scores = &solver->scores;
  const double max_score = kissat_max_score_on_heap (scores);
  kissat_phase (solver, "rescale", GET (rescaled),
                "maximum score %g increment %g", max_score, solver->scinc);
  const double rescale = MAX (max_score, solver->scinc);
  KISSAT_assert (rescale > 0);
  const double factor = 1.0 / rescale;
  kissat_rescale_heap (solver, scores, factor);
  solver->scinc *= factor;
  kissat_phase (solver, "rescale", GET (rescaled), "rescaled by factor %g",
                factor);
}

void kissat_bump_score_increment (kissat *solver) {
  const double old_scinc = solver->scinc;
  const double decay = GET_OPTION (decay) * 1e-3;
  KISSAT_assert (0 <= decay), KISSAT_assert (decay <= 0.5);
  const double factor = 1.0 / (1.0 - decay);
  const double new_scinc = old_scinc * factor;
  LOG ("new score increment %g = %g * %g", new_scinc, factor, old_scinc);
  solver->scinc = new_scinc;
  if (new_scinc > MAX_SCORE)
    kissat_rescale_scores (solver);
}

static inline void bump_analyzed_variable_score (kissat *solver,
                                                 unsigned idx) {
  heap *scores = &solver->scores;
  const double old_score = kissat_get_heap_score (scores, idx);
  const double inc = solver->scinc;
  const double new_score = old_score + inc;
  LOG ("new score[%u] = %g = %g + %g", idx, new_score, old_score, inc);
  kissat_update_heap (solver, scores, idx, new_score);
  if (new_score > MAX_SCORE)
    kissat_rescale_scores (solver);
}

void kissat_bump_variable (kissat *solver, unsigned idx) {
  bump_analyzed_variable_score (solver, idx);
}

static void bump_analyzed_variable_scores (kissat *solver) {
  flags *flags = solver->flags;

  for (all_stack (unsigned, idx, solver->analyzed))
    if (flags[idx].active)
      bump_analyzed_variable_score (solver, idx);

  kissat_bump_score_increment (solver);
}

static void move_analyzed_variables_to_front_of_queue (kissat *solver) {
  KISSAT_assert (EMPTY_STACK (solver->ranks));
  const links *const links = solver->links;
  for (all_stack (unsigned, idx, solver->analyzed)) {
    // clang-format off
    const datarank rank = { .data = idx, .rank = links[idx].stamp };
    // clang-format on
    PUSH_STACK (solver->ranks, rank);
  }

  sort_bump (solver);

  flags *flags = solver->flags;
  unsigned idx;

  for (all_stack (datarank, rank, solver->ranks))
    if (flags[idx = rank.data].active)
      kissat_move_to_front (solver, idx);

  CLEAR_STACK (solver->ranks);
}

void kissat_bump_analyzed (kissat *solver) {
  START (bump);
  const size_t bumped = SIZE_STACK (solver->analyzed);
  if (!solver->stable)
    move_analyzed_variables_to_front_of_queue (solver);
  else
    bump_analyzed_variable_scores (solver);
  ADD (literals_bumped, bumped);
  STOP (bump);
}

void kissat_update_scores (kissat *solver) {
  KISSAT_assert (solver->stable);
  heap *scores = SCORES;
  for (all_variables (idx))
    if (ACTIVE (idx) && !kissat_heap_contains (scores, idx))
      kissat_push_heap (solver, scores, idx);
}

ABC_NAMESPACE_IMPL_END
