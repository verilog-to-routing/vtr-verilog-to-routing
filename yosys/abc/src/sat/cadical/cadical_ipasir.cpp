#include "global.h"

#include "ipasir.h"
#include "ccadical.h"

ABC_NAMESPACE_IMPL_START

const char *ipasir_signature () { return ccadical_signature (); }

void *ipasir_init () { return ccadical_init (); }

void ipasir_release (void *solver) {
  ccadical_release ((CCaDiCaL *) solver);
}

void ipasir_add (void *solver, int lit) {
  ccadical_add ((CCaDiCaL *) solver, lit);
}

void ipasir_assume (void *solver, int lit) {
  ccadical_assume ((CCaDiCaL *) solver, lit);
}

int ipasir_solve (void *solver) {
  return ccadical_solve ((CCaDiCaL *) solver);
}

int ipasir_val (void *solver, int lit) {
  return ccadical_val ((CCaDiCaL *) solver, lit);
}

int ipasir_failed (void *solver, int lit) {
  return ccadical_failed ((CCaDiCaL *) solver, lit);
}

void ipasir_set_terminate (void *solver, void *state,
                           int (*terminate) (void *state)) {
  ccadical_set_terminate ((CCaDiCaL *) solver, state, terminate);
}

void ipasir_set_learn (void *solver, void *state, int max_length,
                       void (*learn) (void *state, int *clause)) {
  ccadical_set_learn ((CCaDiCaL *) solver, state, max_length, learn);
}

ABC_NAMESPACE_IMPL_END
