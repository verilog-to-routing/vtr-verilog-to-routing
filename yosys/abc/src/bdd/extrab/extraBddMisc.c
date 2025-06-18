/**CFile****************************************************************

  FileName    [extraBddMisc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [DD-based utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: extraBddMisc.c,v 1.4 2005/10/04 00:19:54 alanmi Exp $]

***********************************************************************/

#include "extraBdd.h"

ABC_NAMESPACE_IMPL_START


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

// file "extraDdTransfer.c"
static DdNode * extraTransferPermuteRecur( DdManager * ddS, DdManager * ddD, DdNode * f, st__table * table, int * Permute );
static DdNode * extraTransferPermute( DdManager * ddS, DdManager * ddD, DdNode * f, int * Permute );
static DdNode * cuddBddPermuteRecur ARGS( ( DdManager * manager, DdHashTable * table, DdNode * node, int *permut ) );

static DdNode * extraBddAndPermute( DdHashTable * table, DdManager * ddF, DdNode * bF, DdManager * ddG, DdNode * bG, int * pPermute );

// file "cuddUtils.c"
void ddSupportStep2(DdNode *f, int *support);
void ddClearFlag2(DdNode *f);

static DdNode* extraZddPrimes( DdManager *dd, DdNode* F );

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Convert a {A,B}DD from a manager to another with variable remapping.]

  Description [Convert a {A,B}DD from a manager to another one. The orders of the
  variables in the two managers may be different. Returns a
  pointer to the {A,B}DD in the destination manager if successful; NULL
  otherwise. The i-th entry in the array Permute tells what is the index
  of the i-th variable from the old manager in the new manager.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode * Extra_TransferPermute( DdManager * ddSource, DdManager * ddDestination, DdNode * f, int * Permute )
{
    DdNode * bRes;
    do
    {
        ddDestination->reordered = 0;
        bRes = extraTransferPermute( ddSource, ddDestination, f, Permute );
    }
    while ( ddDestination->reordered == 1 );
    return ( bRes );

}                               /* end of Extra_TransferPermute */

/**Function********************************************************************

  Synopsis    [Transfers the BDD from one manager into another level by level.]

  Description [Transfers the BDD from one manager into another while
  preserving the correspondence between variables level by level.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode * Extra_TransferLevelByLevel( DdManager * ddSource, DdManager * ddDestination, DdNode * f )
{
    DdNode * bRes;
    int * pPermute;
    int nMin, nMax, i;

    nMin = ddMin(ddSource->size, ddDestination->size);
    nMax = ddMax(ddSource->size, ddDestination->size);
    pPermute = ABC_ALLOC( int, nMax );
    // set up the variable permutation
    for ( i = 0; i < nMin; i++ )
        pPermute[ ddSource->invperm[i] ] = ddDestination->invperm[i];
    if ( ddSource->size > ddDestination->size )
    {
        for (      ; i < nMax; i++ )
            pPermute[ ddSource->invperm[i] ] = -1;
    }
    bRes = Extra_TransferPermute( ddSource, ddDestination, f, pPermute );
    ABC_FREE( pPermute );
    return bRes;
}

/**Function********************************************************************

  Synopsis    [Remaps the function to depend on the topmost variables on the manager.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_bddRemapUp(
  DdManager * dd,
  DdNode * bF )
{
    int * pPermute;
    DdNode * bSupp, * bTemp, * bRes;
    int Counter;

    pPermute = ABC_ALLOC( int, dd->size );

    // get support
    bSupp = Cudd_Support( dd, bF );    Cudd_Ref( bSupp );

    // create the variable map
    // to remap the DD into the upper part of the manager
    Counter = 0;
    for ( bTemp = bSupp; bTemp != dd->one; bTemp = cuddT(bTemp) )
        pPermute[bTemp->index] = dd->invperm[Counter++];

    // transfer the BDD and remap it
    bRes = Cudd_bddPermute( dd, bF, pPermute );  Cudd_Ref( bRes );

    // remove support
    Cudd_RecursiveDeref( dd, bSupp );

    // return
    Cudd_Deref( bRes );
    ABC_FREE( pPermute );
    return bRes;
}

/**Function********************************************************************

  Synopsis    [Moves the BDD by the given number of variables up or down.]

  Description []

  SideEffects []

  SeeAlso     [Extra_bddShift]

******************************************************************************/
DdNode * Extra_bddMove( 
  DdManager * dd,   /* the DD manager */
  DdNode * bF,
  int nVars) 
{
    DdNode * res;
    DdNode * bVars;
    if ( nVars == 0 )
        return bF;
    if ( Cudd_IsConstant(bF) )
        return bF;
    assert( nVars <= dd->size );
    if ( nVars > 0 )
        bVars = dd->vars[nVars];
    else
        bVars = Cudd_Not(dd->vars[-nVars]);

    do {
        dd->reordered = 0;
        res = extraBddMove( dd, bF, bVars );
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_bddMove */

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_StopManager( DdManager * dd )
{
    int RetValue;
    // check for remaining references in the package
    RetValue = Cudd_CheckZeroRef( dd );
    if ( RetValue > 10 )
//    if ( RetValue )
        printf( "\nThe number of referenced nodes = %d\n\n", RetValue );
//  Cudd_PrintInfo( dd, stdout );
    Cudd_Quit( dd );
}

/**Function********************************************************************

  Synopsis    [Outputs the BDD in a readable format.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
void Extra_bddPrint( DdManager * dd, DdNode * F )
{
    DdGen * Gen;
    int * Cube;
    CUDD_VALUE_TYPE Value;
    int nVars = dd->size;
    int fFirstCube = 1;
    int i;

    if ( F == NULL )
    {
        printf("NULL");
        return;
    }
    if ( F == b0 )
    {
        printf("Constant 0");
        return;
    }
    if ( F == b1 )
    {
        printf("Constant 1");
        return;
    }

    Cudd_ForeachCube( dd, F, Gen, Cube, Value )
    {
        if ( fFirstCube )
            fFirstCube = 0;
        else
//          Output << " + ";
            printf( " + " );

        for ( i = 0; i < nVars; i++ )
            if ( Cube[i] == 0 )
                printf( "[%d]'", i );
//              printf( "%c'", (char)('a'+i) );
            else if ( Cube[i] == 1 )
                printf( "[%d]", i );
//              printf( "%c", (char)('a'+i) );
    }

//  printf("\n");
}

/**Function********************************************************************

  Synopsis    [Outputs the BDD in a readable format.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
void Extra_bddPrintSupport( DdManager * dd, DdNode * F )
{
    DdNode * bSupp;
    bSupp = Cudd_Support( dd, F );   Cudd_Ref( bSupp );
    Extra_bddPrint( dd, bSupp );
    Cudd_RecursiveDeref( dd, bSupp );
}

/**Function********************************************************************

  Synopsis    [Returns the size of the support.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_bddSuppSize( DdManager * dd, DdNode * bSupp )
{
    int Counter = 0;
    while ( bSupp != b1 )
    {
        assert( !Cudd_IsComplement(bSupp) );
        assert( cuddE(bSupp) == b0 );

        bSupp = cuddT(bSupp);
        Counter++;
    }
    return Counter;
}

/**Function********************************************************************

  Synopsis    [Returns 1 if the support contains the given BDD variable.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_bddSuppContainVar( DdManager * dd, DdNode * bS, DdNode * bVar )   
{ 
    for( ; bS != b1; bS = cuddT(bS) )
        if ( bS->index == bVar->index )
            return 1;
    return 0;
}

/**Function********************************************************************

  Synopsis    [Returns 1 if two supports represented as BDD cubes are overlapping.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_bddSuppOverlapping( DdManager * dd, DdNode * S1, DdNode * S2 )
{
    while ( S1->index != CUDD_CONST_INDEX && S2->index != CUDD_CONST_INDEX )
    {
        // if the top vars are the same, they intersect
        if ( S1->index == S2->index )
            return 1;
        // if the top vars are different, skip the one, which is higher
        if ( dd->perm[S1->index] < dd->perm[S2->index] )
            S1 = cuddT(S1);
        else
            S2 = cuddT(S2);
    }
    return 0;
}

/**Function********************************************************************

  Synopsis    [Returns the number of different vars in two supports.]

  Description [Counts the number of variables that appear in one support and 
  does not appear in other support. If the number exceeds DiffMax, returns DiffMax.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_bddSuppDifferentVars( DdManager * dd, DdNode * S1, DdNode * S2, int DiffMax )
{
    int Result = 0;
    while ( S1->index != CUDD_CONST_INDEX && S2->index != CUDD_CONST_INDEX )
    {
        // if the top vars are the same, this var is the same
        if ( S1->index == S2->index )
        {
            S1 = cuddT(S1);
            S2 = cuddT(S2);
            continue;
        }
        // the top var is different
        Result++;

        if ( Result >= DiffMax )
            return DiffMax;

        // if the top vars are different, skip the one, which is higher
        if ( dd->perm[S1->index] < dd->perm[S2->index] )
            S1 = cuddT(S1);
        else
            S2 = cuddT(S2);
    }

    // consider the remaining variables
    if ( S1->index != CUDD_CONST_INDEX )
        Result += Extra_bddSuppSize(dd,S1);
    else if ( S2->index != CUDD_CONST_INDEX )
        Result += Extra_bddSuppSize(dd,S2);

    if ( Result >= DiffMax )
        return DiffMax;
    return Result;
}


/**Function********************************************************************

  Synopsis    [Checks the support containment.]

  Description [This function returns 1 if one support is contained in another.
  In this case, bLarge (bSmall) is assigned to point to the larger (smaller) support.
  If the supports are identical, return 0 and does not assign the supports!]

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_bddSuppCheckContainment( DdManager * dd, DdNode * bL, DdNode * bH, DdNode ** bLarge, DdNode ** bSmall )
{
    DdNode * bSL = bL;
    DdNode * bSH = bH;
    int fLcontainsH = 1;
    int fHcontainsL = 1;
    int TopVar;
    
    if ( bSL == bSH )
        return 0;

    while ( bSL != b1 || bSH != b1 )
    {
        if ( bSL == b1 )
        { // Low component has no vars; High components has some vars
            fLcontainsH = 0;
            if ( fHcontainsL == 0 )
                return 0;
            else
                break;
        }

        if ( bSH == b1 )
        { // similarly
            fHcontainsL = 0;
            if ( fLcontainsH == 0 )
                return 0;
            else
                break;
        }

        // determine the topmost var of the supports by comparing their levels
        if ( dd->perm[bSL->index] < dd->perm[bSH->index] )
            TopVar = bSL->index;
        else
            TopVar = bSH->index;

        if ( TopVar == bSL->index && TopVar == bSH->index ) 
        { // they are on the same level
            // it does not tell us anything about their containment
            // skip this var
            bSL = cuddT(bSL);
            bSH = cuddT(bSH);
        }
        else if ( TopVar == bSL->index ) // and TopVar != bSH->index
        { // Low components is higher and contains more vars
            // it is not possible that High component contains Low
            fHcontainsL = 0;
            // skip this var
            bSL = cuddT(bSL);
        }
        else // if ( TopVar == bSH->index ) // and TopVar != bSL->index
        { // similarly
            fLcontainsH = 0;
            // skip this var
            bSH = cuddT(bSH);
        }

        // check the stopping condition
        if ( !fHcontainsL && !fLcontainsH )
            return 0;
    }
    // only one of them can be true at the same time
    assert( !fHcontainsL || !fLcontainsH );
    if ( fHcontainsL )
    {
        *bLarge = bH;
        *bSmall = bL;
    }
    else // fLcontainsH
    {
        *bLarge = bL;
        *bSmall = bH;
    }
    return 1;
}


/**Function********************************************************************

  Synopsis    [Finds variables on which the DD depends and returns them as am array.]

  Description [Finds the variables on which the DD depends. Returns an array 
  with entries set to 1 for those variables that belong to the support; 
  NULL otherwise. The array is allocated by the user and should have at least
  as many entries as the maximum number of variables in BDD and ZDD parts of 
  the manager.]

  SideEffects [None]

  SeeAlso     [Cudd_Support Cudd_VectorSupport Cudd_ClassifySupport]

******************************************************************************/
int * 
Extra_SupportArray(
  DdManager * dd, /* manager */
  DdNode * f,     /* DD whose support is sought */
  int * support ) /* array allocated by the user */
{
    int i, size;

    /* Initialize support array for ddSupportStep. */
    size = ddMax(dd->size, dd->sizeZ);
    for (i = 0; i < size; i++) 
        support[i] = 0;
    
    /* Compute support and clean up markers. */
    ddSupportStep2(Cudd_Regular(f),support);
    ddClearFlag2(Cudd_Regular(f));

    return(support);

} /* end of Extra_SupportArray */

/**Function********************************************************************

  Synopsis    [Finds the variables on which a set of DDs depends.]

  Description [Finds the variables on which a set of DDs depends.
  The set must contain either BDDs and ADDs, or ZDDs.
  Returns a BDD consisting of the product of the variables if
  successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_Support Cudd_ClassifySupport]

******************************************************************************/
int *
Extra_VectorSupportArray( 
  DdManager * dd, /* manager */ 
  DdNode ** F, /* array of DDs whose support is sought */ 
  int n, /* size of the array */  
  int * support ) /* array allocated by the user */
{
    int i, size;

    /* Allocate and initialize support array for ddSupportStep. */
    size = ddMax( dd->size, dd->sizeZ );
    for ( i = 0; i < size; i++ )
        support[i] = 0;

    /* Compute support and clean up markers. */
    for ( i = 0; i < n; i++ )
        ddSupportStep2( Cudd_Regular(F[i]), support );
    for ( i = 0; i < n; i++ )
        ddClearFlag2( Cudd_Regular(F[i]) );

    return support;
}

/**Function********************************************************************

  Synopsis    [Find any cube belonging to the on-set of the function.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode *  Extra_bddFindOneCube( DdManager * dd, DdNode * bF )
{
    char * s_Temp;
    DdNode * bCube, * bTemp;
    int v;

    // get the vector of variables in the cube
    s_Temp = ABC_ALLOC( char, dd->size );
    Cudd_bddPickOneCube( dd, bF, s_Temp );

    // start the cube
    bCube = b1; Cudd_Ref( bCube );
    for ( v = 0; v < dd->size; v++ )
        if ( s_Temp[v] == 0 )
        {
//          Cube &= !s_XVars[v];
            bCube = Cudd_bddAnd( dd, bTemp = bCube, Cudd_Not(dd->vars[v]) ); Cudd_Ref( bCube );
            Cudd_RecursiveDeref( dd, bTemp );
        }
        else if ( s_Temp[v] == 1 )
        {
//          Cube &=  s_XVars[v];
            bCube = Cudd_bddAnd( dd, bTemp = bCube,          dd->vars[v]  ); Cudd_Ref( bCube );
            Cudd_RecursiveDeref( dd, bTemp );
        }
    Cudd_Deref(bCube);
    ABC_FREE( s_Temp );
    return bCube;
}

/**Function********************************************************************

  Synopsis    [Returns one cube contained in the given BDD.]

  Description [This function returns the cube with the smallest 
  bits-to-integer value.]

  SideEffects []

******************************************************************************/
DdNode * Extra_bddGetOneCube( DdManager * dd, DdNode * bFunc )
{
    DdNode * bFuncR, * bFunc0, * bFunc1;
    DdNode * bRes0,  * bRes1,  * bRes;

    bFuncR = Cudd_Regular(bFunc);
    if ( cuddIsConstant(bFuncR) )
        return bFunc;

    // cofactor
    if ( Cudd_IsComplement(bFunc) )
    {
        bFunc0 = Cudd_Not( cuddE(bFuncR) );
        bFunc1 = Cudd_Not( cuddT(bFuncR) );
    }
    else
    {
        bFunc0 = cuddE(bFuncR);
        bFunc1 = cuddT(bFuncR);
    }

    // try to find the cube with the negative literal
    bRes0 = Extra_bddGetOneCube( dd, bFunc0 );  Cudd_Ref( bRes0 );

    if ( bRes0 != b0 )
    {
        bRes = Cudd_bddAnd( dd, bRes0, Cudd_Not(dd->vars[bFuncR->index]) ); Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bRes0 );
    }
    else
    {
        Cudd_RecursiveDeref( dd, bRes0 );
        // try to find the cube with the positive literal
        bRes1 = Extra_bddGetOneCube( dd, bFunc1 );  Cudd_Ref( bRes1 );
        assert( bRes1 != b0 );
        bRes = Cudd_bddAnd( dd, bRes1, dd->vars[bFuncR->index] ); Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bRes1 );
    }

    Cudd_Deref( bRes );
    return bRes;
}

/**Function********************************************************************

  Synopsis    [Performs the reordering-sensitive step of Extra_bddMove().]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_bddComputeRangeCube( DdManager * dd, int iStart, int iStop )
{
    DdNode * bTemp, * bProd;
    int i;
    assert( iStart <= iStop );
    assert( iStart >= 0 && iStart <= dd->size );
    assert( iStop >= 0  && iStop  <= dd->size );
    bProd = b1;         Cudd_Ref( bProd );
    for ( i = iStart; i < iStop; i++ )
    {
        bProd = Cudd_bddAnd( dd, bTemp = bProd, dd->vars[i] );      Cudd_Ref( bProd );
        Cudd_RecursiveDeref( dd, bTemp ); 
    }
    Cudd_Deref( bProd );
    return bProd;
}

/**Function********************************************************************

  Synopsis    [Computes the cube of BDD variables corresponding to bits it the bit-code]

  Description [Returns a bdd composed of elementary bdds found in array BddVars[] such 
  that the bdd vars encode the number Value of bit length CodeWidth (if fMsbFirst is 1, 
  the most significant bit is encoded with the first bdd variable). If the variables 
  BddVars are not specified, takes the first CodeWidth variables of the manager]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_bddBitsToCube( DdManager * dd, int Code, int CodeWidth, DdNode ** pbVars, int fMsbFirst )
{
    int z;
    DdNode * bTemp, * bVar, * bVarBdd, * bResult;

    bResult = b1;   Cudd_Ref( bResult );
    for ( z = 0; z < CodeWidth; z++ )
    {
        bVarBdd = (pbVars)? pbVars[z]: dd->vars[z];
        if ( fMsbFirst )
            bVar = Cudd_NotCond( bVarBdd, (Code & (1 << (CodeWidth-1-z)))==0 );
        else
            bVar = Cudd_NotCond( bVarBdd, (Code & (1 << (z)))==0 );
        bResult = Cudd_bddAnd( dd, bTemp = bResult, bVar );     Cudd_Ref( bResult );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bResult );

    return bResult;
}  /* end of Extra_bddBitsToCube */

/**Function********************************************************************

  Synopsis    [Finds the support as a negative polarity cube.]

  Description [Finds the variables on which a DD depends. Returns a BDD 
  consisting of the product of the variables in the negative polarity 
  if successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_VectorSupport Cudd_Support]

******************************************************************************/
DdNode * Extra_bddSupportNegativeCube( DdManager * dd, DdNode * f )
{
    int *support;
    DdNode *res, *tmp, *var;
    int i, j;
    int size;

    /* Allocate and initialize support array for ddSupportStep. */
    size = ddMax( dd->size, dd->sizeZ );
    support = ABC_ALLOC( int, size );
    if ( support == NULL )
    {
        dd->errorCode = CUDD_MEMORY_OUT;
        return ( NULL );
    }
    for ( i = 0; i < size; i++ )
    {
        support[i] = 0;
    }

    /* Compute support and clean up markers. */
    ddSupportStep2( Cudd_Regular( f ), support );
    ddClearFlag2( Cudd_Regular( f ) );

    /* Transform support from array to cube. */
    do
    {
        dd->reordered = 0;
        res = DD_ONE( dd );
        cuddRef( res );
        for ( j = size - 1; j >= 0; j-- )
        {                        /* for each level bottom-up */
            i = ( j >= dd->size ) ? j : dd->invperm[j];
            if ( support[i] == 1 )
            {
                var = cuddUniqueInter( dd, i, dd->one, Cudd_Not( dd->one ) );
                //////////////////////////////////////////////////////////////////
                var = Cudd_Not(var);
                //////////////////////////////////////////////////////////////////
                cuddRef( var );
                tmp = cuddBddAndRecur( dd, res, var );
                if ( tmp == NULL )
                {
                    Cudd_RecursiveDeref( dd, res );
                    Cudd_RecursiveDeref( dd, var );
                    res = NULL;
                    break;
                }
                cuddRef( tmp );
                Cudd_RecursiveDeref( dd, res );
                Cudd_RecursiveDeref( dd, var );
                res = tmp;
            }
        }
    }
    while ( dd->reordered == 1 );

    ABC_FREE( support );
    if ( res != NULL )
        cuddDeref( res );
    return ( res );

}                                /* end of Extra_SupportNeg */

/**Function********************************************************************

  Synopsis    [Returns 1 if the BDD is the BDD of elementary variable.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_bddIsVar( DdNode * bFunc )
{
    bFunc = Cudd_Regular( bFunc );
    if ( cuddIsConstant(bFunc) )
        return 0;
    return cuddIsConstant( cuddT(bFunc) ) && cuddIsConstant( Cudd_Regular(cuddE(bFunc)) );
}

/**Function********************************************************************

  Synopsis    [Creates AND composed of the first nVars of the manager.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_bddCreateAnd( DdManager * dd, int nVars )
{
    DdNode * bFunc, * bTemp;
    int i;
    bFunc = Cudd_ReadOne(dd); Cudd_Ref( bFunc );
    for ( i = 0; i < nVars; i++ )
    {
        bFunc = Cudd_bddAnd( dd, bTemp = bFunc, Cudd_bddIthVar(dd,i) );  Cudd_Ref( bFunc );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bFunc );
    return bFunc;
}

/**Function********************************************************************

  Synopsis    [Creates OR composed of the first nVars of the manager.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_bddCreateOr( DdManager * dd, int nVars )
{
    DdNode * bFunc, * bTemp;
    int i;
    bFunc = Cudd_ReadLogicZero(dd); Cudd_Ref( bFunc );
    for ( i = 0; i < nVars; i++ )
    {
        bFunc = Cudd_bddOr( dd, bTemp = bFunc, Cudd_bddIthVar(dd,i) );  Cudd_Ref( bFunc );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bFunc );
    return bFunc;
}

/**Function********************************************************************

  Synopsis    [Creates EXOR composed of the first nVars of the manager.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_bddCreateExor( DdManager * dd, int nVars )
{
    DdNode * bFunc, * bTemp;
    int i;
    bFunc = Cudd_ReadLogicZero(dd); Cudd_Ref( bFunc );
    for ( i = 0; i < nVars; i++ )
    {
        bFunc = Cudd_bddXor( dd, bTemp = bFunc, Cudd_bddIthVar(dd,i) );  Cudd_Ref( bFunc );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bFunc );
    return bFunc;
}

/**Function********************************************************************

  Synopsis    [Computes the set of primes as a ZDD.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_zddPrimes( DdManager * dd, DdNode * F )
{
    DdNode    *res;
    do {
        dd->reordered = 0;
        res = extraZddPrimes(dd, F);
        if ( dd->reordered == 1 )
            printf("\nReordering in Extra_zddPrimes()\n");
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddPrimes */

/**Function********************************************************************

  Synopsis    [Permutes the variables of the array of BDDs.]

  Description [Given a permutation in array permut, creates a new BDD
  with permuted variables. There should be an entry in array permut
  for each variable in the manager. The i-th entry of permut holds the
  index of the variable that is to substitute the i-th variable.
  The DDs in the resulting array are already referenced.]

  SideEffects [None]

  SeeAlso     [Cudd_addPermute Cudd_bddSwapVariables]

******************************************************************************/
void Extra_bddPermuteArray( DdManager * manager, DdNode ** bNodesIn, DdNode ** bNodesOut, int nNodes, int *permut )
{
    DdHashTable *table;
    int i, k;
    do
    {
        manager->reordered = 0;
        table = cuddHashTableInit( manager, 1, 2 );

        /* permute the output functions one-by-one */
        for ( i = 0; i < nNodes; i++ )
        {
            bNodesOut[i] = cuddBddPermuteRecur( manager, table, bNodesIn[i], permut );
            if ( bNodesOut[i] == NULL )
            {
                /* deref the array of the already computed outputs */
                for ( k = 0; k < i; k++ )
                    Cudd_RecursiveDeref( manager, bNodesOut[k] );
                break;
            }
            cuddRef( bNodesOut[i] );
        }
        /* Dispose of local cache. */
        cuddHashTableQuit( table );
    }
    while ( manager->reordered == 1 );
}    /* end of Extra_bddPermuteArray */


/**Function********************************************************************

  Synopsis    [Computes the positive polarty cube composed of the first vars in the array.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_bddComputeCube( DdManager * dd, DdNode ** bXVars, int nVars )
{
    DdNode * bRes;
    DdNode * bTemp;
    int i;

    bRes = b1; Cudd_Ref( bRes );
    for ( i = 0; i < nVars; i++ )
    {
        bRes = Cudd_bddAnd( dd, bTemp = bRes, bXVars[i] );  Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bTemp );
    }

    Cudd_Deref( bRes );
    return bRes;
}

/**Function********************************************************************

  Synopsis    [Changes the polarity of vars listed in the cube.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_bddChangePolarity( 
  DdManager * dd,   /* the DD manager */
  DdNode * bFunc,
  DdNode * bVars) 
{
    DdNode  *res;
    do {
        dd->reordered = 0;
        res = extraBddChangePolarity( dd, bFunc, bVars );
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_bddChangePolarity */


/**Function*************************************************************

  Synopsis    [Checks if the given variable belongs to the cube.]

  Description [Return -1 if the var does not appear in the cube.
  Otherwise, returns polarity (0 or 1) of the var in the cube.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_bddVarIsInCube( DdNode * bCube, int iVar )
{
    DdNode * bCube0, * bCube1;
    while ( Cudd_Regular(bCube)->index != CUDD_CONST_INDEX )
    {
        bCube0 = Cudd_NotCond( cuddE(Cudd_Regular(bCube)), Cudd_IsComplement(bCube) );
        bCube1 = Cudd_NotCond( cuddT(Cudd_Regular(bCube)), Cudd_IsComplement(bCube) );
        // make sure it is a cube
        assert( (Cudd_IsComplement(bCube0) && Cudd_Regular(bCube0)->index == CUDD_CONST_INDEX) || // bCube0 == 0
                (Cudd_IsComplement(bCube1) && Cudd_Regular(bCube1)->index == CUDD_CONST_INDEX) ); // bCube1 == 0
        // quit if it is the last one
        if ( Cudd_Regular(bCube)->index == iVar )
            return (int)(Cudd_IsComplement(bCube0) && Cudd_Regular(bCube0)->index == CUDD_CONST_INDEX);
        // get the next cube
        if ( (Cudd_IsComplement(bCube0) && Cudd_Regular(bCube0)->index == CUDD_CONST_INDEX) )
            bCube = bCube1;
        else
            bCube = bCube0;
    }
    return -1;
}
 
/**Function*************************************************************

  Synopsis    [Computes the AND of two BDD with different orders.]

  Description [Derives the result of Boolean AND of bF and bG in ddF.
  The array pPermute gives the mapping of variables of ddG into that of ddF.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_bddAndPermute( DdManager * ddF, DdNode * bF, DdManager * ddG, DdNode * bG, int * pPermute )
{
    DdHashTable * table;
    DdNode * bRes;
    do
    {
        ddF->reordered = 0;
        table = cuddHashTableInit( ddF, 2, 256 );
        if (table == NULL) return NULL;
        bRes = extraBddAndPermute( table, ddF, bF, ddG, bG, pPermute );  
        if ( bRes ) cuddRef( bRes );
        cuddHashTableQuit( table );
        if ( bRes ) cuddDeref( bRes );
//if ( ddF->reordered == 1 )
//printf( "Reordering happened\n" );
    }
    while ( ddF->reordered == 1 );
//printf( "|F| =%6d  |G| =%6d  |H| =%6d  |F|*|G| =%9d\n", 
//       Cudd_DagSize(bF), Cudd_DagSize(bG), Cudd_DagSize(bRes), 
//       Cudd_DagSize(bF) * Cudd_DagSize(bG) );
    return ( bRes );
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Performs the reordering-sensitive step of Extra_bddMove().]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraBddMove( 
  DdManager * dd,    /* the DD manager */
  DdNode * bF,
  DdNode * bDist) 
{
    DdNode * bRes;

    if ( Cudd_IsConstant(bF) )
        return bF;

    if ( (bRes = cuddCacheLookup2(dd, extraBddMove, bF, bDist)) )
        return bRes;
    else
    {
        DdNode * bRes0, * bRes1;             
        DdNode * bF0, * bF1;             
        DdNode * bFR = Cudd_Regular(bF); 
        int VarNew;
        
        if ( Cudd_IsComplement(bDist) )
            VarNew = bFR->index - Cudd_Not(bDist)->index;
        else
            VarNew = bFR->index + bDist->index;
        assert( VarNew < dd->size );

        // cofactor the functions
        if ( bFR != bF ) // bFunc is complemented 
        {
            bF0 = Cudd_Not( cuddE(bFR) );
            bF1 = Cudd_Not( cuddT(bFR) );
        }
        else
        {
            bF0 = cuddE(bFR);
            bF1 = cuddT(bFR);
        }

        bRes0 = extraBddMove( dd, bF0, bDist );
        if ( bRes0 == NULL ) 
            return NULL;
        cuddRef( bRes0 );

        bRes1 = extraBddMove( dd, bF1, bDist );
        if ( bRes1 == NULL ) 
        {
            Cudd_RecursiveDeref( dd, bRes0 );
            return NULL;
        }
        cuddRef( bRes1 );

        /* only bRes0 and bRes1 are referenced at this point */
        bRes = cuddBddIteRecur( dd, dd->vars[VarNew], bRes1, bRes0 );
        if ( bRes == NULL ) 
        {
            Cudd_RecursiveDeref( dd, bRes0 );
            Cudd_RecursiveDeref( dd, bRes1 );
            return NULL;
        }
        cuddRef( bRes );
        Cudd_RecursiveDeref( dd, bRes0 );
        Cudd_RecursiveDeref( dd, bRes1 );

        /* insert the result into cache */
        cuddCacheInsert2( dd, extraBddMove, bF, bDist, bRes );
        cuddDeref( bRes );
        return bRes;
    }
} /* end of extraBddMove */


/**Function********************************************************************

  Synopsis    [Finds three cofactors of the cover w.r.t. to the topmost variable.]

  Description [Finds three cofactors of the cover w.r.t. to the topmost variable.
  Does not check the cover for being a constant. Assumes that ZDD variables encoding 
  positive and negative polarities are adjacent in the variable order. Is different 
  from cuddZddGetCofactors3() in that it does not compute the cofactors w.r.t. the 
  given variable but takes the cofactors with respent to the topmost variable. 
  This function is more efficient when used in recursive procedures because it does 
  not require referencing of the resulting cofactors (compare cuddZddProduct() 
  and extraZddPrimeProduct()).]

  SideEffects [None]

  SeeAlso     [cuddZddGetCofactors3]

******************************************************************************/
void 
extraDecomposeCover( 
  DdManager* dd,    /* the manager */
  DdNode*  zC,      /* the cover */
  DdNode** zC0,     /* the pointer to the negative var cofactor */ 
  DdNode** zC1,     /* the pointer to the positive var cofactor */ 
  DdNode** zC2 )    /* the pointer to the cofactor without var */ 
{
    if ( (zC->index & 1) == 0 ) 
    { /* the top variable is present in positive polarity and maybe in negative */

        DdNode *Temp = cuddE( zC );
        *zC1  = cuddT( zC );
        if ( cuddIZ(dd,Temp->index) == cuddIZ(dd,zC->index) + 1 )
        {   /* Temp is not a terminal node 
             * top var is present in negative polarity */
            *zC2 = cuddE( Temp );
            *zC0 = cuddT( Temp );
        }
        else
        {   /* top var is not present in negative polarity */
            *zC2 = Temp;
            *zC0 = dd->zero;
        }
    }
    else 
    { /* the top variable is present only in negative */
        *zC1 = dd->zero;
        *zC2 = cuddE( zC );
        *zC0 = cuddT( zC );
    }
} /* extraDecomposeCover */



/**Function********************************************************************

  Synopsis    [Counts the total number of cubes in the ISOPs of the functions.]

  Description [Returns -1 if the number of cubes exceeds the limit.]

  SideEffects [None]

  SeeAlso     [Extra_TransferPermute]

******************************************************************************/
static DdNode * extraBddCountCubes( DdManager * dd, DdNode * L, DdNode * U, st__table *table, int * pnCubes, int Limit )
{
    DdNode      *one = DD_ONE(dd);
    DdNode      *zero = Cudd_Not(one);
    int         v, top_l, top_u;
    DdNode      *Lsub0, *Usub0, *Lsub1, *Usub1, *Ld, *Ud;
    DdNode      *Lsuper0, *Usuper0, *Lsuper1, *Usuper1;
    DdNode      *Isub0, *Isub1, *Id;
    DdNode      *x;
    DdNode      *term0, *term1, *sum;
    DdNode      *Lv, *Uv, *Lnv, *Unv;
    DdNode      *r;
    int         index;
    int         Count0 = 0, Count1 = 0, Count2 = 0;

    statLine(dd);
    if (L == zero)
    {
        *pnCubes = 0;
        return(zero);
    }
    if (U == one)
    {
        *pnCubes = 1;
        return(one);
    }

    /* Check cache */
    r = cuddCacheLookup2(dd, cuddBddIsop, L, U);
    if (r)
    {
        int nCubes = 0;
        if ( st__lookup_int( table, (char *)r, &nCubes ) )
            *pnCubes = nCubes;
        else assert( 0 );
        return r;
    }

    top_l = dd->perm[Cudd_Regular(L)->index];
    top_u = dd->perm[Cudd_Regular(U)->index];
    v = ddMin(top_l, top_u);

    /* Compute cofactors */
    if (top_l == v) {
        index = Cudd_Regular(L)->index;
        Lv = Cudd_T(L);
        Lnv = Cudd_E(L);
        if (Cudd_IsComplement(L)) {
            Lv = Cudd_Not(Lv);
            Lnv = Cudd_Not(Lnv);
        }
    }
    else {
        index = Cudd_Regular(U)->index;
        Lv = Lnv = L;
    }

    if (top_u == v) {
        Uv = Cudd_T(U);
        Unv = Cudd_E(U);
        if (Cudd_IsComplement(U)) {
            Uv = Cudd_Not(Uv);
            Unv = Cudd_Not(Unv);
        }
    }
    else {
        Uv = Unv = U;
    }

    Lsub0 = cuddBddAndRecur(dd, Lnv, Cudd_Not(Uv));
    if (Lsub0 == NULL)
        return(NULL);
    Cudd_Ref(Lsub0);
    Usub0 = Unv;
    Lsub1 = cuddBddAndRecur(dd, Lv, Cudd_Not(Unv));
    if (Lsub1 == NULL) {
        Cudd_RecursiveDeref(dd, Lsub0);
        return(NULL);
    }
    Cudd_Ref(Lsub1);
    Usub1 = Uv;

    Isub0 = extraBddCountCubes(dd, Lsub0, Usub0, table, &Count0, Limit);
    if (Isub0 == NULL) {
        Cudd_RecursiveDeref(dd, Lsub0);
        Cudd_RecursiveDeref(dd, Lsub1);
        return(NULL);
    }
    Cudd_Ref(Isub0);
    Isub1 = extraBddCountCubes(dd, Lsub1, Usub1, table, &Count1, Limit);
    if (Isub1 == NULL) {
        Cudd_RecursiveDeref(dd, Lsub0);
        Cudd_RecursiveDeref(dd, Lsub1);
        Cudd_RecursiveDeref(dd, Isub0);
        return(NULL);
    }
    Cudd_Ref(Isub1);
    Cudd_RecursiveDeref(dd, Lsub0);
    Cudd_RecursiveDeref(dd, Lsub1);

    Lsuper0 = cuddBddAndRecur(dd, Lnv, Cudd_Not(Isub0));
    if (Lsuper0 == NULL) {
        Cudd_RecursiveDeref(dd, Isub0);
        Cudd_RecursiveDeref(dd, Isub1);
        return(NULL);
    }
    Cudd_Ref(Lsuper0);
    Lsuper1 = cuddBddAndRecur(dd, Lv, Cudd_Not(Isub1));
    if (Lsuper1 == NULL) {
        Cudd_RecursiveDeref(dd, Isub0);
        Cudd_RecursiveDeref(dd, Isub1);
        Cudd_RecursiveDeref(dd, Lsuper0);
        return(NULL);
    }
    Cudd_Ref(Lsuper1);
    Usuper0 = Unv;
    Usuper1 = Uv;

    /* Ld = Lsuper0 + Lsuper1 */
    Ld = cuddBddAndRecur(dd, Cudd_Not(Lsuper0), Cudd_Not(Lsuper1));
    Ld = Cudd_NotCond(Ld, Ld != NULL);
    if (Ld == NULL) {
        Cudd_RecursiveDeref(dd, Isub0);
        Cudd_RecursiveDeref(dd, Isub1);
        Cudd_RecursiveDeref(dd, Lsuper0);
        Cudd_RecursiveDeref(dd, Lsuper1);
        return(NULL);
    }
    Cudd_Ref(Ld);
    Ud = cuddBddAndRecur(dd, Usuper0, Usuper1);
    if (Ud == NULL) {
        Cudd_RecursiveDeref(dd, Isub0);
        Cudd_RecursiveDeref(dd, Isub1);
        Cudd_RecursiveDeref(dd, Lsuper0);
        Cudd_RecursiveDeref(dd, Lsuper1);
        Cudd_RecursiveDeref(dd, Ld);
        return(NULL);
    }
    Cudd_Ref(Ud);
    Cudd_RecursiveDeref(dd, Lsuper0);
    Cudd_RecursiveDeref(dd, Lsuper1);

    Id = extraBddCountCubes(dd, Ld, Ud, table, &Count2, Limit);
    if (Id == NULL) {
        Cudd_RecursiveDeref(dd, Isub0);
        Cudd_RecursiveDeref(dd, Isub1);
        Cudd_RecursiveDeref(dd, Ld);
        Cudd_RecursiveDeref(dd, Ud);
        return(NULL);
    }
    Cudd_Ref(Id);
    Cudd_RecursiveDeref(dd, Ld);
    Cudd_RecursiveDeref(dd, Ud);

    x = cuddUniqueInter(dd, index, one, zero);
    if (x == NULL) {
        Cudd_RecursiveDeref(dd, Isub0);
        Cudd_RecursiveDeref(dd, Isub1);
        Cudd_RecursiveDeref(dd, Id);
        return(NULL);
    }
    Cudd_Ref(x);
    term0 = cuddBddAndRecur(dd, Cudd_Not(x), Isub0);
    if (term0 == NULL) {
        Cudd_RecursiveDeref(dd, Isub0);
        Cudd_RecursiveDeref(dd, Isub1);
        Cudd_RecursiveDeref(dd, Id);
        Cudd_RecursiveDeref(dd, x);
        return(NULL);
    }
    Cudd_Ref(term0);
    Cudd_RecursiveDeref(dd, Isub0);
    term1 = cuddBddAndRecur(dd, x, Isub1);
    if (term1 == NULL) {
        Cudd_RecursiveDeref(dd, Isub1);
        Cudd_RecursiveDeref(dd, Id);
        Cudd_RecursiveDeref(dd, x);
        Cudd_RecursiveDeref(dd, term0);
        return(NULL);
    }
    Cudd_Ref(term1);
    Cudd_RecursiveDeref(dd, x);
    Cudd_RecursiveDeref(dd, Isub1);
    /* sum = term0 + term1 */
    sum = cuddBddAndRecur(dd, Cudd_Not(term0), Cudd_Not(term1));
    sum = Cudd_NotCond(sum, sum != NULL);
    if (sum == NULL) {
        Cudd_RecursiveDeref(dd, Id);
        Cudd_RecursiveDeref(dd, term0);
        Cudd_RecursiveDeref(dd, term1);
        return(NULL);
    }
    Cudd_Ref(sum);
    Cudd_RecursiveDeref(dd, term0);
    Cudd_RecursiveDeref(dd, term1);
    /* r = sum + Id */
    r = cuddBddAndRecur(dd, Cudd_Not(sum), Cudd_Not(Id));
    r = Cudd_NotCond(r, r != NULL);
    if (r == NULL) {
        Cudd_RecursiveDeref(dd, Id);
        Cudd_RecursiveDeref(dd, sum);
        return(NULL);
    }
    Cudd_Ref(r);
    Cudd_RecursiveDeref(dd, sum);
    Cudd_RecursiveDeref(dd, Id);

    cuddCacheInsert2(dd, cuddBddIsop, L, U, r);
    *pnCubes = Count0 + Count1 + Count2;
    if ( st__add_direct( table, (char *)r, (char *)(ABC_PTRINT_T)*pnCubes ) == st__OUT_OF_MEM )
    {
        Cudd_RecursiveDeref( dd, r );
        return NULL;
    }
    if ( *pnCubes > Limit )
    {
        Cudd_RecursiveDeref( dd, r );
        return NULL;
    }
    //printf( "%d ", *pnCubes );
    Cudd_Deref(r);
    return r;
} 
int Extra_bddCountCubes( DdManager * dd, DdNode ** pFuncs, int nFuncs, int fMode, int nLimit, int * pGuide )
{
    DdNode * pF0, * pF1; 
    int i, Count, Count0, Count1, CounterAll = 0;
    st__table *table = st__init_table( st__ptrcmp, st__ptrhash );
    unsigned int saveLimit = dd->maxLive;
    dd->maxLive = dd->keys - dd->dead + 1000000; // limit on intermediate BDD nodes
    for ( i = 0; i < nFuncs; i++ )
    {
        if ( !pFuncs[i] )
            continue;
        pF1 = pF0 = NULL;
        Count0 = Count1 = nLimit;
        if ( fMode == -1 || fMode == 1 ) // both or pos
            pF1 = extraBddCountCubes( dd, pFuncs[i], pFuncs[i], table, &Count1, nLimit );
        pFuncs[i] = Cudd_Not( pFuncs[i] );
        if ( fMode == -1 || fMode == 0 ) // both or neg
            pF0 = extraBddCountCubes( dd, pFuncs[i], pFuncs[i], table, &Count0, Count1 );
        pFuncs[i] = Cudd_Not( pFuncs[i] );
        if ( !pF1 && !pF0 )
            break;
        if ( !pF0 )                  pGuide[i] = 1, Count = Count1; // use pos
        else if ( !pF1 )             pGuide[i] = 0, Count = Count0; // use neg
        else if ( Count1 <= Count0 ) pGuide[i] = 1, Count = Count1; // use pos
        else                         pGuide[i] = 0, Count = Count0; // use neg
        CounterAll += Count;
        //printf( "Output %5d has %5d cubes (%d)  (%5d and %5d)\n", nOuts++, Count, pGuide[i], Count1, Count0 );
    }
    dd->maxLive = saveLimit;
    st__free_table( table );
    return i == nFuncs ? CounterAll : -1;
}   /* end of Extra_bddCountCubes */

/*---------------------------------------------------------------------------*/
/* Definition of static Functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Convert a BDD from a manager to another one.]

  Description [Convert a BDD from a manager to another one. Returns a
  pointer to the BDD in the destination manager if successful; NULL
  otherwise.]

  SideEffects [None]

  SeeAlso     [Extra_TransferPermute]

******************************************************************************/
DdNode * extraTransferPermute( DdManager * ddS, DdManager * ddD, DdNode * f, int * Permute )
{
    DdNode *res;
    st__table *table = NULL;
    st__generator *gen = NULL;
    DdNode *key, *value;

    table = st__init_table( st__ptrcmp, st__ptrhash );
    if ( table == NULL )
        goto failure;
    res = extraTransferPermuteRecur( ddS, ddD, f, table, Permute );
    if ( res != NULL )
        cuddRef( res );

    /* Dereference all elements in the table and dispose of the table.
       ** This must be done also if res is NULL to avoid leaks in case of
       ** reordering. */
    gen = st__init_gen( table );
    if ( gen == NULL )
        goto failure;
    while ( st__gen( gen, ( const char ** ) &key, ( char ** ) &value ) )
    {
        Cudd_RecursiveDeref( ddD, value );
    }
    st__free_gen( gen );
    gen = NULL;
    st__free_table( table );
    table = NULL;

    if ( res != NULL )
        cuddDeref( res );
    return ( res );

  failure:
    if ( table != NULL )
        st__free_table( table );
    if ( gen != NULL )
        st__free_gen( gen );
    return ( NULL );

}                               /* end of extraTransferPermute */


/**Function********************************************************************

  Synopsis    [Performs the recursive step of Extra_TransferPermute.]

  Description [Performs the recursive step of Extra_TransferPermute.
  Returns a pointer to the result if successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [extraTransferPermute]

******************************************************************************/
static DdNode * 
extraTransferPermuteRecur( 
  DdManager * ddS, 
  DdManager * ddD, 
  DdNode * f, 
  st__table * table, 
  int * Permute )
{
    DdNode *ft, *fe, *t, *e, *var, *res;
    DdNode *one, *zero;
    int index;
    int comple = 0;

    statLine( ddD );
    one = DD_ONE( ddD );
    comple = Cudd_IsComplement( f );

    /* Trivial cases. */
    if ( Cudd_IsConstant( f ) )
        return ( Cudd_NotCond( one, comple ) );


    /* Make canonical to increase the utilization of the cache. */
    f = Cudd_NotCond( f, comple );
    /* Now f is a regular pointer to a non-constant node. */

    /* Check the cache. */
    if ( st__lookup( table, ( char * ) f, ( char ** ) &res ) )
        return ( Cudd_NotCond( res, comple ) );

    if ( ddS->TimeStop && Abc_Clock() > ddS->TimeStop )
        return NULL;
    if ( ddD->TimeStop && Abc_Clock() > ddD->TimeStop )
        return NULL;

    /* Recursive step. */
    if ( Permute )
        index = Permute[f->index];
    else
        index = f->index;

    ft = cuddT( f );
    fe = cuddE( f );

    t = extraTransferPermuteRecur( ddS, ddD, ft, table, Permute );
    if ( t == NULL )
    {
        return ( NULL );
    }
    cuddRef( t );

    e = extraTransferPermuteRecur( ddS, ddD, fe, table, Permute );
    if ( e == NULL )
    {
        Cudd_RecursiveDeref( ddD, t );
        return ( NULL );
    }
    cuddRef( e );

    zero = Cudd_Not(ddD->one);
    var = cuddUniqueInter( ddD, index, one, zero );
    if ( var == NULL )
    {
        Cudd_RecursiveDeref( ddD, t );
        Cudd_RecursiveDeref( ddD, e );
        return ( NULL );
    }
    res = cuddBddIteRecur( ddD, var, t, e );

    if ( res == NULL )
    {
        Cudd_RecursiveDeref( ddD, t );
        Cudd_RecursiveDeref( ddD, e );
        return ( NULL );
    }
    cuddRef( res );
    Cudd_RecursiveDeref( ddD, t );
    Cudd_RecursiveDeref( ddD, e );

    if ( st__add_direct( table, ( char * ) f, ( char * ) res ) ==
         st__OUT_OF_MEM )
    {
        Cudd_RecursiveDeref( ddD, res );
        return ( NULL );
    }
    return ( Cudd_NotCond( res, comple ) );

}                               /* end of extraTransferPermuteRecur */

/**Function********************************************************************

  Synopsis    [Performs the recursive step of Cudd_Support.]

  Description [Performs the recursive step of Cudd_Support. Performs a
  DFS from f. The support is accumulated in supp as a side effect. Uses
  the LSB of the then pointer as visited flag.]

  SideEffects [None]

  SeeAlso     [ddClearFlag]

******************************************************************************/
void
ddSupportStep2(
  DdNode * f,
  int * support)
{
    if (cuddIsConstant(f) || Cudd_IsComplement(f->next)) {
    return;
    }

    support[f->index] = 1;
    ddSupportStep2(cuddT(f),support);
    ddSupportStep2(Cudd_Regular(cuddE(f)),support);
    /* Mark as visited. */
    f->next = Cudd_Not(f->next);
    return;

} /* end of ddSupportStep */


/**Function********************************************************************

  Synopsis    [Performs a DFS from f, clearing the LSB of the next
  pointers.]

  Description []

  SideEffects [None]

  SeeAlso     [ddSupportStep ddDagInt]

******************************************************************************/
void
ddClearFlag2(
  DdNode * f)
{
    if (!Cudd_IsComplement(f->next)) {
    return;
    }
    /* Clear visited flag. */
    f->next = Cudd_Regular(f->next);
    if (cuddIsConstant(f)) {
    return;
    }
    ddClearFlag2(cuddT(f));
    ddClearFlag2(Cudd_Regular(cuddE(f)));
    return;

} /* end of ddClearFlag */


/**Function********************************************************************

  Synopsis    [Composed three subcovers into one ZDD.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode *
extraComposeCover( 
  DdManager* dd,    /* the manager */
  DdNode* zC0,     /* the pointer to the negative var cofactor */ 
  DdNode* zC1,     /* the pointer to the positive var cofactor */ 
  DdNode* zC2,     /* the pointer to the cofactor without var */ 
  int TopVar)    /* the index of the positive ZDD var */
{
    DdNode * zRes, * zTemp;
    /* compose with-neg-var and without-var using the neg ZDD var */
    zTemp = cuddZddGetNode( dd, 2*TopVar + 1, zC0, zC2 );
    if ( zTemp == NULL ) 
    {
        Cudd_RecursiveDerefZdd(dd, zC0);
        Cudd_RecursiveDerefZdd(dd, zC1);
        Cudd_RecursiveDerefZdd(dd, zC2);
        return NULL;
    }
    cuddRef( zTemp );
    cuddDeref( zC0 );
    cuddDeref( zC2 );

    /* compose with-pos-var and previous result using the pos ZDD var */
    zRes = cuddZddGetNode( dd, 2*TopVar, zC1, zTemp );
    if ( zRes == NULL ) 
    {
        Cudd_RecursiveDerefZdd(dd, zC1);
        Cudd_RecursiveDerefZdd(dd, zTemp);
        return NULL;
    }
    cuddDeref( zC1 );
    cuddDeref( zTemp );
    return zRes;
} /* extraComposeCover */

