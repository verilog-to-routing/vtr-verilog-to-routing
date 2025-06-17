#ifndef _keatures_h_INCLUDED
#define _keatures_h_INCLUDED

#include "global.h"
ABC_NAMESPACE_HEADER_START

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define KISSAT_IS_BIG_ENDIAN
#endif

#if defined(_POSIX_C_SOURCE) || defined(__APPLE__)
#define KISSAT_HAS_COMPRESSION
#define KISSAT_HAS_COLORS
#define KISSAT_HAS_FILENO
#endif

#if defined(_POSIX_C_SOURCE)
#define KISSAT_HAS_UNLOCKEDIO
#endif

ABC_NAMESPACE_HEADER_END

#endif
