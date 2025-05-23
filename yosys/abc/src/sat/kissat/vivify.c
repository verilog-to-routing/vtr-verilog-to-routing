#include "vivify.h"
#include "allocate.h"
#include "backtrack.h"
#include "collect.h"
#include "colors.h"
#include "decide.h"
#include "deduce.h"
#include "inline.h"
#include "print.h"
#include "promote.h"
#include "proprobe.h"
#include "rank.h"
#include "report.h"
#include "sort.h"
#include "terminate.h"
#include "tiers.h"
#include "trail.h"

#include "cover.h"

#include <inttypes.h>
#include <string.h>

ABC_NAMESPACE_IMPL_START

static inline bool more_occurrences (unsigned *counts, unsigned a,
                                     unsigned b) {
  const unsigned s = counts[a], t = counts[b];
  return ((t - s) | ((b - a) & ~(s - t))) >> 31;
}

#define MORE_OCCURRENCES(A, B) more_occurrences (counts, (A), (B))

static void vivify_sort_lits_by_counts (kissat *solver, size_t size,
                                        unsigned *lits, unsigned *counts) {
  SORT (unsigned, size, lits, MORE_OCCURRENCES);
}

static void vivify_sort_stack_by_counts (kissat *solver, unsigneds *stack,
                                         unsigned *counts) {
  const size_t size = SIZE_STACK (*stack);
  unsigned *lits = BEGIN_STACK (*stack);
  vivify_sort_lits_by_counts (solver, size, lits, counts);
}

static void vivify_sort_clause_by_counts (kissat *solver, clause *c,
                                          unsigned *counts) {
  vivify_sort_lits_by_counts (solver, c->size, c->lits, counts);
}

static inline void count_literal (unsigned lit, unsigned *counts) {
  counts[lit] += counts[lit] < (unsigned) INT_MAX;
}

static void count_clause (clause *c, unsigned *counts) {
  for (all_literals_in_clause (lit, c))
    count_literal (lit, counts);
}

static bool simplify_vivification_candidate (kissat *solver,
                                             clause *const c) {
  KISSAT_assert (!solver->level);
  CLEAR_STACK (solver->clause);

  const value *const values = solver->values;

  for (all_literals_in_clause (lit, c)) {
    const value value = values[lit];
    if (value > 0) {
      LOGCLS (c, "vivification %s satisfied candidate", LOGLIT (lit));
      kissat_mark_clause_as_garbage (solver, c);
      return true;
    }
    if (!value)
      PUSH_STACK (solver->clause, lit);
  }

  unsigned non_false = SIZE_STACK (solver->clause);
  KISSAT_assert (non_false <= c->size);
  if (non_false == c->size)
    return false;

  if (non_false == 2) {
    const unsigned first = PEEK_STACK (solver->clause, 0);
    const unsigned second = PEEK_STACK (solver->clause, 1);
    LOGBINARY (first, second, "vivification shrunken candidate");
    kissat_new_binary_clause (solver, first, second);
    kissat_mark_clause_as_garbage (solver, c);
    return true;
  }

  KISSAT_assert (2 < non_false);

  CHECK_AND_ADD_STACK (solver->clause);
  ADD_STACK_TO_PROOF (solver->clause);

  REMOVE_CHECKER_CLAUSE (c);
  DELETE_CLAUSE_FROM_PROOF (c);

  const unsigned old_size = c->size;
  unsigned new_size = 0, *lits = c->lits;
  for (unsigned i = 0; i < old_size; i++) {
    const unsigned lit = lits[i];
    const value value = values[lit];
    KISSAT_assert (value <= 0);
    if (!value)
      lits[new_size++] = lit;
  }

  KISSAT_assert (2 < new_size);
  KISSAT_assert (new_size == non_false);
  KISSAT_assert (new_size < old_size);

  c->size = new_size;
  c->searched = 2;

  if (c->redundant && c->glue >= new_size)
    kissat_promote_clause (solver, c, new_size - 1);
  if (!c->shrunken) {
    c->shrunken = true;
    lits[old_size - 1] = INVALID_LIT;
  }

  LOGCLS (c, "vivification shrunken candidate");
  LOG ("vivification candidate does not need to be simplified");

  return false;
}

static unsigned vivify_tier1_limit (kissat *solver) {
  return GET_OPTION (vivifyfocusedtiers) ? solver->tier1[0] : TIER1;
}

static unsigned vivify_tier2_limit (kissat *solver) {
  return GET_OPTION (vivifyfocusedtiers) ? solver->tier2[0] : TIER2;
}

#define COUNTREF_COUNTS 1
#define LD_MAX_COUNTREF_SIZE 31
#define MAX_COUNTREF_SIZE ((1u << LD_MAX_COUNTREF_SIZE) - 1)

struct countref {
  unsigned vivify : 1;
  unsigned size : LD_MAX_COUNTREF_SIZE;
  unsigned count[COUNTREF_COUNTS];
  reference ref;
};

typedef struct countref countref;
typedef STACK (countref) countrefs;

struct vivifier {
  kissat *solver;
  unsigned *counts;
  references schedule;
  size_t scheduled, tried, vivified;
  countrefs countrefs;
  unsigneds sorted;
#ifndef KISSAT_QUIET
  const char *mode;
  const char *name;
  char tag;
#endif
  int tier;
};

typedef struct vivifier vivifier;

static void init_vivifier (kissat *solver, vivifier *vivifier) {
  vivifier->solver = solver;
  vivifier->counts = (unsigned*)kissat_calloc (solver, LITS, sizeof (unsigned));
  INIT_STACK (vivifier->schedule);
  INIT_STACK (vivifier->countrefs);
  INIT_STACK (vivifier->sorted);
  LOG ("initialized vivifier");
}

static void set_vivifier_mode (vivifier *vivifier, int tier) {
  vivifier->tier = tier;
#ifndef KISSAT_QUIET
  switch (tier) {
  case 1:
    vivifier->mode = "vivify-tier1";
    vivifier->name = "tier1";
    vivifier->tag = 'u';
    break;
  case 2:
    vivifier->mode = "vivify-tier2";
    vivifier->name = "tier2";
    vivifier->tag = 'v';
    break;
  case 3:
    KISSAT_assert (tier == 3);
    vivifier->mode = "vivify-tier3";
    vivifier->name = "tier3";
    vivifier->tag = 'w';
    break;
  default:
    KISSAT_assert (tier == 0);
    vivifier->mode = "vivify-irredundant";
    vivifier->name = "irredundant";
    vivifier->tag = 'x';
    break;
  }
#ifdef LOGGING
  kissat *solver = vivifier->solver;
  LOG ("set vivifier tier %d mode '%s' with tag '%c'", tier, vivifier->mode,
       vivifier->tag);
#endif
#endif
}

