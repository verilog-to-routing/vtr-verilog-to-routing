#ifndef _phases_h_INCLUDED
#define _phases_h_INCLUDED

#include "value.h"

#include "global.h"
ABC_NAMESPACE_HEADER_START

typedef struct phases phases;

struct phases {
  value *best;
  value *saved;
  value *target;
};

#define BEST(IDX) \
  (solver->phases.best[KISSAT_assert (VALID_INTERNAL_INDEX (IDX)), (IDX)])

#define SAVED(IDX) \
  (solver->phases.saved[KISSAT_assert (VALID_INTERNAL_INDEX (IDX)), (IDX)])

#define TARGET(IDX) \
  (solver->phases.target[KISSAT_assert (VALID_INTERNAL_INDEX (IDX)), (IDX)])

struct kissat;

void kissat_increase_phases (struct kissat *, unsigned);
void kissat_decrease_phases (struct kissat *, unsigned);
void kissat_release_phases (struct kissat *);

void kissat_save_best_phases (struct kissat *);
void kissat_save_target_phases (struct kissat *);

ABC_NAMESPACE_HEADER_END

#endif
