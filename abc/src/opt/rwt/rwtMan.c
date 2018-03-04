/**CFile****************************************************************

  FileName    [rwtMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting package.]

  Synopsis    [Rewriting manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rwtMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "rwt.h"
#include "bool/deco/deco.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static unsigned short * s_puCanons = NULL; 
static char *           s_pPhases = NULL; 
static char *           s_pPerms = NULL; 
static unsigned char *  s_pMap = NULL;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts residual rewriting manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwt_ManGlobalStart()
{ 
    if ( s_puCanons == NULL )
        Extra_Truth4VarNPN( &s_puCanons, &s_pPhases, &s_pPerms, &s_pMap );
}

/**Function*************************************************************

  Synopsis    [Starts residual rewriting manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwt_ManGlobalStop()
{ 
    ABC_FREE( s_puCanons );
    ABC_FREE( s_pPhases );
    ABC_FREE( s_pPerms );
    ABC_FREE( s_pMap );
} 

/**Function*************************************************************

  Synopsis    [Starts rewriting manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Rwt_Man_t * Rwt_ManStart( int fPrecompute )
{
    Rwt_Man_t * p;
    abctime clk = Abc_Clock();
clk = Abc_Clock();
    p = ABC_ALLOC( Rwt_Man_t, 1 );
    memset( p, 0, sizeof(Rwt_Man_t) );
    p->nFuncs = (1<<16);
    // copy the global tables
    Rwt_ManGlobalStart();
    p->puCanons = s_puCanons; 
    p->pPhases  = s_pPhases; 
    p->pPerms   = s_pPerms; 
    p->pMap     = s_pMap; 
    // initialize practical NPN classes
    p->pPractical  = Rwt_ManGetPractical( p );
    // create the table
    p->pTable = ABC_ALLOC( Rwt_Node_t *, p->nFuncs );
    memset( p->pTable, 0, sizeof(Rwt_Node_t *) * p->nFuncs );
    // create the elementary nodes
    p->pMmNode  = Mem_FixedStart( sizeof(Rwt_Node_t) );
    p->vForest  = Vec_PtrAlloc( 100 );
    Rwt_ManAddVar( p, 0x0000, fPrecompute ); // constant 0
    Rwt_ManAddVar( p, 0xAAAA, fPrecompute ); // var A
    Rwt_ManAddVar( p, 0xCCCC, fPrecompute ); // var B
    Rwt_ManAddVar( p, 0xF0F0, fPrecompute ); // var C
    Rwt_ManAddVar( p, 0xFF00, fPrecompute ); // var D
    p->nClasses = 5;
    // other stuff
    p->nTravIds   = 1;
    p->pPerms4    = Extra_Permutations( 4 );
    p->vLevNums   = Vec_IntAlloc( 50 );
    p->vFanins    = Vec_PtrAlloc( 50 );
    p->vFaninsCur = Vec_PtrAlloc( 50 );
    p->vNodesTemp = Vec_PtrAlloc( 50 );
    if ( fPrecompute )
    {   // precompute subgraphs
//        Rwt_ManPrecompute( p );
//        Rwt_ManPrint( p );
//        Rwt_ManWriteToArray( p );
    }
    else
    {   // load saved subgraphs
        Rwt_ManLoadFromArray( p, 0 );
//        Rwt_ManPrint( p );
        Rwt_ManPreprocess( p );
    }
p->timeStart = Abc_Clock() - clk;
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops rewriting manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwt_ManStop( Rwt_Man_t * p )
{
    if ( p->vClasses )
    {
        Rwt_Node_t * pNode;
        int i, k;
        Vec_VecForEachEntry( Rwt_Node_t *, p->vClasses, pNode, i, k )
            Dec_GraphFree( (Dec_Graph_t *)pNode->pNext );
    }
    if ( p->vClasses )  Vec_VecFree( p->vClasses );
    Vec_PtrFree( p->vNodesTemp );
    Vec_PtrFree( p->vForest );
    Vec_IntFree( p->vLevNums );
    Vec_PtrFree( p->vFanins );
    Vec_PtrFree( p->vFaninsCur );
    Mem_FixedStop( p->pMmNode, 0 );
    ABC_FREE( p->pMapInv );
    ABC_FREE( p->pTable );
    ABC_FREE( p->pPractical );
    ABC_FREE( p->pPerms4 );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwt_ManPrintStats( Rwt_Man_t * p )
{
    int i, Counter = 0;
    for ( i = 0; i < 222; i++ )
        Counter += (p->nScores[i] > 0);

    printf( "Rewriting statistics:\n" );
    printf( "Total cuts tries  = %8d.\n", p->nCutsGood );
    printf( "Bad cuts found    = %8d.\n", p->nCutsBad );
    printf( "Total subgraphs   = %8d.\n", p->nSubgraphs );
    printf( "Used NPN classes  = %8d.\n", Counter );
    printf( "Nodes considered  = %8d.\n", p->nNodesConsidered );
    printf( "Nodes rewritten   = %8d.\n", p->nNodesRewritten );
    printf( "Calculated gain   = %8d.\n", p->nNodesGained     );
    ABC_PRT( "Start       ", p->timeStart );
    ABC_PRT( "Cuts        ", p->timeCut );
    ABC_PRT( "Truth       ", p->timeTruth );
    ABC_PRT( "Resynthesis ", p->timeRes );
    ABC_PRT( "    Mffc    ", p->timeMffc );
    ABC_PRT( "    Eval    ", p->timeEval );
    ABC_PRT( "Update      ", p->timeUpdate );
    ABC_PRT( "TOTAL       ", p->timeTotal );

/*
    printf( "The scores are:\n" );
    for ( i = 0; i < 222; i++ )
        if ( p->nScores[i] > 0 )
        {
            extern void Ivy_TruthDsdComputePrint( unsigned uTruth );
            printf( "%3d = %8d  canon = %5d  ", i, p->nScores[i], p->pMapInv[i] );
            Ivy_TruthDsdComputePrint( (unsigned)p->pMapInv[i] | ((unsigned)p->pMapInv[i] << 16) );
        }
    printf( "\n" );
*/
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwt_ManPrintStatsFile( Rwt_Man_t * p )
{
    FILE * pTable;
    pTable = fopen( "stats.txt", "a+" );
    fprintf( pTable, "%d ", p->nCutsGood );
    fprintf( pTable, "%d ", p->nSubgraphs );
    fprintf( pTable, "%d ", p->nNodesRewritten );
    fprintf( pTable, "%d", p->nNodesGained );
    fprintf( pTable, "\n" );
    fclose( pTable );
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Rwt_ManReadDecs( Rwt_Man_t * p )
{
    return p->pGraph;
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Rwt_ManReadLeaves( Rwt_Man_t * p )
{
    return p->vFanins;
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rwt_ManReadCompl( Rwt_Man_t * p )
{
    return p->fCompl;
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwt_ManAddTimeCuts( Rwt_Man_t * p, abctime Time )
{
    p->timeCut += Time;
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwt_ManAddTimeUpdate( Rwt_Man_t * p, abctime Time )
{
    p->timeUpdate += Time;
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwt_ManAddTimeTotal( Rwt_Man_t * p, abctime Time )
{
    p->timeTotal += Time;
}


/**Function*************************************************************

  Synopsis    [Precomputes AIG subgraphs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwt_Precompute()
{
    Rwt_Man_t * p;
    p = Rwt_ManStart( 1 );
    Rwt_ManStop( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

