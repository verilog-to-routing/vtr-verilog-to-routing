//===--- vec_int.h ----------------------------------------------------------===
//
//                     satoko: Satisfiability solver
//
// This file is distributed under the BSD 2-Clause License.
// See LICENSE for details.
//
//===------------------------------------------------------------------------===
#ifndef satoko__utils__vec__vec_sdbl_h
#define satoko__utils__vec__vec_sdbl_h

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../mem.h"
#include "../sdbl.h"

#include "misc/util/abc_global.h"
ABC_NAMESPACE_HEADER_START

typedef struct vec_sdbl_t_ vec_sdbl_t;
struct vec_sdbl_t_ {
    unsigned cap;
    unsigned size;
    sdbl_t *data;
};

//===------------------------------------------------------------------------===
// Vector Macros
//===------------------------------------------------------------------------===
#define vec_sdbl_foreach(vec, entry, i) \
    for (i = 0; (i < vec->size) && (((entry) = vec_sdbl_at(vec, i)), 1); i++)

#define vec_sdbl_foreach_start(vec, entry, i, start) \
    for (i = start; (i < vec_sdbl_size(vec)) && (((entry) = vec_sdbl_at(vec, i)), 1); i++)

#define vec_sdbl_foreach_stop(vec, entry, i, stop) \
    for (i = 0; (i < stop) && (((entry) = vec_sdbl_at(vec, i)), 1); i++)

//===------------------------------------------------------------------------===
// Vector API
//===------------------------------------------------------------------------===
static inline vec_sdbl_t *vec_sdbl_alloc(unsigned cap)
{
    vec_sdbl_t* p = satoko_alloc(vec_sdbl_t, 1);

    if (cap > 0 && cap < 16)
        cap = 16;
    p->size = 0;
    p->cap = cap;
    p->data = p->cap ? satoko_alloc(sdbl_t, p->cap) : NULL;
    return p;
}

static inline vec_sdbl_t *vec_sdbl_alloc_exact(unsigned cap)
{
    vec_sdbl_t* p = satoko_alloc(vec_sdbl_t, 1);

    p->size = 0;
    p->cap = cap;
    p->data = p->cap ? satoko_alloc(sdbl_t, p->cap) : NULL;
    return p;
}

static inline vec_sdbl_t *vec_sdbl_init(unsigned size, sdbl_t value)
{
    vec_sdbl_t* p = satoko_alloc(vec_sdbl_t, 1);

    p->cap = size;
    p->size = size;
    p->data = p->cap ? satoko_alloc(sdbl_t, p->cap) : NULL;
    memset(p->data, value, sizeof(sdbl_t) * p->size);
    return p;
}

static inline void vec_sdbl_free(vec_sdbl_t *p)
{
    if (p->data != NULL)
        satoko_free(p->data);
    satoko_free(p);
}

static inline unsigned vec_sdbl_size(vec_sdbl_t *p)
{
    return p->size;
}

static inline void vec_sdbl_shrink(vec_sdbl_t *p, unsigned new_size)
{
    assert(new_size <= p->cap);
    p->size = new_size;
}

static inline void vec_sdbl_resize(vec_sdbl_t *p, unsigned new_size)
{
    p->size = new_size;
    if (p->cap >= new_size)
        return;
    p->data = satoko_realloc(sdbl_t, p->data, new_size);
    assert(p->data != NULL);
    p->cap = new_size;
}

static inline void vec_sdbl_reserve(vec_sdbl_t *p, unsigned new_cap)
{
    if (p->cap >= new_cap)
        return;
    p->data = satoko_realloc(sdbl_t, p->data, new_cap);
    assert(p->data != NULL);
    p->cap = new_cap;
}

static inline unsigned vec_sdbl_capacity(vec_sdbl_t *p)
{
    return p->cap;
}

static inline int vec_sdbl_empty(vec_sdbl_t *p)
{
    return p->size ? 0 : 1;
}

static inline void vec_sdbl_erase(vec_sdbl_t *p)
{
    satoko_free(p->data);
    p->size = 0;
    p->cap = 0;
}

static inline sdbl_t vec_sdbl_at(vec_sdbl_t *p, unsigned i)
{
    assert(i >= 0 && i < p->size);
    return p->data[i];
}

