#include "definition.h"
#include "allocate.h"
#include "gates.h"
#include "inline.h"
#include "kitten.h"
#include "print.h"

ABC_NAMESPACE_IMPL_START

typedef struct definition_extractor definition_extractor;

struct definition_extractor {
  unsigned lit;
  kissat *solver;
  watches *watches[2];
};

static void traverse_definition_core (void *state, unsigned id) {
  definition_extractor *extractor = (definition_extractor*)state;
  kissat *solver = extractor->solver;
  watch watch;
  watches *watches0 = extractor->watches[0];
  watches *watches1 = extractor->watches[1];
  const size_t size_watches0 = SIZE_WATCHES (*watches0);
  KISSAT_assert (size_watches0 <= UINT_MAX);
  unsigned sign;
  if (id < size_watches0) {
    watch = BEGIN_WATCHES (*watches0)[id];
    LOGWATCH (extractor->lit, watch, "gate[0]");
    sign = 0;
  } else {
    unsigned tmp = id - size_watches0;
#ifndef KISSAT_NDEBUG
    const size_t size_watches1 = SIZE_WATCHES (*watches1);
    KISSAT_assert (size_watches1 <= UINT_MAX);
    KISSAT_assert (tmp < size_watches1);
#endif
    watch = BEGIN_WATCHES (*watches1)[tmp];
    LOGWATCH (NOT (extractor->lit), watch, "gate[1]");
    sign = 1;
  }
  PUSH_STACK (solver->gates[sign], watch);
}

#if !defined(KISSAT_NDEBUG) || !defined(KISSAT_NPROOFS)

typedef struct lemma_extractor lemma_extractor;

struct lemma_extractor {
  kissat *solver;
  unsigned lemmas;
  unsigned unit;
};

static void traverse_one_sided_core_lemma (void *state, bool learned,
                                           size_t size,
                                           const unsigned *lits) {
  if (!learned)
    return;
  lemma_extractor *extractor = (lemma_extractor*)state;
  kissat *solver = extractor->solver;
  const unsigned unit = extractor->unit;
  unsigneds *added = &solver->added;
  KISSAT_assert (extractor->lemmas || EMPTY_STACK (*added));
  if (size) {
    PUSH_STACK (*added, size + 1);
    const size_t offset = SIZE_STACK (*added);
    PUSH_STACK (*added, unit);
    const unsigned *end = lits + size;
    for (const unsigned *p = lits; p != end; p++)
      PUSH_STACK (*added, *p);
    unsigned *extended = &PEEK_STACK (*added, offset);
    KISSAT_assert (offset + size + 1 == SIZE_STACK (*added));
    CHECK_AND_ADD_LITS (size + 1, extended);
    ADD_LITS_TO_PROOF (size + 1, extended);
  } else {
    kissat_learned_unit (solver, unit);
    const unsigned *end = END_STACK (*added);
    unsigned *begin = BEGIN_STACK (*added);
    for (unsigned *p = begin, size; p != end; p += size) {
      size = *p++;
      KISSAT_assert (p + size <= end);
      REMOVE_CHECKER_LITS (size, p);
      DELETE_LITS_FROM_PROOF (size, p);
    }
    CLEAR_STACK (*added);
  }
  extractor->lemmas++;
}

#endif

