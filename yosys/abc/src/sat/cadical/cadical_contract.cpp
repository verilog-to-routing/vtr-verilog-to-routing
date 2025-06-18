#include "global.h"

#ifndef CADICAL_NCONTRACTS

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

void fatal_message_start ();

// See comments in 'contract.hpp'. Ugly hack we keep for now.

void require_solver_pointer_to_be_non_zero (const void *ptr,
                                            const char *function_name,
                                            const char *file_name) {
  if (ptr)
    return;
  fatal_message_start ();
  fprintf (stderr,
           "invalid API usage of '%s' in '%s': "
           "solver 'this' pointer zero (not initialized)\n",
           function_name, file_name);
  fflush (stderr);
  abort ();
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END

#endif
