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

#ifndef satVec_h
#define satVec_h

#include <stdlib.h>

// vector of 32-bit intergers (added for 64-bit portability)
struct veci_t {
    int    size;
    int    cap;
    int*   ptr;
};
typedef struct veci_t veci;

static inline void veci_new (veci* v) {
    v->size = 0;
    v->cap  = 4;
    v->ptr  = (int*)malloc(sizeof(int)*v->cap);
}

static inline void   veci_delete (veci* v)          { free(v->ptr);   }
static inline int*   veci_begin  (veci* v)          { return v->ptr;  }
static inline int    veci_size   (veci* v)          { return v->size; }
static inline void   veci_resize (veci* v, int k)   { v->size = k;    } // only safe to shrink !!
static inline void   veci_push   (veci* v, int e)
{
    if (v->size == v->cap) {
        int newsize = v->cap * 2;//+1;
        v->ptr = (int*)realloc(v->ptr,sizeof(int)*newsize);
        v->cap = newsize; }
    v->ptr[v->size++] = e;
}


// vector of 32- or 64-bit pointers
struct vecp_t {
    int    size;
    int    cap;
    void** ptr;
};
typedef struct vecp_t vecp;

static inline void vecp_new (vecp* v) {
    v->size = 0;
    v->cap  = 4;
    v->ptr  = (void**)malloc(sizeof(void*)*v->cap);
}

static inline void   vecp_delete (vecp* v)          { free(v->ptr);   }
static inline void** vecp_begin  (vecp* v)          { return v->ptr;  }
static inline int    vecp_size   (vecp* v)          { return v->size; }
static inline void   vecp_resize (vecp* v, int   k) { v->size = k;    } // only safe to shrink !!
static inline void   vecp_push   (vecp* v, void* e)
{
    if (v->size == v->cap) {
        int newsize = v->cap * 2;//+1;
        v->ptr = (void**)realloc(v->ptr,sizeof(void*)*newsize);
        v->cap = newsize; }
    v->ptr[v->size++] = e;
}


#endif
