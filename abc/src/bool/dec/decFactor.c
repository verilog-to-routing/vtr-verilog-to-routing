/**CFile****************************************************************

  FileName    [ftFactor.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures for algebraic factoring.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ftFactor.c,v 1.3 2003/09/01 04:56:43 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "misc/mvc/mvc.h"
#include "dec.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Dec_Edge_t       Dec_Factor_rec( Dec_Graph_t * pFForm, Mvc_Cover_t * pCover );
static Dec_Edge_t       Dec_FactorLF_rec( Dec_Graph_t * pFForm, Mvc_Cover_t * pCover, Mvc_Cover_t * pSimple );
static Dec_Edge_t       Dec_FactorTrivial( Dec_Graph_t * pFForm, Mvc_Cover_t * pCover );
static Dec_Edge_t       Dec_FactorTrivialCube( Dec_Graph_t * pFForm, Mvc_Cover_t * pCover, Mvc_Cube_t * pCube, Vec_Int_t * vEdgeLits );
static Dec_Edge_t       Dec_FactorTrivialTree_rec( Dec_Graph_t * pFForm, Dec_Edge_t * peNodes, int nNodes, int fNodeOr );
static int              Dec_FactorVerify( char * pSop, Dec_Graph_t * pFForm );
static Mvc_Cover_t *    Dec_ConvertSopToMvc( char * pSop );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Factors the cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Dec_Factor( char * pSop )
{
    Mvc_Cover_t * pCover;
    Dec_Graph_t * pFForm;
    Dec_Edge_t eRoot;
    if ( Abc_SopIsConst0(pSop) )
        return Dec_GraphCreateConst0();
    if ( Abc_SopIsConst1(pSop) )
        return Dec_GraphCreateConst1();

    // derive the cover from the SOP representation
    pCover = Dec_ConvertSopToMvc( pSop );

    // make sure the cover is CCS free (should be done before CST)
    Mvc_CoverContain( pCover );

    // check for trivial functions
    assert( !Mvc_CoverIsEmpty(pCover) );
    assert( !Mvc_CoverIsTautology(pCover) );

    // perform CST
    Mvc_CoverInverse( pCover ); // CST
    // start the factored form
    pFForm = Dec_GraphCreate( Abc_SopGetVarNum(pSop) );
    // factor the cover
    eRoot = Dec_Factor_rec( pFForm, pCover );
    // finalize the factored form
    Dec_GraphSetRoot( pFForm, eRoot );
    // complement the factored form if SOP is complemented
    if ( Abc_SopIsComplement(pSop) )
        Dec_GraphComplement( pFForm );
    // verify the factored form
//    if ( !Dec_FactorVerify( pSop, pFForm ) )
//        printf( "Verification has failed.\n" );
//    Mvc_CoverInverse( pCover ); // undo CST
    Mvc_CoverFree( pCover );
    return pFForm;
}

/**Function*************************************************************

  Synopsis    [Internal recursive factoring procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Edge_t Dec_Factor_rec( Dec_Graph_t * pFForm, Mvc_Cover_t * pCover )
{
    Mvc_Cover_t * pDiv, * pQuo, * pRem, * pCom;
    Dec_Edge_t eNodeDiv, eNodeQuo, eNodeRem;
    Dec_Edge_t eNodeAnd, eNode;

    // make sure the cover contains some cubes
    assert( Mvc_CoverReadCubeNum(pCover) );

    // get the divisor
    pDiv = Mvc_CoverDivisor( pCover );
    if ( pDiv == NULL )
        return Dec_FactorTrivial( pFForm, pCover );

    // divide the cover by the divisor
    Mvc_CoverDivideInternal( pCover, pDiv, &pQuo, &pRem );
    assert( Mvc_CoverReadCubeNum(pQuo) );

    Mvc_CoverFree( pDiv );
    Mvc_CoverFree( pRem );

    // check the trivial case
    if ( Mvc_CoverReadCubeNum(pQuo) == 1 )
    {
        eNode = Dec_FactorLF_rec( pFForm, pCover, pQuo );
        Mvc_CoverFree( pQuo );
        return eNode;
    }

    // make the quotient cube ABC_FREE
    Mvc_CoverMakeCubeFree( pQuo );

    // divide the cover by the quotient
    Mvc_CoverDivideInternal( pCover, pQuo, &pDiv, &pRem );

    // check the trivial case
    if ( Mvc_CoverIsCubeFree( pDiv ) )
    {
        eNodeDiv = Dec_Factor_rec( pFForm, pDiv );
        eNodeQuo = Dec_Factor_rec( pFForm, pQuo );
        Mvc_CoverFree( pDiv );
        Mvc_CoverFree( pQuo );
        eNodeAnd = Dec_GraphAddNodeAnd( pFForm, eNodeDiv, eNodeQuo );
        if ( Mvc_CoverReadCubeNum(pRem) == 0 )
        {
            Mvc_CoverFree( pRem );
            return eNodeAnd;
        }
        else
        {
            eNodeRem = Dec_Factor_rec( pFForm, pRem );
            Mvc_CoverFree( pRem );
            return Dec_GraphAddNodeOr( pFForm, eNodeAnd, eNodeRem );
        }
    }

    // get the common cube
    pCom = Mvc_CoverCommonCubeCover( pDiv );
    Mvc_CoverFree( pDiv );
    Mvc_CoverFree( pQuo );
    Mvc_CoverFree( pRem );

    // solve the simple problem
    eNode = Dec_FactorLF_rec( pFForm, pCover, pCom );
    Mvc_CoverFree( pCom );
    return eNode;
}


/**Function*************************************************************

  Synopsis    [Internal recursive factoring procedure for the leaf case.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Edge_t Dec_FactorLF_rec( Dec_Graph_t * pFForm, Mvc_Cover_t * pCover, Mvc_Cover_t * pSimple )
{
    Dec_Man_t * pManDec = (Dec_Man_t *)Abc_FrameReadManDec();
    Vec_Int_t * vEdgeLits  = pManDec->vLits;
    Mvc_Cover_t * pDiv, * pQuo, * pRem;
    Dec_Edge_t eNodeDiv, eNodeQuo, eNodeRem;
    Dec_Edge_t eNodeAnd;

    // get the most often occurring literal
    pDiv = Mvc_CoverBestLiteralCover( pCover, pSimple );
    // divide the cover by the literal
    Mvc_CoverDivideByLiteral( pCover, pDiv, &pQuo, &pRem );
    // get the node pointer for the literal
    eNodeDiv = Dec_FactorTrivialCube( pFForm, pDiv, Mvc_CoverReadCubeHead(pDiv), vEdgeLits );
    Mvc_CoverFree( pDiv );
    // factor the quotient and remainder
    eNodeQuo = Dec_Factor_rec( pFForm, pQuo );
    Mvc_CoverFree( pQuo );
    eNodeAnd = Dec_GraphAddNodeAnd( pFForm, eNodeDiv, eNodeQuo );
    if ( Mvc_CoverReadCubeNum(pRem) == 0 )
    {
        Mvc_CoverFree( pRem );
        return eNodeAnd;
    }
    else
    {
        eNodeRem = Dec_Factor_rec( pFForm, pRem );
        Mvc_CoverFree( pRem );
        return Dec_GraphAddNodeOr( pFForm,  eNodeAnd, eNodeRem );
    }
}



/**Function*************************************************************

  Synopsis    [Factoring the cover, which has no algebraic divisors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Edge_t Dec_FactorTrivial( Dec_Graph_t * pFForm, Mvc_Cover_t * pCover )
{
    Dec_Man_t * pManDec = (Dec_Man_t *)Abc_FrameReadManDec();
    Vec_Int_t * vEdgeCubes = pManDec->vCubes;
    Vec_Int_t * vEdgeLits  = pManDec->vLits;
    Dec_Edge_t eNode;
    Mvc_Cube_t * pCube;
    // create the factored form for each cube
    Vec_IntClear( vEdgeCubes );
    Mvc_CoverForEachCube( pCover, pCube )
    {
        eNode = Dec_FactorTrivialCube( pFForm, pCover, pCube, vEdgeLits );
        Vec_IntPush( vEdgeCubes, Dec_EdgeToInt_(eNode) );
    }
    // balance the factored forms
    return Dec_FactorTrivialTree_rec( pFForm, (Dec_Edge_t *)vEdgeCubes->pArray, vEdgeCubes->nSize, 1 );
}

/**Function*************************************************************

  Synopsis    [Factoring the cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Edge_t Dec_FactorTrivialCube( Dec_Graph_t * pFForm, Mvc_Cover_t * pCover, Mvc_Cube_t * pCube, Vec_Int_t * vEdgeLits )
{
    Dec_Edge_t eNode;
    int iBit, Value;
    // create the factored form for each literal
    Vec_IntClear( vEdgeLits );
    Mvc_CubeForEachBit( pCover, pCube, iBit, Value )
        if ( Value )
        {
            eNode = Dec_EdgeCreate( iBit/2, iBit%2 ); // CST
            Vec_IntPush( vEdgeLits, Dec_EdgeToInt_(eNode) );
        }
    // balance the factored forms
    return Dec_FactorTrivialTree_rec( pFForm, (Dec_Edge_t *)vEdgeLits->pArray, vEdgeLits->nSize, 0 );
}
 
/**Function*************************************************************

  Synopsis    [Create the well-balanced tree of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Edge_t Dec_FactorTrivialTree_rec( Dec_Graph_t * pFForm, Dec_Edge_t * peNodes, int nNodes, int fNodeOr )
{
    Dec_Edge_t eNode1, eNode2;
    int nNodes1, nNodes2;

    if ( nNodes == 1 )
        return peNodes[0];

    // split the nodes into two parts
    nNodes1 = nNodes/2;
    nNodes2 = nNodes - nNodes1;
//    nNodes2 = nNodes/2;
//    nNodes1 = nNodes - nNodes2;

    // recursively construct the tree for the parts
    eNode1 = Dec_FactorTrivialTree_rec( pFForm, peNodes,           nNodes1, fNodeOr );
    eNode2 = Dec_FactorTrivialTree_rec( pFForm, peNodes + nNodes1, nNodes2, fNodeOr );

    if ( fNodeOr )
        return Dec_GraphAddNodeOr( pFForm, eNode1, eNode2 );
    else
        return Dec_GraphAddNodeAnd( pFForm, eNode1, eNode2 );
}



/**Function*************************************************************

  Synopsis    [Converts SOP into MVC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Dec_ConvertSopToMvc( char * pSop )
{
    Dec_Man_t * pManDec = (Dec_Man_t *)Abc_FrameReadManDec();
    Mvc_Manager_t * pMem = (Mvc_Manager_t *)pManDec->pMvcMem;
    Mvc_Cover_t * pMvc;
    Mvc_Cube_t * pMvcCube;
    char * pCube;
    int nVars, Value, v;

    // start the cover
    nVars = Abc_SopGetVarNum(pSop);
    assert( nVars > 0 );
    pMvc = Mvc_CoverAlloc( pMem, nVars * 2 );
    // check the logic function of the node
    Abc_SopForEachCube( pSop, nVars, pCube )
    {
        // create and add the cube
        pMvcCube = Mvc_CubeAlloc( pMvc );
        Mvc_CoverAddCubeTail( pMvc, pMvcCube );
        // fill in the literals
        Mvc_CubeBitFill( pMvcCube );
        Abc_CubeForEachVar( pCube, Value, v )
        {
            if ( Value == '0' )
                Mvc_CubeBitRemove( pMvcCube, v * 2 + 1 );
            else if ( Value == '1' )
                Mvc_CubeBitRemove( pMvcCube, v * 2 );
        }
    }
    return pMvc;
}

#ifdef ABC_USE_CUDD

/**Function*************************************************************

  Synopsis    [Verifies that the factoring is correct.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dec_FactorVerify( char * pSop, Dec_Graph_t * pFForm )
{
    extern DdNode * Abc_ConvertSopToBdd( DdManager * dd, char * pSop, DdNode ** pbVars );
    extern DdNode * Dec_GraphDeriveBdd( DdManager * dd, Dec_Graph_t * pGraph );
    DdManager * dd = (DdManager *)Abc_FrameReadManDd();
    DdNode * bFunc1, * bFunc2;
    int RetValue;
    bFunc1 = Abc_ConvertSopToBdd( dd, pSop, NULL );    Cudd_Ref( bFunc1 );
    bFunc2 = Dec_GraphDeriveBdd( dd, pFForm );   Cudd_Ref( bFunc2 );
//Extra_bddPrint( dd, bFunc1 ); printf("\n");
//Extra_bddPrint( dd, bFunc2 ); printf("\n");
    RetValue = (bFunc1 == bFunc2);
    if ( bFunc1 != bFunc2 )
    {
        int s;
        Extra_bddPrint( dd, bFunc1 ); printf("\n");
        Extra_bddPrint( dd, bFunc2 ); printf("\n");
        s  = 0;
    }
    Cudd_RecursiveDeref( dd, bFunc1 );
    Cudd_RecursiveDeref( dd, bFunc2 );
    return RetValue;
}

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

