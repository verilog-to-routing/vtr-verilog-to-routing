#ifndef _terminate_h_INCLUDED
#define _terminate_h_INCLUDED

#include "internal.h"

#include "global.h"
ABC_NAMESPACE_HEADER_START

#ifndef KISSAT_QUIET
void kissat_report_termination (kissat *, const char *name,
                                const char *file, long lineno,
                                const char *fun);
#endif

static inline bool kissat_terminated (kissat *solver, int bit,
                                      const char *name, const char *file,
                                      long lineno, const char *fun) {
  KISSAT_assert (0 <= bit), KISSAT_assert (bit < 64);
#ifdef COVERAGE
  const uint64_t mask = (uint64_t) 1 << bit;
  if (!(solver->termination.flagged & mask))
    return false;
  solver->termination.flagged = ~(uint64_t) 0;
#else
  if (!solver->termination.flagged)
    return false;
#endif
#ifndef KISSAT_QUIET
  kissat_report_termination (solver, name, file, lineno, fun);
#else
  (void) file;
  (void) fun;
  (void) lineno;
  (void) name;
#endif
#if !defined(COVERAGE) && defined(KISSAT_NDEBUG)
  (void) bit;
#endif
  return true;
}

#define TERMINATED(BIT) \
  kissat_terminated (solver, BIT, #BIT, __FILE__, __LINE__, __func__)

#define backbone_terminated_1 1
#define backbone_terminated_2 2
#define backbone_terminated_3 3
#define congruence_terminated_1 4
#define congruence_terminated_2 5
#define congruence_terminated_3 6
#define congruence_terminated_4 7
#define congruence_terminated_5 8
#define congruence_terminated_6 9
#define congruence_terminated_7 10
#define congruence_terminated_8 11
#define congruence_terminated_9 12
#define congruence_terminated_10 13
#define congruence_terminated_11 14
#define congruence_terminated_12 15
#define eliminate_terminated_1 16
#define eliminate_terminated_2 17
#define factor_terminated_1 18
#define fastel_terminated_1 19
#define forward_terminated_1 20
#define kitten_terminated_1 21
#define kitten_terminated_2 22
#define preprocess_terminated_1 23
#define search_terminated_1 24
#define substitute_terminated_1 25
#define sweep_terminated_1 26
#define sweep_terminated_2 27
#define sweep_terminated_3 28
#define sweep_terminated_4 29
#define sweep_terminated_5 30
#define sweep_terminated_6 31
#define sweep_terminated_7 32
#define sweep_terminated_8 33
#define transitive_terminated_1 34
#define transitive_terminated_2 35
#define transitive_terminated_3 36
#define vivify_terminated_1 37
#define vivify_terminated_2 38
#define vivify_terminated_3 39
#define vivify_terminated_4 40
#define vivify_terminated_5 41
#define walk_terminated_1 42
#define warmup_terminated_1 43

ABC_NAMESPACE_HEADER_END

#endif
