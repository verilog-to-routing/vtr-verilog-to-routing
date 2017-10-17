/**CFile****************************************************************

  FileName    [lpkMux.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast Boolean matching for LUT structures.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: lpkMux.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

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

  Synopsis    [Find the best cofactoring variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lpk_MapTreeBestCofVar( Lpk_Man_t * p, unsigned * pTruth, int nVars, unsigned * pCof0, unsigned * pCof1 )
{
    int i, iBestVar, nSuppSizeCur0, nSuppSizeCur1, nSuppSizeCur, nSuppSizeMin;
    // iterate through variables
    iBestVar = -1;
    nSuppSizeMin = KIT_INFINITY;
    for ( i = 0; i < nVars; i++ )
    {
        // cofactor the functiona and get support sizes
        Kit_TruthCofactor0New( pCof0, pTruth, nVars, i );
        Kit_TruthCofactor1New( pCof1, pTruth, nVars, i );
        nSuppSizeCur0 = Kit_TruthSupportSize( pCof0, nVars );
        nSuppSizeCur1 = Kit_TruthSupportSize( pCof1, nVars );
        nSuppSizeCur  = nSuppSizeCur0 + nSuppSizeCur1;
        // skip cofactoring that goes above the limit
        if ( nSuppSizeCur0 > p->pPars->nLutSize || nSuppSizeCur1 > p->pPars->nLutSize )
            continue;
        // compare this variable with other variables
        if ( nSuppSizeMin > nSuppSizeCur ) 
        {
            nSuppSizeMin = nSuppSizeCur;
            iBestVar = i;
        }
    }
    // cofactor w.r.t. this variable
    if ( iBestVar != -1 )
    {
        Kit_TruthCofactor0New( pCof0, pTruth, nVars, iBestVar );
        Kit_TruthCofactor1New( pCof1, pTruth, nVars, iBestVar );
    }
    return iBestVar;
}

/**Function*************************************************************

  Synopsis    [Maps the function by the best cofactoring.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Obj_t * Lpk_MapTreeMux_rec( Lpk_Man_t * p, unsigned * pTruth, int nVars, If_Obj_t ** ppLeaves )
{
    unsigned * pCof0 = (unsigned *)Vec_PtrEntry( p->vTtNodes, 0 );
    unsigned * pCof1 = (unsigned *)Vec_PtrEntry( p->vTtNodes, 1 );
    If_Obj_t * pObj0, * pObj1;
    Kit_DsdNtk_t * ppNtks[2];
    int iBestVar;
    assert( nVars > 3 );
    p->fCalledOnce = 1;
    // cofactor w.r.t. the best variable
    iBestVar = Lpk_MapTreeBestCofVar( p, pTruth, nVars, pCof0, pCof1 );
    if ( iBestVar == -1 )
        return NULL;
    // decompose the functions
    ppNtks[0] = Kit_DsdDecompose( pCof0, nVars );
    ppNtks[1] = Kit_DsdDecompose( pCof1, nVars );
    if ( p->pPars->fVeryVerbose )
    {
        printf( "Cofactoring w.r.t. var %c (%d -> %d+%d supp vars):\n", 
            'a'+iBestVar, nVars, Kit_TruthSupportSize(pCof0, nVars), Kit_TruthSupportSize(pCof1, nVars) );
        Kit_DsdPrintExpanded( ppNtks[0] );
        Kit_DsdPrintExpanded( ppNtks[1] );
    }
    // map the DSD structures
    pObj0 = Lpk_MapTree_rec( p, ppNtks[0], ppLeaves, ppNtks[0]->Root, NULL );
    pObj1 = Lpk_MapTree_rec( p, ppNtks[1], ppLeaves, ppNtks[1]->Root, NULL );
    Kit_DsdNtkFree( ppNtks[0] );
    Kit_DsdNtkFree( ppNtks[1] );
    return If_ManCreateMux( p->pIfMan, pObj0, pObj1, ppLeaves[iBestVar] );
}



/**Function*************************************************************

  Synopsis    [Implements support-reducing decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Obj_t * Lpk_MapSuppRedDec_rec( Lpk_Man_t * p, unsigned * pTruth, int nVars, If_Obj_t ** ppLeaves )
{
    Kit_DsdNtk_t * pNtkDec, * pNtkComp, * ppNtks[2], * pTemp;
    If_Obj_t * pObjNew;
    unsigned * pCof0 = (unsigned *)Vec_PtrEntry( p->vTtNodes,  0 );
    unsigned * pCof1 = (unsigned *)Vec_PtrEntry( p->vTtNodes,  1 );
    unsigned * pDec0 = (unsigned *)Vec_PtrEntry( p->vTtNodes,  2 );
    unsigned * pDec1 = (unsigned *)Vec_PtrEntry( p->vTtNodes,  3 );
    unsigned * pDec  = (unsigned *)Vec_PtrEntry( p->vTtNodes,  4 );
    unsigned * pCo00 = (unsigned *)Vec_PtrEntry( p->vTtNodes,  5 );
    unsigned * pCo01 = (unsigned *)Vec_PtrEntry( p->vTtNodes,  6 );
    unsigned * pCo10 = (unsigned *)Vec_PtrEntry( p->vTtNodes,  7 );
    unsigned * pCo11 = (unsigned *)Vec_PtrEntry( p->vTtNodes,  8 );
    unsigned * pCo0  = (unsigned *)Vec_PtrEntry( p->vTtNodes,  9 );
    unsigned * pCo1  = (unsigned *)Vec_PtrEntry( p->vTtNodes, 10 );
    unsigned * pCo   = (unsigned *)Vec_PtrEntry( p->vTtNodes, 11 );
    int TrueMint0, TrueMint1, FalseMint0, FalseMint1;
    int uSubsets, uSubset0, uSubset1, iVar, iVarReused, i;

    // determine if supp-red decomposition exists
    uSubsets = Lpk_MapSuppRedDecSelect( p, pTruth, nVars, &iVar, &iVarReused );
    if ( uSubsets == 0 )
        return NULL;
    p->nCalledSRed++;

    // get the cofactors
    Kit_TruthCofactor0New( pCof0, pTruth, nVars, iVar );
    Kit_TruthCofactor1New( pCof1, pTruth, nVars, iVar );

    // get the bound sets
    uSubset0 = uSubsets & 0xFFFF;
    uSubset1 = uSubsets >> 16;

    // compute the decomposed functions
    ppNtks[0] = Kit_DsdDecompose( pCof0, nVars );
    ppNtks[1] = Kit_DsdDecompose( pCof1, nVars );
    ppNtks[0] = Kit_DsdExpand( pTemp = ppNtks[0] );      Kit_DsdNtkFree( pTemp );
    ppNtks[1] = Kit_DsdExpand( pTemp = ppNtks[1] );      Kit_DsdNtkFree( pTemp );
    Kit_DsdTruthPartial( p->pDsdMan, ppNtks[0], pDec0, uSubset0 );
    Kit_DsdTruthPartial( p->pDsdMan, ppNtks[1], pDec1, uSubset1 );
//    Kit_DsdTruthPartialTwo( p->pDsdMan, ppNtks[0], uSubset0, iVarReused, pCo0, pDec0 );
//    Kit_DsdTruthPartialTwo( p->pDsdMan, ppNtks[1], uSubset1, iVarReused, pCo1, pDec1 );
    Kit_DsdNtkFree( ppNtks[0] );
    Kit_DsdNtkFree( ppNtks[1] );
//Kit_DsdPrintFromTruth( pDec0, nVars );
//Kit_DsdPrintFromTruth( pDec1, nVars );
    // get the decomposed function
    Kit_TruthMuxVar( pDec, pDec0, pDec1, nVars, iVar );

    // find any true assignments of the decomposed functions
    TrueMint0 = Kit_TruthFindFirstBit( pDec0, nVars );
    TrueMint1 = Kit_TruthFindFirstBit( pDec1, nVars );
    assert( TrueMint0 >= 0 && TrueMint1 >= 0 );
    // find any false assignments of the decomposed functions
    FalseMint0 = Kit_TruthFindFirstZero( pDec0, nVars );
    FalseMint1 = Kit_TruthFindFirstZero( pDec1, nVars );
    assert( FalseMint0 >= 0 && FalseMint1 >= 0 );

    // cofactor the cofactors according to these minterms
    Kit_TruthCopy( pCo00, pCof0, nVars );
    Kit_TruthCopy( pCo01, pCof0, nVars );
    for ( i = 0; i < nVars; i++ )
        if ( uSubset0 & (1 << i) )
        {
            if ( FalseMint0 & (1 << i) )
                Kit_TruthCofactor1( pCo00, nVars, i );
            else
                Kit_TruthCofactor0( pCo00, nVars, i );
            if ( TrueMint0 & (1 << i) )
                Kit_TruthCofactor1( pCo01, nVars, i );
            else
                Kit_TruthCofactor0( pCo01, nVars, i );
        }
    Kit_TruthCopy( pCo10, pCof1, nVars );
    Kit_TruthCopy( pCo11, pCof1, nVars );
    for ( i = 0; i < nVars; i++ )
        if ( uSubset1 & (1 << i) )
        {
            if ( FalseMint1 & (1 << i) )
                Kit_TruthCofactor1( pCo10, nVars, i );
            else
                Kit_TruthCofactor0( pCo10, nVars, i );
            if ( TrueMint1 & (1 << i) )
                Kit_TruthCofactor1( pCo11, nVars, i );
            else
                Kit_TruthCofactor0( pCo11, nVars, i );
        }

    // derive the functions by composing them with the new variable (iVarReused)
    Kit_TruthMuxVar( pCo0, pCo00, pCo01, nVars, iVarReused );
    Kit_TruthMuxVar( pCo1, pCo10, pCo11, nVars, iVarReused );
//Kit_DsdPrintFromTruth( pCo0, nVars );
//Kit_DsdPrintFromTruth( pCo1, nVars );

    // derive the composition function
    Kit_TruthMuxVar( pCo , pCo0 , pCo1 , nVars, iVar );

    // process the decomposed function
    pNtkDec = Kit_DsdDecompose( pDec, nVars );
    pNtkComp = Kit_DsdDecompose( pCo, nVars );
//Kit_DsdPrint( stdout, pNtkDec );
//Kit_DsdPrint( stdout, pNtkComp );
//printf( "cofactored variable %c\n", 'a' + iVar );
//printf( "reused variable %c\n", 'a' + iVarReused );

    ppLeaves[iVarReused] = Lpk_MapTree_rec( p, pNtkDec, ppLeaves, pNtkDec->Root, NULL );
    pObjNew = Lpk_MapTree_rec( p, pNtkComp, ppLeaves, pNtkComp->Root, NULL );

    Kit_DsdNtkFree( pNtkDec );
    Kit_DsdNtkFree( pNtkComp );
    return pObjNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