/**Function********************************************************************

  Synopsis    [Performs the recursive step of prime computation.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode* extraZddPrimes( DdManager *dd, DdNode* F )
{
    DdNode *zRes;

    if ( F == Cudd_Not( dd->one ) )
        return dd->zero;
    if ( F == dd->one )
        return dd->one;

    /* check cache */
    zRes = cuddCacheLookup1Zdd(dd, extraZddPrimes, F);
    if (zRes)
        return(zRes);
    {
        /* temporary variables */
        DdNode *bF01, *zP0, *zP1;
        /* three components of the prime set */
        DdNode *zResE, *zResP, *zResN;
        int fIsComp = Cudd_IsComplement( F );

        /* find cofactors of F */
        DdNode * bF0 = Cudd_NotCond( Cudd_E( F ), fIsComp );
        DdNode * bF1 = Cudd_NotCond( Cudd_T( F ), fIsComp );

        /* find the intersection of cofactors */
        bF01 = cuddBddAndRecur( dd, bF0, bF1 );
        if ( bF01 == NULL ) return NULL;
        cuddRef( bF01 );

        /* solve the problems for cofactors */
        zP0 = extraZddPrimes( dd, bF0 );
        if ( zP0 == NULL )
        {
            Cudd_RecursiveDeref( dd, bF01 );
            return NULL;
        }
        cuddRef( zP0 );

        zP1 = extraZddPrimes( dd, bF1 );
        if ( zP1 == NULL )
        {
            Cudd_RecursiveDeref( dd, bF01 );
            Cudd_RecursiveDerefZdd( dd, zP0 );
            return NULL;
        }
        cuddRef( zP1 );

        /* check for local unateness */
        if ( bF01 == bF0 )       /* unate increasing */
        {
            /* intersection is useless */
            cuddDeref( bF01 );
            /* the primes of intersection are the primes of F0 */
            zResE = zP0;
            /* there are no primes with negative var */
            zResN = dd->zero;
            cuddRef( zResN );
            /* primes with positive var are primes of F1 that are not primes of F01 */
            zResP = cuddZddDiff( dd, zP1, zP0 );
            if ( zResP == NULL )  
            {
                Cudd_RecursiveDerefZdd( dd, zResE );
                Cudd_RecursiveDerefZdd( dd, zResN );
                Cudd_RecursiveDerefZdd( dd, zP1 );
                return NULL;
            }
            cuddRef( zResP );
            Cudd_RecursiveDerefZdd( dd, zP1 );
        }
        else if ( bF01 == bF1 ) /* unate decreasing */
        {
            /* intersection is useless */
            cuddDeref( bF01 );
            /* the primes of intersection are the primes of F1 */
            zResE = zP1;
            /* there are no primes with positive var */
            zResP = dd->zero;
            cuddRef( zResP );
            /* primes with negative var are primes of F0 that are not primes of F01 */
            zResN = cuddZddDiff( dd, zP0, zP1 );
            if ( zResN == NULL )  
            {
                Cudd_RecursiveDerefZdd( dd, zResE );
                Cudd_RecursiveDerefZdd( dd, zResP );
                Cudd_RecursiveDerefZdd( dd, zP0 );
                return NULL;
            }
            cuddRef( zResN );
            Cudd_RecursiveDerefZdd( dd, zP0 );
        }
        else /* not unate */
        {
            /* primes without the top var are primes of F10 */
            zResE = extraZddPrimes( dd, bF01 );
            if ( zResE == NULL )  
            {
                Cudd_RecursiveDerefZdd( dd, bF01 );
                Cudd_RecursiveDerefZdd( dd, zP0 );
                Cudd_RecursiveDerefZdd( dd, zP1 );
                return NULL;
            }
            cuddRef( zResE );
            Cudd_RecursiveDeref( dd, bF01 );

            /* primes with the negative top var are those of P0 that are not in F10 */
            zResN = cuddZddDiff( dd, zP0, zResE );
            if ( zResN == NULL )  
            {
                Cudd_RecursiveDerefZdd( dd, zResE );
                Cudd_RecursiveDerefZdd( dd, zP0 );
                Cudd_RecursiveDerefZdd( dd, zP1 );
                return NULL;
            }
            cuddRef( zResN );
            Cudd_RecursiveDerefZdd( dd, zP0 );

            /* primes with the positive top var are those of P1 that are not in F10 */
            zResP = cuddZddDiff( dd, zP1, zResE );
            if ( zResP == NULL )  
            {
                Cudd_RecursiveDerefZdd( dd, zResE );
                Cudd_RecursiveDerefZdd( dd, zResN );
                Cudd_RecursiveDerefZdd( dd, zP1 );
                return NULL;
            }
            cuddRef( zResP );
            Cudd_RecursiveDerefZdd( dd, zP1 );
        }

        zRes = extraComposeCover( dd, zResN, zResP, zResE, Cudd_Regular(F)->index );
        if ( zRes == NULL ) return NULL;

        /* insert the result into cache */
        cuddCacheInsert1(dd, extraZddPrimes, F, zRes);
        return zRes;
    }
} /* end of extraZddPrimes */

