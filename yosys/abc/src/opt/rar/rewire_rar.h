/**CFile****************************************************************

  FileName    [rewire_rar.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Re-wiring.]

  Synopsis    []

  Author      [Jiun-Hao Chen]
  
  Affiliation [National Taiwan University]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rewire_rar.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef RAR_H
#define RAR_H

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "base/abc/abc.h"
#include "aig/gia/gia.h"
#include "aig/miniaig/miniaig.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

Gia_Man_t *Gia_ManRewire(Gia_Man_t *pGia, int nIters, float levelGrowRatio, int nExpands, int nGrowth, int nDivs, int nFaninMax, int nTimeOut, int nMode, int nMappedMode, int nDist, int nSeed, int fVerbose);
Gia_Man_t *Gia_ManRewireInt(Gia_Man_t *pGia, int nIters, float levelGrowRatio, int nExpands, int nGrowth, int nDivs, int nFaninMax, int nTimeOut, int nMode, int nMappedMode, int nDist, int nSeed, int fVerbose);
Abc_Ntk_t *Abc_ManRewire(Abc_Ntk_t *pNtk, int nIters, float levelGrowRatio, int nExpands, int nGrowth, int nDivs, int nFaninMax, int nTimeOut, int nMode, int nMappedMode, int nDist, int nSeed, int fVerbose);
Abc_Ntk_t *Abc_ManRewireInt(Abc_Ntk_t *pNtk, int nIters, float levelGrowRatio, int nExpands, int nGrowth, int nDivs, int nFaninMax, int nTimeOut, int nMode, int nMappedMode, int nDist, int nSeed, int fVerbose);
Mini_Aig_t *MiniAig_ManRewire(Mini_Aig_t *pAig, int nIters, float levelGrowRatio, int nExpands, int nGrowth, int nDivs, int nFaninMax, int nTimeOut, int nMode, int nMappedMode, int nDist, int nSeed, int fVerbose);
Mini_Aig_t *MiniAig_ManRewireInt(Mini_Aig_t *pAig, int nIters, float levelGrowRatio, int nExpands, int nGrowth, int nDivs, int nFaninMax, int nTimeOut, int nMode, int nMappedMode, int nDist, int nSeed, int fVerbose);

ABC_NAMESPACE_HEADER_END

#endif // RAR_H