bool kissat_find_definition (kissat *solver, unsigned lit) {
  if (!GET_OPTION (definitions))
    return false;
  START (definition);
  struct kitten *kitten = solver->kitten;
  KISSAT_assert (kitten);
  kitten_clear (kitten);
  const unsigned not_lit = NOT (lit);
  definition_extractor extractor;
  extractor.lit = lit;
  extractor.solver = solver;
  extractor.watches[0] = &WATCHES (lit);
  extractor.watches[1] = &WATCHES (not_lit);
  kitten_track_antecedents (kitten);
  unsigned exported = 0;
#if !defined(KISSAT_QUIET) || !defined(KISSAT_NDEBUG)
  size_t occs[2] = {0, 0};
#endif
  for (unsigned sign = 0; sign < 2; sign++) {
    const unsigned except = sign ? not_lit : lit;
    for (all_binary_large_watches (watch, *extractor.watches[sign])) {
      if (watch.type.binary) {
        const unsigned other = watch.binary.lit;
        kitten_clause_with_id_and_exception (kitten, exported, 1, &other,
                                             INVALID_LIT);
      } else {
        const reference ref = watch.large.ref;
        clause *c = kissat_dereference_clause (solver, ref);
        kitten_clause_with_id_and_exception (kitten, exported, c->size,
                                             c->lits, except);
      }
#if !defined(KISSAT_QUIET) || !defined(KISSAT_NDEBUG)
      occs[sign]++;
#endif
      exported++;
    }
  }
  bool res = false;
  LOG ("exported %u = %zu + %zu environment clauses to sub-solver",
       exported, occs[0], occs[1]);
  INC (definitions_checked);
  const size_t limit = GET_OPTION (definitionticks);
  kitten_set_ticks_limit (kitten, limit);
  int status = kitten_solve (kitten);
  if (status == 20) {
    LOG ("sub-solver result UNSAT shows definition exists");
    uint64_t learned;
    unsigned reduced = kitten_compute_clausal_core (kitten, &learned);
    LOG ("1st sub-solver core of size %u original clauses out of %u",
         reduced, exported);
    for (int i = 2; i <= GET_OPTION (definitioncores); i++) {
      kitten_shrink_to_clausal_core (kitten);
      kitten_shuffle_clauses (kitten);
      kitten_set_ticks_limit (kitten, 10 * limit);
      int tmp = kitten_solve (kitten);
      KISSAT_assert (!tmp || tmp == 20);
      if (!tmp) {
        LOG ("aborting core extraction");
        goto ABORT;
      }
#ifndef KISSAT_NDEBUG
      unsigned previous = reduced;
#endif
      reduced = kitten_compute_clausal_core (kitten, &learned);
      LOG ("%s sub-solver core of size %u original clauses out of %u",
           FORMAT_ORDINAL (i), reduced, exported);
      KISSAT_assert (reduced <= previous);
#if defined(KISSAT_QUIET) && defined(KISSAT_NDEBUG)
      (void) reduced;
#endif
    }
    INC (definitions_extracted);
    kitten_traverse_core_ids (kitten, &extractor, traverse_definition_core);
    size_t size[2];
    size[0] = SIZE_STACK (solver->gates[0]);
    size[1] = SIZE_STACK (solver->gates[1]);
#if !defined(KISSAT_QUIET) || !defined(KISSAT_NDEBUG)
    KISSAT_assert (reduced == size[0] + size[1]);
#ifdef METRICS
    kissat_extremely_verbose (
        solver,
        "definition extracted[%" PRIu64 "] "
        "size %u = %zu + %zu clauses %.0f%% "
        "of %u = %zu + %zu (checked %" PRIu64 ")",
        solver->statistics.definitions_extracted, reduced, size[0], size[1],
        kissat_percent (reduced, exported), exported, occs[0], occs[1],
        solver->statistics.definitions_checked);
#else
    kissat_extremely_verbose (solver,
                              "definition extracted with core "
                              "size %u = %zu + %zu clauses %.0f%% "
                              "of %u = %zu + %zu",
                              reduced, size[0], size[1],
                              kissat_percent (reduced, exported), exported,
                              occs[0], occs[1]);
#endif
#endif
    unsigned unit = INVALID_LIT;
    if (!size[0]) {
      unit = not_lit;
      KISSAT_assert (size[1]);
    } else if (!size[1])
      unit = lit;

    if (unit != INVALID_LIT) {
      INC (definition_units);

      kissat_extremely_verbose (solver, "one sided core "
                                        "definition extraction yields "
                                        "failed literal");
#if !defined(KISSAT_NDEBUG) || !defined(KISSAT_NPROOFS)
      if (false
#ifndef KISSAT_NDEBUG
          || GET_OPTION (check) > 1
#endif
#ifndef KISSAT_NPROOFS
          || solver->proof
#endif
      ) {
        lemma_extractor extractor;
        extractor.solver = solver;
        extractor.unit = unit;
        extractor.lemmas = 0;
        kitten_traverse_core_clauses (kitten, &extractor,
                                      traverse_one_sided_core_lemma);
      } else
#endif
        kissat_learned_unit (solver, unit);
    }
    solver->gate_eliminated = GATE_ELIMINATED (definitions);
    solver->resolve_gate = true;
    res = true;
  } else {
  ABORT:
    LOG ("sub-solver failed to show that definition exists");
  }
  CLEAR_STACK (solver->analyzed);
  STOP (definition);
  return res;
}

ABC_NAMESPACE_IMPL_END
