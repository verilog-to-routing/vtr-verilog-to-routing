//===--- mem.h --------------------------------------------------------------===
//
//                     satoko: Satisfiability solver
//
// This file is distributed under the BSD 2-Clause License.
// See LICENSE for details.
//
//===------------------------------------------------------------------------===
#ifndef satoko__utils__mem_h
#define satoko__utils__mem_h

#include <stdlib.h>

#include "misc/util/abc_global.h"
ABC_NAMESPACE_HEADER_START

#define satoko_alloc(type, n_elements) ((type *) malloc((n_elements) * sizeof(type)))
#define satoko_calloc(type, n_elements) ((type *) calloc((n_elements), sizeof(type)))
#define satoko_realloc(type, ptr, n_elements) ((type *) realloc(ptr, (n_elements) * sizeof(type)))
#define satoko_free(p) do { free(p); p = NULL; } while(0)

ABC_NAMESPACE_HEADER_END
#endif
