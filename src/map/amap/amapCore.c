/**CFile****************************************************************

  FileName    [amapCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Technology mapper for standard cells.]

  Synopsis    [Core mapping procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: amapCore.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "amapInt.h"
#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_ManSetDefaultParams( Amap_Par_t * p )
{
    memset( p, 0, sizeof(Amap_Par_t) );
    p->nIterFlow = 1;            // iterations of area flow
    p->nIterArea = 4;            // iteratoins of exact area
    p->nCutsMax  = 500;          // the maximum number of cuts at a node
    p->fUseMuxes = 0;            // enables the use of MUXes
    p->fUseXors  = 1;            // enables the use of XORs
    p->fFreeInvs = 0;            // assume inverters are free (area = 0)
    p->fEpsilon  = (float)0.001; // used to compare floating point numbers
    p->fVerbose  = 0;            // verbosity flag
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Amap_ManTest( Aig_Man_t * pAig, Amap_Par_t * pPars )
{
//    extern void * Abc_FrameReadLibGen2();
    Vec_Ptr_t * vRes;
    Amap_Man_t * p;
    Amap_Lib_t * pLib;
    abctime clkTotal = Abc_Clock();
    pLib = (Amap_Lib_t *)Abc_FrameReadLibGen2();
    if ( pLib == NULL )
    {
        printf( "Library is not available.\n" );
        return NULL;
    }
    p = Amap_ManStart( Aig_ManNodeNum(pAig) );
    p->pPars = pPars;
    p->pLib  = pLib;
    p->fAreaInv = pPars->fFreeInvs? 0.0 : pLib->pGateInv->dArea;
    p->fUseMux = pPars->fUseMuxes && pLib->fHasMux;
    p->fUseXor = pPars->fUseXors && pLib->fHasXor;
    p->ppCutsTemp = ABC_CALLOC( Amap_Cut_t *, 2 * pLib->nNodes );
    p->pMatsTemp = ABC_CALLOC( int, 2 * pLib->nNodes );
    Amap_ManCreate( p, pAig );
    Amap_ManMap( p );
    vRes = NULL;
    vRes = Amap_ManProduceMapped( p );
    Amap_ManStop( p );
if ( pPars->fVerbose )
{
ABC_PRT( "Total runtime", Abc_Clock() - clkTotal );
}
    return vRes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

