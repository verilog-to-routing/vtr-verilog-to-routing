/**CFile****************************************************************

  FileName    [rewire_vec.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Re-wiring.]

  Synopsis    []

  Author      [Jiun-Hao Chen]
  
  Affiliation [National Taiwan University]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rewire_vec.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef RAR_VI_H
#define RAR_VI_H

/*************************************************************
                 vector of 32-bit integers
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "rewire_rng.h"

ABC_NAMESPACE_HEADER_START

// swapping two variables
#define RW_SWAP(Type, a, b) \
    {                       \
        Type t = a;         \
        a = b;              \
        b = t;              \
    }

typedef struct vi_ {
    int size;
    int cap;
    int *ptr;
} vi;

// iterator through the entries in the vector
#define Vi_ForEachEntry(v, entry, i)             for (i = 0; (i < (v)->size) && (((entry) = Vi_Read((v), i)), 1); i++)
#define Vi_ForEachEntryReverse(v, entry, i)      for (i = (v)->size - 1; (i >= 0) && (((entry) = Vi_Read((v), i)), 1); i--)
#define Vi_ForEachEntryStart(v, entry, i, start) for (i = start; (i < (v)->size) && (((entry) = Vi_Read((v), i)), 1); i++)
#define Vi_ForEachEntryStop(v, entry, i, stop)   for (i = 0; (i < stop) && (((entry) = Vi_Read((v), i)), 1); i++)

static inline void Vi_Start(vi *v, int cap) {
    v->size = 0;
    v->cap = cap;
    v->ptr = (int *)malloc(sizeof(int) * cap);
}

static inline vi *Vi_Alloc(int cap) {
    vi *v = (vi *)malloc(sizeof(vi));
    Vi_Start(v, cap);
    return v;
}

static inline void Vi_Stop(vi *v) {
    if (v->ptr) free(v->ptr);
}

static inline void Vi_Free(vi *v) {
    if (v->ptr) free(v->ptr);
    free(v);
}

static inline int Vi_Size(vi *v) {
    return v->size;
}

static inline int Vi_Space(vi *v) {
    return v->cap - v->size;
}

static inline int *Vi_Array(vi *v) {
    return v->ptr;
}

static inline int Vi_Read(vi *v, int k) {
    assert(k < v->size);
    return v->ptr[k];
}

static inline void Vi_Write(vi *v, int k, int e) {
    assert(k < v->size);
    v->ptr[k] = e;
}

static inline void Vi_Shrink(vi *v, int k) {
    assert(k <= v->size);
    v->size = k;
} // only safe to shrink !!

static inline int Vi_Pop(vi *v) {
    assert(v->size > 0);
    return v->ptr[--v->size];
}

static inline void Vi_Grow(vi *v) {
    if (v->size < v->cap)
        return;
    int newcap = (v->cap < 4) ? 8 : (v->cap / 2) * 3;
    v->ptr = (int *)realloc(v->ptr, sizeof(int) * newcap);
    if (v->ptr == NULL) {
        printf("Failed to realloc memory from %.1f MB to %.1f MB.\n", 4.0 * v->cap / (1 << 20), (float)4.0 * newcap / (1 << 20));
        fflush(stdout);
    }
    v->cap = newcap;
}

static inline vi *Vi_Dup(vi *v) {
    vi *pNew = Vi_Alloc(v->size);
    memcpy(pNew->ptr, v->ptr, sizeof(int) * v->size);
    pNew->size = v->size;
    return pNew;
}

static inline void Vi_Push(vi *v, int e) {
    Vi_Grow(v);
    v->ptr[v->size++] = e;
}

static inline void Vi_PushTwo(vi *v, int e1, int e2) {
    Vi_Push(v, e1);
    Vi_Push(v, e2);
}

static inline void Vi_PushArray(vi *v, int *p, int n) {
    for (int i = 0; i < n; i++)
        Vi_Push(v, p[i]);
}

static inline void Vi_PushOrder(vi *v, int e) {
    Vi_Push(v, e);
    if (v->size > 1)
        for (int i = v->size - 2; i >= 0; i--) {
            if (v->ptr[i] > v->ptr[i + 1]) {
                RW_SWAP(int, v->ptr[i], v->ptr[i + 1])
            } else {
                break;
            }
        }
}

static inline void Vi_Fill(vi *v, int n, int fill) {
    int i;
    Vi_Shrink(v, 0);
    for (i = 0; i < n; i++)
        Vi_Push(v, fill);
}

static inline int Vi_Drop(vi *v, int i) {
    assert(i >= 0 && i < v->size);
    int Entry = v->ptr[i];
    for (; i < v->size - 1; i++)
        v->ptr[i] = v->ptr[i + 1];
    Vi_Shrink(v, v->size - 1);
    return Entry;
}

static inline int Vi_Find(vi *v, int e) {
    int j;
    for (j = 0; j < v->size; j++)
        if (v->ptr[j] == e)
            return j;
    return -1;
}

static inline int Vi_Remove(vi *v, int e) {
    int j = Vi_Find(v, e);
    if (j == -1)
        return 0;
    Vi_Drop(v, j);
    return 1;
}

static inline void Vi_Randomize(vi *v) {
    for (int i = 0; i < v->size; i++) {
        int iRand = Random_Num(0) % v->size;
        RW_SWAP(int, v->ptr[iRand], v->ptr[i]);
    }
}

static inline void Vi_Reverse(vi *v) {
    int i, j;
    for (i = 0, j = v->size - 1; i < j; i++, j--)
        RW_SWAP(int, v->ptr[i], v->ptr[j]);
}

static inline void Vi_Print(vi *v) {
    printf("Array with %d entries:", v->size);
    int i, entry;
    Vi_ForEachEntry(v, entry, i)
        printf(" %d", entry);
    printf("\n");
}

static inline void Vi_SelectSort(vi *v) {
    int *pArray = Vi_Array(v);
    int nSize = Vi_Size(v);
    int temp, i, j, best_i;
    for (i = 0; i < nSize - 1; i++) {
        best_i = i;
        for (j = i + 1; j < nSize; j++)
            if (pArray[j] < pArray[best_i])
                best_i = j;
        temp = pArray[i];
        pArray[i] = pArray[best_i];
        pArray[best_i] = temp;
    }
}

ABC_NAMESPACE_HEADER_END

#endif // RAR_VI_H