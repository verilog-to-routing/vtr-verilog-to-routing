/**CFile****************************************************************

  FileName    [rwrMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting package.]

  Synopsis    [Rewriting manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rwrMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "rwr.h"
#include "base/main/main.h"
#include "bool/dec/dec.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts rewriting manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Rwr_Man_t * Rwr_ManStart( int  fPrecompute )
{
    Dec_Man_t * pManDec;
    Rwr_Man_t * p;
    abctime clk = Abc_Clock();
clk = Abc_Clock();
    p = ABC_ALLOC( Rwr_Man_t, 1 );
    memset( p, 0, sizeof(Rwr_Man_t) );
    p->nFuncs = (1<<16);
    pManDec   = (Dec_Man_t *)Abc_FrameReadManDec();
    p->puCanons = pManDec->puCanons; 
    p->pPhases  = pManDec->pPhases; 
    p->pPerms   = pManDec->pPerms; 
    p->pMap     = pManDec->pMap; 
    // initialize practical NPN classes
    p->pPractical  = Rwr_ManGetPractical( p );
    // create the table
    p->pTable = ABC_ALLOC( Rwr_Node_t *, p->nFuncs );
    memset( p->pTable, 0, sizeof(Rwr_Node_t *) * p->nFuncs );
    // create the elementary nodes
    p->pMmNode  = Extra_MmFixedStart( sizeof(Rwr_Node_t) );
    p->vForest  = Vec_PtrAlloc( 100 );
    Rwr_ManAddVar( p, 0x0000, fPrecompute ); // constant 0
    Rwr_ManAddVar( p, 0xAAAA, fPrecompute ); // var A
    Rwr_ManAddVar( p, 0xCCCC, fPrecompute ); // var B
    Rwr_ManAddVar( p, 0xF0F0, fPrecompute ); // var C
    Rwr_ManAddVar( p, 0xFF00, fPrecompute ); // var D
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
        Rwr_ManPrecompute( p );
//        Rwr_ManPrint( p );
        Rwr_ManWriteToArray( p );
    }
    else
    {   // load saved subgraphs
        Rwr_ManLoadFromArray( p, 0 );
//        Rwr_ManPrint( p );
        Rwr_ManPreprocess( p );
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
void Rwr_ManStop( Rwr_Man_t * p )
{
    if ( p->vClasses )
    {
        Rwr_Node_t * pNode;
        int i, k;
        Vec_VecForEachEntry( Rwr_Node_t *, p->vClasses, pNode, i, k )
            Dec_GraphFree( (Dec_Graph_t *)pNode->pNext );
    }
    if ( p->vClasses )  Vec_VecFree( p->vClasses );
    Vec_PtrFree( p->vNodesTemp );
    Vec_PtrFree( p->vForest );
    Vec_IntFree( p->vLevNums );
    Vec_PtrFree( p->vFanins );
    Vec_PtrFree( p->vFaninsCur );
    Extra_MmFixedStop( p->pMmNode );
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
void Rwr_ManPrintStats( Rwr_Man_t * p )
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
    printf( "Gain              = %8d. (%6.2f %%).\n", p->nNodesBeg-p->nNodesEnd, 100.0*(p->nNodesBeg-p->nNodesEnd)/p->nNodesBeg );
    ABC_PRT( "Start       ", p->timeStart );
    ABC_PRT( "Cuts        ", p->timeCut );
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
*/
    printf( "\n" );

}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwr_ManPrintStatsFile( Rwr_Man_t * p )
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
void * Rwr_ManReadDecs( Rwr_Man_t * p )
{
    return p->pGraph;
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Rwr_ManReadLeaves( Rwr_Man_t * p )
{
    return p->vFanins;
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rwr_ManReadCompl( Rwr_Man_t * p )
{
    return p->fCompl;
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwr_ManAddTimeCuts( Rwr_Man_t * p, abctime Time )
{
    p->timeCut += Time;
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwr_ManAddTimeUpdate( Rwr_Man_t * p, abctime Time )
{
    p->timeUpdate += Time;
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwr_ManAddTimeTotal( Rwr_Man_t * p, abctime Time )
{
    p->timeTotal += Time;
}


/**Function*************************************************************

  Synopsis    [Precomputes AIG subgraphs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwr_Precompute()
{
    Rwr_Man_t * p;
    p = Rwr_ManStart( 1 );
    Rwr_ManStop( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

