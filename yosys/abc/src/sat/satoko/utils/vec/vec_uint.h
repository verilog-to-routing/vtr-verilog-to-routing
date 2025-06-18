//===--- vec_uint.h ---------------------------------------------------------===
//
//                     satoko: Satisfiability solver
//
// This file is distributed under the BSD 2-Clause License.
// See LICENSE for details.
//
//===------------------------------------------------------------------------===
#ifndef satoko__utils__vec__vec_uint_h
#define satoko__utils__vec__vec_uint_h

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../mem.h"

#include "misc/util/abc_global.h"
ABC_NAMESPACE_HEADER_START

typedef struct vec_uint_t_ vec_uint_t;
struct vec_uint_t_ {
    unsigned cap;
    unsigned size;
    unsigned* data;
};

//===------------------------------------------------------------------------===
// Vector Macros
//===------------------------------------------------------------------------===
#define vec_uint_foreach(vec, entry, i) \
    for (i = 0; (i < vec_uint_size(vec)) && (((entry) = vec_uint_at(vec, i)), 1); i++)

#define vec_uint_foreach_start(vec, entry, i, start) \
    for (i = start; (i < vec_uint_size(vec)) && (((entry) = vec_uint_at(vec, i)), 1); i++)

#define vec_uint_foreach_stop(vec, entry, i, stop) \
    for (i = 0; (i < stop) && (((entry) = vec_uint_at(vec, i)), 1); i++)

//===------------------------------------------------------------------------===
// Vector API
//===------------------------------------------------------------------------===
static inline vec_uint_t * vec_uint_alloc(unsigned cap)
{
    vec_uint_t *p = satoko_alloc(vec_uint_t, 1);

    if (cap > 0 && cap < 16)
        cap = 16;
    p->size = 0;
    p->cap = cap;
    p->data = p->cap ? satoko_alloc(unsigned, p->cap) : NULL;
    return p;
}

static inline vec_uint_t * vec_uint_alloc_exact(unsigned cap)
{
    vec_uint_t *p = satoko_alloc(vec_uint_t, 1);

    cap = 0;
    p->size = 0;
    p->cap = cap;
    p->data = p->cap ? satoko_alloc(unsigned, p->cap ) : NULL;
    return p;
}

static inline vec_uint_t * vec_uint_init(unsigned size, unsigned value)
{
    vec_uint_t *p = satoko_alloc(vec_uint_t, 1);

    p->cap = size;
    p->size = size;
    p->data = p->cap ? satoko_alloc(unsigned, p->cap ) : NULL;
    memset(p->data, value, sizeof(unsigned) * p->size);
    return p;
}

static inline void vec_uint_free(vec_uint_t *p)
{
    if (p->data != NULL)
        satoko_free(p->data);
    satoko_free(p);
}

static inline unsigned vec_uint_size(vec_uint_t *p)
{
    return p->size;
}

static inline void vec_uint_resize(vec_uint_t *p, unsigned new_size)
{
    p->size = new_size;
    if (p->cap >= new_size)
        return;
    p->data = satoko_realloc(unsigned, p->data, new_size);
    assert(p->data != NULL);
    p->cap = new_size;
}

static inline void vec_uint_shrink(vec_uint_t *p, unsigned new_size)
{
    assert(p->cap >= new_size);
    p->size = new_size;
}

static inline void vec_uint_reserve(vec_uint_t *p, unsigned new_cap)
{
    if (p->cap >= new_cap)
        return;
    p->data = satoko_realloc(unsigned, p->data, new_cap);
    assert(p->data != NULL);
    p->cap = new_cap;
}

static inline unsigned vec_uint_capacity(vec_uint_t *p)
{
    return p->cap;
}

static inline int vec_uint_empty(vec_uint_t *p)
{
    return p->size ? 0 : 1;
}

static inline void vec_uint_erase(vec_uint_t *p)
{
    satoko_free(p->data);
    p->size = 0;
    p->cap = 0;
}

static inline unsigned vec_uint_at(vec_uint_t *p, unsigned idx)
{
    assert(idx >= 0 && idx < p->size);
    return p->data[idx];
}

