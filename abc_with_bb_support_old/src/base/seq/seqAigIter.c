/**CFile****************************************************************

  FileName    [seqRetIter.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Construction and manipulation of sequential AIGs.]

  Synopsis    [The iterative L-Value computation for retiming procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: seqRetIter.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "seqInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// the internal procedures
static int Seq_RetimeSearch_rec( Abc_Ntk_t * pNtk, int FiMin, int FiMax, int fVerbose );
static int Seq_RetimeForPeriod( Abc_Ntk_t * pNtk, int Fi, int fVerbose );
static int Seq_RetimeNodeUpdateLValue( Abc_Obj_t * pObj, int Fi );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Retimes AIG for optimal delay using Pan's algorithm.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_AigRetimeDelayLags( Abc_Ntk_t * pNtk, int fVerbose )
{
    Abc_Seq_t * p = pNtk->pManFunc;
    Abc_Obj_t * pNode;
    int i, FiMax, RetValue, clk, clkIter;
    char NodeLag;

    assert( Abc_NtkIsSeq( pNtk ) );

    // get the upper bound on the clock period
    FiMax = 2 + Seq_NtkLevelMax(pNtk);

    // make sure this clock period is feasible
    if ( !Seq_RetimeForPeriod( pNtk, FiMax, fVerbose ) )
    {
        Vec_StrFill( p->vLags, p->nSize, 0 );
        printf( "Error: The upper bound on the clock period cannot be computed.\n" );
        printf( "The reason for this error may be the presence in the circuit of logic\n" );
        printf( "that is not reachable from the PIs. Mapping/retiming is not performed.\n" );
        return 0;
    }

    // search for the optimal clock period between 0 and nLevelMax
clk = clock();
    p->FiBestInt = Seq_RetimeSearch_rec( pNtk, 0, FiMax, fVerbose );
clkIter = clock() - clk;

    // recompute the best l-values
    RetValue = Seq_RetimeForPeriod( pNtk, p->FiBestInt, fVerbose );
    assert( RetValue );

    // fix the problem with non-converged delays
    Abc_AigForEachAnd( pNtk, pNode, i )
        if ( Seq_NodeGetLValue(pNode) < -ABC_INFINITY/2 )
            Seq_NodeSetLValue( pNode, 0 );

    // write the retiming lags
    Vec_StrFill( p->vLags, p->nSize, 0 );
    Abc_AigForEachAnd( pNtk, pNode, i )
    {
        NodeLag = Seq_NodeComputeLag( Seq_NodeGetLValue(pNode), p->FiBestInt );
        Seq_NodeSetLag( pNode, NodeLag );
    }

    // print the result
    if ( fVerbose )
    printf( "The best clock period is %3d.\n", p->FiBestInt );

/*
    printf( "lvalues and lags : " );
    Abc_AigForEachAnd( pNtk, pNode, i )
        printf( "%d=%d(%d) ", pNode->Id, Seq_NodeGetLValue(pNode), Seq_NodeGetLag(pNode) );
    printf( "\n" );
*/
/*
    {
        FILE * pTable;
        pTable = fopen( "stats.txt", "a+" );
        fprintf( pTable, "%s ",  pNtk->pName );
        fprintf( pTable, "%d ", FiBest );
        fprintf( pTable, "\n" );
        fclose( pTable );
    }
*/
/*
    {
        FILE * pTable;
        pTable = fopen( "stats.txt", "a+" );
        fprintf( pTable, "%s ",  pNtk->pName );
        fprintf( pTable, "%.2f ", (float)(p->timeCuts)/(float)(CLOCKS_PER_SEC) );
        fprintf( pTable, "%.2f ", (float)(clkIter)/(float)(CLOCKS_PER_SEC) );
        fprintf( pTable, "\n" );
        fclose( pTable );
    }
*/
    return 1;

}

