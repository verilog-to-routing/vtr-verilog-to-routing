#ifndef _value_h_INCLUDED
#define _value_h_INCLUDED

#include "global.h"
ABC_NAMESPACE_HEADER_START

typedef signed char value;
typedef signed char mark;

#define VALUE(LIT) (solver->values[KISSAT_assert ((LIT) < LITS), (LIT)])

#define MARK(LIT) (solver->marks[KISSAT_assert ((LIT) < LITS), (LIT)])

#define BOOL_TO_VALUE(B) ((signed char) ((B) ? -1 : 1))

ABC_NAMESPACE_HEADER_END

#endif