static inline unsigned * vec_uint_at_ptr(vec_uint_t *p, unsigned idx)
{
    assert(idx >= 0 && idx < p->size);
    return p->data + idx;
}

static inline unsigned vec_uint_find(vec_uint_t *p, unsigned entry)
{
    unsigned i;
    for (i = 0; i < p->size; i++)
        if (p->data[i] == entry)
            return 1;
    return 0;
}

static inline unsigned * vec_uint_data(vec_uint_t *p)
{
    assert(p);
    return p->data;
}

static inline void vec_uint_duplicate(vec_uint_t *dest, const vec_uint_t *src)
{
    assert(dest != NULL && src != NULL);
    vec_uint_resize(dest, src->cap);
    memcpy(dest->data, src->data, sizeof(unsigned) * src->cap);
    dest->size = src->size;
}

static inline void vec_uint_copy(vec_uint_t *dest, const vec_uint_t *src)
{
    assert(dest != NULL && src != NULL);
    vec_uint_resize(dest, src->size);
    memcpy(dest->data, src->data, sizeof(unsigned) * src->size);
    dest->size = src->size;
}

static inline void vec_uint_push_back(vec_uint_t *p, unsigned value)
{
    if (p->size == p->cap) {
        if (p->cap < 16)
            vec_uint_reserve(p, 16);
        else
            vec_uint_reserve(p, 2 * p->cap);
    }
    p->data[p->size] = value;
    p->size++;
}

static inline unsigned vec_uint_pop_back(vec_uint_t *p)
{
    assert(p && p->size);
    return p->data[--p->size];
}

static inline void vec_uint_assign(vec_uint_t *p, unsigned idx, unsigned value)
{
    assert((idx >= 0) && (idx < vec_uint_size(p)));
    p->data[idx] = value;
}

static inline void vec_uint_insert(vec_uint_t *p, unsigned idx, unsigned value)
{
    assert((idx >= 0) && (idx < vec_uint_size(p)));
    vec_uint_push_back(p, 0);
    memmove(p->data + idx + 1, p->data + idx, (p->size - idx - 2) * sizeof(unsigned));
    p->data[idx] = value;
}

static inline void vec_uint_drop(vec_uint_t *p, unsigned idx)
{
    assert((idx >= 0) && (idx < vec_uint_size(p)));
    memmove(p->data + idx, p->data + idx + 1, (p->size - idx - 1) * sizeof(unsigned));
    p->size -= 1;
}

static inline void vec_uint_clear(vec_uint_t *p)
{
    p->size = 0;
}

static inline int vec_uint_asc_compare(const void *p1, const void *p2)
{
    const unsigned *pp1 = (const unsigned *) p1;
    const unsigned *pp2 = (const unsigned *) p2;

    if ( *pp1 < *pp2 )
        return -1;
    if ( *pp1 > *pp2 )
        return 1;
    return 0;
}

static inline int vec_uint_desc_compare(const void *p1, const void *p2)
{
    const unsigned *pp1 = (const unsigned *) p1;
    const unsigned *pp2 = (const unsigned *) p2;

    if ( *pp1 > *pp2 )
        return -1;
    if ( *pp1 < *pp2 )
        return 1;
    return 0;
}

static inline void vec_uint_sort(vec_uint_t *p, int ascending)
{
    if (ascending)
        qsort((void *) p->data, (size_t)p->size, sizeof(unsigned),
              (int (*)(const void *, const void *)) vec_uint_asc_compare);
    else
        qsort((void*) p->data, (size_t)p->size, sizeof(unsigned),
              (int (*)(const void *, const void *)) vec_uint_desc_compare);
}

static inline long vec_uint_memory(vec_uint_t *p)
{
    return p == NULL ? 0 : sizeof(unsigned) * p->cap + sizeof(vec_uint_t);
}

static inline void vec_uint_print(vec_uint_t* p)
{
    unsigned i;
    assert(p != NULL);
    fprintf(stdout, "Vector has %u(%u) entries: {", p->size, p->cap);
    for (i = 0; i < p->size; i++)
        fprintf(stdout, " %u", p->data[i]);
    fprintf(stdout, " }\n");
}

ABC_NAMESPACE_HEADER_END
#endif /* satoko__utils__vec__vec_uint_h */
