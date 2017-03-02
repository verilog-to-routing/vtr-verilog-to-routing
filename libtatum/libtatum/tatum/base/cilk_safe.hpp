#pragma once

#ifdef __cilk
    //Include the standard cilk header that defines
    //cilk_for, cilk_spawn and cilk_sync
    #include <cilk/cilk.h>
#else
    //If there is no cilk support define away the
    //cilk constructs
    #define cilk_for for //serialized for loop
    #define cilk_spawn //nop
    #define cilk_sync //nop
#endif
