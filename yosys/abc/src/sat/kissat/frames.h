#ifndef _frames_h_INCLUDED
#define _frames_h_INCLUDED

#include "literal.h"
#include "stack.h"

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

typedef struct frame frame;
typedef struct slice slice;

struct frame {
  bool promote;
  unsigned decision;
  unsigned trail;
  unsigned used;
#ifndef KISSAT_NDEBUG
  unsigned saved;
#endif
};

// clang-format off

typedef STACK (frame) frames;

// clang-format on

struct kissat;

#define FRAME(LEVEL) (PEEK_STACK (solver->frames, (LEVEL)))

ABC_NAMESPACE_HEADER_END

#endif
