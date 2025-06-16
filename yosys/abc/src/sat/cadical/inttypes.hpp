#ifndef _inttypes_h_INCLUDED
#define _inttypes_h_INCLUDED

#include "global.h"

// This is an essence a wrapper around '<cinttypes>' respectively
// 'inttypes.h' in order to please the 'MinGW' cross-compiler (we are using
// 'i686-w64-mingw32-gcc') to produce correct 'printf' style formatting for
// 64-bit numbers as this does not work out-of-the-box (which is also very
// annoying).  This also produces lots of warnings (through '-Wformat' and
// the corresponding 'attribute' declaration for 'printf' style functions).
// Again 'MinGW' is not fully standard compliant here and we have to cover
// up for that manually.

// We repeat the code on making this work which is also contained in
// 'cadical.hpp' as we do not want to require users of the library to
// include another header file (like this one) beside 'cadical.hpp'.

#ifndef PRINTF_FORMAT
#ifdef __MINGW32__
#define __USE_MINGW_ANSI_STDIO 1
#define PRINTF_FORMAT __MINGW_PRINTF_FORMAT
#else
#define PRINTF_FORMAT printf
#endif
#endif

#include <cinttypes>

#endif