/**Function********************************************************************

  Synopsis    [Implements the recursive step of Cudd_bddPermute.]

  Description [ Recursively puts the BDD in the order given in the array permut.
  Checks for trivial cases to terminate recursion, then splits on the
  children of this node.  Once the solutions for the children are
  obtained, it puts into the current position the node from the rest of
  the BDD that should be here. Then returns this BDD.
  The key here is that the node being visited is NOT put in its proper
  place by this instance, but rather is switched when its proper position
  is reached in the recursion tree.<p>
  The DdNode * that is returned is the same BDD as passed in as node,
  but in the new order.]

  SideEffects [None]

  SeeAlso     [Cudd_bddPermute cuddAddPermuteRecur]

******************************************************************************/
static DdNode *
cuddBddPermuteRecur( DdManager * manager /* DD manager */ ,
                     DdHashTable * table /* computed table */ ,
                     DdNode * node /* BDD to be reordered */ ,
                     int *permut /* permutation array */  )
{
    DdNode *N, *T, *E;
    DdNode *res;
    int index;

    statLine( manager );
    N = Cudd_Regular( node );

    /* Check for terminal case of constant node. */
    if ( cuddIsConstant( N ) )
    {
        return ( node );
    }

    /* If problem already solved, look up answer and return. */
    if ( N->ref != 1 && ( res = cuddHashTableLookup1( table, N ) ) != NULL )
    {
        return ( Cudd_NotCond( res, N != node ) );
    }

    /* Split and recur on children of this node. */
    T = cuddBddPermuteRecur( manager, table, cuddT( N ), permut );
    if ( T == NULL )
        return ( NULL );
    cuddRef( T );
    E = cuddBddPermuteRecur( manager, table, cuddE( N ), permut );
    if ( E == NULL )
    {
        Cudd_IterDerefBdd( manager, T );
        return ( NULL );
    }
    cuddRef( E );

    /* Move variable that should be in this position to this position
       ** by retrieving the single var BDD for that variable, and calling
       ** cuddBddIteRecur with the T and E we just created.
     */
    index = permut[N->index];
    res = cuddBddIteRecur( manager, manager->vars[index], T, E );
    if ( res == NULL )
    {
        Cudd_IterDerefBdd( manager, T );
        Cudd_IterDerefBdd( manager, E );
        return ( NULL );
    }
    cuddRef( res );
    Cudd_IterDerefBdd( manager, T );
    Cudd_IterDerefBdd( manager, E );

    /* Do not keep the result if the reference count is only 1, since
       ** it will not be visited again.
     */
    if ( N->ref != 1 )
    {
        ptrint fanout = ( ptrint ) N->ref;
        cuddSatDec( fanout );
        if ( !cuddHashTableInsert1( table, N, res, fanout ) )
        {
            Cudd_IterDerefBdd( manager, res );
            return ( NULL );
        }
    }
    cuddDeref( res );
    return ( Cudd_NotCond( res, N != node ) );

}                                /* end of cuddBddPermuteRecur */


