/**CFile****************************************************************

  FileName    [retLvalue.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Retiming package.]

  Synopsis    [Implementation of Pan's retiming algorithm.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Oct 31, 2006.]

  Revision    [$Id: retLvalue.c,v 1.00 2006/10/31 00:00:00 alanmi Exp $]

***********************************************************************/

#include "retInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// node status after updating its arrival time
enum { ABC_RET_UPDATE_FAIL, ABC_RET_UPDATE_NO, ABC_RET_UPDATE_YES };

// the internal procedures
static Vec_Int_t * Abc_NtkRetimeGetLags( Abc_Ntk_t * pNtk, int nIterLimit, int fVerbose );
static int         Abc_NtkRetimeSearch_rec( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes, Vec_Ptr_t * vLatches, int FiMin, int FiMax, int nMaxIters, int fVerbose );
static int         Abc_NtkRetimeForPeriod( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes, Vec_Ptr_t * vLatches, int Fi, int nMaxIters, int fVerbose );
static int         Abc_NtkRetimeUpdateLValue( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes, Vec_Ptr_t * vLatches, int Fi );
static int         Abc_NtkRetimePosOverLimit( Abc_Ntk_t * pNtk, int Fi );
static Vec_Ptr_t * Abc_ManCollectLatches( Abc_Ntk_t * pNtk );
static int         Abc_NtkRetimeUsingLags( Abc_Ntk_t * pNtk, Vec_Int_t * vLags, int fVerbose );

static inline int  Abc_NodeComputeLag( int LValue, int Fi )          { return (LValue + (1<<16)*Fi)/Fi - (1<<16) - (int)(LValue % Fi == 0);     }
static inline int  Abc_NodeGetLValue( Abc_Obj_t * pNode )            { return (int)(ABC_PTRUINT_T)pNode->pCopy;        }
static inline void Abc_NodeSetLValue( Abc_Obj_t * pNode, int Value ) { pNode->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)Value;    }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Implements Pan's retiming algorithm.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRetimeLValue( Abc_Ntk_t * pNtk, int nIterLimit, int fVerbose )
{
    Vec_Int_t * vLags;
    int nLatches = Abc_NtkLatchNum(pNtk);
    assert( Abc_NtkIsLogic( pNtk ) );
    // get the lags
    vLags = Abc_NtkRetimeGetLags( pNtk, nIterLimit, fVerbose );
    // compute the retiming
//    Abc_NtkRetimeUsingLags( pNtk, vLags, fVerbose );
    Vec_IntFree( vLags );
    // fix the COs
//    Abc_NtkLogicMakeSimpleCos( pNtk, 0 );
    // check for correctness
    if ( !Abc_NtkCheck( pNtk ) )
        fprintf( stdout, "Abc_NtkRetimeLValue(): Network check has failed.\n" );
    // return the number of latches saved
    return nLatches - Abc_NtkLatchNum(pNtk);
}

/**Function*************************************************************

  Synopsis    [Computes the retiming lags.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkRetimeGetLags( Abc_Ntk_t * pNtk, int nIterLimit, int fVerbose )
{
    Vec_Int_t * vLags;
    Vec_Ptr_t * vNodes, * vLatches;
    Abc_Obj_t * pNode;
    int i, FiMax, FiBest, RetValue;
    abctime clk, clkIter;
    char NodeLag;

    // get the upper bound on the clock period
    FiMax = Abc_NtkLevel(pNtk);

    // make sure this clock period is feasible
    vNodes = Abc_NtkDfs( pNtk, 0 );
    vLatches = Abc_ManCollectLatches( pNtk );
    if ( !Abc_NtkRetimeForPeriod( pNtk, vNodes, vLatches, FiMax, nIterLimit, fVerbose ) )
    {
        Vec_PtrFree( vLatches );
        Vec_PtrFree( vNodes );
        printf( "Abc_NtkRetimeGetLags() error: The upper bound on the clock period cannot be computed.\n" );
        return Vec_IntStart( Abc_NtkObjNumMax(pNtk) + 1 );
    }
 
    // search for the optimal clock period between 0 and nLevelMax
clk = Abc_Clock();
    FiBest = Abc_NtkRetimeSearch_rec( pNtk, vNodes, vLatches, 0, FiMax, nIterLimit, fVerbose );
clkIter = Abc_Clock() - clk;

    // recompute the best l-values
    RetValue = Abc_NtkRetimeForPeriod( pNtk, vNodes, vLatches, FiBest, nIterLimit, fVerbose );
    assert( RetValue );

    // fix the problem with non-converged delays
    Abc_NtkForEachNode( pNtk, pNode, i )
        if ( Abc_NodeGetLValue(pNode) < -ABC_INFINITY/2 )
            Abc_NodeSetLValue( pNode, 0 );

    // write the retiming lags
    vLags = Vec_IntStart( Abc_NtkObjNumMax(pNtk) + 1 );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        NodeLag = Abc_NodeComputeLag( Abc_NodeGetLValue(pNode), FiBest );
        Vec_IntWriteEntry( vLags, pNode->Id, NodeLag );
    }
/*
    Abc_NtkForEachPo( pNtk, pNode, i )
        printf( "%d ", Abc_NodeGetLValue(Abc_ObjFanin0(pNode)) );
    printf( "\n" );
    Abc_NtkForEachLatch( pNtk, pNode, i )
        printf( "%d/%d ", Abc_NodeGetLValue(Abc_ObjFanout0(pNode)), Abc_NodeGetLValue(Abc_ObjFanout0(pNode)) + FiBest );
    printf( "\n" );
*/

    // print the result
