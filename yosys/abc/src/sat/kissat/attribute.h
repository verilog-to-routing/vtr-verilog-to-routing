#ifndef _attribute_h_INCLUDED
#define _attribute_h_INCLUDED

#include "global.h"
ABC_NAMESPACE_HEADER_START

/* #define ATTRIBUTE_FORMAT(FORMAT_POSITION, VARIADIC_ARGUMENT_POSITION) \ */
/*   __attribute__ (( \ */
/*       format (printf, FORMAT_POSITION, VARIADIC_ARGUMENT_POSITION))) */

#define ATTRIBUTE_FORMAT(FORMAT_POSITION, VARIADIC_ARGUMENT_POSITION)

/* #define ATTRIBUTE_ALWAYS_INLINE __attribute__ ((always_inline)) */

#define ATTRIBUTE_ALWAYS_INLINE

ABC_NAMESPACE_HEADER_END

#endif
