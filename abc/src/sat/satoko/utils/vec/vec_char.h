//===--- vec_char.h ---------------------------------------------------------===
//
//                     satoko: Satisfiability solver
//
// This file is distributed under the BSD 2-Clause License.
// See LICENSE for details.
//
//===------------------------------------------------------------------------===
#ifndef satoko__utils__vec__vec_char_h
#define satoko__utils__vec__vec_char_h

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../mem.h"

#include "misc/util/abc_global.h"
ABC_NAMESPACE_HEADER_START

typedef struct vec_char_t_ vec_char_t;
struct vec_char_t_ {
    unsigned cap;
    unsigned size;
    char *data;
};

//===------------------------------------------------------------------------===
// Vector Macros
//===------------------------------------------------------------------------===
#define vec_char_foreach(vec, entry, i) \
    for (i = 0; (i < vec_char_size(vec)) && (((entry) = vec_char_at(vec, i)), 1); i++)

#define vec_char_foreach_start(vec, entry, i, start) \
    for (i = start; (i < vec_char_size(vec)) && (((entry) = vec_char_at(vec, i)), 1); i++)

#define vec_char_foreach_stop(vec, entry, i, stop) \
    for (i = 0; (i < stop) && (((entry) = vec_char_at(vec, i)), 1); i++)

//===------------------------------------------------------------------------===
// Vector API
//===------------------------------------------------------------------------===
static inline vec_char_t * vec_char_alloc(unsigned cap)
{
    vec_char_t *p = satoko_alloc(vec_char_t, 1);

    if (cap > 0 && cap < 16)
        cap = 16;
    p->size = 0;
    p->cap = cap;
    p->data = p->cap ? satoko_alloc(char, p->cap) : NULL;
    return p;
}

static inline vec_char_t * vec_char_alloc_exact(unsigned cap)
{
    vec_char_t *p = satoko_alloc(vec_char_t, 1);

    cap = 0;
    p->size = 0;
    p->cap = cap;
    p->data = p->cap ? satoko_alloc(char, p->cap ) : NULL;
    return p;
}

static inline vec_char_t * vec_char_init(unsigned size, char value)
{
    vec_char_t *p = satoko_alloc(vec_char_t, 1);

    p->cap = size;
    p->size = size;
    p->data = p->cap ? satoko_alloc(char, p->cap) : NULL;
    memset(p->data, value, sizeof(char) * p->size);
    return p;
}

static inline void vec_char_free(vec_char_t *p)
{
    if (p->data != NULL)
        satoko_free(p->data);
    satoko_free(p);
}

static inline unsigned vec_char_size(vec_char_t *p)
{
    return p->size;
}

static inline void vec_char_resize(vec_char_t *p, unsigned new_size)
{
    p->size = new_size;
    if (p->cap >= new_size)
        return;
    p->data = satoko_realloc(char, p->data, new_size);
    assert(p->data != NULL);
    p->cap = new_size;
}

static inline void vec_char_shrink(vec_char_t *p, unsigned new_size)
{
    assert(p->cap > new_size);
    p->size = new_size;
}

static inline void vec_char_reserve(vec_char_t *p, unsigned new_cap)
{
    if (p->cap >= new_cap)
        return;
    p->data = satoko_realloc(char, p->data, new_cap);
    assert(p->data != NULL);
    p->cap = new_cap;
}

static inline unsigned vec_char_capacity(vec_char_t *p)
{
    return p->cap;
}

static inline int vec_char_empty(vec_char_t *p)
{
    return p->size ? 0 : 1;
}

static inline void vec_char_erase(vec_char_t *p)
{
    satoko_free(p->data);
    p->size = 0;
    p->cap = 0;
}

static inline char vec_char_at(vec_char_t *p, unsigned idx)
{
    assert(idx >= 0 && idx < p->size);
    return p->data[idx];
}

static inline char * vec_char_at_ptr(vec_char_t *p, unsigned idx)
{
    assert(idx >= 0 && idx < p->size);
    return p->data + idx;
}

static inline char * vec_char_data(vec_char_t *p)
{
    assert(p);
    return p->data;
}

static inline void vec_char_duplicate(vec_char_t *dest, const vec_char_t *src)
{
    assert(dest != NULL && src != NULL);
    vec_char_resize(dest, src->cap);
    memcpy(dest->data, src->data, sizeof(char) * src->cap);
    dest->size = src->size;
}

static inline void vec_char_copy(vec_char_t *dest, const vec_char_t *src)
{
    assert(dest != NULL && src != NULL);
    vec_char_resize(dest, src->size);
    memcpy(dest->data, src->data, sizeof(char) * src->size);
    dest->size = src->size;
}

static inline void vec_char_push_back(vec_char_t *p, char value)
{
    if (p->size == p->cap) {
        if (p->cap < 16)
            vec_char_reserve(p, 16);
        else
            vec_char_reserve(p, 2 * p->cap);
    }
    p->data[p->size] = value;
    p->size++;
}

static inline char vec_char_pop_back(vec_char_t *p)
{
    assert(p && p->size);
    return p->data[--p->size];
}

static inline void vec_char_assign(vec_char_t *p, unsigned idx, char value)
{
    assert((idx >= 0) && (idx < vec_char_size(p)));
    p->data[idx] = value;
}

static inline void vec_char_insert(vec_char_t *p, unsigned idx, char value)
{
    assert((idx >= 0) && (idx < vec_char_size(p)));
    vec_char_push_back(p, 0);
    memmove(p->data + idx + 1, p->data + idx, (p->size - idx - 2) * sizeof(char));
    p->data[idx] = value;
}

static inline void vec_char_drop(vec_char_t *p, unsigned idx)
{
    assert((idx >= 0) && (idx < vec_char_size(p)));
    memmove(p->data + idx, p->data + idx + 1, (p->size - idx - 1) * sizeof(char));
    p->size -= 1;
}

static inline void vec_char_clear(vec_char_t *p)
{
    p->size = 0;
}

static inline int vec_char_asc_compare(const void *p1, const void *p2)
{
    const char *pp1 = (const char *)p1;
    const char *pp2 = (const char *)p2;

    if (*pp1 < *pp2)
        return -1;
    if (*pp1 > *pp2)
        return 1;
    return 0;
}

static inline int vec_char_desc_compare(const void *p1, const void *p2)
{
    const char *pp1 = (const char *)p1;
    const char *pp2 = (const char *)p2;

    if (*pp1 > *pp2)
        return -1;
    if (*pp1 < *pp2)
        return 1;
    return 0;
}

static inline void vec_char_sort(vec_char_t *p, int ascending)
{
    if (ascending)
        qsort((void *) p->data, p->size, sizeof(char),
              (int (*)(const void *, const void *)) vec_char_asc_compare);
    else
        qsort((void*) p->data, p->size, sizeof(char),
              (int (*)(const void *, const void *)) vec_char_desc_compare);
}


static inline long vec_char_memory(vec_char_t *p)
{
    return p == NULL ? 0 : sizeof(char) * p->cap + sizeof(vec_char_t);
}

static inline void vec_char_print(vec_char_t* p)
{
    unsigned i;
    assert(p != NULL);
    fprintf(stdout, "Vector has %u(%u) entries: {", p->size, p->cap);
    for (i = 0; i < p->size; i++)
        fprintf(stdout, " %d", p->data[i]);
    fprintf(stdout, " }\n");
}

ABC_NAMESPACE_HEADER_END
#endif /* satoko__utils__vec__vec_char_h */
