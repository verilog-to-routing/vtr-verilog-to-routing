/**CFile****************************************************************

  FileName    [seqRetIter.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Construction and manipulation of sequential AIGs.]

  Synopsis    [Iterative delay computation in FPGA mapping/retiming package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: seqRetIter.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "seqInt.h"
#include "main.h"
#include "fpga.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static float Seq_NtkMappingSearch_rec( Abc_Ntk_t * pNtk, float FiMin, float FiMax, float Delta, int fVerbose );
static int   Seq_NtkMappingForPeriod( Abc_Ntk_t * pNtk, float Fi, int fVerbose );
static int   Seq_NtkNodeUpdateLValue( Abc_Obj_t * pObj, float Fi, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vDelays );
static void  Seq_NodeRetimeSetLag_rec( Abc_Obj_t * pNode, char Lag );

static void  Seq_NodePrintInfo( Abc_Obj_t * pNode );
static void  Seq_NodePrintInfoPlus( Abc_Obj_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the retiming lags for arbitrary network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_NtkRetimeDelayLags( Abc_Ntk_t * pNtkOld, Abc_Ntk_t * pNtk, int fVerbose )
{
    Abc_Seq_t * p = pNtk->pManFunc;
    Abc_Obj_t * pNode;
    float FiMax, Delta;
    int i, RetValue;
    char NodeLag;

    assert( Abc_NtkIsSeq( pNtk ) );

    // the root AND gates and node delay should be assigned
    assert( p->vMapAnds );
    assert( p->vMapCuts );
    assert( p->vMapDelays );
    assert( p->vMapFanins );

    // guess the upper bound on the clock period
    if ( Abc_NtkHasMapping(pNtkOld) )
    {
        // assign the accuracy for min-period computation
        Delta = Mio_LibraryReadDelayNand2Max(Abc_FrameReadLibGen());
        if ( Delta == 0.0 )
        {
            Delta = Mio_LibraryReadDelayAnd2Max(Abc_FrameReadLibGen());
            if ( Delta == 0.0 )
            {
                printf( "Cannot retime/map if the library does not have NAND2 or AND2.\n" );
                return 0;
            }
        }
        // get the upper bound on the clock period
        FiMax = Delta * 2 + Abc_NtkDelayTrace(pNtkOld);
        Delta /= 2;
    }
    else
    {
        FiMax = (float)2.0 + Abc_NtkGetLevelNum(pNtkOld);
        Delta = 1;
    }

    // make sure this clock period is feasible
    if ( !Seq_NtkMappingForPeriod( pNtk, FiMax, fVerbose ) )
    {
        printf( "Error: The upper bound on the clock period cannot be computed.\n" );
        printf( "The reason for this error may be the presence in the circuit of logic\n" );
        printf( "that is not reachable from the PIs. Mapping/retiming is not performed.\n" );
        return 0;
    }
 
    // search for the optimal clock period between 0 and nLevelMax
    p->FiBestFloat = Seq_NtkMappingSearch_rec( pNtk, 0.0, FiMax, Delta, fVerbose );

    // recompute the best l-values
    RetValue = Seq_NtkMappingForPeriod( pNtk, p->FiBestFloat, fVerbose );
    assert( RetValue );

    // fix the problem with non-converged delays
    Vec_PtrForEachEntry( p->vMapAnds, pNode, i )
        if ( Seq_NodeGetLValueP(pNode) < -ABC_INFINITY/2 )
            Seq_NodeSetLValueP( pNode, 0 );

    // experiment by adding an epsilon to all LValues
//    Vec_PtrForEachEntry( p->vMapAnds, pNode, i )
//        Seq_NodeSetLValueP( pNode, Seq_NodeGetLValueP(pNode) - p->fEpsilon );

    // save the retiming lags
    // mark the nodes
    Vec_PtrForEachEntry( p->vMapAnds, pNode, i )
        pNode->fMarkA = 1;
    // process the nodes
    Vec_StrFill( p->vLags,  p->nSize, 0 );
    Vec_PtrForEachEntry( p->vMapAnds, pNode, i )
    {
        if ( Vec_PtrSize( Vec_VecEntry(p->vMapCuts, i) ) == 0 )
        {
            Seq_NodeSetLag( pNode, 0 );
            continue;
        }
        NodeLag = Seq_NodeComputeLagFloat( Seq_NodeGetLValueP(pNode), p->FiBestFloat );
        Seq_NodeRetimeSetLag_rec( pNode, NodeLag );
    }
    // unmark the nodes
    Vec_PtrForEachEntry( p->vMapAnds, pNode, i )
        pNode->fMarkA = 0;

    // print the result
    if ( fVerbose )
        printf( "The best clock period is %6.2f.\n", p->FiBestFloat );
/*
    {
        FILE * pTable;
        pTable = fopen( "stats.txt", "a+" );
        fprintf( pTable, "%s ",  pNtk->pName );
        fprintf( pTable, "%.2f ", FiBest );
        fprintf( pTable, "\n" );
        fclose( pTable );
    }
*/
//    Seq_NodePrintInfo( Abc_NtkObj(pNtk, 847) );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Performs binary search for the optimal clock period.]

  Description [Assumes that FiMin is infeasible while FiMax is feasible.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Seq_NtkMappingSearch_rec( Abc_Ntk_t * pNtk, float FiMin, float FiMax, float Delta, int fVerbose )
{
    float Median;
    assert( FiMin < FiMax );
    if ( FiMin + Delta >= FiMax )
        return FiMax;
    Median = FiMin + (FiMax - FiMin)/2;
    if ( Seq_NtkMappingForPeriod( pNtk, Median, fVerbose ) )
        return Seq_NtkMappingSearch_rec( pNtk, FiMin, Median, Delta, fVerbose ); // Median is feasible
    else 
        return Seq_NtkMappingSearch_rec( pNtk, Median, FiMax, Delta, fVerbose ); // Median is infeasible
}

/**Function*************************************************************

  Synopsis    [Returns 1 if retiming with this clock period is feasible.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_NtkMappingForPeriod( Abc_Ntk_t * pNtk, float Fi, int fVerbose )
{
    Abc_Seq_t * p = pNtk->pManFunc;
    Vec_Ptr_t * vLeaves, * vDelays;
    Abc_Obj_t * pObj;
    int i, c, RetValue, fChange, Counter;
    char * pReason = "";

    // set l-values of all nodes to be minus infinity
    Vec_IntFill( p->vLValues,  p->nSize, Abc_Float2Int( (float)-ABC_INFINITY ) );

    // set l-values of constants and PIs
    pObj = Abc_NtkObj( pNtk, 0 );
    Seq_NodeSetLValueP( pObj, 0.0 );
    Abc_NtkForEachPi( pNtk, pObj, i )
        Seq_NodeSetLValueP( pObj, 0.0 );

    // update all values iteratively
    Counter = 0;
    for ( c = 0; c < p->nMaxIters; c++ )
    {
        fChange = 0;
        Vec_PtrForEachEntry( p->vMapAnds, pObj, i )
        {
            Counter++;
            vLeaves = Vec_VecEntry( p->vMapCuts, i );
            vDelays = Vec_VecEntry( p->vMapDelays, i );
            if ( Vec_PtrSize(vLeaves) == 0 )
            {
                Seq_NodeSetLValueP( pObj, 0.0 );
                continue;
            }
            RetValue = Seq_NtkNodeUpdateLValue( pObj, Fi, vLeaves, vDelays );
            if ( RetValue == SEQ_UPDATE_YES ) 
                fChange = 1;
        }
        Abc_NtkForEachPo( pNtk, pObj, i )
        {
            RetValue = Seq_NtkNodeUpdateLValue( pObj, Fi, NULL, NULL );
            if ( RetValue == SEQ_UPDATE_FAIL )
                break;
        }
        if ( RetValue == SEQ_UPDATE_FAIL )
            break;
        if ( fChange == 0 )
            break;
    }
    if ( c == p->nMaxIters )
    {
        RetValue = SEQ_UPDATE_FAIL;
        pReason = "(timeout)";
    }
    else
        c++;

    // report the results
    if ( fVerbose )
    {
        if ( RetValue == SEQ_UPDATE_FAIL )
            printf( "Period = %6.2f.  Iterations = %3d.  Updates = %10d.    Infeasible %s\n", Fi, c, Counter, pReason );
        else
            printf( "Period = %6.2f.  Iterations = %3d.  Updates = %10d.      Feasible\n",    Fi, c, Counter );
    }
    return RetValue != SEQ_UPDATE_FAIL;
}

/**Function*************************************************************

  Synopsis    [Computes the l-value of the node.]

  Description [The node can be internal or a PO.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_NtkNodeUpdateLValue( Abc_Obj_t * pObj, float Fi, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vDelays )
{
    Abc_Seq_t * p = pObj->pNtk->pManFunc;
    float lValueOld, lValueNew, lValueCur, lValuePin;
    unsigned SeqEdge;
    Abc_Obj_t * pLeaf;
    int i;

    assert( !Abc_ObjIsPi(pObj) );
    assert( Abc_ObjFaninNum(pObj) > 0 );
    // consider the case of the PO
    if ( Abc_ObjIsPo(pObj) )
    {
        lValueCur = Seq_NodeGetLValueP(Abc_ObjFanin0(pObj)) - Fi * Seq_ObjFaninL0(pObj);
        return (lValueCur > Fi + p->fEpsilon)? SEQ_UPDATE_FAIL : SEQ_UPDATE_NO;
    }
    // get the new arrival time of the cut output
    lValueNew = -ABC_INFINITY;
    Vec_PtrForEachEntry( vLeaves, pLeaf, i )
    {
        SeqEdge   = (unsigned)pLeaf;
        pLeaf     = Abc_NtkObj( pObj->pNtk, SeqEdge >> 8 );
        lValueCur = Seq_NodeGetLValueP(pLeaf) - Fi * (SeqEdge & 255);
        lValuePin = Abc_Int2Float( (int)Vec_PtrEntry(vDelays, i) );
        if ( lValueNew < lValuePin + lValueCur )
            lValueNew = lValuePin + lValueCur;
    }
    // compare 
    lValueOld = Seq_NodeGetLValueP( pObj );
    if ( lValueNew <= lValueOld + p->fEpsilon )
        return SEQ_UPDATE_NO;
    // update the values
    if ( lValueNew > lValueOld + p->fEpsilon )
        Seq_NodeSetLValueP( pObj, lValueNew );
    return SEQ_UPDATE_YES;
}



/**Function*************************************************************

  Synopsis    [Add sequential edges.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NodeRetimeSetLag_rec( Abc_Obj_t * pNode, char Lag )
{
    Abc_Obj_t * pFanin;
    if ( !Abc_AigNodeIsAnd(pNode) )
        return;
    Seq_NodeSetLag( pNode, Lag );
    // consider the first fanin
    pFanin = Abc_ObjFanin0(pNode);
    if ( pFanin->fMarkA == 0 ) // internal node
        Seq_NodeRetimeSetLag_rec( pFanin, Lag );
    // consider the second fanin
    pFanin = Abc_ObjFanin1(pNode);
    if ( pFanin->fMarkA == 0 ) // internal node
        Seq_NodeRetimeSetLag_rec( pFanin, Lag );
}


/**Function*************************************************************

  Synopsis    [Add sequential edges.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NodePrintInfo( Abc_Obj_t * pNode )
{
    Abc_Seq_t * p = pNode->pNtk->pManFunc;
    Abc_Obj_t * pFanin, * pObj, * pLeaf;
    Vec_Ptr_t * vLeaves;
    unsigned SeqEdge;
    int i, Number;

    // print the node
    printf( "    Node      = %6d.  LValue = %7.2f.  Lag = %2d.\n", 
        pNode->Id, Seq_NodeGetLValueP(pNode), Seq_NodeGetLag(pNode) );

    // find the number
    Vec_PtrForEachEntry( p->vMapAnds, pObj, Number )
        if ( pObj == pNode )
            break;

    // get the leaves
    vLeaves = Vec_VecEntry( p->vMapCuts, Number );

    // print the leaves
    Vec_PtrForEachEntry( vLeaves, pLeaf, i )
    {
        SeqEdge   = (unsigned)pLeaf;
        pFanin    = Abc_NtkObj( pNode->pNtk, SeqEdge >> 8 );
        // print the leaf
        printf( "    Fanin%d(%d) = %6d.  LValue = %7.2f.  Lag = %2d.\n", i, SeqEdge & 255,
            pFanin->Id, Seq_NodeGetLValueP(pFanin), Seq_NodeGetLag(pFanin) );
    }
}

/**Function*************************************************************

  Synopsis    [Add sequential edges.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NodePrintInfoPlus( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanout;
    int i;
    printf( "CENTRAL NODE:\n" );
    Seq_NodePrintInfo( pNode );
    Abc_ObjForEachFanout( pNode, pFanout, i )
    {
        printf( "FANOUT%d:\n", i );
        Seq_NodePrintInfo( pFanout );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