static void clear_vivifier (vivifier *vivifier) {
  kissat *solver = vivifier->solver;
  LOG ("clearing vivifier");
  unsigned *counts = vivifier->counts;
  memset (counts, 0, LITS * sizeof *counts);
  CLEAR_STACK (vivifier->schedule);
  CLEAR_STACK (vivifier->countrefs);
  CLEAR_STACK (vivifier->sorted);
}

static void release_vivifier (vivifier *vivifier) {
  kissat *solver = vivifier->solver;
  LOG ("releasing vivifier");
  unsigned *counts = vivifier->counts;
  kissat_dealloc (solver, counts, LITS, sizeof *counts);
  RELEASE_STACK (vivifier->schedule);
  RELEASE_STACK (vivifier->countrefs);
  RELEASE_STACK (vivifier->sorted);
}

static void schedule_vivification_candidates (vivifier *vivifier) {
  kissat *solver = vivifier->solver;
  LOG ("scheduling vivification candidates");
  int tier = vivifier->tier;
  unsigned lower_glue_limit, upper_glue_limit;
  unsigned tier1 = vivify_tier1_limit (solver);
  unsigned tier2 = MAX (tier1, vivify_tier2_limit (solver));
  KISSAT_assert (tier1 <= tier2);
  switch (tier) {
  case 1:
    lower_glue_limit = 0;
    upper_glue_limit = tier1;
    break;
  case 2:
    lower_glue_limit = tier1 < tier2 ? tier1 + 1 : 0;
    upper_glue_limit = tier2;
    break;
  case 3:
    lower_glue_limit = tier2 + 1;
    upper_glue_limit = UINT_MAX;
    break;
  default:
    KISSAT_assert (tier == 0);
    lower_glue_limit = 0;
    upper_glue_limit = UINT_MAX;
    break;
  }
  KISSAT_assert (lower_glue_limit <= upper_glue_limit);
  ward *const arena = BEGIN_STACK (solver->arena);
  size_t prioritized = 0;
  unsigned *counts = vivifier->counts;
  references *schedule = &vivifier->schedule;
  for (unsigned prioritize = 0; prioritize < 2; prioritize++) {
    for (all_clauses (c)) {
      if (c->garbage)
        continue;
      if (prioritize)
        count_clause (c, counts);
      if (tier) {
        if (!c->redundant)
          continue;
        if (c->glue < lower_glue_limit)
          continue;
        if (c->glue > upper_glue_limit)
          continue;
      } else if (c->redundant)
        continue;
      if (c->vivify != prioritize)
        continue;
      if (simplify_vivification_candidate (solver, c))
        continue;
      if (prioritize)
        prioritized++;
      const reference ref = (ward *) c - arena;
      PUSH_STACK (*schedule, ref);
    }
  }
  CLEAR_STACK (solver->clause);
  size_t scheduled = SIZE_STACK (*schedule);
  if (prioritized) {
    kissat_phase (solver, vivifier->mode, GET (vivifications),
                  "prioritized %zu clauses %.0f%%", prioritized,
                  kissat_percent (prioritized, scheduled));
  } else {
    kissat_phase (solver, vivifier->mode, GET (vivifications),
                  "prioritizing all %zu scheduled clauses", scheduled);
    for (all_stack (reference, ref, *schedule)) {
      clause *c = (clause *) (arena + ref);
      KISSAT_assert (kissat_clause_in_arena (solver, c));
      c->vivify = true;
    }
  }
  vivifier->scheduled = scheduled;
  vivifier->tried = vivifier->vivified = 0;
}

static inline bool worse_candidate (kissat *solver, unsigned *counts,
                                    reference r, reference s) {
  const clause *const c = kissat_dereference_clause (solver, r);
  const clause *const d = kissat_dereference_clause (solver, s);

  if (!c->vivify && d->vivify)
    return true;

  if (c->vivify && !d->vivify)
    return false;

  unsigned const *p = BEGIN_LITS (c);
  unsigned const *q = BEGIN_LITS (d);
  const unsigned *const e = END_LITS (c);
  const unsigned *const f = END_LITS (d);

  unsigned a = INVALID_LIT, b = INVALID_LIT;

  while (p != e && q != f) {
    a = *p++;
    b = *q++;
    const unsigned u = counts[a];
    const unsigned v = counts[b];
    if (u < v)
      return true;
    if (u > v)
      return false;
  }

  if (p != e && q == f)
    return true;

  if (p == e && q != f)
    return false;

  KISSAT_assert (p == e && q == f);

  if (a < b)
    return true;
  if (a > b)
    return false;

  return r < s;
}

#define WORSE_CANDIDATE(A, B) worse_candidate (solver, counts, (A), (B))

static void
sort_vivification_candidates_after_sorting_literals (vivifier *vivifier) {
  kissat *solver = vivifier->solver;
  unsigned *counts = vivifier->counts;
  references *schedule = &vivifier->schedule;
  SORT_STACK (reference, *schedule, WORSE_CANDIDATE);
}

static void sort_scheduled_candidate_literals (vivifier *vivifier) {
  kissat *solver = vivifier->solver;
  unsigned *counts = vivifier->counts;
  references *schedule = &vivifier->schedule;
  for (all_stack (reference, ref, *schedule)) {
    clause *c = kissat_dereference_clause (solver, ref);
    vivify_sort_clause_by_counts (solver, c, counts);
  }
}

static inline void init_countref (countref *cr, clause *c, reference ref,
                                  unsigned *counts) {
  KISSAT_assert (COUNTREF_COUNTS <= c->size);
  KISSAT_assert (c->size <= MAX_COUNTREF_SIZE);
  cr->size = c->size;
  cr->vivify = c->vivify;
  cr->ref = ref;
  unsigned lits[COUNTREF_COUNTS];
  for (unsigned i = 0; i != COUNTREF_COUNTS; i++) {
    unsigned best = INVALID_LIT;
    unsigned best_count = 0;
    for (all_literals_in_clause (lit, c)) {
      int continue_with_next_literal = 0;
      for (unsigned j = 0; j != i; j++)
        if (lits[j] == lit) {
          continue_with_next_literal = 1;
          break;
        }
      if(continue_with_next_literal) {
        continue;
      }
      const unsigned lit_count = counts[lit];
      KISSAT_assert (lit_count);
      if (lit_count <= best_count)
        continue;
      best_count = lit_count;
      best = lit;
    }
    KISSAT_assert (best != INVALID_LIT);
    KISSAT_assert (best_count);
    cr->count[i] = best_count;
    lits[i] = best;
  }
}

