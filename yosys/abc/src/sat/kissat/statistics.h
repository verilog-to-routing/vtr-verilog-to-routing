#ifndef _statistics_h_INCLUDED
#define _statistics_h_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

// clang-format off

#define METRICS_COUNTERS_AND_STATISTICS \
\
  METRIC (allocated_collected, 2, PCNT_RESIDENT_SET, "%", "resident set") \
  METRIC (allocated_current, 2, PCNT_RESIDENT_SET, "%", "resident set") \
  METRIC (allocated_max, 2, PCNT_RESIDENT_SET, "%", "resident set") \
  STATISTIC (ands_eliminated, 1, PCNT_ELIMINATED, "%", "eliminated") \
  METRIC (ands_extracted, 1, PCNT_EXTRACTED, "%", "extracted") \
  METRIC (arena_enlarged, 1, PCNT_ARENA_RESIZED, "%", "resize") \
  METRIC (arena_garbage, 1, PCNT_RESIDENT_SET, "%", "resident set") \
  METRIC (arena_resized, 1, CONF_INT, "", "interval") \
  METRIC (arena_shrunken, 1, PCNT_ARENA_RESIZED, "%", "resize") \
  COUNTER (backbone_computations, 2, CONF_INT, "", "interval") \
  METRIC (backbone_implied, 1, PER_BACKBONE_UNIT, 0, "per unit") \
  METRIC (backbone_probes, 2, PER_VARIABLE, "", "per variable") \
  METRIC (backbone_propagations, 2, PCNT_PROPS, "%", "propagations") \
  METRIC (backbone_rounds, 2, PER_BACKBONE, 0, "per backbone") \
  COUNTER (backbone_ticks, 2, PCNT_TICKS, "%", "ticks") \
  STATISTIC (backbone_units, 1, PCNT_VARIABLES, "%", "variables") \
  METRIC (best_saved, 1, CONF_INT, "", "interval") \
  COUNTER (chronological, 1, PCNT_CONFLICTS, "%", "conflicts") \
  COUNTER (clauses_added, 2, PCNT_CLS_ADDED, "%", "added") \
  COUNTER (clauses_binary, 2, PCNT_CLS_ADDED, "%", "added") \
  STATISTIC (clauses_deleted, 1, PCNT_CLS_ADDED, "%", "added") \
  STATISTIC (clauses_factored, 1, PCNT_CLS_ADDED, "%", "added") \
  STATISTIC (clauses_improved, 1, PCNT_CLS_LEARNED, "%", "learned") \
  COUNTER (clauses_irredundant, 2, PCNT_CLS_ADDED, "%", "added") \
  STATISTIC (clauses_kept1, 1, PCNT_CLS_IMPROVED, "%", "improved") \
  STATISTIC (clauses_kept2, 1, PCNT_CLS_IMPROVED, "%", "improved") \
  STATISTIC (clauses_kept3, 1, PCNT_CLS_IMPROVED, "%", "improved") \
  COUNTER (clauses_learned, 2, PCNT_CONFLICTS, "%", "conflicts") \
  COUNTER (clauses_original, 2, PCNT_CLS_ADDED, "%", "added") \
  STATISTIC (clauses_promoted1, 1, PCNT_CLS_IMPROVED, "%", "improved") \
  STATISTIC (clauses_promoted2, 1, PCNT_CLS_IMPROVED, "%", "improved") \
  STATISTIC (clauses_reduced, 1, PCNT_CLS_LEARNED, "%", "learned") \
  STATISTIC (clauses_reduced_tier1, 1, PCNT_CLS_REDUCED, "%", "reduced") \
  STATISTIC (clauses_reduced_tier2, 1, PCNT_CLS_REDUCED, "%", "reduced") \
  STATISTIC (clauses_reduced_tier3, 1, PCNT_CLS_REDUCED, "%", "reduced") \
  COUNTER (clauses_redundant, 2, NO_SECONDARY, 0, 0) \
  STATISTIC (clauses_unfactored, 1, PCNT_CLS_FACTORED, "%", "factored") \
  COUNTER (clauses_used, 2, PCNT_CLS_LEARNED, "%", "learned") \
  COUNTER (clauses_used_focused, 2, PCNT_CLS_USED, "%", "used") \
  COUNTER (clauses_used_stable, 2, PCNT_CLS_USED, "%", "used") \
  COUNTER (closures, 2, CONF_INT, "", "interval") \
  METRIC (compacted, 1, PCNT_REDUCTIONS, "%", "reductions") \
  COUNTER (conflicts, 0, PER_SECOND, 0, "per second") \
  COUNTER (congruent, 1, PCNT_VARIABLES, "%", "variables") \
  STATISTIC (congruent_ands, 1, PCNT_CONGRUENT, "%", "congruent") \
  STATISTIC (congruent_arity, 1, PER_CONGRGATES, 0, "per gate") \
  STATISTIC (congruent_arity_ands, 1, PER_CONGRANDS, 0, "per AND") \
  STATISTIC (congruent_arity_xors, 1, PER_CONGRXORS, 0, "per XOR") \
  STATISTIC (congruent_binaries, 1, PCNT_CLS_ADDED, "%", "clauses added") \
  STATISTIC (congruent_ites, 1, PCNT_CONGRUENT, "%", "congruent") \
  STATISTIC (congruent_collisions, 1, PER_CONGRLOOKUP, 0, "per lookup") \
  STATISTIC (congruent_collisions_find, 1, PCNT_CONGRCOLS, "%", "collisions") \
  STATISTIC (congruent_collisions_index, 1, PCNT_CONGRCOLS, "%", "collisions") \
  STATISTIC (congruent_collisions_removed, 1, PCNT_CONGRCOLS, "%", "collisions") \
  STATISTIC (congruent_equivalences, 1, PCNT_CONGRUENT, "%", "congruent") \
  COUNTER (congruent_gates, 2, PER_CLOSURE, 0, "per closure") \
  COUNTER (congruent_gates_ands, 2, PCNT_CONGRGATES, "%", "gates") \
  COUNTER (congruent_gates_ites, 2, PCNT_CONGRGATES, "%", "gates") \
  COUNTER (congruent_gates_xors, 2, PCNT_CONGRGATES, "%", "gates") \
  STATISTIC (congruent_indexed, 1, PER_CONGRGATES, 0, "per gate") \
  STATISTIC (congruent_lookups, 1, PER_CONGRGATES, 0, "per gate") \
  STATISTIC (congruent_lookups_find, 1, PCNT_CONGRLOOKUP, "%", "lookups") \
  STATISTIC (congruent_lookups_removed, 1, PCNT_CONGRLOOKUP, "%", "lookups") \
  COUNTER (congruent_matched, 2, PCNT_CONGRUENT, "%", "congruent") \
  COUNTER (congruent_matched_ands, 2, PCNT_CONGRMATCHED, "%", "matched") \
  COUNTER (congruent_matched_ites, 2, PCNT_CONGRMATCHED, "%", "matched") \
  COUNTER (congruent_matched_xors, 2, PCNT_CONGRMATCHED, "%", "matched") \
  STATISTIC (congruent_rewritten, 1, PCNT_CONGRGATES, "%", "gates") \
  STATISTIC (congruent_rewritten_ands, 1, PCNT_CONGREWR, "%", "rewritten") \
  STATISTIC (congruent_rewritten_ites, 1, PCNT_CONGREWR, "%", "rewritten") \
  STATISTIC (congruent_rewritten_xors, 1, PCNT_CONGREWR, "%", "rewritten") \
  STATISTIC (congruent_simplified, 1, PCNT_CONGRGATES, "%", "gates") \
  STATISTIC (congruent_simplified_ands, 1, PCNT_CONGRSIMPS, "%", "simplified") \
  STATISTIC (congruent_simplified_ites, 1, PCNT_CONGRSIMPS, "%", "simplified") \
  STATISTIC (congruent_simplified_xors, 1, PCNT_CONGRSIMPS, "%", "simplified") \
  STATISTIC (congruent_subsumed, 1, PCNT_CLS_ORIGINAL, "%", "original") \
  STATISTIC (congruent_trivial_ite, 1, PCNT_CONGRUENT, "%", "congruent") \
  STATISTIC (congruent_unary, 1, PCNT_CONGRUENT, "%", "congruent") \
  STATISTIC (congruent_unary_ands, 1, PCNT_CONGRUNARY, "%", "unary") \
  STATISTIC (congruent_unary_ites, 1, PCNT_CONGRUNARY, "%", "unary") \
  STATISTIC (congruent_unary_xors, 1, PCNT_CONGRUNARY, "%", "unary") \
  STATISTIC (congruent_units, 1, PCNT_VARIABLES, "%", "variables") \
  STATISTIC (congruent_xors, 1, PCNT_CONGRUENT, "%", "congruent") \
  COUNTER (decisions, 0, PER_CONFLICT, 0, "per conflict") \
  METRIC (definitions_checked, 1, PCNT_ELIM_ATTEMPTS, "%", "attempts") \
  STATISTIC (definitions_eliminated, 1, PCNT_ELIMINATED, "%", "eliminated") \
  METRIC (definitions_extracted, 1, PCNT_EXTRACTED, "%", "extracted") \
  STATISTIC (definition_units, 1, PCNT_VARIABLES, "%", "variables") \
  METRIC (defragmentations, 1, CONF_INT, "", "interval") \
  METRIC (dense_garbage_collections, 2, PCNT_COLLECTIONS, "%", "collections") \
  METRIC (dense_propagations, 1, PCNT_PROPS, "%", "propagations") \
  METRIC (dense_ticks, 1, PCNT_TICKS, "%", "ticks") \
  METRIC (duplicated, 1, PCNT_CLS_ADDED, "%", "added") \
  STATISTIC (eagerly_subsumed, 1, PCNT_CLS_LEARNED, "%", "learned") \
  STATISTIC (eliminate_attempted, 1, PER_VARIABLE, 0, "per variable") \
  COUNTER (eliminated, 1, PCNT_VARIABLES, "%", "variables") \
  COUNTER (eliminate_resolutions, 2, PER_SECOND, 0, "per second") \
  STATISTIC (eliminate_units, 1, PCNT_VARIABLES, "%", "variables") \
  COUNTER (eliminations, 2, CONF_INT, "", "interval") \
  STATISTIC (equivalences_eliminated, 1, PCNT_ELIMINATED, "%", "eliminated") \
  METRIC (equivalences_extracted, 1, PCNT_EXTRACTED, "%", "extracted") \
  METRIC (extensions, 1, PCNT_SEARCHES, "%", "searches") \
  COUNTER (factored, 1, PCNT_VARIABLES, "%", "variables") \
  COUNTER (factorizations, 2, CONF_INT, "", "interval") \
  COUNTER (factor_ticks, 2, PCNT_TICKS, "%", "ticks") \
  COUNTER (fast_eliminated, 1, PCNT_ELIMINATED, "%", "eliminated") \
  COUNTER (fast_strengthened, 1, PCNT_STRENGTHENED, "%", "per strengthened") \
  COUNTER (fast_subsumed, 1, PCNT_SUBSUMED, "%", "per subsumed") \
  STATISTIC (fresh, 1, PCNT_VARIABLES, "%", "variables") \
  STATISTIC (flipped, 1, PER_WALKS, 0, "per walk") \
  METRIC (flushed, 2, PER_FIXED, 0, "per fixed") \
  METRIC (focused_decisions, 1, PCNT_DECISIONS, "%", "decisions") \
  METRIC (focused_modes, 1, CONF_INT, "", "interval") \
  METRIC (focused_propagations, 1, PCNT_PROPS, "%", "propagations") \
  METRIC (focused_restarts, 1, PCNT_RESTARTS, "%", "restarts") \
  METRIC (focused_ticks, 1, PCNT_TICKS, "%", "ticks") \
  COUNTER (forward_checks, 2, NO_SECONDARY, 0, 0) \
  COUNTER (forward_steps, 2, PER_FORWARD_CHECK, 0, "per check") \
  STATISTIC (forward_strengthened, 1, PCNT_STRENGTHENED, "%", "per strengthened") \
  STATISTIC (forward_subsumed, 1, PCNT_SUBSUMED, "%", "per subsumed") \
  METRIC (forward_subsumptions, 1, CONF_INT, "", "interval") \
  METRIC (garbage_collections, 2, CONF_INT, "", "interval") \
  METRIC (gates_checked, 1, PCNT_ELIM_ATTEMPTS, "%", "attempts") \
  STATISTIC (gates_eliminated, 1, PCNT_ELIMINATED, "%", "eliminated") \
  METRIC (gates_extracted, 1, PCNT_ELIM_ATTEMPTS, "%", "attempts") \
  STATISTIC (if_then_else_eliminated, 1, PCNT_ELIMINATED, "%", "eliminated") \
  METRIC (if_then_else_extracted, 1, PCNT_EXTRACTED, "%", "extracted") \
  METRIC (initial_decisions, 1, PCNT_DECISIONS, "%", "decisions") \
  COUNTER (iterations, 1, PCNT_VARIABLES, "%", "variables") \
  STATISTIC (jumped_reasons, 1, PCNT_PROPS, "%", "propagations") \
  STATISTIC (kitten_conflicts, 1, PER_KITTEN_SOLVED, 0, "per solved") \
  STATISTIC (kitten_decisions, 1, PER_KITTEN_SOLVED, 0, "per solved") \
  STATISTIC (kitten_flip, 1, NO_SECONDARY, 0, 0) \
  STATISTIC (kitten_flipped, 1, PCNT_KITTEN_FLIP, "%", "flip") \
  COUNTER (kitten_propagations, 2, PER_KITTEN_SOLVED, 0, "per solved") \
  STATISTIC (kitten_sat, 1, PCNT_KITTEN_SOLVED, "%", "solved") \
  COUNTER (kitten_solved, 2, NO_SECONDARY, 0, 0) \
  COUNTER (kitten_ticks, 2, PER_KITTEN_PROP, 0, "per prop") \
  STATISTIC (kitten_unknown, 1, PCNT_KITTEN_SOLVED, "%", "solved") \
  STATISTIC (kitten_unsat, 1, PCNT_KITTEN_SOLVED, "%", "solved") \
  METRIC (literals_bumped, 1, PER_CLS_LEARNED, 0, "per clause") \
  METRIC (literals_deduced, 1, PER_CLS_LEARNED, 0, "per clause") \
  COUNTER (literals_factor, 2, PER_VARIABLE, 0, "per variable") \
  STATISTIC (literals_factored, 1, PER_CLS_FACTORED, 0, "per factored") \
  METRIC (literals_learned, 1, PER_CLS_LEARNED, 0, "per clause") \
  METRIC (literals_minimized, 1, PCNT_LITS_DEDUCED, "%", "deduced") \
  METRIC (literals_minshrunken, 1, PCNT_LITS_SHRUNKEN, "%", "shrunken") \
  METRIC (literals_shrunken, 1, PCNT_LITS_DEDUCED, "%", "deduced") \
  STATISTIC (literals_unfactored, 2, PER_CLS_UNFACTORED, 0, "per unfactored") \
  METRIC (moved, 1, PCNT_REDUCTIONS, "%", "reductions") \
  STATISTIC (on_the_fly_strengthened, 1, PCNT_CONFLICTS, "%", "of conflicts") \
  STATISTIC (on_the_fly_subsumed, 1, PCNT_CONFLICTS, "%", "of conflicts") \
  METRIC (probing_propagations, 1, PCNT_PROPS, "%", "propagations") \
  COUNTER (probings, 2, CONF_INT, "", "interval") \
  COUNTER (probing_ticks, 2, PCNT_TICKS, "%", "ticks") \
  COUNTER (propagations, 0, PER_SECOND, "", "per second") \
  STATISTIC (queue_decisions, 1, PCNT_DECISIONS, "%", "decision") \
  STATISTIC (random_decisions, 1, PCNT_DECISIONS, "%", "decision") \
  COUNTER (random_sequences, 2, CONF_INT, "", "interval") \
  COUNTER (reductions, 1, CONF_INT, "", "interval") \
  COUNTER (reordered, 1, CONF_INT, "", "interval") \
  STATISTIC (reordered_focused, 1, PCNT_REORDERED, "%", "reordered") \
  STATISTIC (reordered_stable, 1, PCNT_REORDERED, "%", "reordered") \
  COUNTER (rephased, 1, CONF_INT, "", "interval") \
  METRIC (rephased_best, 1, PCNT_REPHASED, "%", "rephased") \
  METRIC (rephased_inverted, 1, PCNT_REPHASED, "%", "rephased") \
  METRIC (rephased_original, 1, PCNT_REPHASED, "%", "rephased") \
  METRIC (rephased_walking, 1, PCNT_REPHASED, "%", "rephased") \
  METRIC (rescaled, 2, CONF_INT, "", "interval") \
  COUNTER (restarts, 1, CONF_INT, "", "interval") \
  STATISTIC (restarts_levels, 1, PER_RESTART, 0, "per restart") \
  STATISTIC (restarts_reused_levels, 1, PCNT_RESTARTS_LEVELS, "%", "levels") \
  STATISTIC (restarts_reused_trails, 1, PCNT_RESTARTS, "%", "restarts") \
  COUNTER (retiered, 2, CONF_INT, "", "interval") \
  METRIC (saved_decisions, 1, PCNT_DECISIONS, "%", "decisions") \
  METRIC (score_decisions, 0, PCNT_DECISIONS, "%", "decision") \
  COUNTER (searches, 2, CONF_INT, "", "interval") \
  METRIC (search_propagations, 2, PCNT_PROPS, "%", "propagations") \
  COUNTER (search_ticks, 2, PCNT_TICKS, "%", "ticks") \
  METRIC (sparse_gcs, 2, PCNT_COLLECTIONS, "%", "collections") \
  METRIC (stable_decisions, 1, PCNT_DECISIONS, "%", "decisions") \
  METRIC (stable_modes, 2, CONF_INT, "", "interval") \
  METRIC (stable_propagations, 1, PCNT_PROPS, "%", "propagations") \
  METRIC (stable_restarts, 1, PCNT_RESTARTS, "%", "restarts") \
  METRIC (stable_ticks, 2, PCNT_TICKS, "%", "ticks") \
  COUNTER (strengthened, 1, PCNT_SUBSUMPTION_CHECK, "%", "checks") \
  COUNTER (substituted, 1, PCNT_VARIABLES, "%", "variables") \
  COUNTER (substitute_ticks, 2, PCNT_TICKS, "%", "ticks") \
  STATISTIC (substitute_units, 1, PCNT_VARIABLES, "%", "variables") \
  STATISTIC (substitutions, 2, CONF_INT, "", "interval") \
  COUNTER (subsumed, 1, PCNT_SUBSUMPTION_CHECK, "%", "checks") \
  COUNTER (subsumption_checks, 2, NO_SECONDARY, 0, 0) \
  COUNTER (sweep, 2, CONF_INT, "", "interval") \
  STATISTIC (sweep_clauses, 1, PER_SWEEP_VARIABLES, 0, "per sweep_variables") \
  COUNTER (sweep_completed, 2, SWEEPS_PER_COMPLETED, 0, "sweeps") \
  STATISTIC (sweep_depth, 1, PER_SWEEP_VARIABLES, 0, "per sweep_variables") \
  STATISTIC (sweep_environment, 1, PER_SWEEP_VARIABLES, 0, "per sweep_variables") \
  COUNTER (sweep_equivalences, 2, PCNT_VARIABLES, "%", "variables") \
  STATISTIC (sweep_fixed_backbone, 1, PER_SWEEP_VARIABLES, 0, "per sweep_variables") \
  STATISTIC (sweep_flip_backbone, 1, PER_SWEEP_VARIABLES, 0, "per sweep_variables") \
  STATISTIC (sweep_flipped_backbone, 1, PCNT_SWEEP_FLIP_BACKBONE, "%", "sweep_flip_backbone") \
  STATISTIC (sweep_flip_equivalences, 1, PER_SWEEP_VARIABLES, 0, "per sweep_variables") \
  STATISTIC (sweep_flipped_equivalences, 1, PCNT_SWEEP_FLIP_EQUIVALENCES, "%", "sweep_flip_equivalences") \
  STATISTIC (sweep_sat, 1, PCNT_SWEEP_SOLVED, "%", "sweep_solved") \
  STATISTIC (sweep_sat_backbone, 1, PCNT_SWEEP_SOLVED_BACKBONE, "%", "sweep_solved_backbone") \
  STATISTIC (sweep_sat_equivalences, 1, PCNT_SWEEP_SOLVED_EQUIVALENCES, "%", "sweep_solved_equivalences") \
  COUNTER (sweep_solved, 2, PCNT_KITTEN_SOLVED, "%", "kitten_solved") \
  STATISTIC (sweep_solved_backbone, 1, PCNT_SWEEP_SOLVED, "%", "sweep_solved") \
  STATISTIC (sweep_solved_equivalences, 1, PCNT_SWEEP_SOLVED, "%", "sweep_solved") \
  STATISTIC (sweep_unknown_backbone, 1, PCNT_SWEEP_SOLVED_BACKBONE, "%", "sweep_solved_backbone") \
  STATISTIC (sweep_unknown_equivalences, 1, PCNT_SWEEP_SOLVED_EQUIVALENCES, "%", "sweep_solved_equivalences") \
  COUNTER (sweep_units, 2, PCNT_VARIABLES, "%", "variables") \
  STATISTIC (sweep_unsat, 1, PCNT_SWEEP_SOLVED, "%", "sweep_solved") \
  STATISTIC (sweep_unsat_backbone, 1, PCNT_SWEEP_SOLVED_BACKBONE, "%", "sweep_solve_backbone") \
  STATISTIC (sweep_unsat_equivalences, 1, PCNT_SWEEP_SOLVED_EQUIVALENCES, "%", "sweep_solve_equivalences") \
  STATISTIC (sweep_variables, 1, PCNT_VARIABLES, "%", "variables") \
  COUNTER (switched, 0, CONF_INT, "", "interval") \
  METRIC (target_decisions, 1, PCNT_DECISIONS, "%", "decisions") \
  METRIC (target_saved, 1, CONF_INT, "", "interval") \
  STATISTIC (ticks, 2, PER_PROPAGATION, 0, "per prop") \
  METRIC (transitive_probes, 2, PER_VARIABLE, "", "per variable") \
  METRIC (transitive_propagations, 2, PCNT_PROPS, "%", "propagations") \
  METRIC (transitive_reduced, 1, PCNT_CLS_ADDED, "%", "added") \
  METRIC (transitive_reductions, 1, CONF_INT, "", "interval") \
  COUNTER (transitive_ticks, 2, PCNT_TICKS, "%", "ticks") \
  METRIC (transitive_units, 1, PCNT_VARIABLES, "%", "variables") \
  COUNTER (units, 2, PCNT_VARIABLES, "%", "variables") \
  COUNTER (variables_activated, 2, PER_VARIABLE, 0, "per variable") \
  COUNTER (variables_eliminate, 2, PER_VARIABLE, 0, "variables") \
  COUNTER (variables_extension, 2, PER_VARIABLE, 0, "per variable") \
  COUNTER (variables_factor, 2, PER_VARIABLE, 0, "per variable") \
  COUNTER (variables_original, 2, PER_VARIABLE, 0, "per variable") \
  COUNTER (variables_subsume, 2, PER_VARIABLE, 0, "per variable") \
  METRIC (vectors_defrags_needed, 1, PCNT_DEFRAGS, "%", "defrags") \
  METRIC (vectors_enlarged, 2, CONF_INT, "", "interval") \
  COUNTER (vivifications, 2, CONF_INT, "", "interval") \
  COUNTER (vivified, 1, PCNT_VIVIFY_CHECK, "%", "checks") \
  STATISTIC (vivified_asym, 1, PCNT_VIVIFIED, "%", "vivified") \
  STATISTIC (vivified_implied, 1, PCNT_VIVIFIED, "%", "vivified") \
  STATISTIC (vivified_instantiated, 1, PCNT_VIVIFIED, "%", "vivified") \
  STATISTIC (vivified_instirr, 1, PCNT_VIVIFY_INST, "%", "instantiated") \
  STATISTIC (vivified_instred, 1, PCNT_VIVIFY_INST, "%", "instantiated") \
  STATISTIC (vivified_irredundant, 1, PCNT_VIVIFIED, "%", "vivified") \
  STATISTIC (vivified_promoted, 1, PCNT_VIVIFIED, "%", "vivified") \
  STATISTIC (vivified_shrunken, 1, PCNT_VIVIFIED, "%", "vivified") \
  STATISTIC (vivified_shrunkirr, 1, PCNT_VIVIFY_SHRUNKEN, "%", "shrunken") \
  STATISTIC (vivified_shrunkred, 1, PCNT_VIVIFY_SHRUNKEN, "%", "shrunken") \
  STATISTIC (vivified_subirr, 1, PCNT_VIVIFY_SUB, "%", "subsumed") \
  STATISTIC (vivified_subred, 1, PCNT_VIVIFY_SUB, "%", "subsumed") \
  STATISTIC (vivified_subsumed, 1, PCNT_VIVIFIED, "%", "vivified") \
  STATISTIC (vivified_tier1, 1, PCNT_VIVIFIED, "%", "vivified") \
  STATISTIC (vivified_tier2, 1, PCNT_VIVIFIED, "%", "vivified") \
  STATISTIC (vivified_tier3, 1, PCNT_VIVIFIED, "%", "vivified") \
  STATISTIC (vivified_unlearn, 1, PCNT_VIVIFIED, "%", "vivified") \
  COUNTER (vivify_checks, 2, PER_VIVIFICATION, "", "per vivify") \
  COUNTER (vivify_probes, 2, PER_VIVIFY_CHECK, 0, "per check") \
  STATISTIC (vivify_propagations, 2, PCNT_PROPS, "%", "propagations") \
  COUNTER (vivify_reused, 2, PCNT_VIVIFY_PROBES, "%", "probes") \
  STATISTIC (vivify_ticks, 2, PCNT_TICKS, "%", "ticks") \
  STATISTIC (vivify_units, 1, PCNT_VARIABLES, "%", "variables") \
  METRIC (walk_decisions, 1, PCNT_WALKS, "%", "walks") \
  STATISTIC (walk_improved, 1, PCNT_WALKS, "%", "walks") \
  METRIC (walk_previous, 1, PCNT_WALKS, "%", "walks") \
  COUNTER (walks, 1, CONF_INT, "", "interval") \
  COUNTER (walk_steps, 2, PER_FLIPPED, 0, "per flipped") \
  STATISTIC (warming_conflicts, 1, PER_WALKS, 0, "per walk") \
  COUNTER (warming_decisions, 2, PER_WALKS, 0, "per walk") \
  COUNTER (warming_propagations, 2, PCNT_PROPS, "%", "propagations") \
  COUNTER (warmups, 2, PCNT_WALKS, "%", "walks") \
  METRIC (weakened, 1, PCNT_CLS_ADDED, "%", "added")