/**Function*************************************************************

  Synopsis    [Performs binary search for the optimal clock period.]

  Description [Assumes that FiMin is infeasible while FiMax is feasible.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_RetimeSearch_rec( Abc_Ntk_t * pNtk, int FiMin, int FiMax, int fVerbose )
{
    int Median;
    assert( FiMin < FiMax );
    if ( FiMin + 1 == FiMax )
        return FiMax;
    Median = FiMin + (FiMax - FiMin)/2;
    if ( Seq_RetimeForPeriod( pNtk, Median, fVerbose ) )
        return Seq_RetimeSearch_rec( pNtk, FiMin, Median, fVerbose ); // Median is feasible
    else 
        return Seq_RetimeSearch_rec( pNtk, Median, FiMax, fVerbose ); // Median is infeasible
}

/**Function*************************************************************

  Synopsis    [Returns 1 if retiming with this clock period is feasible.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_RetimeForPeriod( Abc_Ntk_t * pNtk, int Fi, int fVerbose )
{
    Abc_Seq_t * p = pNtk->pManFunc;
    Abc_Obj_t * pObj;
    int i, c, RetValue, fChange, Counter;
    char * pReason = "";

    // set l-values of all nodes to be minus infinity
    Vec_IntFill( p->vLValues, p->nSize, -ABC_INFINITY );

    // set l-values of constants and PIs
    pObj = Abc_NtkObj( pNtk, 0 );
    Seq_NodeSetLValue( pObj, 0 );
    Abc_NtkForEachPi( pNtk, pObj, i )
        Seq_NodeSetLValue( pObj, 0 );

    // update all values iteratively
    Counter = 0;
    for ( c = 0; c < p->nMaxIters; c++ )
    {
        fChange = 0;
        Abc_AigForEachAnd( pNtk, pObj, i )
        {
            Counter++;
            if ( Seq_NodeCutMan(pObj) )
                RetValue = Seq_FpgaNodeUpdateLValue( pObj, Fi );
            else
                RetValue = Seq_RetimeNodeUpdateLValue( pObj, Fi );
            if ( RetValue == SEQ_UPDATE_YES ) 
                fChange = 1;
        }
        Abc_NtkForEachPo( pNtk, pObj, i )
        {
            if ( Seq_NodeCutMan(pObj) )
                RetValue = Seq_FpgaNodeUpdateLValue( pObj, Fi );
            else
                RetValue = Seq_RetimeNodeUpdateLValue( pObj, Fi );
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
            printf( "Period = %3d.  Iterations = %3d.  Updates = %10d.    Infeasible %s\n", Fi, c, Counter, pReason );
        else
            printf( "Period = %3d.  Iterations = %3d.  Updates = %10d.      Feasible\n",    Fi, c, Counter );
    }
/*
    // check if any AND gates have infinite delay
    Counter = 0;
    Abc_AigForEachAnd( pNtk, pObj, i )
        Counter += (Seq_NodeGetLValue(pObj) < -ABC_INFINITY/2);
    if ( Counter > 0 )
        printf( "Warning: %d internal nodes have wrong l-values!\n", Counter );
*/
    return RetValue != SEQ_UPDATE_FAIL;
}

/**Function*************************************************************

  Synopsis    [Computes the l-value of the node.]

  Description [The node can be internal or a PO.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_RetimeNodeUpdateLValue( Abc_Obj_t * pObj, int Fi )
{
    int lValueNew, lValueOld, lValue0, lValue1;
    assert( !Abc_ObjIsPi(pObj) );
    assert( Abc_ObjFaninNum(pObj) > 0 );
    lValue0 = Seq_NodeGetLValue(Abc_ObjFanin0(pObj)) - Fi * Seq_ObjFaninL0(pObj);
    if ( Abc_ObjIsPo(pObj) )
        return (lValue0 > Fi)? SEQ_UPDATE_FAIL : SEQ_UPDATE_NO;
    if ( Abc_ObjFaninNum(pObj) == 2 )
        lValue1 = Seq_NodeGetLValue(Abc_ObjFanin1(pObj)) - Fi * Seq_ObjFaninL1(pObj);
    else
        lValue1 = -ABC_INFINITY;
    lValueNew = 1 + ABC_MAX( lValue0, lValue1 );
    lValueOld = Seq_NodeGetLValue(pObj);
//    if ( lValueNew == lValueOld )
    if ( lValueNew <= lValueOld )
        return SEQ_UPDATE_NO;
    Seq_NodeSetLValue( pObj, lValueNew );
    return SEQ_UPDATE_YES;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