static void init_countrefs (vivifier *vivifier) {
  kissat *const solver = vivifier->solver;
  references *schedule = &vivifier->schedule;
  countrefs *countrefs = &vivifier->countrefs;
  unsigned *counts = vivifier->counts;
  KISSAT_assert (EMPTY_STACK (*countrefs));
  for (all_stack (reference, ref, *schedule)) {
    clause *c = kissat_dereference_clause (solver, ref);
    countref countref;
    init_countref (&countref, c, ref, counts);
    PUSH_STACK (*countrefs, countref);
  }
  RELEASE_STACK (*schedule);
}

#define RANK_COUNTREF_BY_INVERSE_SIZE(CR) ((unsigned) (~(CR).size))
#define RANK_COUNTREF_BY_VIVIFY(CR) ((unsigned) ((CR).vivify))
#define RANK_COUNTREF_BY_COUNT(CR) \
  ((unsigned) ((CR).count[COUNTREF_COUNTS - 1 - i]))

static void rank_vivification_candidates (vivifier *vivifier) {
  kissat *solver = vivifier->solver;
  countrefs *countrefs = &vivifier->countrefs;
  RADIX_STACK (countref, unsigned, *countrefs,
               RANK_COUNTREF_BY_INVERSE_SIZE);
  for (unsigned i = 0; i != COUNTREF_COUNTS; i++)
    RADIX_STACK (countref, unsigned, *countrefs, RANK_COUNTREF_BY_COUNT);
  RADIX_STACK (countref, unsigned, *countrefs, RANK_COUNTREF_BY_VIVIFY);
}

static void copy_countrefs (vivifier *vivifier) {
  kissat *solver = vivifier->solver;
  references *schedule = &vivifier->schedule;
  countrefs *countrefs = &vivifier->countrefs;
  KISSAT_assert (EMPTY_STACK (*schedule));
  for (all_stack (countref, cr, *countrefs))
    PUSH_STACK (*schedule, cr.ref);
  RELEASE_STACK (*countrefs);
}

static void sort_vivification_candidates (vivifier *vivifier) {
#ifndef KISSAT_QUIET
  kissat *solver = vivifier->solver;
#endif
  START (vivifysort);
  if (vivifier->tier) {
    kissat_extremely_verbose (
        solver, "sorting %s vivification candidates precisely",
        vivifier->name);
    sort_scheduled_candidate_literals (vivifier);
    sort_vivification_candidates_after_sorting_literals (vivifier);
  } else {
    kissat_extremely_verbose (
        solver,
        "sorting %s vivification candidates imprecisely "
        "by first %u literals",
        vivifier->name, (unsigned) COUNTREF_COUNTS);
    init_countrefs (vivifier);
    rank_vivification_candidates (vivifier);
    copy_countrefs (vivifier);
  }
  STOP (vivifysort);
}

static void vivify_deduce (vivifier *vivifier, clause *candidate,
                           clause *conflict, unsigned implied,
                           clause **subsuming_ptr, bool *redundant_ptr) {
  kissat *solver = vivifier->solver;
  LOG ("starting vivification conflict analysis");
  bool redundant = false;
  bool subsumes = false;

  KISSAT_assert (solver->level);
  KISSAT_assert (EMPTY_STACK (solver->clause));
  KISSAT_assert (EMPTY_STACK (solver->analyzed));

  if (implied != INVALID_LIT) {
    unsigned not_implied = NOT (implied);
    LOG ("vivify analyzing %s", LOGLIT (not_implied));
    assigned *const a = ASSIGNED (not_implied);
    KISSAT_assert (a->level);
    KISSAT_assert (!a->analyzed);
    a->analyzed = true;
    PUSH_STACK (solver->analyzed, not_implied);
    PUSH_STACK (solver->clause, implied);
  } else {
    clause *reason = conflict ? conflict : candidate;
    KISSAT_assert (reason), KISSAT_assert (!reason->garbage);
    if (reason->redundant)
      redundant = true;
    subsumes = (reason != candidate);
    for (all_literals_in_clause (other, reason)) {
      KISSAT_assert (VALUE (other) < 0);
      const value value = kissat_fixed (solver, other);
      if (value < 0)
        continue;
      LOG ("vivify analyzing %s", LOGLIT (other));
      KISSAT_assert (!value);
      assigned *const a = ASSIGNED (other);
      KISSAT_assert (a->level);
      KISSAT_assert (!a->analyzed);
      a->analyzed = true;
      PUSH_STACK (solver->analyzed, other);
      if (solver->marks[other] <= 0)
        subsumes = false;
    }
    if (reason != candidate && reason->redundant)
      kissat_recompute_and_promote (solver, reason);
    if (subsumes) {
      LOGCLS (candidate, "vivify subsumed");
      LOGCLS (reason, "vivify subsuming");
      *subsuming_ptr = reason;
      return;
    }
  }

  const mark *const marks = solver->marks;
  size_t analyzed = 0;
  while (analyzed < SIZE_STACK (solver->analyzed)) {
    const unsigned not_lit = PEEK_STACK (solver->analyzed, analyzed);
    const unsigned lit = NOT (not_lit);
    KISSAT_assert (VALUE (lit) > 0);
    analyzed++;
    assigned *a = ASSIGNED (lit);
    KISSAT_assert (a->level);
    KISSAT_assert (a->analyzed);
    if (a->reason == DECISION_REASON) {
      LOG ("vivify analyzing decision %s", LOGLIT (not_lit));
      PUSH_STACK (solver->clause, not_lit);
    } else if (a->binary) {
      const unsigned other = a->reason;
      if (marks[lit] > 0 && marks[other] > 0) {
        LOGCLS (candidate, "vivify subsumed");
        LOGBINARY (lit, other, "vivify subsuming"); // Might be jumped!
        *subsuming_ptr = kissat_binary_conflict (solver, lit, other);
        return;
      }
      KISSAT_assert (VALUE (other) < 0);
      assigned *b = ASSIGNED (other);
      KISSAT_assert (b->level);
      if (b->analyzed)
        continue;
      LOGBINARY (lit, other, "vivify analyzing %s reason", LOGLIT (lit));
      b->analyzed = true;
      PUSH_STACK (solver->analyzed, other);
    } else {
      const reference ref = a->reason;
      LOGREF (ref, "vivify analyzing %s reason", LOGLIT (lit));
      clause *reason = kissat_dereference_clause (solver, ref);
      KISSAT_assert (reason != candidate);
      if (reason->redundant)
        redundant = true;
      subsumes = true;
      for (all_literals_in_clause (other, reason)) {
        if (marks[other] <= 0)
          subsumes = false;
        if (other == lit)
          continue;
        KISSAT_assert (other != not_lit);
        KISSAT_assert (VALUE (other) < 0);
        assigned *b = ASSIGNED (other);
        if (!b->level)
          continue;
        if (b->analyzed)
          continue;
        b->analyzed = true;
        PUSH_STACK (solver->analyzed, other);
      }
      if (reason->redundant)
        kissat_recompute_and_promote (solver, reason);
      if (subsumes) {
        LOGCLS (candidate, "vivify subsumed");
        LOGCLS (reason, "vivify subsuming");
        *subsuming_ptr = reason;
        return;
      }
    }
  }

  LOG ("vivification analysis %s redundant clause",
       redundant ? "used" : "without");
  LOGTMP ("vivification analysis");

  *redundant_ptr = redundant;
}