// clang-format on

/*------------------------------------------------------------------------*/
#ifdef METRICS

#define METRIC COUNTER
#define STATISTIC COUNTER

#ifndef STATISTICS
#define STATISTICS
#endif

#elif STATISTICS

#define METRIC IGNORE
#define STATISTIC COUNTER

#else

#define METRIC IGNORE
#define STATISTIC IGNORE

#endif
/*------------------------------------------------------------------------*/
// clang-format off

typedef struct statistics statistics;

#define MAX_GLUE_USED 127

struct statistics
{
#define COUNTER(NAME,VERBOSE,OTHER,UNITS,TYPE) \
  uint64_t NAME;
#define IGNORE(...)

  METRICS_COUNTERS_AND_STATISTICS

#undef COUNTER
#undef IGNORE

  struct {
    uint64_t glue[MAX_GLUE_USED + 1];
  } used[2];
};

// clang-format on
/*------------------------------------------------------------------------*/

#define CLAUSES (IRREDUNDANT_CLAUSES + BINARY_CLAUSES + REDUNDANT_CLAUSES)
#define CONFLICTS (solver->statistics_.conflicts)
#define DECISIONS (solver->statistics_.decisions)
#define IRREDUNDANT_CLAUSES (solver->statistics_.clauses_irredundant)
#define LEARNED_CLAUSES (solver->statistics_.learned)
#define REDUNDANT_CLAUSES (solver->statistics_.clauses_redundant)
#define BINARY_CLAUSES (solver->statistics_.clauses_binary)
#define BINIRR_CLAUSES (BINARY_CLAUSES + IRREDUNDANT_CLAUSES)

