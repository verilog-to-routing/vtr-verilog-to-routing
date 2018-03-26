//===--- vec_int.h ----------------------------------------------------------===
//
//                     satoko: Satisfiability solver
//
// This file is distributed under the BSD 2-Clause License.
// See LICENSE for details.
//
//===------------------------------------------------------------------------===
#ifndef satoko__utils__vec__vec_int_h
#define satoko__utils__vec__vec_int_h

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../mem.h"

#include "misc/util/abc_global.h"
ABC_NAMESPACE_HEADER_START

typedef struct vec_int_t_ vec_int_t;
struct vec_int_t_ {
    unsigned cap;
    unsigned size;
    int *data;
};

//===------------------------------------------------------------------------===
// Vector Macros
//===------------------------------------------------------------------------===
#define vec_int_foreach(vec, entry, i) \
    for (i = 0; (i < vec->size) && (((entry) = vec_int_at(vec, i)), 1); i++)

#define vec_int_foreach_start(vec, entry, i, start) \
    for (i = start; (i < vec_int_size(vec)) && (((entry) = vec_int_at(vec, i)), 1); i++)

#define vec_int_foreach_stop(vec, entry, i, stop) \
    for (i = 0; (i < stop) && (((entry) = vec_int_at(vec, i)), 1); i++)

//===------------------------------------------------------------------------===
// Vector API
//===------------------------------------------------------------------------===
static inline vec_int_t *vec_int_alloc(unsigned cap)
{
    vec_int_t* p = satoko_alloc(vec_int_t, 1);

    if (cap > 0 && cap < 16)
        cap = 16;
    p->size = 0;
    p->cap = cap;
    p->data = p->cap ? satoko_alloc(int, p->cap) : NULL;
    return p;
}

static inline vec_int_t *vec_int_alloc_exact(unsigned cap)
{
    vec_int_t* p = satoko_alloc(vec_int_t, 1);

    p->size = 0;
    p->cap = cap;
    p->data = p->cap ? satoko_alloc(int, p->cap) : NULL;
    return p;
}

static inline vec_int_t *vec_int_init(unsigned size, int value)
{
    vec_int_t* p = satoko_alloc(vec_int_t, 1);

    p->cap = size;
    p->size = size;
    p->data = p->cap ? satoko_alloc(int, p->cap) : NULL;
    memset(p->data, value, sizeof(int) * p->size);
    return p;
}

static inline void vec_int_free(vec_int_t *p)
{
    if (p->data != NULL)
        satoko_free(p->data);
    satoko_free(p);
}

static inline unsigned vec_int_size(vec_int_t *p)
{
    return p->size;
}

static inline void vec_int_resize(vec_int_t *p, unsigned new_size)
{
    p->size = new_size;
    if (p->cap >= new_size)
        return;
    p->data = satoko_realloc(int, p->data, new_size);
    assert(p->data != NULL);
    p->cap = new_size;
}

static inline void vec_int_reserve(vec_int_t *p, unsigned new_cap)
{
    if (p->cap >= new_cap)
        return;
    p->data = satoko_realloc(int, p->data, new_cap);
    assert(p->data != NULL);
    p->cap = new_cap;
}

static inline unsigned vec_int_capacity(vec_int_t *p)
{
    return p->cap;
}

static inline int vec_int_empty(vec_int_t *p)
{
    return p->size ? 0 : 1;
}

static inline void vec_int_erase(vec_int_t *p)
{
    satoko_free(p->data);
    p->size = 0;
    p->cap = 0;
}

static inline int vec_int_at(vec_int_t *p, unsigned i)
{
    assert(i >= 0 && i < p->size);
    return p->data[i];
}

static inline int *vec_int_at_ptr(vec_int_t *p, unsigned i)
{
    assert(i >= 0 && i < p->size);
    return p->data + i;
}

static inline void vec_int_duplicate(vec_int_t *dest, const vec_int_t *src)
{
    assert(dest != NULL && src != NULL);
    vec_int_resize(dest, src->cap);
    memcpy(dest->data, src->data, sizeof(int) * src->cap);
    dest->size = src->size;
}

static inline void vec_int_copy(vec_int_t *dest, const vec_int_t *src)
{
    assert(dest != NULL && src != NULL);
    vec_int_resize(dest, src->size);
    memcpy(dest->data, src->data, sizeof(int) * src->size);
    dest->size = src->size;
}

static inline void vec_int_push_back(vec_int_t *p, int value)
{
    if (p->size == p->cap) {
        if (p->cap < 16)
            vec_int_reserve(p, 16);
        else
            vec_int_reserve(p, 2 * p->cap);
    }
    p->data[p->size] = value;
    p->size++;
}

static inline void vec_int_assign(vec_int_t *p, unsigned i, int value)
{
    assert((i >= 0) && (i < vec_int_size(p)));
    p->data[i] = value;
}

static inline void vec_int_insert(vec_int_t *p, unsigned i, int value)
{
    assert((i >= 0) && (i < vec_int_size(p)));
    vec_int_push_back(p, 0);
    memmove(p->data + i + 1, p->data + i, (p->size - i - 2) * sizeof(int));
    p->data[i] = value;
}

static inline void vec_int_drop(vec_int_t *p, unsigned i)
{
    assert((i >= 0) && (i < vec_int_size(p)));
    memmove(p->data + i, p->data + i + 1, (p->size - i - 1) * sizeof(int));
    p->size -= 1;
}

static inline void vec_int_clear(vec_int_t *p)
{
    p->size = 0;
}

static inline int vec_int_asc_compare(const void *p1, const void *p2)
{
    const int *pp1 = (const int *) p1;
    const int *pp2 = (const int *) p2;

    if (*pp1 < *pp2)
        return -1;
    if (*pp1 > *pp2)
        return 1;
    return 0;
}

static inline int vec_int_desc_compare(const void* p1, const void* p2)
{
    const int *pp1 = (const int *) p1;
    const int *pp2 = (const int *) p2;

    if (*pp1 > *pp2)
        return -1;
    if (*pp1 < *pp2)
        return 1;
    return 0;
}

static inline void vec_int_sort(vec_int_t* p, int ascending)
{
    if (ascending)
        qsort((void *) p->data, p->size, sizeof(int),
              (int (*)(const void*, const void*)) vec_int_asc_compare);
    else
        qsort((void *) p->data, p->size, sizeof(int),
              (int (*)(const void*, const void*)) vec_int_desc_compare);
}

static inline long vec_int_memory(vec_int_t *p)
{
    return p == NULL ? 0 : sizeof(int) * p->cap + sizeof(vec_int_t);
}

static inline void vec_int_print(vec_int_t *p)
{
    unsigned i;
    assert(p != NULL);
    fprintf(stdout, "Vector has %u(%u) entries: {", p->size, p->cap);
    for (i = 0; i < p->size; i++)
        fprintf(stdout, " %d", p->data[i]);
    fprintf(stdout, " }\n");
}

ABC_NAMESPACE_HEADER_END
#endif /* satoko__utils__vec__vec_int_h */
