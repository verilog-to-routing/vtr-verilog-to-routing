//===--- vec_int.h ----------------------------------------------------------===
//
//                     satoko: Satisfiability solver
//
// This file is distributed under the BSD 2-Clause License.
// See LICENSE for details.
//
//===------------------------------------------------------------------------===
#ifndef satoko__utils__vec__vec_flt_h
#define satoko__utils__vec__vec_flt_h

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../mem.h"

#include "misc/util/abc_global.h"
ABC_NAMESPACE_HEADER_START

typedef struct vec_flt_t_ vec_flt_t;
struct vec_flt_t_ {
    unsigned cap;
    unsigned size;
    float *data;
};

//===------------------------------------------------------------------------===
// Vector Macros
//===------------------------------------------------------------------------===
#define vec_flt_foreach(vec, entry, i) \
    for (i = 0; (i < vec->size) && (((entry) = vec_flt_at(vec, i)), 1); i++)

#define vec_flt_foreach_start(vec, entry, i, start) \
    for (i = start; (i < vec_flt_size(vec)) && (((entry) = vec_flt_at(vec, i)), 1); i++)

#define vec_flt_foreach_stop(vec, entry, i, stop) \
    for (i = 0; (i < stop) && (((entry) = vec_flt_at(vec, i)), 1); i++)

//===------------------------------------------------------------------------===
// Vector API
//===------------------------------------------------------------------------===
static inline vec_flt_t *vec_flt_alloc(unsigned cap)
{
    vec_flt_t* p = satoko_alloc(vec_flt_t, 1);

    if (cap > 0 && cap < 16)
        cap = 16;
    p->size = 0;
    p->cap = cap;
    p->data = p->cap ? satoko_alloc(float, p->cap) : NULL;
    return p;
}

static inline vec_flt_t *vec_flt_alloc_exact(unsigned cap)
{
    vec_flt_t* p = satoko_alloc(vec_flt_t, 1);

    p->size = 0;
    p->cap = cap;
    p->data = p->cap ? satoko_alloc(float, p->cap) : NULL;
    return p;
}

static inline vec_flt_t *vec_flt_init(unsigned size, float value)
{
    vec_flt_t* p = satoko_alloc(vec_flt_t, 1);

    p->cap = size;
    p->size = size;
    p->data = p->cap ? satoko_alloc(float, p->cap) : NULL;
    memset(p->data, value, sizeof(float) * p->size);
    return p;
}

static inline void vec_flt_free(vec_flt_t *p)
{
    if (p->data != NULL)
        satoko_free(p->data);
    satoko_free(p);
}

static inline unsigned vec_flt_size(vec_flt_t *p)
{
    return p->size;
}

static inline void vec_flt_resize(vec_flt_t *p, unsigned new_size)
{
    p->size = new_size;
    if (p->cap >= new_size)
        return;
    p->data = satoko_realloc(float, p->data, new_size);
    assert(p->data != NULL);
    p->cap = new_size;
}

static inline void vec_flt_reserve(vec_flt_t *p, unsigned new_cap)
{
    if (p->cap >= new_cap)
        return;
    p->data = satoko_realloc(float, p->data, new_cap);
    assert(p->data != NULL);
    p->cap = new_cap;
}

static inline unsigned vec_flt_capacity(vec_flt_t *p)
{
    return p->cap;
}

static inline int vec_flt_empty(vec_flt_t *p)
{
    return p->size ? 0 : 1;
}

static inline void vec_flt_erase(vec_flt_t *p)
{
    satoko_free(p->data);
    p->size = 0;
    p->cap = 0;
}

static inline float vec_flt_at(vec_flt_t *p, unsigned i)
{
    assert(i >= 0 && i < p->size);
    return p->data[i];
}

static inline float *vec_flt_at_ptr(vec_flt_t *p, unsigned i)
{
    assert(i >= 0 && i < p->size);
    return p->data + i;
}

static inline float *vec_flt_data(vec_flt_t *p)
{
    assert(p);
    return p->data;
}

static inline void vec_flt_duplicate(vec_flt_t *dest, const vec_flt_t *src)
{
    assert(dest != NULL && src != NULL);
    vec_flt_resize(dest, src->cap);
    memcpy(dest->data, src->data, sizeof(float) * src->cap);
    dest->size = src->size;
}

static inline void vec_flt_copy(vec_flt_t *dest, const vec_flt_t *src)
{
    assert(dest != NULL && src != NULL);
    vec_flt_resize(dest, src->size);
    memcpy(dest->data, src->data, sizeof(float) * src->size);
    dest->size = src->size;
}

static inline void vec_flt_push_back(vec_flt_t *p, float value)
{
    if (p->size == p->cap) {
        if (p->cap < 16)
            vec_flt_reserve(p, 16);
        else
            vec_flt_reserve(p, 2 * p->cap);
    }
    p->data[p->size] = value;
    p->size++;
}

static inline void vec_flt_assign(vec_flt_t *p, unsigned i, float value)
{
    assert((i >= 0) && (i < vec_flt_size(p)));
    p->data[i] = value;
}

static inline void vec_flt_insert(vec_flt_t *p, unsigned i, float value)
{
    assert((i >= 0) && (i < vec_flt_size(p)));
    vec_flt_push_back(p, 0);
    memmove(p->data + i + 1, p->data + i, (p->size - i - 2) * sizeof(float));
    p->data[i] = value;
}

static inline void vec_flt_drop(vec_flt_t *p, unsigned i)
{
    assert((i >= 0) && (i < vec_flt_size(p)));
    memmove(p->data + i, p->data + i + 1, (p->size - i - 1) * sizeof(float));
    p->size -= 1;
}

static inline void vec_flt_clear(vec_flt_t *p)
{
    p->size = 0;
}

static inline int vec_flt_asc_compare(const void *p1, const void *p2)
{
    const float *pp1 = (const float *) p1;
    const float *pp2 = (const float *) p2;

    if (*pp1 < *pp2)
        return -1;
    if (*pp1 > *pp2)
        return 1;
    return 0;
}

static inline int vec_flt_desc_compare(const void* p1, const void* p2)
{
    const float *pp1 = (const float *) p1;
    const float *pp2 = (const float *) p2;

    if (*pp1 > *pp2)
        return -1;
    if (*pp1 < *pp2)
        return 1;
    return 0;
}

static inline void vec_flt_sort(vec_flt_t* p, int ascending)
{
    if (ascending)
        qsort((void *) p->data, p->size, sizeof(float),
              (int (*)(const void*, const void*)) vec_flt_asc_compare);
    else
        qsort((void *) p->data, p->size, sizeof(float),
              (int (*)(const void*, const void*)) vec_flt_desc_compare);
}

static inline long vec_flt_memory(vec_flt_t *p)
{
    return p == NULL ? 0 : sizeof(float) * p->cap + sizeof(vec_flt_t);
}

static inline void vec_flt_print(vec_flt_t *p)
{
    unsigned i;
    assert(p != NULL);
    fprintf(stdout, "Vector has %u(%u) entries: {", p->size, p->cap);
    for (i = 0; i < p->size; i++)
        fprintf(stdout, " %f", p->data[i]);
    fprintf(stdout, " }\n");
}

ABC_NAMESPACE_HEADER_END
#endif /* satoko__utils__vec__vec_flt_h */
