/**CFile****************************************************************

  FileName    [intInter.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Interpolation engine.]

  Synopsis    [Experimental procedures to derive and compare interpolants.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 24, 2008.]

  Revision    [$Id: intInter.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "intInt.h"

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
Aig_Man_t * Inter_ManDupExpand( Aig_Man_t * pInter, Aig_Man_t * pOther )
{
    Aig_Man_t * pInterC;
    assert( Aig_ManCiNum(pInter) <= Aig_ManCiNum(pOther) );
    pInterC = Aig_ManDupSimple( pInter );
    Aig_IthVar( pInterC, Aig_ManCiNum(pOther)-1 );
    assert( Aig_ManCiNum(pInterC) == Aig_ManCiNum(pOther) );
    return pInterC;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Inter_ManVerifyInterpolant1( Inta_Man_t * pMan, Sto_Man_t * pCnf, Aig_Man_t * pInter )
{
    extern Aig_Man_t * Inta_ManDeriveClauses( Inta_Man_t * pMan, Sto_Man_t * pCnf, int fClausesA );
    Aig_Man_t * pLower, * pUpper, * pInterC;
    int RetValue1, RetValue2;

    pLower = Inta_ManDeriveClauses( pMan, pCnf, 1 );
    pUpper = Inta_ManDeriveClauses( pMan, pCnf, 0 );
    Aig_ManFlipFirstPo( pUpper );

    pInterC = Inter_ManDupExpand( pInter, pLower );
    RetValue1 = Inter_ManCheckContainment( pLower, pInterC );
    Aig_ManStop( pInterC );

    pInterC = Inter_ManDupExpand( pInter, pUpper );
    RetValue2 = Inter_ManCheckContainment( pInterC, pUpper );
    Aig_ManStop( pInterC );
    
    if ( RetValue1 && RetValue2 )
        printf( "Im is correct.\n" );
    if ( !RetValue1 )
        printf( "Property A => Im fails.\n" );
    if ( !RetValue2 )
        printf( "Property Im => !B fails.\n" );

    Aig_ManStop( pLower );
    Aig_ManStop( pUpper );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Inter_ManVerifyInterpolant2( Intb_Man_t * pMan, Sto_Man_t * pCnf, Aig_Man_t * pInter )
{
    extern Aig_Man_t * Intb_ManDeriveClauses( Intb_Man_t * pMan, Sto_Man_t * pCnf, int fClausesA );
    Aig_Man_t * pLower, * pUpper, * pInterC;
    int RetValue1, RetValue2;

    pLower = Intb_ManDeriveClauses( pMan, pCnf, 1 );
    pUpper = Intb_ManDeriveClauses( pMan, pCnf, 0 );
    Aig_ManFlipFirstPo( pUpper );

    pInterC = Inter_ManDupExpand( pInter, pLower );
//Aig_ManPrintStats( pLower );
//Aig_ManPrintStats( pUpper );
//Aig_ManPrintStats( pInterC );
//Aig_ManDumpBlif( pInterC, "inter_c.blif", NULL, NULL );
    RetValue1 = Inter_ManCheckContainment( pLower, pInterC );
    Aig_ManStop( pInterC );

    pInterC = Inter_ManDupExpand( pInter, pUpper );
    RetValue2 = Inter_ManCheckContainment( pInterC, pUpper );
    Aig_ManStop( pInterC );
    
    if ( RetValue1 && RetValue2 )
        printf( "Ip is correct.\n" );
    if ( !RetValue1 )
        printf( "Property A => Ip fails.\n" );
    if ( !RetValue2 )
        printf( "Property Ip => !B fails.\n" );

    Aig_ManStop( pLower );
    Aig_ManStop( pUpper );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

