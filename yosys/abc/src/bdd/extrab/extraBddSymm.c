/**CFile****************************************************************

  FileName    [extraBddSymm.c]

  PackageName [extra]

  Synopsis    [Efficient methods to compute the information about
  symmetric variables using the algorithm presented in the paper:
  A. Mishchenko. Fast Computation of Symmetries in Boolean Functions. 
  Transactions on CAD, Nov. 2003.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - September 1, 2003.]

  Revision    [$Id: extraBddSymm.c,v 1.0 2003/09/01 00:00:00 alanmi Exp $]

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

#define DD_GET_SYMM_VARS_TAG          0x0a /* former DD_BDD_XOR_EXIST_ABSTRACT_TAG */

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

  Description [Returns the symmetry information in the form of Extra_SymmInfo_t structure.]

  SideEffects [If the ZDD variables are not derived from BDD variables with
  multiplicity 2, this function may derive them in a wrong way.]

  SeeAlso     []

******************************************************************************/
Extra_SymmInfo_t * Extra_SymmPairsCompute( 
  DdManager * dd,   /* the manager */
  DdNode * bFunc)   /* the function whose symmetries are computed */
{
    DdNode * bSupp;
    DdNode * zRes;
    Extra_SymmInfo_t * p;

    bSupp = Cudd_Support( dd, bFunc );                      Cudd_Ref( bSupp );
    zRes  = Extra_zddSymmPairsCompute( dd, bFunc, bSupp );  Cudd_Ref( zRes );

    p = Extra_SymmPairsCreateFromZdd( dd, zRes, bSupp );

    Cudd_RecursiveDeref( dd, bSupp );
    Cudd_RecursiveDerefZdd( dd, zRes );

    return p;

} /* end of Extra_SymmPairsCompute */


