#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

// Sam Buss suggested to debug the case where a solver incorrectly claims
// the formula to be unsatisfiable by checking every learned clause to be
// satisfied by a satisfying assignment.  Thus the first inconsistent
// learned clause will be immediately flagged without the need to generate
// proof traces and perform forward proof checking.  The incorrectly derived
// clause will raise an abort signal and thus allows to debug the issue with
// a symbolic debugger immediately.

void External::check_solution_on_learned_clause () {
  CADICAL_assert (solution);
  for (const auto &lit : internal->clause)
    if (sol (internal->externalize (lit)) == lit)
      return;
  fatal_message_start ();
  fputs ("learned clause unsatisfied by solution:\n", stderr);
  for (const auto &lit : internal->clause)
    fprintf (stderr, "%d ", lit);
  fputc ('0', stderr);
  fatal_message_end ();
}

void External::check_solution_on_shrunken_clause (Clause *c) {
  CADICAL_assert (solution);
  for (const auto &lit : *c)
    if (sol (internal->externalize (lit)) == lit)
      return;
  fatal_message_start ();
  for (const auto &lit : *c)
    fprintf (stderr, "%d ", lit);
  fputc ('0', stderr);
  fatal_message_end ();
}

void External::check_no_solution_after_learning_empty_clause () {
  CADICAL_assert (solution);
  FATAL ("learned empty clause but got solution");
}

void External::check_solution_on_learned_unit_clause (int unit) {
  CADICAL_assert (solution);
  if (sol (internal->externalize (unit)) == unit)
    return;
  FATAL ("learned unit %d contradicts solution", unit);
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
