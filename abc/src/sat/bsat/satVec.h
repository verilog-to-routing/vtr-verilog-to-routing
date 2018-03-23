/**************************************************************************************************
MiniSat -- Copyright (c) 2005, Niklas Sorensson
http://www.cs.chalmers.se/Cs/Research/FormalMethods/MiniSat/

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/
// Modified to compile with MS Visual Studio 6.0 by Alan Mishchenko

#ifndef ABC__sat__bsat__satVec_h
#define ABC__sat__bsat__satVec_h

#include "misc/util/abc_global.h"

ABC_NAMESPACE_HEADER_START


// vector of 32-bit intergers (added for 64-bit portability)
struct veci_t {
    int    cap;
    int    size;
    int*   ptr;
};
typedef struct veci_t veci;

static inline void veci_new (veci* v) {
    v->cap  = 4;
    v->size = 0;
    v->ptr  = (int*)ABC_ALLOC( char, sizeof(int)*v->cap);
}

static inline void   veci_delete (veci* v)          { ABC_FREE(v->ptr);   }
static inline int*   veci_begin  (veci* v)          { return v->ptr;  }
static inline int    veci_size   (veci* v)          { return v->size; }
static inline void   veci_resize (veci* v, int k)   { 
    assert(k <= v->size); 
//    memset( veci_begin(v) + k, -1, sizeof(int) * (veci_size(v) - k) );
    v->size = k;         
} // only safe to shrink !!
static inline int    veci_pop    (veci* v)          { assert(v->size); return v->ptr[--v->size]; }
static inline void   veci_push   (veci* v, int e)
{
    if (v->size == v->cap) {
//        int newsize = v->cap * 2;//+1;
        int newsize = (v->cap < 4) ? v->cap * 2 : (v->cap / 2) * 3;
        v->ptr = ABC_REALLOC( int, v->ptr, newsize );
        if ( v->ptr == NULL )
        {
            printf( "Failed to realloc memory from %.1f MB to %.1f MB.\n", 
                1.0 * v->cap / (1<<20), 1.0 * newsize / (1<<20) );
            fflush( stdout );
        }
        v->cap = newsize; }
    v->ptr[v->size++] = e;
}
static inline void   veci_remove(veci* v, int e)
{
    int * ws = (int*)veci_begin(v);
    int    j  = 0;
    for (; ws[j] != e  ; j++);
    assert(j < veci_size(v));
    for (; j < veci_size(v)-1; j++) ws[j] = ws[j+1];
    veci_resize(v,veci_size(v)-1);
}


// vector of 32- or 64-bit pointers
struct vecp_t {
    int    cap;
    int    size;
    void** ptr;
};
typedef struct vecp_t vecp;

static inline void vecp_new (vecp* v) {
    v->size = 0;
    v->cap  = 4;
    v->ptr  = (void**)ABC_ALLOC( char, sizeof(void*)*v->cap);
}

static inline void   vecp_delete (vecp* v)          { ABC_FREE(v->ptr);   }
static inline void** vecp_begin  (vecp* v)          { return v->ptr;  }
static inline int    vecp_size   (vecp* v)          { return v->size; }
static inline void   vecp_resize (vecp* v, int   k) { assert(k <= v->size); v->size = k;    } // only safe to shrink !!
static inline void   vecp_push   (vecp* v, void* e)
{
    if (v->size == v->cap) {
//        int newsize = v->cap * 2;//+1;
        int newsize = (v->cap < 4) ? v->cap * 2 : (v->cap / 2) * 3;
        v->ptr = ABC_REALLOC( void*, v->ptr, newsize );
        v->cap = newsize; }
    v->ptr[v->size++] = e;
}
static inline void   vecp_remove(vecp* v, void* e)
{
    void** ws = vecp_begin(v);
    int    j  = 0;
    for (; ws[j] != e  ; j++);
    assert(j < vecp_size(v));
    for (; j < vecp_size(v)-1; j++) ws[j] = ws[j+1];
    vecp_resize(v,vecp_size(v)-1);
}



//=================================================================================================
// Simple types:

#ifndef __cplusplus
#ifndef false
#  define false 0
#endif
#ifndef true
#  define true 1
#endif
#endif

typedef int    lit;
typedef int    cla;

typedef char               lbool;

static const int   var_Undef = -1;
static const lit   lit_Undef = -2;

static const lbool l_Undef   =  0;
static const lbool l_True    =  1;
static const lbool l_False   = -1;

static inline lit  toLit    (int v)        { return v + v; }
static inline lit  toLitCond(int v, int c) { return v + v + (c != 0); }
static inline lit  lit_neg  (lit l)        { return l ^ 1; }
static inline int  lit_var  (lit l)        { return l >> 1; }
static inline int  lit_sign (lit l)        { return l & 1; }
static inline int  lit_print(lit l)        { return lit_sign(l)? -lit_var(l)-1 : lit_var(l)+1; }
static inline lit  lit_read (int s)        { return s > 0 ? toLit(s-1) : lit_neg(toLit(-s-1)); }
static inline int  lit_check(lit l, int n) { return l >= 0 && lit_var(l) < n;                  }

struct stats_t
{
    unsigned starts, clauses, learnts;
    ABC_INT64_T decisions, propagations, inspects, conflicts;
    ABC_INT64_T clauses_literals, learnts_literals, tot_literals;
};
typedef struct stats_t stats_t;

ABC_NAMESPACE_HEADER_END

#endif
