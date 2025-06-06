#ifndef _literal_h_INCLUDED
#define _literal_h_INCLUDED

#include <limits.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

#define LD_MAX_VAR 30u
#define LD_MAX_LIT (1 + LD_MAX_VAR)

#define EXTERNAL_MAX_VAR ((1 << LD_MAX_VAR) - 1)
#define INTERNAL_MAX_VAR ((1u << LD_MAX_VAR) - 2)
#define INTERNAL_MAX_LIT (2 * INTERNAL_MAX_VAR + 1)

#define ILLEGAL_LIT ((1u << LD_MAX_LIT) - 1)

#define INVALID_IDX UINT_MAX
#define INVALID_LIT UINT_MAX

#define VALID_INTERNAL_INDEX(IDX) ((IDX) < VARS)

#define VALID_INTERNAL_LITERAL(LIT) ((LIT) < LITS)

#define VALID_EXTERNAL_LITERAL(LIT) \
  ((LIT) && ((LIT) != INT_MIN) && ABS (LIT) <= EXTERNAL_MAX_VAR)

#define IDX(LIT) \
  (KISSAT_assert (VALID_INTERNAL_LITERAL (LIT)), (((unsigned) (LIT)) >> 1))

#define LIT(IDX) (KISSAT_assert (VALID_INTERNAL_INDEX (IDX)), ((IDX) << 1))

#define NOT(LIT) (KISSAT_assert (VALID_INTERNAL_LITERAL (LIT)), ((LIT) ^ 1u))

#define NEGATED(LIT) (KISSAT_assert (VALID_INTERNAL_LITERAL (LIT)), ((LIT) & 1u))

#define STRIP(LIT) (KISSAT_assert (VALID_INTERNAL_LITERAL (LIT)), ((LIT) & ~1u))

ABC_NAMESPACE_HEADER_END

#endif
