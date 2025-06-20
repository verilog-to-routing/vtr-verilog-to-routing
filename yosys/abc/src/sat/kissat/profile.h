#ifndef _profile_h_INCLUDED
#define _profile_h_INCLUDED

#include "stack.h"

#include "global.h"
ABC_NAMESPACE_HEADER_START

#ifndef KISSAT_QUIET

typedef struct profile profile;
typedef struct profiles profiles;

#define PROFS \
  PROF (analyze, 3) \
  PROF (backbone, 2) \
  PROF (bump, 3) \
  PROF (collect, 3) \
  PROF (congruence, 2) \
  PROF (decide, 4) \
  PROF (deduce, 3) \
  PROF (definition, 3) \
  PROF (defrag, 3) \
  PROF (dominate, 4) \
  PROF (eliminate, 2) \
  PROF (extend, 2) \
  PROF (extract, 3) \
  PROF (extractands, 3) \
  PROF (extractbinaries, 3) \
  PROF (extractites, 3) \
  PROF (extractxors, 3) \
  PROF (factor, 2) \
  PROF (fastel, 2) \
  PROF (focused, 2) \
  PROF (forward, 4) \
  PROF (lucky, 2) \
  PROF (matching, 3) \
  PROF (merge, 3) \
  PROF (minimize, 3) \
  PROF (parse, 1) \
  PROF (preprocess, 2) \
  PROF (probe, 2) \
  PROF (propagate, 4) \
  PROF (radix, 4) \
  PROF (reduce, 2) \
  PROF (reorder, 3) \
  PROF (rephase, 3) \
  PROF (restart, 3) \
  PROF (search, 1) \
  PROF (shrink, 3) \
  PROF (simplify, 1) \
  PROF (sort, 4) \
  PROF (stable, 2) \
  PROF (substitute, 2) \
  PROF (subsume, 2) \
  PROF (sweep, 2) \
  PROF (sweepbackbone, 3) \
  PROF (sweepequivalences, 3) \
  PROF (total, 0) \
  PROF (transitive, 2) \
  PROF (vivify, 2) \
  PROF (vivify0, 3) \
  PROF (vivify1, 3) \
  PROF (vivify2, 3) \
  PROF (vivify3, 3) \
  PROF (vivifysort, 4) \
  PROF (walking, 2) \
  PROF (warmup, 3)

struct profile {
  int level;
  const char *name;
  double entered;
  double time;
};

struct profiles {
#define PROF(NAME, LEVEL) profile NAME;
  PROFS
#undef PROF
  STACK (profile *) stack;
};

struct kissat;

void kissat_init_profiles (profiles *);
void kissat_profiles_print (struct kissat *);
void kissat_start (struct kissat *, profile *);
void kissat_stop (struct kissat *, profile *);

void kissat_stop_search_and_start_simplifier (struct kissat *, profile *);
void kissat_stop_simplifier_and_resume_search (struct kissat *, profile *);

double kissat_time (struct kissat *);

#define PROFILE(NAME) (solver->profiles.NAME)

#define START(NAME) \
  do { \
    profile *profile = &PROFILE (NAME); \
    if (GET_OPTION (profile) >= profile->level) \
      kissat_start (solver, profile); \
  } while (0)

#define STOP(NAME) \
  do { \
    profile *profile = &PROFILE (NAME); \
    if (GET_OPTION (profile) >= profile->level) \
      kissat_stop (solver, profile); \
  } while (0)

#define STOP_SEARCH_AND_START_SIMPLIFIER(NAME) \
  do { \
    if (GET_OPTION (profile) >= PROFILE (search).level) \
      kissat_stop_search_and_start_simplifier (solver, &PROFILE (NAME)); \
  } while (0)

#define STOP_SIMPLIFIER_AND_RESUME_SEARCH(NAME) \
  do { \
    if (GET_OPTION (profile) >= PROFILE (search).level) \
      kissat_stop_simplifier_and_resume_search (solver, &PROFILE (NAME)); \
  } while (0)

#else

#define START(...) \
  do { \
  } while (0)
#define STOP(...) \
  do { \
  } while (0)

#define STOP_SEARCH_AND_START_SIMPLIFIER(...) \
  do { \
  } while (0)
#define STOP_SIMPLIFIER_AND_RESUME_SEARCH(...) \
  do { \
  } while (0)

#define STOP_AND_START(...) \
  do { \
  } while (0)

#endif

ABC_NAMESPACE_HEADER_END

#endif
