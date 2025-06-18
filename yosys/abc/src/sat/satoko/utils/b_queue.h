//===--- b_queue.h ----------------------------------------------------------===
//
//                     satoko: Satisfiability solver
//
// This file is distributed under the BSD 2-Clause License.
// See LICENSE for details.
//
//===------------------------------------------------------------------------===
#ifndef satoko__utils__b_queue_h
#define satoko__utils__b_queue_h

#include "mem.h"

#include "misc/util/abc_global.h"
ABC_NAMESPACE_HEADER_START

/* Bounded Queue */
typedef struct b_queue_t_ b_queue_t;
struct b_queue_t_ {
    unsigned size;
    unsigned cap;
    unsigned i_first;
    unsigned i_empty;
    unsigned long sum;
    unsigned *data;
};

//===------------------------------------------------------------------------===
// Bounded Queue API
//===------------------------------------------------------------------------===
static inline b_queue_t *b_queue_alloc(unsigned cap)
{
    b_queue_t *p = satoko_calloc(b_queue_t, 1);
    p->cap = cap;
    p->data = satoko_calloc(unsigned, cap);
    return p;
}

static inline void b_queue_free(b_queue_t *p)
{
    satoko_free(p->data);
    satoko_free(p);
}

static inline void b_queue_push(b_queue_t *p, unsigned Value)
{
    if (p->size == p->cap) {
        assert(p->i_first == p->i_empty);
        p->sum -= p->data[p->i_first];
        p->i_first = (p->i_first + 1) % p->cap;
    } else
        p->size++;

    p->sum += Value;
    p->data[p->i_empty] = Value;
    if ((++p->i_empty) == p->cap) {
        p->i_empty = 0;
        p->i_first = 0;
    }
}

static inline unsigned b_queue_avg(b_queue_t *p)
{
    return (unsigned)(p->sum / ((unsigned long) p->size));
}

static inline unsigned b_queue_is_valid(b_queue_t *p)
{
    return (p->cap == p->size);
}

static inline void b_queue_clean(b_queue_t *p)
{
    p->i_first = 0;
    p->i_empty = 0;
    p->size = 0;
    p->sum = 0;
}

ABC_NAMESPACE_HEADER_END
#endif /* satoko__utils__b_queue_h */
