/**C++File**************************************************************

  FileName    [ac_wrapper.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Ashenhurst-Curtis decomposition.]

  Synopsis    [Interface with the FPGA mapping package.]

  Author      [Alessandro Tempia Calvino]
  
  Affiliation [EPFL]

  Date        [Ver. 1.0. Started - November 20, 2023.]

***********************************************************************/

#pragma once
#ifndef __ACD_WRAPPER_H_
#define __ACD_WRAPPER_H_

#ifdef _MSC_VER
#  include <intrin.h>
#  define __builtin_popcount __popcnt
//#  define __builtin_popcountl __popcnt64 // the compiler does not find __popcnt64
#  define __builtin_popcountl __popcnt
#endif

#include "misc/util/abc_global.h"
#include "map/if/if.h"

ABC_NAMESPACE_HEADER_START

int acd_evaluate( word * pTruth, unsigned nVars, int lutSize, unsigned *pdelay, unsigned *cost, int try_no_late_arrival );
int acd_decompose( word * pTruth, unsigned nVars, int lutSize, unsigned *pdelay, unsigned char *decomposition );
int acd2_evaluate( word * pTruth, unsigned nVars, int lutSize, unsigned *pdelay, unsigned *cost, int try_no_late_arrival );
int acd2_decompose( word * pTruth, unsigned nVars, int lutSize, unsigned *pdelay, unsigned char *decomposition );

int acdXX_evaluate( word * pTruth, unsigned lutSize, unsigned nVars );
int acdXX_decompose( word * pTruth, unsigned lutSize, unsigned nVars, unsigned char *decomposition );

ABC_NAMESPACE_HEADER_END

#endif