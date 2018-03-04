/**CFile****************************************************************

  FileName    [abcMv.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Multi-valued decomposition.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcMv.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#ifdef ABC_USE_CUDD

typedef struct Mv_Man_t_ Mv_Man_t;
struct Mv_Man_t_
{
    int         nInputs;          // the number of 4-valued input variables
    int         nFuncs;           // the number of 4-valued functions
    DdManager * dd;               // representation of functions
    DdNode *    bValues[15][4];   // representation of i-sets
    DdNode *    bValueDcs[15][4]; // representation of i-sets don't-cares
    DdNode *    bFuncs[15];       // representation of functions
};

static void     Abc_MvDecompose( Mv_Man_t * p );
static void     Abc_MvPrintStats( Mv_Man_t * p );
static void     Abc_MvRead( Mv_Man_t * p );
static void     Abc_MvDeref( Mv_Man_t * p );
static DdNode * Abc_MvReadCube( DdManager * dd, char * pLine, int nVars );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MvExperiment()
{
    Mv_Man_t * p;
    // get the functions
    p = ABC_ALLOC( Mv_Man_t, 1 );
    memset( p, 0, sizeof(Mv_Man_t) );
    p->dd = Cudd_Init( 32, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    p->nFuncs  = 15;
    p->nInputs =  9;
    Abc_MvRead( p );
    // process the functions
    Abc_MvPrintStats( p );
//    Cudd_ReduceHeap( p->dd, CUDD_REORDER_SYMM_SIFT, 1 );
//    Abc_MvPrintStats( p );
    // try detecting support reducing bound set
    Abc_MvDecompose( p );

    // remove the manager
    Abc_MvDeref( p );
    Extra_StopManager( p->dd );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MvPrintStats( Mv_Man_t * p )
{
    int i, v;
    for ( i = 0; i < 15; i++ )
    {
        printf( "%2d : ", i );
        printf( "%3d (%2d)    ", Cudd_DagSize(p->bFuncs[i])-1, Cudd_SupportSize(p->dd, p->bFuncs[i]) );
        for ( v = 0; v < 4; v++ )
            printf( "%d = %3d (%2d)  ", v, Cudd_DagSize(p->bValues[i][v])-1, Cudd_SupportSize(p->dd, p->bValues[i][v]) );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_MvReadCube( DdManager * dd, char * pLine, int nVars )
{
    DdNode * bCube, * bVar, * bTemp;
    int i;
    bCube = Cudd_ReadOne(dd);  Cudd_Ref( bCube );
    for ( i = 0; i < nVars; i++ )
    {
        if ( pLine[i] == '-' )
            continue;
        else if ( pLine[i] == '0' ) // 0
            bVar = Cudd_Not( Cudd_bddIthVar(dd, 29-i) );
        else if ( pLine[i] == '1' ) // 1
            bVar = Cudd_bddIthVar(dd, 29-i);
        else assert(0);
        bCube = Cudd_bddAnd( dd, bTemp = bCube, bVar );  Cudd_Ref( bCube );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bCube );
    return bCube;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MvRead( Mv_Man_t * p )
{
    FILE * pFile;
    char Buffer[1000], * pLine;
    DdNode * bCube, * bTemp, * bProd, * bVar0, * bVar1, * bCubeSum;
    int i, v;

    // start the cube
    bCubeSum = Cudd_ReadLogicZero(p->dd);  Cudd_Ref( bCubeSum );

    // start the values
    for ( i = 0; i < 15; i++ )
    for ( v = 0; v < 4; v++ )
    {
        p->bValues[i][v]   = Cudd_ReadLogicZero(p->dd);  Cudd_Ref( p->bValues[i][v] );
        p->bValueDcs[i][v] = Cudd_ReadLogicZero(p->dd);  Cudd_Ref( p->bValueDcs[i][v] );
    }

    // read the file
    pFile = fopen( "input.pla", "r" );
    while ( fgets( Buffer, 1000, pFile ) )
    {
        if ( Buffer[0] == '#' )
            continue;
        if ( Buffer[0] == '.' )
        {
            if ( Buffer[1] == 'e' )
                break;
            continue;
        }

        // get the cube 
        bCube = Abc_MvReadCube( p->dd, Buffer, 18 );  Cudd_Ref( bCube );

        // add it to the values of the output functions
        pLine = Buffer + 19;
        for ( i = 0; i < 15; i++ )
        {
            if ( pLine[2*i] == '-' && pLine[2*i+1] == '-' )
            {
                for ( v = 0; v < 4; v++ )
                {
                    p->bValueDcs[i][v] = Cudd_bddOr( p->dd, bTemp = p->bValueDcs[i][v], bCube );  Cudd_Ref( p->bValueDcs[i][v] );
                    Cudd_RecursiveDeref( p->dd, bTemp );
                }
                continue;
            }
            else if ( pLine[2*i] == '0' && pLine[2*i+1] == '0' ) // 0
                v = 0;
            else if ( pLine[2*i] == '1' && pLine[2*i+1] == '0' ) // 1
                v = 1;
            else if ( pLine[2*i] == '0' && pLine[2*i+1] == '1' ) // 2
                v = 2;
            else if ( pLine[2*i] == '1' && pLine[2*i+1] == '1' ) // 3
                v = 3;
            else assert( 0 );
            // add the value
            p->bValues[i][v] = Cudd_bddOr( p->dd, bTemp = p->bValues[i][v], bCube );  Cudd_Ref( p->bValues[i][v] );
            Cudd_RecursiveDeref( p->dd, bTemp );
        }

        // add the cube
        bCubeSum = Cudd_bddOr( p->dd, bTemp = bCubeSum, bCube );  Cudd_Ref( bCubeSum );
        Cudd_RecursiveDeref( p->dd, bTemp );
        Cudd_RecursiveDeref( p->dd, bCube );
    }

    // add the complement of the domain to all values
    for ( i = 0; i < 15; i++ )
        for ( v = 0; v < 4; v++ )
        {
            if ( p->bValues[i][v] == Cudd_Not(Cudd_ReadOne(p->dd)) )
                continue;
            p->bValues[i][v] = Cudd_bddOr( p->dd, bTemp = p->bValues[i][v], p->bValueDcs[i][v] );  Cudd_Ref( p->bValues[i][v] );
            Cudd_RecursiveDeref( p->dd, bTemp );
            p->bValues[i][v] = Cudd_bddOr( p->dd, bTemp = p->bValues[i][v], Cudd_Not(bCubeSum) );  Cudd_Ref( p->bValues[i][v] );
            Cudd_RecursiveDeref( p->dd, bTemp );
        }
    printf( "Domain = %5.2f %%.\n", 100.0*Cudd_CountMinterm(p->dd, bCubeSum, 32)/Cudd_CountMinterm(p->dd, Cudd_ReadOne(p->dd), 32) ); 
    Cudd_RecursiveDeref( p->dd, bCubeSum );

    // create each output function
    for ( i = 0; i < 15; i++ )
    {
        p->bFuncs[i] = Cudd_ReadLogicZero(p->dd);  Cudd_Ref( p->bFuncs[i] );
        for ( v = 0; v < 4; v++ )
        {
            bVar0 = Cudd_NotCond( Cudd_bddIthVar(p->dd, 30), ((v & 1) == 0) );
            bVar1 = Cudd_NotCond( Cudd_bddIthVar(p->dd, 31), ((v & 2) == 0) );
            bCube = Cudd_bddAnd( p->dd, bVar0, bVar1 );             Cudd_Ref( bCube );
            bProd = Cudd_bddAnd( p->dd, p->bValues[i][v], bCube );  Cudd_Ref( bProd );
            Cudd_RecursiveDeref( p->dd, bCube );
            // add the value
            p->bFuncs[i] = Cudd_bddOr( p->dd, bTemp = p->bFuncs[i], bProd );  Cudd_Ref( p->bFuncs[i] );
            Cudd_RecursiveDeref( p->dd, bTemp );
            Cudd_RecursiveDeref( p->dd, bProd );
        }
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MvDeref( Mv_Man_t * p )
{
    int i, v;
    for ( i = 0; i < 15; i++ )
    for ( v = 0; v < 4; v++ )
    {
        Cudd_RecursiveDeref( p->dd, p->bValues[i][v] );
        Cudd_RecursiveDeref( p->dd, p->bValueDcs[i][v] );
    }
    for ( i = 0; i < 15; i++ )
        Cudd_RecursiveDeref( p->dd, p->bFuncs[i] );
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MvDecompose( Mv_Man_t * p )
{
    DdNode * bCofs[16], * bVarCube1, * bVarCube2, * bVarCube, * bCube, * bVar0, * bVar1;//, * bRes;
    int k, i1, i2, v1, v2;//, c1, c2, Counter;

    bVar0 = Cudd_bddIthVar(p->dd, 30);
    bVar1 = Cudd_bddIthVar(p->dd, 31);
    bCube = Cudd_bddAnd( p->dd, bVar0, bVar1 );  Cudd_Ref( bCube );

    for ( k = 0; k < p->nFuncs; k++ )
    {
        printf( "FUNCTION %d\n", k );
        for ( i1 = 0; i1 < p->nFuncs; i1++ )
        for ( i2 = i1+1; i2 < p->nFuncs; i2++ )
        {
            Vec_Ptr_t * vCofs;

            for ( v1 = 0; v1 < 4; v1++ )
            {
                bVar0 = Cudd_NotCond( Cudd_bddIthVar(p->dd, 29-2*i1  ), ((v1 & 1) == 0) );
                bVar1 = Cudd_NotCond( Cudd_bddIthVar(p->dd, 29-2*i1-1), ((v1 & 2) == 0) );
                bVarCube1 = Cudd_bddAnd( p->dd, bVar0, bVar1 );  Cudd_Ref( bVarCube1 );
                for ( v2 = 0; v2 < 4; v2++ )
                {
                    bVar0 = Cudd_NotCond( Cudd_bddIthVar(p->dd, 29-2*i2  ), ((v2 & 1) == 0) );
                    bVar1 = Cudd_NotCond( Cudd_bddIthVar(p->dd, 29-2*i2-1), ((v2 & 2) == 0) );
                    bVarCube2 = Cudd_bddAnd( p->dd, bVar0, bVar1 );         Cudd_Ref( bVarCube2 );
                    bVarCube = Cudd_bddAnd( p->dd, bVarCube1, bVarCube2 );  Cudd_Ref( bVarCube );
                    bCofs[v1 * 4 + v2] = Cudd_Cofactor( p->dd, p->bFuncs[k], bVarCube );  Cudd_Ref( bCofs[v1 * 4 + v2] );
                    Cudd_RecursiveDeref( p->dd, bVarCube );
                    Cudd_RecursiveDeref( p->dd, bVarCube2 );
                }
                Cudd_RecursiveDeref( p->dd, bVarCube1 );
            }
/*
            // check the compatibility of cofactors
            Counter = 0;
            for ( c1 = 0; c1 < 16; c1++ )
            {
                for ( c2 = 0; c2 <= c1; c2++ )
                    printf( " " );
                for ( c2 = c1+1; c2 < 16; c2++ )
                {
                    bRes = Cudd_bddAndAbstract( p->dd, bCofs[c1], bCofs[c2], bCube );  Cudd_Ref( bRes );
                    if ( bRes == Cudd_ReadOne(p->dd) )
                    {
                        printf( "+" );
                        Counter++;
                    }
                    else
                    {
                        printf( " " );
                    }
                    Cudd_RecursiveDeref( p->dd, bRes );
                }
                printf( "\n" );
            }
*/

            vCofs = Vec_PtrAlloc( 16 );
            for ( v1 = 0; v1 < 4; v1++ )
            for ( v2 = 0; v2 < 4; v2++ )
                Vec_PtrPushUnique( vCofs, bCofs[v1 * 4 + v2] );
            printf( "%d ", Vec_PtrSize(vCofs) );
            Vec_PtrFree( vCofs );

            // free the cofactors
            for ( v1 = 0; v1 < 4; v1++ )
            for ( v2 = 0; v2 < 4; v2++ )
                Cudd_RecursiveDeref( p->dd, bCofs[v1 * 4 + v2] );

            printf( "\n" );
//            printf( "%2d, %2d : %3d\n", i1, i2, Counter );
        }
    }

    Cudd_RecursiveDeref( p->dd, bCube );
}
#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

