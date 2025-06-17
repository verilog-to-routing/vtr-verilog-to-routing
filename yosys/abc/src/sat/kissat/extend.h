#ifndef _extend_h_INCLUDED
#define _extend_h_INCLUDED

#include "stack.h"
#include "utilities.h"

#include "global.h"
ABC_NAMESPACE_HEADER_START

typedef struct extension extension;

struct extension {
  signed int lit : 31;
  bool blocking : 1;
};

// clang-format off
typedef STACK (extension) extensions;
// clang-format on

static inline extension kissat_extension (bool blocking, int lit) {
  KISSAT_assert (ABS (lit) < (1 << 30));
  extension res;
  res.blocking = blocking;
  res.lit = lit;
  return res;
}

struct kissat;

void kissat_extend (struct kissat *solver);

ABC_NAMESPACE_HEADER_END

#endif
