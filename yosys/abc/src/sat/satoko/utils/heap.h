//===--- heap.h ----------------------------------------------------------===
//
//                     satoko: Satisfiability solver
//
// This file is distributed under the BSD 2-Clause License.
// See LICENSE for details.
//
//===------------------------------------------------------------------------===
#ifndef satoko__utils__heap_h
#define satoko__utils__heap_h

#include "mem.h"
#include "../types.h"
#include "vec/vec_sdbl.h"
#include "vec/vec_int.h"
#include "vec/vec_uint.h"

#include "misc/util/abc_global.h"
ABC_NAMESPACE_HEADER_START

typedef struct heap_t_ heap_t;
struct heap_t_ {
    vec_int_t *indices;
    vec_uint_t *data;
    vec_act_t *weights;
};
//===------------------------------------------------------------------------===
// Heap internal functions
//===------------------------------------------------------------------------===
static inline unsigned left(unsigned i) { return 2 * i + 1; }
static inline unsigned right(unsigned i) { return (i + 1) * 2; }
static inline unsigned parent(unsigned i) { return (i - 1) >> 1; }

static inline int compare(heap_t *p, unsigned x, unsigned y)
{
    return vec_act_at(p->weights, x) > vec_act_at(p->weights, y);
}

static inline void heap_percolate_up(heap_t *h, unsigned i)
{
    unsigned x = vec_uint_at(h->data, i);
    unsigned p = parent(i);

    while (i != 0 && compare(h, x, vec_uint_at(h->data, p))) {
        vec_uint_assign(h->data, i, vec_uint_at(h->data, p));
        vec_int_assign(h->indices, vec_uint_at(h->data, p), (int) i);
        i = p;
        p = parent(p);
    }
    vec_uint_assign(h->data, i, x);
    vec_int_assign(h->indices, x, (int) i);
}

static inline void heap_percolate_down(heap_t *h, unsigned i)
{
    unsigned x = vec_uint_at(h->data, i);

    while (left(i) < vec_uint_size(h->data)) {
        unsigned child = right(i) < vec_uint_size(h->data) &&
                     compare(h, vec_uint_at(h->data, right(i)), vec_uint_at(h->data, left(i)))
                   ? right(i)
                   : left(i);

        if (!compare(h, vec_uint_at(h->data, child), x))
            break;

        vec_uint_assign(h->data, i, vec_uint_at(h->data, child));
        vec_int_assign(h->indices, vec_uint_at(h->data, i), (int) i);
        i = child;
    }
    vec_uint_assign(h->data, i, x);
    vec_int_assign(h->indices, x, (int) i);
}

//===------------------------------------------------------------------------===
// Heap API
//===------------------------------------------------------------------------===
static inline heap_t *heap_alloc(vec_act_t *weights)
{
    heap_t *p = satoko_alloc(heap_t, 1);
    p->weights = weights;
    p->indices = vec_int_alloc(0);
    p->data = vec_uint_alloc(0);
    return p;
}

static inline void heap_free(heap_t *p)
{
    vec_int_free(p->indices);
    vec_uint_free(p->data);
    satoko_free(p);
}

static inline unsigned heap_size(heap_t *p)
{
    return vec_uint_size(p->data);
}

static inline int heap_in_heap(heap_t *p, unsigned entry)
{
    return (entry < vec_int_size(p->indices)) &&
           (vec_int_at(p->indices, entry) >= 0);
}

static inline void heap_increase(heap_t *p, unsigned entry)
{
    assert(heap_in_heap(p, entry));
    heap_percolate_down(p, (unsigned) vec_int_at(p->indices, entry));
}

static inline void heap_decrease(heap_t *p, unsigned entry)
{
    assert(heap_in_heap(p, entry));
    heap_percolate_up(p, (unsigned) vec_int_at(p->indices, entry));
}

static inline void heap_insert(heap_t *p, unsigned entry)
{
    if (vec_int_size(p->indices) < entry + 1) {
        unsigned old_size = vec_int_size(p->indices);
        unsigned i;
        int e;
        vec_int_resize(p->indices, entry + 1);
        vec_int_foreach_start(p->indices, e, i, old_size)
            vec_int_assign(p->indices, i, -1);
    }
    assert(!heap_in_heap(p, entry));
    vec_int_assign(p->indices, entry, (int) vec_uint_size(p->data));
    vec_uint_push_back(p->data, entry);
    heap_percolate_up(p, (unsigned) vec_int_at(p->indices, entry));
}

static inline void heap_update(heap_t *p, unsigned i)
{
    if (!heap_in_heap(p, i))
        heap_insert(p, i);
    else {
        heap_percolate_up(p, (unsigned) vec_int_at(p->indices, i));
        heap_percolate_down(p, (unsigned) vec_int_at(p->indices, i));
    }
}

static inline void heap_build(heap_t *p, vec_uint_t *entries)
{
    int i;
    unsigned j, entry;

    vec_uint_foreach(p->data, entry, j)
        vec_int_assign(p->indices, entry, -1);
    vec_uint_clear(p->data);
    vec_uint_foreach(entries, entry, j) {
        vec_int_assign(p->indices, entry, (int) j);
        vec_uint_push_back(p->data, entry);
    }
    for ((i = vec_uint_size(p->data) / 2 - 1); i >= 0; i--)
        heap_percolate_down(p, (unsigned) i);
}

static inline void heap_clear(heap_t *p)
{
    vec_int_clear(p->indices);
    vec_uint_clear(p->data);
}

static inline unsigned heap_remove_min(heap_t *p)
{
    unsigned x = vec_uint_at(p->data, 0);
    vec_uint_assign(p->data, 0, vec_uint_at(p->data, vec_uint_size(p->data) - 1));
    vec_int_assign(p->indices, vec_uint_at(p->data, 0), 0);
    vec_int_assign(p->indices, x, -1);
    vec_uint_pop_back(p->data);
    if (vec_uint_size(p->data) > 1)
        heap_percolate_down(p, 0);
    return x;
}

ABC_NAMESPACE_HEADER_END
#endif /* satoko__utils__heap_h */
