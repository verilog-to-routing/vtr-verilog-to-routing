/**CFile****************************************************************

  FileName    [lpkMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast Boolean matching for LUT structures.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: lpkMan.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "lpkInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lpk_Man_t * Lpk_ManStart( Lpk_Par_t * pPars )
{
    Lpk_Man_t * p;
    int i, nWords;
    assert( pPars->nLutsMax <= 16 );
    assert( pPars->nVarsMax > 0 && pPars->nVarsMax <= 16 );
    p = ABC_ALLOC( Lpk_Man_t, 1 );
    memset( p, 0, sizeof(Lpk_Man_t) );
    p->pPars = pPars;
    p->nCutsMax = LPK_CUTS_MAX;
    p->vTtElems = Vec_PtrAllocTruthTables( pPars->nVarsMax );
    p->vTtNodes = Vec_PtrAllocSimInfo( 1024, Abc_TruthWordNum(pPars->nVarsMax) );
    p->vCover = Vec_IntAlloc( 1 << 12 );
    p->vLeaves = Vec_PtrAlloc( 32 );
    for ( i = 0; i < 8; i++ )
        p->vSets[i] = Vec_IntAlloc(100);
    p->pDsdMan = Kit_DsdManAlloc( pPars->nVarsMax, 64 );
    p->vMemory = Vec_IntAlloc( 1024 * 32 );
    p->vBddDir = Vec_IntAlloc( 256 );
    p->vBddInv = Vec_IntAlloc( 256 );
    // allocate temporary storage for truth tables
    nWords = Kit_TruthWordNum(pPars->nVarsMax);
    p->ppTruths[0][0] = ABC_ALLOC( unsigned, 32 * nWords );
    p->ppTruths[1][0] = p->ppTruths[0][0] + 1 * nWords;
    for ( i = 1; i < 2; i++ )
        p->ppTruths[1][i] = p->ppTruths[1][0] + i * nWords;
    p->ppTruths[2][0] = p->ppTruths[1][0] + 2 * nWords;
    for ( i = 1; i < 4; i++ )
        p->ppTruths[2][i] = p->ppTruths[2][0] + i * nWords;
    p->ppTruths[3][0] = p->ppTruths[2][0] + 4 * nWords; 
    for ( i = 1; i < 8; i++ )
        p->ppTruths[3][i] = p->ppTruths[3][0] + i * nWords;
    p->ppTruths[4][0] = p->ppTruths[3][0] + 8 * nWords; 
    for ( i = 1; i < 16; i++ )
        p->ppTruths[4][i] = p->ppTruths[4][0] + i * nWords;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lpk_ManStop( Lpk_Man_t * p )
{
    int i;
    ABC_FREE( p->ppTruths[0][0] );
    Vec_IntFree( p->vBddDir );
    Vec_IntFree( p->vBddInv );
    Vec_IntFree( p->vMemory );
    Kit_DsdManFree( p->pDsdMan );
    for ( i = 0; i < 8; i++ )
        Vec_IntFree(p->vSets[i]);
    if ( p->pIfMan )
    {
        void * pPars = p->pIfMan->pPars;
        If_ManStop( p->pIfMan );
        ABC_FREE( pPars );
    }
    if ( p->vLevels )
        Vec_VecFree( p->vLevels );
    if ( p->vVisited )
        Vec_VecFree( p->vVisited );
    Vec_PtrFree( p->vLeaves );
    Vec_IntFree( p->vCover );
    Vec_PtrFree( p->vTtElems );
    Vec_PtrFree( p->vTtNodes );
    ABC_FREE( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

