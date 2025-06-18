#include "colors.h"

#if defined(WIN32) && !defined(__MINGW32__)
#define isatty _isatty
#else
#include <unistd.h>
#endif

ABC_NAMESPACE_IMPL_START

int kissat_is_terminal[3] = {0, -1, -1};

int kissat_initialize_terminal (int fd) {
  KISSAT_assert (fd == 1 || fd == 2);
  KISSAT_assert (kissat_is_terminal[fd] < 0);
  return kissat_is_terminal[fd] = isatty (fd);
}

void kissat_force_colors (void) {
  kissat_is_terminal[1] = kissat_is_terminal[2] = 1;
}

void kissat_force_no_colors (void) {
  kissat_is_terminal[1] = kissat_is_terminal[2] = 0;
}

ABC_NAMESPACE_IMPL_END
