#include "classify.h"
#include "internal.h"
#include "print.h"

ABC_NAMESPACE_IMPL_START

void kissat_classify (struct kissat *solver) {
  statistics *s = &solver->statistics_;
  uint64_t clauses = s->clauses_binary + s->clauses_irredundant;
  unsigned small_clauses_limit = GET_OPTION (smallclauses);
  if (clauses <= small_clauses_limit) {
    solver->classification.small = true;
    solver->classification.bigbig = false;
  } else {
    solver->classification.small = false;
    unsigned bigbigfraction = GET_OPTION (bigbigfraction);
    double percent = bigbigfraction / 1000.0;
    double actual = kissat_percent (s->clauses_binary, clauses);
    if (actual >= percent)
      solver->classification.bigbig = true;
    else
      solver->classification.bigbig = false;
  }
  kissat_very_verbose (
      solver, "formula classified as having a %s total number of clauses",
      solver->classification.small ? "small" : "large");
  kissat_very_verbose (
      solver, "formula classified to have a %s binary clauses fraction",
      solver->classification.bigbig ? "large" : "small");
}

ABC_NAMESPACE_IMPL_END