static void reset_vivify_analyzed (vivifier *vivifier) {
  kissat *solver = vivifier->solver;
  LOG ("reset vivification conflict analysis");
  struct assigned *assigned = solver->assigned;
  for (all_stack (unsigned, lit, solver->analyzed)) {
    const unsigned idx = IDX (lit);
    struct assigned *a = assigned + idx;
    a->analyzed = false;
  }
  CLEAR_STACK (solver->analyzed);
  CLEAR_STACK (solver->clause);
}

static bool vivify_shrinkable (kissat *solver, unsigneds *sorted,
                               clause *conflict) {
  KISSAT_assert (SIZE_STACK (solver->clause) <= SIZE_STACK (*sorted));
  if (SIZE_STACK (solver->clause) == SIZE_STACK (*sorted))
    return false;
  const assigned *const assigned = solver->assigned;
  const value *const values = solver->values;
  unsigned count_implied = 0;
  for (all_stack (unsigned, lit, *sorted)) {
    value value = values[lit];
    if (!value) {
      LOG ("vivification unassigned %s thus shrinking", LOGLIT (lit));
      return true;
    }
    if (value > 0) {
      LOG ("vivification implied satisfied %s", LOGLIT (lit));
      if (conflict) {
        LOG ("at least one implied literal with conflict thus shrinking");
        return true;
      }
      if (count_implied++) {
        LOG ("at least two implied literals thus shrinking");
        return true;
      }
    } else {
      KISSAT_assert (value < 0);
      const unsigned idx = IDX (lit);
      const struct assigned *const a = assigned + idx;
      KISSAT_assert (a->level);
      if (!a->analyzed) {
        LOG ("vivification non-analyzed %s thus shrinking", LOGLIT (lit));
        return true;
      }
      if (a->reason != DECISION_REASON) {
        LOG ("vivification implied falsified %s thus shrinking",
             LOGLIT (lit));
        return true;
      }
    }
  }
  LOG ("vivification learned clause not shrinkable");
  return false;
}

static void vivify_learn_unit (kissat *solver, clause *c) {
  LOG ("vivification learns unit clause");
  KISSAT_assert (SIZE_STACK (solver->clause) == 1);
  kissat_backtrack_without_updating_phases (solver, 0);
  const unsigned unit = PEEK_STACK (solver->clause, 0);
  kissat_learned_unit (solver, unit);
  solver->iterating = true;
  kissat_mark_clause_as_garbage (solver, c);
  KISSAT_assert (!solver->level);
  clause *conflict = kissat_probing_propagate (solver, 0, true);
  KISSAT_assert (!conflict || solver->inconsistent);
  INC (vivify_units);
  (void) conflict;
}

static void vivify_learn_binary (kissat *solver, clause *c) {
  LOG ("vivification learns binary clause");
  kissat_backtrack_without_updating_phases (solver, 0);
  KISSAT_assert (SIZE_STACK (solver->clause) == 2);
  if (c->redundant)
    (void) kissat_new_redundant_clause (solver, 1);
  else
    (void) kissat_new_irredundant_clause (solver);
  kissat_mark_clause_as_garbage (solver, c);
}

static void swap_first_literal_with_best_watch (kissat *solver,
                                                unsigned *lits,
                                                unsigned size) {
  KISSAT_assert (size);
  unsigned *best_ptr = lits;
  unsigned first = *best_ptr, best = first;
  signed char value = VALUE (best);
  unsigned best_level = LEVEL (best);
  const unsigned *const end = lits + size;
  for (unsigned *p = lits + 1; value < 0 && p != end; p++) {
    unsigned lit = *p;
    const signed char value = VALUE (lit);
    if (value < 0) {
      const unsigned level = LEVEL (lit);
      if (level <= best_level)
        continue;
      best_level = level;
    }
    best_ptr = p;
    best = lit;
  }
  if (best_ptr == lits)
    return;
  LOG ("better watch %s instead of %s", LOGLIT (best), LOGLIT (first));
  *best_ptr = first;
  *lits = best;
}

static void vivify_unwatch_clause (kissat *solver, clause *c) {
  unsigned *lits = c->lits;
  const reference ref = kissat_reference_clause (solver, c);
  kissat_unwatch_blocking (solver, lits[0], ref);
  kissat_unwatch_blocking (solver, lits[1], ref);
}

static void vivify_watch_clause (kissat *solver, clause *c) {
  unsigned size = c->size;
  unsigned *lits = c->lits;
  const reference ref = kissat_reference_clause (solver, c);
  swap_first_literal_with_best_watch (solver, lits, size);
  swap_first_literal_with_best_watch (solver, lits + 1, size - 1);
  kissat_watch_blocking (solver, lits[0], lits[1], ref);
  kissat_watch_blocking (solver, lits[1], lits[0], ref);
}

static void vivify_learn_large (kissat *solver, clause *c,
                                unsigned implied) {
  KISSAT_assert (!c->garbage);
  LOG ("vivification learns large clause");

  CHECK_AND_ADD_STACK (solver->clause);
  ADD_STACK_TO_PROOF (solver->clause);

  REMOVE_CHECKER_CLAUSE (c);
  DELETE_CLAUSE_FROM_PROOF (c);

  vivify_unwatch_clause (solver, c);

  bool irredundant = !c->redundant;

  if (irredundant) { // TODO this could be made more precise.
    for (all_literals_in_clause (lit, c))
      kissat_mark_removed_literal (solver, lit);
  }

  unsigneds *learned = &solver->clause;
  const unsigned old_size = c->size;
  const unsigned new_size = SIZE_STACK (*learned);
  KISSAT_assert (new_size <= old_size);

  unsigned *lits = c->lits;
  memcpy (lits, BEGIN_STACK (*learned), new_size * sizeof *lits);

  if (irredundant)
    for (all_literals_in_clause (lit, c))
      kissat_mark_added_literal (solver, lit);

  KISSAT_assert (new_size < old_size);
  if (!c->shrunken) {
    c->shrunken = true;
    lits[old_size - 1] = INVALID_LIT;
  }
  c->size = new_size;
  if (!irredundant && c->glue >= new_size)
    kissat_promote_clause (solver, c, new_size - 1);
  c->searched = 2;

  if (implied == INVALID_LIT) {
    LOGCLS (c, "vivified shrunken after conflict");
    kissat_backtrack_without_updating_phases (solver, new_size - 2);
  } else {
    LOGCLS (c, "vivified shrunken after implied");
    KISSAT_assert (solver->level >= new_size - 1);
  }

  vivify_watch_clause (solver, c);
}

