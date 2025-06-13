#include "global.h"

#include "cadical.hpp"

#include <cstdlib>
#include <cstring>

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

struct Wrapper : Learner, Terminator {

  Solver *solver;
  struct {
    void *state;
    int (*function) (void *);
  } terminator;

  struct {
    void *state;
    int max_length;
    int *begin_clause, *end_clause, *capacity_clause;
    void (*function) (void *, int *);
  } learner;

  bool terminate () {
    if (!terminator.function)
      return false;
    return terminator.function (terminator.state);
  }

  bool learning (int size) {
    if (!learner.function)
      return false;
    return size <= learner.max_length;
  }

  void learn (int lit) {
    if (learner.end_clause == learner.capacity_clause) {
      size_t count = learner.end_clause - learner.begin_clause;
      size_t size = count ? 2 * count : 1;
      learner.begin_clause =
          (int *) realloc (learner.begin_clause, size * sizeof (int));
      learner.end_clause = learner.begin_clause + count;
      learner.capacity_clause = learner.begin_clause + size;
    }
    *learner.end_clause++ = lit;
    if (lit)
      return;
    learner.function (learner.state, learner.begin_clause);
    learner.end_clause = learner.begin_clause;
  }

  Wrapper () : solver (new Solver ()) {
    memset (&terminator, 0, sizeof terminator);
    memset (&learner, 0, sizeof learner);
  }

  ~Wrapper () {
    terminator.function = 0;
    if (learner.begin_clause)
      free (learner.begin_clause);
    delete solver;
  }
};

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END

#include "ccadical.h"

ABC_NAMESPACE_IMPL_START

using namespace CaDiCaL;

const char *ccadical_signature (void) { return Solver::signature (); }

CCaDiCaL *ccadical_init (void) { return (CCaDiCaL *) new Wrapper (); }

void ccadical_release (CCaDiCaL *wrapper) { delete (Wrapper *) wrapper; }

void ccadical_constrain (CCaDiCaL *wrapper, int lit) {
  ((Wrapper *) wrapper)->solver->constrain (lit);
}

int ccadical_constraint_failed (CCaDiCaL *wrapper) {
  return ((Wrapper *) wrapper)->solver->constraint_failed ();
}

void ccadical_set_option (CCaDiCaL *wrapper, const char *name, int val) {
  ((Wrapper *) wrapper)->solver->set (name, val);
}

void ccadical_limit (CCaDiCaL *wrapper, const char *name, int val) {
  ((Wrapper *) wrapper)->solver->limit (name, val);
}

int ccadical_get_option (CCaDiCaL *wrapper, const char *name) {
  return ((Wrapper *) wrapper)->solver->get (name);
}

void ccadical_add (CCaDiCaL *wrapper, int lit) {
  ((Wrapper *) wrapper)->solver->add (lit);
}

void ccadical_assume (CCaDiCaL *wrapper, int lit) {
  ((Wrapper *) wrapper)->solver->assume (lit);
}

int ccadical_solve (CCaDiCaL *wrapper) {
  return ((Wrapper *) wrapper)->solver->solve ();
}

int ccadical_simplify (CCaDiCaL *wrapper) {
  return ((Wrapper *) wrapper)->solver->simplify ();
}

int ccadical_val (CCaDiCaL *wrapper, int lit) {
  return ((Wrapper *) wrapper)->solver->val (lit);
}

int ccadical_failed (CCaDiCaL *wrapper, int lit) {
  return ((Wrapper *) wrapper)->solver->failed (lit);
}

void ccadical_print_statistics (CCaDiCaL *wrapper) {
  ((Wrapper *) wrapper)->solver->statistics ();
}

void ccadical_terminate (CCaDiCaL *wrapper) {
  ((Wrapper *) wrapper)->solver->terminate ();
}

int64_t ccadical_active (CCaDiCaL *wrapper) {
  return ((Wrapper *) wrapper)->solver->active ();
}

int64_t ccadical_irredundant (CCaDiCaL *wrapper) {
  return ((Wrapper *) wrapper)->solver->irredundant ();
}

int ccadical_fixed (CCaDiCaL *wrapper, int lit) {
  return ((Wrapper *) wrapper)->solver->fixed (lit);
}

void ccadical_set_terminate (CCaDiCaL *ptr, void *state,
                             int (*terminate) (void *)) {
  Wrapper *wrapper = (Wrapper *) ptr;
  wrapper->terminator.state = state;
  wrapper->terminator.function = terminate;
  if (terminate)
    wrapper->solver->connect_terminator (wrapper);
  else
    wrapper->solver->disconnect_terminator ();
}

void ccadical_set_learn (CCaDiCaL *ptr, void *state, int max_length,
                         void (*learn) (void *state, int *clause)) {
  Wrapper *wrapper = (Wrapper *) ptr;
  wrapper->learner.state = state;
  wrapper->learner.max_length = max_length;
  wrapper->learner.function = learn;
  if (learn)
    wrapper->solver->connect_learner (wrapper);
  else
    wrapper->solver->disconnect_learner ();
}

void ccadical_freeze (CCaDiCaL *ptr, int lit) {
  ((Wrapper *) ptr)->solver->freeze (lit);
}

void ccadical_melt (CCaDiCaL *ptr, int lit) {
  ((Wrapper *) ptr)->solver->melt (lit);
}

int ccadical_frozen (CCaDiCaL *ptr, int lit) {
  return ((Wrapper *) ptr)->solver->frozen (lit);
}

int ccadical_trace_proof (CCaDiCaL *ptr, FILE *file, const char *path) {
  return ((Wrapper *) ptr)->solver->trace_proof (file, path);
}

void ccadical_close_proof (CCaDiCaL *ptr) {
  ((Wrapper *) ptr)->solver->close_proof_trace ();
}

void ccadical_conclude (CCaDiCaL *ptr) {
  ((Wrapper *) ptr)->solver->conclude ();
}

int ccadical_vars (CCaDiCaL *ptr) {
  return ((Wrapper *) ptr)->solver->vars ();
}

int ccadical_reserve_difference (CCaDiCaL *ptr, int number_of_vars) {
  return ((Wrapper *) ptr)->solver->reserve_difference (number_of_vars);
}

void ccadical_reserve(CCaDiCaL *ptr, int min_max_var) {
  ((Wrapper *) ptr)->solver->reserve(min_max_var);
}

int ccadical_is_inconsistent(CCaDiCaL *ptr) {
  return ((Wrapper *) ptr)->solver->inconsistent ();
}

ABC_NAMESPACE_IMPL_END
