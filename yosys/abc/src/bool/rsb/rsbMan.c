/**CFile****************************************************************

  FileName    [rsbMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Truth-table based resubstitution.]

  Synopsis    [Manager maintenance.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rsbMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "rsbInt.h"

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
Rsb_Man_t * Rsb_ManAlloc( int nLeafMax, int nDivMax, int nDecMax, int fVerbose )
{
    Rsb_Man_t * p;
    assert( nLeafMax <= 20 );
    assert( nDivMax  <= 200 );
    p = ABC_CALLOC( Rsb_Man_t, 1 );
    p->nLeafMax   = nLeafMax;
    p->nDivMax    = nDivMax;
    p->nDecMax    = nDecMax;
    p->fVerbose   = fVerbose;
    // decomposition
    p->vCexes     = Vec_WrdAlloc( nDivMax + 150 );
    p->vDecPats   = Vec_IntAlloc( Abc_TtWordNum(nLeafMax) );
    p->vFanins    = Vec_IntAlloc( 10 );
    p->vFaninsOld = Vec_IntAlloc( 10 );
    p->vTries     = Vec_IntAlloc( 10 );
    return p;
}
void Rsb_ManFree( Rsb_Man_t * p )
{
    Vec_WrdFree( p->vCexes );
    Vec_IntFree( p->vDecPats );
    Vec_IntFree( p->vFanins );
    Vec_IntFree( p->vFaninsOld );
    Vec_IntFree( p->vTries );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Rsb_ManGetFanins( Rsb_Man_t * p )
{
    return p->vFanins;
}
Vec_Int_t * Rsb_ManGetFaninsOld( Rsb_Man_t * p )
{
    return p->vFaninsOld;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