//    if ( fVerbose )
    printf( "The best clock period is %3d. (Currently, network is not modified.)\n", FiBest );
/*
    {
        FILE * pTable;
        pTable = fopen( "iscas/seqmap__stats2.txt", "a+" );
        fprintf( pTable, "%d ", FiBest );
        fprintf( pTable, "\n" );
        fclose( pTable );
    }
*/
    Vec_PtrFree( vNodes );
    Vec_PtrFree( vLatches );
    return vLags;
}

/**Function*************************************************************

  Synopsis    [Performs binary search for the optimal clock period.]

  Description [Assumes that FiMin is infeasible while FiMax is feasible.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRetimeSearch_rec( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes, Vec_Ptr_t * vLatches, int FiMin, int FiMax, int nMaxIters, int fVerbose )
{
    int Median;
    assert( FiMin < FiMax );
    if ( FiMin + 1 == FiMax )
        return FiMax;
    Median = FiMin + (FiMax - FiMin)/2;
    if ( Abc_NtkRetimeForPeriod( pNtk, vNodes, vLatches, Median, nMaxIters, fVerbose ) )
        return Abc_NtkRetimeSearch_rec( pNtk, vNodes, vLatches, FiMin, Median, nMaxIters, fVerbose ); // Median is feasible
    else 
        return Abc_NtkRetimeSearch_rec( pNtk, vNodes, vLatches, Median, FiMax, nMaxIters, fVerbose ); // Median is infeasible
}

/**Function*************************************************************

  Synopsis    [Returns 1 if retiming with this clock period is feasible.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRetimeForPeriod( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes, Vec_Ptr_t * vLatches, int Fi, int nMaxIters, int fVerbose )
{
    Abc_Obj_t * pObj;
    int c, i, fConverged;
    // set l-values of all nodes to be minus infinity, except PIs and constants
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( Abc_ObjFaninNum(pObj) == 0 )
            Abc_NodeSetLValue( pObj, 0 );
        else
            Abc_NodeSetLValue( pObj, -ABC_INFINITY );
    // update all values iteratively
    fConverged = 0;
    for ( c = 1; c <= nMaxIters; c++ )
    {
        if ( !Abc_NtkRetimeUpdateLValue( pNtk, vNodes, vLatches, Fi ) )
        {
            fConverged = 1;
            break;
        }
        if ( Abc_NtkRetimePosOverLimit(pNtk, Fi) )
            break;
    }
    // report the results
    if ( fVerbose )
    {
        if ( !fConverged )
            printf( "Period = %3d.  Iterations = %3d.    Infeasible %s\n", Fi, c, (c > nMaxIters)? "(timeout)" : "" );
        else
            printf( "Period = %3d.  Iterations = %3d.      Feasible\n",    Fi, c );
    }
/*
    // check if any AND gates have infinite delay
    Counter = 0;
    Abc_NtkForEachNode( pNtk, pObj, i )
        Counter += (Abc_NodeGetLValue(pObj) < -ABC_INFINITY/2);
    if ( Counter > 0 )
        printf( "Warning: %d internal nodes have wrong l-values!\n", Counter );
*/
    return fConverged;
}