static void vivify_learn (kissat *solver, clause *c, unsigned implied) {
  size_t size = SIZE_STACK (solver->clause);
  if (size == 1)
    vivify_learn_unit (solver, c);
  else if (size == 2)
    vivify_learn_binary (solver, c);
  else
    vivify_learn_large (solver, c, implied);
}

static void binary_strengthen_after_instantiation (kissat *solver,
                                                   clause *c,
                                                   unsigned remove) {
  LOG ("vivification instantiation yields binary clause");
  KISSAT_assert (solver->level == 3);

  unsigned first = INVALID_LIT, second = INVALID_LIT;
  for (all_literals_in_clause (lit, c))
    if (lit != remove) {
      KISSAT_assert (VALUE (lit) < 0);
      if (LEVEL (lit)) {
        if (first == INVALID_LIT)
          first = lit;
        else {
          KISSAT_assert (second == INVALID_LIT);
          second = lit;
        }
      }
    }
  KISSAT_assert (first != INVALID_LIT);
  KISSAT_assert (second != INVALID_LIT);
  LOGBINARY (first, second, "vivified strengthened through instantiation");
  CLEAR_STACK (solver->clause);
  PUSH_STACK (solver->clause, first);
  PUSH_STACK (solver->clause, second);
  if (c->redundant)
    (void) kissat_new_redundant_clause (solver, 1);
  else
    (void) kissat_new_irredundant_clause (solver);

  kissat_mark_clause_as_garbage (solver, c);
  kissat_backtrack_without_updating_phases (solver, 0);
}

static void large_strengthen_after_instantiation (kissat *solver, clause *c,
                                                  unsigned remove) {
  LOG ("vivification instantiation yields large clause");
  KISSAT_assert (solver->level > 3);

  SHRINK_CLAUSE_IN_PROOF (c, remove, INVALID_LIT);
  CHECK_SHRINK_CLAUSE (c, remove, INVALID_LIT);

  vivify_unwatch_clause (solver, c);

  bool irredundant = !c->redundant;
  const unsigned old_size = c->size;
  KISSAT_assert (old_size > 3);
  unsigned new_size = 0, *lits = c->lits;
  for (unsigned i = 0; i != old_size; i++) {
    const unsigned lit = lits[i];
    if (lit == remove) {
      if (irredundant)
        kissat_mark_removed_literal (solver, lit);
    } else if (kissat_fixed (solver, lit) >= 0) {
      lits[new_size++] = lit;
      if (irredundant)
        kissat_mark_added_literal (solver, lit);
    }
  }
  KISSAT_assert (2 < new_size);
  KISSAT_assert (new_size < old_size);
  if (!c->shrunken) {
    c->shrunken = true;
    lits[old_size - 1] = INVALID_LIT;
  }
  c->size = new_size;
  if (!irredundant && c->glue >= new_size)
    kissat_promote_clause (solver, c, new_size - 1);
  c->searched = 2;
  LOGCLS (c, "vivified strengthened through instantiation");

  kissat_backtrack_without_updating_phases (solver, solver->level - 2);
  vivify_watch_clause (solver, c);
}

static void vivify_strengthen_after_instantiation (kissat *solver,
                                                   clause *c,
                                                   unsigned remove) {
  KISSAT_assert (solver->level >= 3);
  KISSAT_assert (VALUE (remove) > 0);
  KISSAT_assert (LEVEL (remove) == solver->level);

  if (solver->level == 3)
    binary_strengthen_after_instantiation (solver, c, remove);
  else
    large_strengthen_after_instantiation (solver, c, remove);
}

static void vivify_mark_sorted_literals (vivifier *vivifier) {
  kissat *solver = vivifier->solver;
  LOG ("marking sorted literals");
  unsigneds *sorted = &vivifier->sorted;
  value *marks = solver->marks;
  for (all_stack (unsigned, lit, *sorted))
    KISSAT_assert (!marks[lit]), marks[lit] = 1, marks[NOT (lit)] = -1;
}

static void vivify_unmark_sorted_literals (vivifier *vivifier) {
  kissat *solver = vivifier->solver;
  LOG ("unmarking sorted literals");
  unsigneds *sorted = &vivifier->sorted;
  value *marks = solver->marks;
  for (all_stack (unsigned, lit, *sorted))
    KISSAT_assert (solver->marks[lit] > 0), KISSAT_assert (marks[NOT (lit)] < 0),
        marks[lit] = marks[NOT (lit)] = 0;
}

static void reestablish_watch_invariant_for_candidate (kissat *solver,
                                                       clause *candidate) {
  if (!solver->level)
    return;
  if (candidate->garbage)
    return;
  unsigned *lits = candidate->lits;
  unsigned first = lits[0];
  unsigned second = lits[1];
  const signed char first_val = VALUE (first);
  const signed char second_val = VALUE (second);
  unsigned first_level = first_val ? LEVEL (first) : 0;
  unsigned second_level = second_val ? LEVEL (second) : 0;
  unsigned new_level;
  if (first_val >= 0 && second_val >= 0)
    return;
  if (first_val < 0 && !second_val)
    new_level = first_level;
  else if (first_val < 0 && second_val > 0) {
    if (first_level >= second_level)
      return;
    new_level = first_level;
  } else if (second_val < 0 && !first_val)
    new_level = second_level;
  else if (second_val < 0 && first_val > 0) {
    if (second_level >= first_level)
      return;
    new_level = second_level;
  } else {
    KISSAT_assert (first_val < 0), KISSAT_assert (second_val < 0);
    new_level = MIN (first_level, second_level);
  }
  KISSAT_assert (new_level);
  LOGCLS (candidate, "reestablish watch invariant by backtracking to %u",
          new_level - 1);
  kissat_backtrack_without_updating_phases (solver, new_level - 1);
}

