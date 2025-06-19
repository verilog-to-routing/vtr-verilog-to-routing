#include "internal.h"
#include "logging.h"
#include "resize.h"

ABC_NAMESPACE_IMPL_START

static void adjust_imports_for_external_literal (kissat *solver,
                                                 unsigned eidx) {
  while (eidx >= SIZE_STACK (solver->import)) {
    struct import import;
    import.lit = 0;
    import.extension = false;
    import.imported = false;
    import.eliminated = false;
    PUSH_STACK (solver->import, import);
  }
}

static void adjust_exports_for_external_literal (kissat *solver,
                                                 unsigned eidx,
                                                 bool extension) {
  struct import *import = &PEEK_STACK (solver->import, eidx);
  unsigned iidx = solver->vars;
  kissat_enlarge_variables (solver, iidx + 1);
  unsigned ilit = 2 * iidx;
  import->extension = extension;
  import->imported = true;
  if (extension)
    INC (variables_extension);
  else
    INC (variables_original);
  KISSAT_assert (!import->eliminated);
  import->lit = ilit;
  LOG ("importing %s external variable %u as internal literal %u",
       extension ? "extension" : "original", eidx, ilit);
  while (iidx >= SIZE_STACK (solver->export_))
    PUSH_STACK (solver->export_, 0);
  POKE_STACK (solver->export_, iidx, (int) eidx);
  LOG ("exporting internal variable %u as external literal %u", iidx, eidx);
}

static inline unsigned import_literal (kissat *solver, int elit,
                                       bool extension) {
  const unsigned eidx = ABS (elit);
  adjust_imports_for_external_literal (solver, eidx);
  struct import *import = &PEEK_STACK (solver->import, eidx);
  if (import->eliminated)
    return INVALID_LIT;
  unsigned ilit;
  if (!import->imported)
    adjust_exports_for_external_literal (solver, eidx, extension);
  KISSAT_assert (import->imported);
  ilit = import->lit;
  if (elit < 0)
    ilit = NOT (ilit);
  KISSAT_assert (VALID_INTERNAL_LITERAL (ilit));
  return ilit;
}

unsigned kissat_import_literal (kissat *solver, int elit) {
  KISSAT_assert (VALID_EXTERNAL_LITERAL (elit));
  if (GET_OPTION (tumble))
    return import_literal (solver, elit, false);
  const unsigned eidx = ABS (elit);
  KISSAT_assert (SIZE_STACK (solver->import) <= UINT_MAX);
  unsigned other = SIZE_STACK (solver->import);
  if (eidx < other)
    return import_literal (solver, elit, false);
  if (!other)
    adjust_imports_for_external_literal (solver, other++);

  unsigned ilit = 0;
  do {
    KISSAT_assert (VALID_EXTERNAL_LITERAL ((int) other));
    ilit = import_literal (solver, other, false);
  } while (other++ < eidx);

  if (elit < 0)
    ilit = NOT (ilit);

  return ilit;
}

unsigned kissat_fresh_literal (kissat *solver) {
  size_t imported = SIZE_STACK (solver->import);
  KISSAT_assert (imported <= EXTERNAL_MAX_VAR);
  if (imported == EXTERNAL_MAX_VAR) {
    LOG ("can not get another external variable");
    return INVALID_LIT;
  }
  KISSAT_assert (imported <= (unsigned) INT_MAX);
  int eidx = (int) imported;
  unsigned res = import_literal (solver, eidx, true);
#ifndef KISSAT_NDEBUG
  struct import *import = &PEEK_STACK (solver->import, imported);
  KISSAT_assert (import->imported);
  KISSAT_assert (import->extension);
#endif
  INC (fresh);
  kissat_activate_literal (solver, res);
  return res;
}

ABC_NAMESPACE_IMPL_END