/**Function********************************************************************

  Synopsis    [Performs the reordering-sensitive step of Extra_bddChangePolarity().]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraBddChangePolarity( 
  DdManager * dd,    /* the DD manager */
  DdNode * bFunc, 
  DdNode * bVars) 
{
    DdNode * bRes;

    if ( bVars == b1 )
        return bFunc;
    if ( Cudd_IsConstant(bFunc) )
        return bFunc;

    if ( (bRes = cuddCacheLookup2(dd, extraBddChangePolarity, bFunc, bVars)) )
        return bRes;
    else
    {
        DdNode * bFR = Cudd_Regular(bFunc); 
        int LevelF   = dd->perm[bFR->index];
        int LevelV   = dd->perm[bVars->index];

        if ( LevelV < LevelF )
            bRes = extraBddChangePolarity( dd, bFunc, cuddT(bVars) );
        else // if ( LevelF <= LevelV )
        {
            DdNode * bRes0, * bRes1;             
            DdNode * bF0, * bF1;             
            DdNode * bVarsNext;

            // cofactor the functions
            if ( bFR != bFunc ) // bFunc is complemented 
            {
                bF0 = Cudd_Not( cuddE(bFR) );
                bF1 = Cudd_Not( cuddT(bFR) );
            }
            else
            {
                bF0 = cuddE(bFR);
                bF1 = cuddT(bFR);
            }

            if ( LevelF == LevelV )
                bVarsNext = cuddT(bVars);
            else
                bVarsNext = bVars;

            bRes0 = extraBddChangePolarity( dd, bF0, bVarsNext );
            if ( bRes0 == NULL ) 
                return NULL;
            cuddRef( bRes0 );

            bRes1 = extraBddChangePolarity( dd, bF1, bVarsNext );
            if ( bRes1 == NULL ) 
            {
                Cudd_RecursiveDeref( dd, bRes0 );
                return NULL;
            }
            cuddRef( bRes1 );

            if ( LevelF == LevelV )
            { // swap the cofactors
                DdNode * bTemp;
                bTemp = bRes0;
                bRes0 = bRes1;
                bRes1 = bTemp;
            }

            /* only aRes0 and aRes1 are referenced at this point */

            /* consider the case when Res0 and Res1 are the same node */
            if ( bRes0 == bRes1 )
                bRes = bRes1;
            /* consider the case when Res1 is complemented */
            else if ( Cudd_IsComplement(bRes1) ) 
            {
                bRes = cuddUniqueInter(dd, bFR->index, Cudd_Not(bRes1), Cudd_Not(bRes0));
                if ( bRes == NULL ) 
                {
                    Cudd_RecursiveDeref(dd,bRes0);
                    Cudd_RecursiveDeref(dd,bRes1);
                    return NULL;
                }
                bRes = Cudd_Not(bRes);
            } 
            else 
            {
                bRes = cuddUniqueInter( dd, bFR->index, bRes1, bRes0 );
                if ( bRes == NULL ) 
                {
                    Cudd_RecursiveDeref(dd,bRes0);
                    Cudd_RecursiveDeref(dd,bRes1);
                    return NULL;
                }
            }
            cuddDeref( bRes0 );
            cuddDeref( bRes1 );
        }
            
        /* insert the result into cache */
        cuddCacheInsert2(dd, extraBddChangePolarity, bFunc, bVars, bRes);
        return bRes;
    }
} /* end of extraBddChangePolarity */



