/**CFile****************************************************************

  FileName    [extraBddUnate.c]

  PackageName [extra]

  Synopsis    [Efficient methods to compute the information about
  unate variables using an algorithm that is conceptually similar to
  the algorithm for two-variable symmetry computation presented in:
  A. Mishchenko. Fast Computation of Symmetries in Boolean Functions. 
  Transactions on CAD, Nov. 2003.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - September 1, 2003.]

  Revision    [$Id: extraBddUnate.c,v 1.0 2003/09/01 00:00:00 alanmi Exp $]

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

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Computes the classical symmetry information for the function.]

  Description [Returns the symmetry information in the form of Extra_UnateInfo_t structure.]

  SideEffects [If the ZDD variables are not derived from BDD variables with
  multiplicity 2, this function may derive them in a wrong way.]

  SeeAlso     []

******************************************************************************/
Extra_UnateInfo_t * Extra_UnateComputeFast( 
  DdManager * dd,   /* the manager */
  DdNode * bFunc)   /* the function whose symmetries are computed */
{
    DdNode * bSupp;
    DdNode * zRes;
    Extra_UnateInfo_t * p;

    bSupp = Cudd_Support( dd, bFunc );                      Cudd_Ref( bSupp );
    zRes  = Extra_zddUnateInfoCompute( dd, bFunc, bSupp );  Cudd_Ref( zRes );

    p = Extra_UnateInfoCreateFromZdd( dd, zRes, bSupp );

    Cudd_RecursiveDeref( dd, bSupp );
    Cudd_RecursiveDerefZdd( dd, zRes );

    return p;

} /* end of Extra_UnateInfoCompute */


