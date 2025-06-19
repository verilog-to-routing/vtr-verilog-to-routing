//===--- clause.h -----------------------------------------------------------===
//
//                     satoko: Satisfiability solver
//
// This file is distributed under the BSD 2-Clause License.
// See LICENSE for details.
//
//===------------------------------------------------------------------------===
#ifndef satoko__clause_h
#define satoko__clause_h

#include "types.h"

#include "misc/util/abc_global.h"
ABC_NAMESPACE_HEADER_START

struct clause {
    unsigned f_learnt    : 1;
    unsigned f_mark      : 1;
    unsigned f_reallocd  : 1;
    unsigned f_deletable : 1;
    unsigned lbd : 28;
    unsigned size;
    union {
        unsigned lit;
        clause_act_t act;
    } data[0];
};

//===------------------------------------------------------------------------===
// Clause API
//===------------------------------------------------------------------------===
static inline int clause_compare(const void *p1, const void *p2)
{
    const struct clause *c1 = (const struct clause *)p1;
    const struct clause *c2 = (const struct clause *)p2;

    if (c1->size > 2 && c2->size == 2)
        return 1;
    if (c1->size == 2 && c2->size > 2)
        return 0;
    if (c1->size == 2 && c2->size == 2)
        return 0;

    if (c1->lbd > c2->lbd)
        return 1;
    if (c1->lbd < c2->lbd)
        return 0;

    return c1->data[c1->size].act < c2->data[c2->size].act;
}

static inline void clause_print(struct clause *clause)
{
    unsigned i;
    printf("{ ");
    for (i = 0; i < clause->size; i++)
        printf("%u ", clause->data[i].lit);
    printf("}\n");
}

static inline void clause_dump(FILE *file, struct clause *clause, int no_zero_var) 
{
    unsigned i;
    for (i = 0; i < clause->size; i++) {
        int var = (clause->data[i].lit >> 1);
        char pol = (clause->data[i].lit & 1);
        fprintf(file, "%d ", pol ? -(var + no_zero_var) : (var + no_zero_var));
    }
    if (no_zero_var)
        fprintf(file, "0\n");
    else
        fprintf(file, "\n");
}

ABC_NAMESPACE_HEADER_END
#endif /* satoko__clause_h */