/*------------------------------------------------------------------------*/

#define COUNTER(NAME, VERBOSE, OTHER, UNITS, TYPE) \
\
  static inline void kissat_inc_##NAME (statistics *statistics) { \
    KISSAT_assert (statistics->NAME < UINT64_MAX); \
    statistics->NAME++; \
  } \
\
  static inline void kissat_dec_##NAME (statistics *statistics) { \
    KISSAT_assert (statistics->NAME); \
    statistics->NAME--; \
  } \
\
  static inline void kissat_add_##NAME (statistics *statistics, \
                                        uint64_t n) { \
    KISSAT_assert (UINT64_MAX - n >= statistics->NAME); \
    statistics->NAME += n; \
  } \
\
  static inline void kissat_sub_##NAME (statistics *statistics, \
                                        uint64_t n) { \
    KISSAT_assert (n <= statistics->NAME); \
    statistics->NAME -= n; \
  } \
\
  static inline uint64_t kissat_get_##NAME (statistics *statistics) { \
    return statistics->NAME; \
  }

/*------------------------------------------------------------------------*/

#define IGNORE(NAME, VERBOSE, OTHER, UNITS, TYPE) \
\
  static inline void kissat_inc_##NAME (statistics *statistics) { \
    (void) statistics; \
  } \
