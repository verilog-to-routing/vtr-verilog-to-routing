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
    int i;
    assert( pPars->nLutsMax <= 16 );
    assert( pPars->nVarsMax > 0 );
    p = ALLOC( Lpk_Man_t, 1 );
    memset( p, 0, sizeof(Lpk_Man_t) );
    p->pPars = pPars;
    p->nCutsMax = LPK_CUTS_MAX;
    p->vTtElems = Vec_PtrAllocTruthTables( pPars->nVarsMax );
    p->vTtNodes = Vec_PtrAllocSimInfo( 1024, Abc_TruthWordNum(pPars->nVarsMax) );
    p->vCover = Vec_IntAlloc( 1 << 12 );
    for ( i = 0; i < 8; i++ )
        p->vSets[i] = Vec_IntAlloc(100);
    p->pDsdMan = Kit_DsdManAlloc( pPars->nVarsMax );
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
    Kit_DsdManFree( p->pDsdMan );
    for ( i = 0; i < 8; i++ )
        Vec_IntFree(p->vSets[i]);
    if ( p->pIfMan )
    {
        void * pPars = p->pIfMan->pPars;
        If_ManStop( p->pIfMan );
        free( pPars );
    }
    if ( p->vLevels )
        Vec_VecFree( p->vLevels );
    if ( p->vVisited )
        Vec_VecFree( p->vVisited );
    Vec_IntFree( p->vCover );
    Vec_PtrFree( p->vTtElems );
    Vec_PtrFree( p->vTtNodes );
    free( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