static int Counter = 0;

/**Function*************************************************************

  Synopsis    [Computes the AND of two BDD with different orders.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * extraBddAndPermute( DdHashTable * table, DdManager * ddF, DdNode * bF, DdManager * ddG, DdNode * bG, int * pPermute )
{
    DdNode * bF0, * bF1, * bG0, * bG1, * bRes0, * bRes1, * bRes, * bVar;
    int LevF, LevG, Lev;

    // if F == 0, return 0 
    if ( bF == Cudd_Not(ddF->one) )
        return Cudd_Not(ddF->one);
    // if G == 0, return 0 
    if ( bG == Cudd_Not(ddG->one) )
        return Cudd_Not(ddF->one);
    // if G == 1, return F
    if ( bG == ddG->one )
        return bF;
    // cannot use F == 1, because the var order of G has to be changed

    // check cache
    if ( //(Cudd_Regular(bF)->ref != 1 || Cudd_Regular(bG)->ref != 1) && 
         (bRes = cuddHashTableLookup2(table, bF, bG)) )
        return bRes;
    Counter++;

    if ( ddF->TimeStop && Abc_Clock() > ddF->TimeStop )
        return NULL;
    if ( ddG->TimeStop && Abc_Clock() > ddG->TimeStop )
        return NULL;

    // find the topmost variable in F and G using var order of F
    LevF = cuddI( ddF, Cudd_Regular(bF)->index );
    LevG = cuddI( ddF, pPermute ? pPermute[Cudd_Regular(bG)->index] : Cudd_Regular(bG)->index );
    Lev  = Abc_MinInt( LevF, LevG );
    assert( Lev < ddF->size );
    bVar = ddF->vars[ddF->invperm[Lev]];

    // cofactor
    bF0 = (Lev < LevF) ? bF : Cudd_NotCond( cuddE(Cudd_Regular(bF)), Cudd_IsComplement(bF) );
    bF1 = (Lev < LevF) ? bF : Cudd_NotCond( cuddT(Cudd_Regular(bF)), Cudd_IsComplement(bF) );
    bG0 = (Lev < LevG) ? bG : Cudd_NotCond( cuddE(Cudd_Regular(bG)), Cudd_IsComplement(bG) );
    bG1 = (Lev < LevG) ? bG : Cudd_NotCond( cuddT(Cudd_Regular(bG)), Cudd_IsComplement(bG) );

    // call for cofactors
    bRes0 = extraBddAndPermute( table, ddF, bF0, ddG, bG0, pPermute );
    if ( bRes0 == NULL ) 
        return NULL;
    cuddRef( bRes0 );
    // call for cofactors
    bRes1 = extraBddAndPermute( table, ddF, bF1, ddG, bG1, pPermute );
    if ( bRes1 == NULL ) 
    {
        Cudd_IterDerefBdd( ddF, bRes0 );
        return NULL;
    }
    cuddRef( bRes1 );

    // compose the result
    bRes = cuddBddIteRecur( ddF, bVar, bRes1, bRes0 );
    if ( bRes == NULL )
    {
        Cudd_IterDerefBdd( ddF, bRes0 );
        Cudd_IterDerefBdd( ddF, bRes1 );
        return NULL;
    }
    cuddRef( bRes );
    Cudd_IterDerefBdd( ddF, bRes0 );
    Cudd_IterDerefBdd( ddF, bRes1 );

    // cache the result
//    if ( Cudd_Regular(bF)->ref != 1 || Cudd_Regular(bG)->ref != 1 )
    {
        ptrint fanout = (ptrint)Cudd_Regular(bF)->ref * Cudd_Regular(bG)->ref;
        cuddSatDec(fanout);
        cuddHashTableInsert2( table, bF, bG, bRes, fanout );
    }
    cuddDeref( bRes );
    return bRes;
} 

/**Function*************************************************************

  Synopsis    [Testbench.]

  Description [This procedure takes BDD manager ddF and two BDDs
  in this manager (bF and bG).  It creates a new manager ddG,
  transfers bG into it and then reorders it, resulting in bG2.
  Then it tries to compute the product of bF and bG2 in ddF.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_TestAndPerm( DdManager * ddF, DdNode * bF, DdNode * bG )
{
    DdManager * ddG;
    DdNode * bG2, * bRes1, * bRes2;
    abctime clk;
    // disable variable ordering in ddF
    Cudd_AutodynDisable( ddF );

    // create new BDD manager
    ddG = Cudd_Init( ddF->size, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    // transfer BDD into it
    Cudd_ShuffleHeap( ddG, ddF->invperm );
    bG2 = Extra_TransferLevelByLevel( ddF, ddG, bG );   Cudd_Ref( bG2 );
    // reorder the new manager
    Cudd_ReduceHeap( ddG, CUDD_REORDER_SYMM_SIFT, 1 );

    // compute the result
clk = Abc_Clock();
    bRes1 = Cudd_bddAnd( ddF, bF, bG );                 Cudd_Ref( bRes1 );
Abc_PrintTime( 1, "Runtime of Cudd_bddAnd  ", Abc_Clock() - clk );

    // compute the result
Counter = 0;
clk = Abc_Clock();
    bRes2 = Extra_bddAndPermute( ddF, bF, ddG, bG2, NULL );      Cudd_Ref( bRes2 );
Abc_PrintTime( 1, "Runtime of new procedure", Abc_Clock() - clk );
printf( "Recursive calls = %d\n", Counter );
printf( "|F| =%6d  |G| =%6d  |H| =%6d  |F|*|G| =%9d  ", 
       Cudd_DagSize(bF), Cudd_DagSize(bG), Cudd_DagSize(bRes2), 
       Cudd_DagSize(bF) * Cudd_DagSize(bG) );

    if ( bRes1 == bRes2 )
        printf( "Result verified.\n\n" );
    else
        printf( "Result is incorrect.\n\n" );

    Cudd_RecursiveDeref( ddF, bRes1 );
    Cudd_RecursiveDeref( ddF, bRes2 );
    // quit the new manager
    Cudd_RecursiveDeref( ddG, bG2 );
    Extra_StopManager( ddG );

    // re-enable variable ordering in ddF
    Cudd_AutodynEnable( ddF, CUDD_REORDER_SYMM_SIFT );
}

/**Function*************************************************************

  Synopsis    [Writes ZDD into a PLA file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_zddDumpPla( DdManager * dd, DdNode * F, int nVars, char * pFileName )
{
    DdGen *gen;
    char * pCube;
    int * pPath, i;
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        return;
    }
    fprintf( pFile, "# PLA file dumped by Extra_zddDumpPla() in ABC\n" );
    fprintf( pFile, ".i %d\n", nVars );
    fprintf( pFile, ".o 1\n" );
    pCube = ABC_CALLOC( char, dd->sizeZ );
    Cudd_zddForeachPath( dd, F, gen, pPath )
    {
        for ( i = 0; i < nVars; i++ )
            pCube[i] = '-';
        for ( i = 0; i < nVars; i++ )
            if ( pPath[2*i] == 1 || pPath[2*i+1] == 1 )
                pCube[i] = '0' + (pPath[2*i] == 1);
        fprintf( pFile, "%s 1\n", pCube );           
    }
    fprintf( pFile, ".e\n\n" );
    fclose( pFile );
    ABC_FREE( pCube );
}

/**Function*************************************************************

  Synopsis    [Constructing ZDD of a graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_GraphExperiment()
{
    int Edges[5][5] = { 
        {1, 3, 4}, 
        {1, 5}, 
        {2, 3, 5}, 
        {2, 4} 
    };
    int e, n;

    DdManager * dd = Cudd_Init( 0, 6, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );

    // create the edges
    DdNode * zGraph, * zEdge, * zVar, * zTemp;
    zGraph = DD_ZERO(dd);                                                      Cudd_Ref( zGraph );
    for ( e = 0; Edges[e][0]; e++ )
    {
        zEdge = DD_ONE(dd);                                                    Cudd_Ref( zEdge );
        for ( n = 0; Edges[e][n]; n++ )
        {
            zVar = cuddZddGetNode( dd, Edges[e][n], DD_ONE(dd), DD_ZERO(dd) ); Cudd_Ref( zVar );
            zEdge = Cudd_zddUnateProduct( dd, zTemp = zEdge, zVar );           Cudd_Ref( zEdge );
            Cudd_RecursiveDerefZdd( dd, zTemp );
            Cudd_RecursiveDerefZdd( dd, zVar );
        }
        zGraph = Cudd_zddUnion( dd, zTemp = zGraph, zEdge );                   Cudd_Ref( zGraph );
        Cudd_RecursiveDerefZdd( dd, zTemp );
        Cudd_RecursiveDerefZdd( dd, zEdge );
    }

    Cudd_zddPrintMinterm( dd, zGraph );

    Cudd_RecursiveDerefZdd( dd, zGraph );
    Cudd_Quit(dd);
}




/**Function********************************************************************

  Synopsis    [Performs the reordering-sensitive step of Extra_zddCombination().]

  Description [Generates in a bottom-up fashion ZDD for one combination 
               whose var values are given in the array VarValues. If necessary,
               creates new variables on the fly.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraZddCombination(
  DdManager* dd, 
  int* VarValues, 
  int nVars )
{
    int lev, index;
    DdNode *zRes, *zTemp;

    /* transform the combination from the array VarValues into a ZDD cube. */
    zRes = dd->one;
    cuddRef(zRes);

    /*  go through levels starting bottom-up and create nodes 
     *  if these variables are present in the comb
     */
    for (lev = nVars - 1; lev >= 0; lev--) 
    { 
        index = (lev >= dd->sizeZ) ? lev : dd->invpermZ[lev];
        if (VarValues[index] == 1) 
        {
            /* compose zRes with ZERO for the given ZDD variable */
            zRes = cuddZddGetNode( dd, index, zTemp = zRes, dd->zero );
            if ( zRes == NULL ) 
            {
                Cudd_RecursiveDerefZdd( dd, zTemp );
                return NULL;
            }
            cuddRef( zRes );
            cuddDeref( zTemp );
        }
    }
    cuddDeref( zRes );
    return zRes;

} /* end of extraZddCombination */

