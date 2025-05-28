#ifndef _format_h_INCLUDED
#define _format_h_INCLUDED

#include "utilities.h"

#include <stdbool.h>
#include <stdint.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

#define NUM_FORMAT_STRINGS 16
#define FORMAT_STRING_SIZE 128

typedef struct kormat kormat;

struct kormat {
  unsigned pos;
  char str[NUM_FORMAT_STRINGS][FORMAT_STRING_SIZE];
};

char *kissat_next_format_string (kormat *);

char const *kissat_format_bytes (kormat *, uint64_t bytes);
char const *kissat_format_count (kormat *, uint64_t);
char const *kissat_format_ordinal (kormat *, uint64_t);
char const *kissat_format_signs (kormat *, unsigned size, word);
char const *kissat_format_time (kormat *, double seconds);
char const *kissat_format_value (kormat *, bool boolean, int value);

#define FORMAT_BYTES(BYTES) kissat_format_bytes (&solver->format, BYTES)

#define FORMAT_COUNT(WORD) kissat_format_count (&solver->format, WORD)

#define FORMAT_ORDINAL(WORD) kissat_format_ordinal (&solver->format, WORD)

#define FORMAT_SIGNS(SIZE, SIGNS) \
  kissat_format_signs (&solver->format, SIZE, SIGNS)

#define FORMAT_TIME(SECONDS) kissat_format_time (&solver->format, SECONDS)

#define FORMAT_VALUE(BOOLEAN, VALUE) \
  kissat_format_value (&solver->format, BOOLEAN, VALUE)

ABC_NAMESPACE_HEADER_END

#endif