\
  static inline void kissat_dec_##NAME (statistics *statistics) { \
    (void) statistics; \
  } \
\
  static inline void kissat_add_##NAME (statistics *statistics, \
                                        uint64_t n) { \
    (void) statistics; \
    (void) n; \
  } \
\
  static inline void kissat_sub_##NAME (statistics *statistics, \
                                        uint64_t n) { \
    (void) statistics; \
    (void) n; \
  } \
  static inline uint64_t kissat_get_##NAME (statistics *statistics) { \
    (void) statistics; \
    return UINT64_MAX; \
  }

/*------------------------------------------------------------------------*/
// clang-format off

METRICS_COUNTERS_AND_STATISTICS

#undef COUNTER
#undef IGNORE

// clang-format on
/*------------------------------------------------------------------------*/

#define INC(NAME) kissat_inc_##NAME (&solver->statistics_)
#define DEC(NAME) kissat_dec_##NAME (&solver->statistics_)
#define ADD(NAME, N) kissat_add_##NAME (&solver->statistics_, (N))
#define SUB(NAME, N) kissat_sub_##NAME (&solver->statistics_, (N))
#define GET(NAME) kissat_get_##NAME (&solver->statistics_)

/*------------------------------------------------------------------------*/
#ifndef KISSAT_QUIET

