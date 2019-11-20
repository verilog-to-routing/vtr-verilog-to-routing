/**CFile****************************************************************

  FileName    [abcFunc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Transformations between different functionality representations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcFunc.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "main.h"
#include "mio.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define ABC_MUX_CUBES   100000

static int Abc_ConvertZddToSop( DdManager * dd, DdNode * zCover, char * pSop, int nFanins, Vec_Str_t * vCube, int fPhase );
static DdNode * Abc_ConvertAigToBdd( DdManager * dd, Hop_Obj_t * pRoot);
static Hop_Obj_t * Abc_ConvertSopToAig( Hop_Man_t * pMan, char * pSop );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Converts the network from SOP to BDD representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkSopToBdd( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    DdManager * dd;
    int nFaninsMax, i;
 
    assert( Abc_NtkHasSop(pNtk) ); 

    // start the functionality manager
    nFaninsMax = Abc_NtkGetFaninMax( pNtk );
    if ( nFaninsMax == 0 )
        printf( "Warning: The network has only constant nodes.\n" );

    dd = Cudd_Init( nFaninsMax, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );

    // convert each node from SOP to BDD
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        assert( pNode->pData );
        pNode->pData = Abc_ConvertSopToBdd( dd, pNode->pData );
        if ( pNode->pData == NULL )
        {
            printf( "Abc_NtkSopToBdd: Error while converting SOP into BDD.\n" );
            return 0;
        }
        Cudd_Ref( pNode->pData );
    }

    Extra_MmFlexStop( pNtk->pManFunc );
    pNtk->pManFunc = dd;

    // update the network type
    pNtk->ntkFunc = ABC_FUNC_BDD;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Converts the node from SOP to BDD representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_ConvertSopToBdd( DdManager * dd, char * pSop )
{
    DdNode * bSum, * bCube, * bTemp, * bVar;
    char * pCube;
    int nVars, Value, v;

    // start the cover
    nVars = Abc_SopGetVarNum(pSop);
    bSum = Cudd_ReadLogicZero(dd);   Cudd_Ref( bSum );
    if ( Abc_SopIsExorType(pSop) )
    {
        for ( v = 0; v < nVars; v++ )
        {
            bSum  = Cudd_bddXor( dd, bTemp = bSum, Cudd_bddIthVar(dd, v) );   Cudd_Ref( bSum );
            Cudd_RecursiveDeref( dd, bTemp );
        }
    }
    else
    {
        // check the logic function of the node
        Abc_SopForEachCube( pSop, nVars, pCube )
        {
            bCube = Cudd_ReadOne(dd);   Cudd_Ref( bCube );
            Abc_CubeForEachVar( pCube, Value, v )
            {
                if ( Value == '0' )
                    bVar = Cudd_Not( Cudd_bddIthVar( dd, v ) );
                else if ( Value == '1' )
                    bVar = Cudd_bddIthVar( dd, v );
                else
                    continue;
                bCube  = Cudd_bddAnd( dd, bTemp = bCube, bVar );   Cudd_Ref( bCube );
                Cudd_RecursiveDeref( dd, bTemp );
            }
            bSum = Cudd_bddOr( dd, bTemp = bSum, bCube );   
            Cudd_Ref( bSum );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bCube );
        }
    }
    // complement the result if necessary
    bSum = Cudd_NotCond( bSum, !Abc_SopGetPhase(pSop) );
    Cudd_Deref( bSum );
    return bSum;
}

/**Function*************************************************************

  Synopsis    [Removes complemented SOP covers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkLogicMakeDirectSops( Abc_Ntk_t * pNtk )
{
    DdManager * dd;
    DdNode * bFunc;
    Vec_Str_t * vCube;
    Abc_Obj_t * pNode;
    int nFaninsMax, fFound, i;

    assert( Abc_NtkHasSop(pNtk) );

    // check if there are nodes with complemented SOPs
    fFound = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
        if ( Abc_SopIsComplement(pNode->pData) )
        {
            fFound = 1;
            break;
        }
    if ( !fFound )
        return;

    // start the BDD package
    nFaninsMax = Abc_NtkGetFaninMax( pNtk );
    if ( nFaninsMax == 0 )
        printf( "Warning: The network has only constant nodes.\n" );
    dd = Cudd_Init( nFaninsMax, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );

    // change the cover of negated nodes
    vCube = Vec_StrAlloc( 100 );
    Abc_NtkForEachNode( pNtk, pNode, i )
        if ( Abc_SopIsComplement(pNode->pData) )
        {
            bFunc = Abc_ConvertSopToBdd( dd, pNode->pData );  Cudd_Ref( bFunc );
            pNode->pData = Abc_ConvertBddToSop( pNtk->pManFunc, dd, bFunc, bFunc, Abc_ObjFaninNum(pNode), 0, vCube, 1 );
            Cudd_RecursiveDeref( dd, bFunc );
            assert( !Abc_SopIsComplement(pNode->pData) );
        }
    Vec_StrFree( vCube );
    Extra_StopManager( dd );
}





/**Function*************************************************************

  Synopsis    [Converts the network from BDD to SOP representation.]

  Description [If the flag is set to 1, forces the direct phase of all covers.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkBddToSop( Abc_Ntk_t * pNtk, int fDirect )
{
    Abc_Obj_t * pNode;
    Extra_MmFlex_t * pManNew;
    DdManager * dd = pNtk->pManFunc;
    DdNode * bFunc;
    Vec_Str_t * vCube;
    int i, fMode;

    if ( fDirect )
        fMode = 1;
    else
        fMode = -1;

    assert( Abc_NtkHasBdd(pNtk) );
    if ( dd->size > 0 )
    Cudd_zddVarsFromBddVars( dd, 2 );
    // create the new manager
    pManNew = Extra_MmFlexStart();

    // go through the objects
    vCube = Vec_StrAlloc( 100 );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        assert( pNode->pData );
        bFunc = pNode->pData;
        pNode->pNext = (Abc_Obj_t *)Abc_ConvertBddToSop( pManNew, dd, bFunc, bFunc, Abc_ObjFaninNum(pNode), 0, vCube, fMode );
        if ( pNode->pNext == NULL )
        {
            Extra_MmFlexStop( pManNew );
            Abc_NtkCleanNext( pNtk );
//            printf( "Converting from BDDs to SOPs has failed.\n" );
            Vec_StrFree( vCube );
            return 0;
        }
    }
    Vec_StrFree( vCube );

    // update the network type
    pNtk->ntkFunc = ABC_FUNC_SOP;
    // set the new manager
    pNtk->pManFunc = pManNew;
    // transfer from next to data
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        Cudd_RecursiveDeref( dd, pNode->pData );
        pNode->pData = pNode->pNext;
        pNode->pNext = NULL;
    }

    // check for remaining references in the package
    Extra_StopManager( dd );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Converts the node from BDD to SOP representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_ConvertBddToSop( Extra_MmFlex_t * pMan, DdManager * dd, DdNode * bFuncOn, DdNode * bFuncOnDc, int nFanins, int fAllPrimes, Vec_Str_t * vCube, int fMode )
{
    int fVerify = 0;
    char * pSop;
    DdNode * bFuncNew, * bCover, * zCover, * zCover0, * zCover1;
    int nCubes, nCubes0, nCubes1, fPhase;

    assert( bFuncOn == bFuncOnDc || Cudd_bddLeq( dd, bFuncOn, bFuncOnDc ) );
    if ( Cudd_IsConstant(bFuncOn) || Cudd_IsConstant(bFuncOnDc) )
    {
        if ( fMode == -1 ) // if the phase is not known, write constant 1
            fMode = 1;
        Vec_StrFill( vCube, nFanins, '-' );
        Vec_StrPush( vCube, '\0' );
        if ( pMan )
            pSop = Extra_MmFlexEntryFetch( pMan, nFanins + 4 );
        else
            pSop = ALLOC( char, nFanins + 4 );
        if ( bFuncOn == Cudd_ReadOne(dd) )
            sprintf( pSop, "%s %d\n", vCube->pArray, fMode );
        else
            sprintf( pSop, "%s %d\n", vCube->pArray, !fMode );
        return pSop;
    }


    if ( fMode == -1 )
    { // try both phases
        assert( fAllPrimes == 0 );

        // get the ZDD of the negative polarity
        bCover = Cudd_zddIsop( dd, Cudd_Not(bFuncOnDc), Cudd_Not(bFuncOn), &zCover0 );
        Cudd_Ref( zCover0 );
        Cudd_Ref( bCover );
        Cudd_RecursiveDeref( dd, bCover );
        nCubes0 = Abc_CountZddCubes( dd, zCover0 );

        // get the ZDD of the positive polarity
        bCover = Cudd_zddIsop( dd, bFuncOn, bFuncOnDc, &zCover1 );
        Cudd_Ref( zCover1 );
        Cudd_Ref( bCover );
        Cudd_RecursiveDeref( dd, bCover );
        nCubes1 = Abc_CountZddCubes( dd, zCover1 );

        // compare the number of cubes
        if ( nCubes1 <= nCubes0 )
        { // use positive polarity
            nCubes = nCubes1;
            zCover = zCover1;
            Cudd_RecursiveDerefZdd( dd, zCover0 );
            fPhase = 1;
        }
        else
        { // use negative polarity
            nCubes = nCubes0;
            zCover = zCover0;
            Cudd_RecursiveDerefZdd( dd, zCover1 );
            fPhase = 0;
        }
    }
    else if ( fMode == 0 )
    {
        // get the ZDD of the negative polarity
        if ( fAllPrimes )
        {
            zCover = Extra_zddPrimes( dd, Cudd_Not(bFuncOnDc) ); 
            Cudd_Ref( zCover );
        }
        else
        {
            bCover = Cudd_zddIsop( dd, Cudd_Not(bFuncOnDc), Cudd_Not(bFuncOn), &zCover );
            Cudd_Ref( zCover );
            Cudd_Ref( bCover );
            Cudd_RecursiveDeref( dd, bCover );
        }
        nCubes = Abc_CountZddCubes( dd, zCover );
        fPhase = 0;
    }
    else if ( fMode == 1 )
    {
        // get the ZDD of the positive polarity
        if ( fAllPrimes )
        {
            zCover = Extra_zddPrimes( dd, bFuncOnDc ); 
            Cudd_Ref( zCover );
        }
        else
        {
            bCover = Cudd_zddIsop( dd, bFuncOn, bFuncOnDc, &zCover );
            Cudd_Ref( zCover );
            Cudd_Ref( bCover );
            Cudd_RecursiveDeref( dd, bCover );
        }
        nCubes = Abc_CountZddCubes( dd, zCover );
        fPhase = 1;
    }
    else
    {
        assert( 0 );
    }

    if ( nCubes > ABC_MUX_CUBES )
    {
        Cudd_RecursiveDerefZdd( dd, zCover );
        printf( "The number of cubes exceeded the predefined limit (%d).\n", ABC_MUX_CUBES );
        return NULL;
    }

    // allocate memory for the cover
    if ( pMan )
        pSop = Extra_MmFlexEntryFetch( pMan, (nFanins + 3) * nCubes + 1 );
    else 
        pSop = ALLOC( char, (nFanins + 3) * nCubes + 1 );
    pSop[(nFanins + 3) * nCubes] = 0;
    // create the SOP
    Vec_StrFill( vCube, nFanins, '-' );
    Vec_StrPush( vCube, '\0' );
    Abc_ConvertZddToSop( dd, zCover, pSop, nFanins, vCube, fPhase );
    Cudd_RecursiveDerefZdd( dd, zCover );

    // verify
    if ( fVerify )
    {
        bFuncNew = Abc_ConvertSopToBdd( dd, pSop );  Cudd_Ref( bFuncNew );
        if ( bFuncOn == bFuncOnDc )
        {
            if ( bFuncNew != bFuncOn )
                printf( "Verification failed.\n" );
        }
        else
        {
            if ( !Cudd_bddLeq(dd, bFuncOn, bFuncNew) || !Cudd_bddLeq(dd, bFuncNew, bFuncOnDc) )
                printf( "Verification failed.\n" );
        }
        Cudd_RecursiveDeref( dd, bFuncNew );
    }
    return pSop;
}

/**Function*************************************************************

  Synopsis    [Derive the SOP from the ZDD representation of the cubes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ConvertZddToSop_rec( DdManager * dd, DdNode * zCover, char * pSop, int nFanins, Vec_Str_t * vCube, int fPhase, int * pnCubes )
{
    DdNode * zC0, * zC1, * zC2;
    int Index;

    if ( zCover == dd->zero )
        return;
    if ( zCover == dd->one )
    {
        char * pCube;
        pCube = pSop + (*pnCubes) * (nFanins + 3);
        sprintf( pCube, "%s %d\n", vCube->pArray, fPhase );
        (*pnCubes)++;
        return;
    }
    Index = zCover->index/2;
    assert( Index < nFanins );
    extraDecomposeCover( dd, zCover, &zC0, &zC1, &zC2 );
    vCube->pArray[Index] = '0';
    Abc_ConvertZddToSop_rec( dd, zC0, pSop, nFanins, vCube, fPhase, pnCubes );
    vCube->pArray[Index] = '1';
    Abc_ConvertZddToSop_rec( dd, zC1, pSop, nFanins, vCube, fPhase, pnCubes );
    vCube->pArray[Index] = '-';
    Abc_ConvertZddToSop_rec( dd, zC2, pSop, nFanins, vCube, fPhase, pnCubes );
}

/**Function*************************************************************

  Synopsis    [Derive the BDD for the function in the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ConvertZddToSop( DdManager * dd, DdNode * zCover, char * pSop, int nFanins, Vec_Str_t * vCube, int fPhase )
{
    int nCubes = 0;
    Abc_ConvertZddToSop_rec( dd, zCover, pSop, nFanins, vCube, fPhase, &nCubes );
    return nCubes;
}


/**Function*************************************************************

  Synopsis    [Computes the SOPs of the negative and positive phase of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeBddToCnf( Abc_Obj_t * pNode, Extra_MmFlex_t * pMmMan, Vec_Str_t * vCube, int fAllPrimes, char ** ppSop0, char ** ppSop1 )
{
    assert( Abc_NtkHasBdd(pNode->pNtk) ); 
    *ppSop0 = Abc_ConvertBddToSop( pMmMan, pNode->pNtk->pManFunc, pNode->pData, pNode->pData, Abc_ObjFaninNum(pNode), fAllPrimes, vCube, 0 );
    *ppSop1 = Abc_ConvertBddToSop( pMmMan, pNode->pNtk->pManFunc, pNode->pData, pNode->pData, Abc_ObjFaninNum(pNode), fAllPrimes, vCube, 1 );
}




/**Function*************************************************************

  Synopsis    [Count the number of paths in the ZDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_CountZddCubes_rec( DdManager * dd, DdNode * zCover, int * pnCubes )
{
    DdNode * zC0, * zC1, * zC2;
    if ( zCover == dd->zero )
        return;
    if ( zCover == dd->one )
    {
        (*pnCubes)++;
        return;
    }
    if ( (*pnCubes) > ABC_MUX_CUBES )
        return;
    extraDecomposeCover( dd, zCover, &zC0, &zC1, &zC2 );
    Abc_CountZddCubes_rec( dd, zC0, pnCubes );
    Abc_CountZddCubes_rec( dd, zC1, pnCubes );
    Abc_CountZddCubes_rec( dd, zC2, pnCubes );
}

/**Function*************************************************************

  Synopsis    [Count the number of paths in the ZDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_CountZddCubes( DdManager * dd, DdNode * zCover )
{
    int nCubes = 0;
    Abc_CountZddCubes_rec( dd, zCover, &nCubes );
    return nCubes;
}


/**Function*************************************************************

  Synopsis    [Converts the network from SOP to AIG representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkSopToAig( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    Hop_Man_t * pMan;
    int i;

    assert( Abc_NtkHasSop(pNtk) ); 

    // start the functionality manager
    pMan = Hop_ManStart();

    // convert each node from SOP to BDD
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        assert( pNode->pData );
        pNode->pData = Abc_ConvertSopToAig( pMan, pNode->pData );
        if ( pNode->pData == NULL )
        {
            printf( "Abc_NtkSopToAig: Error while converting SOP into AIG.\n" );
            return 0;
        }
    }
    Extra_MmFlexStop( pNtk->pManFunc );
    pNtk->pManFunc = pMan;

    // update the network type
    pNtk->ntkFunc = ABC_FUNC_AIG;
    return 1;
}


/**Function*************************************************************

  Synopsis    [Strashes one logic node using its SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Abc_ConvertSopToAigInternal( Hop_Man_t * pMan, char * pSop )
{
    Hop_Obj_t * pAnd, * pSum;
    int i, Value, nFanins;
    char * pCube;
    // get the number of variables
    nFanins = Abc_SopGetVarNum(pSop);
    // go through the cubes of the node's SOP
    pSum = Hop_ManConst0(pMan);
    Abc_SopForEachCube( pSop, nFanins, pCube )
    {
        // create the AND of literals
        pAnd = Hop_ManConst1(pMan);
        Abc_CubeForEachVar( pCube, Value, i )
        {
            if ( Value == '1' )
                pAnd = Hop_And( pMan, pAnd, Hop_IthVar(pMan,i) );
            else if ( Value == '0' )
                pAnd = Hop_And( pMan, pAnd, Hop_Not(Hop_IthVar(pMan,i)) );
        }
        // add to the sum of cubes
        pSum = Hop_Or( pMan, pSum, pAnd );
    }
    // decide whether to complement the result
    if ( Abc_SopIsComplement(pSop) )
        pSum = Hop_Not(pSum);
    return pSum;
}

/**Function*************************************************************

  Synopsis    [Converts the network from AIG to BDD representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Abc_ConvertSopToAig( Hop_Man_t * pMan, char * pSop )
{
    extern Hop_Obj_t * Dec_GraphFactorSop( Hop_Man_t * pMan, char * pSop );
    int fUseFactor = 1;
    // consider the constant node
    if ( Abc_SopGetVarNum(pSop) == 0 )
        return Hop_NotCond( Hop_ManConst1(pMan), Abc_SopIsConst0(pSop) );
    // consider the special case of EXOR function
    if ( Abc_SopIsExorType(pSop) )
        return Hop_NotCond( Hop_CreateExor(pMan, Abc_SopGetVarNum(pSop)), Abc_SopIsComplement(pSop) );
    // decide when to use factoring
    if ( fUseFactor && Abc_SopGetVarNum(pSop) > 2 && Abc_SopGetCubeNum(pSop) > 1 )
        return Dec_GraphFactorSop( pMan, pSop );
    return Abc_ConvertSopToAigInternal( pMan, pSop );
}

/**Function*************************************************************

  Synopsis    [Converts the network from AIG to BDD representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkAigToBdd( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    Hop_Man_t * pMan;
    DdManager * dd;
    int nFaninsMax, i;

    assert( Abc_NtkHasAig(pNtk) ); 

    // start the functionality manager
    nFaninsMax = Abc_NtkGetFaninMax( pNtk );
    if ( nFaninsMax == 0 )
        printf( "Warning: The network has only constant nodes.\n" );

    dd = Cudd_Init( nFaninsMax, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );

    // set the mapping of elementary AIG nodes into the elementary BDD nodes
    pMan = pNtk->pManFunc;
    assert( Hop_ManPiNum(pMan) >= nFaninsMax ); 
    for ( i = 0; i < nFaninsMax; i++ )
    {
        Hop_ManPi(pMan, i)->pData = Cudd_bddIthVar(dd, i);
        Cudd_Ref( Hop_ManPi(pMan, i)->pData );
    }

    // convert each node from SOP to BDD
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        assert( pNode->pData );
        pNode->pData = Abc_ConvertAigToBdd( dd, pNode->pData );
        if ( pNode->pData == NULL )
        {
            printf( "Abc_NtkSopToBdd: Error while converting SOP into BDD.\n" );
            return 0;
        }
        Cudd_Ref( pNode->pData );
    }

    // dereference intermediate BDD nodes
    for ( i = 0; i < nFaninsMax; i++ )
        Cudd_RecursiveDeref( dd, Hop_ManPi(pMan, i)->pData );

    Hop_ManStop( pNtk->pManFunc );
    pNtk->pManFunc = dd;

    // update the network type
    pNtk->ntkFunc = ABC_FUNC_BDD;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Construct BDDs and mark AIG nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ConvertAigToBdd_rec1( DdManager * dd, Hop_Obj_t * pObj )
{
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || Hop_ObjIsMarkA(pObj) )
        return;
    Abc_ConvertAigToBdd_rec1( dd, Hop_ObjFanin0(pObj) ); 
    Abc_ConvertAigToBdd_rec1( dd, Hop_ObjFanin1(pObj) );
    pObj->pData = Cudd_bddAnd( dd, (DdNode *)Hop_ObjChild0Copy(pObj), (DdNode *)Hop_ObjChild1Copy(pObj) ); 
    Cudd_Ref( pObj->pData );
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
}

/**Function*************************************************************

  Synopsis    [Dereference BDDs and unmark AIG nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ConvertAigToBdd_rec2( DdManager * dd, Hop_Obj_t * pObj )
{
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || !Hop_ObjIsMarkA(pObj) )
        return;
    Abc_ConvertAigToBdd_rec2( dd, Hop_ObjFanin0(pObj) ); 
    Abc_ConvertAigToBdd_rec2( dd, Hop_ObjFanin1(pObj) );
    Cudd_RecursiveDeref( dd, pObj->pData );
    pObj->pData = NULL;
    assert( Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjClearMarkA( pObj );
}

/**Function*************************************************************

  Synopsis    [Converts the network from AIG to BDD representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_ConvertAigToBdd( DdManager * dd, Hop_Obj_t * pRoot )
{
    DdNode * bFunc;
    // check the case of a constant
    if ( Hop_ObjIsConst1( Hop_Regular(pRoot) ) )
        return Cudd_NotCond( Cudd_ReadOne(dd), Hop_IsComplement(pRoot) );
    // construct BDD
    Abc_ConvertAigToBdd_rec1( dd, Hop_Regular(pRoot) );
    // hold on to the result
    bFunc = Cudd_NotCond( Hop_Regular(pRoot)->pData, Hop_IsComplement(pRoot) );  Cudd_Ref( bFunc );
    // dereference BDD
    Abc_ConvertAigToBdd_rec2( dd, Hop_Regular(pRoot) );
    // return the result
    Cudd_Deref( bFunc );
    return bFunc;
}



/**Function*************************************************************

  Synopsis    [Construct BDDs and mark AIG nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ConvertAigToTruth_rec1( Hop_Obj_t * pObj )
{
    int Counter = 0;
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || Hop_ObjIsMarkA(pObj) )
        return 0;
    Counter += Abc_ConvertAigToTruth_rec1( Hop_ObjFanin0(pObj) ); 
    Counter += Abc_ConvertAigToTruth_rec1( Hop_ObjFanin1(pObj) );
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
    return Counter + 1;
}

/**Function*************************************************************

  Synopsis    [Computes truth table of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Abc_ConvertAigToTruth_rec2( Hop_Obj_t * pObj, Vec_Int_t * vTruth, int nWords )
{
    unsigned * pTruth, * pTruth0, * pTruth1;
    int i;
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || !Hop_ObjIsMarkA(pObj) )
        return pObj->pData;
    // compute the truth tables of the fanins
    pTruth0 = Abc_ConvertAigToTruth_rec2( Hop_ObjFanin0(pObj), vTruth, nWords );
    pTruth1 = Abc_ConvertAigToTruth_rec2( Hop_ObjFanin1(pObj), vTruth, nWords );
    // creat the truth table of the node
    pTruth  = Vec_IntFetch( vTruth, nWords );
    if ( Hop_ObjIsExor(pObj) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] ^ pTruth1[i];
    else if ( !Hop_ObjFaninC0(pObj) && !Hop_ObjFaninC1(pObj) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] & pTruth1[i];
    else if ( !Hop_ObjFaninC0(pObj) && Hop_ObjFaninC1(pObj) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] & ~pTruth1[i];
    else if ( Hop_ObjFaninC0(pObj) && !Hop_ObjFaninC1(pObj) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ~pTruth0[i] & pTruth1[i];
    else // if ( Hop_ObjFaninC0(pObj) && Hop_ObjFaninC1(pObj) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ~pTruth0[i] & ~pTruth1[i];
    assert( Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjClearMarkA( pObj );
    pObj->pData = pTruth;
    return pTruth;
}

/**Function*************************************************************

  Synopsis    [Computes truth table of the node.]

  Description [Assumes that the structural support is no more than 8 inputs.
  Uses array vTruth to store temporary truth tables. The returned pointer should 
  be used immediately.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Abc_ConvertAigToTruth( Hop_Man_t * p, Hop_Obj_t * pRoot, int nVars, Vec_Int_t * vTruth, int fMsbFirst )
{
    static unsigned uTruths[8][8] = { // elementary truth tables
        { 0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA },
        { 0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC },
        { 0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0 },
        { 0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00 },
        { 0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000 }, 
        { 0x00000000,0xFFFFFFFF,0x00000000,0xFFFFFFFF,0x00000000,0xFFFFFFFF,0x00000000,0xFFFFFFFF }, 
        { 0x00000000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00000000,0xFFFFFFFF,0xFFFFFFFF }, 
        { 0x00000000,0x00000000,0x00000000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF } 
    };
    Hop_Obj_t * pObj;
    unsigned * pTruth, * pTruth2;
    int i, nWords, nNodes;
    Vec_Ptr_t * vTtElems;

    // if the number of variables is more than 8, allocate truth tables
    if ( nVars > 8 )
        vTtElems = Vec_PtrAllocTruthTables( nVars );
    else
        vTtElems = NULL;

    // clear the data fields and set marks
    nNodes = Abc_ConvertAigToTruth_rec1( pRoot );
    // prepare memory
    nWords = Hop_TruthWordNum( nVars );
    Vec_IntClear( vTruth );
    Vec_IntGrow( vTruth, nWords * (nNodes+1) );
    pTruth = Vec_IntFetch( vTruth, nWords );
    // check the case of a constant
    if ( Hop_ObjIsConst1( Hop_Regular(pRoot) ) )
    {
        assert( nNodes == 0 );
        if ( Hop_IsComplement(pRoot) )
            Extra_TruthClear( pTruth, nVars );
        else
            Extra_TruthFill( pTruth, nVars );
        return pTruth;
    }
    // set elementary truth tables at the leaves
    assert( nVars <= Hop_ManPiNum(p) );
//    assert( Hop_ManPiNum(p) <= 8 ); 
    if ( fMsbFirst )
    {
        Hop_ManForEachPi( p, pObj, i )
        {
            if ( vTtElems )
                pObj->pData = Vec_PtrEntry(vTtElems, nVars-1-i);
            else               
                pObj->pData = (void *)uTruths[nVars-1-i];
        }
    }
    else
    {
        Hop_ManForEachPi( p, pObj, i )
        {
            if ( vTtElems )
                pObj->pData = Vec_PtrEntry(vTtElems, i);
            else               
                pObj->pData = (void *)uTruths[i];
        }
    }
    // clear the marks and compute the truth table
    pTruth2 = Abc_ConvertAigToTruth_rec2( pRoot, vTruth, nWords );
    // copy the result
    Extra_TruthCopy( pTruth, pTruth2, nVars );
    if ( vTtElems )
        Vec_PtrFree( vTtElems );
    return pTruth;
}


/**Function*************************************************************

  Synopsis    [Construct BDDs and mark AIG nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ConvertAigToAig_rec( Abc_Ntk_t * pNtkAig, Hop_Obj_t * pObj )
{
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || Hop_ObjIsMarkA(pObj) )
        return;
    Abc_ConvertAigToAig_rec( pNtkAig, Hop_ObjFanin0(pObj) ); 
    Abc_ConvertAigToAig_rec( pNtkAig, Hop_ObjFanin1(pObj) );
    pObj->pData = Abc_AigAnd( pNtkAig->pManFunc, (Abc_Obj_t *)Hop_ObjChild0Copy(pObj), (Abc_Obj_t *)Hop_ObjChild1Copy(pObj) ); 
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
}

/**Function*************************************************************

  Synopsis    [Converts the network from AIG to BDD representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_ConvertAigToAig( Abc_Ntk_t * pNtkAig, Abc_Obj_t * pObjOld )
{
    Hop_Man_t * pHopMan;
    Hop_Obj_t * pRoot;
    Abc_Obj_t * pFanin;
    int i;
    // get the local AIG
    pHopMan = pObjOld->pNtk->pManFunc;
    pRoot = pObjOld->pData;
    // check the case of a constant
    if ( Hop_ObjIsConst1( Hop_Regular(pRoot) ) )
        return Abc_ObjNotCond( Abc_AigConst1(pNtkAig), Hop_IsComplement(pRoot) );
    // assign the fanin nodes
    Abc_ObjForEachFanin( pObjOld, pFanin, i )
    {
        assert( pFanin->pCopy != NULL );
        Hop_ManPi(pHopMan, i)->pData = pFanin->pCopy;
    }
    // construct the AIG
    Abc_ConvertAigToAig_rec( pNtkAig, Hop_Regular(pRoot) );
    Hop_ConeUnmark_rec( Hop_Regular(pRoot) );
    // return the result
    return Abc_ObjNotCond( Hop_Regular(pRoot)->pData, Hop_IsComplement(pRoot) );  
}


/**Function*************************************************************

  Synopsis    [Unmaps the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMapToSop( Abc_Ntk_t * pNtk )
{
    extern void * Abc_FrameReadLibGen();                    
    Abc_Obj_t * pNode;
    char * pSop;
    int i;

    assert( Abc_NtkHasMapping(pNtk) );
    // update the functionality manager
    assert( pNtk->pManFunc == Abc_FrameReadLibGen() );
    pNtk->pManFunc = Extra_MmFlexStart();
    pNtk->ntkFunc  = ABC_FUNC_SOP;
    // update the nodes
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        pSop = Mio_GateReadSop(pNode->pData);
        assert( Abc_SopGetVarNum(pSop) == Abc_ObjFaninNum(pNode) );
        pNode->pData = Abc_SopRegister( pNtk->pManFunc, pSop );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Converts SOP functions into BLIF-MV functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkSopToBlifMv( Abc_Ntk_t * pNtk )
{
    return 1;
}

/**Function*************************************************************

  Synopsis    [Convers logic network to the SOP form.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkToSop( Abc_Ntk_t * pNtk, int fDirect )
{
    assert( !Abc_NtkIsStrash(pNtk) );
    if ( Abc_NtkHasSop(pNtk) )
    {
        if ( !fDirect )
            return 1;
        if ( !Abc_NtkSopToBdd(pNtk) )
            return 0;
        return Abc_NtkBddToSop(pNtk, fDirect);
    }
    if ( Abc_NtkHasMapping(pNtk) )
        return Abc_NtkMapToSop(pNtk);
    if ( Abc_NtkHasBdd(pNtk) )
        return Abc_NtkBddToSop(pNtk, fDirect);
    if ( Abc_NtkHasAig(pNtk) )
    {
        if ( !Abc_NtkAigToBdd(pNtk) )
            return 0;
        return Abc_NtkBddToSop(pNtk, fDirect);
    }
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Convers logic network to the SOP form.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkToBdd( Abc_Ntk_t * pNtk )
{
    assert( !Abc_NtkIsStrash(pNtk) );
    if ( Abc_NtkHasBdd(pNtk) )
        return 1;
    if ( Abc_NtkHasMapping(pNtk) )
    {
        Abc_NtkMapToSop(pNtk);
        return Abc_NtkSopToBdd(pNtk);
    }
    if ( Abc_NtkHasSop(pNtk) )
        return Abc_NtkSopToBdd(pNtk);
    if ( Abc_NtkHasAig(pNtk) )
        return Abc_NtkAigToBdd(pNtk);
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Convers logic network to the SOP form.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkToAig( Abc_Ntk_t * pNtk )
{
    assert( !Abc_NtkIsStrash(pNtk) );
    if ( Abc_NtkHasAig(pNtk) )
        return 1;
    if ( Abc_NtkHasMapping(pNtk) )
    {
        Abc_NtkMapToSop(pNtk);
        return Abc_NtkSopToAig(pNtk);
    }
    if ( Abc_NtkHasBdd(pNtk) )
    {
        if ( !Abc_NtkBddToSop(pNtk,0) )
            return 0;
        return Abc_NtkSopToAig(pNtk);
    }
    if ( Abc_NtkHasSop(pNtk) )
        return Abc_NtkSopToAig(pNtk);
    assert( 0 );
    return 0;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


