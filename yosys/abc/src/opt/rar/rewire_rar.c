/**CFile****************************************************************

  FileName    [rewire_rar.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Re-wiring.]

  Synopsis    []

  Author      [Jiun-Hao Chen]
  
  Affiliation [National Taiwan University]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rewire_rar.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "rewire_rar.h"

ABC_NAMESPACE_IMPL_START

Gia_Man_t *Gia_ManRewire(Gia_Man_t *pGia, int nIters, float levelGrowRatio, int nExpands, int nGrowth, int nDivs, int nFaninMax, int nTimeOut, int nMode, int nMappedMode, int nDist, int nSeed, int fVerbose) {
    return Gia_ManRewireInt(pGia, nIters, levelGrowRatio, nExpands, nGrowth, nDivs, nFaninMax, nTimeOut, nMode, nMappedMode, nDist, nSeed, fVerbose);
}

Abc_Ntk_t *Abc_ManRewire(Abc_Ntk_t *pNtk, int nIters, float levelGrowRatio, int nExpands, int nGrowth, int nDivs, int nFaninMax, int nTimeOut, int nMode, int nMappedMode, int nDist, int nSeed, int fVerbose) {
    return Abc_ManRewireInt(pNtk, nIters, levelGrowRatio, nExpands, nGrowth, nDivs, nFaninMax, nTimeOut, nMode, nMappedMode, nDist, nSeed, fVerbose);
}

Mini_Aig_t *MiniAig_ManRewire(Mini_Aig_t *pAig, int nIters, float levelGrowRatio, int nExpands, int nGrowth, int nDivs, int nFaninMax, int nTimeOut, int nMode, int nMappedMode, int nDist, int nSeed, int fVerbose) {
    return MiniAig_ManRewireInt(pAig, nIters, levelGrowRatio, nExpands, nGrowth, nDivs, nFaninMax, nTimeOut, nMode, nMappedMode, nDist, nSeed, fVerbose);
}

ABC_NAMESPACE_IMPL_END