/**Function********************************************************************

  Synopsis    [Computes the classical symmetry information as a ZDD.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_zddSymmPairsCompute( 
  DdManager * dd,   /* the DD manager */
  DdNode * bF,
  DdNode * bVars) 
{
    DdNode * res;
    do {
        dd->reordered = 0;
        res = extraZddSymmPairsCompute( dd, bF, bVars );
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddSymmPairsCompute */

/**Function********************************************************************

  Synopsis    [Returns a singleton-set ZDD containing all variables that are symmetric with the given one.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_zddGetSymmetricVars( 
  DdManager * dd,    /* the DD manager */
  DdNode * bF,       /* the first function  - originally, the positive cofactor */
  DdNode * bG,       /* the second fucntion - originally, the negative cofactor */
  DdNode * bVars)    /* the set of variables, on which F and G depend */
{
    DdNode * res;
    do {
        dd->reordered = 0;
        res = extraZddGetSymmetricVars( dd, bF, bG, bVars );
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddGetSymmetricVars */


/**Function********************************************************************

  Synopsis    [Converts a set of variables into a set of singleton subsets.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_zddGetSingletons( 
  DdManager * dd,    /* the DD manager */
  DdNode * bVars)    /* the set of variables */
{
    DdNode * res;
    do {
        dd->reordered = 0;
        res = extraZddGetSingletons( dd, bVars );
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddGetSingletons */

/**Function********************************************************************

  Synopsis    [Filters the set of variables using the support of the function.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_bddReduceVarSet( 
  DdManager * dd,    /* the DD manager */
  DdNode * bVars,    /* the set of variables to be reduced */
  DdNode * bF)       /* the function whose support is used for reduction */
{
    DdNode * res;
    do {
        dd->reordered = 0;
        res = extraBddReduceVarSet( dd, bVars, bF );
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_bddReduceVarSet */


/**Function********************************************************************

  Synopsis    [Allocates symmetry information structure.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
Extra_SymmInfo_t * Extra_SymmPairsAllocate( int nVars )
{
    int i;
    Extra_SymmInfo_t * p;

    // allocate and clean the storage for symmetry info
    p = ABC_ALLOC( Extra_SymmInfo_t, 1 );
    memset( p, 0, sizeof(Extra_SymmInfo_t) );
    p->nVars     = nVars;
    p->pVars     = ABC_ALLOC( int, nVars );  
    p->pSymms    = ABC_ALLOC( char *, nVars );  
    p->pSymms[0] = ABC_ALLOC( char  , nVars * nVars );
    memset( p->pSymms[0], 0, nVars * nVars * sizeof(char) );

    for ( i = 1; i < nVars; i++ )
        p->pSymms[i] = p->pSymms[i-1] + nVars;

    return p;
} /* end of Extra_SymmPairsAllocate */

/**Function********************************************************************

  Synopsis    [Deallocates symmetry information structure.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Extra_SymmPairsDissolve( Extra_SymmInfo_t * p )
{
    ABC_FREE( p->pVars );
    ABC_FREE( p->pSymms[0] );
    ABC_FREE( p->pSymms    );
    ABC_FREE( p );
} /* end of Extra_SymmPairsDissolve */

/**Function********************************************************************

  Synopsis    [Allocates symmetry information structure.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Extra_SymmPairsPrint( Extra_SymmInfo_t * p )
{
    int i, k;
    printf( "\n" );
    for ( i = 0; i < p->nVars; i++ )
    {
        for ( k = 0; k <= i; k++ )
            printf( " " );
        for ( k = i+1; k < p->nVars; k++ )
            if ( p->pSymms[i][k] )
                printf( "1" );
            else
                printf( "." );
        printf( "\n" );
    }
} /* end of Extra_SymmPairsPrint */


/**Function********************************************************************

  Synopsis    [Creates the symmetry information structure from ZDD.]

  Description [ZDD representation of symmetries is the set of cubes, each
  of which has two variables in the positive polarity. These variables correspond
  to the symmetric variable pair.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
Extra_SymmInfo_t * Extra_SymmPairsCreateFromZdd( DdManager * dd, DdNode * zPairs, DdNode * bSupp )
{
    int i;
    int nSuppSize;
    Extra_SymmInfo_t * p;
    int * pMapVars2Nums;
    DdNode * bTemp;
    DdNode * zSet, * zCube, * zTemp;
    int iVar1, iVar2;

    nSuppSize = Extra_bddSuppSize( dd, bSupp );

    // allocate and clean the storage for symmetry info
    p = Extra_SymmPairsAllocate( nSuppSize );

    // allocate the storage for the temporary map
    pMapVars2Nums = ABC_ALLOC( int, dd->size );
    memset( pMapVars2Nums, 0, dd->size * sizeof(int) );

    // assign the variables
    p->nVarsMax = dd->size;
//  p->nNodes   = Cudd_DagSize( zPairs );
    p->nNodes   = 0;
    for ( i = 0, bTemp = bSupp; bTemp != b1; bTemp = cuddT(bTemp), i++ )
    {
        p->pVars[i] = bTemp->index;
        pMapVars2Nums[bTemp->index] = i;
    }

    // write the symmetry info into the structure
    zSet = zPairs;   Cudd_Ref( zSet );
    while ( zSet != z0 )
    {
        // get the next cube
        zCube  = Extra_zddSelectOneSubset( dd, zSet );    Cudd_Ref( zCube );

        // add these two variables to the data structure
        assert( cuddT( cuddT(zCube) ) == z1 );
        iVar1 = zCube->index/2;
        iVar2 = cuddT(zCube)->index/2;
        if ( pMapVars2Nums[iVar1] < pMapVars2Nums[iVar2] )
            p->pSymms[ pMapVars2Nums[iVar1] ][ pMapVars2Nums[iVar2] ] = 1;
        else
            p->pSymms[ pMapVars2Nums[iVar2] ][ pMapVars2Nums[iVar1] ] = 1;
        // count the symmetric pairs
        p->nSymms ++;

        // update the cuver and deref the cube
        zSet = Cudd_zddDiff( dd, zTemp = zSet, zCube );     Cudd_Ref( zSet );
        Cudd_RecursiveDerefZdd( dd, zTemp );
        Cudd_RecursiveDerefZdd( dd, zCube );

    } // for each cube 
    Cudd_RecursiveDerefZdd( dd, zSet );

    ABC_FREE( pMapVars2Nums );
    return p;

} /* end of Extra_SymmPairsCreateFromZdd */


/**Function********************************************************************

  Synopsis    [Checks the possibility of two variables being symmetric.]

  Description [Returns 0 if vars are not symmetric. Return 1 if vars can be symmetric.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_bddCheckVarsSymmetric( 
  DdManager * dd,   /* the DD manager */
  DdNode * bF,
  int iVar1,
  int iVar2) 
{
    DdNode * bVars;
    int Res;

//  return 1;

    assert( iVar1 != iVar2 );
    assert( iVar1 < dd->size );
    assert( iVar2 < dd->size );

    bVars = Cudd_bddAnd( dd, dd->vars[iVar1], dd->vars[iVar2] );   Cudd_Ref( bVars );

    Res = (int)( extraBddCheckVarsSymmetric( dd, bF, bVars ) == b1 );

    Cudd_RecursiveDeref( dd, bVars );

    return Res;
} /* end of Extra_bddCheckVarsSymmetric */


/**Function********************************************************************

  Synopsis    [Computes the classical symmetry information for the function.]

  Description [Uses the naive way of comparing cofactors.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
Extra_SymmInfo_t * Extra_SymmPairsComputeNaive( DdManager * dd, DdNode * bFunc )
{
    DdNode * bSupp, * bTemp;
    int nSuppSize;
    Extra_SymmInfo_t * p;
    int i, k;

    // compute the support
    bSupp = Cudd_Support( dd, bFunc );   Cudd_Ref( bSupp );
    nSuppSize = Extra_bddSuppSize( dd, bSupp );
//printf( "Support = %d. ", nSuppSize );
//Extra_bddPrint( dd, bSupp );
//printf( "%d ", nSuppSize );

    // allocate the storage for symmetry info
    p = Extra_SymmPairsAllocate( nSuppSize );

    // assign the variables
    p->nVarsMax = dd->size;
    for ( i = 0, bTemp = bSupp; bTemp != b1; bTemp = cuddT(bTemp), i++ )
        p->pVars[i] = bTemp->index;

    // go through the candidate pairs and check using Idea1
    for ( i = 0; i < nSuppSize; i++ )
    for ( k = i+1; k < nSuppSize; k++ )
    {
        p->pSymms[k][i] = p->pSymms[i][k] = Extra_bddCheckVarsSymmetricNaive( dd, bFunc, p->pVars[i], p->pVars[k] );
        if ( p->pSymms[i][k] )
             p->nSymms++;
    }

    Cudd_RecursiveDeref( dd, bSupp );
    return p;

} /* end of Extra_SymmPairsComputeNaive */

/**Function********************************************************************

  Synopsis    [Checks if the two variables are symmetric.]

  Description [Returns 0 if vars are not symmetric. Return 1 if vars are symmetric.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_bddCheckVarsSymmetricNaive( 
  DdManager * dd,   /* the DD manager */
  DdNode * bF,
  int iVar1,
  int iVar2) 
{
    DdNode * bCube1, * bCube2;
    DdNode * bCof01, * bCof10;
    int Res;

    assert( iVar1 != iVar2 );
    assert( iVar1 < dd->size );
    assert( iVar2 < dd->size );

    bCube1 = Cudd_bddAnd( dd, Cudd_Not( dd->vars[iVar1] ), dd->vars[iVar2] );   Cudd_Ref( bCube1 );
    bCube2 = Cudd_bddAnd( dd, Cudd_Not( dd->vars[iVar2] ), dd->vars[iVar1] );   Cudd_Ref( bCube2 );

    bCof01 = Cudd_Cofactor( dd, bF, bCube1 );  Cudd_Ref( bCof01 );
    bCof10 = Cudd_Cofactor( dd, bF, bCube2 );  Cudd_Ref( bCof10 );

    Res = (int)( bCof10 == bCof01 );

    Cudd_RecursiveDeref( dd, bCof01 );
    Cudd_RecursiveDeref( dd, bCof10 );
    Cudd_RecursiveDeref( dd, bCube1 );
    Cudd_RecursiveDeref( dd, bCube2 );

    return Res;
} /* end of Extra_bddCheckVarsSymmetricNaive */


/**Function********************************************************************

  Synopsis    [Builds ZDD representing the set of fixed-size variable tuples.]

  Description [Creates ZDD of all combinations of variables in Support that
  is represented by a BDD.]

  SideEffects [New ZDD variables are created if indices of the variables
               present in the combination are larger than the currently
               allocated number of ZDD variables.]

  SeeAlso     []

******************************************************************************/
DdNode* Extra_zddTuplesFromBdd( 
  DdManager * dd,   /* the DD manager */
  int K,            /* the number of variables in tuples */
  DdNode * bVarsN)   /* the set of all variables represented as a BDD */
{
    DdNode  *zRes;
    int     autoDynZ;

    autoDynZ = dd->autoDynZ;
    dd->autoDynZ = 0;

    do {
        /* transform the numeric arguments (K) into a DdNode* argument;
         * this allows us to use the standard internal CUDD cache */
        DdNode *bVarSet = bVarsN, *bVarsK = bVarsN;
        int     nVars = 0, i;

        /* determine the number of variables in VarSet */
        while ( bVarSet != b1 )
        {
            nVars++;
            /* make sure that the VarSet is a cube */
            if ( cuddE( bVarSet ) != b0 )
                return NULL;
            bVarSet = cuddT( bVarSet );
        }
        /* make sure that the number of variables in VarSet is less or equal 
           that the number of variables that should be present in the tuples
        */
        if ( K > nVars )
            return NULL;

        /* the second argument in the recursive call stannds for <n>;
         * reate the first argument, which stands for <k>
         * as when we are talking about the tuple of <k> out of <n> */
        for ( i = 0; i < nVars-K; i++ )
            bVarsK = cuddT( bVarsK );

        dd->reordered = 0;
        zRes = extraZddTuplesFromBdd(dd, bVarsK, bVarsN );

    } while (dd->reordered == 1);
    dd->autoDynZ = autoDynZ;
    return zRes;

} /* end of Extra_zddTuplesFromBdd */

/**Function********************************************************************

  Synopsis    [Selects one subset from the set of subsets represented by a ZDD.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode* Extra_zddSelectOneSubset( 
  DdManager * dd,   /* the DD manager */
  DdNode * zS)      /* the ZDD */
{
    DdNode  *res;
    do {
        dd->reordered = 0;
        res = extraZddSelectOneSubset(dd, zS);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddSelectOneSubset */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Performs a recursive step of Extra_SymmPairsCompute.]

  Description [Returns the set of symmetric variable pairs represented as a set 
  of two-literal ZDD cubes. Both variables always appear in the positive polarity
  in the cubes. This function works without building new BDD nodes. Some relatively 
  small number of ZDD nodes may be built to ensure proper bookkeeping of the 
  symmetry information.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * 
extraZddSymmPairsCompute( 
  DdManager * dd,   /* the manager */
  DdNode * bFunc,   /* the function whose symmetries are computed */
  DdNode * bVars )  /* the set of variables on which this function depends */
{
    DdNode * zRes;
    DdNode * bFR = Cudd_Regular(bFunc); 

    if ( cuddIsConstant(bFR) )
    {
        int nVars, i;

        // determine how many vars are in the bVars
        nVars = Extra_bddSuppSize( dd, bVars );
        if ( nVars < 2 )
            return z0;
        else
        {
            DdNode * bVarsK;

            // create the BDD bVarsK corresponding to K = 2;
            bVarsK = bVars;
            for ( i = 0; i < nVars-2; i++ )
                bVarsK = cuddT( bVarsK );
            return extraZddTuplesFromBdd( dd, bVarsK, bVars );
        }
    }
    assert( bVars != b1 );

    if ( (zRes = cuddCacheLookup2Zdd(dd, extraZddSymmPairsCompute, bFunc, bVars)) )
        return zRes;
    else
    {
        DdNode * zRes0, * zRes1;
        DdNode * zTemp, * zPlus, * zSymmVars;             
        DdNode * bF0, * bF1;             
        DdNode * bVarsNew;
        int nVarsExtra;
        int LevelF;

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
        zRes0 = extraZddSymmPairsCompute( dd, bF0, cuddT(bVarsNew) );
        if ( zRes0 == NULL )
            return NULL;
        cuddRef( zRes0 );

        // if there is no symmetries in the negative cofactor
        // there is no need to test the positive cofactor
        if ( zRes0 == z0 )
            zRes = zRes0;  // zRes takes reference
        else
        {
            zRes1 = extraZddSymmPairsCompute( dd, bF1, cuddT(bVarsNew) );
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

        // consider the current top-most variable and find all the vars
        // that are pairwise symmetric with it
        // these variables are returned as a set of ZDD singletons
        zSymmVars = extraZddGetSymmetricVars( dd, bF1, bF0, cuddT(bVarsNew) );
        if ( zSymmVars == NULL )
        {
            Cudd_RecursiveDerefZdd( dd, zRes );
            return NULL;
        }
        cuddRef( zSymmVars );

        // attach the topmost variable to the set, to get the variable pairs
        // use the positive polarity ZDD variable for the purpose

        // there is no need to do so, if zSymmVars is empty
        if ( zSymmVars == z0 )
            Cudd_RecursiveDerefZdd( dd, zSymmVars );
        else
        {
            zPlus = cuddZddGetNode( dd, 2*bFR->index, zSymmVars, z0 );
            if ( zPlus == NULL ) 
            {
                Cudd_RecursiveDerefZdd( dd, zRes );
                Cudd_RecursiveDerefZdd( dd, zSymmVars );
                return NULL;
            }
            cuddRef( zPlus );
            cuddDeref( zSymmVars );

            // add these variable pairs to the result
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
        if ( nVarsExtra )
        {
            // it is possible to improve this step:
            // (1) there is no need to enter here, if nVarsExtra < 2

            // create the set of topmost nVarsExtra in bVars
            DdNode * bVarsExtra;
            int nVars;

            // remove from bVars all the variable that are in the support of bFunc
            bVarsExtra = extraBddReduceVarSet( dd, bVars, bFunc );  
            if ( bVarsExtra == NULL )
            {
                Cudd_RecursiveDerefZdd( dd, zRes );
                return NULL;
            }
            cuddRef( bVarsExtra );

            // determine how many vars are in the bVarsExtra
            nVars = Extra_bddSuppSize( dd, bVarsExtra );
            if ( nVars < 2 )
            {
                Cudd_RecursiveDeref( dd, bVarsExtra );
            }
            else
            {
                int i;
                DdNode * bVarsK;

                // create the BDD bVarsK corresponding to K = 2;
                bVarsK = bVarsExtra;
                for ( i = 0; i < nVars-2; i++ )
                    bVarsK = cuddT( bVarsK );

                // create the 2 variable tuples
                zPlus = extraZddTuplesFromBdd( dd, bVarsK, bVarsExtra );
                if ( zPlus == NULL )
                {
                    Cudd_RecursiveDeref( dd, bVarsExtra );
                    Cudd_RecursiveDerefZdd( dd, zRes );
                    return NULL;
                }
                cuddRef( zPlus );
                Cudd_RecursiveDeref( dd, bVarsExtra );

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
        }
        cuddDeref( zRes );


        /* insert the result into cache */
        cuddCacheInsert2(dd, extraZddSymmPairsCompute, bFunc, bVars, zRes);
        return zRes;
    }
} /* end of extraZddSymmPairsCompute */

/**Function********************************************************************

  Synopsis    [Performs a recursive step of Extra_zddGetSymmetricVars.]

  Description [Returns the set of ZDD singletons, containing those positive
  ZDD variables that correspond to BDD variables x, for which it is true 
  that bF(x=0) == bG(x=1).]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraZddGetSymmetricVars( 
  DdManager * dd,    /* the DD manager */
  DdNode * bF,       /* the first function  - originally, the positive cofactor */
  DdNode * bG,       /* the second function - originally, the negative cofactor */
  DdNode * bVars)    /* the set of variables, on which F and G depend */
{
    DdNode * zRes;
    DdNode * bFR = Cudd_Regular(bF); 
    DdNode * bGR = Cudd_Regular(bG); 

    if ( cuddIsConstant(bFR) && cuddIsConstant(bGR) )
    {
        if ( bF == bG )
            return extraZddGetSingletons( dd, bVars );
        else 
            return z0;
    }
    assert( bVars != b1 );

    if ( (zRes = cuddCacheLookupZdd(dd, DD_GET_SYMM_VARS_TAG, bF, bG, bVars)) )
        return zRes;
    else
    {
        DdNode * zRes0, * zRes1;             
        DdNode * zPlus, * zTemp;
        DdNode * bF0, * bF1;             
        DdNode * bG0, * bG1;             
        DdNode * bVarsNew;
    
        int LevelF = cuddI(dd,bFR->index);
        int LevelG = cuddI(dd,bGR->index);
        int LevelFG;

        if ( LevelF < LevelG )
            LevelFG = LevelF;
        else
            LevelFG = LevelG;

        // at least one of the arguments is not a constant
        assert( LevelFG < dd->size );

        // every variable in bF and bG should be also in bVars, therefore LevelFG cannot be above LevelV
        // if LevelFG is below LevelV, scroll through the vars in bVars to the same level as LevelFG
        for ( bVarsNew = bVars; LevelFG > dd->perm[bVarsNew->index]; bVarsNew = cuddT(bVarsNew) );
        assert( LevelFG == dd->perm[bVarsNew->index] );

        // cofactor the functions
        if ( LevelF == LevelFG )
        {
            if ( bFR != bF ) // bF is complemented 
            {
                bF0 = Cudd_Not( cuddE(bFR) );
                bF1 = Cudd_Not( cuddT(bFR) );
            }
            else
            {
                bF0 = cuddE(bFR);
                bF1 = cuddT(bFR);
            }
        }
        else 
            bF0 = bF1 = bF;

        if ( LevelG == LevelFG )
        {
            if ( bGR != bG ) // bG is complemented 
            {
                bG0 = Cudd_Not( cuddE(bGR) );
                bG1 = Cudd_Not( cuddT(bGR) );
            }
            else
            {
                bG0 = cuddE(bGR);
                bG1 = cuddT(bGR);
            }
        }
        else 
            bG0 = bG1 = bG;

        // solve subproblems
        zRes0 = extraZddGetSymmetricVars( dd, bF0, bG0, cuddT(bVarsNew) );
        if ( zRes0 == NULL ) 
            return NULL;
        cuddRef( zRes0 );

        // if there is not symmetries in the negative cofactor
        // there is no need to test the positive cofactor
        if ( zRes0 == z0 )
            zRes = zRes0;  // zRes takes reference
        else
        {
            zRes1 = extraZddGetSymmetricVars( dd, bF1, bG1, cuddT(bVarsNew) );
            if ( zRes1 == NULL ) 
            {
                Cudd_RecursiveDerefZdd( dd, zRes0 );
                return NULL;
            }
            cuddRef( zRes1 );

            // only those variables should belong to the resulting set 
            // for which the property is true for both cofactors
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

        // add one more singleton if the property is true for this variable
        if ( bF0 == bG1 )
        {
            zPlus = cuddZddGetNode( dd, 2*bVarsNew->index, z1, z0 );
            if ( zPlus == NULL ) 
            {
                Cudd_RecursiveDerefZdd( dd, zRes );
                return NULL;
            }
            cuddRef( zPlus );

            // add these variable pairs to the result
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

        if ( bF == bG && bVars != bVarsNew )
        { 
            // if the functions are equal, so are their cofactors
            // add those variables from V that are above F and G 

            DdNode * bVarsExtra;

            assert( LevelFG > dd->perm[bVars->index] );

            // create the BDD of the extra variables
            bVarsExtra = cuddBddExistAbstractRecur( dd, bVars, bVarsNew );
            if ( bVarsExtra == NULL )
            {
                Cudd_RecursiveDerefZdd( dd, zRes );
                return NULL;
            }
            cuddRef( bVarsExtra );

            zPlus = extraZddGetSingletons( dd, bVarsExtra );
            if ( zPlus == NULL )
            {
                Cudd_RecursiveDeref( dd, bVarsExtra );
                Cudd_RecursiveDerefZdd( dd, zRes );
                return NULL;
            }
            cuddRef( zPlus );
            Cudd_RecursiveDeref( dd, bVarsExtra );

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

        cuddCacheInsert( dd, DD_GET_SYMM_VARS_TAG, bF, bG, bVars, zRes );
        return zRes;
    }
}   /* end of extraZddGetSymmetricVars */


/**Function********************************************************************

  Synopsis    [Performs a recursive step of Extra_zddGetSingletons.]

  Description [Returns the set of ZDD singletons, containing those positive 
  polarity ZDD variables that correspond to the BDD variables in bVars.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraZddGetSingletons( 
  DdManager * dd,    /* the DD manager */
  DdNode * bVars)    /* the set of variables */
{
    DdNode * zRes;

    if ( bVars == b1 )
//    if ( bVars == b0 )  // bug fixed by Jin Zhang, Jan 23, 2004
        return z1;

    if ( (zRes = cuddCacheLookup1Zdd(dd, extraZddGetSingletons, bVars)) )
        return zRes;
    else
    {
        DdNode * zTemp, * zPlus;          

        // solve subproblem
        zRes = extraZddGetSingletons( dd, cuddT(bVars) );
        if ( zRes == NULL ) 
            return NULL;
        cuddRef( zRes );
        
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

        cuddCacheInsert1( dd, extraZddGetSingletons, bVars, zRes );
        return zRes;
    }
}   /* end of extraZddGetSingletons */


/**Function********************************************************************

  Synopsis    [Performs a recursive step of Extra_bddReduceVarSet.]

  Description [Returns the set of all variables in the given set that are not in the 
  support of the given function.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraBddReduceVarSet( 
  DdManager * dd,    /* the DD manager */
  DdNode * bVars,    /* the set of variables to be reduced */
  DdNode * bF)       /* the function whose support is used for reduction */
{
    DdNode * bRes;
    DdNode * bFR = Cudd_Regular(bF);

    if ( cuddIsConstant(bFR) || bVars == b1 )
        return bVars;

    if ( (bRes = cuddCacheLookup2(dd, extraBddReduceVarSet, bVars, bF)) )
        return bRes;
    else
    {
        DdNode * bF0, * bF1;             
        DdNode * bVarsThis, * bVarsLower, * bTemp;
        int LevelF;

        // if LevelF is below LevelV, scroll through the vars in bVars 
        LevelF = dd->perm[bFR->index];
        for ( bVarsThis = bVars; LevelF > cuddI(dd,bVarsThis->index); bVarsThis = cuddT(bVarsThis) );
        // scroll also through the current var, because it should be not be added
        if ( LevelF == cuddI(dd,bVarsThis->index) )
            bVarsLower = cuddT(bVarsThis);
        else
            bVarsLower = bVarsThis;

        // cofactor the function
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

        // solve subproblems
        bRes = extraBddReduceVarSet( dd, bVarsLower, bF0 );
        if ( bRes == NULL ) 
            return NULL;
        cuddRef( bRes );

        bRes = extraBddReduceVarSet( dd, bTemp = bRes, bF1 );
        if ( bRes == NULL ) 
        {
            Cudd_RecursiveDeref( dd, bTemp );
            return NULL;
        }
        cuddRef( bRes );
        Cudd_RecursiveDeref( dd, bTemp );

        // the current var should not be added
        // add the skipped vars
        if ( bVarsThis != bVars )
        {
            DdNode * bVarsExtra;

            // extract the skipped variables
            bVarsExtra = cuddBddExistAbstractRecur( dd, bVars, bVarsThis );
            if ( bVarsExtra == NULL )
            {
                Cudd_RecursiveDeref( dd, bRes );
                return NULL;
            }
            cuddRef( bVarsExtra );

            // add these variables
            bRes = cuddBddAndRecur( dd, bTemp = bRes, bVarsExtra );
            if ( bRes == NULL ) 
            {
                Cudd_RecursiveDeref( dd, bTemp );
                Cudd_RecursiveDeref( dd, bVarsExtra );
                return NULL;
            }
            cuddRef( bRes );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bVarsExtra );
        }
        cuddDeref( bRes );

        cuddCacheInsert2( dd, extraBddReduceVarSet, bVars, bF, bRes );
        return bRes;
    }
}   /* end of extraBddReduceVarSet */


/**Function********************************************************************

  Synopsis    [Performs the recursive step of Extra_bddCheckVarsSymmetric().]

  Description [Returns b0 if the variables are not symmetric. Returns b1 if the
  variables can be symmetric. The variables are represented in the form of a 
  two-variable cube. In case the cube contains one variable (below Var1 level),
  the cube's pointer is complemented if the variable Var1 occurred on the 
  current path; otherwise, the cube's pointer is regular. Uses additional 
  complemented bit (Hash_Not) to mark the result if in the BDD rooted that this
  node there is a branch passing though the node labeled with Var2.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraBddCheckVarsSymmetric( 
  DdManager * dd,    /* the DD manager */
  DdNode * bF,
  DdNode * bVars) 
{
    DdNode * bRes;

    if ( bF == b0 )
        return b1;

    assert( bVars != b1 );
    
    if ( (bRes = cuddCacheLookup2(dd, extraBddCheckVarsSymmetric, bF, bVars)) )
        return bRes;
    else
    {
        DdNode * bRes0, * bRes1;
        DdNode * bF0, * bF1;             
        DdNode * bFR = Cudd_Regular(bF);
        int LevelF = cuddI(dd,bFR->index);

        DdNode * bVarsR = Cudd_Regular(bVars);
        int fVar1Pres;
        int iLev1;
        int iLev2;

        if ( bVarsR != bVars ) // cube's pointer is complemented
        {
            assert( cuddT(bVarsR) == b1 );
            fVar1Pres = 1;                    // the first var is present on the path
            iLev1 = -1;                       // we are already below the first var level
            iLev2 = dd->perm[bVarsR->index];  // the level of the second var
        }
        else  // cube's pointer is NOT complemented
        {
            fVar1Pres = 0;                    // the first var is absent on the path
            if ( cuddT(bVars) == b1 )
            {
                iLev1 = -1;                             // we are already below the first var level 
                iLev2 = dd->perm[bVars->index];         // the level of the second var
            }
            else
            {
                assert( cuddT(cuddT(bVars)) == b1 );
                iLev1 = dd->perm[bVars->index];         // the level of the first var 
                iLev2 = dd->perm[cuddT(bVars)->index];  // the level of the second var
            }
        }

        // cofactor the function
        // the cofactors are needed only if we are above the second level
        if ( LevelF < iLev2 )
        {
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
        }
        else
            bF0 = bF1 = NULL;

        // consider five cases:
        // (1) F is above iLev1
        // (2) F is on the level iLev1
        // (3) F is between iLev1 and iLev2
        // (4) F is on the level iLev2
        // (5) F is below iLev2

        // (1) F is above iLev1
        if ( LevelF < iLev1 )
        {
            // the returned result cannot have the hash attribute
            // because we still did not reach the level of Var1;
            // the attribute never travels above the level of Var1
            bRes0 = extraBddCheckVarsSymmetric( dd, bF0, bVars );
//          assert( !Hash_IsComplement( bRes0 ) );
            assert( bRes0 != z0 );
            if ( bRes0 == b0 )
                bRes = b0;
            else
                bRes = extraBddCheckVarsSymmetric( dd, bF1, bVars );
//          assert( !Hash_IsComplement( bRes ) );
            assert( bRes != z0 );
        }
        // (2) F is on the level iLev1
        else if ( LevelF == iLev1 )
        {
            bRes0 = extraBddCheckVarsSymmetric( dd, bF0, Cudd_Not( cuddT(bVars) ) );
            if ( bRes0 == b0 )
                bRes = b0;
            else
            {
                bRes1 = extraBddCheckVarsSymmetric( dd, bF1, Cudd_Not( cuddT(bVars) ) );
                if ( bRes1 == b0 )
                    bRes = b0;
                else
                {
//                  if ( Hash_IsComplement( bRes0 ) || Hash_IsComplement( bRes1 ) )
                    if ( bRes0 == z0 || bRes1 == z0 )
                        bRes = b1;
                    else
                        bRes = b0;
                }
            }
        }
        // (3) F is between iLev1 and iLev2
        else if ( LevelF < iLev2 )
        {
            bRes0 = extraBddCheckVarsSymmetric( dd, bF0, bVars );
            if ( bRes0 == b0 )
                bRes = b0;
            else
            {
                bRes1 = extraBddCheckVarsSymmetric( dd, bF1, bVars );
                if ( bRes1 == b0 )
                    bRes = b0;
                else
                {
//                  if ( Hash_IsComplement( bRes0 ) || Hash_IsComplement( bRes1 ) )
//                      bRes = Hash_Not( b1 );
                    if ( bRes0 == z0 || bRes1 == z0 )
                        bRes = z0;
                    else
                        bRes = b1;
                }
            }
        }
        // (4) F is on the level iLev2
        else if ( LevelF == iLev2 )
        {
            // this is the only place where the hash attribute (Hash_Not) can be added 
            // to the result; it can be added only if the path came through the node
            // lebeled with Var1; therefore, the hash attribute cannot be returned
            // to the caller function
            if ( fVar1Pres )
//              bRes = Hash_Not( b1 );
                bRes = z0;
            else 
                bRes = b0;
        }
        // (5) F is below iLev2
        else // if ( LevelF > iLev2 )
        {
            // it is possible that the path goes through the node labeled by Var1
            // and still everything is okay; we do not label with Hash_Not here
            // because the path does not go through node labeled by Var2
            bRes = b1;
        }

        cuddCacheInsert2(dd, extraBddCheckVarsSymmetric, bF, bVars, bRes);
        return bRes;
    }
} /* end of extraBddCheckVarsSymmetric */

/**Function********************************************************************

  Synopsis    [Performs the reordering-sensitive step of Extra_zddTupleFromBdd().]

  Description [Generates in a bottom-up fashion ZDD for all combinations
               composed of k variables out of variables belonging to Support.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode* extraZddTuplesFromBdd( 
  DdManager * dd,   /* the DD manager */
  DdNode * bVarsK,   /* the number of variables in tuples */
  DdNode * bVarsN)   /* the set of all variables */
{
    DdNode *zRes, *zRes0, *zRes1;
    statLine(dd); 

    /* terminal cases */
/*  if ( k < 0 || k > n )
 *      return dd->zero;
 *  if ( n == 0 )
 *      return dd->one; 
 */
    if ( cuddI( dd, bVarsK->index ) < cuddI( dd, bVarsN->index ) )
        return z0;
    if ( bVarsN == b1 )
        return z1;

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddTuplesFromBdd, bVarsK, bVarsN);
    if (zRes)
        return(zRes);

    /* ZDD in which this variable is 0 */
/*  zRes0 = extraZddTuplesFromBdd( dd, k,     n-1 ); */
    zRes0 = extraZddTuplesFromBdd( dd, bVarsK, cuddT(bVarsN) );
    if ( zRes0 == NULL ) 
        return NULL;
    cuddRef( zRes0 );

    /* ZDD in which this variable is 1 */
/*  zRes1 = extraZddTuplesFromBdd( dd, k-1,          n-1 ); */
    if ( bVarsK == b1 )
    {
        zRes1 = z0;
        cuddRef( zRes1 );
    }
    else
    {
        zRes1 = extraZddTuplesFromBdd( dd, cuddT(bVarsK), cuddT(bVarsN) );
        if ( zRes1 == NULL ) 
        {
            Cudd_RecursiveDerefZdd( dd, zRes0 );
            return NULL;
        }
        cuddRef( zRes1 );
    }

    /* compose Res0 and Res1 with the given ZDD variable */
    zRes = cuddZddGetNode( dd, 2*bVarsN->index, zRes1, zRes0 );
    if ( zRes == NULL ) 
    {
        Cudd_RecursiveDerefZdd( dd, zRes0 );
        Cudd_RecursiveDerefZdd( dd, zRes1 );
        return NULL;
    }
    cuddDeref( zRes0 );
    cuddDeref( zRes1 );

    /* insert the result into cache */
    cuddCacheInsert2(dd, extraZddTuplesFromBdd, bVarsK, bVarsN, zRes);
    return zRes;

} /* end of extraZddTuplesFromBdd */


/**Function********************************************************************

  Synopsis    [Performs the recursive step of Extra_zddSelectOneSubset.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode * extraZddSelectOneSubset( 
  DdManager * dd, 
  DdNode * zS )
// selects one subset from the ZDD zS
// returns z0 if and only if zS is an empty set of cubes
{
    DdNode * zRes;

    if ( zS == z0 )    return z0;
    if ( zS == z1 )    return z1;
    
    // check cache
    if ( (zRes = cuddCacheLookup1Zdd( dd, extraZddSelectOneSubset, zS )) )
        return zRes;
    else
    {
        DdNode * zS0, * zS1, * zTemp; 

        zS0 = cuddE(zS);
        zS1 = cuddT(zS);

        if ( zS0 != z0 )
        {
            zRes = extraZddSelectOneSubset( dd, zS0 );
            if ( zRes == NULL )
                return NULL;
        }
        else // if ( zS0 == z0 )
        {
            assert( zS1 != z0 );
            zRes = extraZddSelectOneSubset( dd, zS1 );
            if ( zRes == NULL )
                return NULL;
            cuddRef( zRes );

            zRes = cuddZddGetNode( dd, zS->index, zTemp = zRes, z0 );
            if ( zRes == NULL ) 
            {
                Cudd_RecursiveDerefZdd( dd, zTemp );
                return NULL;
            }
            cuddDeref( zTemp );
        }

        // insert the result into cache
        cuddCacheInsert1( dd, extraZddSelectOneSubset, zS, zRes );
        return zRes;
    }       
} /* end of extraZddSelectOneSubset */


/*---------------------------------------------------------------------------*/
/* Definition of static Functions                                            */
/*---------------------------------------------------------------------------*/
ABC_NAMESPACE_IMPL_END

