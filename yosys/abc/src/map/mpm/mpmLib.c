/**CFile****************************************************************

  FileName    [mpmLib.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Configurable technology mapper.]

  Synopsis    [DSD manipulation for 6-input functions.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 1, 2013.]

  Revision    [$Id: mpmLib.c,v 1.00 2013/06/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "mpmInt.h"

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
Mpm_LibLut_t * Mpm_LibLutSetSimple( int nLutSize )
{
    Mpm_LibLut_t * pLib;
    int i, k;
    assert( nLutSize <= MPM_VAR_MAX );
    pLib = ABC_CALLOC( Mpm_LibLut_t, 1 );
    pLib->LutMax = nLutSize;
    for ( i = 1; i <= pLib->LutMax; i++ )
    {
        pLib->pLutAreas[i] = MPM_UNIT_AREA;
        for ( k = 0; k < i; k++ )
            pLib->pLutDelays[i][k] = MPM_UNIT_TIME;
    }
    return pLib;
}
void Mpm_LibLutFree( Mpm_LibLut_t * pLib )
{
    if ( pLib == NULL )
        return;
    ABC_FREE( pLib->pName );
    ABC_FREE( pLib );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

