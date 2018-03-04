/**CFile****************************************************************

  FileName    [ifCache.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifCache.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"
#include "misc/vec/vecHsh.h"

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
void If_ManCacheRecord( If_Man_t * p, int iDsd0, int iDsd1, int nShared, int iDsd )
{
    assert( nShared >= 0 && nShared <= p->pPars->nLutSize );
    if ( p->vCutData == NULL )
        p->vCutData = Vec_IntAlloc( 10000 );
    if ( iDsd0 > iDsd1 )
        ABC_SWAP( int, iDsd0, iDsd1 );
    Vec_IntPush( p->vCutData, iDsd0 );
    Vec_IntPush( p->vCutData, iDsd1 );
    Vec_IntPush( p->vCutData, nShared );
    Vec_IntPush( p->vCutData, iDsd );
//    printf( "%6d %6d %6d %6d\n", iDsd0, iDsd1, nShared, iDsd );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManCacheAnalize( If_Man_t * p )
{
    Vec_Int_t * vRes, * vTest[32];
    int i, Entry, uUnique;
    vRes = Hsh_IntManHashArray( p->vCutData, 4 );
    for ( i = 0; i <= p->pPars->nLutSize; i++ )
        vTest[i] = Vec_IntAlloc( 1000 );
    Vec_IntForEachEntry( vRes, Entry, i )
        Vec_IntPush( vTest[Vec_IntEntry(p->vCutData, 4*i+2)], Entry );
    for ( i = 0; i <= p->pPars->nLutSize; i++ )
    {
        uUnique = Vec_IntCountUnique(vTest[i]);
        printf( "%2d-var entries = %8d. (%6.2f %%)  Unique entries = %8d. (%6.2f %%)\n", 
            i, Vec_IntSize(vTest[i]), 100.0*Vec_IntSize(vTest[i])/Abc_MaxInt(1, Vec_IntSize(vRes)), 
            uUnique, 100.0*uUnique/Abc_MaxInt(1, Vec_IntSize(vTest[i])) );
    }
    for ( i = 0; i <= p->pPars->nLutSize; i++ )
        Vec_IntFree( vTest[i] );
    uUnique = Vec_IntCountUnique(vRes);
    printf( "Total  entries = %8d. (%6.2f %%)  Unique entries = %8d. (%6.2f %%)\n", 
        Vec_IntSize(p->vCutData)/4, 100.0, uUnique, 100.0*uUnique/Abc_MaxInt(1, Vec_IntSize(p->vCutData)/4) );
    Vec_IntFree( vRes );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