/**Function********************************************************************

  Synopsis    [Creates ZDD of the combination containing given variables.]

  Description [Creates ZDD of the combination containing given variables.
               VarValues contains 1 for a variable that belongs to the 
               combination and 0 for a varible that does not belong. 
               nVars is number of ZDD variables in the array.]

  SideEffects [New ZDD variables are created if indices of the variables
               present in the combination are larger than the currently
               allocated number of ZDD variables.]

  SeeAlso     []

******************************************************************************/
DdNode * Extra_zddCombination( 
  DdManager *dd, 
  int* VarValues, 
  int nVars )
{
    DdNode  *res;
    do {
    dd->reordered = 0;
    res = extraZddCombination(dd, VarValues, nVars);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddCombination */

/**Function********************************************************************

  Synopsis    [Generates a random set of combinations.]

  Description [Given a set of n elements, each of which is encoded using one
               ZDD variable, this function generates a random set of k subsets 
               (combinations of elements) with density d. Assumes that k and n
               are positive integers. Returns NULL if density is less than 0.0 
               or more than 1.0.]

  SideEffects [Allocates new ZDD variables if their current number is less than n.]

  SeeAlso     []

******************************************************************************/
DdNode* Extra_zddRandomSet( 
  DdManager * dd,   /* the DD manager */
  int n,            /* the number of elements */
  int k,            /* the number of combinations (subsets) */
  double d)         /* average density of elements in combinations */
{
    DdNode *Result, *TempComb, *Aux;
    int c, v, Limit, *VarValues;

    /* sanity check the parameters */
    if ( n <= 0 || k <= 0 || d < 0.0 || d > 1.0 )
        return NULL;

    /* allocate temporary storage for variable values */
    VarValues = ABC_ALLOC( int, n );
    if (VarValues == NULL) 
    {
        dd->errorCode = CUDD_MEMORY_OUT;
        return NULL;
    }

    /* start the new set */
    Result = dd->zero;
    Cudd_Ref( Result );

    /* seed random number generator */
    Cudd_Srandom( time(NULL) );
//  Cudd_Srandom( 4 );
    /* determine the limit below which var belongs to the combination */
    Limit = (int)(d * 2147483561.0);

    /* add combinations one by one */
    for ( c = 0; c < k; c++ )
    {
        for ( v = 0; v < n; v++ )
            if ( Cudd_Random() <= Limit )
                VarValues[v] = 1;
            else
                VarValues[v] = 0;

        TempComb = Extra_zddCombination( dd, VarValues, n );
        Cudd_Ref( TempComb );

        /* make sure that this combination is not already in the set */
        if ( c )
        { /* at least one combination is already included */

            Aux = Cudd_zddDiff( dd, Result, TempComb );
            Cudd_Ref( Aux );
            if ( Aux != Result )
            {
                Cudd_RecursiveDerefZdd( dd, Aux );
                Cudd_RecursiveDerefZdd( dd, TempComb );
                c--;
                continue;
            }
            else 
            { /* Aux is the same node as Result */
                Cudd_Deref( Aux );
            }
        }

        Result = Cudd_zddUnion( dd, Aux = Result, TempComb );
        Cudd_Ref( Result );
        Cudd_RecursiveDerefZdd( dd, Aux );
        Cudd_RecursiveDerefZdd( dd, TempComb );
    }

    ABC_FREE( VarValues );
    Cudd_Deref( Result );
    return Result;

} /* end of Extra_zddRandomSet */

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Extra_ZddTest()
{
    int N  = 64;
    int K0 = 1000;
    int i, Size;
    DdManager * dd = Cudd_Init( 0, 32, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    for ( i = 1; i <= 10; i++ )
    {
        int K = K0 * i;
        DdNode * zRandSet = Extra_zddRandomSet( dd, N, K, 0.5 );  Cudd_Ref(zRandSet);
        Size = Cudd_zddDagSize(zRandSet);
        //Cudd_zddPrintMinterm( dd, zRandSet );
        printf( "N = %5d  K = %5d  BddSize = %6d   MemBdd = %8.3f MB   MemBit = %8.3f MB   Ratio = %8.3f %%\n", 
            N, K, Size, 20.0*Size/(1<<20), 0.125 * N * K /(1<<20), 100.0*(0.125 * N * K)/(20.0*Size) );
        Cudd_RecursiveDerefZdd( dd, zRandSet );
    }
    Cudd_Quit(dd);
}


/**Function********************************************************************

  Synopsis    [Performs the reordering-sensitive step of Extra_bddTuples().]

  Description [Generates in a bottom-up fashion BDD for all combinations
               composed of k variables out of variables belonging to bVarsN.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraBddTuples( 
  DdManager * dd,    /* the DD manager */
  DdNode * bVarsK,   /* the number of variables in tuples */
  DdNode * bVarsN)   /* the set of all variables */
{
    DdNode *bRes, *bRes0, *bRes1;
    statLine(dd); 

    /* terminal cases */
/*  if ( k < 0 || k > n )
 *      return dd->zero;
 *  if ( n == 0 )
 *      return dd->one; 
 */
    if ( cuddI( dd, bVarsK->index ) < cuddI( dd, bVarsN->index ) )
        return b0;
    if ( bVarsN == b1 )
        return b1;

    /* check cache */
    bRes = cuddCacheLookup2(dd, extraBddTuples, bVarsK, bVarsN);
    if (bRes)
        return(bRes);

    /* ZDD in which is variable is 0 */
/*  bRes0 = extraBddTuples( dd, k,     n-1 ); */
    bRes0 = extraBddTuples( dd, bVarsK, cuddT(bVarsN) );
    if ( bRes0 == NULL ) 
        return NULL;
    cuddRef( bRes0 );

    /* ZDD in which is variable is 1 */
/*  bRes1 = extraBddTuples( dd, k-1,          n-1 ); */
    if ( bVarsK == b1 )
    {
        bRes1 = b0;
        cuddRef( bRes1 );
    }
    else
    {
        bRes1 = extraBddTuples( dd, cuddT(bVarsK), cuddT(bVarsN) );
        if ( bRes1 == NULL ) 
        {
            Cudd_RecursiveDeref( dd, bRes0 );
            return NULL;
        }
        cuddRef( bRes1 );
    }

    /* consider the case when Res0 and Res1 are the same node */
    if ( bRes0 == bRes1 )
        bRes = bRes1;
    /* consider the case when Res1 is complemented */
    else if ( Cudd_IsComplement(bRes1) ) 
    {
        bRes = cuddUniqueInter(dd, bVarsN->index, Cudd_Not(bRes1), Cudd_Not(bRes0));
        if ( bRes == NULL ) 
        {
            Cudd_RecursiveDeref(dd,bRes0);
            Cudd_RecursiveDeref(dd,bRes1);
            return NULL;
        }
        bRes = Cudd_Not(bRes);
    } 
    else 
    {
        bRes = cuddUniqueInter( dd, bVarsN->index, bRes1, bRes0 );
        if ( bRes == NULL ) 
        {
            Cudd_RecursiveDeref(dd,bRes0);
            Cudd_RecursiveDeref(dd,bRes1);
            return NULL;
        }
    }
    cuddDeref( bRes0 );
    cuddDeref( bRes1 );

    /* insert the result into cache */
    cuddCacheInsert2(dd, extraBddTuples, bVarsK, bVarsN, bRes);
    return bRes;

} /* end of extraBddTuples */

/**Function********************************************************************

  Synopsis    [Builds BDD representing the set of fixed-size variable tuples.]

  Description [Creates BDD of all combinations of variables in Support that have k vars in them.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_bddTuples( 
  DdManager * dd,   /* the DD manager */
  int K,            /* the number of variables in tuples */
  DdNode * VarsN)   /* the set of all variables represented as a BDD */
{
    DdNode  *res;
    int     autoDyn;

    /* it is important that reordering does not happen, 
       otherwise, this methods will not work */

    autoDyn = dd->autoDyn;
    dd->autoDyn = 0;

    do {
        /* transform the numeric arguments (K) into a DdNode * argument;
         * this allows us to use the standard internal CUDD cache */
        DdNode *VarSet = VarsN, *VarsK = VarsN;
        int     nVars = 0, i;

        /* determine the number of variables in VarSet */
        while ( VarSet != b1 )
        {
            nVars++;
            /* make sure that the VarSet is a cube */
            if ( cuddE( VarSet ) != b0 )
                return NULL;
            VarSet = cuddT( VarSet );
        }
        /* make sure that the number of variables in VarSet is less or equal 
           that the number of variables that should be present in the tuples
        */
        if ( K > nVars )
            return NULL;

        /* the second argument in the recursive call stands for <n>;
         * create the first argument, which stands for <k> 
         * as when we are talking about the tuple of <k> out of <n> */
        for ( i = 0; i < nVars-K; i++ )
            VarsK = cuddT( VarsK );

        dd->reordered = 0;
        res = extraBddTuples(dd, VarsK, VarsN );

    } while (dd->reordered == 1);
    dd->autoDyn = autoDyn;
    return(res);

} /* end of Extra_bddTuples */


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