struct kissat;

void kissat_statistics_print (struct kissat *, bool verbose);
void kissat_print_glue_usage (struct kissat *);

// Format widths of individual parts during printing statistics lines.
// Shared between 'statistics.c' and 'resources.c' to align the printing.

#define SFW1 "30"
#define SFW2 "12"
#define SFW3 "5"
#define SFW4 "10"

#define SFW34 "16"         // SFE3 + space + SFE 4
#define SFW34EXTENDED "19" // SFE3 + space + SFE 4 + ".2"

#define PRINT_STAT(NAME, PRIMARY, SECONDARY, UNITS, TYPE) \
  do { \
    printf ("%s%-" SFW1 "s %" SFW2 PRIu64 " ", solver->prefix, NAME ":", \
            (uint64_t) PRIMARY); \
    const double SAVED_SECONDARY = (double) (SECONDARY); \
    const char *SAVED_UNITS = (const char *) (UNITS); \
    const char *SAVED_TYPE = (const char *) (TYPE); \
    if (SAVED_TYPE && SAVED_SECONDARY >= 0) { \
      if (SAVED_UNITS) \
        printf ("%" SFW34 ".0f %-2s", SAVED_SECONDARY, SAVED_UNITS); \
      else \
        printf ("%" SFW34EXTENDED ".2f", SAVED_SECONDARY); \
      fputc (' ', stdout); \
      fputs (SAVED_TYPE, stdout); \
    } \
    fputc ('\n', stdout); \
  } while (0)

#endif

#ifndef KISSAT_NDEBUG

struct kissat;
void kissat_check_statistics (struct kissat *);

#else

#define kissat_check_statistics(...) \
  do { \
  } while (0)

#endif

ABC_NAMESPACE_HEADER_END

#endif