/**Function*************************************************************

  Synopsis    [Performs one iteration of l-value computation for the nodes.]

  Description [Experimentally it was found that checking POs changes
  is not enough to detect the convergence of l-values in the network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRetimeUpdateLValue( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes, Vec_Ptr_t * vLatches, int Fi )
{
    Abc_Obj_t * pObj, * pFanin;
    int i, k, lValueNew, fChange;
    // go through the nodes and detect change
    fChange = 0;
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        assert( Abc_ObjIsNode(pObj) );
        lValueNew = -ABC_INFINITY;
        Abc_ObjForEachFanin( pObj, pFanin, k )
        {
            if ( lValueNew < Abc_NodeGetLValue(pFanin) )
                lValueNew = Abc_NodeGetLValue(pFanin);
        }
        lValueNew++;
        if ( Abc_NodeGetLValue(pObj) < lValueNew )
        {
            Abc_NodeSetLValue( pObj, lValueNew );
            fChange = 1;
        }
    }
    // propagate values through the latches
    Vec_PtrForEachEntry( Abc_Obj_t *, vLatches, pObj, i )
        Abc_NodeSetLValue( Abc_ObjFanout0(pObj), Abc_NodeGetLValue(Abc_ObjFanin0(Abc_ObjFanin0(pObj))) - Fi );
    return fChange;
}

/**Function*************************************************************

  Synopsis    [Detects the case when l-values exceeded the limit.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRetimePosOverLimit( Abc_Ntk_t * pNtk, int Fi )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachPo( pNtk, pObj, i )
        if ( Abc_NodeGetLValue(Abc_ObjFanin0(pObj)) > Fi )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Collects latches in the topological order.]

  Description []
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
void Abc_ManCollectLatches_rec( Abc_Obj_t * pObj, Vec_Ptr_t * vLatches )
{
    Abc_Obj_t * pDriver;
    if ( !Abc_ObjIsLatch(pObj) )
        return;
    // skip already collected latches
    if ( Abc_NodeIsTravIdCurrent(pObj)  )
        return;
    Abc_NodeSetTravIdCurrent(pObj);
    // get the driver node feeding into the latch
    pDriver = Abc_ObjFanin0(Abc_ObjFanin0(pObj));
    // call recursively if the driver looks like a latch output
    if ( Abc_ObjIsBo(pDriver) )
        Abc_ManCollectLatches_rec( Abc_ObjFanin0(pDriver), vLatches );
    // collect the latch
    Vec_PtrPush( vLatches, pObj );
}

/**Function*************************************************************

  Synopsis    [Collects latches in the topological order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_ManCollectLatches( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vLatches;
    Abc_Obj_t * pObj;
    int i;
    vLatches = Vec_PtrAlloc( Abc_NtkLatchNum(pNtk) );
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachLatch( pNtk, pObj, i )
        Abc_ManCollectLatches_rec( pObj, vLatches );
    assert( Vec_PtrSize(vLatches) == Abc_NtkLatchNum(pNtk) );
    return vLatches;
}

/**Function*************************************************************

  Synopsis    [Implements the retiming given as the array of retiming lags.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRetimeUsingLags( Abc_Ntk_t * pNtk, Vec_Int_t * vLags, int fVerbose )
{
    Abc_Obj_t * pObj;
    int fChanges, fForward, nTotalMoves, Lag, Counter, i;
    // iterate over the nodes
    nTotalMoves = 0;
    do {
        fChanges = 0;
        Abc_NtkForEachNode( pNtk, pObj, i )
        {
            Lag = Vec_IntEntry( vLags, pObj->Id );
            if ( !Lag )
                continue;
            fForward = (Lag < 0);
            if ( Abc_NtkRetimeNodeIsEnabled( pObj, fForward ) )
            {
                Abc_NtkRetimeNode( pObj, fForward, 0 );
                fChanges = 1;
                nTotalMoves++;
                Vec_IntAddToEntry( vLags, pObj->Id, fForward? 1 : -1 );
            }
        }
    } while ( fChanges );
    if ( fVerbose )
        printf( "Total latch moves = %d.\n", nTotalMoves );
    // check if there are remaining lags
    Counter = 0;
    Abc_NtkForEachNode( pNtk, pObj, i )
        Counter += (Vec_IntEntry( vLags, pObj->Id ) != 0);
    if ( Counter )
        printf( "Warning! The number of nodes with unrealized lag = %d.\n", Counter );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

