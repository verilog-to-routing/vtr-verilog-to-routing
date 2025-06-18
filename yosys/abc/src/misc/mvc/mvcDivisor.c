/**CFile****************************************************************

  FileName    [mvcDivisor.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures for compute the quick divisor.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcDivisor.c,v 1.1 2003/04/03 15:34:08 alanmi Exp $]

***********************************************************************/

#include "mvc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void  Mvc_CoverDivisorZeroKernel( Mvc_Cover_t * pCover );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the quick divisor of the cover.]

  Description [Returns NULL, if there is not divisor other than
  trivial.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverDivisor( Mvc_Cover_t * pCover )
{
    Mvc_Cover_t * pKernel;
    if ( Mvc_CoverReadCubeNum(pCover) <= 1 )
        return NULL;
    // allocate the literal array and count literals
    if ( Mvc_CoverAnyLiteral( pCover, NULL ) == -1 )
        return NULL;
    // duplicate the cover
    pKernel = Mvc_CoverDup(pCover);
    // perform the kerneling
    Mvc_CoverDivisorZeroKernel( pKernel );
    assert( Mvc_CoverReadCubeNum(pKernel) );
    return pKernel;
}

/**Function*************************************************************

  Synopsis    [Computes a level-zero kernel.]

  Description [Modifies the cover to contain one level-zero kernel.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverDivisorZeroKernel( Mvc_Cover_t * pCover )
{
    int iLit;
    // find any literal that occurs at least two times
//    iLit = Mvc_CoverAnyLiteral( pCover, NULL );
    iLit = Mvc_CoverWorstLiteral( pCover, NULL );
//    iLit = Mvc_CoverBestLiteral( pCover, NULL );
    if ( iLit == -1 )
        return;
    // derive the cube-free quotient
    Mvc_CoverDivideByLiteralQuo( pCover, iLit ); // the same cover
    Mvc_CoverMakeCubeFree( pCover );             // the same cover
    // call recursively
    Mvc_CoverDivisorZeroKernel( pCover );              // the same cover
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

