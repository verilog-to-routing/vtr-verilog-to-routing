/**CFile****************************************************************

  FileName    [rwrExp.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting package.]

  Synopsis    [Computation of practically used NN-classes of 4-input cuts.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rwrExp.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "rwr.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Rwr_Man4_t_ Rwr_Man4_t;
struct Rwr_Man4_t_
{
    // internal lookups
    int                nFuncs;           // the number of four-var functions
    unsigned short *   puCanons;         // canonical forms
    int *              pnCounts;         // the counters of functions in each class
    int                nConsidered;      // the number of nodes considered
    int                nClasses;         // the number of NN classes
};

typedef struct Rwr_Man5_t_ Rwr_Man5_t;
struct Rwr_Man5_t_
{
    // internal lookups
    stmm_table *       tTableNN;         // the NN canonical forms
    stmm_table *       tTableNPN;        // the NPN canonical forms
};

static Rwr_Man4_t * s_pManRwrExp4 = NULL;
static Rwr_Man5_t * s_pManRwrExp5 = NULL;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Collects stats about 4-var functions appearing in netlists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwt_Man4ExploreStart()
{
    Rwr_Man4_t * p;
    p = ABC_ALLOC( Rwr_Man4_t, 1 );
    memset( p, 0, sizeof(Rwr_Man4_t) );
    // canonical forms
    p->nFuncs    = (1<<16);
    // canonical forms, phases, perms
    Extra_Truth4VarNPN( &p->puCanons, NULL, NULL, NULL );
    // counters
    p->pnCounts  = ABC_ALLOC( int, p->nFuncs );
    memset( p->pnCounts, 0, sizeof(int) * p->nFuncs );
    s_pManRwrExp4 = p;
}

/**Function*************************************************************

  Synopsis    [Collects stats about 4-var functions appearing in netlists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwt_Man4ExploreCount( unsigned uTruth )
{
    assert( uTruth < (1<<16) );
    s_pManRwrExp4->pnCounts[ s_pManRwrExp4->puCanons[uTruth] ]++;    
}

/**Function*************************************************************

  Synopsis    [Collects stats about 4-var functions appearing in netlists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwt_Man4ExplorePrint()
{
    FILE * pFile;
    int i, CountMax, CountWrite, nCuts, nClasses;
    int * pDistrib;
    int * pReprs;
    // find the max number of occurences
    nCuts = nClasses = 0;
    CountMax = 0;
    for ( i = 0; i < s_pManRwrExp4->nFuncs; i++ )
    {
        if ( CountMax < s_pManRwrExp4->pnCounts[i] )
            CountMax = s_pManRwrExp4->pnCounts[i];
        nCuts += s_pManRwrExp4->pnCounts[i];
        if ( s_pManRwrExp4->pnCounts[i] > 0 )
            nClasses++;
    }
    printf( "Number of cuts considered       = %8d.\n", nCuts );
    printf( "Classes occurring at least once = %8d.\n", nClasses );
    // print the distribution of classes
    pDistrib = ABC_ALLOC( int, CountMax + 1 );
    pReprs   = ABC_ALLOC( int, CountMax + 1 );
    memset( pDistrib, 0, sizeof(int)*(CountMax + 1) );
    for ( i = 0; i < s_pManRwrExp4->nFuncs; i++ )
    {
        pDistrib[ s_pManRwrExp4->pnCounts[i] ]++;
        pReprs[ s_pManRwrExp4->pnCounts[i] ] = i;
    }

    printf( "Occurence = %6d.  Num classes = %4d.  \n", 0, 2288-nClasses );
    for ( i = 1; i <= CountMax; i++ )
        if ( pDistrib[i] )
        {
            printf( "Occurence = %6d.  Num classes = %4d.  Repr = ", i, pDistrib[i] );
            Extra_PrintBinary( stdout, (unsigned*)&(pReprs[i]), 16 ); 
            printf( "\n" );
        }
    ABC_FREE( pDistrib );
    ABC_FREE( pReprs );
    // write into a file all classes above limit (5)
    CountWrite = 0;
    pFile = fopen( "npnclass_stats4.txt", "w" );
    for ( i = 0; i < s_pManRwrExp4->nFuncs; i++ )
        if ( s_pManRwrExp4->pnCounts[i] > 0 )
        {
            Extra_PrintHex( pFile, (unsigned *)&i, 4 );
            fprintf( pFile, " %10d\n", s_pManRwrExp4->pnCounts[i] );
//            fprintf( pFile, "%d ", i );
            CountWrite++;
        }
    fclose( pFile );
    printf( "%d classes written into file \"%s\".\n", CountWrite, "npnclass_stats4.txt" );
}




/**Function*************************************************************

  Synopsis    [Collects stats about 4-var functions appearing in netlists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwt_Man5ExploreStart()
{
    Rwr_Man5_t * p;
    p = ABC_ALLOC( Rwr_Man5_t, 1 );
    memset( p, 0, sizeof(Rwr_Man5_t) );
    p->tTableNN  = stmm_init_table( st__numcmp, st__numhash );
    p->tTableNPN = stmm_init_table( st__numcmp, st__numhash );
    s_pManRwrExp5 = p;

//Extra_PrintHex( stdout, Extra_TruthCanonNPN( 0x0000FFFF, 5 ), 5 );
//printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Collects stats about 4-var functions appearing in netlists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwt_Man5ExploreCount( unsigned uTruth )
{
    int * pCounter;
    if ( !stmm_find_or_add( s_pManRwrExp5->tTableNN, (char *)(ABC_PTRUINT_T)uTruth, (char***)&pCounter ) )
        *pCounter = 0;
    (*pCounter)++;
}

/**Function*************************************************************

  Synopsis    [Collects stats about 4-var functions appearing in netlists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwt_Man5ExplorePrint()
{
    FILE * pFile;
    stmm_generator * gen;
    int i, CountMax, nCuts, Counter;
    int * pDistrib;
    unsigned * pReprs;
    unsigned uTruth, uTruthC;
    abctime clk = Abc_Clock();
    Vec_Int_t * vClassesNN, * vClassesNPN;

    // find the max number of occurences
    nCuts = 0;
    CountMax = 0;
    stmm_foreach_item( s_pManRwrExp5->tTableNN, gen, (char **)&uTruth, (char **)&Counter )
    {
        nCuts += Counter;
        if ( CountMax < Counter )
            CountMax = Counter;
    }
    printf( "Number of cuts considered        = %8d.\n", nCuts );
    printf( "Classes occurring at least once  = %8d.\n", stmm_count(s_pManRwrExp5->tTableNN) );
    printf( "The largest number of occurrence = %8d.\n", CountMax );

    // print the distribution of classes
    pDistrib = ABC_ALLOC( int, CountMax + 1 );
    pReprs   = ABC_ALLOC( unsigned, CountMax + 1 );
    memset( pDistrib, 0, sizeof(int)*(CountMax + 1) );
    stmm_foreach_item( s_pManRwrExp5->tTableNN, gen, (char **)&uTruth, (char **)&Counter )
    {
        assert( Counter <= CountMax );
        pDistrib[ Counter ]++;
        pReprs[ Counter ] = uTruth;
    }

    for ( i = 1; i <= CountMax; i++ )
        if ( pDistrib[i] )
        {
            printf( "Occurence = %6d.  Num classes = %4d.  Repr = ", i, pDistrib[i] );
            Extra_PrintBinary( stdout, pReprs + i, 32 ); 
            printf( "\n" );
        }
    ABC_FREE( pDistrib );
    ABC_FREE( pReprs );


    // put them into an array
    vClassesNN = Vec_IntAlloc( stmm_count(s_pManRwrExp5->tTableNN) );
    stmm_foreach_item( s_pManRwrExp5->tTableNN, gen, (char **)&uTruth, NULL )
        Vec_IntPush( vClassesNN, (int)uTruth );
    Vec_IntSortUnsigned( vClassesNN );

    // write into a file all classes
    pFile = fopen( "nnclass_stats5.txt", "w" );
    Vec_IntForEachEntry( vClassesNN, uTruth, i )
    {
        if ( !stmm_lookup( s_pManRwrExp5->tTableNN, (char *)(ABC_PTRUINT_T)uTruth, (char **)&Counter ) )
        {
            assert( 0 );
        }
        Extra_PrintHex( pFile, &uTruth, 5 );
        fprintf( pFile, " %10d\n", Counter );
    }
    fclose( pFile );
    printf( "%d classes written into file \"%s\".\n", vClassesNN->nSize, "nnclass_stats5.txt" );


clk = Abc_Clock();
    // how many NPN classes exist?
    Vec_IntForEachEntry( vClassesNN, uTruth, i )
    {
        int * pCounter;
        uTruthC = Extra_TruthCanonNPN( uTruth, 5 );
        if ( !stmm_find_or_add( s_pManRwrExp5->tTableNPN, (char *)(ABC_PTRUINT_T)uTruthC, (char***)&pCounter ) )
            *pCounter = 0;
        if ( !stmm_lookup( s_pManRwrExp5->tTableNN, (char *)(ABC_PTRUINT_T)uTruth, (char **)&Counter ) )
        {
            assert( 0 );
        }
        (*pCounter) += Counter;
    }
    printf( "The numbe of NPN classes = %d.\n", stmm_count(s_pManRwrExp5->tTableNPN) );
ABC_PRT( "Computing NPN classes", Abc_Clock() - clk );

    // put them into an array
    vClassesNPN = Vec_IntAlloc( stmm_count(s_pManRwrExp5->tTableNPN) );
    stmm_foreach_item( s_pManRwrExp5->tTableNPN, gen, (char **)&uTruth, NULL )
        Vec_IntPush( vClassesNPN, (int)uTruth );
    Vec_IntSortUnsigned( vClassesNPN );

    // write into a file all classes
    pFile = fopen( "npnclass_stats5.txt", "w" );
    Vec_IntForEachEntry( vClassesNPN, uTruth, i )
    {
        if ( !stmm_lookup( s_pManRwrExp5->tTableNPN, (char *)(ABC_PTRUINT_T)uTruth, (char **)&Counter ) )
        {
            assert( 0 );
        }
        Extra_PrintHex( pFile, &uTruth, 5 );
        fprintf( pFile, " %10d\n", Counter );
    }
    fclose( pFile );
    printf( "%d classes written into file \"%s\".\n", vClassesNPN->nSize, "npnclass_stats5.txt" );


    // can they be uniquely characterized?

}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