static bool vivify_clause (vivifier *vivifier, clause *candidate) {

  KISSAT_assert (!candidate->garbage);

  kissat *solver = vivifier->solver;

  KISSAT_assert (solver->probing);
  KISSAT_assert (solver->watching);
  KISSAT_assert (!solver->inconsistent);

  LOGCLS (candidate, "vivifying candidate");
  LOGCOUNTEDCLS (candidate, vivifier->counts,
                 "vivifying unsorted counted candidate");

  unsigneds *sorted = &vivifier->sorted;
  CLEAR_STACK (*sorted);

  for (all_literals_in_clause (lit, candidate)) {
    const value value = kissat_fixed (solver, lit);
    if (value < 0)
      continue;
    if (value > 0) {
      LOGCLS (candidate, "%s satisfied", LOGLIT (lit));
      kissat_mark_clause_as_garbage (solver, candidate);
      return false;
    }
    PUSH_STACK (*sorted, lit);
  }

  KISSAT_assert (!candidate->garbage);

  const unsigned non_false = SIZE_STACK (*sorted);

  KISSAT_assert (1 < non_false);
  KISSAT_assert (non_false <= candidate->size);

#ifdef LOGGING
  if (non_false == candidate->size)
    LOG ("all literals root level unassigned");
  else
    LOG ("found %u root level non-falsified literals", non_false);
#endif

  if (non_false == 2) {
    LOGCLS (candidate, "skipping actually binary");
    return false;
  }

  INC (vivify_checks);

  unsigned unit = INVALID_LIT;
  for (all_literals_in_clause (lit, candidate)) {
    const value value = VALUE (lit);
    if (value < 0)
      continue;
    if (!value) {
      unit = INVALID_LIT;
      break;
    }
    KISSAT_assert (value > 0);
    if (unit != INVALID_LIT) {
      unit = INVALID_LIT;
      break;
    }
    unit = lit;
  }
  reference cand_ref = kissat_reference_clause (solver, candidate);
  if (unit != INVALID_LIT) {
    assigned *a = ASSIGNED (unit);
    KISSAT_assert (a->level);
    if (a->binary)
      unit = INVALID_LIT;
    else {
      if (a->reason != cand_ref)
        unit = INVALID_LIT;
    }
  }
  if (unit == INVALID_LIT)
    LOG ("non-reason candidate clause");
  else {
    LOG ("candidate is the reason of %s", LOGLIT (unit));
    const unsigned level = LEVEL (unit);
    KISSAT_assert (level > 0);
    LOG ("forced to backtrack to level %u", level - 1);
    kissat_backtrack_without_updating_phases (solver, level - 1);
  }

  KISSAT_assert (EMPTY_STACK (solver->analyzed));
  KISSAT_assert (EMPTY_STACK (solver->clause));

  unsigned *counts = vivifier->counts;
  vivify_sort_stack_by_counts (solver, sorted, counts);

#ifdef LOGGING
  if (candidate->redundant)
    LOGCOUNTEDREFLITS (cand_ref, SIZE_STACK (*sorted), sorted->begin,
                       counts, "vivifying sorted redundant glue %u",
                       candidate->glue);
  else
    LOGCOUNTEDREFLITS (cand_ref, SIZE_STACK (*sorted), sorted->begin,
                       counts, "vivifying sorted irredundant");
#endif

  vivify_mark_sorted_literals (vivifier);

  unsigned implied = INVALID_LIT;
  clause *conflict = 0;
  unsigned level = 0;

  for (all_stack (unsigned, lit, *sorted)) {
    if (level++ < solver->level) {
      frame *frame = &FRAME (level);
      const unsigned not_lit = NOT (lit);
      if (frame->decision == not_lit) {
        LOG ("reusing assumption %s", LOGLIT (not_lit));
        INC (vivify_reused);
        INC (vivify_probes);
        KISSAT_assert (VALUE (lit) < 0);
        continue;
      }

      LOG ("forced to backtrack to decision level %u", level - 1);
      kissat_backtrack_without_updating_phases (solver, level - 1);
    }

    const value value = VALUE (lit);
    KISSAT_assert (!value || LEVEL (lit) <= level);

    if (value < 0) {
      KISSAT_assert (LEVEL (lit));
      LOG ("literal %s already falsified", LOGLIT (lit));
      continue;
    }

    if (value > 0) {
      KISSAT_assert (LEVEL (lit));
      LOG ("literal %s already initially satisfied", LOGLIT (lit));
      implied = lit;
      break;
    }

    KISSAT_assert (!value);

    LOG ("literal %s unassigned", LOGLIT (lit));
    const unsigned not_lit = NOT (lit);
    INC (vivify_probes);

    kissat_internal_assume (solver, not_lit);
    KISSAT_assert (solver->level >= 1);

    const unsigned *p = solver->propagate;
    conflict = kissat_probing_propagate (solver, candidate, true);
    if (conflict)
      break;

    const unsigned *end = END_ARRAY (solver->trail);
    const signed char *const marks = solver->marks;
    while (p != end) {
      const unsigned other = *p++;
      const signed char mark = marks[other];
      if (mark > 0) {
        LOG ("literal %s already implied satisfied", LOGLIT (other));
        implied = other;
        goto EXIT_ASSUMPTION_LOOP;
      }
    }
  }
EXIT_ASSUMPTION_LOOP:;

#ifdef LOGGING
  if (!conflict && implied == INVALID_LIT)
    LOG ("vivification assumptions propagate without conflict nor "
         "producing an implied literal");
#endif
  KISSAT_assert (!conflict || implied == INVALID_LIT);

  if (implied != INVALID_LIT) {
    unsigned better_implied = INVALID_LIT;
    unsigned better_implied_trail = INVALID_TRAIL;
    for (all_stack (unsigned, lit, *sorted)) {
      if (VALUE (lit) <= 0)
        continue;
      unsigned lit_trail = TRAIL (lit);
      if (lit_trail > better_implied_trail)
        continue;
      better_implied_trail = lit_trail;
      better_implied = lit;
    }
    if (better_implied != implied) {
      LOG ("better implied satisfied literal %s at level %u",
           LOGLIT (better_implied), LEVEL (better_implied));
      implied = better_implied;
    }
  }

  unsigned level_after_assumptions = solver->level;
  KISSAT_assert (level_after_assumptions);

  clause *subsuming = 0;
  bool redundant = false;
  vivify_deduce (vivifier, candidate, conflict, implied, &subsuming,
                 &redundant);

  vivify_unmark_sorted_literals (vivifier);

  bool res;

  if (subsuming) {
    KISSAT_assert (!subsuming->garbage);
    if (candidate->redundant) {
      LOGCLS (candidate, "vivification subsumed");
      kissat_mark_clause_as_garbage (solver, candidate);
      if (subsuming->redundant && candidate->glue < subsuming->glue) {
        LOG ("vivify candidate with smaller glue than subsuming clause");
        kissat_promote_clause (solver, subsuming, candidate->glue);
      }
      INC (vivified_subred);
      INC (vivified_subsumed);
      res = true;
    } else if (!subsuming->redundant) {
      KISSAT_assert (!candidate->redundant);
      LOGCLS (candidate, "vivification subsumed");
      kissat_mark_clause_as_garbage (solver, candidate);
      INC (vivified_subirr);
      INC (vivified_subsumed);
      res = true;
    } else {
      KISSAT_assert (!candidate->redundant);
      KISSAT_assert (subsuming->redundant);
      LOGCLS (candidate, "vivification subsumed");
      kissat_mark_clause_as_garbage (solver, candidate);
      subsuming->redundant = false;
      KISSAT_assert (solver->statistics_.clauses_redundant);
      solver->statistics_.clauses_redundant--;
      KISSAT_assert (solver->statistics_.clauses_irredundant < UINT64_MAX);
      solver->statistics_.clauses_irredundant++;
      INC (vivified_promoted);
      LOGCLS (subsuming, "vivification promoted from redundant to");
      kissat_update_last_irredundant (solver, subsuming);
      kissat_mark_added_literals (solver, subsuming->size, subsuming->lits);
      INC (vivified_subirr);
      INC (vivified_subsumed);
      res = true;
    }
  } else if (vivify_shrinkable (solver, sorted, conflict)) {
    vivify_learn (solver, candidate, implied);
    INC (vivified_shrunken);
    if (candidate->redundant)
      INC (vivified_shrunkred);
    else
      INC (vivified_shrunkirr);
    res = true;
  } else if (implied != INVALID_LIT && candidate->redundant) {
    LOGCLS (candidate, "vivification implied");
    kissat_mark_clause_as_garbage (solver, candidate);
    INC (vivified_implied);
    res = true;
  } else if ((conflict || implied != INVALID_LIT) &&
             !candidate->redundant && !redundant) {
    LOGCLS (candidate, "vivification asymmetric tautology");
    kissat_mark_clause_as_garbage (solver, candidate);
    INC (vivified_asym);
    res = true;
  } else if (implied != INVALID_LIT) {
    LOGCLS (candidate,
            "no vivification instantiation with implied literal %s",
            LOGLIT (implied));
    KISSAT_assert (!candidate->redundant);
    KISSAT_assert (redundant);
    res = false;
  } else {
    KISSAT_assert (solver->level > 2);
    KISSAT_assert (solver->level == SIZE_STACK (*sorted));
    unsigned lit = TOP_STACK (*sorted);
    KISSAT_assert (VALUE (lit) < 0);
    KISSAT_assert (LEVEL (lit) == solver->level);
    LOG ("trying vivification instantiation of %s#%u respectively %s#%u",
         LOGLIT (lit), counts[lit], LOGLIT (NOT (lit)), counts[NOT (lit)]);
    kissat_backtrack_without_updating_phases (solver, solver->level - 1);
    conflict = 0;
    KISSAT_assert (!VALUE (lit));
    kissat_internal_assume (solver, lit);
    LOGCLS (candidate, "vivification instantiation candidate");
    conflict = kissat_probing_propagate (solver, candidate, true);
    if (conflict) {
      LOG ("vivification instantiation succeeded");
      vivify_strengthen_after_instantiation (solver, candidate, lit);
      INC (vivified_instantiated);
      if (candidate->redundant)
        INC (vivified_instred);
      else
        INC (vivified_instirr);
      res = true;
    } else {
      LOG ("vivification instantiation failed");
      kissat_backtrack_without_updating_phases (solver, solver->level - 2);
      res = false;
    }
  }

  reset_vivify_analyzed (vivifier);
  if (conflict && solver->level == level_after_assumptions) {
    LOG ("forcing backtracking at least one level after conflict");
    kissat_backtrack_without_updating_phases (solver, solver->level - 1);
  }
  reestablish_watch_invariant_for_candidate (solver, candidate);

  if (res) {
    INC (vivified);
    switch (vivifier->tier) {
    case 1:
      INC (vivified_tier1);
      break;
    case 2:
      INC (vivified_tier2);
      break;
    case 3:
      INC (vivified_tier3);
      break;
    default:
      KISSAT_assert (vivifier->tier == 0);
      INC (vivified_irredundant);
      break;
    }
  }

  LOG ("vivification %s", res ? "succeeded" : "failed");
  return res;
}

