#include "internal.h"

ABC_NAMESPACE_IMPL_START

void kissat_init_averages (kissat *solver, averages *averages) {
  if (averages->initialized)
    return;
#define INIT_EMA(EMA, WINDOW) \
  kissat_init_smooth (solver, &averages->EMA, WINDOW, #EMA)
#ifndef KISSAT_QUIET
  INIT_EMA (level, GET_OPTION (emaslow));
  INIT_EMA (size, GET_OPTION (emaslow));
  INIT_EMA (trail, GET_OPTION (emaslow));
#endif
  INIT_EMA (fast_glue, GET_OPTION (emafast));
  INIT_EMA (slow_glue, GET_OPTION (emaslow));
  INIT_EMA (decision_rate, GET_OPTION (emaslow));
  averages->initialized = true;
}

ABC_NAMESPACE_IMPL_END
