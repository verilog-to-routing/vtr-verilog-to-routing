/**CFile****************************************************************

  FileName    [darScript.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting.]

  Synopsis    [Rewriting scripts.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: darScript.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "darInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs one iteration of AIG rewriting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Dar_ManRewriteDefault( Aig_Man_t * pAig )
{
    Aig_Man_t * pTemp;
    Dar_RwrPar_t Pars, * pPars = &Pars;
    Dar_ManDefaultRwrParams( pPars );
    pAig = Aig_ManDup( pTemp = pAig, 0 ); 
    Aig_ManStop( pTemp );
    Dar_ManRewrite( pAig, pPars );
    pAig = Aig_ManDup( pTemp = pAig, 0 ); 
    Aig_ManStop( pTemp );
    return pAig;
}

/**Function*************************************************************

  Synopsis    [Reproduces script "compress2".]

  Description []
               
  SideEffects [This procedure does not tighten level during restructuring.]

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Dar_ManCompress2( Aig_Man_t * pAig, int fBalance, int fUpdateLevel, int fVerbose )
//alias compress2   "b -l; rw -l; rf -l; b -l; rw -l; rwz -l; b -l; rfz -l; rwz -l; b -l"
{
    Aig_Man_t * pTemp;

    Dar_RwrPar_t ParsRwr, * pParsRwr = &ParsRwr;
    Dar_RefPar_t ParsRef, * pParsRef = &ParsRef;

    Dar_ManDefaultRwrParams( pParsRwr );
    Dar_ManDefaultRefParams( pParsRef );

    pParsRwr->fUpdateLevel = fUpdateLevel;
    pParsRef->fUpdateLevel = fUpdateLevel;

    pParsRwr->fVerbose = fVerbose;
    pParsRef->fVerbose = fVerbose;

    // balance
    if ( fBalance )
    {
//    pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
//    Aig_ManStop( pTemp );
    }
    
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDup( pTemp = pAig, 0 ); 
    Aig_ManStop( pTemp );
    
    // refactor
    Dar_ManRefactor( pAig, pParsRef );
    pAig = Aig_ManDup( pTemp = pAig, 0 ); 
    Aig_ManStop( pTemp );

    // balance
//    if ( fBalance )
    {
    pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
    Aig_ManStop( pTemp );
    }
    
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDup( pTemp = pAig, 0 ); 
    Aig_ManStop( pTemp );

    pParsRwr->fUseZeros = 1;
    pParsRef->fUseZeros = 1;
    
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDup( pTemp = pAig, 0 ); 
    Aig_ManStop( pTemp );

    // balance
    if ( fBalance )
    {
    pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
    Aig_ManStop( pTemp );
    }
    
    // refactor
    Dar_ManRefactor( pAig, pParsRef );
    pAig = Aig_ManDup( pTemp = pAig, 0 ); 
    Aig_ManStop( pTemp );
    
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDup( pTemp = pAig, 0 ); 
    Aig_ManStop( pTemp );

    // balance
    if ( fBalance )
    {
    pAig = Dar_ManBalance( pTemp = pAig, fUpdateLevel );
    Aig_ManStop( pTemp );
    }
    return pAig;
}

/**Function*************************************************************

  Synopsis    [Reproduces script "compress2".]

  Description []
               
  SideEffects [This procedure does not tighten level during restructuring.]

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Dar_ManRwsat( Aig_Man_t * pAig, int fBalance, int fVerbose )
//alias rwsat       "st; rw -l; b -l; rw -l; rf -l"
{
    Aig_Man_t * pTemp;

    Dar_RwrPar_t ParsRwr, * pParsRwr = &ParsRwr;
    Dar_RefPar_t ParsRef, * pParsRef = &ParsRef;

    Dar_ManDefaultRwrParams( pParsRwr );
    Dar_ManDefaultRefParams( pParsRef );

    pParsRwr->fUpdateLevel = 0;
    pParsRef->fUpdateLevel = 0;

    pParsRwr->fVerbose = fVerbose;
    pParsRef->fVerbose = fVerbose;
    
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDup( pTemp = pAig, 0 ); 
    Aig_ManStop( pTemp );
    
    // refactor
    Dar_ManRefactor( pAig, pParsRef );
    pAig = Aig_ManDup( pTemp = pAig, 0 ); 
    Aig_ManStop( pTemp );

    // balance
    if ( fBalance )
    {
    pAig = Dar_ManBalance( pTemp = pAig, 0 );
    Aig_ManStop( pTemp );
    }
    
    // rewrite
    Dar_ManRewrite( pAig, pParsRwr );
    pAig = Aig_ManDup( pTemp = pAig, 0 ); 
    Aig_ManStop( pTemp );

    return pAig;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