static inline sdbl_t *vec_sdbl_at_ptr(vec_sdbl_t *p, unsigned i)
{
    assert(i >= 0 && i < p->size);
    return p->data + i;
}

static inline sdbl_t *vec_sdbl_data(vec_sdbl_t *p)
{
    assert(p);
    return p->data;
}

static inline void vec_sdbl_duplicate(vec_sdbl_t *dest, const vec_sdbl_t *src)
{
    assert(dest != NULL && src != NULL);
    vec_sdbl_resize(dest, src->cap);
    memcpy(dest->data, src->data, sizeof(sdbl_t) * src->cap);
    dest->size = src->size;
}

static inline void vec_sdbl_copy(vec_sdbl_t *dest, const vec_sdbl_t *src)
{
    assert(dest != NULL && src != NULL);
    vec_sdbl_resize(dest, src->size);
    memcpy(dest->data, src->data, sizeof(sdbl_t) * src->size);
    dest->size = src->size;
}

static inline void vec_sdbl_push_back(vec_sdbl_t *p, sdbl_t value)
{
    if (p->size == p->cap) {
        if (p->cap < 16)
            vec_sdbl_reserve(p, 16);
        else
            vec_sdbl_reserve(p, 2 * p->cap);
    }
    p->data[p->size] = value;
    p->size++;
}

static inline void vec_sdbl_assign(vec_sdbl_t *p, unsigned i, sdbl_t value)
{
    assert((i >= 0) && (i < vec_sdbl_size(p)));
    p->data[i] = value;
}

static inline void vec_sdbl_insert(vec_sdbl_t *p, unsigned i, sdbl_t value)
{
    assert((i >= 0) && (i < vec_sdbl_size(p)));
    vec_sdbl_push_back(p, 0);
    memmove(p->data + i + 1, p->data + i, (p->size - i - 2) * sizeof(sdbl_t));
    p->data[i] = value;
}

static inline void vec_sdbl_drop(vec_sdbl_t *p, unsigned i)
{
    assert((i >= 0) && (i < vec_sdbl_size(p)));
    memmove(p->data + i, p->data + i + 1, (p->size - i - 1) * sizeof(sdbl_t));
    p->size -= 1;
}

static inline void vec_sdbl_clear(vec_sdbl_t *p)
{
    p->size = 0;
}

static inline int vec_sdbl_asc_compare(const void *p1, const void *p2)
{
    const sdbl_t *pp1 = (const sdbl_t *) p1;
    const sdbl_t *pp2 = (const sdbl_t *) p2;

    if (*pp1 < *pp2)
        return -1;
    if (*pp1 > *pp2)
        return 1;
    return 0;
}

static inline int vec_sdbl_desc_compare(const void* p1, const void* p2)
{
    const sdbl_t *pp1 = (const sdbl_t *) p1;
    const sdbl_t *pp2 = (const sdbl_t *) p2;

    if (*pp1 > *pp2)
        return -1;
    if (*pp1 < *pp2)
        return 1;
    return 0;
}

static inline void vec_sdbl_sort(vec_sdbl_t* p, int ascending)
{
    if (ascending)
        qsort((void *) p->data, (size_t)p->size, sizeof(sdbl_t),
              (int (*)(const void*, const void*)) vec_sdbl_asc_compare);
    else
        qsort((void *) p->data, (size_t)p->size, sizeof(sdbl_t),
              (int (*)(const void*, const void*)) vec_sdbl_desc_compare);
}

static inline long vec_sdbl_memory(vec_sdbl_t *p)
{
    return p == NULL ? 0 : sizeof(sdbl_t) * p->cap + sizeof(vec_sdbl_t);
}

static inline void vec_sdbl_print(vec_sdbl_t *p)
{
    unsigned i;
    assert(p != NULL);
    fprintf(stdout, "Vector has %u(%u) entries: {", p->size, p->cap);
    for (i = 0; i < p->size; i++)
        fprintf(stdout, " %f", sdbl2double(p->data[i]));
    fprintf(stdout, " }\n");
}

ABC_NAMESPACE_HEADER_END
#endif /* satoko__utils__vec__vec_sdbl_h */