static void vivify_round (vivifier *vivifier, uint64_t limit) {
  int tier = vivifier->tier;
  kissat *solver = vivifier->solver;

  if (tier && !REDUNDANT_CLAUSES)
    return;

  KISSAT_assert (0 <= tier && tier <= 3);
  KISSAT_assert (solver->watching);
  KISSAT_assert (solver->probing);

  kissat_flush_large_watches (solver);

  schedule_vivification_candidates (vivifier);
  if (GET_OPTION (vivifysort)) {
    if (tier || IRREDUNDANT_CLAUSES / 10 <= REDUNDANT_CLAUSES)
      sort_vivification_candidates (vivifier);
    else
      kissat_extremely_verbose (
          solver, "not sorting %s vivification candidates", vivifier->name);
  }

  kissat_watch_large_clauses (solver);
#ifndef KISSAT_QUIET
  uint64_t start = solver->statistics_.probing_ticks;
  uint64_t delta = limit - start;
  kissat_very_verbose (solver,
                       "vivification %s effort limit %" PRIu64 " = %" PRIu64
                       " + %" PRIu64 " 'probing_ticks'",
                       vivifier->name, limit, start, delta);
  const size_t total = tier ? REDUNDANT_CLAUSES : IRREDUNDANT_CLAUSES;
  const size_t scheduled = SIZE_STACK (vivifier->schedule);
  kissat_phase (solver, vivifier->mode, GET (vivifications),
                "scheduled %zu clauses %.0f%% of %zu", scheduled,
                kissat_percent (scheduled, total), total);
#endif
  KISSAT_assert (!vivifier->vivified);
  KISSAT_assert (!vivifier->tried);
  while (!EMPTY_STACK (vivifier->schedule)) {
    const uint64_t probing_ticks = solver->statistics_.probing_ticks;
    if (probing_ticks > limit) {
      kissat_extremely_verbose (solver,
                                "vivification %s ticks limit %" PRIu64
                                " hit after %" PRIu64 " 'probing_ticks'",
                                vivifier->name, limit, probing_ticks);
      break;
    }
    if (TERMINATED (vivify_terminated_1))
      break;
    const reference ref = POP_STACK (vivifier->schedule);
    clause *candidate = kissat_dereference_clause (solver, ref);
    KISSAT_assert (!candidate->garbage);
    vivifier->tried++;
    if (vivify_clause (vivifier, candidate))
      vivifier->vivified++;
    if (solver->inconsistent)
      break;
    candidate->vivify = false;
  }
  if (solver->level)
    kissat_backtrack_without_updating_phases (solver, 0);
#ifndef KISSAT_QUIET
  kissat_phase (solver, vivifier->mode, GET (vivifications),
                "vivified %zu clauses %.0f%% out of %zu tried",
                vivifier->vivified,
                kissat_percent (vivifier->vivified, vivifier->tried),
                vivifier->tried);
  if (!solver->inconsistent) {
    size_t remain = SIZE_STACK (vivifier->schedule);
    if (remain) {
      kissat_phase (solver, vivifier->mode, GET (vivifications),
                    "%zu clauses remain %.0f%% out of %zu scheduled",
                    remain, kissat_percent (remain, scheduled), scheduled);

      ward *const arena = BEGIN_STACK (solver->arena);
      size_t prioritized = 0;
      while (!EMPTY_STACK (vivifier->schedule)) {
        const reference ref = POP_STACK (vivifier->schedule);
        clause *c = (clause *) (arena + ref);
        if (c->vivify)
          prioritized++;
      }
      if (!prioritized)
        kissat_phase (solver, vivifier->mode, GET (vivifications),
                      "no remaining prioritized clauses");
      else
        kissat_phase (solver, vivifier->mode, GET (vivifications),
                      "keeping all %zu remaining clauses "
                      "prioritized %.0f%%",
                      prioritized, kissat_percent (prioritized, remain));
    } else
      kissat_phase (solver, vivifier->mode, GET (vivifications),
                    "no untried clauses remain");
  }
#endif
  REPORT (!vivifier->vivified, vivifier->tag);
}

