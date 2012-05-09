/**CFile****************************************************************

  FileName    [simSupp.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Simulation to determine functional support.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: simSupp.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "fraig.h"
#include "sim.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
static int    Sim_ComputeSuppRound( Sim_Man_t * p, bool fUseTargets );
static int    Sim_ComputeSuppRoundNode( Sim_Man_t * p, int iNumCi, bool fUseTargets );
static void   Sim_ComputeSuppSetTargets( Sim_Man_t * p );

static void   Sim_UtilAssignRandom( Sim_Man_t * p );
static void   Sim_UtilAssignFromFifo( Sim_Man_t * p );
static void   Sim_SolveTargetsUsingSat( Sim_Man_t * p, int nCounters );
static int    Sim_SolveSuppModelVerify( Abc_Ntk_t * pNtk, int * pModel, int Input, int Output );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes structural supports.]

  Description [Supports are returned as an array of bit strings, one
  for each CO.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Sim_ComputeStrSupp( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vSuppStr;
    Abc_Obj_t * pNode;
    unsigned * pSimmNode, * pSimmNode1, * pSimmNode2;
    int nSuppWords, i, k;
    // allocate room for structural supports
    nSuppWords = SIM_NUM_WORDS( Abc_NtkCiNum(pNtk) );
    vSuppStr   = Sim_UtilInfoAlloc( Abc_NtkObjNumMax(pNtk), nSuppWords, 1 );
    // assign the structural support to the PIs
    Abc_NtkForEachCi( pNtk, pNode, i )
        Sim_SuppStrSetVar( vSuppStr, pNode, i );
    // derive the structural supports of the internal nodes
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        pSimmNode  = vSuppStr->pArray[ pNode->Id ];
        pSimmNode1 = vSuppStr->pArray[ Abc_ObjFaninId0(pNode) ];
        pSimmNode2 = vSuppStr->pArray[ Abc_ObjFaninId1(pNode) ];
        for ( k = 0; k < nSuppWords; k++ )
            pSimmNode[k] = pSimmNode1[k] | pSimmNode2[k];
    }
    // set the structural supports of the PO nodes
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        pSimmNode  = vSuppStr->pArray[ pNode->Id ];
        pSimmNode1 = vSuppStr->pArray[ Abc_ObjFaninId0(pNode) ];
        for ( k = 0; k < nSuppWords; k++ )
            pSimmNode[k] = pSimmNode1[k];
    }
    return vSuppStr;
}

/**Function*************************************************************

  Synopsis    [Compute functional supports.]

  Description [Supports are returned as an array of bit strings, one
  for each CO.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Sim_ComputeFunSupp( Abc_Ntk_t * pNtk, int fVerbose )
{
    Sim_Man_t * p;
    Vec_Ptr_t * vResult;
    int nSolved, i, clk = clock();

    srand( 0xABC );

    // start the simulation manager
    p = Sim_ManStart( pNtk, 0 );

    // compute functional support using one round of random simulation
    Sim_UtilAssignRandom( p );
    Sim_ComputeSuppRound( p, 0 );

    // set the support targets 
    Sim_ComputeSuppSetTargets( p );
if ( fVerbose )
    printf( "Number of support targets after simulation = %5d.\n", Vec_VecSizeSize(p->vSuppTargs) );
    if ( Vec_VecSizeSize(p->vSuppTargs) == 0 )
        goto exit;

    for ( i = 0; i < 1; i++ )
    {
        // compute patterns using one round of random simulation
        Sim_UtilAssignRandom( p );
        nSolved = Sim_ComputeSuppRound( p, 1 );
        if ( Vec_VecSizeSize(p->vSuppTargs) == 0 )
            goto exit;

if ( fVerbose )
    printf( "Targets = %5d.   Solved = %5d.  Fifo = %5d.\n", 
       Vec_VecSizeSize(p->vSuppTargs), nSolved, Vec_PtrSize(p->vFifo) );
    }

    // try to solve the support targets
    while ( Vec_VecSizeSize(p->vSuppTargs) > 0 )
    {
        // solve targets until the first disproved one (which gives counter-example)
        Sim_SolveTargetsUsingSat( p, p->nSimWords/p->nSuppWords );
        // compute additional functional support
        Sim_UtilAssignFromFifo( p );
        nSolved = Sim_ComputeSuppRound( p, 1 );

if ( fVerbose )
    printf( "Targets = %5d.   Solved = %5d.  Fifo = %5d.  SAT runs = %3d.\n", 
            Vec_VecSizeSize(p->vSuppTargs), nSolved, Vec_PtrSize(p->vFifo), p->nSatRuns );
    }

exit:
p->timeTotal = clock() - clk;
    vResult = p->vSuppFun;  
    //  p->vSuppFun = NULL;
    Sim_ManStop( p );
    return vResult;
}

/**Function*************************************************************

  Synopsis    [Computes functional support using one round of simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_ComputeSuppRound( Sim_Man_t * p, bool fUseTargets )
{
    Vec_Int_t * vTargets;
    int i, Counter = 0;
    int clk;
    // perform one round of random simulation
clk = clock();
    Sim_UtilSimulate( p, 0 );
p->timeSim += clock() - clk;
    // iterate through the CIs and detect COs that depend on them
    for ( i = p->iInput; i < p->nInputs; i++ )
    {
        vTargets = p->vSuppTargs->pArray[i];
        if ( fUseTargets && vTargets->nSize == 0 )
            continue;
        Counter += Sim_ComputeSuppRoundNode( p, i, fUseTargets );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Computes functional support for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_ComputeSuppRoundNode( Sim_Man_t * p, int iNumCi, bool fUseTargets )
{
    int fVerbose = 0;
    Sim_Pat_t * pPat;
    Vec_Int_t * vTargets;
    Vec_Vec_t * vNodesByLevel;
    Abc_Obj_t * pNodeCi, * pNode;
    int i, k, v, Output, LuckyPat, fType0, fType1;
    int Counter = 0;
    int fFirst = 1;
    int clk;
    // collect nodes by level in the TFO of the CI 
    // this proceduredoes not collect the CIs and COs
    // but it increments TravId of the collected nodes and CIs/COs
clk = clock();
    pNodeCi       = Abc_NtkCi( p->pNtk, iNumCi );
    vNodesByLevel = Abc_DfsLevelized( pNodeCi, 0 );
p->timeTrav += clock() - clk;
    // complement the simulation info of the selected CI
    Sim_UtilInfoFlip( p, pNodeCi );
    // simulate the levelized structure of nodes
    Vec_VecForEachEntry( vNodesByLevel, pNode, i, k )
    {
        fType0 = Abc_NodeIsTravIdCurrent( Abc_ObjFanin0(pNode) );
        fType1 = Abc_NodeIsTravIdCurrent( Abc_ObjFanin1(pNode) );
clk = clock();
        Sim_UtilSimulateNode( p, pNode, 1, fType0, fType1 );
p->timeSim += clock() - clk;
    }
    // set the simulation info of the affected COs
    if ( fUseTargets )
    {
        vTargets = p->vSuppTargs->pArray[iNumCi];
        for ( i = vTargets->nSize - 1; i >= 0; i-- )
        {
            // get the target output
            Output = vTargets->pArray[i];
            // get the target node
            pNode  = Abc_ObjFanin0( Abc_NtkCo(p->pNtk, Output) );
            // the output should be in the cone
            assert( Abc_NodeIsTravIdCurrent(pNode) );

            // skip if the simulation info is equal
            if ( Sim_UtilInfoCompare( p, pNode ) )
                continue;

            // otherwise, we solved a new target
            Vec_IntRemove( vTargets, Output );
if ( fVerbose )
    printf( "(%d,%d) ", iNumCi, Output );
            Counter++;
            // make sure this variable is not yet detected
            assert( !Sim_SuppFunHasVar(p->vSuppFun, Output, iNumCi) );
            // set this variable
            Sim_SuppFunSetVar( p->vSuppFun, Output, iNumCi );
            
            // detect the differences in the simulation info
            Sim_UtilInfoDetectDiffs( p->vSim0->pArray[pNode->Id], p->vSim1->pArray[pNode->Id], p->nSimWords, p->vDiffs );
            // create new patterns
            if ( !fFirst && p->vFifo->nSize > 1000 )
                continue;

            Vec_IntForEachEntry( p->vDiffs, LuckyPat, k )
            {
                // set the new pattern
                pPat = Sim_ManPatAlloc( p );
                pPat->Input  = iNumCi;
                pPat->Output = Output;
                Abc_NtkForEachCi( p->pNtk, pNodeCi, v )
                    if ( Sim_SimInfoHasVar( p->vSim0, pNodeCi, LuckyPat ) )
                        Sim_SetBit( pPat->pData, v );
                Vec_PtrPush( p->vFifo, pPat );

                fFirst = 0;
                break;
            }
        }
if ( fVerbose && Counter )
printf( "\n" );
    }
    else
    {
        Abc_NtkForEachCo( p->pNtk, pNode, Output )
        {
            if ( !Abc_NodeIsTravIdCurrent( pNode ) )
                continue;
            if ( !Sim_UtilInfoCompare( p, Abc_ObjFanin0(pNode) ) )
            {
                if ( !Sim_SuppFunHasVar(p->vSuppFun, Output, iNumCi) )
                {
                    Counter++;
                    Sim_SuppFunSetVar( p->vSuppFun, Output, iNumCi );
                }
            }
        }
    }
    Vec_VecFree( vNodesByLevel );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Sets the simulation targets.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_ComputeSuppSetTargets( Sim_Man_t * p )
{
    Abc_Obj_t * pNode;
    unsigned * pSuppStr, * pSuppFun;
    int i, k, Num;
    Abc_NtkForEachCo( p->pNtk, pNode, i )
    {
        pSuppStr = p->vSuppStr->pArray[pNode->Id];
        pSuppFun = p->vSuppFun->pArray[i];
        // find vars in the structural support that are not in the functional support
        Sim_UtilInfoDetectNews( pSuppFun, pSuppStr, p->nSuppWords, p->vDiffs );
        Vec_IntForEachEntry( p->vDiffs, Num, k )
            Vec_VecPush( p->vSuppTargs, Num, (void *)i );
    }
}

/**Function*************************************************************

  Synopsis    [Assigns random simulation info to the PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_UtilAssignRandom( Sim_Man_t * p )
{
    Abc_Obj_t * pNode;
    unsigned * pSimInfo;
    int i, k;
    // assign the random/systematic simulation info to the PIs
    Abc_NtkForEachCi( p->pNtk, pNode, i )
    {
        pSimInfo = p->vSim0->pArray[pNode->Id];
        for ( k = 0; k < p->nSimWords; k++ )
            pSimInfo[k] = SIM_RANDOM_UNSIGNED;
    }
}

/**Function*************************************************************

  Synopsis    [Sets the new patterns from fifo.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_UtilAssignFromFifo( Sim_Man_t * p )
{
    int fUseOneWord = 0;
    Abc_Obj_t * pNode;
    Sim_Pat_t * pPat;
    unsigned * pSimInfo;
    int nWordsNew, iWord, iWordLim, i, w;
    int iBeg, iEnd;
    int Counter = 0;
    // go through the patterns and fill in the dist-1 minterms for each
    for ( iWord = 0; p->vFifo->nSize > 0; iWord = iWordLim )
    {
        ++Counter;
        // get the pattern
        pPat = Vec_PtrPop( p->vFifo );
        if ( fUseOneWord )
        {
            // get the first word of the next series
            iWordLim = iWord + 1; 
            // set the pattern for all PIs from iBit to iWord + p->nInputs
            iBeg = p->iInput;
            iEnd = ABC_MIN( iBeg + 32, p->nInputs );
//            for ( i = iBeg; i < iEnd; i++ )
            Abc_NtkForEachCi( p->pNtk, pNode, i )
            {
                pNode = Abc_NtkCi(p->pNtk,i);
                pSimInfo = p->vSim0->pArray[pNode->Id];
                if ( Sim_HasBit(pPat->pData, i) )
                    pSimInfo[iWord] = SIM_MASK_FULL;
                else
                    pSimInfo[iWord] = 0;
                // flip one bit
                if ( i >= iBeg && i < iEnd )
                    Sim_XorBit( pSimInfo + iWord, i-iBeg );
            }
        }
        else
        {
            // get the number of words for the remaining inputs
            nWordsNew = p->nSuppWords;
//            nWordsNew = SIM_NUM_WORDS( p->nInputs - p->iInput );
            // get the first word of the next series
            iWordLim = (iWord + nWordsNew < p->nSimWords)? iWord + nWordsNew : p->nSimWords; 
            // set the pattern for all CIs from iWord to iWord + nWordsNew
            Abc_NtkForEachCi( p->pNtk, pNode, i )
            {
                pSimInfo = p->vSim0->pArray[pNode->Id];
                if ( Sim_HasBit(pPat->pData, i) )
                {
                    for ( w = iWord; w < iWordLim; w++ )
                        pSimInfo[w] = SIM_MASK_FULL;
                }
                else
                {
                    for ( w = iWord; w < iWordLim; w++ )
                        pSimInfo[w] = 0;
                }
                Sim_XorBit( pSimInfo + iWord, i );
                // flip one bit
//                if ( i >= p->iInput )
//                    Sim_XorBit( pSimInfo + iWord, i-p->iInput );
            }
        }
        Sim_ManPatFree( p, pPat );
        // stop if we ran out of room for patterns
        if ( iWordLim == p->nSimWords )
            break;
//        if ( Counter == 1 )
//            break;
    }
}

/**Function*************************************************************

  Synopsis    [Get the given number of counter-examples using SAT.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_SolveTargetsUsingSat( Sim_Man_t * p, int Limit )
{
    Fraig_Params_t Params;
    Fraig_Man_t * pMan;
    Abc_Obj_t * pNodeCi;
    Abc_Ntk_t * pMiter;
    Sim_Pat_t * pPat;
    void * pEntry;
    int * pModel;
    int RetValue, Output, Input, k, v;
    int Counter = 0;
    int clk;

    p->nSatRuns = 0;
    // put targets into one array
    Vec_VecForEachEntryReverse( p->vSuppTargs, pEntry, Input, k )
    {
        p->nSatRuns++;
        Output = (int)pEntry;

        // set up the miter for the two cofactors of this output w.r.t. this input
        pMiter = Abc_NtkMiterForCofactors( p->pNtk, Output, Input, -1 );

        // transform the miter into a fraig
        Fraig_ParamsSetDefault( &Params );
        Params.nSeconds = ABC_INFINITY;
        Params.fInternal = 1;
clk = clock();
        pMan = Abc_NtkToFraig( pMiter, &Params, 0, 0 ); 
p->timeFraig += clock() - clk;
clk = clock();
        Fraig_ManProveMiter( pMan );
p->timeSat += clock() - clk;

        // analyze the result
        RetValue = Fraig_ManCheckMiter( pMan );
        assert( RetValue >= 0 );
        if ( RetValue == 1 ) // unsat
        {
            p->nSatRunsUnsat++;
            pModel = NULL;
            Vec_PtrRemove( p->vSuppTargs->pArray[Input], pEntry );
        }
        else // sat
        {
            p->nSatRunsSat++;
            pModel = Fraig_ManReadModel( pMan );
            assert( pModel != NULL );
            assert( Sim_SolveSuppModelVerify( p->pNtk, pModel, Input, Output ) );

//printf( "Solved by SAT (%d,%d).\n", Input, Output );
            // set the new pattern
            pPat = Sim_ManPatAlloc( p );
            pPat->Input  = Input;
            pPat->Output = Output;
            Abc_NtkForEachCi( p->pNtk, pNodeCi, v )
                if ( pModel[v] )
                    Sim_SetBit( pPat->pData, v );
            Vec_PtrPush( p->vFifo, pPat );
/*
            // set the new pattern
            pPat = Sim_ManPatAlloc( p );
            pPat->Input  = Input;
            pPat->Output = Output;
            Abc_NtkForEachCi( p->pNtk, pNodeCi, v )
                if ( pModel[v] )
                    Sim_SetBit( pPat->pData, v );
            Sim_XorBit( pPat->pData, Input );  // add this bit in the opposite polarity
            Vec_PtrPush( p->vFifo, pPat );
*/
            Counter++;
        }
        // delete the fraig manager
        Fraig_ManFree( pMan );
        // delete the miter
        Abc_NtkDelete( pMiter );

        // makr the input, which we are processing
        p->iInput = Input;

        // stop when we found enough patterns
