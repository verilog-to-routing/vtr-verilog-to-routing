// clang-format off

#ifndef _print_h_INCLUDED
#define _print_h_INCLUDED

#include <stdint.h>

#include "attribute.h"

#include "global.h"
ABC_NAMESPACE_HEADER_START

#ifndef KISSAT_QUIET

struct kissat;

int kissat_verbosity (struct kissat *);

void kissat_line (struct kissat *);
void kissat_prefix (struct kissat*);
void kissat_signal (struct kissat *, const char *type, int sig);
void kissat_section (struct kissat *, const char *name);

void
kissat_message (struct kissat *, const char *fmt, ...)
ATTRIBUTE_FORMAT (2, 3);

void kissat_verbose (struct kissat *, const char *fmt, ...)
ATTRIBUTE_FORMAT (2, 3);

void kissat_very_verbose (struct kissat *, const char *fmt, ...)
ATTRIBUTE_FORMAT (2, 3);

void kissat_extremely_verbose (struct kissat *, const char *fmt, ...)
ATTRIBUTE_FORMAT (2, 3);

void kissat_warning (struct kissat *, const char *fmt, ...)
ATTRIBUTE_FORMAT (2, 3);

void kissat_phase (struct kissat *, const char *name, uint64_t,
		   const char * fmt, ...)
ATTRIBUTE_FORMAT (4, 5);

#else

#define kissat_line(...) do { } while (0)
#define kissat_message(...) do { } while (0)
#define kissat_phase(...) do { } while (0)
#define kissat_section(...) do { } while (0)
#define kissat_signal(...) do { } while (0)
#define kissat_verbose(...) do { } while (0)
#define kissat_very_verbose(...) do { } while (0)
#define kissat_extremely_verbose(...) do { } while (0)
#define kissat_warning(...) do { } while (0)

#endif

#define VERY_VERBOSE_OR_LOG(ONLY_LOG, SOLVER, ...) \
do { \
  if (ONLY_LOG) \
    LOG (__VA_ARGS__); \
  else \
    kissat_very_verbose (SOLVER, __VA_ARGS__); \
} while (0)

ABC_NAMESPACE_HEADER_END

#endif

// clang-format on
