#include "krite.h"
#include "inline.h"
#include "internal.h"
#include "watch.h"

#include <inttypes.h>

ABC_NAMESPACE_IMPL_START

void kissat_write_dimacs (kissat *solver, FILE *file) {
  size_t imported = SIZE_STACK (solver->import);
  if (imported)
    imported--;
  fprintf (file, "p cnf %zu %" PRIu64 "\n", imported, BINIRR_CLAUSES);
  KISSAT_assert (solver->watching);
  if (solver->watching) {
    for (all_literals (ilit))
      for (all_binary_blocking_watches (watch, WATCHES (ilit)))
        if (watch.type.binary) {
          const unsigned iother = watch.binary.lit;
          if (iother < ilit)
            continue;
          const int elit = kissat_export_literal (solver, ilit);
          const int eother = kissat_export_literal (solver, iother);
          fprintf (file, "%d %d 0\n", elit, eother);
        }
  } else {
    for (all_literals (ilit))
      for (all_binary_large_watches (watch, WATCHES (ilit)))
        if (watch.type.binary) {
          const unsigned iother = watch.binary.lit;
          if (iother < ilit)
            continue;
          const int elit = kissat_export_literal (solver, ilit);
          const int eother = kissat_export_literal (solver, iother);
          fprintf (file, "%d %d 0\n", elit, eother);
        }
  }
  for (all_clauses (c))
    if (!c->garbage && !c->redundant) {
      for (all_literals_in_clause (ilit, c)) {
        const int elit = kissat_export_literal (solver, ilit);
        fprintf (file, "%d ", elit);
      }
      fputs ("0\n", file);
    }
}

ABC_NAMESPACE_IMPL_END
