#include "global.h"

#if !defined(KISSAT_QUIET) || !defined(KISSAT_NDEBUG)
#include "internal.h"
#endif

#ifndef KISSAT_QUIET

#include "resources.h"
#include "tiers.h"
#include "utilities.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

ABC_NAMESPACE_IMPL_START

void kissat_print_glue_usage (kissat *solver) {
  const int64_t stable = solver->statistics.clauses_used_stable;
  const int64_t focused = solver->statistics.clauses_used_focused;
  if (!stable && !focused)
    printf ("%sno clauses used at all\n", solver->prefix);
  else {
    if (focused)
      kissat_print_tier_usage_statistics (solver, false);
    if (focused && stable)
      printf ("c\n");
    if (stable)
      kissat_print_tier_usage_statistics (solver, true);
  }
  fflush (stdout);
}

// clang-format off

void
kissat_statistics_print (kissat * solver, bool verbose)
{
  statistics *statistics = &solver->statistics;

  const double time = kissat_process_time ();
  size_t variables = solver->statistics.variables_original;
#ifdef METRICS
  const double rss = kissat_maximum_resident_set_size ();
#endif

/*------------------------------------------------------------------------*/

#define RELATIVE(FIRST,SECOND) \
  kissat_average (statistics->FIRST, statistics->SECOND)

/*------------------------------------------------------------------------*/

#define CONF_INT(NAME) \
  RELATIVE (conflicts, NAME)

#define SWEEPS_PER_COMPLETED(NAME) \
  RELATIVE (sweep, NAME)

#define NO_SECONDARY(NAME) \
  0

/*------------------------------------------------------------------------*/

#define PER_BACKBONE(NAME) \
  RELATIVE (NAME, backbone_computations)

#define PER_BACKBONE_UNIT(NAME) \
  RELATIVE (NAME, backbone_units)

#define PER_CLOSURE(NAME) \
  RELATIVE(NAME, closures)

#define PER_CLS_ADDED(NAME) \
  RELATIVE (NAME, clauses_added)

#define PER_CLS_FACTORED(NAME) \
  RELATIVE (NAME, clauses_factored)

#define PER_CLS_LEARNED(NAME) \
  RELATIVE (NAME, clauses_learned)

#define PER_CLS_UNFACTORED(NAME) \
  RELATIVE (NAME, clauses_unfactored)

#define PER_CONFLICT(NAME) \
  RELATIVE (NAME, conflicts)

#define PER_CONGRANDS(NAME) \
  RELATIVE (NAME, congruent_gates_ands)

#define PER_CONGRGATES(NAME) \
  RELATIVE (NAME, congruent_gates)

#define PER_CONGRITES(NAME) \
  RELATIVE (NAME, congruent_gates_ites)

#define PER_CONGRLOOKUP(NAME) \
  RELATIVE (NAME, congruent_lookups)

#define PER_CONGRXORS(NAME) \
  RELATIVE (NAME, congruent_gates_xors)

#define PER_FIXED(NAME) \
  RELATIVE (NAME, units)

#ifndef STATISTICS
#define PER_FLIPPED(NAME) \
  -1
#else
#define PER_FLIPPED(NAME) \
  RELATIVE (NAME, flipped)
#endif

#define PER_FORWARD_CHECK(NAME) \
  RELATIVE (NAME, forward_checks)

#define PER_KITTEN_PROP(NAME) \
  RELATIVE (NAME, kitten_propagations)

#define PER_KITTEN_SOLVED(NAME) \
  RELATIVE (NAME, kitten_solved)

#define PER_PROPAGATION(NAME) \
  RELATIVE (NAME, propagations)

#define PER_RESTART(NAME) \
  RELATIVE (NAME, restarts)

#define PER_SECOND(NAME) \
  kissat_average (statistics->NAME, time)

#define PER_SWEEP_VARIABLES(NAME) \
  kissat_average (statistics->NAME, statistics->sweep_variables)

#define PER_VARIABLE(NAME) \
  kissat_average (statistics->NAME, variables)

#define PER_VIVIFICATION(NAME) \
  RELATIVE (NAME, vivifications)

#define PER_VIVIFY_CHECK(NAME) \
  RELATIVE (NAME, vivify_checks)

#if 0 //ndef STATISTICS
#define PER_WALKS(NAME) \
  -1
#else
#define PER_WALKS(NAME) \
  RELATIVE (NAME, walks)
#endif

/*------------------------------------------------------------------------*/

#define PERCENT(FIRST,SECOND) \
  kissat_percent (statistics->FIRST, statistics->SECOND)

/*------------------------------------------------------------------------*/

#define PCNT_ARENA_RESIZED(NAME) \
  PERCENT (NAME, arena_resized)

#define PCNT_CLS_ADDED(NAME) \
  PERCENT (NAME, clauses_added)

#define PCNT_CLS_IMPROVED(NAME) \
  PERCENT (NAME, clauses_improved)

#define PCNT_CLS_IMPROVED(NAME) \
  PERCENT (NAME, clauses_improved)

#define PCNT_CLS_FACTORED(NAME) \
  PERCENT (NAME, clauses_factored)

#define PCNT_CLS_LEARNED(NAME) \
  PERCENT (NAME, clauses_learned)

#define PCNT_CLS_ORIGINAL(NAME) \
  PERCENT (NAME, clauses_original)

#define PCNT_CLS_REDUCED(NAME) \
  PERCENT (NAME, clauses_reduced)

#define PCNT_CLS_USED(NAME) \
  PERCENT (NAME, clauses_used)

#define PCNT_COLLECTIONS(NAME) \
  PERCENT (NAME, garbage_collections)

#define PCNT_CONFLICTS(NAME) \
  PERCENT (NAME, conflicts)

#define PCNT_CONGRCOLS(NAME) \
  PERCENT (NAME, congruent_collisions)

#define PCNT_CONGRGATES(NAME) \
  PERCENT (NAME, congruent_gates)

#define PCNT_CONGRLOOKUP(NAME) \
  PERCENT (NAME, congruent_lookups)

#define PCNT_CONGRMATCHED(NAME) \
  PERCENT (NAME, congruent_matched)

#define PCNT_CONGREWR(NAME) \
  PERCENT (NAME, congruent_rewritten)

#define PCNT_CONGRSIMPS(NAME) \
  PERCENT (NAME, congruent_simplified)

#define PCNT_CONGRUENT(NAME) \
  PERCENT (NAME, congruent)

#define PCNT_CONGRUNARY(NAME) \
  PERCENT (NAME, congruent_unary)

#define PCNT_DECISIONS(NAME) \
  PERCENT (NAME, decisions)

#define PCNT_DEFRAGS(NAME) \
  PERCENT (NAME, defragmentations)

#define PCNT_ELIM_ATTEMPTS(NAME) \
  PERCENT (NAME, eliminate_attempted)

#define PCNT_ELIMINATED(NAME) \
  PERCENT (NAME, eliminated)

#define PCNT_EXTRACTED(NAME) \
  PERCENT (NAME, gates_extracted)

#define PCNT_IRREDUNDANT(NAME) \
  PERCENT (NAME, clauses_irredundant)

#define PCNT_KITTEN_FLIP(NAME) \
  PERCENT (NAME, kitten_flip)

#define PCNT_KITTEN_SOLVED(NAME) \
  PERCENT (NAME, kitten_solved)

#define PCNT_LITS_DEDUCED(NAME) \
  PERCENT (NAME, literals_deduced)

#define PCNT_LITS_SHRUNKEN(NAME) \
  PERCENT (NAME, literals_shrunken)

#define PCNT_PROPS(NAME) \
  PERCENT (NAME, propagations)

#define PCNT_REDUCTIONS(NAME) \
  PERCENT (NAME, reductions)

#define PCNT_REDUNDANT_CLAUSES(NAME) \
  PERCENT (NAME, clauses_redundant)

#define PCNT_REORDERED(NAME) \
  PERCENT (NAME, reordered)

#define PCNT_REPHASED(NAME) \
  PERCENT (NAME, rephased)

#define PCNT_RESIDENT_SET(NAME) \
  kissat_percent (statistics->NAME, rss)

#define PCNT_RESTARTS(NAME) \
  PERCENT (NAME, restarts)

#define PCNT_RESTARTS_LEVELS(NAME) \
  PERCENT (NAME, restarts_levels)

#define PCNT_SEARCHES(NAME) \
  PERCENT (NAME, searches)

#define PCNT_STRENGTHENED(NAME) \
  PERCENT (NAME, strengthened)

#define PCNT_SUBSUMED(NAME) \
  PERCENT (NAME, subsumed)

#define PCNT_SUBSUMPTION_CHECK(NAME) \
  PERCENT (NAME, subsumption_checks)

#define PCNT_SWEEP_FLIP_BACKBONE(NAME) \
  PERCENT (NAME, sweep_flip_backbone)

#define PCNT_SWEEP_FLIP_EQUIVALENCES(NAME) \
  PERCENT (NAME, sweep_flip_equivalences)

#define PCNT_SWEEP_SOLVED_BACKBONE(NAME) \
  PERCENT (NAME, sweep_solved_backbone)

#define PCNT_SWEEP_SOLVED_EQUIVALENCES(NAME) \
  PERCENT (NAME, sweep_solved_equivalences)

#define PCNT_SWEEP_SOLVED(NAME) \
  PERCENT (NAME, sweep_solved)

#ifndef STATISTICS
#define PCNT_TICKS(NAME) \
  -1
#else
#define PCNT_TICKS(NAME) \
  PERCENT (NAME, ticks)
#endif

#define PCNT_VARIABLES(NAME) \
  kissat_percent (statistics->NAME, variables)

#define PCNT_VIVIFIED(NAME) \
  PERCENT (NAME, vivified)

#define PCNT_VIVIFY_IMP(NAME) \
  PERCENT (NAME, vivified_implied)

#define PCNT_VIVIFY_INST(NAME) \
  PERCENT (NAME, vivified_instantiated)

#define PCNT_VIVIFY_CHECK(NAME) \
  PERCENT (NAME, vivify_checks)

#define PCNT_VIVIFY_PROBES(NAME) \
  PERCENT (NAME, vivify_probes)

#define PCNT_VIVIFY_SHRUNKEN(NAME) \
  PERCENT (NAME, vivified_shrunken)

#define PCNT_VIVIFY_SUB(NAME) \
  PERCENT (NAME, vivified_subsumed)

#define PCNT_WALKS(NAME) \
  PERCENT (NAME, walks)

#define COUNTER(NAME,VERBOSE,OTHER,UNITS,TYPE) \
  if (verbose || !VERBOSE || (VERBOSE == 1 && statistics->NAME)) \
    PRINT_STAT (#NAME, statistics->NAME, OTHER(NAME), UNITS, TYPE);
#define IGNORE(...)

  METRICS_COUNTERS_AND_STATISTICS

#undef COUNTER
#undef METRIC
#undef STATISTIC

  fflush (stdout);
}

// clang-format on

ABC_NAMESPACE_IMPL_END

#elif defined(KISSAT_NDEBUG)

ABC_NAMESPACE_IMPL_START

int kissat_statistics_dummy_to_avoid_warning;

ABC_NAMESPACE_IMPL_END

#endif

/*------------------------------------------------------------------------*/

#ifndef KISSAT_NDEBUG

#include "inlinevector.h"

ABC_NAMESPACE_IMPL_START

void kissat_check_statistics (kissat *solver) {
  if (solver->inconsistent)
    return;

  size_t redundant = 0;
  size_t irredundant = 0;
  size_t arena_garbage = 0;

  for (all_clauses (c)) {
    if (c->garbage) {
      arena_garbage += kissat_actual_bytes_of_clause (c);
      continue;
    }
    if (c->redundant)
      redundant++;
    else
      irredundant++;
  }

  size_t binary = 0;

  if (solver->watching) {
    for (all_literals (lit)) {
      watches *watches = &WATCHES (lit);

      for (all_binary_blocking_watches (watch, *watches)) {
        if (watch.type.binary) {
          binary++;
        }
      }
    }
  } else {
    for (all_literals (lit)) {
      watches *watches = &WATCHES (lit);

      for (all_binary_large_watches (watch, *watches)) {
        if (watch.type.binary) {
          binary++;
        }
      }
    }
  }

  KISSAT_assert (!(binary & 1));
  binary /= 2;

  statistics *statistics = &solver->statistics_;
  KISSAT_assert (statistics->clauses_binary == binary);
  KISSAT_assert (statistics->clauses_redundant == redundant);
  KISSAT_assert (statistics->clauses_irredundant == irredundant);
#ifdef METRICS
  KISSAT_assert (statistics->arena_garbage == arena_garbage);
#else
  (void) arena_garbage;
#endif
}

ABC_NAMESPACE_IMPL_END

#endif
