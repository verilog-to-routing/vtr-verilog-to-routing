//===--- types.h ------------------------------------------------------------===
//
//                     satoko: Satisfiability solver
//
// This file is distributed under the BSD 2-Clause License.
// See LICENSE for details.
//
//===------------------------------------------------------------------------===
#ifndef satoko__types_h
#define satoko__types_h

#include "utils/sdbl.h"
#include "utils/vec/vec_sdbl.h"

#include "misc/util/abc_global.h"
ABC_NAMESPACE_HEADER_START

/* In Satoko ABC version this file is useless */

#define VAR_ACT_INIT_INC SDBL_CONST1
#define VAR_ACT_LIMIT ABC_CONST(0x014c924d692ca61b)
#define VAR_ACT_RESCALE 200
typedef sdbl_t act_t;
typedef vec_sdbl_t vec_act_t ;
#define vec_act_alloc(size) vec_sdbl_alloc(size)
#define vec_act_free(vec) vec_sdbl_free(vec)
#define vec_act_size(vec) vec_sdbl_size(vec)
#define vec_act_data(vec) vec_sdbl_data(vec)
#define vec_act_clear(vec) vec_sdbl_clear(vec)
#define vec_act_shrink(vec, size) vec_sdbl_shrink(vec, size)
#define vec_act_at(vec, idx) vec_sdbl_at(vec, idx)
#define vec_act_push_back(vec, value) vec_sdbl_push_back(vec, value)


#define CLAUSE_ACT_INIT_INC (1 << 11)
typedef unsigned clause_act_t;

ABC_NAMESPACE_HEADER_END
#endif /* satoko__types_h */