static void vivify_tier1 (vivifier *vivifier, uint64_t limit) {
#ifndef KISSAT_QUIET
  kissat *solver = vivifier->solver;
#endif
  START (vivify1);
  set_vivifier_mode (vivifier, 1);
  vivify_round (vivifier, limit);
  STOP (vivify1);
}

static void vivify_tier2 (vivifier *vivifier, uint64_t limit) {
#ifndef KISSAT_QUIET
  kissat *solver = vivifier->solver;
#endif
  START (vivify2);
  clear_vivifier (vivifier);
  set_vivifier_mode (vivifier, 2);
  vivify_round (vivifier, limit);
  STOP (vivify2);
}

static void vivify_tier3 (vivifier *vivifier, uint64_t limit) {
#ifndef KISSAT_QUIET
  kissat *solver = vivifier->solver;
#endif
  START (vivify3);
  clear_vivifier (vivifier);
  set_vivifier_mode (vivifier, 3);
  vivify_round (vivifier, limit);
  STOP (vivify3);
}

static void vivify_irredundant (vivifier *vivifier, uint64_t limit) {
#ifndef KISSAT_QUIET
  kissat *solver = vivifier->solver;
#endif
  START (vivify0);
  clear_vivifier (vivifier);
  set_vivifier_mode (vivifier, 0);
  vivify_round (vivifier, limit);
  STOP (vivify0);
}

void kissat_vivify (kissat *solver) {
  if (solver->inconsistent)
    return;
  KISSAT_assert (!solver->level);
  KISSAT_assert (solver->probing);
  KISSAT_assert (solver->watching);
  if (!GET_OPTION (vivify))
    return;
  if (TERMINATED (vivify_terminated_2))
    return;
  double irr_budget = DELAYING (vivifyirr) ? 0 : GET_OPTION (vivifyirr);
  double tier1_budget = GET_OPTION (vivifytier1);
  double tier2_budget = GET_OPTION (vivifytier2);
  double tier3_buget = GET_OPTION (vivifytier3);
  if (!REDUNDANT_CLAUSES)
    tier1_budget = tier2_budget = tier3_buget = 0;
  double sum = irr_budget + tier1_budget + tier2_budget + tier3_buget;
  if (!sum)
    return;

  START (vivify);
  INC (vivifications);
#if !defined(KISSAT_NDEBUG) || defined(METRICS)
  KISSAT_assert (!solver->vivifying);
  solver->vivifying = true;
#endif

  SET_EFFORT_LIMIT (limit, vivify, probing_ticks);
  const uint64_t total = limit - solver->statistics_.probing_ticks;
  limit = solver->statistics_.probing_ticks;
  unsigned tier1_limit = vivify_tier1_limit (solver);
  unsigned tier2_limit = vivify_tier2_limit (solver);
  if (tier1_budget && tier2_budget && tier1_limit == tier2_limit) {
    tier1_budget += tier2_budget, tier2_budget = 0;
    LOG ("vivification tier1 matches tier2 "
         "thus using tier2 budget for tier1");
  }

  {
    vivifier vivifier;
    init_vivifier (solver, &vivifier);
    if (tier1_budget) {
      limit += (total * tier1_budget) / sum;
      vivify_tier1 (&vivifier, limit);
    }
    if (tier2_budget && !solver->inconsistent &&
        !TERMINATED (vivify_terminated_3)) {
      limit += (total * tier2_budget) / sum;
      vivify_tier2 (&vivifier, limit);
    }
    if (tier3_buget && !solver->inconsistent &&
        !TERMINATED (vivify_terminated_4)) {
      limit += (total * tier3_buget) / sum;
      vivify_tier3 (&vivifier, limit);
    }
    if (irr_budget && !solver->inconsistent &&
        !TERMINATED (vivify_terminated_5)) {
      limit += (total * irr_budget) / sum;
      vivify_irredundant (&vivifier, limit);
      if (kissat_average (vivifier.vivified, vivifier.tried) < 0.01)
        BUMP_DELAY (vivifyirr);
      else
        REDUCE_DELAY (vivifyirr);
    }
    release_vivifier (&vivifier);
  }

#ifndef KISSAT_QUIET
  if (solver->statistics_.probing_ticks < limit) {
    uint64_t delta = limit - solver->statistics_.probing_ticks;
    kissat_phase (solver, "vivify-limit", GET (vivifications),
                  "has %" PRIu64 " ticks left %.2f%%", delta,
                  kissat_percent (delta, total));
  } else {
    uint64_t delta = solver->statistics_.probing_ticks - limit;
    kissat_phase (solver, "vivify-limit", GET (vivifications),
                  "exceeded by %" PRIu64 " ticks %.2f%%", delta,
                  kissat_percent (delta, total));
  }
#endif
#if !defined(KISSAT_NDEBUG) || defined(METRICS)
  KISSAT_assert (solver->vivifying);
  solver->vivifying = false;
#endif
  STOP (vivify);
}

ABC_NAMESPACE_IMPL_END
