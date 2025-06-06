#ifndef _reference_h_INCLUDED
#define _reference_h_INCLUDED

#include "stack.h"

#include "global.h"
ABC_NAMESPACE_HEADER_START

typedef unsigned reference;

#define REFERENCE_FORMAT "u"

#define LD_MAX_REF 31u
#define MAX_REF ((1u << LD_MAX_REF) - 1)

#define INVALID_REF UINT_MAX

// clang-format off
typedef STACK (reference) references;
// clang-format on

ABC_NAMESPACE_HEADER_END

#endif
