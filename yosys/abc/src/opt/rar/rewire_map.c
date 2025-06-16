/**CFile****************************************************************

  FileName    [rewire_map.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Re-wiring.]

  Synopsis    []

  Author      [Jiun-Hao Chen]
  
  Affiliation [National Taiwan University]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rewire_map.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "rewire_map.h"

ABC_NAMESPACE_IMPL_START

extern Abc_Ntk_t *Abc_NtkFromAigPhase(Aig_Man_t *pMan);
extern Abc_Ntk_t *Abc_NtkDarAmap(Abc_Ntk_t *pNtk, Amap_Par_t *pPars);
extern void *Abc_FrameReadLibGen2();
extern Vec_Int_t * Abc_NtkWriteMiniMapping( Abc_Ntk_t * pNtk );
extern void Abc_NtkPrintMiniMapping( int * pArray );
extern Abc_Ntk_t * Abc_NtkFromMiniMapping( int *vMapping );
extern Mini_Aig_t * Abc_MiniAigFromNtk ( Abc_Ntk_t *pNtk );
extern void Nf_ManSetDefaultPars( Jf_Par_t * pPars );
extern Gia_Man_t * Nf_ManPerformMapping( Gia_Man_t * pGia, Jf_Par_t * pPars );
extern Abc_Ntk_t * Abc_NtkFromMappedGia( Gia_Man_t * p, int fFindEnables, int fUseBuffs );
extern Abc_Ntk_t * Abc_NtkFromCellMappedGia( Gia_Man_t * p, int fUseBuffs );
extern int Gia_ManSimpleMapping( Gia_Man_t * p, int nBound, int Seed, int nBTLimit, int nTimeout, int fVerbose, int fKeepFile, int argc, char ** argv );

Abc_Ntk_t *Gia_ManRewirePut(Gia_Man_t *pGia) {
    Aig_Man_t *pMan = Gia_ManToAig(pGia, 0);
    Abc_Ntk_t *pNtk = Abc_NtkFromAigPhase(pMan);
    Abc_NtkSetName(pNtk, Abc_UtilStrsav(Gia_ManName(pGia)));
    Aig_ManStop(pMan);
    return pNtk;
}

Abc_Ntk_t *Abc_ManRewireMapAmap(Abc_Ntk_t *pNtk) {
    Amap_Par_t Pars, *pPars = &Pars;
    Amap_ManSetDefaultParams(pPars);
    Abc_Ntk_t *pNtkMapped = Abc_NtkDarAmap(pNtk, pPars);
    if (pNtkMapped == NULL) {
        Abc_Print(-1, "Mapping has failed.\n");
        return NULL;
    }
    return pNtkMapped;
}

Abc_Ntk_t *Gia_ManRewireMapNf(Gia_Man_t *pGia) {
    Jf_Par_t Pars, * pPars = &Pars;
    Nf_ManSetDefaultPars( pPars );
    Gia_Man_t *pGiaNew = Nf_ManPerformMapping(pGia, pPars);
    if (pGiaNew == NULL) {
        Abc_Print(-1, "Mapping has failed.\n");
        return NULL;
    }
    Abc_Ntk_t *pNtkMapped = Abc_NtkFromCellMappedGia(pGiaNew, 0);
    return pNtkMapped;
}

Abc_Ntk_t *Gia_ManRewireMapSimap(Gia_Man_t *pGia, int nBound, int nBTLimit, int nTimeout) {
    if (!Gia_ManSimpleMapping(pGia, nBound, 0, nBTLimit, nTimeout, 0, 0, 0, NULL)) {
        // Abc_Print(-1, "Mapping has failed.\n");
        return NULL;
    }
    Abc_Ntk_t *pNtkMapped = Abc_NtkFromCellMappedGia(pGia, 0);
    return pNtkMapped;
}

Vec_Int_t *Abc_ManRewireNtkWriteMiniMapping(Abc_Ntk_t *pNtk) {
    return Abc_NtkWriteMiniMapping(pNtk);
}

Abc_Ntk_t *Abc_ManRewireNtkFromMiniMapping(int *vMapping) {
    return Abc_NtkFromMiniMapping(vMapping);
}

Mini_Aig_t *Abc_ManRewireMiniAigFromNtk(Abc_Ntk_t *pNtk) {
    return Abc_MiniAigFromNtk(pNtk);
}

ABC_NAMESPACE_IMPL_END