/**Function********************************************************************

  Synopsis    [Computes the classical symmetry information as a ZDD.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_zddUnateInfoCompute( 
  DdManager * dd,   /* the DD manager */
  DdNode * bF,
  DdNode * bVars) 
{
    DdNode * res;
    do {
        dd->reordered = 0;
        res = extraZddUnateInfoCompute( dd, bF, bVars );
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddUnateInfoCompute */


/**Function********************************************************************

  Synopsis    [Converts a set of variables into a set of singleton subsets.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_zddGetSingletonsBoth( 
  DdManager * dd,    /* the DD manager */
  DdNode * bVars)    /* the set of variables */
{
    DdNode * res;
    do {
        dd->reordered = 0;
        res = extraZddGetSingletonsBoth( dd, bVars );
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddGetSingletonsBoth */

/**Function********************************************************************

  Synopsis    [Allocates unateness information structure.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
Extra_UnateInfo_t *  Extra_UnateInfoAllocate( int nVars )
{
    Extra_UnateInfo_t * p;
    // allocate and clean the storage for unateness info
    p = ABC_ALLOC( Extra_UnateInfo_t, 1 );
    memset( p, 0, sizeof(Extra_UnateInfo_t) );
    p->nVars     = nVars;
    p->pVars     = ABC_ALLOC( Extra_UnateVar_t, nVars );  
    memset( p->pVars, 0, nVars * sizeof(Extra_UnateVar_t) );
    return p;
} /* end of Extra_UnateInfoAllocate */

/**Function********************************************************************

  Synopsis    [Deallocates symmetry information structure.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Extra_UnateInfoDissolve( Extra_UnateInfo_t * p )
{
    ABC_FREE( p->pVars );
    ABC_FREE( p );
} /* end of Extra_UnateInfoDissolve */

/**Function********************************************************************

  Synopsis    [Allocates symmetry information structure.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Extra_UnateInfoPrint( Extra_UnateInfo_t * p )
{
    char * pBuffer;
    int i;
    pBuffer = ABC_ALLOC( char, p->nVarsMax+1 );
    memset( pBuffer, ' ', (size_t)p->nVarsMax );
    pBuffer[p->nVarsMax] = 0;
    for ( i = 0; i < p->nVars; i++ )
        if ( p->pVars[i].Neg )
            pBuffer[ p->pVars[i].iVar ] = 'n';
        else if ( p->pVars[i].Pos )
            pBuffer[ p->pVars[i].iVar ] = 'p';
        else
            pBuffer[ p->pVars[i].iVar ] = '.';
    printf( "%s\n", pBuffer );
    ABC_FREE( pBuffer );
} /* end of Extra_UnateInfoPrint */


/**Function********************************************************************

  Synopsis    [Creates the symmetry information structure from ZDD.]

  Description [ZDD representation of symmetries is the set of cubes, each
  of which has two variables in the positive polarity. These variables correspond
  to the symmetric variable pair.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
Extra_UnateInfo_t * Extra_UnateInfoCreateFromZdd( DdManager * dd, DdNode * zPairs, DdNode * bSupp )
{
    Extra_UnateInfo_t * p;
    DdNode * bTemp, * zSet, * zCube, * zTemp;
    int * pMapVars2Nums;
    int i, nSuppSize;

    nSuppSize = Extra_bddSuppSize( dd, bSupp );

    // allocate and clean the storage for symmetry info
    p = Extra_UnateInfoAllocate( nSuppSize );

    // allocate the storage for the temporary map
    pMapVars2Nums = ABC_ALLOC( int, dd->size );
    memset( pMapVars2Nums, 0, dd->size * sizeof(int) );

    // assign the variables
    p->nVarsMax = dd->size;
    for ( i = 0, bTemp = bSupp; bTemp != b1; bTemp = cuddT(bTemp), i++ )
    {
        p->pVars[i].iVar = bTemp->index;
        pMapVars2Nums[bTemp->index] = i;
    }

    // write the symmetry info into the structure
    zSet = zPairs;   Cudd_Ref( zSet );
//    Cudd_zddPrintCover( dd, zPairs );    printf( "\n" );
    while ( zSet != z0 )
    {
        // get the next cube
        zCube  = Extra_zddSelectOneSubset( dd, zSet );    Cudd_Ref( zCube );

        // add this var to the data structure
        assert( cuddT(zCube) == z1 && cuddE(zCube) == z0 );
        if ( zCube->index & 1 ) // neg
            p->pVars[ pMapVars2Nums[zCube->index/2] ].Neg = 1;
        else
            p->pVars[ pMapVars2Nums[zCube->index/2] ].Pos = 1;
        // count the unate vars
        p->nUnate++;

        // update the cuver and deref the cube
        zSet = Cudd_zddDiff( dd, zTemp = zSet, zCube );     Cudd_Ref( zSet );
        Cudd_RecursiveDerefZdd( dd, zTemp );
        Cudd_RecursiveDerefZdd( dd, zCube );

    } // for each cube 
    Cudd_RecursiveDerefZdd( dd, zSet );
    ABC_FREE( pMapVars2Nums );
    return p;

} /* end of Extra_UnateInfoCreateFromZdd */



/**Function********************************************************************

  Synopsis    [Computes the classical unateness information for the function.]

  Description [Uses the naive way of comparing cofactors.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
Extra_UnateInfo_t * Extra_UnateComputeSlow( DdManager * dd, DdNode * bFunc )
{
    int nSuppSize;
    DdNode * bSupp, * bTemp;
    Extra_UnateInfo_t * p;
    int i, Res;

    // compute the support
    bSupp = Cudd_Support( dd, bFunc );   Cudd_Ref( bSupp );
    nSuppSize = Extra_bddSuppSize( dd, bSupp );
//printf( "Support = %d. ", nSuppSize );
//Extra_bddPrint( dd, bSupp );
//printf( "%d ", nSuppSize );

    // allocate the storage for symmetry info
    p = Extra_UnateInfoAllocate( nSuppSize );

    // assign the variables
    p->nVarsMax = dd->size;
    for ( i = 0, bTemp = bSupp; bTemp != b1; bTemp = cuddT(bTemp), i++ )
    {
        Res = Extra_bddCheckUnateNaive( dd, bFunc, bTemp->index );
        p->pVars[i].iVar = bTemp->index;
        if ( Res == -1 )
            p->pVars[i].Neg = 1;
        else if ( Res == 1 )
            p->pVars[i].Pos = 1;
        p->nUnate += (Res != 0);
    }
    Cudd_RecursiveDeref( dd, bSupp );
    return p;

} /* end of Extra_UnateComputeSlow */

/**Function********************************************************************

  Synopsis    [Checks if the two variables are symmetric.]

  Description [Returns 0 if vars are not unate. Return -1/+1 if the var is neg/pos unate.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_bddCheckUnateNaive( 
  DdManager * dd,   /* the DD manager */
  DdNode * bF,
  int iVar) 
{
    DdNode * bCof0, * bCof1;
    int Res;

    assert( iVar < dd->size );

    bCof0 = Cudd_Cofactor( dd, bF, Cudd_Not(Cudd_bddIthVar(dd,iVar)) );  Cudd_Ref( bCof0 );
    bCof1 = Cudd_Cofactor( dd, bF, Cudd_bddIthVar(dd,iVar) );            Cudd_Ref( bCof1 );

    if ( Cudd_bddLeq( dd, bCof0, bCof1 ) )
        Res = 1;
    else if ( Cudd_bddLeq( dd, bCof1, bCof0 ) )
        Res =-1;
    else
        Res = 0;

    Cudd_RecursiveDeref( dd, bCof0 );
    Cudd_RecursiveDeref( dd, bCof1 );
    return Res;
} /* end of Extra_bddCheckUnateNaive */



/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Performs a recursive step of Extra_UnateInfoCompute.]

  Description [Returns the set of symmetric variable pairs represented as a set 
  of two-literal ZDD cubes. Both variables always appear in the positive polarity
  in the cubes. This function works without building new BDD nodes. Some relatively 
  small number of ZDD nodes may be built to ensure proper bookkeeping of the 
  symmetry information.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * 
extraZddUnateInfoCompute( 
  DdManager * dd,   /* the manager */
  DdNode * bFunc,   /* the function whose symmetries are computed */
  DdNode * bVars )  /* the set of variables on which this function depends */
{
    DdNode * zRes;
    DdNode * bFR = Cudd_Regular(bFunc); 

    if ( cuddIsConstant(bFR) )
    {
        if ( cuddIsConstant(bVars) )
            return z0;
        return extraZddGetSingletonsBoth( dd, bVars );
    }
    assert( bVars != b1 );

    if ( (zRes = cuddCacheLookup2Zdd(dd, extraZddUnateInfoCompute, bFunc, bVars)) )
        return zRes;
    else
    {
        DdNode * zRes0, * zRes1;
        DdNode * zTemp, * zPlus;             
        DdNode * bF0, * bF1;             
        DdNode * bVarsNew;
        int nVarsExtra;
        int LevelF;
        int AddVar;

        // every variable in bF should be also in bVars, therefore LevelF cannot be above LevelV
        // if LevelF is below LevelV, scroll through the vars in bVars to the same level as F
        // count how many extra vars are there in bVars
        nVarsExtra = 0;
        LevelF = dd->perm[bFR->index];
        for ( bVarsNew = bVars; LevelF > dd->perm[bVarsNew->index]; bVarsNew = cuddT(bVarsNew) )
            nVarsExtra++; 
        // the indexes (level) of variables should be synchronized now
        assert( bFR->index == bVarsNew->index );

        // cofactor the function
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

        // solve subproblems
        zRes0 = extraZddUnateInfoCompute( dd, bF0, cuddT(bVarsNew) );
        if ( zRes0 == NULL )
            return NULL;
        cuddRef( zRes0 );

        // if there is no symmetries in the negative cofactor
        // there is no need to test the positive cofactor
        if ( zRes0 == z0 )
            zRes = zRes0;  // zRes takes reference
        else
        {
            zRes1 = extraZddUnateInfoCompute( dd, bF1, cuddT(bVarsNew) );
            if ( zRes1 == NULL )
            {
                Cudd_RecursiveDerefZdd( dd, zRes0 );
                return NULL;
            }
            cuddRef( zRes1 );

            // only those variables are pair-wise symmetric 
            // that are pair-wise symmetric in both cofactors
            // therefore, intersect the solutions
            zRes = cuddZddIntersect( dd, zRes0, zRes1 );
            if ( zRes == NULL )
            {
                Cudd_RecursiveDerefZdd( dd, zRes0 );
                Cudd_RecursiveDerefZdd( dd, zRes1 );
                return NULL;
            }
            cuddRef( zRes );
            Cudd_RecursiveDerefZdd( dd, zRes0 );
            Cudd_RecursiveDerefZdd( dd, zRes1 );
        }

        // consider the current top-most variable
        AddVar = -1;
        if ( Cudd_bddLeq( dd, bF0, bF1 ) ) // pos
            AddVar = 0;
        else if ( Cudd_bddLeq( dd, bF1, bF0 ) ) // neg
            AddVar = 1;
        if ( AddVar >= 0 )
        {
            // create the singleton
            zPlus = cuddZddGetNode( dd, 2*bFR->index + AddVar, z1, z0 );
            if ( zPlus == NULL ) 
            {
                Cudd_RecursiveDerefZdd( dd, zRes );
                return NULL;
            }
            cuddRef( zPlus );

            // add these to the result
            zRes = cuddZddUnion( dd, zTemp = zRes, zPlus );
            if ( zRes == NULL )
            {
                Cudd_RecursiveDerefZdd( dd, zTemp );
                Cudd_RecursiveDerefZdd( dd, zPlus );
                return NULL;
            }
            cuddRef( zRes );
            Cudd_RecursiveDerefZdd( dd, zTemp );
            Cudd_RecursiveDerefZdd( dd, zPlus );
        }
        // only zRes is referenced at this point

        // if we skipped some variables, these variables cannot be symmetric with
        // any variables that are currently in the support of bF, but they can be 
        // symmetric with the variables that are in bVars but not in the support of bF
        for ( bVarsNew = bVars; LevelF > dd->perm[bVarsNew->index]; bVarsNew = cuddT(bVarsNew) )
        {
            // create the negative singleton
            zPlus = cuddZddGetNode( dd, 2*bVarsNew->index+1, z1, z0 );
            if ( zPlus == NULL ) 
            {
                Cudd_RecursiveDerefZdd( dd, zRes );
                return NULL;
            }
            cuddRef( zPlus );

            // add these to the result
            zRes = cuddZddUnion( dd, zTemp = zRes, zPlus );
            if ( zRes == NULL )
            {
                Cudd_RecursiveDerefZdd( dd, zTemp );
                Cudd_RecursiveDerefZdd( dd, zPlus );
                return NULL;
            }
            cuddRef( zRes );
            Cudd_RecursiveDerefZdd( dd, zTemp );
            Cudd_RecursiveDerefZdd( dd, zPlus );
        

            // create the positive singleton
            zPlus = cuddZddGetNode( dd, 2*bVarsNew->index, z1, z0 );
            if ( zPlus == NULL ) 
            {
                Cudd_RecursiveDerefZdd( dd, zRes );
                return NULL;
            }
            cuddRef( zPlus );

            // add these to the result
            zRes = cuddZddUnion( dd, zTemp = zRes, zPlus );
            if ( zRes == NULL )
            {
                Cudd_RecursiveDerefZdd( dd, zTemp );
                Cudd_RecursiveDerefZdd( dd, zPlus );
                return NULL;
            }
            cuddRef( zRes );
            Cudd_RecursiveDerefZdd( dd, zTemp );
            Cudd_RecursiveDerefZdd( dd, zPlus );
        }
        cuddDeref( zRes );

        /* insert the result into cache */
        cuddCacheInsert2(dd, extraZddUnateInfoCompute, bFunc, bVars, zRes);
        return zRes;
    }
} /* end of extraZddUnateInfoCompute */


/**Function********************************************************************

  Synopsis    [Performs a recursive step of Extra_zddGetSingletons.]

  Description [Returns the set of ZDD singletons, containing those pos/neg 
  polarity ZDD variables that correspond to the BDD variables in bVars.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraZddGetSingletonsBoth( 
  DdManager * dd,    /* the DD manager */
  DdNode * bVars)    /* the set of variables */
{
    DdNode * zRes;

    if ( bVars == b1 )
        return z1;

    if ( (zRes = cuddCacheLookup1Zdd(dd, extraZddGetSingletonsBoth, bVars)) )
        return zRes;
    else
    {
        DdNode * zTemp, * zPlus;          

        // solve subproblem
        zRes = extraZddGetSingletonsBoth( dd, cuddT(bVars) );
        if ( zRes == NULL ) 
            return NULL;
        cuddRef( zRes );

        
        // create the negative singleton
        zPlus = cuddZddGetNode( dd, 2*bVars->index+1, z1, z0 );
        if ( zPlus == NULL ) 
        {
            Cudd_RecursiveDerefZdd( dd, zRes );
            return NULL;
        }
        cuddRef( zPlus );

        // add these to the result
        zRes = cuddZddUnion( dd, zTemp = zRes, zPlus );
        if ( zRes == NULL )
        {
            Cudd_RecursiveDerefZdd( dd, zTemp );
            Cudd_RecursiveDerefZdd( dd, zPlus );
            return NULL;
        }
        cuddRef( zRes );
        Cudd_RecursiveDerefZdd( dd, zTemp );
        Cudd_RecursiveDerefZdd( dd, zPlus );
        

        // create the positive singleton
        zPlus = cuddZddGetNode( dd, 2*bVars->index, z1, z0 );
        if ( zPlus == NULL ) 
        {
            Cudd_RecursiveDerefZdd( dd, zRes );
            return NULL;
        }
        cuddRef( zPlus );

        // add these to the result
        zRes = cuddZddUnion( dd, zTemp = zRes, zPlus );
        if ( zRes == NULL )
        {
            Cudd_RecursiveDerefZdd( dd, zTemp );
            Cudd_RecursiveDerefZdd( dd, zPlus );
            return NULL;
        }
        cuddRef( zRes );
        Cudd_RecursiveDerefZdd( dd, zTemp );
        Cudd_RecursiveDerefZdd( dd, zPlus );

        cuddDeref( zRes );
        cuddCacheInsert1( dd, extraZddGetSingletonsBoth, bVars, zRes );
        return zRes;
    }
}   /* end of extraZddGetSingletonsBoth */


/*---------------------------------------------------------------------------*/
/* Definition of static Functions                                            */
/*---------------------------------------------------------------------------*/
ABC_NAMESPACE_IMPL_END