//        if ( Counter == Limit )
        if ( Counter == 1 )
            return;
    }
}


/**Function*************************************************************

  Synopsis    [Saves the counter example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_NtkSimTwoPats_rec( Abc_Obj_t * pNode )
{
    int Value0, Value1;
    if ( Abc_NodeIsTravIdCurrent( pNode ) )
        return (int)pNode->pCopy;
    Abc_NodeSetTravIdCurrent( pNode );
    Value0 = Sim_NtkSimTwoPats_rec( Abc_ObjFanin0(pNode) );
    Value1 = Sim_NtkSimTwoPats_rec( Abc_ObjFanin1(pNode) );
    if ( Abc_ObjFaninC0(pNode) )
        Value0 = ~Value0;
    if ( Abc_ObjFaninC1(pNode) )
        Value1 = ~Value1;
    pNode->pCopy = (Abc_Obj_t *)(Value0 & Value1);
    return Value0 & Value1;
}

/**Function*************************************************************

  Synopsis    [Verifies that pModel proves the presence of Input in the support of Output.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_SolveSuppModelVerify( Abc_Ntk_t * pNtk, int * pModel, int Input, int Output )
{
    Abc_Obj_t * pNode;
    int RetValue, i;
    // set the PI values
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        Abc_NodeSetTravIdCurrent( pNode );
        if ( pNode == Abc_NtkCi(pNtk,Input) )
            pNode->pCopy = (Abc_Obj_t *)1;
        else if ( pModel[i] == 1 )
            pNode->pCopy = (Abc_Obj_t *)3;
        else
            pNode->pCopy = NULL;
    }
    // perform the traversal
    RetValue = 3 & Sim_NtkSimTwoPats_rec( Abc_ObjFanin0( Abc_NtkCo(pNtk,Output) ) );
//    assert( RetValue == 1 || RetValue == 2 ); 
    return RetValue == 1 || RetValue == 2;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


