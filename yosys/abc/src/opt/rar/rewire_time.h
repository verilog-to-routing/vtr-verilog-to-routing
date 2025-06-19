/**CFile****************************************************************

  FileName    [rewire_time.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Re-wiring.]

  Synopsis    []

  Author      [Jiun-Hao Chen]
  
  Affiliation [National Taiwan University]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rewire_time.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef RAR_TIME_H
#define RAR_TIME_H

/*************************************************************
                 counting wall time
**************************************************************/

#include "base/abc/abc.h"

ABC_NAMESPACE_HEADER_START

static inline iword Time_Clock() {
#if defined(__APPLE__) && defined(__MACH__)
#define APPLE_MACH (__APPLE__ & __MACH__)
#else
#define APPLE_MACH 0
#endif
#if (defined(LIN) || defined(LIN64)) && !APPLE_MACH && !defined(__MINGW32__)
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
        return (iword)-1;
    iword res = ((iword)ts.tv_sec) * CLOCKS_PER_SEC;
    res += (((iword)ts.tv_nsec) * CLOCKS_PER_SEC) / 1000000000;
    return res;
#else
    return (iword)clock();
#endif
}

static inline void Time_Print(const char *pStr, iword time) {
    printf("%s = %10.2f sec", pStr, (float)1.0 * ((double)(time)) / ((double)CLOCKS_PER_SEC));
}

static inline void Time_PrintEndl(const char *pStr, iword time) {
    Time_Print(pStr, time);
    printf("\n");
}

ABC_NAMESPACE_HEADER_END

#endif // RAR_TIME_H
