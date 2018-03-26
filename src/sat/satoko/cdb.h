//===--- cdb.h --------------------------------------------------------------===
//
//                     satoko: Satisfiability solver
//
// This file is distributed under the BSD 2-Clause License.
// See LICENSE for details.
//
//===------------------------------------------------------------------------===
#ifndef satoko__cdb_h
#define satoko__cdb_h

#include "clause.h"

#include "misc/util/abc_global.h"
ABC_NAMESPACE_HEADER_START

/* Clauses DB data structure */
struct cdb {
    unsigned size;
    unsigned cap;
    unsigned wasted;
    unsigned *data;
};

//===------------------------------------------------------------------------===
// Clause DB API
//===------------------------------------------------------------------------===
static inline struct clause *cdb_handler(struct cdb *p, unsigned cref)
{
    return cref != 0xFFFFFFFF ? (struct clause *)(p->data + cref) : NULL;
}

static inline unsigned cdb_cref(struct cdb *p, unsigned *clause)
{
    return (unsigned)(clause - &(p->data[0]));
}

static inline void cdb_grow(struct cdb *p, unsigned cap)
{
    unsigned prev_cap = p->cap;

    if (p->cap >= cap)
        return;
    while (p->cap < cap) {
        unsigned delta = ((p->cap >> 1) + (p->cap >> 3) + 2) & (unsigned)(~1);
        p->cap += delta;
        assert(p->cap >= prev_cap);
    }
    assert(p->cap > 0);
    p->data = satoko_realloc(unsigned, p->data, p->cap);
}

static inline struct cdb *cdb_alloc(unsigned cap)
{
    struct cdb *p = satoko_calloc(struct cdb, 1);
    if (cap <= 0)
        cap = 1024 * 1024;
    cdb_grow(p, cap);
    return p;
}

static inline void cdb_free(struct cdb *p)
{
    satoko_free(p->data);
    satoko_free(p);
}

static inline unsigned cdb_append(struct cdb *p, unsigned size)
{
    unsigned prev_size;
    assert(size > 0);
    cdb_grow(p, p->size + size);
    prev_size = p->size;
    p->size += size;
    assert(p->size > prev_size);
    return prev_size;
}

static inline void cdb_remove(struct cdb *p, struct clause *clause)
{
    p->wasted += clause->size;
}

static inline void cdb_clear(struct cdb *p)
{
    p->wasted = 0;
    p->size = 0;
}

static inline unsigned cdb_capacity(struct cdb *p)
{
    return p->cap;
}

static inline unsigned cdb_size(struct cdb *p)
{
    return p->size;
}

static inline unsigned cdb_wasted(struct cdb *p)
{
    return p->wasted;
}

ABC_NAMESPACE_HEADER_END
#endif /* satoko__cdb_h */
