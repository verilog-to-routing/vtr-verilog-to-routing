#ifndef _internal_h_INCLUDED
#define _internal_h_INCLUDED

#include "arena.h"
#include "array.h"
#include "assign.h"
#include "averages.h"
#include "check.h"
#include "classify.h"
#include "clause.h"
#include "cover.h"
#include "extend.h"
#include "flags.h"
#include "format.h"
#include "frames.h"
#include "heap.h"
#include "kimits.h"
#include "kissat.h"
#include "literal.h"
#include "mode.h"
#include "options.h"
#include "phases.h"
#include "profile.h"
#include "proof.h"
#include "queue.h"
#include "random.h"
#include "reluctant.h"
#include "rephase.h"
#include "smooth.h"
#include "stack.h"
#include "statistics.h"
#include "value.h"
#include "vector.h"
#include "watch.h"

#include "global.h"
ABC_NAMESPACE_HEADER_START

typedef struct datarank datarank;

struct datarank {
  unsigned data;
  unsigned rank;
};

typedef struct import import;

struct import {
  unsigned lit;
  bool extension;
  bool imported;
  bool eliminated;
};

typedef struct termination termination;

struct termination {
#ifdef COVERAGE
  volatile uint64_t flagged;
#else
  volatile bool flagged;
#endif
  volatile void *state;
  int (*volatile terminate) (void *);
};

// clang-format off

typedef STACK (value) eliminated;
typedef STACK (import) imports;
typedef STACK (datarank) dataranks;
typedef STACK (watch) statches;
typedef STACK (watch *) patches;

// clang-format on

struct kitten;

struct kissat {
#if !defined(KISSAT_NDEBUG) || defined(METRICS)
  bool backbone_computing;
#endif
#ifdef LOGGING
  bool compacting;
#endif
  bool extended;
  bool inconsistent;
  bool iterating;
  bool preprocessing;
  bool probing;
#ifndef KISSAT_QUIET
  bool sectioned;
#endif
  bool stable;
#if !defined(KISSAT_NDEBUG) || defined(METRICS)
  bool transitive_reducing;
  bool vivifying;
#endif
  bool warming;
  bool watching;

  bool large_clauses_watched_after_binary_clauses;

  termination termination;

  unsigned vars;
  unsigned size;
  unsigned active;
  unsigned randec;

  ints export_;
  ints units;
  imports import;
  extensions extend;
  unsigneds witness;

  assigned *assigned;
  flags *flags;

  mark *marks;

  value *values;
  phases phases;

  eliminated eliminated;
  unsigneds etrail;

  links *links;
  queue queue;

  heap scores;
  double scinc;

  heap schedule;
  double scoreshift;

  unsigned level;
  frames frames;

  unsigned_array trail;
  unsigned *propagate;

  unsigned best_assigned;
  unsigned target_assigned;
  unsigned unflushed;
  unsigned unassigned;

  unsigneds delayed;

#if defined(LOGGING) || !defined(KISSAT_NDEBUG)
  unsigneds resolvent;
#endif
  unsigned resolvent_size;
  unsigned antecedent_size;

  dataranks ranks;

  unsigneds analyzed;
  unsigneds levels;
  unsigneds minimize;
  unsigneds poisoned;
  unsigneds promote;
  unsigneds removable;
  unsigneds shrinkable;

  clause conflict;

  bool clause_satisfied;
  bool clause_shrink;
  bool clause_trivial;

  unsigneds clause;
  unsigneds shadow;

  arena arena;
  vectors vectors;
  reference first_reducible;
  reference last_irredundant;
  watches *watches;

  reference last_learned[4];

  sizes sorter;

  generator random;
  averages averages[2];
  unsigned tier1[2], tier2[2];
  reluctant reluctant;

  bounds bounds;
  classification classification;
  delays delays;
  enabled enabled;
  limited limited;
  limits limits;
  remember last;
  unsigned walked;

  mode mode;

  uint64_t ticks;

  kormat format;
  char *prefix;

  statches antecedents[2];
  statches gates[2];
  patches xorted[2];
  unsigneds resolvents;
  bool resolve_gate;

  struct kitten *kitten;
#ifdef METRICS
  uint64_t *gate_eliminated;
#else
  bool gate_eliminated;
#endif
  bool sweep_incomplete;
  unsigneds sweep_schedule;

#if !defined(KISSAT_NDEBUG) || !defined(KISSAT_NPROOFS)
  unsigneds added;
  unsigneds removed;
#endif

#if !defined(KISSAT_NDEBUG) || !defined(KISSAT_NPROOFS) || defined(LOGGING)
  ints original;
  size_t offset_of_last_original_clause;
#endif

#ifndef KISSAT_QUIET
  profiles profiles;
#endif

#ifndef KISSAT_NOPTIONS
  options options;
#endif

#ifndef KISSAT_NDEBUG
  checker *checker;
#endif

#ifndef KISSAT_NPROOFS
  proof *proof;
#endif

  statistics statistics_;
};

#define VARS (solver->vars)
#define LITS (2 * solver->vars)

#if 0
#define TIEDX (GET_OPTION (focusedtiers) ? 0 : solver->stable)
#define TIER1 (solver->tier1[TIEDX])
#define TIER2 (solver->tier2[TIEDX])
#else
#define TIER1 (solver->tier1[0])
#define TIER2 (solver->tier2[1])
#endif

#define SCORES (&solver->scores)

static inline unsigned kissat_assigned (kissat *solver) {
  KISSAT_assert (VARS >= solver->unassigned);
  return VARS - solver->unassigned;
}

#define all_variables(IDX) \
  unsigned IDX = 0, IDX##_END = solver->vars; \
  IDX != IDX##_END; \
  ++IDX

#define all_literals(LIT) \
  unsigned LIT = 0, LIT##_END = LITS; \
  LIT != LIT##_END; \
  ++LIT

#define all_clauses(C) \
  clause *C = (clause *) BEGIN_STACK (solver->arena), \
         *const C##_END = (clause *) END_STACK (solver->arena), *C##_NEXT; \
  C != C##_END && (C##_NEXT = kissat_next_clause (C), true); \
  C = C##_NEXT

#define capacity_last_learned \
  (sizeof solver->last_learned / sizeof *solver->last_learned)

#define real_end_last_learned (solver->last_learned + capacity_last_learned)

#define really_all_last_learned(REF_PTR) \
  reference *REF_PTR = solver->last_learned, \
            *REF_PTR##_END = real_end_last_learned; \
  REF_PTR != REF_PTR##_END; \
  REF_PTR++

void kissat_reset_last_learned (kissat *solver);

ABC_NAMESPACE_HEADER_END

#endif
