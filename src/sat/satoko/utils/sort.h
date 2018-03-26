//===--- sort.h -------------------------------------------------------------===
//
//                     satoko: Satisfiability solver
//
// This file is distributed under the BSD 2-Clause License.
// See LICENSE for details.
//
//===------------------------------------------------------------------------===
#ifndef satoko__utils__sort_h
#define satoko__utils__sort_h

#include "misc/util/abc_global.h"
ABC_NAMESPACE_HEADER_START

static inline void select_sort(void **data, unsigned size,
                   int (*comp_fn)(const void *, const void *))
{
    unsigned i, j, i_best;
    void *temp;

    for (i = 0; i < (size - 1); i++) {
        i_best = i;
        for (j = i + 1; j < size; j++) {
            if (comp_fn(data[j], data[i_best]))
                i_best = j;
        }
        temp = data[i];
        data[i] = data[i_best];
        data[i_best] = temp;
    }
}

static void satoko_sort(void **data, unsigned size,
            int (*comp_fn)(const void *, const void *))
{
    if (size <= 15)
        select_sort(data, size, comp_fn);
    else {
        void *pivot = data[size / 2];
        void *temp;
        unsigned j = size;
        int i = -1;

        for (;;) {
            do {
                i++;
            } while (comp_fn(data[i], pivot));
            do {
                j--;
            } while (comp_fn(pivot, data[j]));

            if ((unsigned) i >= j)
                break;

            temp = data[i];
            data[i] = data[j];
            data[j] = temp;
        }
        satoko_sort(data, (unsigned) i, comp_fn);
        satoko_sort(data + i, (size - (unsigned) i), comp_fn);
    }
}

ABC_NAMESPACE_HEADER_END
#endif /* satoko__utils__sort_h */
