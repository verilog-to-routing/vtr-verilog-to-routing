/**CFile****************************************************************

  FileName    [abcResub.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Resubstitution manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcResub.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "base/abc/abc.h"
#include "bool/dec/dec.h"
#include "opt/rwr/rwr.h"
#include "bool/kit/kit.h"

ABC_NAMESPACE_IMPL_START

 
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
static Cut_Man_t * Abc_NtkStartCutManForRewrite( Abc_Ntk_t * pNtk );
extern void        Abc_NodePrintCuts( Abc_Obj_t * pNode );
extern void        Abc_ManShowCutCone( Abc_Obj_t * pNode, Vec_Ptr_t * vLeaves );

extern void  Abc_PlaceBegin( Abc_Ntk_t * pNtk );
extern void  Abc_PlaceEnd( Abc_Ntk_t * pNtk );
extern void  Abc_PlaceUpdate( Vec_Ptr_t * vAddedCells, Vec_Ptr_t * vUpdatedNets );

extern int         Dec_GraphUpdateNetwork( Abc_Obj_t * pRoot, Dec_Graph_t * pGraph, int fUpdateLevel, int nGain );

#define ABC_RS_DIV1_MAX    150   // the max number of divisors to consider
#define ABC_RS_DIV2_MAX    500   // the max number of pair-wise divisors to consider

typedef struct Abc_ManRes_t_ Abc_ManRes_t;
struct Abc_ManRes_t_
{
    // paramers
    int                nLeavesMax; // the max number of leaves in the cone
    int                nDivsMax;   // the max number of divisors in the cone
    // representation of the cone
    Abc_Obj_t *        pRoot;      // the root of the cone
    int                nLeaves;    // the number of leaves
    int                nDivs;      // the number of all divisor (including leaves)
    int                nMffc;      // the size of MFFC
    int                nLastGain;  // the gain the number of nodes
    Vec_Ptr_t *        vDivs;      // the divisors
    // representation of the simulation info
    int                nBits;      // the number of simulation bits
    int                nWords;     // the number of unsigneds for siminfo
    Vec_Ptr_t        * vSims;      // simulation info
    unsigned         * pInfo;      // pointer to simulation info
    // observability don't-cares
    unsigned *         pCareSet;
    // internal divisor storage
    Vec_Ptr_t        * vDivs1UP;   // the single-node unate divisors
    Vec_Ptr_t        * vDivs1UN;   // the single-node unate divisors
    Vec_Ptr_t        * vDivs1B;    // the single-node binate divisors
    Vec_Ptr_t        * vDivs2UP0;  // the double-node unate divisors
    Vec_Ptr_t        * vDivs2UP1;  // the double-node unate divisors
    Vec_Ptr_t        * vDivs2UN0;  // the double-node unate divisors
    Vec_Ptr_t        * vDivs2UN1;  // the double-node unate divisors
    // other data
    Vec_Ptr_t        * vTemp;      // temporary array of nodes
    // runtime statistics
    abctime            timeCut;
    abctime            timeTruth;
    abctime            timeRes;
    abctime            timeDiv;
    abctime            timeMffc;
    abctime            timeSim;
    abctime            timeRes1;
    abctime            timeResD;
    abctime            timeRes2;
    abctime            timeRes3;
    abctime            timeNtk;
    abctime            timeTotal;
    // improvement statistics
    int                nUsedNodeC;
    int                nUsedNode0;
    int                nUsedNode1Or;
    int                nUsedNode1And;
    int                nUsedNode2Or;
    int                nUsedNode2And;
    int                nUsedNode2OrAnd;
    int                nUsedNode2AndOr;
    int                nUsedNode3OrAnd;
    int                nUsedNode3AndOr;
    int                nUsedNodeTotal;
    int                nTotalDivs;
    int                nTotalLeaves;
    int                nTotalGain;
    int                nNodesBeg;
    int                nNodesEnd;
};

// external procedures
static Abc_ManRes_t* Abc_ManResubStart( int nLeavesMax, int nDivsMax );
static void          Abc_ManResubStop( Abc_ManRes_t * p );
static Dec_Graph_t * Abc_ManResubEval( Abc_ManRes_t * p, Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves, int nSteps, int fUpdateLevel, int fVerbose );
static void          Abc_ManResubCleanup( Abc_ManRes_t * p );
static void          Abc_ManResubPrint( Abc_ManRes_t * p );

// other procedures
static int           Abc_ManResubCollectDivs( Abc_ManRes_t * p, Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves, int Required );
static void          Abc_ManResubSimulate( Vec_Ptr_t * vDivs, int nLeaves, Vec_Ptr_t * vSims, int nLeavesMax, int nWords );
static void          Abc_ManResubPrintDivs( Abc_ManRes_t * p, Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves );

static void          Abc_ManResubDivsS( Abc_ManRes_t * p, int Required );
static void          Abc_ManResubDivsD( Abc_ManRes_t * p, int Required );
static Dec_Graph_t * Abc_ManResubQuit( Abc_ManRes_t * p );
static Dec_Graph_t * Abc_ManResubDivs0( Abc_ManRes_t * p );
static Dec_Graph_t * Abc_ManResubDivs1( Abc_ManRes_t * p, int Required );
static Dec_Graph_t * Abc_ManResubDivs12( Abc_ManRes_t * p, int Required );
static Dec_Graph_t * Abc_ManResubDivs2( Abc_ManRes_t * p, int Required );
static Dec_Graph_t * Abc_ManResubDivs3( Abc_ManRes_t * p, int Required );

static Vec_Ptr_t *   Abc_CutFactorLarge( Abc_Obj_t * pNode, int nLeavesMax );
static int           Abc_CutVolumeCheck( Abc_Obj_t * pNode, Vec_Ptr_t * vLeaves );

extern abctime s_ResubTime;

typedef struct Abc_ManRef_t_   Abc_ManRef_t;
struct Abc_ManRef_t_
{
    int              nNodeSizeMax;      // the limit on the size of the supernode
    int              nConeSizeMax;      // the limit on the size of the containing cone
    int              fVerbose;          // the verbosity flag
    Vec_Ptr_t *      vVars;             // truth tables
    Vec_Ptr_t *      vFuncs;            // functions
    Vec_Int_t *      vMemory;           // memory
    Vec_Str_t *      vCube;             // temporary
    Vec_Int_t *      vForm;             // temporary
    Vec_Ptr_t *      vVisited;          // temporary
    Vec_Ptr_t *      vLeaves;           // temporary
    int              nLastGain;
    int              nNodesConsidered;
    int              nNodesRefactored;
    int              nNodesGained;
    int              nNodesBeg;
    int              nNodesEnd;
    abctime          timeCut;
    abctime          timeTru;
    abctime          timeDcs;
    abctime          timeSop;
    abctime          timeFact;
    abctime          timeEval;
    abctime          timeRes;
    abctime          timeNtk;
    abctime          timeTotal;
};   
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
/** Function***********************************************************
 Rewrite
**********************************************************************/

int Abc_NtkRewrite3( Abc_Ntk_t * pNtk, Vec_Int_t **pGain_rw, int fUpdateLevel, int fUseZeros, int fVerbose, int fVeryVerbose, int fPlaceEnable )
{
    ProgressBar * pProgress;
    Cut_Man_t * pManCut;
    Rwr_Man_t * pManRwr;
    Abc_Obj_t * pNode;
    FILE *fpt;
    Dec_Graph_t * pGraph;
    int i, nNodes, nGain, fCompl;
    int success = 0;
    int fail = 0;
    abctime clk, clkStart = Abc_Clock();
    assert( Abc_NtkIsStrash(pNtk) );
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);
    pManRwr = Rwr_ManStart( 0 );
    if ( pManRwr == NULL )
        return 0;
    if ( fUpdateLevel )
        Abc_NtkStartReverseLevels( pNtk, 0 );
clk = Abc_Clock();
    pManCut = Abc_NtkStartCutManForRewrite( pNtk );
Rwr_ManAddTimeCuts( pManRwr, Abc_Clock() - clk );
    pNtk->pManCut = pManCut;

    if ( fVeryVerbose )
        Rwr_ScoresClean( pManRwr );

    pManRwr->nNodesBeg = Abc_NtkNodeNum(pNtk);
    nNodes = Abc_NtkObjNumMax(pNtk);

    //printf("nNodes: %d\n", nNodes);
    if ( pGain_rw ) *pGain_rw = Vec_IntAlloc(1);

    pProgress = Extra_ProgressBarStart( stdout, nNodes );
    fpt = fopen("rewrite_id_nGain.csv", "w");
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        //printf("rewrite: %d\n", pNode->Id);
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        if ( i >= nNodes )
            break;
        if ( Abc_NodeIsPersistant(pNode) )
        {
            Vec_IntPush( (*pGain_rw), -99);
            fprintf(fpt, "%d, %d\n", pNode->Id, -99);
            continue;
        }
        if ( Abc_ObjFanoutNum(pNode) > 1000 )
        {
            fprintf(fpt, "%d, %d\n", pNode->Id, -99);
            Vec_IntPush( (*pGain_rw), -99);
            continue;
         }
        nGain = Rwr_NodeRewrite( pManRwr, pManCut, pNode, fUpdateLevel, fUseZeros, fPlaceEnable );
        //printf("nGain3: %d id: %d\n", nGain, i);
        fprintf(fpt, "%d, %d\n", pNode->Id, nGain);
        Vec_IntPush( (*pGain_rw), nGain);
        //printf("size of vector: %d\n", (**pGain_rw).nSize);
        //printf("write nGain in vector.\n");

        if ( !(nGain > 0 || (nGain == 0 && fUseZeros)) ){
            fail++;
            continue;
        }
        success++;
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);

        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        //Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );

        if ( fCompl ) Dec_GraphComplement( pGraph );
    }
    fclose(fpt);
    printf("size of vector: %d\n", (**pGain_rw).nSize);
    //printf("nGain in vector: %d\n", (**pGain_rw).pArray[61]);
    Extra_ProgressBarStop( pProgress );
Rwr_ManAddTimeTotal( pManRwr, Abc_Clock() - clkStart );
    pManRwr->nNodesEnd = Abc_NtkNodeNum(pNtk);
    if ( fVerbose )
        Rwr_ManPrintStats( pManRwr );
    if ( fVeryVerbose )
        Rwr_ScoresReport( pManRwr );
    Rwr_ManStop( pManRwr );
    Cut_ManStop( pManCut );
    pNtk->pManCut = NULL;

    {
    Abc_NtkReassignIds( pNtk );
    }
    if ( fUpdateLevel )
        Abc_NtkStopReverseLevels( pNtk );
    else
        Abc_NtkLevel( pNtk );
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Abc_NtkRewrite3: The network check has failed.\n" );
        return 0;
    }
        printf( "Abc_NtkRewrite3: success : %d; fail : %d\n", success, fail );
    return 1;
}

Cut_Man_t * Abc_NtkStartCutManForRewrite( Abc_Ntk_t * pNtk )
{
    static Cut_Params_t Params, * pParams = &Params;
    Cut_Man_t * pManCut;
    Abc_Obj_t * pObj;
    int i;
    memset( pParams, 0, sizeof(Cut_Params_t) );
    pParams->nVarsMax  = 4;     // the max cut size ("k" of the k-feasible cuts)
    pParams->nKeepMax  = 250;   // the max number of cuts kept at a node
    pParams->fTruth    = 1;     // compute truth tables
    pParams->fFilter   = 1;     // filter dominated cuts
    pParams->fSeq      = 0;     // compute sequential cuts
    pParams->fDrop     = 0;     // drop cuts on the fly
    pParams->fVerbose  = 0;     // the verbosiness flag
    pParams->nIdsMax   = Abc_NtkObjNumMax( pNtk );
    pManCut = Cut_ManStart( pParams );
    if ( pParams->fDrop )
        Cut_ManSetFanoutCounts( pManCut, Abc_NtkFanoutCounts(pNtk) );
    Abc_NtkForEachCi( pNtk, pObj, i )
        if ( Abc_ObjFanoutNum(pObj) > 0 )
            Cut_NodeSetTriv( pManCut, pObj->Id );
    return pManCut;
}

/******Function******************************************
 Refactor
********************************************************/
word * Abc_NodeConeTruth_1( Vec_Ptr_t * vVars, Vec_Ptr_t * vFuncs, int nWordsMax, Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vVisited )
{   
    Abc_Obj_t * pNode;
    word * pTruth0, * pTruth1, * pTruth = NULL;
    int i, k, nWords = Abc_Truth6WordNum( Vec_PtrSize(vLeaves) );
    Abc_NodeConeCollect( &pRoot, 1, vLeaves, vVisited, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pNode, i )
        pNode->pCopy = (Abc_Obj_t *)Vec_PtrEntry( vVars, i );
    for ( i = Vec_PtrSize(vFuncs); i < Vec_PtrSize(vVisited); i++ )
        Vec_PtrPush( vFuncs, ABC_ALLOC(word, nWordsMax) );
    Vec_PtrForEachEntry( Abc_Obj_t *, vVisited, pNode, i )
    {   
        assert( !Abc_ObjIsPi(pNode) );
        pTruth0 = (word *)Abc_ObjFanin0(pNode)->pCopy;
        pTruth1 = (word *)Abc_ObjFanin1(pNode)->pCopy;
        pTruth  = (word *)Vec_PtrEntry( vFuncs, i );
        if ( Abc_ObjFaninC0(pNode) )
        {   
            if ( Abc_ObjFaninC1(pNode) ) 
                for ( k = 0; k < nWords; k++ )
                    pTruth[k] = ~pTruth0[k] & ~pTruth1[k];
            else
                for ( k = 0; k < nWords; k++ )
                    pTruth[k] = ~pTruth0[k] &  pTruth1[k];
        }
        else
        {
            if ( Abc_ObjFaninC1(pNode) )
                for ( k = 0; k < nWords; k++ )
                    pTruth[k] =  pTruth0[k] & ~pTruth1[k];
            else
                for ( k = 0; k < nWords; k++ )
                    pTruth[k] =  pTruth0[k] &  pTruth1[k];
        }
        pNode->pCopy = (Abc_Obj_t *)pTruth;
    }
    return pTruth;
}
int Abc_NodeConeIsConst0_1( word * pTruth, int nVars )
{
    int k, nWords = Abc_Truth6WordNum( nVars );
    for ( k = 0; k < nWords; k++ )
        if ( pTruth[k] )
            return 0;
    return 1;
}
int Abc_NodeConeIsConst1_1( word * pTruth, int nVars )
{
    int k, nWords = Abc_Truth6WordNum( nVars );
    for ( k = 0; k < nWords; k++ )
        if ( ~pTruth[k] )
            return 0;
    return 1;
}



Dec_Graph_t * Abc_NodeRefactor_1( Abc_ManRef_t * p, Abc_Obj_t * pNode, Vec_Ptr_t * vFanins, int fUpdateLevel, int fUseZeros, int fUseDcs, int fVerbose )
{
    extern int    Dec_GraphToNetworkCount( Abc_Obj_t * pRoot, Dec_Graph_t * pGraph, int NodeMax, int LevelMax );
    int fVeryVerbose = 0;
    int nVars = Vec_PtrSize(vFanins);
    int nWordsMax = Abc_Truth6WordNum(p->nNodeSizeMax);
    Dec_Graph_t * pFForm;
    Abc_Obj_t * pFanin;
    word * pTruth;
    abctime clk;
    int i, nNodesSaved, nNodesAdded, Required;

    p->nNodesConsidered++;

    Required = fUpdateLevel? Abc_ObjRequiredLevel(pNode) : ABC_INFINITY;
clk = Abc_Clock();
    pTruth = Abc_NodeConeTruth_1( p->vVars, p->vFuncs, nWordsMax, pNode, vFanins, p->vVisited );
p->timeTru += Abc_Clock() - clk;
    if ( pTruth == NULL )
        return NULL;
    if ( Abc_NodeConeIsConst0_1(pTruth, nVars) || Abc_NodeConeIsConst1_1(pTruth, nVars) )
    {
        p->nLastGain = Abc_NodeMffcSize( pNode );
        p->nNodesGained += p->nLastGain;
        p->nNodesRefactored++;
        return Abc_NodeConeIsConst0_1(pTruth, nVars) ? Dec_GraphCreateConst0() : Dec_GraphCreateConst1();
    }
clk = Abc_Clock();
    pFForm = (Dec_Graph_t *)Kit_TruthToGraph( (unsigned *)pTruth, nVars, p->vMemory );
p->timeFact += Abc_Clock() - clk;
    Vec_PtrForEachEntry( Abc_Obj_t *, vFanins, pFanin, i )
        pFanin->vFanouts.nSize++;
    Abc_NtkIncrementTravId( pNode->pNtk );
    nNodesSaved = Abc_NodeMffcLabelAig( pNode );
    Vec_PtrForEachEntry( Abc_Obj_t *, vFanins, pFanin, i )
    {
        pFanin->vFanouts.nSize--;
        Dec_GraphNode(pFForm, i)->pFunc = pFanin;
    }
clk = Abc_Clock();
    nNodesAdded = Dec_GraphToNetworkCount( pNode, pFForm, nNodesSaved, Required );
p->timeEval += Abc_Clock() - clk;
    if ( nNodesAdded == -1 || (nNodesAdded == nNodesSaved && !fUseZeros) )
    {
        Dec_GraphFree( pFForm );
        return NULL;
    }

    p->nLastGain = nNodesSaved - nNodesAdded;
    p->nNodesGained += p->nLastGain;
    p->nNodesRefactored++;

    if ( fVeryVerbose )
    {
        printf( "Node %6s : ",  Abc_ObjName(pNode) );
        printf( "Cone = %2d. ", vFanins->nSize );
        printf( "FF = %2d. ",   1 + Dec_GraphNodeNum(pFForm) );
        printf( "MFFC = %2d. ", nNodesSaved );
        printf( "Add = %2d. ",  nNodesAdded );
        printf( "GAIN = %2d. ", p->nLastGain );
        printf( "\n" );
    }
    return pFForm;
}



Abc_ManRef_t * Abc_NtkManRefStart_1( int nNodeSizeMax, int nConeSizeMax, int fUseDcs, int fVerbose )
{   
    Abc_ManRef_t * p;
    p = ABC_ALLOC( Abc_ManRef_t, 1 );
    memset( p, 0, sizeof(Abc_ManRef_t) );
    p->vCube        = Vec_StrAlloc( 100 );
    p->vVisited     = Vec_PtrAlloc( 100 );
    p->nNodeSizeMax = nNodeSizeMax;
    p->nConeSizeMax = nConeSizeMax;
    p->fVerbose     = fVerbose;
    p->vVars        = Vec_PtrAllocTruthTables( Abc_MaxInt(nNodeSizeMax, 6) );
    p->vFuncs       = Vec_PtrAlloc( 100 );
    p->vMemory      = Vec_IntAlloc( 1 << 16 );
    return p;
}

void Abc_NtkManRefStop_1( Abc_ManRef_t * p )
{   
    Vec_PtrFreeFree( p->vFuncs );
    Vec_PtrFree( p->vVars );
    Vec_IntFree( p->vMemory );
    Vec_PtrFree( p->vVisited );
    Vec_StrFree( p->vCube );
    ABC_FREE( p );
}

void Abc_NtkManRefPrintStats_1( Abc_ManRef_t * p )
{   
    printf( "Refactoring statistics:\n" );
    printf( "Nodes considered  = %8d.\n", p->nNodesConsidered );
    printf( "Nodes refactored  = %8d.\n", p->nNodesRefactored );
    printf( "Gain              = %8d. (%6.2f %%).\n", p->nNodesBeg-p->nNodesEnd, 100.0*(p->nNodesBeg-p->nNodesEnd)/p->nNodesBeg );
    ABC_PRT( "Cuts       ", p->timeCut );
    ABC_PRT( "Resynthesis", p->timeRes );
    ABC_PRT( "    BDD    ", p->timeTru );
    ABC_PRT( "    DCs    ", p->timeDcs );
    ABC_PRT( "    SOP    ", p->timeSop );
    ABC_PRT( "    FF     ", p->timeFact );
    ABC_PRT( "    Eval   ", p->timeEval );
    ABC_PRT( "AIG update ", p->timeNtk );
    ABC_PRT( "TOTAL      ", p->timeTotal );
}

int Abc_NtkRefactor3( Abc_Ntk_t * pNtk, Vec_Int_t **pGain_ref, int nNodeSizeMax, int nConeSizeMax, int fUpdateLevel, int fUseZeros, int fUseDcs, int fVerbose )
{
    ProgressBar * pProgress;
    Abc_ManRef_t * pManRef;
    Abc_ManCut_t * pManCut;
    Dec_Graph_t * pFForm;
    Vec_Ptr_t * vFanins;
    Abc_Obj_t * pNode;
    FILE *fpt;
    abctime clk, clkStart = Abc_Clock();
    int i, nNodes, RetValue = 1;

    assert( Abc_NtkIsStrash(pNtk) );
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);
    pManCut = Abc_NtkManCutStart( nNodeSizeMax, nConeSizeMax, 2, 1000 );
    pManRef = Abc_NtkManRefStart_1( nNodeSizeMax, nConeSizeMax, fUseDcs, fVerbose );
    pManRef->vLeaves   = Abc_NtkManCutReadCutLarge( pManCut );
    if ( fUpdateLevel )
        Abc_NtkStartReverseLevels( pNtk, 0 );
    pManRef->nNodesBeg = Abc_NtkNodeNum(pNtk);
    nNodes = Abc_NtkObjNumMax(pNtk);
    //printf("nNodes: %d\n", nNodes);
    if (pGain_ref) *pGain_ref = Vec_IntAlloc(1);

    pProgress = Extra_ProgressBarStart( stdout, nNodes );
    fpt = fopen("refactor_id_nGain.csv", "w");

    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        //printf("Refactor3 Id: %d\n", pNode->Id);
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        if ( Abc_NodeIsPersistant(pNode) )
            continue;
        if ( Abc_ObjFanoutNum(pNode) > 1000 )
            continue;
        if ( i >= nNodes )
            break;
clk = Abc_Clock();
        vFanins = Abc_NodeFindCut( pManCut, pNode, fUseDcs );
pManRef->timeCut += Abc_Clock() - clk;
clk = Abc_Clock();
        pFForm = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros, fUseDcs, fVerbose );
pManRef->timeRes += Abc_Clock() - clk;
        //printf("nLastGain3: %d\n", pManRef->nLastGain);
        fprintf(fpt, "%d, %d\n", pNode->Id, pManRef->nLastGain);
        Vec_IntPush((*pGain_ref), pManRef->nLastGain);

        if ( pFForm == NULL )
            continue;
clk = Abc_Clock();

/*
        if ( !Dec_GraphUpdateNetwork( pNode, pFForm, fUpdateLevel, pManRef->nLastGain ) )
        {
            Dec_GraphFree( pFForm );
            RetValue = -1;
            break;
        }
*/

pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFForm );
    }
    fclose(fpt);
    printf("size of vector: %d\n", (**pGain_ref).nSize);
    //printf("nGain in vector: %d\n", (**pGain_ref).pArray[20]);
    Extra_ProgressBarStop( pProgress );
pManRef->timeTotal = Abc_Clock() - clkStart;
    pManRef->nNodesEnd = Abc_NtkNodeNum(pNtk);

    if ( fVerbose )
        Abc_NtkManRefPrintStats_1( pManRef );
    Abc_NtkManCutStop( pManCut );
    Abc_NtkManRefStop_1( pManRef );
    Abc_NtkReassignIds( pNtk );
    if ( RetValue != -1 )
    {
        if ( fUpdateLevel )
            Abc_NtkStopReverseLevels( pNtk );
        else
            Abc_NtkLevel( pNtk );
        if ( !Abc_NtkCheck( pNtk ) )
        {
            printf( "Abc_NtkRefactor: The network check has failed.\n" );
            return 0;
        }
    }
    return RetValue;
}
/**Function*************************************************************

  Synopsis    [Performs incremental resynthesis of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

int Abc_NtkResubstitute3( Abc_Ntk_t * pNtk, Vec_Int_t **pGain_res, int nCutMax, int nStepsMax, int nLevelsOdc, int fUpdateLevel, int fVerbose, int fVeryVerbose )
{
    ProgressBar * pProgress;
    Abc_ManRes_t * pManRes;
    Abc_ManCut_t * pManCut;
    Odc_Man_t * pManOdc = NULL;
    Dec_Graph_t * pFForm;
    Vec_Ptr_t * vLeaves;
    Abc_Obj_t * pNode;
    FILE *fpt;
    abctime clk, clkStart = Abc_Clock();
    abctime s_ResubTime;
    int i, nNodes;

    assert( Abc_NtkIsStrash(pNtk) );

    // cleanup the AIG
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);
    // start the managers
    pManCut = Abc_NtkManCutStart( nCutMax, 100000, 100000, 100000 );
    pManRes = Abc_ManResubStart( nCutMax, ABC_RS_DIV1_MAX );
    if ( nLevelsOdc > 0 )
    pManOdc = Abc_NtkDontCareAlloc( nCutMax, nLevelsOdc, fVerbose, fVeryVerbose );

    // compute the reverse levels if level update is requested
    if ( fUpdateLevel )
        Abc_NtkStartReverseLevels( pNtk, 0 );

    if ( Abc_NtkLatchNum(pNtk) ) {
        Abc_NtkForEachLatch(pNtk, pNode, i)
            pNode->pNext = (Abc_Obj_t *)pNode->pData;
    }

    // resynthesize each node once
    pManRes->nNodesBeg = Abc_NtkNodeNum(pNtk);
    nNodes = Abc_NtkObjNumMax(pNtk);
    //printf("nNodes: %d\n", nNodes);
    if (pGain_res) *pGain_res = Vec_IntAlloc(1);

    pProgress = Extra_ProgressBarStart( stdout, nNodes );
    fpt = fopen("resub_id_nGain.csv", "w");

    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        //printf("resub id: %d\n", pNode->Id);
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        // skip the constant node
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        // skip persistant nodes
        if ( Abc_NodeIsPersistant(pNode) )
        {
            fprintf(fpt, "%d, %d\n", pNode->Id, -99);
            Vec_IntPush((*pGain_res), -99);
            continue;
        } 
        // skip the nodes with many fanouts
        if ( Abc_ObjFanoutNum(pNode) > 1000 )
        {
            fprintf(fpt, "%d, %d\n", pNode->Id, -99);
            Vec_IntPush((*pGain_res), -99);
            continue;
        }
        // stop if all nodes have been tried once
        if ( i >= nNodes )
            break;

        // compute a reconvergence-driven cut
clk = Abc_Clock();
        vLeaves = Abc_NodeFindCut( pManCut, pNode, 0 );
//        vLeaves = Abc_CutFactorLarge( pNode, nCutMax );
pManRes->timeCut += Abc_Clock() - clk;
/*
        if ( fVerbose && vLeaves )
        printf( "Node %6d : Leaves = %3d. Volume = %3d.\n", pNode->Id, Vec_PtrSize(vLeaves), Abc_CutVolumeCheck(pNode, vLeaves) );
        if ( vLeaves == NULL )
            continue;
*/
        // get the don't-cares
        if ( pManOdc )
        {
clk = Abc_Clock();
            Abc_NtkDontCareClear( pManOdc );
            Abc_NtkDontCareCompute( pManOdc, pNode, vLeaves, pManRes->pCareSet );
pManRes->timeTruth += Abc_Clock() - clk;
        }

        // evaluate this cut
clk = Abc_Clock();
        pFForm = Abc_ManResubEval( pManRes, pNode, vLeaves, nStepsMax, fUpdateLevel, fVerbose );
//        Vec_PtrFree( vLeaves );
//        Abc_ManResubCleanup( pManRes );
pManRes->timeRes += Abc_Clock() - clk;
        // put nGain in Vector
        //printf("nLastGain3: %d\n", pManRes->nLastGain);
        fprintf(fpt, "%d, %d\n", pNode->Id, pManRes->nLastGain);
        Vec_IntPush((*pGain_res), pManRes->nLastGain);
        // printf("size of vector %d\n", (**pGain).nSize);
        if ( pFForm == NULL )
            continue;
        pManRes->nTotalGain += pManRes->nLastGain;
/*
        if ( pManRes->nLeaves == 4 && pManRes->nMffc == 2 && pManRes->nLastGain == 1 )
        {
            printf( "%6d :  L = %2d. V = %2d. Mffc = %2d. Divs = %3d.   Up = %3d. Un = %3d. B = %3d.\n", 
                   pNode->Id, pManRes->nLeaves, Abc_CutVolumeCheck(pNode, vLeaves), pManRes->nMffc, pManRes->nDivs, 
                   pManRes->vDivs1UP->nSize, pManRes->vDivs1UN->nSize, pManRes->vDivs1B->nSize );
            Abc_ManResubPrintDivs( pManRes, pNode, vLeaves );
        }
*/

clk = Abc_Clock();
        //Dec_GraphUpdateNetwork( pNode, pFForm, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFForm );

    }
    fclose(fpt);
    printf("size of vector %d\n", (**pGain_res).nSize);
    //printf("nGain in vector: %d\n", (**pGain_res).pArray[20]);
    Extra_ProgressBarStop( pProgress );
pManRes->timeTotal = Abc_Clock() - clkStart;
    pManRes->nNodesEnd = Abc_NtkNodeNum(pNtk);

    // print statistics
    if ( fVerbose )
    Abc_ManResubPrint( pManRes );

    // delete the managers
    Abc_ManResubStop( pManRes );
    Abc_NtkManCutStop( pManCut );
    if ( pManOdc ) Abc_NtkDontCareFree( pManOdc );

    // clean the data field
    Abc_NtkForEachObj( pNtk, pNode, i )
        pNode->pData = NULL;

    if ( Abc_NtkLatchNum(pNtk) ) {
        Abc_NtkForEachLatch(pNtk, pNode, i)
            pNode->pData = pNode->pNext, pNode->pNext = NULL;
    }

    // put the nodes into the DFS order and reassign their IDs
    Abc_NtkReassignIds( pNtk );
//    Abc_AigCheckFaninOrder( pNtk->pManFunc );
    // fix the levels
    if ( fUpdateLevel )
        Abc_NtkStopReverseLevels( pNtk );
    else
        Abc_NtkLevel( pNtk );
    // check
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Abc_NtkRefactor: The network check has failed.\n" );
        return 0;
    }
s_ResubTime = Abc_Clock() - clkStart;
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_ManRes_t * Abc_ManResubStart( int nLeavesMax, int nDivsMax )
{
    Abc_ManRes_t * p;
    unsigned * pData;
    int i, k;
    assert( sizeof(unsigned) == 4 );
    p = ABC_ALLOC( Abc_ManRes_t, 1 );
    memset( p, 0, sizeof(Abc_ManRes_t) );
    p->nLeavesMax = nLeavesMax;
    p->nDivsMax   = nDivsMax;
    p->vDivs      = Vec_PtrAlloc( p->nDivsMax );
    // allocate simulation info
    p->nBits      = (1 << p->nLeavesMax);
    p->nWords     = (p->nBits <= 32)? 1 : (p->nBits / 32);
    p->pInfo      = ABC_ALLOC( unsigned, p->nWords * (p->nDivsMax + 1) );
    memset( p->pInfo, 0, sizeof(unsigned) * p->nWords * p->nLeavesMax );
    p->vSims      = Vec_PtrAlloc( p->nDivsMax );
    for ( i = 0; i < p->nDivsMax; i++ )
        Vec_PtrPush( p->vSims, p->pInfo + i * p->nWords );
    // assign the care set
    p->pCareSet  = p->pInfo + p->nDivsMax * p->nWords;
    Abc_InfoFill( p->pCareSet, p->nWords );
    // set elementary truth tables
    for ( k = 0; k < p->nLeavesMax; k++ )
    {
        pData = (unsigned *)p->vSims->pArray[k];
        for ( i = 0; i < p->nBits; i++ )
            if ( i & (1 << k) )
                pData[i>>5] |= (1 << (i&31));
    }
    // create the remaining divisors
    p->vDivs1UP  = Vec_PtrAlloc( p->nDivsMax );
    p->vDivs1UN  = Vec_PtrAlloc( p->nDivsMax );
    p->vDivs1B   = Vec_PtrAlloc( p->nDivsMax );
    p->vDivs2UP0 = Vec_PtrAlloc( p->nDivsMax );
    p->vDivs2UP1 = Vec_PtrAlloc( p->nDivsMax );
    p->vDivs2UN0 = Vec_PtrAlloc( p->nDivsMax );
    p->vDivs2UN1 = Vec_PtrAlloc( p->nDivsMax );
    p->vTemp     = Vec_PtrAlloc( p->nDivsMax );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ManResubStop( Abc_ManRes_t * p )
{
    Vec_PtrFree( p->vDivs );
    Vec_PtrFree( p->vSims );
    Vec_PtrFree( p->vDivs1UP );
    Vec_PtrFree( p->vDivs1UN );
    Vec_PtrFree( p->vDivs1B );
    Vec_PtrFree( p->vDivs2UP0 );
    Vec_PtrFree( p->vDivs2UP1 );
    Vec_PtrFree( p->vDivs2UN0 );
    Vec_PtrFree( p->vDivs2UN1 );
    Vec_PtrFree( p->vTemp );
    ABC_FREE( p->pInfo );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ManResubPrint( Abc_ManRes_t * p )
{
    printf( "Used constants    = %6d.             ", p->nUsedNodeC );          ABC_PRT( "Cuts  ", p->timeCut );
    printf( "Used replacements = %6d.             ", p->nUsedNode0 );          ABC_PRT( "Resub ", p->timeRes );
    printf( "Used single ORs   = %6d.             ", p->nUsedNode1Or );        ABC_PRT( " Div  ", p->timeDiv );
    printf( "Used single ANDs  = %6d.             ", p->nUsedNode1And );       ABC_PRT( " Mffc ", p->timeMffc );
    printf( "Used double ORs   = %6d.             ", p->nUsedNode2Or );        ABC_PRT( " Sim  ", p->timeSim );
    printf( "Used double ANDs  = %6d.             ", p->nUsedNode2And );       ABC_PRT( " 1    ", p->timeRes1 );
    printf( "Used OR-AND       = %6d.             ", p->nUsedNode2OrAnd );     ABC_PRT( " D    ", p->timeResD );
    printf( "Used AND-OR       = %6d.             ", p->nUsedNode2AndOr );     ABC_PRT( " 2    ", p->timeRes2 );
    printf( "Used OR-2ANDs     = %6d.             ", p->nUsedNode3OrAnd );     ABC_PRT( "Truth ", p->timeTruth ); //ABC_PRT( " 3    ", p->timeRes3 );
    printf( "Used AND-2ORs     = %6d.             ", p->nUsedNode3AndOr );     ABC_PRT( "AIG   ", p->timeNtk );
    printf( "TOTAL             = %6d.             ", p->nUsedNodeC +
                                                     p->nUsedNode0 +
                                                     p->nUsedNode1Or +
                                                     p->nUsedNode1And +
                                                     p->nUsedNode2Or +
                                                     p->nUsedNode2And +
                                                     p->nUsedNode2OrAnd +
                                                     p->nUsedNode2AndOr +
                                                     p->nUsedNode3OrAnd +
                                                     p->nUsedNode3AndOr
                                                   );                          ABC_PRT( "TOTAL ", p->timeTotal );
    printf( "Total leaves   = %8d.\n", p->nTotalLeaves );
    printf( "Total divisors = %8d.\n", p->nTotalDivs );
//    printf( "Total gain     = %8d.\n", p->nTotalGain );
    printf( "Gain           = %8d. (%6.2f %%).\n", p->nNodesBeg-p->nNodesEnd, 100.0*(p->nNodesBeg-p->nNodesEnd)/p->nNodesBeg );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void Abc_ManResubCollectDivs_rec1( Abc_Obj_t * pNode, Vec_Ptr_t * vInternal )
{
    // skip visited nodes
    if ( Abc_NodeIsTravIdCurrent(pNode) )
        return;
    Abc_NodeSetTravIdCurrent(pNode);
    // collect the fanins
    Abc_ManResubCollectDivs_rec1( Abc_ObjFanin0(pNode), vInternal );
    Abc_ManResubCollectDivs_rec1( Abc_ObjFanin1(pNode), vInternal );
    // collect the internal node
    if ( pNode->fMarkA == 0 ) 
        Vec_PtrPush( vInternal, pNode );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ManResubCollectDivs( Abc_ManRes_t * p, Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves, int Required )
{
    Abc_Obj_t * pNode, * pFanout;
    int i, k, Limit, Counter;

    Vec_PtrClear( p->vDivs1UP );
    Vec_PtrClear( p->vDivs1UN );
    Vec_PtrClear( p->vDivs1B );

    // add the leaves of the cuts to the divisors
    Vec_PtrClear( p->vDivs );
    Abc_NtkIncrementTravId( pRoot->pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pNode, i )
    {
        Vec_PtrPush( p->vDivs, pNode );
        Abc_NodeSetTravIdCurrent( pNode );        
    }

    // mark nodes in the MFFC
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vTemp, pNode, i )
        pNode->fMarkA = 1;
    // collect the cone (without MFFC)
    Abc_ManResubCollectDivs_rec1( pRoot, p->vDivs );
    // unmark the current MFFC
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vTemp, pNode, i )
        pNode->fMarkA = 0;

    // check if the number of divisors is not exceeded
    if ( Vec_PtrSize(p->vDivs) - Vec_PtrSize(vLeaves) + Vec_PtrSize(p->vTemp) >= Vec_PtrSize(p->vSims) - p->nLeavesMax )
        return 0;

    // get the number of divisors to collect
    Limit = Vec_PtrSize(p->vSims) - p->nLeavesMax - (Vec_PtrSize(p->vDivs) - Vec_PtrSize(vLeaves) + Vec_PtrSize(p->vTemp));

    // explore the fanouts, which are not in the MFFC
    Counter = 0;
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs, pNode, i )
    {
        if ( Abc_ObjFanoutNum(pNode) > 100 )
        {
//            printf( "%d ", Abc_ObjFanoutNum(pNode) );
            continue;
        }
        // if the fanout has both fanins in the set, add it
        Abc_ObjForEachFanout( pNode, pFanout, k )
        {
            if ( Abc_NodeIsTravIdCurrent(pFanout) || Abc_ObjIsCo(pFanout) || (int)pFanout->Level > Required )
                continue;
            if ( Abc_NodeIsTravIdCurrent(Abc_ObjFanin0(pFanout)) && Abc_NodeIsTravIdCurrent(Abc_ObjFanin1(pFanout)) )
            {
                if ( Abc_ObjFanin0(pFanout) == pRoot || Abc_ObjFanin1(pFanout) == pRoot )
                    continue;
                Vec_PtrPush( p->vDivs, pFanout );
                Abc_NodeSetTravIdCurrent( pFanout );
                // quit computing divisors if there is too many of them
                if ( ++Counter == Limit )
                    goto Quits;
            }
        }
    }

Quits :
    // get the number of divisors
    p->nDivs = Vec_PtrSize(p->vDivs);

    // add the nodes in the MFFC
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vTemp, pNode, i )
        Vec_PtrPush( p->vDivs, pNode );
    assert( pRoot == Vec_PtrEntryLast(p->vDivs) );

    assert( Vec_PtrSize(p->vDivs) - Vec_PtrSize(vLeaves) <= Vec_PtrSize(p->vSims) - p->nLeavesMax );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ManResubPrintDivs( Abc_ManRes_t * p, Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves )
{
    Abc_Obj_t * pFanin, * pNode;
    int i, k;
    // print the nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs, pNode, i )
    {
        if ( i < Vec_PtrSize(vLeaves) )
        {
            printf( "%6d : %c\n", pNode->Id, 'a'+i );
            continue;
        }
        printf( "%6d : %2d = ", pNode->Id, i );
        // find the first fanin
        Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs, pFanin, k )
            if ( Abc_ObjFanin0(pNode) == pFanin )
                break;
        if ( k < Vec_PtrSize(vLeaves) )
            printf( "%c", 'a' + k );
        else
            printf( "%d", k );
        printf( "%s ", Abc_ObjFaninC0(pNode)? "\'" : "" );
        // find the second fanin
        Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs, pFanin, k )
            if ( Abc_ObjFanin1(pNode) == pFanin )
                break;
        if ( k < Vec_PtrSize(vLeaves) )
            printf( "%c", 'a' + k );
        else
            printf( "%d", k );
        printf( "%s ", Abc_ObjFaninC1(pNode)? "\'" : "" );
        if ( pNode == pRoot )
            printf( " root" );
        printf( "\n" );
    }
    printf( "\n" );
}


/**Function*************************************************************

  Synopsis    [Performs simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ManResubSimulate( Vec_Ptr_t * vDivs, int nLeaves, Vec_Ptr_t * vSims, int nLeavesMax, int nWords )
{
    Abc_Obj_t * pObj;
    unsigned * puData0, * puData1, * puData;
    int i, k;
    assert( Vec_PtrSize(vDivs) - nLeaves <= Vec_PtrSize(vSims) - nLeavesMax );
    // simulate
    Vec_PtrForEachEntry( Abc_Obj_t *, vDivs, pObj, i )
    {
        if ( i < nLeaves )
        { // initialize the leaf
            pObj->pData = Vec_PtrEntry( vSims, i );
            continue;
        }
        // set storage for the node's simulation info
        pObj->pData = Vec_PtrEntry( vSims, i - nLeaves + nLeavesMax );
        // get pointer to the simulation info
        puData  = (unsigned *)pObj->pData;
        puData0 = (unsigned *)Abc_ObjFanin0(pObj)->pData;
        puData1 = (unsigned *)Abc_ObjFanin1(pObj)->pData;
        // simulate
        if ( Abc_ObjFaninC0(pObj) && Abc_ObjFaninC1(pObj) )
            for ( k = 0; k < nWords; k++ )
                puData[k] = ~puData0[k] & ~puData1[k];
        else if ( Abc_ObjFaninC0(pObj) )
            for ( k = 0; k < nWords; k++ )
                puData[k] = ~puData0[k] & puData1[k];
        else if ( Abc_ObjFaninC1(pObj) )
            for ( k = 0; k < nWords; k++ )
                puData[k] = puData0[k] & ~puData1[k];
        else 
            for ( k = 0; k < nWords; k++ )
                puData[k] = puData0[k] & puData1[k];
    }
    // normalize
    Vec_PtrForEachEntry( Abc_Obj_t *, vDivs, pObj, i )
    {
        puData = (unsigned *)pObj->pData;
        pObj->fPhase = (puData[0] & 1);
        if ( pObj->fPhase )
            for ( k = 0; k < nWords; k++ )
                puData[k] = ~puData[k];
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

Dec_Graph_t * Abc_ManResubQuit0_1( Abc_Obj_t * pRoot, Abc_Obj_t * pObj )
{
    Dec_Graph_t * pGraph;
    Dec_Edge_t eRoot;
    pGraph = Dec_GraphCreate( 1 );
    Dec_GraphNode( pGraph, 0 )->pFunc = pObj;
    eRoot = Dec_EdgeCreate( 0, pObj->fPhase );
    Dec_GraphSetRoot( pGraph, eRoot );
    if ( pRoot->fPhase )
        Dec_GraphComplement( pGraph );
    return pGraph;
}
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

Dec_Graph_t * Abc_ManResubQuit1_1( Abc_Obj_t * pRoot, Abc_Obj_t * pObj0, Abc_Obj_t * pObj1, int fOrGate )
{
    Dec_Graph_t * pGraph;
    Dec_Edge_t eRoot, eNode0, eNode1;
    assert( Abc_ObjRegular(pObj0) != Abc_ObjRegular(pObj1) );
    pGraph = Dec_GraphCreate( 2 );
    Dec_GraphNode( pGraph, 0 )->pFunc = Abc_ObjRegular(pObj0);
    Dec_GraphNode( pGraph, 1 )->pFunc = Abc_ObjRegular(pObj1);
    eNode0 = Dec_EdgeCreate( 0, Abc_ObjRegular(pObj0)->fPhase ^ Abc_ObjIsComplement(pObj0) );
    eNode1 = Dec_EdgeCreate( 1, Abc_ObjRegular(pObj1)->fPhase ^ Abc_ObjIsComplement(pObj1) );
    if ( fOrGate ) 
        eRoot  = Dec_GraphAddNodeOr( pGraph, eNode0, eNode1 );
    else
        eRoot  = Dec_GraphAddNodeAnd( pGraph, eNode0, eNode1 );
    Dec_GraphSetRoot( pGraph, eRoot );
    if ( pRoot->fPhase )
        Dec_GraphComplement( pGraph );
    return pGraph;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

Dec_Graph_t * Abc_ManResubQuit21_1( Abc_Obj_t * pRoot, Abc_Obj_t * pObj0, Abc_Obj_t * pObj1, Abc_Obj_t * pObj2, int fOrGate )
{
    Dec_Graph_t * pGraph;
    Dec_Edge_t eRoot, eNode0, eNode1, eNode2;
    assert( Abc_ObjRegular(pObj0) != Abc_ObjRegular(pObj1) );
    assert( Abc_ObjRegular(pObj0) != Abc_ObjRegular(pObj2) );
    assert( Abc_ObjRegular(pObj1) != Abc_ObjRegular(pObj2) );
    pGraph = Dec_GraphCreate( 3 );
    Dec_GraphNode( pGraph, 0 )->pFunc = Abc_ObjRegular(pObj0);
    Dec_GraphNode( pGraph, 1 )->pFunc = Abc_ObjRegular(pObj1);
    Dec_GraphNode( pGraph, 2 )->pFunc = Abc_ObjRegular(pObj2);
    eNode0 = Dec_EdgeCreate( 0, Abc_ObjRegular(pObj0)->fPhase ^ Abc_ObjIsComplement(pObj0) );
    eNode1 = Dec_EdgeCreate( 1, Abc_ObjRegular(pObj1)->fPhase ^ Abc_ObjIsComplement(pObj1) );
    eNode2 = Dec_EdgeCreate( 2, Abc_ObjRegular(pObj2)->fPhase ^ Abc_ObjIsComplement(pObj2) );
    if ( fOrGate ) 
    {
        eRoot  = Dec_GraphAddNodeOr( pGraph, eNode0, eNode1 );
        eRoot  = Dec_GraphAddNodeOr( pGraph, eNode2, eRoot );
    }
    else
    {
        eRoot  = Dec_GraphAddNodeAnd( pGraph, eNode0, eNode1 );
        eRoot  = Dec_GraphAddNodeAnd( pGraph, eNode2, eRoot );
    }
    Dec_GraphSetRoot( pGraph, eRoot );
    if ( pRoot->fPhase )
        Dec_GraphComplement( pGraph );
    return pGraph;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

Dec_Graph_t * Abc_ManResubQuit2_1( Abc_Obj_t * pRoot, Abc_Obj_t * pObj0, Abc_Obj_t * pObj1, Abc_Obj_t * pObj2, int fOrGate )
{
    Dec_Graph_t * pGraph;
    Dec_Edge_t eRoot, ePrev, eNode0, eNode1, eNode2;
    assert( Abc_ObjRegular(pObj0) != Abc_ObjRegular(pObj1) );
    assert( Abc_ObjRegular(pObj0) != Abc_ObjRegular(pObj2) );
    assert( Abc_ObjRegular(pObj1) != Abc_ObjRegular(pObj2) );
    pGraph = Dec_GraphCreate( 3 );
    Dec_GraphNode( pGraph, 0 )->pFunc = Abc_ObjRegular(pObj0);
    Dec_GraphNode( pGraph, 1 )->pFunc = Abc_ObjRegular(pObj1);
    Dec_GraphNode( pGraph, 2 )->pFunc = Abc_ObjRegular(pObj2);
    eNode0 = Dec_EdgeCreate( 0, Abc_ObjRegular(pObj0)->fPhase ^ Abc_ObjIsComplement(pObj0) );
    if ( Abc_ObjIsComplement(pObj1) && Abc_ObjIsComplement(pObj2) )
    {
        eNode1 = Dec_EdgeCreate( 1, Abc_ObjRegular(pObj1)->fPhase );
        eNode2 = Dec_EdgeCreate( 2, Abc_ObjRegular(pObj2)->fPhase );
        ePrev  = Dec_GraphAddNodeOr( pGraph, eNode1, eNode2 );
    }
    else
    {
        eNode1 = Dec_EdgeCreate( 1, Abc_ObjRegular(pObj1)->fPhase ^ Abc_ObjIsComplement(pObj1) );
        eNode2 = Dec_EdgeCreate( 2, Abc_ObjRegular(pObj2)->fPhase ^ Abc_ObjIsComplement(pObj2) );
        ePrev  = Dec_GraphAddNodeAnd( pGraph, eNode1, eNode2 );
    }
    if ( fOrGate ) 
        eRoot  = Dec_GraphAddNodeOr( pGraph, eNode0, ePrev );
    else
        eRoot  = Dec_GraphAddNodeAnd( pGraph, eNode0, ePrev );
    Dec_GraphSetRoot( pGraph, eRoot );
    if ( pRoot->fPhase )
        Dec_GraphComplement( pGraph );
    return pGraph;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

Dec_Graph_t * Abc_ManResubQuit3_1( Abc_Obj_t * pRoot, Abc_Obj_t * pObj0, Abc_Obj_t * pObj1, Abc_Obj_t * pObj2, Abc_Obj_t * pObj3, int fOrGate )
{
    Dec_Graph_t * pGraph;
    Dec_Edge_t eRoot, ePrev0, ePrev1, eNode0, eNode1, eNode2, eNode3;
    assert( Abc_ObjRegular(pObj0) != Abc_ObjRegular(pObj1) );
    assert( Abc_ObjRegular(pObj2) != Abc_ObjRegular(pObj3) );
    pGraph = Dec_GraphCreate( 4 );
    Dec_GraphNode( pGraph, 0 )->pFunc = Abc_ObjRegular(pObj0);
    Dec_GraphNode( pGraph, 1 )->pFunc = Abc_ObjRegular(pObj1);
    Dec_GraphNode( pGraph, 2 )->pFunc = Abc_ObjRegular(pObj2);
    Dec_GraphNode( pGraph, 3 )->pFunc = Abc_ObjRegular(pObj3);
    if ( Abc_ObjIsComplement(pObj0) && Abc_ObjIsComplement(pObj1) )
    {
        eNode0 = Dec_EdgeCreate( 0, Abc_ObjRegular(pObj0)->fPhase );
        eNode1 = Dec_EdgeCreate( 1, Abc_ObjRegular(pObj1)->fPhase );
        ePrev0 = Dec_GraphAddNodeOr( pGraph, eNode0, eNode1 );
        if ( Abc_ObjIsComplement(pObj2) && Abc_ObjIsComplement(pObj3) )
        {
            eNode2 = Dec_EdgeCreate( 2, Abc_ObjRegular(pObj2)->fPhase );
            eNode3 = Dec_EdgeCreate( 3, Abc_ObjRegular(pObj3)->fPhase );
            ePrev1 = Dec_GraphAddNodeOr( pGraph, eNode2, eNode3 );
        }
        else
        {
            eNode2 = Dec_EdgeCreate( 2, Abc_ObjRegular(pObj2)->fPhase ^ Abc_ObjIsComplement(pObj2) );
            eNode3 = Dec_EdgeCreate( 3, Abc_ObjRegular(pObj3)->fPhase ^ Abc_ObjIsComplement(pObj3) );
            ePrev1 = Dec_GraphAddNodeAnd( pGraph, eNode2, eNode3 );
        }
    }
    else
    {
        eNode0 = Dec_EdgeCreate( 0, Abc_ObjRegular(pObj0)->fPhase ^ Abc_ObjIsComplement(pObj0) );
        eNode1 = Dec_EdgeCreate( 1, Abc_ObjRegular(pObj1)->fPhase ^ Abc_ObjIsComplement(pObj1) );
        ePrev0 = Dec_GraphAddNodeAnd( pGraph, eNode0, eNode1 );
        if ( Abc_ObjIsComplement(pObj2) && Abc_ObjIsComplement(pObj3) )
        {
            eNode2 = Dec_EdgeCreate( 2, Abc_ObjRegular(pObj2)->fPhase );
            eNode3 = Dec_EdgeCreate( 3, Abc_ObjRegular(pObj3)->fPhase );
            ePrev1 = Dec_GraphAddNodeOr( pGraph, eNode2, eNode3 );
        }
        else
        {
            eNode2 = Dec_EdgeCreate( 2, Abc_ObjRegular(pObj2)->fPhase ^ Abc_ObjIsComplement(pObj2) );
            eNode3 = Dec_EdgeCreate( 3, Abc_ObjRegular(pObj3)->fPhase ^ Abc_ObjIsComplement(pObj3) );
            ePrev1 = Dec_GraphAddNodeAnd( pGraph, eNode2, eNode3 );
        }
    }
    if ( fOrGate ) 
        eRoot = Dec_GraphAddNodeOr( pGraph, ePrev0, ePrev1 );
    else
        eRoot = Dec_GraphAddNodeAnd( pGraph, ePrev0, ePrev1 );
    Dec_GraphSetRoot( pGraph, eRoot );
    if ( pRoot->fPhase )
        Dec_GraphComplement( pGraph );
    return pGraph;
}




/**Function*************************************************************

  Synopsis    [Derives single-node unate/binate divisors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ManResubDivsS( Abc_ManRes_t * p, int Required )
{
    int fMoreDivs = 1; // bug fix by Siang-Yun Lee
    Abc_Obj_t * pObj;
    unsigned * puData, * puDataR;
    int i, w;
    Vec_PtrClear( p->vDivs1UP );
    Vec_PtrClear( p->vDivs1UN );
    Vec_PtrClear( p->vDivs1B );
    puDataR = (unsigned *)p->pRoot->pData;
    Vec_PtrForEachEntryStop( Abc_Obj_t *, p->vDivs, pObj, i, p->nDivs )
    {
        if ( (int)pObj->Level > Required - 1 )
            continue;

        puData = (unsigned *)pObj->pData;
        // check positive containment
        for ( w = 0; w < p->nWords; w++ )
//            if ( puData[w] & ~puDataR[w] )
            if ( puData[w] & ~puDataR[w] & p->pCareSet[w] ) // care set
                break;
        if ( w == p->nWords )
        {
            Vec_PtrPush( p->vDivs1UP, pObj );
            continue;
        }
        if ( fMoreDivs )
        {
            for ( w = 0; w < p->nWords; w++ )
    //            if ( ~puData[w] & ~puDataR[w] )
                if ( ~puData[w] & ~puDataR[w] & p->pCareSet[w] ) // care set
                    break;
            if ( w == p->nWords )
            {
                Vec_PtrPush( p->vDivs1UP, Abc_ObjNot(pObj) );
                continue;
            }
        }
        // check negative containment
        for ( w = 0; w < p->nWords; w++ )
//            if ( ~puData[w] & puDataR[w] )
            if ( ~puData[w] & puDataR[w] & p->pCareSet[w] ) // care set
                break;
        if ( w == p->nWords )
        {
            Vec_PtrPush( p->vDivs1UN, pObj );
            continue;
        }
        if ( fMoreDivs )
        {
            for ( w = 0; w < p->nWords; w++ )
    //            if ( puData[w] & puDataR[w] )
                if ( puData[w] & puDataR[w] & p->pCareSet[w] ) // care set
                    break;
            if ( w == p->nWords )
            {
                Vec_PtrPush( p->vDivs1UN, Abc_ObjNot(pObj) );
                continue;
            }
        }
        // add the node to binates
        Vec_PtrPush( p->vDivs1B, pObj );
    }
}

/**Function*************************************************************

  Synopsis    [Derives double-node unate/binate divisors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ManResubDivsD( Abc_ManRes_t * p, int Required )
{
    Abc_Obj_t * pObj0, * pObj1;
    unsigned * puData0, * puData1, * puDataR;
    int i, k, w;
    Vec_PtrClear( p->vDivs2UP0 );
    Vec_PtrClear( p->vDivs2UP1 );
    Vec_PtrClear( p->vDivs2UN0 );
    Vec_PtrClear( p->vDivs2UN1 );
    puDataR = (unsigned *)p->pRoot->pData;
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs1B, pObj0, i )
    {
        if ( (int)pObj0->Level > Required - 2 )
            continue;

        puData0 = (unsigned *)pObj0->pData;
        Vec_PtrForEachEntryStart( Abc_Obj_t *, p->vDivs1B, pObj1, k, i + 1 )
        {
            if ( (int)pObj1->Level > Required - 2 )
                continue;

            puData1 = (unsigned *)pObj1->pData;

            if ( Vec_PtrSize(p->vDivs2UP0) < ABC_RS_DIV2_MAX )
            {
                // get positive unate divisors
                for ( w = 0; w < p->nWords; w++ )
//                    if ( (puData0[w] & puData1[w]) & ~puDataR[w] )
                    if ( (puData0[w] & puData1[w]) & ~puDataR[w] & p->pCareSet[w] ) // care set
                        break;
                if ( w == p->nWords )
                {
                    Vec_PtrPush( p->vDivs2UP0, pObj0 );
                    Vec_PtrPush( p->vDivs2UP1, pObj1 );
                }
                for ( w = 0; w < p->nWords; w++ )
//                    if ( (~puData0[w] & puData1[w]) & ~puDataR[w] )
                    if ( (~puData0[w] & puData1[w]) & ~puDataR[w] & p->pCareSet[w] ) // care set
                        break;
                if ( w == p->nWords )
                {
                    Vec_PtrPush( p->vDivs2UP0, Abc_ObjNot(pObj0) );
                    Vec_PtrPush( p->vDivs2UP1, pObj1 );
                }
                for ( w = 0; w < p->nWords; w++ )
//                    if ( (puData0[w] & ~puData1[w]) & ~puDataR[w] )
                    if ( (puData0[w] & ~puData1[w]) & ~puDataR[w] & p->pCareSet[w] ) // care set
                        break;
                if ( w == p->nWords )
                {
                    Vec_PtrPush( p->vDivs2UP0, pObj0 );
                    Vec_PtrPush( p->vDivs2UP1, Abc_ObjNot(pObj1) );
                }
                for ( w = 0; w < p->nWords; w++ )
//                    if ( (puData0[w] | puData1[w]) & ~puDataR[w] )
                    if ( (puData0[w] | puData1[w]) & ~puDataR[w] & p->pCareSet[w] ) // care set
                        break;
                if ( w == p->nWords )
                {
                    Vec_PtrPush( p->vDivs2UP0, Abc_ObjNot(pObj0) );
                    Vec_PtrPush( p->vDivs2UP1, Abc_ObjNot(pObj1) );
                }
            }

            if ( Vec_PtrSize(p->vDivs2UN0) < ABC_RS_DIV2_MAX )
            {
                // get negative unate divisors
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ~(puData0[w] & puData1[w]) & puDataR[w] )
                    if ( ~(puData0[w] & puData1[w]) & puDataR[w] & p->pCareSet[w] ) // care set
                        break;
                if ( w == p->nWords )
                {
                    Vec_PtrPush( p->vDivs2UN0, pObj0 );
                    Vec_PtrPush( p->vDivs2UN1, pObj1 );
                }
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ~(~puData0[w] & puData1[w]) & puDataR[w] )
                    if ( ~(~puData0[w] & puData1[w]) & puDataR[w] & p->pCareSet[w] ) // care set
                        break;
                if ( w == p->nWords )
                {
                    Vec_PtrPush( p->vDivs2UN0, Abc_ObjNot(pObj0) );
                    Vec_PtrPush( p->vDivs2UN1, pObj1 );
                }
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ~(puData0[w] & ~puData1[w]) & puDataR[w] )
                    if ( ~(puData0[w] & ~puData1[w]) & puDataR[w] & p->pCareSet[w] ) // care set
                        break;
                if ( w == p->nWords )
                {
                    Vec_PtrPush( p->vDivs2UN0, pObj0 );
                    Vec_PtrPush( p->vDivs2UN1, Abc_ObjNot(pObj1) );
                }
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ~(puData0[w] | puData1[w]) & puDataR[w] )
                    if ( ~(puData0[w] | puData1[w]) & puDataR[w] & p->pCareSet[w] ) // care set
                        break;
                if ( w == p->nWords )
                {
                    Vec_PtrPush( p->vDivs2UN0, Abc_ObjNot(pObj0) );
                    Vec_PtrPush( p->vDivs2UN1, Abc_ObjNot(pObj1) );
                }
            }
        }
    }
//    printf( "%d %d  ", Vec_PtrSize(p->vDivs2UP0), Vec_PtrSize(p->vDivs2UN0) );
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Abc_ManResubQuit( Abc_ManRes_t * p )
{
    Dec_Graph_t * pGraph;
    unsigned * upData;
    int w;
    upData = (unsigned *)p->pRoot->pData;
    for ( w = 0; w < p->nWords; w++ )
//        if ( upData[w] )
        if ( upData[w] & p->pCareSet[w] ) // care set
            break;
    if ( w != p->nWords )
        return NULL;
    // get constant node graph
    if ( p->pRoot->fPhase )
        pGraph = Dec_GraphCreateConst1();
    else 
        pGraph = Dec_GraphCreateConst0();
    return pGraph;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Abc_ManResubDivs0( Abc_ManRes_t * p )
{
    Abc_Obj_t * pObj;
    unsigned * puData, * puDataR;
    int i, w;
    puDataR = (unsigned *)p->pRoot->pData;
    Vec_PtrForEachEntryStop( Abc_Obj_t *, p->vDivs, pObj, i, p->nDivs )
    {
        puData = (unsigned *)pObj->pData;
        for ( w = 0; w < p->nWords; w++ )
//            if ( puData[w] != puDataR[w] )
            if ( (puData[w] ^ puDataR[w]) & p->pCareSet[w] ) // care set
                break;
        if ( w == p->nWords )
            return Abc_ManResubQuit0_1( p->pRoot, pObj );
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Abc_ManResubDivs1( Abc_ManRes_t * p, int Required )
{
    Abc_Obj_t * pObj0, * pObj1;
    unsigned * puData0, * puData1, * puDataR;
    int i, k, w;
    puDataR = (unsigned *)p->pRoot->pData;
    // check positive unate divisors
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs1UP, pObj0, i )
    {
        puData0 = (unsigned *)Abc_ObjRegular(pObj0)->pData;
        Vec_PtrForEachEntryStart( Abc_Obj_t *, p->vDivs1UP, pObj1, k, i + 1 )
        {
            puData1 = (unsigned *)Abc_ObjRegular(pObj1)->pData;
            if ( Abc_ObjIsComplement(pObj0) && Abc_ObjIsComplement(pObj1) )
            {
                for ( w = 0; w < p->nWords; w++ )
    //                if ( (puData0[w] | puData1[w]) != puDataR[w] )
                    if ( ((~puData0[w] | ~puData1[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
            }
            else if ( Abc_ObjIsComplement(pObj0) )
            {
                for ( w = 0; w < p->nWords; w++ )
    //                if ( (puData0[w] | puData1[w]) != puDataR[w] )
                    if ( ((~puData0[w] | puData1[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
            }
            else if ( Abc_ObjIsComplement(pObj1) )
            {
                for ( w = 0; w < p->nWords; w++ )
    //                if ( (puData0[w] | puData1[w]) != puDataR[w] )
                    if ( ((puData0[w] | ~puData1[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
            }
            else 
            {
                for ( w = 0; w < p->nWords; w++ )
    //                if ( (puData0[w] | puData1[w]) != puDataR[w] )
                    if ( ((puData0[w] | puData1[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
            }
            if ( w == p->nWords )
            {
                p->nUsedNode1Or++;
                return Abc_ManResubQuit1_1( p->pRoot, pObj0, pObj1, 1 );
            }
        }
    }
    // check negative unate divisors
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs1UN, pObj0, i )
    {
        puData0 = (unsigned *)Abc_ObjRegular(pObj0)->pData;
        Vec_PtrForEachEntryStart( Abc_Obj_t *, p->vDivs1UN, pObj1, k, i + 1 )
        {
            puData1 = (unsigned *)Abc_ObjRegular(pObj1)->pData;
            if ( Abc_ObjIsComplement(pObj0) && Abc_ObjIsComplement(pObj1) )
            {
                for ( w = 0; w < p->nWords; w++ )
    //                if ( (puData0[w] & puData1[w]) != puDataR[w] )
                    if ( ((~puData0[w] & ~puData1[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
            }
            if ( Abc_ObjIsComplement(pObj0) )
            {
                for ( w = 0; w < p->nWords; w++ )
    //                if ( (puData0[w] & puData1[w]) != puDataR[w] )
                    if ( ((~puData0[w] & puData1[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
            }
            if ( Abc_ObjIsComplement(pObj1) )
            {
                for ( w = 0; w < p->nWords; w++ )
    //                if ( (puData0[w] & puData1[w]) != puDataR[w] )
                    if ( ((puData0[w] & ~puData1[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
            }
            else
            {
                for ( w = 0; w < p->nWords; w++ )
    //                if ( (puData0[w] & puData1[w]) != puDataR[w] )
                    if ( ((puData0[w] & puData1[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
            }
            if ( w == p->nWords )
            {
                p->nUsedNode1And++;
                return Abc_ManResubQuit1_1( p->pRoot, pObj0, pObj1, 0 );
            }
        }
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Abc_ManResubDivs12( Abc_ManRes_t * p, int Required )
{
    Abc_Obj_t * pObj0, * pObj1, * pObj2, * pObjMax, * pObjMin0 = NULL, * pObjMin1 = NULL;
    unsigned * puData0, * puData1, * puData2, * puDataR;
    int i, k, j, w, LevelMax;
    puDataR = (unsigned *)p->pRoot->pData;
    // check positive unate divisors
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs1UP, pObj0, i )
    {
        puData0 = (unsigned *)Abc_ObjRegular(pObj0)->pData;
        Vec_PtrForEachEntryStart( Abc_Obj_t *, p->vDivs1UP, pObj1, k, i + 1 )
        {
            puData1 = (unsigned *)Abc_ObjRegular(pObj1)->pData;
            Vec_PtrForEachEntryStart( Abc_Obj_t *, p->vDivs1UP, pObj2, j, k + 1 )
            {
                puData2 = (unsigned *)Abc_ObjRegular(pObj2)->pData;
                if ( Abc_ObjIsComplement(pObj0) && Abc_ObjIsComplement(pObj1) && Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] | puData1[w] | puData2[w]) != puDataR[w] )
                        if ( ((~puData0[w] | ~puData1[w] | ~puData2[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( Abc_ObjIsComplement(pObj0) && Abc_ObjIsComplement(pObj1) && !Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] | puData1[w] | puData2[w]) != puDataR[w] )
                        if ( ((~puData0[w] | ~puData1[w] | puData2[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( Abc_ObjIsComplement(pObj0) && !Abc_ObjIsComplement(pObj1) && Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] | puData1[w] | puData2[w]) != puDataR[w] )
                        if ( ((~puData0[w] | puData1[w] | ~puData2[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( Abc_ObjIsComplement(pObj0) && !Abc_ObjIsComplement(pObj1) && !Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] | puData1[w] | puData2[w]) != puDataR[w] )
                        if ( ((~puData0[w] | puData1[w] | puData2[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( !Abc_ObjIsComplement(pObj0) && Abc_ObjIsComplement(pObj1) && Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] | puData1[w] | puData2[w]) != puDataR[w] )
                        if ( ((puData0[w] | ~puData1[w] | ~puData2[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( !Abc_ObjIsComplement(pObj0) && Abc_ObjIsComplement(pObj1) && !Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] | puData1[w] | puData2[w]) != puDataR[w] )
                        if ( ((puData0[w] | ~puData1[w] | puData2[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( !Abc_ObjIsComplement(pObj0) && !Abc_ObjIsComplement(pObj1) && Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] | puData1[w] | puData2[w]) != puDataR[w] )
                        if ( ((puData0[w] | puData1[w] | ~puData2[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( !Abc_ObjIsComplement(pObj0) && !Abc_ObjIsComplement(pObj1) && !Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] | puData1[w] | puData2[w]) != puDataR[w] )
                        if ( ((puData0[w] | puData1[w] | puData2[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else assert( 0 );
                if ( w == p->nWords )
                {
                    LevelMax = Abc_MaxInt( Abc_ObjRegular(pObj0)->Level, Abc_MaxInt(Abc_ObjRegular(pObj1)->Level, Abc_ObjRegular(pObj2)->Level) );
                    assert( LevelMax <= Required - 1 );

                    pObjMax = NULL;
                    if ( (int)Abc_ObjRegular(pObj0)->Level == LevelMax )
                        pObjMax = pObj0, pObjMin0 = pObj1, pObjMin1 = pObj2;
                    if ( (int)Abc_ObjRegular(pObj1)->Level == LevelMax )
                    {
                        if ( pObjMax ) continue;
                        pObjMax = pObj1, pObjMin0 = pObj0, pObjMin1 = pObj2;
                    }
                    if ( (int)Abc_ObjRegular(pObj2)->Level == LevelMax )
                    {
                        if ( pObjMax ) continue;
                        pObjMax = pObj2, pObjMin0 = pObj0, pObjMin1 = pObj1;
                    }

                    p->nUsedNode2Or++;
                    assert(pObjMin0);
                    assert(pObjMin1);
                    return Abc_ManResubQuit21_1( p->pRoot, pObjMin0, pObjMin1, pObjMax, 1 );
                }
            }
        }
    }
    // check negative unate divisors
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs1UN, pObj0, i )
    {
        puData0 = (unsigned *)Abc_ObjRegular(pObj0)->pData;
        Vec_PtrForEachEntryStart( Abc_Obj_t *, p->vDivs1UN, pObj1, k, i + 1 )
        {
            puData1 = (unsigned *)Abc_ObjRegular(pObj1)->pData;
            Vec_PtrForEachEntryStart( Abc_Obj_t *, p->vDivs1UN, pObj2, j, k + 1 )
            {
                puData2 = (unsigned *)Abc_ObjRegular(pObj2)->pData;
                if ( Abc_ObjIsComplement(pObj0) && Abc_ObjIsComplement(pObj1) && Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] & puData1[w] & puData2[w]) != puDataR[w] )
                        if ( ((~puData0[w] & ~puData1[w] & ~puData2[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( Abc_ObjIsComplement(pObj0) && Abc_ObjIsComplement(pObj1) && !Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] & puData1[w] & puData2[w]) != puDataR[w] )
                        if ( ((~puData0[w] & ~puData1[w] & puData2[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( Abc_ObjIsComplement(pObj0) && !Abc_ObjIsComplement(pObj1) && Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] & puData1[w] & puData2[w]) != puDataR[w] )
                        if ( ((~puData0[w] & puData1[w] & ~puData2[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( Abc_ObjIsComplement(pObj0) && !Abc_ObjIsComplement(pObj1) && !Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] & puData1[w] & puData2[w]) != puDataR[w] )
                        if ( ((~puData0[w] & puData1[w] & puData2[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( !Abc_ObjIsComplement(pObj0) && Abc_ObjIsComplement(pObj1) && Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] & puData1[w] & puData2[w]) != puDataR[w] )
                        if ( ((puData0[w] & ~puData1[w] & ~puData2[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( !Abc_ObjIsComplement(pObj0) && Abc_ObjIsComplement(pObj1) && !Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] & puData1[w] & puData2[w]) != puDataR[w] )
                        if ( ((puData0[w] & ~puData1[w] & puData2[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( !Abc_ObjIsComplement(pObj0) && !Abc_ObjIsComplement(pObj1) && Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] & puData1[w] & puData2[w]) != puDataR[w] )
                        if ( ((puData0[w] & puData1[w] & ~puData2[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( !Abc_ObjIsComplement(pObj0) && !Abc_ObjIsComplement(pObj1) && !Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] & puData1[w] & puData2[w]) != puDataR[w] )
                        if ( ((puData0[w] & puData1[w] & puData2[w]) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else assert( 0 );
                if ( w == p->nWords )
                {
                    LevelMax = Abc_MaxInt( Abc_ObjRegular(pObj0)->Level, Abc_MaxInt(Abc_ObjRegular(pObj1)->Level, Abc_ObjRegular(pObj2)->Level) );
                    assert( LevelMax <= Required - 1 );

                    pObjMax = NULL;
                    if ( (int)Abc_ObjRegular(pObj0)->Level == LevelMax )
                        pObjMax = pObj0, pObjMin0 = pObj1, pObjMin1 = pObj2;
                    if ( (int)Abc_ObjRegular(pObj1)->Level == LevelMax )
                    {
                        if ( pObjMax ) continue;
                        pObjMax = pObj1, pObjMin0 = pObj0, pObjMin1 = pObj2;
                    }
                    if ( (int)Abc_ObjRegular(pObj2)->Level == LevelMax )
                    {
                        if ( pObjMax ) continue;
                        pObjMax = pObj2, pObjMin0 = pObj0, pObjMin1 = pObj1;
                    }

                    p->nUsedNode2And++;
                    assert(pObjMin0);
                    assert(pObjMin1);
                    return Abc_ManResubQuit21_1( p->pRoot, pObjMin0, pObjMin1, pObjMax, 0 );
                }
            }
        }
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Abc_ManResubDivs2( Abc_ManRes_t * p, int Required )
{
    Abc_Obj_t * pObj0, * pObj1, * pObj2;
    unsigned * puData0, * puData1, * puData2, * puDataR;
    int i, k, w;
    puDataR = (unsigned *)p->pRoot->pData;
    // check positive unate divisors
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs1UP, pObj0, i )
    {
        puData0 = (unsigned *)Abc_ObjRegular(pObj0)->pData;
        Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs2UP0, pObj1, k )
        {
            pObj2 = (Abc_Obj_t *)Vec_PtrEntry( p->vDivs2UP1, k );

            puData1 = (unsigned *)Abc_ObjRegular(pObj1)->pData;
            puData2 = (unsigned *)Abc_ObjRegular(pObj2)->pData;
            if ( Abc_ObjIsComplement(pObj0) )
            {
                if ( Abc_ObjIsComplement(pObj1) && Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] | (puData1[w] | puData2[w])) != puDataR[w] )
                        if ( ((~puData0[w] | (puData1[w] | puData2[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( Abc_ObjIsComplement(pObj1) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] | (~puData1[w] & puData2[w])) != puDataR[w] )
                        if ( ((~puData0[w] | (~puData1[w] & puData2[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] | (puData1[w] & ~puData2[w])) != puDataR[w] )
                        if ( ((~puData0[w] | (puData1[w] & ~puData2[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else 
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] | (puData1[w] & puData2[w])) != puDataR[w] )
                        if ( ((~puData0[w] | (puData1[w] & puData2[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
            }
            else
            {
                if ( Abc_ObjIsComplement(pObj1) && Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] | (puData1[w] | puData2[w])) != puDataR[w] )
                        if ( ((puData0[w] | (puData1[w] | puData2[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( Abc_ObjIsComplement(pObj1) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] | (~puData1[w] & puData2[w])) != puDataR[w] )
                        if ( ((puData0[w] | (~puData1[w] & puData2[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] | (puData1[w] & ~puData2[w])) != puDataR[w] )
                        if ( ((puData0[w] | (puData1[w] & ~puData2[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else 
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] | (puData1[w] & puData2[w])) != puDataR[w] )
                        if ( ((puData0[w] | (puData1[w] & puData2[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
            }
            if ( w == p->nWords )
            {
                p->nUsedNode2OrAnd++;
                return Abc_ManResubQuit2_1( p->pRoot, pObj0, pObj1, pObj2, 1 );
            }
        }
    }
    // check negative unate divisors
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs1UN, pObj0, i )
    {
        puData0 = (unsigned *)Abc_ObjRegular(pObj0)->pData;
        Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs2UN0, pObj1, k )
        {
            pObj2 = (Abc_Obj_t *)Vec_PtrEntry( p->vDivs2UN1, k );

            puData1 = (unsigned *)Abc_ObjRegular(pObj1)->pData;
            puData2 = (unsigned *)Abc_ObjRegular(pObj2)->pData;
            if ( Abc_ObjIsComplement(pObj0) )
            {
                if ( Abc_ObjIsComplement(pObj1) && Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] & (puData1[w] | puData2[w])) != puDataR[w] )
                        if ( ((~puData0[w] & (puData1[w] | puData2[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( Abc_ObjIsComplement(pObj1) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] & (~puData1[w] & puData2[w])) != puDataR[w] )
                        if ( ((~puData0[w] & (~puData1[w] & puData2[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] & (puData1[w] & ~puData2[w])) != puDataR[w] )
                        if ( ((~puData0[w] & (puData1[w] & ~puData2[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else 
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] & (puData1[w] & puData2[w])) != puDataR[w] )
                        if ( ((~puData0[w] & (puData1[w] & puData2[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
            }
            else
            {
                if ( Abc_ObjIsComplement(pObj1) && Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] & (puData1[w] | puData2[w])) != puDataR[w] )
                        if ( ((puData0[w] & (puData1[w] | puData2[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( Abc_ObjIsComplement(pObj1) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] & (~puData1[w] & puData2[w])) != puDataR[w] )
                        if ( ((puData0[w] & (~puData1[w] & puData2[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else if ( Abc_ObjIsComplement(pObj2) )
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] & (puData1[w] & ~puData2[w])) != puDataR[w] )
                        if ( ((puData0[w] & (puData1[w] & ~puData2[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
                else 
                {
                    for ( w = 0; w < p->nWords; w++ )
    //                    if ( (puData0[w] & (puData1[w] & puData2[w])) != puDataR[w] )
                        if ( ((puData0[w] & (puData1[w] & puData2[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                            break;
                }
            }
            if ( w == p->nWords )
            {
                p->nUsedNode2AndOr++;
                return Abc_ManResubQuit2_1( p->pRoot, pObj0, pObj1, pObj2, 0 );
            }
        }
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Abc_ManResubDivs3( Abc_ManRes_t * p, int Required )
{
    Abc_Obj_t * pObj0, * pObj1, * pObj2, * pObj3;
    unsigned * puData0, * puData1, * puData2, * puData3, * puDataR;
    int i, k, w = 0, Flag;
    puDataR = (unsigned *)p->pRoot->pData;
    // check positive unate divisors
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs2UP0, pObj0, i )
    {
        pObj1 = (Abc_Obj_t *)Vec_PtrEntry( p->vDivs2UP1, i );
        puData0 = (unsigned *)Abc_ObjRegular(pObj0)->pData;
        puData1 = (unsigned *)Abc_ObjRegular(pObj1)->pData;
        Flag = (Abc_ObjIsComplement(pObj0) << 3) | (Abc_ObjIsComplement(pObj1) << 2);

        Vec_PtrForEachEntryStart( Abc_Obj_t *, p->vDivs2UP0, pObj2, k, i + 1 )
        {
            pObj3 = (Abc_Obj_t *)Vec_PtrEntry( p->vDivs2UP1, k );
            puData2 = (unsigned *)Abc_ObjRegular(pObj2)->pData;
            puData3 = (unsigned *)Abc_ObjRegular(pObj3)->pData;

            Flag = (Flag & 12) | ((int)Abc_ObjIsComplement(pObj2) << 1) | (int)Abc_ObjIsComplement(pObj3);
            assert( Flag < 16 );
            switch( Flag )
            {
            case 0: // 0000
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ((puData0[w] & puData1[w]) | (puData2[w] & puData3[w])) != puDataR[w] )
                    if ( (((puData0[w] & puData1[w]) | (puData2[w] & puData3[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
                break;
            case 1: // 0001
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ((puData0[w] & puData1[w]) | (puData2[w] & ~puData3[w])) != puDataR[w] )
                    if ( (((puData0[w] & puData1[w]) | (puData2[w] & ~puData3[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
                break;
            case 2: // 0010
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ((puData0[w] & puData1[w]) | (~puData2[w] & puData3[w])) != puDataR[w] )
                    if ( (((puData0[w] & puData1[w]) | (~puData2[w] & puData3[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
                break;
            case 3: // 0011
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ((puData0[w] & puData1[w]) | (puData2[w] | puData3[w])) != puDataR[w] )
                    if ( (((puData0[w] & puData1[w]) | (puData2[w] | puData3[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
                break;

            case 4: // 0100
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ((puData0[w] & ~puData1[w]) | (puData2[w] & puData3[w])) != puDataR[w] )
                    if ( (((puData0[w] & ~puData1[w]) | (puData2[w] & puData3[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
                break;
            case 5: // 0101
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ((puData0[w] & ~puData1[w]) | (puData2[w] & ~puData3[w])) != puDataR[w] )
                    if ( (((puData0[w] & ~puData1[w]) | (puData2[w] & ~puData3[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
                break;
            case 6: // 0110
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ((puData0[w] & ~puData1[w]) | (~puData2[w] & puData3[w])) != puDataR[w] )
                    if ( (((puData0[w] & ~puData1[w]) | (~puData2[w] & puData3[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
                break;
            case 7: // 0111
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ((puData0[w] & ~puData1[w]) | (puData2[w] | puData3[w])) != puDataR[w] )
                    if ( (((puData0[w] & ~puData1[w]) | (puData2[w] | puData3[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
                break;

            case 8: // 1000
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ((~puData0[w] & puData1[w]) | (puData2[w] & puData3[w])) != puDataR[w] )
                    if ( (((~puData0[w] & puData1[w]) | (puData2[w] & puData3[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
                break;
            case 9: // 1001
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ((~puData0[w] & puData1[w]) | (puData2[w] & ~puData3[w])) != puDataR[w] )
                    if ( (((~puData0[w] & puData1[w]) | (puData2[w] & ~puData3[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
                break;
            case 10: // 1010
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ((~puData0[w] & puData1[w]) | (~puData2[w] & puData3[w])) != puDataR[w] )
                    if ( (((~puData0[w] & puData1[w]) | (~puData2[w] & puData3[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
                break;
            case 11: // 1011
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ((~puData0[w] & puData1[w]) | (puData2[w] | puData3[w])) != puDataR[w] )
                    if ( (((~puData0[w] & puData1[w]) | (puData2[w] | puData3[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
                break;

            case 12: // 1100
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ((puData0[w] | puData1[w]) | (puData2[w] & puData3[w])) != puDataR[w] )
                    if ( (((puData0[w] | puData1[w]) | (puData2[w] & puData3[w])) ^ puDataR[w]) & p->pCareSet[w] ) // care set
                        break;
                break;
            case 13: // 1101
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ((puData0[w] | puData1[w]) | (puData2[w] & ~puData3[w])) != puDataR[w] )
                    if ( (((puData0[w] | puData1[w]) | (puData2[w] & ~puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;
            case 14: // 1110
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ((puData0[w] | puData1[w]) | (~puData2[w] & puData3[w])) != puDataR[w] )
                    if ( (((puData0[w] | puData1[w]) | (~puData2[w] & puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;
            case 15: // 1111
                for ( w = 0; w < p->nWords; w++ )
//                    if ( ((puData0[w] | puData1[w]) | (puData2[w] | puData3[w])) != puDataR[w] )
                    if ( (((puData0[w] | puData1[w]) | (puData2[w] | puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;

            }
            if ( w == p->nWords )
            {
                p->nUsedNode3OrAnd++;
                return Abc_ManResubQuit3_1( p->pRoot, pObj0, pObj1, pObj2, pObj3, 1 );
            }
        }
    }

/*
    // check negative unate divisors
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs2UN0, pObj0, i )
    {
        pObj1 = Vec_PtrEntry( p->vDivs2UN1, i );
        puData0 = Abc_ObjRegular(pObj0)->pData;
        puData1 = Abc_ObjRegular(pObj1)->pData;
        Flag = (Abc_ObjIsComplement(pObj0) << 3) | (Abc_ObjIsComplement(pObj1) << 2);

        Vec_PtrForEachEntryStart( Abc_Obj_t *, p->vDivs2UN0, pObj2, k, i + 1 )
        {
            pObj3 = Vec_PtrEntry( p->vDivs2UN1, k );
            puData2 = Abc_ObjRegular(pObj2)->pData;
            puData3 = Abc_ObjRegular(pObj3)->pData;

            Flag = (Flag & 12) | (Abc_ObjIsComplement(pObj2) << 1) | Abc_ObjIsComplement(pObj3);
            assert( Flag < 16 );
            switch( Flag )
            {
            case 0: // 0000
                for ( w = 0; w < p->nWords; w++ )
                    if ( (((puData0[w] & puData1[w]) & (puData2[w] & puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;
            case 1: // 0001
                for ( w = 0; w < p->nWords; w++ )
                    if ( (((puData0[w] & puData1[w]) & (puData2[w] & ~puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;
            case 2: // 0010
                for ( w = 0; w < p->nWords; w++ )
                    if ( (((puData0[w] & puData1[w]) & (~puData2[w] & puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;
            case 3: // 0011
                for ( w = 0; w < p->nWords; w++ )
                    if ( (((puData0[w] & puData1[w]) & (puData2[w] | puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;

            case 4: // 0100
                for ( w = 0; w < p->nWords; w++ )
                    if ( (((puData0[w] & ~puData1[w]) & (puData2[w] & puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;
            case 5: // 0101
                for ( w = 0; w < p->nWords; w++ )
                    if ( (((puData0[w] & ~puData1[w]) & (puData2[w] & ~puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;
            case 6: // 0110
                for ( w = 0; w < p->nWords; w++ )
                    if ( (((puData0[w] & ~puData1[w]) & (~puData2[w] & puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;
            case 7: // 0111
                for ( w = 0; w < p->nWords; w++ )
                    if ( (((puData0[w] & ~puData1[w]) & (puData2[w] | puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;

            case 8: // 1000
                for ( w = 0; w < p->nWords; w++ )
                    if ( (((~puData0[w] & puData1[w]) & (puData2[w] & puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;
            case 9: // 1001
                for ( w = 0; w < p->nWords; w++ )
                    if ( (((~puData0[w] & puData1[w]) & (puData2[w] & ~puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;
            case 10: // 1010
                for ( w = 0; w < p->nWords; w++ )
                    if ( (((~puData0[w] & puData1[w]) & (~puData2[w] & puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;
            case 11: // 1011
                for ( w = 0; w < p->nWords; w++ )
                    if ( (((~puData0[w] & puData1[w]) & (puData2[w] | puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;

            case 12: // 1100
                for ( w = 0; w < p->nWords; w++ )
                    if ( (((puData0[w] | puData1[w]) & (puData2[w] & puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;
            case 13: // 1101
                for ( w = 0; w < p->nWords; w++ )
                    if ( (((puData0[w] | puData1[w]) & (puData2[w] & ~puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;
            case 14: // 1110
                for ( w = 0; w < p->nWords; w++ )
                    if ( (((puData0[w] | puData1[w]) & (~puData2[w] & puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;
            case 15: // 1111
                for ( w = 0; w < p->nWords; w++ )
                    if ( (((puData0[w] | puData1[w]) & (puData2[w] | puData3[w])) ^ puDataR[w]) & p->pCareSet[w] )
                        break;
                break;

            }
            if ( w == p->nWords )
            {
                p->nUsedNode3AndOr++;
                return Abc_ManResubQuit3_1( p->pRoot, pObj0, pObj1, pObj2, pObj3, 0 );
            }
        }
    }
*/
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ManResubCleanup( Abc_ManRes_t * p )
{
    Abc_Obj_t * pObj;
    int i;
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs, pObj, i )
        pObj->pData = NULL;
    Vec_PtrClear( p->vDivs );
    p->pRoot = NULL;
}

/**Function*************************************************************

  Synopsis    [Evaluates resubstution of one cut.]

  Description [Returns the graph to add if any.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Abc_ManResubEval( Abc_ManRes_t * p, Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves, int nSteps, int fUpdateLevel, int fVerbose )
{
    extern int Abc_NodeMffcInside( Abc_Obj_t * pNode, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vInside );
    Dec_Graph_t * pGraph;
    int Required;
    abctime clk;

    Required = fUpdateLevel? Abc_ObjRequiredLevel(pRoot) : ABC_INFINITY;

    assert( nSteps >= 0 );
    assert( nSteps <= 3 );
    p->pRoot = pRoot;
    p->nLeaves = Vec_PtrSize(vLeaves);
    p->nLastGain = -1;

    // collect the MFFC
clk = Abc_Clock();
    p->nMffc = Abc_NodeMffcInside( pRoot, vLeaves, p->vTemp );
p->timeMffc += Abc_Clock() - clk;
    assert( p->nMffc > 0 );

    // collect the divisor nodes
clk = Abc_Clock();
    if ( !Abc_ManResubCollectDivs( p, pRoot, vLeaves, Required ) )
        return NULL;
    p->timeDiv += Abc_Clock() - clk;

    p->nTotalDivs   += p->nDivs;
    p->nTotalLeaves += p->nLeaves;

    // simulate the nodes
clk = Abc_Clock();
    Abc_ManResubSimulate( p->vDivs, p->nLeaves, p->vSims, p->nLeavesMax, p->nWords );
p->timeSim += Abc_Clock() - clk;

clk = Abc_Clock();
    // consider constants
    if ( (pGraph = Abc_ManResubQuit( p )) )
    {
        p->nUsedNodeC++;
        p->nLastGain = p->nMffc;
        return pGraph;
    }

    // consider equal nodes
    if ( (pGraph = Abc_ManResubDivs0( p )) )
    {
p->timeRes1 += Abc_Clock() - clk;
        p->nUsedNode0++;
        p->nLastGain = p->nMffc;
        return pGraph;
    }
    if ( nSteps == 0 || p->nMffc == 1 )
    {
p->timeRes1 += Abc_Clock() - clk;
        return NULL;
    }

    // get the one level divisors
    Abc_ManResubDivsS( p, Required );

    // consider one node
    if ( (pGraph = Abc_ManResubDivs1( p, Required )) )
    {
p->timeRes1 += Abc_Clock() - clk;
        p->nLastGain = p->nMffc - 1;
        return pGraph;
    }
p->timeRes1 += Abc_Clock() - clk;
    if ( nSteps == 1 || p->nMffc == 2 )
        return NULL;

clk = Abc_Clock();
    // consider triples
    if ( (pGraph = Abc_ManResubDivs12( p, Required )) )
    {
p->timeRes2 += Abc_Clock() - clk;
        p->nLastGain = p->nMffc - 2;
        return pGraph;
    }
p->timeRes2 += Abc_Clock() - clk;

    // get the two level divisors
clk = Abc_Clock();
    Abc_ManResubDivsD( p, Required );
p->timeResD += Abc_Clock() - clk;

    // consider two nodes
clk = Abc_Clock();
    if ( (pGraph = Abc_ManResubDivs2( p, Required )) )
    {
p->timeRes2 += Abc_Clock() - clk;
        p->nLastGain = p->nMffc - 2;
        return pGraph;
    }
p->timeRes2 += Abc_Clock() - clk;
    if ( nSteps == 2 || p->nMffc == 3 )
        return NULL;

    // consider two nodes
clk = Abc_Clock();
    if ( (pGraph = Abc_ManResubDivs3( p, Required )) )
    {
p->timeRes3 += Abc_Clock() - clk;
        p->nLastGain = p->nMffc - 3;
        return pGraph;
    }
p->timeRes3 += Abc_Clock() - clk;
    if ( nSteps == 3 || p->nLeavesMax == 4 )
        return NULL;
    return NULL;
}




/**Function*************************************************************

  Synopsis    [Computes the volume and checks if the cut is feasible.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

int Abc_CutVolumeCheck_rec_1( Abc_Obj_t * pObj )
{
    // quit if the node is visited (or if it is a leaf)
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return 0;
    Abc_NodeSetTravIdCurrent(pObj);
    // report the error
    if ( Abc_ObjIsCi(pObj) )
        printf( "Abc_CutVolumeCheck() ERROR: The set of nodes is not a cut!\n" );
    // count the number of nodes in the leaves
    return 1 + Abc_CutVolumeCheck_rec_1( Abc_ObjFanin0(pObj) ) +
        Abc_CutVolumeCheck_rec_1( Abc_ObjFanin1(pObj) );
}
/**Function*************************************************************

  Synopsis    [Computes the volume and checks if the cut is feasible.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_CutVolumeCheck( Abc_Obj_t * pNode, Vec_Ptr_t * vLeaves )
{
    Abc_Obj_t * pObj;
    int i;
    // mark the leaves
    Abc_NtkIncrementTravId( pNode->pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pObj, i )
        Abc_NodeSetTravIdCurrent( pObj ); 
    // traverse the nodes starting from the given one and count them
    return Abc_CutVolumeCheck_rec_1( pNode );
}

/**Function*************************************************************

  Synopsis    [Computes the factor cut of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void Abc_CutFactor_rec_1( Abc_Obj_t * pObj, Vec_Ptr_t * vLeaves )
{
    if ( pObj->fMarkA )
        return;
    if ( Abc_ObjIsCi(pObj) || (Abc_ObjFanoutNum(pObj) > 1 && !Abc_NodeIsMuxControlType(pObj)) )
    {
        Vec_PtrPush( vLeaves, pObj );
        pObj->fMarkA = 1;
        return;
    }
    Abc_CutFactor_rec_1( Abc_ObjFanin0(pObj), vLeaves );
    Abc_CutFactor_rec_1( Abc_ObjFanin1(pObj), vLeaves );
}


/**Function*************************************************************

  Synopsis    [Computes the factor cut of the node.]

  Description [Factor-cut is the cut at a node in terms of factor-nodes.
  Factor-nodes are roots of the node trees (MUXes/EXORs are counted as single nodes).
  Factor-cut is unique for the given node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

Vec_Ptr_t * Abc_CutFactor_1( Abc_Obj_t * pNode )
{
    Vec_Ptr_t * vLeaves;
    Abc_Obj_t * pObj;
    int i;
    assert( !Abc_ObjIsCi(pNode) );
    vLeaves  = Vec_PtrAlloc( 10 );
    Abc_CutFactor_rec_1( Abc_ObjFanin0(pNode), vLeaves );
    Abc_CutFactor_rec_1( Abc_ObjFanin1(pNode), vLeaves );
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pObj, i )
        pObj->fMarkA = 0;
    return vLeaves;
}


/**Function*************************************************************

  Synopsis    [Cut computation.]

  Description [This cut computation works as follows: 
  It starts with the factor cut at the node. If the factor-cut is large, quit.
  It supports the set of leaves of the cut under construction and labels all nodes
  in the cut under construction, including the leaves.
  It computes the factor-cuts of the leaves and checks if it is easible to add any of them.
  If it is, it randomly chooses one feasible and continues.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_CutFactorLarge( Abc_Obj_t * pNode, int nLeavesMax )
{
    Vec_Ptr_t * vLeaves, * vFactors, * vFact, * vNext;
    Vec_Int_t * vFeasible;
    Abc_Obj_t * pLeaf, * pTemp;
    int i, k, Counter, RandLeaf;
    int BestCut, BestShare;
    assert( Abc_ObjIsNode(pNode) );
    // get one factor-cut
    vLeaves = Abc_CutFactor_1( pNode );
    if ( Vec_PtrSize(vLeaves) > nLeavesMax )
    {
        Vec_PtrFree(vLeaves);
        return NULL;
    }
    if ( Vec_PtrSize(vLeaves) == nLeavesMax )
        return vLeaves;
    // initialize the factor cuts for the leaves
    vFactors = Vec_PtrAlloc( nLeavesMax );
    Abc_NtkIncrementTravId( pNode->pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pLeaf, i )
    {
        Abc_NodeSetTravIdCurrent( pLeaf ); 
        if ( Abc_ObjIsCi(pLeaf) )
            Vec_PtrPush( vFactors, NULL );
        else
            Vec_PtrPush( vFactors, Abc_CutFactor_1(pLeaf) );
    }
    // construct larger factor cuts
    vFeasible = Vec_IntAlloc( nLeavesMax );
    while ( 1 )
    {
        BestCut = -1, BestShare = -1;
        // find the next feasible cut to add
        Vec_IntClear( vFeasible );
        Vec_PtrForEachEntry( Vec_Ptr_t *, vFactors, vFact, i )
        {
            if ( vFact == NULL )
                continue;
            // count the number of unmarked leaves of this factor cut
            Counter = 0;
            Vec_PtrForEachEntry( Abc_Obj_t *, vFact, pTemp, k )
                Counter += !Abc_NodeIsTravIdCurrent(pTemp);
            // if the number of new leaves is smaller than the diff, it is feasible
            if ( Counter <= nLeavesMax - Vec_PtrSize(vLeaves) + 1 )
            {
                Vec_IntPush( vFeasible, i );
                if ( BestCut == -1 || BestShare < Vec_PtrSize(vFact) - Counter )
                    BestCut = i, BestShare = Vec_PtrSize(vFact) - Counter;
            }
        }
        // quit if there is no feasible factor cuts
        if ( Vec_IntSize(vFeasible) == 0 )
            break;
        // randomly choose one leaf and get its factor cut
//        RandLeaf = Vec_IntEntry( vFeasible, rand() % Vec_IntSize(vFeasible) );
        // choose the cut that has most sharing with the other cuts
        RandLeaf = BestCut;

        pLeaf = (Abc_Obj_t *)Vec_PtrEntry( vLeaves, RandLeaf );
        vNext = (Vec_Ptr_t *)Vec_PtrEntry( vFactors, RandLeaf );
        // unmark this leaf
        Abc_NodeSetTravIdPrevious( pLeaf ); 
        // remove this cut from the leaves and factor cuts
        for ( i = RandLeaf; i < Vec_PtrSize(vLeaves)-1; i++ )
        {
            Vec_PtrWriteEntry( vLeaves,  i, Vec_PtrEntry(vLeaves, i+1) );
            Vec_PtrWriteEntry( vFactors, i, Vec_PtrEntry(vFactors,i+1) );
        }
        Vec_PtrShrink( vLeaves,  Vec_PtrSize(vLeaves) -1 );
        Vec_PtrShrink( vFactors, Vec_PtrSize(vFactors)-1 );
        // add new leaves, compute their factor cuts
        Vec_PtrForEachEntry( Abc_Obj_t *, vNext, pLeaf, i )
        {
            if ( Abc_NodeIsTravIdCurrent(pLeaf) )
                continue;
            Abc_NodeSetTravIdCurrent( pLeaf ); 
            Vec_PtrPush( vLeaves, pLeaf );
            if ( Abc_ObjIsCi(pLeaf) )
                Vec_PtrPush( vFactors, NULL );
            else
                Vec_PtrPush( vFactors, Abc_CutFactor_1(pLeaf) );
        }
        Vec_PtrFree( vNext );
        assert( Vec_PtrSize(vLeaves) <= nLeavesMax );
        if ( Vec_PtrSize(vLeaves) == nLeavesMax )
            break;
    }

    // remove temporary storage
    Vec_PtrForEachEntry( Vec_Ptr_t *, vFactors, vFact, i )
        if ( vFact ) Vec_PtrFree( vFact );
    Vec_PtrFree( vFactors );
    Vec_IntFree( vFeasible );
    return vLeaves;
}


int Abc_NtkOrchSA( Abc_Ntk_t * pNtk, Vec_Int_t **pGain_rwr, Vec_Int_t **pGain_res,Vec_Int_t **pGain_ref, Vec_Int_t **PolicyList, char * DecisionFile, int fUseZeros_rwr, int fUseZeros_ref, int fPlaceEnable, int nCutMax, int nStepsMax, int nLevelsOdc, int fUpdateLevel, int fVerbose, int fVeryVerbose, int nNodeSizeMax, int nConeSizeMax, int fUseDcs )
{
    ProgressBar * pProgress;
    // For resub
    Abc_ManRes_t * pManRes;
    Abc_ManCut_t * pManCutRes;
    Odc_Man_t * pManOdc = NULL;
    Dec_Graph_t * pFFormRes;
    Vec_Ptr_t * vLeaves;
    // For rewrite
    Cut_Man_t * pManCutRwr;
    Rwr_Man_t * pManRwr;
    Dec_Graph_t * pGraph;
    // For refactor
    Abc_ManRef_t * pManRef;
    Abc_ManCut_t * pManCutRef;
    Dec_Graph_t * pFFormRef;
    Vec_Ptr_t * vFanins;
    Vec_Int_t * DecisionMask = Vec_IntAlloc(1);

    Abc_Obj_t * pNode;
    FILE *fpt;
    abctime clk, clkStart = Abc_Clock();
    abctime s_ResubTime;
    int i, nNodes, nGain, fCompl, RetValue = 1;
    int ops_rwr = 0;
    int ops_res = 0;
    int ops_ref = 0;
    int ops_null = 0;
    int sOpsOrder = 0;
    assert( Abc_NtkIsStrash(pNtk) );

    // cleanup the AIG
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);

    // start the managers resub
    pManCutRes = Abc_NtkManCutStart( nCutMax, 100000, 100000, 100000 );
    pManRes = Abc_ManResubStart( nCutMax, ABC_RS_DIV1_MAX );
    if ( nLevelsOdc > 0 )
    pManOdc = Abc_NtkDontCareAlloc( nCutMax, nLevelsOdc, fVerbose, fVeryVerbose );
    // start the managers refactor
    pManCutRef = Abc_NtkManCutStart( nNodeSizeMax, nConeSizeMax, 2, 1000 );
    pManRef = Abc_NtkManRefStart_1( nNodeSizeMax, nConeSizeMax, fUseDcs, fVerbose );
    pManRef->vLeaves   = Abc_NtkManCutReadCutLarge( pManCutRef );
    // start the managers rewrite
    pManRwr = Rwr_ManStart( 0 );
    if ( pManRwr == NULL )
        return 0;

    // compute the reverse levels if level update is requested
    if ( fUpdateLevel )
        Abc_NtkStartReverseLevels( pNtk, 0 );
    if ( Abc_NtkLatchNum(pNtk) ) {
        Abc_NtkForEachLatch(pNtk, pNode, i)
            pNode->pNext = (Abc_Obj_t *)pNode->pData;
    }
clk = Abc_Clock();
    pManCutRwr = Abc_NtkStartCutManForRewrite( pNtk );
Rwr_ManAddTimeCuts( pManRwr, Abc_Clock() - clk );
    pNtk->pManCut = pManCutRwr;

    if ( fVeryVerbose )
        Rwr_ScoresClean( pManRwr );

    pManRes->nNodesBeg = Abc_NtkNodeNum(pNtk);
    pManRwr->nNodesBeg = Abc_NtkNodeNum(pNtk);
    pManRef->nNodesBeg = Abc_NtkNodeNum(pNtk);

    nNodes = Abc_NtkObjNumMax(pNtk);
    //printf("nNodes: %d\n", nNodes);
    if (pGain_res) *pGain_res = Vec_IntAlloc(1);
    if (pGain_ref) *pGain_ref = Vec_IntAlloc(1);
    if (pGain_rwr) *pGain_rwr = Vec_IntAlloc(1);
    for(int i=0; i < nNodes; i++){Vec_IntPush(DecisionMask, atoi("-1"));}

    pProgress = Extra_ProgressBarStart( stdout, nNodes );
    fpt = fopen(DecisionFile, "w");

    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        int iterNode = pNode->Id;
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        if ( Abc_NodeIsPersistant(pNode) )
        {
            //fprintf(fpt, "%d, %d\n", iterNode, -99);
            Vec_IntPush((*pGain_res), -99);
            Vec_IntPush((*pGain_ref), -99);
            Vec_IntPush((*pGain_rwr), -99);
            continue;
        }
        if ( Abc_ObjFanoutNum(pNode) > 1000 )
        {
            //fprintf(fpt, "%d, %d\n", iterNode, -99);
            Vec_IntPush((*pGain_res), -99);
            Vec_IntPush((*pGain_ref), -99);
            Vec_IntPush((*pGain_rwr), -99);
            continue;
        }
        if ( i >= nNodes )
            break;

//refactor
clk = Abc_Clock();
        vFanins = Abc_NodeFindCut( pManCutRef, pNode, fUseDcs );
pManRef->timeCut += Abc_Clock() - clk;
clk = Abc_Clock();
        pFFormRef = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros_ref, fUseDcs, fVerbose );
pManRef->timeRes += Abc_Clock() - clk;
        Vec_IntPush((*pGain_ref), pManRef->nLastGain);
//resub
        vLeaves = Abc_NodeFindCut( pManCutRes, pNode, 0 );
pManRes->timeCut += Abc_Clock() - clk;
        if ( pManOdc )
        {
clk = Abc_Clock();
            Abc_NtkDontCareClear( pManOdc );
            Abc_NtkDontCareCompute( pManOdc, pNode, vLeaves, pManRes->pCareSet );
pManRes->timeTruth += Abc_Clock() - clk;
        }
clk = Abc_Clock();
        pFFormRes = Abc_ManResubEval( pManRes, pNode, vLeaves, nStepsMax, fUpdateLevel, fVerbose );
pManRes->timeRes += Abc_Clock() - clk;
        Vec_IntPush((*pGain_res), pManRes->nLastGain);
//rewrite
        nGain = Rwr_NodeRewrite( pManRwr, pManCutRwr, pNode, fUpdateLevel, fUseZeros_rwr, fPlaceEnable );
        Vec_IntPush( (*pGain_rwr), nGain);
        //fprintf(fpt, "%d, %s, %d, %s, %d, %s, %d\n", pNode->Id, "Oches_Res", pManRes->nLastGain, "Oches_Ref", pManRef->nLastGain, "Oches_Rwr", nGain);

      if ((**PolicyList).pArray[iterNode] == 0){sOpsOrder = 0;}
      if ((**PolicyList).pArray[iterNode] == 1){sOpsOrder = 1;}
      if ((**PolicyList).pArray[iterNode] == 2){sOpsOrder = 2;}
      if ((**PolicyList).pArray[iterNode] == 3){sOpsOrder = 3;}
      if ((**PolicyList).pArray[iterNode] == 4){sOpsOrder = 4;}
      if ((**PolicyList).pArray[iterNode] == 5){sOpsOrder = 5;}

     if ( sOpsOrder == 0)
      {
        //printf("policy orchestration with o1.");
        if (nGain > 0 || (nGain == 0 && fUseZeros_rwr))
        {
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        ops_rwr++;
        //fprintf(fpt, "%d, %d\n", iterNode, 0);
        (DecisionMask)->pArray[iterNode] = 0;
        continue;
        }

        if (pManRes->nLastGain > 0)
        {

        if ( pFFormRes == NULL )
            continue;
        pManRes->nTotalGain += pManRes->nLastGain;
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pFFormRes, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRes );
        ops_res++;
        //fprintf(fpt, "%d, %d\n", iterNode, 1);
        (DecisionMask)->pArray[iterNode] = 2;
        continue;
        }

        if (pManRef->nLastGain > 0 || (pManRef->nLastGain ==0 && fUseZeros_ref))
        {
        if ( pFFormRef == NULL )
            continue;
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef );
        ops_ref++;
        //fprintf(fpt, "%d, %d\n", iterNode, 2);
        (DecisionMask)->pArray[iterNode] = 3;
        continue;
        }
        if (! (nGain > 0 || pManRef->nLastGain > 0 || pManRes->nLastGain > 0 || (nGain == 0 && fUseZeros_rwr)))
        {
        ops_null++;
        //fprintf(fpt, "%d, %d\n", iterNode, -1);
        continue;
        }
      }

      if ( sOpsOrder == 1)
      {
        //printf("policy orchestration with o2.");
        if (nGain > 0 || (nGain == 0 && fUseZeros_rwr))
        {
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        ops_rwr++;
        //fprintf(fpt, "%d, %d\n", iterNode, 0);
        (DecisionMask)->pArray[iterNode] = 0;
        continue;
        }

        if (pManRef->nLastGain > 0 || (pManRef->nLastGain ==0 && fUseZeros_ref))
        {
        if ( pFFormRef == NULL )
            continue;
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef );
        ops_ref++;
        //fprintf(fpt, "%d, %d\n", iterNode, 2);
        (DecisionMask)->pArray[iterNode] = 3;
        continue;
        }

        if (pManRes->nLastGain > 0)
        {
        if ( pFFormRes == NULL )
            continue;
        pManRes->nTotalGain += pManRes->nLastGain;
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pFFormRes, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk; 
        Dec_GraphFree( pFFormRes );
        ops_res++;
        //fprintf(fpt, "%d, %d\n", iterNode, 1);
        (DecisionMask)->pArray[iterNode] = 2;
        continue;
        }
        if (! (nGain > 0 || pManRef->nLastGain > 0 || pManRes->nLastGain > 0 || (nGain == 0 && fUseZeros_rwr)))
        {
        ops_null++;
        //fprintf(fpt, "%d, %d\n", iterNode, -1);
        continue;
        }
      } 
      
      if ( sOpsOrder == 2)
      {
        //printf("policy orchestration with o3.");
        if (pManRes->nLastGain > 0)
        {
        if ( pFFormRes == NULL )
            continue;
        pManRes->nTotalGain += pManRes->nLastGain;
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pFFormRes, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRes );
        ops_res++;
        //fprintf(fpt, "%d, %d\n", iterNode, 1);
        (DecisionMask)->pArray[iterNode] = 2;
        continue;
        }

        if (nGain > 0 || (nGain == 0 && fUseZeros_rwr))
        {
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        ops_rwr++;
        //fprintf(fpt, "%d, %d\n", iterNode, 0);
        (DecisionMask)->pArray[iterNode] = 0;
        continue;
        }

        if (pManRef->nLastGain > 0 || (pManRef->nLastGain ==0 && fUseZeros_ref))
        {
        if ( pFFormRef == NULL )
            continue;
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef );
        ops_ref++;
        //fprintf(fpt, "%d, %d\n", iterNode, 2);
        (DecisionMask)->pArray[iterNode] = 3;
        continue;
        }
        if (! (nGain > 0 || pManRef->nLastGain > 0 || pManRes->nLastGain > 0 || (nGain == 0 && fUseZeros_rwr)))
        {
        ops_null++;
        //fprintf(fpt, "%d, %d\n", iterNode, -1);
        continue;
        }
      }

      if ( sOpsOrder == 3)
      {
        //printf("policy orchestration with o4.");
        if (pManRes->nLastGain > 0)
        {
        if ( pFFormRes == NULL )
            continue;
        pManRes->nTotalGain += pManRes->nLastGain;
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pFFormRes, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRes );
        ops_res++;
        //fprintf(fpt, "%d, %d\n", iterNode, 1);
        (DecisionMask)->pArray[iterNode] = 2;
        continue;
        }

        if (pManRef->nLastGain > 0 || (pManRef->nLastGain ==0 && fUseZeros_ref))
        {
       if ( pFFormRef == NULL )
            continue;
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef );
        ops_ref++;
        //fprintf(fpt, "%d, %d\n", iterNode, 2);
        (DecisionMask)->pArray[iterNode] = 3;
        continue;
        }

        if (nGain > 0 || (nGain == 0 && fUseZeros_rwr))
        {
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        ops_rwr++;
        //fprintf(fpt, "%d, %d\n", iterNode, 0);
        (DecisionMask)->pArray[iterNode] = 0;
        continue;
        }

        if (! (nGain > 0 || pManRef->nLastGain > 0 || pManRes->nLastGain > 0 || (nGain == 0 && fUseZeros_rwr)))
        {
        ops_null++;
        //fprintf(fpt, "%d, %d\n", iterNode, -1);
        continue;
        }
      }

      if ( sOpsOrder == 4)
      {
        //printf("policy orchestration with o5.");
       if (pManRef->nLastGain > 0 || (pManRef->nLastGain ==0 && fUseZeros_ref))
        {
        if ( pFFormRef == NULL )
            continue;
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef );
        ops_ref++;
        //fprintf(fpt, "%d, %d\n", iterNode, 2);
        (DecisionMask)->pArray[iterNode] = 3;
        continue;
        }

        if (nGain > 0 || (nGain == 0 && fUseZeros_rwr))
        {
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        ops_rwr++;
        //fprintf(fpt, "%d, %d\n", iterNode, 0);
        (DecisionMask)->pArray[iterNode] = 0;
        continue;
        }

        if (pManRes->nLastGain > 0)
        {
        if ( pFFormRes == NULL )
            continue;
        pManRes->nTotalGain += pManRes->nLastGain;
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pFFormRes, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRes );
        ops_res++;
        //fprintf(fpt, "%d, %d\n", iterNode, 1);
        (DecisionMask)->pArray[iterNode] = 2;
        continue;
        }
       if (! (nGain > 0 || pManRef->nLastGain > 0 || pManRes->nLastGain > 0 || (nGain == 0 && fUseZeros_rwr)))
        {
        ops_null++;
        //fprintf(fpt, "%d, %d\n", iterNode, -1);
        continue;
        }
      }

      if ( sOpsOrder == 5)
      {
        //printf("policy orchestration with o6.");
        if (pManRef->nLastGain > 0 || (pManRef->nLastGain ==0 && fUseZeros_ref))
        {
        if ( pFFormRef == NULL )
            continue;
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef );
        ops_ref++;
        //fprintf(fpt, "%d, %d\n", iterNode, 2);
        (DecisionMask)->pArray[iterNode] = 3;
        continue;
        }

        if (pManRes->nLastGain > 0)
        {
        if ( pFFormRes == NULL )
            continue;
        pManRes->nTotalGain += pManRes->nLastGain;
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pFFormRes, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRes );
        ops_res++;
        //fprintf(fpt, "%d, %d\n", iterNode, 1);
        (DecisionMask)->pArray[iterNode] = 2;
        continue;
        }

        if (nGain > 0 || (nGain == 0 && fUseZeros_rwr))
        {
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        ops_rwr++;
        //fprintf(fpt, "%d, %d\n", iterNode, 0);
        (DecisionMask)->pArray[iterNode] = 0;
        continue;
        }
        if (! (nGain > 0 || pManRef->nLastGain > 0 || pManRes->nLastGain > 0 || (nGain == 0 && fUseZeros_rwr)))
        {
        ops_null++;
        //fprintf(fpt, "%d, %d\n", iterNode, -1);
        continue;
        }
      }

    }
    for (int i=0; i<nNodes; i++){fprintf(fpt, "%d\n", (DecisionMask)->pArray[i]);}
    fclose(fpt);
    /*
    printf("size of vector %d\n", (**pGain_res).nSize);
    printf("Nodes with rewrite: %d\n", ops_rwr);
    printf("Nodes with resub: %d\n", ops_res);
    printf("Nodes with refactor: %d\n", ops_ref);
    printf("Nodes without updates: %d\n", ops_null);
	*/
    Extra_ProgressBarStop( pProgress );
Rwr_ManAddTimeTotal( pManRwr, Abc_Clock() - clkStart );
    pManRwr->nNodesEnd = Abc_NtkNodeNum(pNtk);

pManRes->timeTotal = Abc_Clock() - clkStart;
    pManRes->nNodesEnd = Abc_NtkNodeNum(pNtk);

pManRef->timeTotal = Abc_Clock() - clkStart;
    pManRef->nNodesEnd = Abc_NtkNodeNum(pNtk);

    if ( fVerbose ){
        Abc_ManResubPrint( pManRes );
        Rwr_ManPrintStats( pManRwr );
        Abc_NtkManRefPrintStats_1( pManRef );
    }
    if ( fVeryVerbose )
        Rwr_ScoresReport( pManRwr );

    Abc_ManResubStop( pManRes );
    Abc_NtkManCutStop( pManCutRes );

    Rwr_ManStop( pManRwr );
    Cut_ManStop( pManCutRwr );
    pNtk->pManCut = NULL;

   Abc_NtkManCutStop( pManCutRef );
    Abc_NtkManRefStop_1( pManRef );

    if ( pManOdc ) Abc_NtkDontCareFree( pManOdc );

    Abc_NtkForEachObj( pNtk, pNode, i )
        pNode->pData = NULL;

    if ( Abc_NtkLatchNum(pNtk) ) {
        Abc_NtkForEachLatch(pNtk, pNode, i)
            pNode->pData = pNode->pNext, pNode->pNext = NULL;
    }
    Abc_NtkReassignIds( pNtk );
    if ( fUpdateLevel )
        Abc_NtkStopReverseLevels( pNtk );
    else
        Abc_NtkLevel( pNtk );
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Abc_NtkOchestraction: The network check has failed.\n" );
        return 0;
    }
s_ResubTime = Abc_Clock() - clkStart;
    return 1;
}

// local greedy orchestration
int Abc_NtkOrchLocal( Abc_Ntk_t * pNtk, int fUseZeros_rwr, int fUseZeros_ref, int fPlaceEnable, int nCutMax, int nStepsMax, int nLevelsOdc, int fUpdateLevel, int fVerbose, int fVeryVerbose, int nNodeSizeMax, int nConeSizeMax, int fUseDcs )
{
    ProgressBar * pProgress;
    // For resub
    Abc_ManRes_t * pManRes;
    Abc_ManCut_t * pManCutRes;
    Odc_Man_t * pManOdc = NULL;
    Dec_Graph_t * pFFormRes;
    //Dec_Graph_t * pFFormRef_zeros;
    Vec_Ptr_t * vLeaves;
    // For rewrite
    Cut_Man_t * pManCutRwr;
    Rwr_Man_t * pManRwr;
    Dec_Graph_t * pGraph;
    // For refactor
    Abc_ManRef_t * pManRef;
    Abc_ManCut_t * pManCutRef;
    Dec_Graph_t * pFFormRef;
    Vec_Ptr_t * vFanins;

    Abc_Obj_t * pNode;//, * pFanin;
    //int fanin_i;
    //FILE * fpt;
    abctime clk, clkStart = Abc_Clock();
    abctime s_ResubTime;
    int i, nNodes, nGain, fCompl, RetValue = 1;//, nGain_zeros;
    //int decisionOps = 0;
    int ops_rwr = 0;
    int ops_res = 0;
    int ops_ref = 0;
    int ops_null = 0;
    assert( Abc_NtkIsStrash(pNtk) );

    // cleanup the AIG
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);

    // start the managers resub
    pManCutRes = Abc_NtkManCutStart( nCutMax, 100000, 100000, 100000 );
    pManRes = Abc_ManResubStart( nCutMax, ABC_RS_DIV1_MAX );
    if ( nLevelsOdc > 0 )
    pManOdc = Abc_NtkDontCareAlloc( nCutMax, nLevelsOdc, fVerbose, fVeryVerbose );
    // start the managers refactor
    pManCutRef = Abc_NtkManCutStart( nNodeSizeMax, nConeSizeMax, 2, 1000 );
    pManRef = Abc_NtkManRefStart_1( nNodeSizeMax, nConeSizeMax, fUseDcs, fVerbose );
    pManRef->vLeaves   = Abc_NtkManCutReadCutLarge( pManCutRef );
    // start the managers rewrite
    pManRwr = Rwr_ManStart( 0 );
    if ( pManRwr == NULL )
        return 0;

    // compute the reverse levels if level update is requested
    if ( fUpdateLevel )
        Abc_NtkStartReverseLevels( pNtk, 0 );

    // 'Resub only'
    if ( Abc_NtkLatchNum(pNtk) ) {
        Abc_NtkForEachLatch(pNtk, pNode, i)
            pNode->pNext = (Abc_Obj_t *)pNode->pData;
    }
    // cut manager for rewrite
clk = Abc_Clock();
    pManCutRwr = Abc_NtkStartCutManForRewrite( pNtk );
Rwr_ManAddTimeCuts( pManRwr, Abc_Clock() - clk );
    pNtk->pManCut = pManCutRwr;

    if ( fVeryVerbose )
        Rwr_ScoresClean( pManRwr );

  // resynthesize each node once
  // resub
    pManRes->nNodesBeg = Abc_NtkNodeNum(pNtk);
  // rewrite
    pManRwr->nNodesBeg = Abc_NtkNodeNum(pNtk);
  // refactor
    pManRef->nNodesBeg = Abc_NtkNodeNum(pNtk);

    nNodes = Abc_NtkObjNumMax(pNtk);
    //printf("nNodes: %d\n", nNodes);
    //if (pGain_res) *pGain_res = Vec_IntAlloc(1);
    //if (pGain_ref) *pGain_ref = Vec_IntAlloc(1);
    //if (pGain_rwr) *pGain_rwr = Vec_IntAlloc(1);

    pProgress = Extra_ProgressBarStart( stdout, nNodes );

    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        //int iterNode = pNode->Id;
        //printf("Nodes ID: %d\n", pNode->Id);
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        // skip the constant node
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        // skip persistant nodes

        if ( Abc_NodeIsPersistant(pNode) )
        {   
            continue;
        } 
        // skip the nodes with many fanouts
        if ( Abc_ObjFanoutNum(pNode) > 1000 )
        {
            continue;
        }
        // stop if all nodes have been tried once
        if ( i >= nNodes )
            break;
        
clk = Abc_Clock();

//Refactor
        vFanins = Abc_NodeFindCut( pManCutRef, pNode, fUseDcs );
pManRef->timeCut += Abc_Clock() - clk;
clk = Abc_Clock();
        //pFFormRef = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros, fUseDcs, fVerbose );
        pFFormRef = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros_ref, fUseDcs, fVerbose );
pManRef->timeRes += Abc_Clock() - clk;

// Resub
        // compute a reconvergence-driven cut
        vLeaves = Abc_NodeFindCut( pManCutRes, pNode, 0 );
//        vLeaves = Abc_CutFactorLarge( pNode, nCutMax );
pManRes->timeCut += Abc_Clock() - clk;
        // get the don't-cares
        if ( pManOdc )
        {
clk = Abc_Clock();
            Abc_NtkDontCareClear( pManOdc );
            Abc_NtkDontCareCompute( pManOdc, pNode, vLeaves, pManRes->pCareSet );
pManRes->timeTruth += Abc_Clock() - clk;
        }
        // evaluate this cut
clk = Abc_Clock();
        pFFormRes = Abc_ManResubEval( pManRes, pNode, vLeaves, nStepsMax, fUpdateLevel, fVerbose );
//        Vec_PtrFree( vLeaves );
//        Abc_ManResubCleanup( pManRes );
pManRes->timeRes += Abc_Clock() - clk;

// Rewrite
        //nGain = Rwr_NodeRewrite( pManRwr, pManCutRwr, pNode, fUpdateLevel, fUseZeros, fPlaceEnable );
        nGain = Rwr_NodeRewrite( pManRwr, pManCutRwr, pNode, fUpdateLevel, fUseZeros_rwr, fPlaceEnable );

     // compare local reward and update
        // if (((! (nGain < 0)) && (! (nGain < pManRes->nLastGain)) && (! (pManRes->nLastGain < pManRef->nLastGain))) || ((! (nGain < 0)) && (! (nGain < pManRef->nLastGain)) && (! (pManRef->nLastGain < pManRes->nLastGain)))){
        if (((! (nGain < 0)) && (! (nGain < pManRes->nLastGain)) && (! (nGain < pManRef->nLastGain)))){
        // update with rewrite
            pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
            fCompl = Rwr_ManReadCompl(pManRwr);
            if ( fPlaceEnable )
                Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
            if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
            Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
            if ( fCompl ) Dec_GraphComplement( pGraph );
            ops_rwr++;
            continue;
        } 
        // if (((! (pManRes->nLastGain < 0)) && (! (pManRes->nLastGain < nGain)) && (! (nGain < pManRef->nLastGain))) || ((! (pManRes->nLastGain < 0)) && (! (pManRes->nLastGain < pManRef->nLastGain)) && (! (pManRef->nLastGain < nGain)))){
        if (((! (pManRes->nLastGain < 0)) && (! (pManRes->nLastGain < nGain)) && (! (pManRes->nLastGain < pManRef->nLastGain)))){
        // update with Resub
            if ( pFFormRes == NULL )
                continue;
            pManRes->nTotalGain += pManRes->nLastGain;
clk = Abc_Clock();
            Dec_GraphUpdateNetwork( pNode, pFFormRes, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk;
            Dec_GraphFree( pFFormRes );
            ops_res++;
            continue;
        }
        // if (((! (pManRef->nLastGain < 0)) && (! (pManRef->nLastGain < nGain)) && (! (nGain < pManRes->nLastGain))) || ((! (pManRef->nLastGain < 0)) && (! (pManRef->nLastGain < pManRes->nLastGain)) && (! (pManRes->nLastGain < nGain)))){
        if (((! (pManRef->nLastGain < 0)) && (! (pManRef->nLastGain < nGain)) && (! (pManRef->nLastGain < pManRes->nLastGain)))){
        // update with Refactor
            if ( pFFormRef == NULL )
                continue;
clk = Abc_Clock();
            if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
            Dec_GraphFree( pFFormRef );
            ops_ref++;
            continue;
        }
        else{ops_null++; continue;}
    }

    /*
    printf("Nodes with rewrite: %d\n", ops_rwr);
    printf("Nodes with resub: %d\n", ops_res);
    printf("Nodes with refactor: %d\n", ops_ref);
    printf("Nodes without updates: %d\n", ops_null); 
    Extra_ProgressBarStop( pProgress );
     */

// Rewrite
Rwr_ManAddTimeTotal( pManRwr, Abc_Clock() - clkStart );
    pManRwr->nNodesEnd = Abc_NtkNodeNum(pNtk);

// Resub
pManRes->timeTotal = Abc_Clock() - clkStart;
    pManRes->nNodesEnd = Abc_NtkNodeNum(pNtk);

// Refactor
pManRef->timeTotal = Abc_Clock() - clkStart;
    pManRef->nNodesEnd = Abc_NtkNodeNum(pNtk);

    // print statistics
    if ( fVerbose ){
        Abc_ManResubPrint( pManRes );
        Rwr_ManPrintStats( pManRwr );
        Abc_NtkManRefPrintStats_1( pManRef );
    }
    if ( fVeryVerbose )
        Rwr_ScoresReport( pManRwr );
    // delete the managers
    // resub
    Abc_ManResubStop( pManRes );
    Abc_NtkManCutStop( pManCutRes );
    // rewrite
    Rwr_ManStop( pManRwr );
    Cut_ManStop( pManCutRwr );
    pNtk->pManCut = NULL;
    // refactor
    Abc_NtkManCutStop( pManCutRef );
    Abc_NtkManRefStop_1( pManRef );

    if ( pManOdc ) Abc_NtkDontCareFree( pManOdc );

    // clean the data field
    Abc_NtkForEachObj( pNtk, pNode, i )
        pNode->pData = NULL;

    if ( Abc_NtkLatchNum(pNtk) ) {
        Abc_NtkForEachLatch(pNtk, pNode, i)
            pNode->pData = pNode->pNext, pNode->pNext = NULL;
    }

    // put the nodes into the DFS order and reassign their IDs
    Abc_NtkReassignIds( pNtk );
//    Abc_AigCheckFaninOrder( pNtk->pManFunc );

    // fix the levels
    if ( fUpdateLevel )
        Abc_NtkStopReverseLevels( pNtk );
    else
        Abc_NtkLevel( pNtk );
    // check
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Abc_NtkOchestraction: The network check has failed.\n" );
        return 0;
    }
s_ResubTime = Abc_Clock() - clkStart;
    return 1;
}


// priority order orchestration (runtime improved TBD)
int Abc_NtkOchestration( Abc_Ntk_t * pNtk, Vec_Int_t **pGain_rwr, Vec_Int_t **pGain_res,Vec_Int_t **pGain_ref, int sOpsOrder, int fUseZeros_rwr, int fUseZeros_ref, int fPlaceEnable, int nCutMax, int nStepsMax, int nLevelsOdc, int fUpdateLevel, int fVerbose, int fVeryVerbose, int nNodeSizeMax, int nConeSizeMax, int fUseDcs )
{
    ProgressBar * pProgress;
    // For resub
    Abc_ManRes_t * pManRes;
    Abc_ManCut_t * pManCutRes;
    Odc_Man_t * pManOdc = NULL;
    Dec_Graph_t * pFFormRes = NULL;
    Vec_Ptr_t * vLeaves;
    // For rewrite
    Cut_Man_t * pManCutRwr;
    Rwr_Man_t * pManRwr;
    Dec_Graph_t * pGraph;
    // For refactor
    Abc_ManRef_t * pManRef;
    Abc_ManCut_t * pManCutRef;
    Dec_Graph_t * pFFormRef;
    Vec_Ptr_t * vFanins;

    Abc_Obj_t * pNode;
    FILE *fpt;
    abctime clk, clkStart = Abc_Clock();
    abctime s_ResubTime;
    int i, nNodes, nGain, fCompl;
    int RetValue = 1;
    int ops_rwr = 0;
    int ops_res = 0;
    int ops_ref = 0;
    int ops_null = 0;
    fUseZeros_rwr = 0;
    fUseZeros_ref = 0;
    //clock_t begin= clock();
    assert( Abc_NtkIsStrash(pNtk) );

    // cleanup the AIG
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);

    // start the managers resub
    pManCutRes = Abc_NtkManCutStart( nCutMax, 100000, 100000, 100000 );
    pManRes = Abc_ManResubStart( nCutMax, ABC_RS_DIV1_MAX );
    if ( nLevelsOdc > 0 )
    pManOdc = Abc_NtkDontCareAlloc( nCutMax, nLevelsOdc, fVerbose, fVeryVerbose );
    // start the managers refactor
    pManCutRef = Abc_NtkManCutStart( nNodeSizeMax, nConeSizeMax, 2, 1000 );
    pManRef = Abc_NtkManRefStart_1( nNodeSizeMax, nConeSizeMax, fUseDcs, fVerbose );
    pManRef->vLeaves   = Abc_NtkManCutReadCutLarge( pManCutRef );
    // start the managers rewrite
    pManRwr = Rwr_ManStart( 0 );
    if ( pManRwr == NULL )
        return 0;

    // compute the reverse levels if level update is requested
    if ( fUpdateLevel )
        Abc_NtkStartReverseLevels( pNtk, 0 );

    // 'Resub only'
    
    if ( Abc_NtkLatchNum(pNtk) ) {
        Abc_NtkForEachLatch(pNtk, pNode, i)
            pNode->pNext = (Abc_Obj_t *)pNode->pData;
    }
    
    // cut manager for rewrite
clk = Abc_Clock();
    pManCutRwr = Abc_NtkStartCutManForRewrite( pNtk );
Rwr_ManAddTimeCuts( pManRwr, Abc_Clock() - clk );
    pNtk->pManCut = pManCutRwr;

    if ( fVeryVerbose )
        Rwr_ScoresClean( pManRwr );

  // resynthesize each node once
  // resub
    pManRes->nNodesBeg = Abc_NtkNodeNum(pNtk);
  // rewrite
    pManRwr->nNodesBeg = Abc_NtkNodeNum(pNtk);
  // refactor
    pManRef->nNodesBeg = Abc_NtkNodeNum(pNtk);

//clock_t resyn_end=clock();
//double resyn_time_spent = (double)(resyn_end-begin)/CLOCKS_PER_SEC;
//printf("time %f\n", resyn_time_spent);
    nNodes = Abc_NtkObjNumMax(pNtk);
    //printf("nNodes: %d\n", nNodes);
    if (pGain_res) *pGain_res = Vec_IntAlloc(1);
    if (pGain_ref) *pGain_ref = Vec_IntAlloc(1);
    if (pGain_rwr) *pGain_rwr = Vec_IntAlloc(1);

    pProgress = Extra_ProgressBarStart( stdout, nNodes );
    fpt = fopen("Ochestration_id_ops_nGain.csv", "w");

    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        //printf("Ochestration id: %d\n", pNode->Id);
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        // skip the constant node
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        // stop if all nodes have been tried once
        if ( i >= nNodes )
            break;
        // skip persistant nodes
        if ( Abc_NodeIsPersistant(pNode) )
        {
            fprintf(fpt, "%d, %s, %d\n", pNode->Id, "None" , -99);
            Vec_IntPush((*pGain_res), -99);
            Vec_IntPush((*pGain_ref), -99);
            Vec_IntPush((*pGain_rwr), -99);
            continue;
        } 
        // skip the nodes with many fanouts
        if ( Abc_ObjFanoutNum(pNode) > 1000 )
        {
            fprintf(fpt, "%d, %s, %d\n", pNode->Id,"None", -99);
            Vec_IntPush((*pGain_res), -99);
            Vec_IntPush((*pGain_ref), -99);
            Vec_IntPush((*pGain_rwr), -99);
            continue;
        }
clk = Abc_Clock();
/*
      if ( sOpsOrder == 0)
      {
// the order is rwr res ref
        //printf("The graph update order is rwr res ref.\n");
        //printf("fUsezeros:%d \n", fUseZeros_rwr);
       
        nGain = Rwr_NodeRewrite( pManRwr, pManCutRwr, pNode, fUpdateLevel, fUseZeros_rwr, fPlaceEnable );
        //printf("nGain:%d\n", nGain);
        Vec_IntPush( (*pGain_rwr), nGain);
        if ((nGain > 0 || (nGain == 0 && fUseZeros_rwr)))
        {
// Graph update with Rewrite
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        ops_rwr++;
        continue;
        }
       
        else{
        //printf("Rewrite not work--resub\n");
        //check res
        vLeaves = Abc_NodeFindCut( pManCutRes, pNode, 0 );
//        vLeaves = Abc_CutFactorLarge( pNode, nCutMax );
pManRes->timeCut += Abc_Clock() - clk;
        // get the don't-cares
        if ( pManOdc )
        {
clk = Abc_Clock();
            Abc_NtkDontCareClear( pManOdc );
            Abc_NtkDontCareCompute( pManOdc, pNode, vLeaves, pManRes->pCareSet );
pManRes->timeTruth += Abc_Clock() - clk;
        }
        // evaluate this cut
clk = Abc_Clock();
        pFFormRes = Abc_ManResubEval( pManRes, pNode, vLeaves, nStepsMax, fUpdateLevel, fVerbose );
//        Vec_PtrFree( vLeaves );
//        Abc_ManResubCleanup( pManRes );
pManRes->timeRes += Abc_Clock() - clk;
        // put nGain in Vector
        Vec_IntPush((*pGain_res), pManRes->nLastGain);
        if (pManRes->nLastGain > 0)
        {
// Graph update with Resub
        //printf("Graph Update with Resub\n");
        if ( pFFormRes == NULL )
            continue;
        pManRes->nTotalGain += pManRes->nLastGain;
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pFFormRes, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRes );
        ops_res++;
        continue;
        }
        else{
        //check ref
        //printf("Rewrite not work--ref\n");
        vFanins = Abc_NodeFindCut( pManCutRef, pNode, fUseDcs );
pManRef->timeCut += Abc_Clock() - clk;
clk = Abc_Clock();
        pFFormRef = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros_ref, fUseDcs, fVerbose );
pManRef->timeRes += Abc_Clock() - clk;

        Vec_IntPush((*pGain_ref), pManRef->nLastGain);
        if (pManRef->nLastGain > 0 || (pManRef->nLastGain ==0 && fUseZeros_ref))
        {
// Graph update with Refactor
        //printf("Graph Update with Refactor\n");
        if ( pFFormRef == NULL )
            continue;
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef );
        ops_ref++;
        continue;
        }
//}
//}
           ops_null++; 
           //continue;         
}
*/


// Update the graph with pre-defined order!
      if ( sOpsOrder == 0)
      {
// the order is rwr res ref
        //printf("new imp: The graph update order is rwr res ref.\n");
        nGain = Rwr_NodeRewrite( pManRwr, pManCutRwr, pNode, fUpdateLevel, fUseZeros_rwr, fPlaceEnable );
        Vec_IntPush( (*pGain_rwr), nGain);
        if (nGain > 0 || (nGain == 0 && fUseZeros_rwr))
        {
// Graph update with Rewrite
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        ops_rwr++;
        continue;
        }
        else{
        //printf("Rewrite not work--resub\n");
        //check res
        vLeaves = Abc_NodeFindCut( pManCutRes, pNode, 0 );
//        vLeaves = Abc_CutFactorLarge( pNode, nCutMax );
pManRes->timeCut += Abc_Clock() - clk;
        // get the don't-cares
        if ( pManOdc )
        {
clk = Abc_Clock();
            Abc_NtkDontCareClear( pManOdc );
            Abc_NtkDontCareCompute( pManOdc, pNode, vLeaves, pManRes->pCareSet );
pManRes->timeTruth += Abc_Clock() - clk;
        }
        // evaluate this cut
clk = Abc_Clock();
        pFFormRes = Abc_ManResubEval( pManRes, pNode, vLeaves, nStepsMax, fUpdateLevel, fVerbose );
//        Vec_PtrFree( vLeaves );
//        Abc_ManResubCleanup( pManRes );
pManRes->timeRes += Abc_Clock() - clk;
        // put nGain in Vector
        Vec_IntPush((*pGain_res), pManRes->nLastGain);
        if (pManRes->nLastGain > 0)
        {
// Graph update with Resub
        //printf("Graph Update with Resub\n");
        if ( pFFormRes != NULL ){
            //continue;
        pManRes->nTotalGain += pManRes->nLastGain;
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pFFormRes, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRes );
        ops_res++;
        continue;}
        }
        else{
        //check ref
        //printf("Rewrite not work--ref\n");
        vFanins = Abc_NodeFindCut( pManCutRef, pNode, fUseDcs );
pManRef->timeCut += Abc_Clock() - clk;
clk = Abc_Clock();
        pFFormRef = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros_ref, fUseDcs, fVerbose );
pManRef->timeRes += Abc_Clock() - clk;

        Vec_IntPush((*pGain_ref), pManRef->nLastGain);
        if (pManRef->nLastGain > 0 || (pManRef->nLastGain ==0 && fUseZeros_ref))
        {
// Graph update with Refactor
        //printf("Graph Update with Refactor\n");
        if ( pFFormRef != NULL ){
            //continue;
clk = Abc_Clock();
        
       Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain);

/*
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef );
                     RetValue = -1;
                     break;
                 }
*/

pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef );
        ops_ref++;
        continue;}
        }
}
}
           ops_null++; 
           continue;         
}


      if ( sOpsOrder == 1)
      {
// the order is rwr ref res
        //printf("The graph update order is rwr ref res.\n");
        nGain = Rwr_NodeRewrite( pManRwr, pManCutRwr, pNode, fUpdateLevel, fUseZeros_rwr, fPlaceEnable );
        Vec_IntPush( (*pGain_rwr), nGain);
        if (nGain > 0 || (nGain == 0 && fUseZeros_rwr))
        {
// Graph update with Rewrite
        //printf("Graph Update with Rewrite");
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        ops_rwr++;
        continue;
        }
        else{
        vFanins = Abc_NodeFindCut( pManCutRef, pNode, fUseDcs );
pManRef->timeCut += Abc_Clock() - clk;
clk = Abc_Clock();
        pFFormRef = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros_ref, fUseDcs, fVerbose );
pManRef->timeRes += Abc_Clock() - clk;
        Vec_IntPush((*pGain_ref), pManRef->nLastGain);
        }
        if (pManRef->nLastGain > 0 || (pManRef->nLastGain ==0 && fUseZeros_ref))
        {
// Graph update with Refactor
        //printf("Graph Update with Refactor");
        if ( pFFormRef != NULL ){
            //continue;
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef );
        ops_ref++;
        continue;}
        }
        else{
        vLeaves = Abc_NodeFindCut( pManCutRes, pNode, 0 );
pManRes->timeCut += Abc_Clock() - clk;
        if ( pManOdc )
        {
clk = Abc_Clock();
            Abc_NtkDontCareClear( pManOdc );
            Abc_NtkDontCareCompute( pManOdc, pNode, vLeaves, pManRes->pCareSet );
pManRes->timeTruth += Abc_Clock() - clk;
        }
clk = Abc_Clock();
        pFFormRes = Abc_ManResubEval( pManRes, pNode, vLeaves, nStepsMax, fUpdateLevel, fVerbose );
pManRes->timeRes += Abc_Clock() - clk;
        Vec_IntPush((*pGain_res), pManRes->nLastGain);
        }
        if (pManRes->nLastGain > 0)
        {
// Graph update with Resub
        //printf("Graph Update with Resub");
        if ( pFFormRes != NULL ){
            //continue;
        pManRes->nTotalGain += pManRes->nLastGain;
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pFFormRes, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRes );
        ops_res++;
        continue;}
        }
// No available updats
        //if (! (nGain > 0 || pManRef->nLastGain > 0 || pManRes->nLastGain > 0 || (nGain == 0 && fUseZeros_rwr)))
        else{
        ops_null++;
        continue;
        }
// }
// }
      }

      if ( sOpsOrder == 2)
      {
// the order is res rwr ref
        //printf("The graph update order is res rwr ref.\n");
        vLeaves = Abc_NodeFindCut( pManCutRes, pNode, 0 );
pManRes->timeCut += Abc_Clock() - clk;
        if ( pManOdc )
        {
clk = Abc_Clock();
            Abc_NtkDontCareClear( pManOdc );
            Abc_NtkDontCareCompute( pManOdc, pNode, vLeaves, pManRes->pCareSet );
pManRes->timeTruth += Abc_Clock() - clk;
        }
clk = Abc_Clock();
        pFFormRes = Abc_ManResubEval( pManRes, pNode, vLeaves, nStepsMax, fUpdateLevel, fVerbose );
pManRes->timeRes += Abc_Clock() - clk;
  Vec_IntPush((*pGain_res), pManRes->nLastGain);

        if (pManRes->nLastGain > 0)
        {
// Graph update with Resub
        //printf("Graph Update with Resub");
        if ( pFFormRes != NULL ){
            //continue;
        pManRes->nTotalGain += pManRes->nLastGain;
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pFFormRes, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRes );
        ops_res++;
        continue;}
        }
        else{
        nGain = Rwr_NodeRewrite( pManRwr, pManCutRwr, pNode, fUpdateLevel, fUseZeros_rwr, fPlaceEnable );
        Vec_IntPush( (*pGain_rwr), nGain);

        if (nGain > 0 || (nGain == 0 && fUseZeros_rwr))
        {
// Graph update with Rewrite
        //printf("Graph Update with Rewrite");
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        ops_rwr++;
        continue;
        }
        else{
        vFanins = Abc_NodeFindCut( pManCutRef, pNode, fUseDcs );
pManRef->timeCut += Abc_Clock() - clk;
clk = Abc_Clock();
        pFFormRef = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros_ref, fUseDcs, fVerbose );
pManRef->timeRes += Abc_Clock() - clk;
        Vec_IntPush((*pGain_ref), pManRef->nLastGain);

        if (pManRef->nLastGain > 0 || (pManRef->nLastGain ==0 && fUseZeros_ref))
        {
// Graph update with Refactor
        //printf("Graph Update with Refactor");
        if ( pFFormRef != NULL ){
            //continue;
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef );
        ops_ref++;
        continue;}
        }
// No available updates
        //if (! (nGain > 0 || pManRef->nLastGain > 0 || pManRes->nLastGain > 0 || (nGain == 0 && fUseZeros_rwr)))
        else{
        ops_null++;
        continue;
        }
}}
      }

      if ( sOpsOrder == 3)
      {
// the order is res ref rwr
        //printf("The graph update order is res ref rwr.\n");
         vLeaves = Abc_NodeFindCut( pManCutRes, pNode, 0 );
pManRes->timeCut += Abc_Clock() - clk;
        if ( pManOdc )
        {
clk = Abc_Clock();
            Abc_NtkDontCareClear( pManOdc );
            Abc_NtkDontCareCompute( pManOdc, pNode, vLeaves, pManRes->pCareSet );
pManRes->timeTruth += Abc_Clock() - clk;
        }
clk = Abc_Clock();
        pFFormRes = Abc_ManResubEval( pManRes, pNode, vLeaves, nStepsMax, fUpdateLevel, fVerbose );
pManRes->timeRes += Abc_Clock() - clk;
        Vec_IntPush((*pGain_res), pManRes->nLastGain);

        if (pManRes->nLastGain > 0)
        {
// Graph update with Resub
        //printf("Graph Update with Resub");
        if ( pFFormRes != NULL ){
            //continue;
        pManRes->nTotalGain += pManRes->nLastGain;
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pFFormRes, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRes );
        ops_res++;
        continue;}
        }
        else{
        vFanins = Abc_NodeFindCut( pManCutRef, pNode, fUseDcs );
pManRef->timeCut += Abc_Clock() - clk;
clk = Abc_Clock();
        pFFormRef = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros_ref, fUseDcs, fVerbose );
pManRef->timeRes += Abc_Clock() - clk;
        Vec_IntPush((*pGain_ref), pManRef->nLastGain);
        //printf("refactor gain: %d\n", pManRef->nLastGain);
        if (pManRef->nLastGain > 0 || (pManRef->nLastGain ==0 && fUseZeros_ref))
        {
// Graph update with Refactor
        //printf("Graph Update with Refactor");
        if ( pFFormRef != NULL ){
            //continue;
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef );
        ops_ref++;
        continue;}
        }
        else{
        nGain = Rwr_NodeRewrite( pManRwr, pManCutRwr, pNode, fUpdateLevel, fUseZeros_rwr, fPlaceEnable );
        Vec_IntPush( (*pGain_rwr), nGain);
        if (nGain > 0 || (nGain == 0 && fUseZeros_rwr))
        {
// Graph update with Rewrite
        //printf("Graph Update with Rewrite");
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        ops_rwr++;
        continue;
        }

// No available updates
       // if (! (nGain > 0 || pManRef->nLastGain > 0 || pManRes->nLastGain > 0 || (nGain == 0 && fUseZeros_rwr)))
        else{
        ops_null++;
        continue;
        }
}
}
      }

      if ( sOpsOrder == 4)
      {
// the order is ref rwr res
        //printf("The graph update order is ref rwr res.\n");
        vFanins = Abc_NodeFindCut( pManCutRef, pNode, fUseDcs );
pManRef->timeCut += Abc_Clock() - clk;
clk = Abc_Clock();
        pFFormRef = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros_ref, fUseDcs, fVerbose );
pManRef->timeRes += Abc_Clock() - clk;
        Vec_IntPush((*pGain_ref), pManRef->nLastGain);

        if (pManRef->nLastGain > 0 || (pManRef->nLastGain ==0 && fUseZeros_ref))
        {
// Graph update with Refactor
        //printf("Graph Update with Refactor");
        if ( pFFormRef != NULL ){
            //continue;
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef );
        ops_ref++;
        continue;}
        }
        else{
        nGain = Rwr_NodeRewrite( pManRwr, pManCutRwr, pNode, fUpdateLevel, fUseZeros_rwr, fPlaceEnable );
        Vec_IntPush( (*pGain_rwr), nGain);

        if (nGain > 0 || (nGain == 0 && fUseZeros_rwr))
        {
// Graph update with Rewrite
        //printf("Graph Update with Rewrite");
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        ops_rwr++;
        continue;
        }
        else{
        vLeaves = Abc_NodeFindCut( pManCutRes, pNode, 0 );
pManRes->timeCut += Abc_Clock() - clk;
        if ( pManOdc )
        {
clk = Abc_Clock();
            Abc_NtkDontCareClear( pManOdc );
            Abc_NtkDontCareCompute( pManOdc, pNode, vLeaves, pManRes->pCareSet );
pManRes->timeTruth += Abc_Clock() - clk;
        }
clk = Abc_Clock();
        pFFormRes = Abc_ManResubEval( pManRes, pNode, vLeaves, nStepsMax, fUpdateLevel, fVerbose );
pManRes->timeRes += Abc_Clock() - clk;
        Vec_IntPush((*pGain_res), pManRes->nLastGain);

        if (pManRes->nLastGain > 0)
        {
// Graph update with Resub
        //printf("Graph Update with Resub");
        if ( pFFormRes != NULL ){
            //continue;
        pManRes->nTotalGain += pManRes->nLastGain;
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pFFormRes, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRes );
        ops_res++;
        continue;}
        }
// No available updates
        //if (! (nGain > 0 || pManRef->nLastGain > 0 || pManRes->nLastGain > 0 || (nGain == 0 && fUseZeros_rwr)))
        else{
        ops_null++;
        continue;
        }
}}
      }

      if ( sOpsOrder == 5)
      {
// the order is ref res rwr
        //printf("The graph update order is ref res rwr.\n");
        vFanins = Abc_NodeFindCut( pManCutRef, pNode, fUseDcs );
pManRef->timeCut += Abc_Clock() - clk;
clk = Abc_Clock();
        pFFormRef = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros_ref, fUseDcs, fVerbose );
pManRef->timeRes += Abc_Clock() - clk;
        Vec_IntPush((*pGain_ref), pManRef->nLastGain);

        if (pManRef->nLastGain > 0 || (pManRef->nLastGain ==0 && fUseZeros_ref))
        {
// Graph update with Refactor
        //printf("Graph Update with Refactor");
        if ( pFFormRef != NULL ){
            //continue;
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef );
        ops_ref++;
        continue;}
        }
        else{
        vLeaves = Abc_NodeFindCut( pManCutRes, pNode, 0 );
pManRes->timeCut += Abc_Clock() - clk;
        if ( pManOdc )
        {
clk = Abc_Clock();
            Abc_NtkDontCareClear( pManOdc );
            Abc_NtkDontCareCompute( pManOdc, pNode, vLeaves, pManRes->pCareSet );
pManRes->timeTruth += Abc_Clock() - clk;
        }
clk = Abc_Clock();
        pFFormRes = Abc_ManResubEval( pManRes, pNode, vLeaves, nStepsMax, fUpdateLevel, fVerbose );
pManRes->timeRes += Abc_Clock() - clk;
        Vec_IntPush((*pGain_res), pManRes->nLastGain);

        if (pManRes->nLastGain > 0)
        {
// Graph update with Resub
        //printf("Graph Update with Resub");
        if ( pFFormRes != NULL ){
            //continue;
        pManRes->nTotalGain += pManRes->nLastGain;
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pFFormRes, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRes );
        ops_res++;
        continue;}
        }
        else{
        nGain = Rwr_NodeRewrite( pManRwr, pManCutRwr, pNode, fUpdateLevel, fUseZeros_rwr, fPlaceEnable );
        Vec_IntPush( (*pGain_rwr), nGain);

        if (nGain > 0 || (nGain == 0 && fUseZeros_rwr))
        {
// Graph update with Rewrite
        //printf("Graph Update with Rewrite");
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        ops_rwr++;
        continue;
        }
// No available updates
        //if (! (nGain > 0 || pManRef->nLastGain > 0 || pManRes->nLastGain > 0 || (nGain == 0 && fUseZeros_rwr)))
        else{
        ops_null++;
        continue;
        }
}}
      }

    }

    fclose(fpt);
    //printf("size of vector %d\n", (**pGain_rwr).nSize);
    //printf("nGain in vector: %d\n", (**pGain_res).pArray[20]);
    //printf("Nodes with rewrite: %d\n", ops_rwr);
    //printf("Nodes with resub: %d\n", ops_res);
    //printf("Nodes with refactor: %d\n", ops_ref);
    //printf("Nodes without updates: %d\n", ops_null);
    Extra_ProgressBarStop( pProgress );
// Rewrite
Rwr_ManAddTimeTotal( pManRwr, Abc_Clock() - clkStart );
    pManRwr->nNodesEnd = Abc_NtkNodeNum(pNtk);

// Resub
pManRes->timeTotal = Abc_Clock() - clkStart;
    pManRes->nNodesEnd = Abc_NtkNodeNum(pNtk);

// Refactor
pManRef->timeTotal = Abc_Clock() - clkStart;
    pManRef->nNodesEnd = Abc_NtkNodeNum(pNtk);

    // print statistics
    if ( fVerbose ){
        Abc_ManResubPrint( pManRes );
        Rwr_ManPrintStats( pManRwr );
        Abc_NtkManRefPrintStats_1( pManRef );
    }
    if ( fVeryVerbose )
        Rwr_ScoresReport( pManRwr );
    // delete the managers
    // resub
    Abc_ManResubStop( pManRes );
    Abc_NtkManCutStop( pManCutRes );
    // rewrite
    Rwr_ManStop( pManRwr );
    Cut_ManStop( pManCutRwr );
    pNtk->pManCut = NULL;
    // refactor
    Abc_NtkManCutStop( pManCutRef );
    Abc_NtkManRefStop_1( pManRef );

    if ( pManOdc ) Abc_NtkDontCareFree( pManOdc );

    // clean the data field
    Abc_NtkForEachObj( pNtk, pNode, i )
        pNode->pData = NULL;

    if ( Abc_NtkLatchNum(pNtk) ) {
        Abc_NtkForEachLatch(pNtk, pNode, i)
            pNode->pData = pNode->pNext, pNode->pNext = NULL;
    }

    // put the nodes into the DFS order and reassign their IDs
    Abc_NtkReassignIds( pNtk );
//    Abc_AigCheckFaninOrder( pNtk->pManFunc );

    // fix the levels
    if ( fUpdateLevel )
        Abc_NtkStopReverseLevels( pNtk );
    else
        Abc_NtkLevel( pNtk );
    // check
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Abc_NtkOchestraction: The network check has failed.\n" );
        return 0;
    }
s_ResubTime = Abc_Clock() - clkStart;
//clock_t end=clock();
//double time_spent = (double)(end-begin)/CLOCKS_PER_SEC;
//printf("time %f\n", time_spent);
    return 1;
}

// random orchestration with rw, rwz, rf, rfz, rs
int Abc_NtkOchestration3( Abc_Ntk_t * pNtk, Vec_Int_t **pGain_rwr, Vec_Int_t **pGain_res, Vec_Int_t **pGain_ref, Vec_Int_t **pOps_num, int fUseZeros, int fUseZeros_rwr, int fUseZeros_ref, int fPlaceEnable, int nCutMax, int nStepsMax, int nLevelsOdc, int fUpdateLevel, int fVerbose, int fVeryVerbose, int nNodeSizeMax, int nConeSizeMax, int fUseDcs )
{
    ProgressBar * pProgress;
    // For resub
    Abc_ManRes_t * pManRes;
    Abc_ManCut_t * pManCutRes;
    Odc_Man_t * pManOdc = NULL;
    Dec_Graph_t * pFFormRes;
    Dec_Graph_t * pFFormRef_zeros;
    Vec_Ptr_t * vLeaves;
    // For rewrite
    Cut_Man_t * pManCutRwr;
    Rwr_Man_t * pManRwr;
    Dec_Graph_t * pGraph;
    // For refactor
    Abc_ManRef_t * pManRef;
    Abc_ManCut_t * pManCutRef;
    Dec_Graph_t * pFFormRef;
    Vec_Ptr_t * vFanins;

    Abc_Obj_t * pNode;
    FILE *fpt;
    abctime clk, clkStart = Abc_Clock();
    abctime s_ResubTime;
    int i, nNodes, nGain, nGain_zeros, fCompl, RetValue = 1;
    int ops_rwr = 0;
    int ops_rwr_z = 0;
    int ops_res = 0;
    int ops_ref_z = 0;
    int ops_ref = 0;
    int ops_null = 0;
    assert( Abc_NtkIsStrash(pNtk) );

    // cleanup the AIG
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);

    // start the managers resub
    pManCutRes = Abc_NtkManCutStart( nCutMax, 100000, 100000, 100000 );
    pManRes = Abc_ManResubStart( nCutMax, ABC_RS_DIV1_MAX );
    if ( nLevelsOdc > 0 )
    pManOdc = Abc_NtkDontCareAlloc( nCutMax, nLevelsOdc, fVerbose, fVeryVerbose );
    // start the managers refactor
    pManCutRef = Abc_NtkManCutStart( nNodeSizeMax, nConeSizeMax, 2, 1000 );
    pManRef = Abc_NtkManRefStart_1( nNodeSizeMax, nConeSizeMax, fUseDcs, fVerbose );
    pManRef->vLeaves   = Abc_NtkManCutReadCutLarge( pManCutRef );
    // start the managers rewrite
    pManRwr = Rwr_ManStart( 0 );
    if ( pManRwr == NULL )
        return 0;

    // compute the reverse levels if level update is requested
    if ( fUpdateLevel )
        Abc_NtkStartReverseLevels( pNtk, 0 );

    // 'Resub only'
    if ( Abc_NtkLatchNum(pNtk) ) {
        Abc_NtkForEachLatch(pNtk, pNode, i)
            pNode->pNext = (Abc_Obj_t *)pNode->pData;
    }
    // cut manager for rewrite
clk = Abc_Clock();
    pManCutRwr = Abc_NtkStartCutManForRewrite( pNtk );
Rwr_ManAddTimeCuts( pManRwr, Abc_Clock() - clk );
    pNtk->pManCut = pManCutRwr;

    if ( fVeryVerbose )
        Rwr_ScoresClean( pManRwr );

  // resynthesize each node once
  // resub
    pManRes->nNodesBeg = Abc_NtkNodeNum(pNtk);
  // rewrite
    pManRwr->nNodesBeg = Abc_NtkNodeNum(pNtk);
  // refactor
    pManRef->nNodesBeg = Abc_NtkNodeNum(pNtk);

    nNodes = Abc_NtkObjNumMax(pNtk);
    //printf("nNodes: %d\n", nNodes);
    if (pGain_res) *pGain_res = Vec_IntAlloc(1);
    if (pGain_ref) *pGain_ref = Vec_IntAlloc(1);
    if (pGain_rwr) *pGain_rwr = Vec_IntAlloc(1);

    pProgress = Extra_ProgressBarStart( stdout, nNodes );
    fpt = fopen("Ochestration_id_ops_nGain.csv", "w");

    Abc_NtkForEachNode( pNtk, pNode, i )
    {
    if (pOps_num) *pOps_num = Vec_IntAlloc(1);
        //printf("Ochestration id: %d\n", pNode->Id);
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        // skip the constant node
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        // skip persistant nodes
        if ( Abc_NodeIsPersistant(pNode) )
        {
            fprintf(fpt, "%d, %s, %d\n", pNode->Id, "None" , -99);
            Vec_IntPush((*pGain_res), -99);
            Vec_IntPush((*pGain_ref), -99);
            Vec_IntPush((*pGain_rwr), -99);
            continue;
        } 
        // skip the nodes with many fanouts
        if ( Abc_ObjFanoutNum(pNode) > 1000 )
        {
            fprintf(fpt, "%d, %s, %d\n", pNode->Id,"None", -99);
            Vec_IntPush((*pGain_res), -99);
            Vec_IntPush((*pGain_ref), -99);
            Vec_IntPush((*pGain_rwr), -99);
            continue;
        }
        // stop if all nodes have been tried once
        if ( i >= nNodes )
            break;
clk = Abc_Clock();

//Refactor
        vFanins = Abc_NodeFindCut( pManCutRef, pNode, fUseDcs );
pManRef->timeCut += Abc_Clock() - clk;
clk = Abc_Clock();
        pFFormRef = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros, fUseDcs, fVerbose );
        pFFormRef_zeros = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros_ref, fUseDcs, fVerbose );
pManRef->timeRes += Abc_Clock() - clk;
        Vec_IntPush((*pGain_ref), pManRef->nLastGain);

// Resub
        // compute a reconvergence-driven cut
        vLeaves = Abc_NodeFindCut( pManCutRes, pNode, 0 );
//        vLeaves = Abc_CutFactorLarge( pNode, nCutMax );
pManRes->timeCut += Abc_Clock() - clk;
        // get the don't-cares
        if ( pManOdc )
        {
clk = Abc_Clock();
            Abc_NtkDontCareClear( pManOdc );
            Abc_NtkDontCareCompute( pManOdc, pNode, vLeaves, pManRes->pCareSet );
pManRes->timeTruth += Abc_Clock() - clk;
        }
        // evaluate this cut
clk = Abc_Clock();
        pFFormRes = Abc_ManResubEval( pManRes, pNode, vLeaves, nStepsMax, fUpdateLevel, fVerbose );
//        Vec_PtrFree( vLeaves );
//        Abc_ManResubCleanup( pManRes );
pManRes->timeRes += Abc_Clock() - clk;
        // put nGain in Vector
        Vec_IntPush((*pGain_res), pManRes->nLastGain);
        // printf("size of vector %d\n", (**pGain).nSize);

// Rewrite
        nGain = Rwr_NodeRewrite( pManRwr, pManCutRwr, pNode, fUpdateLevel, fUseZeros, fPlaceEnable );
        nGain_zeros = Rwr_NodeRewrite( pManRwr, pManCutRwr, pNode, fUpdateLevel, fUseZeros_rwr, fPlaceEnable );
        Vec_IntPush( (*pGain_rwr), nGain);

        //printf("Res Ochestration: %d\n", pManRes->nLastGain);
        //printf("Ref Ochestration: %d\n", pManRef->nLastGain);
        //printf("Rwr Ochestration: %d\n", nGain);
        fprintf(fpt, "%d, %s, %d, %s, %d, %s, %d, %s, %d\n", pNode->Id, "Oches_Res", pManRes->nLastGain, "Oches_Ref", pManRef->nLastGain, "Oches_Rwr", nGain, "Oches_Rwr_Zeros", nGain_zeros);
      
// Generate the valid operator array for the node
      if ( nGain > 0 )
      {
          Vec_IntPush((*pOps_num), 0);
      }
      if ( nGain_zeros > 0 )
      {
          Vec_IntPush((*pOps_num), 1);
      }
      if ( pManRef->nLastGain > 0 )
      {
          Vec_IntPush((*pOps_num), 2);
          Vec_IntPush((*pOps_num), 3);
      }
      if ( pManRes->nLastGain > 0 )
      {
          Vec_IntPush((*pOps_num), 4);
      }
// No available updats
      //if (! (nGain > 0 || nGain_zeros > 0 || pManRef->nLastGain > 0 || pManRes->nLastGain > 0 || (nGain == 0 && fUseZeros_rwr)))
      //if (! (nGain > 0 ||(nGain == 0 && fUseZeros_rwr)))
        //{
        //ops_null++;
        //continue;
        //}
       if (! ((**pOps_num).nSize > 0))
       {
       ops_null++;
       continue;
       }

      //printf("available operations: %d.\n", (**pOps_num).nSize);
// Randomly pick a operation number
      //int Ops_num = 0;
      int Ops_size = (**pOps_num).nSize;
      int r = rand() % Ops_size;
      int Ops_num = (**pOps_num).pArray[r];
      //printf("the picked operation: %d.\n", Ops_num);

// Update the graph with random picked operation!
      if ( Ops_num == 0)
      {
// Graph update with Rewrite
        //printf("Graph Update with Rewrite");
        ops_rwr++;
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        continue;
      }

      if ( Ops_num == 1)
      {
// Graph update with Rewrite -z
        //printf("Graph Update with Rewrite -z");
        ops_rwr_z++;
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain_zeros );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        continue;
      }
      
      if ( Ops_num == 2)
      {
// Graph update with Refactor
        //printf("Graph Update with Refactor");
        ops_ref++;
        if ( pFFormRef == NULL )
            continue;
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef );
        continue;
      }
     
      if ( Ops_num == 3)
      {
// Graph update with Refactor -z
        //printf("Graph Update with Refactor");
        ops_ref_z++;
        if ( pFFormRef_zeros == NULL )
            continue;
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef_zeros, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef_zeros );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef_zeros );
        continue;
      }

      if (Ops_num == 4)
      {
// Graph update with Resub
        //printf("Graph Update with Resub");
        ops_res++;
        if ( pFFormRes == NULL )
            continue;
        pManRes->nTotalGain += pManRes->nLastGain;
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pFFormRes, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRes );
        continue;
        }

    }

    fclose(fpt);
    /*
    printf("size of vector %d\n", (**pGain_rwr).nSize);
    printf("nGain in vector: %d\n", (**pGain_res).pArray[20]);
    printf("Nodes with rewrite: %d\n", ops_rwr);
    printf("Nodes with rewrite -z: %d\n", ops_rwr_z);
    printf("Nodes with resub: %d\n", ops_res);
    printf("Nodes with refactor: %d\n", ops_ref);
    printf("Nodes with refactor -z: %d\n", ops_ref_z);
    printf("Nodes without updates: %d\n", ops_null);
    */
    Extra_ProgressBarStop( pProgress );
// Rewrite
Rwr_ManAddTimeTotal( pManRwr, Abc_Clock() - clkStart );
    pManRwr->nNodesEnd = Abc_NtkNodeNum(pNtk);

// Resub
pManRes->timeTotal = Abc_Clock() - clkStart;
    pManRes->nNodesEnd = Abc_NtkNodeNum(pNtk);

// Refactor
pManRef->timeTotal = Abc_Clock() - clkStart;
    pManRef->nNodesEnd = Abc_NtkNodeNum(pNtk);

    // print statistics
    if ( fVerbose ){
        Abc_ManResubPrint( pManRes );
        Rwr_ManPrintStats( pManRwr );
        Abc_NtkManRefPrintStats_1( pManRef );
    }
    if ( fVeryVerbose )
        Rwr_ScoresReport( pManRwr );
    // delete the managers
    // resub
    Abc_ManResubStop( pManRes );
    Abc_NtkManCutStop( pManCutRes );
    // rewrite
    Rwr_ManStop( pManRwr );
    Cut_ManStop( pManCutRwr );
    pNtk->pManCut = NULL;
    // refactor
    Abc_NtkManCutStop( pManCutRef );
    Abc_NtkManRefStop_1( pManRef );

    if ( pManOdc ) Abc_NtkDontCareFree( pManOdc );

    // clean the data field
    Abc_NtkForEachObj( pNtk, pNode, i )
        pNode->pData = NULL;

    if ( Abc_NtkLatchNum(pNtk) ) {
        Abc_NtkForEachLatch(pNtk, pNode, i)
            pNode->pData = pNode->pNext, pNode->pNext = NULL;
    }

    // put the nodes into the DFS order and reassign their IDs
    Abc_NtkReassignIds( pNtk );
//    Abc_AigCheckFaninOrder( pNtk->pManFunc );

    // fix the levels
    if ( fUpdateLevel )
        Abc_NtkStopReverseLevels( pNtk );
    else
        Abc_NtkLevel( pNtk );
    // check
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Abc_NtkOchestraction: The network check has failed.\n" );
        return 0;
    }
s_ResubTime = Abc_Clock() - clkStart;
    return 1;
}

// random orchestration with rw, rs, rf
int Abc_NtkOchestration2( Abc_Ntk_t * pNtk, Vec_Int_t **pGain_rwr, Vec_Int_t **pGain_res, Vec_Int_t **pGain_ref, Vec_Int_t **pOps_num, int fUseZeros, int fUseZeros_rwr, int fUseZeros_ref, int fPlaceEnable, int nCutMax, int nStepsMax, int nLevelsOdc, int fUpdateLevel, int fVerbose, int fVeryVerbose, int nNodeSizeMax, int nConeSizeMax, int fUseDcs )
{
    ProgressBar * pProgress;
    // For resub
    Abc_ManRes_t * pManRes;
    Abc_ManCut_t * pManCutRes;
    Odc_Man_t * pManOdc = NULL;
    Dec_Graph_t * pFFormRes;
    Dec_Graph_t * pFFormRef_zeros;
    Vec_Ptr_t * vLeaves;
    // For rewrite
    Cut_Man_t * pManCutRwr;
    Rwr_Man_t * pManRwr;
    Dec_Graph_t * pGraph;
    // For refactor
    Abc_ManRef_t * pManRef;
    Abc_ManCut_t * pManCutRef;
    Dec_Graph_t * pFFormRef;
    Vec_Ptr_t * vFanins;

    Abc_Obj_t * pNode;
    FILE *fpt;
    abctime clk, clkStart = Abc_Clock();
    abctime s_ResubTime;
    int i, nNodes, nGain, nGain_zeros, fCompl, RetValue = 1;
    int ops_rwr = 0;
    int ops_res = 0;
    int ops_ref = 0;
    int ops_null = 0;
    int rwr_ok = 0;
    int res_ok = 0;
    int ref_ok = 0;
    int decisionOps = 0;
    assert( Abc_NtkIsStrash(pNtk) );

    // cleanup the AIG
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);

    // start the managers resub
    pManCutRes = Abc_NtkManCutStart( nCutMax, 100000, 100000, 100000 );
    pManRes = Abc_ManResubStart( nCutMax, ABC_RS_DIV1_MAX );
    if ( nLevelsOdc > 0 )
    pManOdc = Abc_NtkDontCareAlloc( nCutMax, nLevelsOdc, fVerbose, fVeryVerbose );
    // start the managers refactor
    pManCutRef = Abc_NtkManCutStart( nNodeSizeMax, nConeSizeMax, 2, 1000 );
    pManRef = Abc_NtkManRefStart_1( nNodeSizeMax, nConeSizeMax, fUseDcs, fVerbose );
    pManRef->vLeaves   = Abc_NtkManCutReadCutLarge( pManCutRef );
    // start the managers rewrite
    pManRwr = Rwr_ManStart( 0 );
    if ( pManRwr == NULL )
        return 0;

    // compute the reverse levels if level update is requested
    if ( fUpdateLevel )
        Abc_NtkStartReverseLevels( pNtk, 0 );

    // 'Resub only'
    if ( Abc_NtkLatchNum(pNtk) ) {
        Abc_NtkForEachLatch(pNtk, pNode, i)
            pNode->pNext = (Abc_Obj_t *)pNode->pData;
    }
    // cut manager for rewrite
clk = Abc_Clock();
    pManCutRwr = Abc_NtkStartCutManForRewrite( pNtk );
Rwr_ManAddTimeCuts( pManRwr, Abc_Clock() - clk );
    pNtk->pManCut = pManCutRwr;

    if ( fVeryVerbose )
        Rwr_ScoresClean( pManRwr );

  // resynthesize each node once
  // resub
    pManRes->nNodesBeg = Abc_NtkNodeNum(pNtk);
  // rewrite
    pManRwr->nNodesBeg = Abc_NtkNodeNum(pNtk);
  // refactor
    pManRef->nNodesBeg = Abc_NtkNodeNum(pNtk);

    nNodes = Abc_NtkObjNumMax(pNtk);
    //printf("nNodes: %d\n", nNodes);
    if (pGain_res) *pGain_res = Vec_IntAlloc(1);
    if (pGain_ref) *pGain_ref = Vec_IntAlloc(1);
    if (pGain_rwr) *pGain_rwr = Vec_IntAlloc(1);

    pProgress = Extra_ProgressBarStart( stdout, nNodes );
    fpt = fopen("Ochestration_id_ops_nGain.csv", "w");

    Abc_NtkForEachNode( pNtk, pNode, i )
    {
    //printf("Nodes ID: %d\n", pNode->Id);
    rwr_ok = 0;
    ref_ok = 0;
    res_ok = 0;
    if (pOps_num) *pOps_num = Vec_IntAlloc(1);
        //printf("Ochestration id: %d\n", pNode->Id);
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        // skip the constant node
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        // skip persistant nodes
        if ( Abc_NodeIsPersistant(pNode) )
        {
            fprintf(fpt, "%d, %d, %d, %d, %d\n", pNode->Id, 0, 0, 0, 0);
            Vec_IntPush((*pGain_res), -99);
            Vec_IntPush((*pGain_ref), -99);
            Vec_IntPush((*pGain_rwr), -99);
            continue;
        } 
        // skip the nodes with many fanouts
        if ( Abc_ObjFanoutNum(pNode) > 1000 )
        {
            fprintf(fpt, "%d, %d, %d, %d, %d\n", pNode->Id, 0, 0, 0, 0);
            Vec_IntPush((*pGain_res), -99);
            Vec_IntPush((*pGain_ref), -99);
            Vec_IntPush((*pGain_rwr), -99);
            continue;
        }
        // stop if all nodes have been tried once
        if ( i >= nNodes )
            break;
clk = Abc_Clock();

//Refactor
        vFanins = Abc_NodeFindCut( pManCutRef, pNode, fUseDcs );
pManRef->timeCut += Abc_Clock() - clk;
clk = Abc_Clock();
        pFFormRef = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros, fUseDcs, fVerbose );
        pFFormRef_zeros = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros_ref, fUseDcs, fVerbose );
pManRef->timeRes += Abc_Clock() - clk;
        Vec_IntPush((*pGain_ref), pManRef->nLastGain);

// Resub
        // compute a reconvergence-driven cut
        vLeaves = Abc_NodeFindCut( pManCutRes, pNode, 0 );
//        vLeaves = Abc_CutFactorLarge( pNode, nCutMax );
pManRes->timeCut += Abc_Clock() - clk;
        // get the don't-cares
        if ( pManOdc )
        {
clk = Abc_Clock();
            Abc_NtkDontCareClear( pManOdc );
            Abc_NtkDontCareCompute( pManOdc, pNode, vLeaves, pManRes->pCareSet );
pManRes->timeTruth += Abc_Clock() - clk;
        }
        // evaluate this cut
clk = Abc_Clock();
        pFFormRes = Abc_ManResubEval( pManRes, pNode, vLeaves, nStepsMax, fUpdateLevel, fVerbose );
//        Vec_PtrFree( vLeaves );
//        Abc_ManResubCleanup( pManRes );
pManRes->timeRes += Abc_Clock() - clk;
        // put nGain in Vector
        Vec_IntPush((*pGain_res), pManRes->nLastGain);
        // printf("size of vector %d\n", (**pGain).nSize);

// Rewrite
        nGain = Rwr_NodeRewrite( pManRwr, pManCutRwr, pNode, fUpdateLevel, fUseZeros, fPlaceEnable );
        nGain_zeros = Rwr_NodeRewrite( pManRwr, pManCutRwr, pNode, fUpdateLevel, fUseZeros_rwr, fPlaceEnable );
        Vec_IntPush( (*pGain_rwr), nGain);

        //printf("Res Ochestration: %d\n", pManRes->nLastGain);
        //printf("Ref Ochestration: %d\n", pManRef->nLastGain);
        //printf("Rwr Ochestration: %d\n", nGain);
        fprintf(fpt, "%d, %s, %d, %s, %d, %s, %d, %s, %d\n", pNode->Id, "Oches_Res", pManRes->nLastGain, "Oches_Ref", pManRef->nLastGain, "Oches_Rwr", nGain, "Oches_Rwr_Zeros", nGain_zeros);

        //fprintf(fpt, "%d, %s, %d, %s, %d, %s, %d, %s, %d\n", pNode->Id, "Oches_Res", pManRes->nLastGain, "Oches_Ref", pManRef->nLastGain, "Oches_Rwr", nGain, "Oches_Rwr_Zeros", nGain_zeros);
      
// Generate the valid operator array for the node
      if ( nGain > 0 )
      {   
          rwr_ok = 1;
          Vec_IntPush((*pOps_num), 0);
      }
      //if ( nGain_zeros > 0 )
      //{
      //    Vec_IntPush((*pOps_num), 1);
      //}
      //take refactor put

      if ( pManRef->nLastGain > 0 )
      {   
          ref_ok = 1;
          Vec_IntPush((*pOps_num), 2);
          //Vec_IntPush((*pOps_num), 3);
      }

      if ( pManRes->nLastGain > 0 )
      {   
          res_ok = 1;
          Vec_IntPush((*pOps_num), 1);
      }
// No available updats
      //if (! (nGain > 0 || nGain_zeros > 0 || pManRef->nLastGain > 0 || pManRes->nLastGain > 0 || (nGain == 0 && fUseZeros_rwr)))
      //if (! (nGain > 0 ||(nGain == 0 && fUseZeros_rwr)))
        //{
        //ops_null++;
        //continue;
        //}
       if (! ((**pOps_num).nSize > 0))
       {
       fprintf(fpt, "%d, %d, %d, %d, %d\n", pNode->Id, 0, 0, 0, 0);
       ops_null++;
       continue;
       }
/*
        if (pManRes->nLastGain > 0){
            res_ok = 1;
        }
        if (pManRef->nLastGain > 0){
            ref_ok = 1;
        }
        if (nGain > 0){
            rwr_ok = 1;
        }
*/

      //printf("available operations: %d.\n", (**pOps_num).nSize);
// Randomly pick a operation number
      //int Ops_num = 0;
      int Ops_size = (**pOps_num).nSize;
      int r = rand() % Ops_size;
      int Ops_num = (**pOps_num).pArray[r];
      //printf("the picked operation: %d.\n", Ops_num);

// Update the graph with random picked operation!
      if ( Ops_num == 0)
      {
// Graph update with Rewrite
        //printf("Graph Update with Rewrite");
        decisionOps = 1;
        ops_rwr++;
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        //printf("Nodes ID: %d\n", pNode->Id);
        fprintf(fpt, "%d, %d, %d, %d, %d\n", pNode->Id, rwr_ok, ref_ok, res_ok, decisionOps);
        continue;
      }
/*
      if ( Ops_num == 1)
      {
// Graph update with Rewrite -z
        //printf("Graph Update with Rewrite -z");
        ops_rwr_z++;
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain_zeros );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        continue;
      }
*/      
      if ( Ops_num == 2)
      {
// Graph update with Refactor
        //printf("Graph Update with Refactor");
        decisionOps = 2;
        ops_ref++;
        if ( pFFormRef == NULL )
            continue;
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef );
        fprintf(fpt, "%d, %d, %d, %d, %d\n", pNode->Id, rwr_ok, ref_ok, res_ok, decisionOps);
        continue;
      }
/*     
      if ( Ops_num == 3)
      {
// Graph update with Refactor -z
        //printf("Graph Update with Refactor");
        ops_ref_z++;
        if ( pFFormRef_zeros == NULL )
            continue;
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef_zeros, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef_zeros );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef_zeros );
        continue;
      }
*/

      if (Ops_num == 1)
      {
// Graph update with Resub
        //printf("Graph Update with Resub");
        decisionOps = 3;
        ops_res++;
        if ( pFFormRes == NULL )
            continue;
        pManRes->nTotalGain += pManRes->nLastGain;
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pFFormRes, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRes );
        fprintf(fpt, "%d, %d, %d, %d, %d\n", pNode->Id, rwr_ok, ref_ok, res_ok, decisionOps);
        continue;
        }

    }

    fclose(fpt);
   
    /* 
    printf("size of vector %d\n", (**pGain_rwr).nSize);
    printf("nGain in vector: %d\n", (**pGain_res).pArray[20]);
    printf("Nodes with rewrite: %d\n", ops_rwr);
    //printf("Nodes with rewrite -z: %d\n", ops_rwr_z);
    printf("Nodes with resub: %d\n", ops_res);
    printf("Nodes with refactor: %d\n", ops_ref);
    //printf("Nodes with refactor -z: %d\n", ops_ref_z);
    printf("Nodes without updates: %d\n", ops_null);
    */
    Extra_ProgressBarStop( pProgress );
// Rewrite
Rwr_ManAddTimeTotal( pManRwr, Abc_Clock() - clkStart );
    pManRwr->nNodesEnd = Abc_NtkNodeNum(pNtk);

// Resub
pManRes->timeTotal = Abc_Clock() - clkStart;
    pManRes->nNodesEnd = Abc_NtkNodeNum(pNtk);

// Refactor
pManRef->timeTotal = Abc_Clock() - clkStart;
    pManRef->nNodesEnd = Abc_NtkNodeNum(pNtk);

    // print statistics
    if ( fVerbose ){
        Abc_ManResubPrint( pManRes );
        Rwr_ManPrintStats( pManRwr );
        Abc_NtkManRefPrintStats_1( pManRef );
    }
    if ( fVeryVerbose )
        Rwr_ScoresReport( pManRwr );
    // delete the managers
    // resub
    Abc_ManResubStop( pManRes );
    Abc_NtkManCutStop( pManCutRes );
    // rewrite
    Rwr_ManStop( pManRwr );
    Cut_ManStop( pManCutRwr );
    pNtk->pManCut = NULL;
    // refactor
    Abc_NtkManCutStop( pManCutRef );
    Abc_NtkManRefStop_1( pManRef );

    if ( pManOdc ) Abc_NtkDontCareFree( pManOdc );

    // clean the data field
    Abc_NtkForEachObj( pNtk, pNode, i )
        pNode->pData = NULL;

    if ( Abc_NtkLatchNum(pNtk) ) {
        Abc_NtkForEachLatch(pNtk, pNode, i)
            pNode->pData = pNode->pNext, pNode->pNext = NULL;
    }

    // put the nodes into the DFS order and reassign their IDs
    Abc_NtkReassignIds( pNtk );
//    Abc_AigCheckFaninOrder( pNtk->pManFunc );

    // fix the levels
    if ( fUpdateLevel )
        Abc_NtkStopReverseLevels( pNtk );
    else
        Abc_NtkLevel( pNtk );
    // check
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Abc_NtkOchestraction: The network check has failed.\n" );
        return 0;
    }
s_ResubTime = Abc_Clock() - clkStart;
    return 1;
}

// rw rs rf embedding generation for GNN learning
int Abc_NtkOrchGNN( Abc_Ntk_t * pNtk,  char * edgelistFile, char * featFile, int fUseZeros, int fUseZeros_rwr, int fUseZeros_ref, int fPlaceEnable, int nCutMax, int nStepsMax, int nLevelsOdc, int fUpdateLevel, int fVerbose, int fVeryVerbose, int nNodeSizeMax, int nConeSizeMax, int fUseDcs )
{
    ProgressBar * pProgress;
    // For resub
    Abc_ManRes_t * pManRes;
    Abc_ManCut_t * pManCutRes;
    Odc_Man_t * pManOdc = NULL;
    Dec_Graph_t * pFFormRes;
    Dec_Graph_t * pFFormRef_zeros;
    Vec_Ptr_t * vLeaves;
    // For rewrite
    Cut_Man_t * pManCutRwr;
    Rwr_Man_t * pManRwr;
    //Dec_Graph_t * pGraph;
    // For refactor
    Abc_ManRef_t * pManRef;
    Abc_ManCut_t * pManCutRef;
    Dec_Graph_t * pFFormRef;
    Vec_Ptr_t * vFanins;

    Abc_Obj_t * pNode, * pFanin;
    int fanin_i;
    FILE * f_el;
    FILE * f_feats;
    //FILE * fpt;
    abctime clk, clkStart = Abc_Clock();
    abctime s_ResubTime;
    int i, nNodes, nGain, nGain_zeros;//, fCompl, RetValue = 1;
    int rwr_ok = 0;
    int res_ok = 0;
    int ref_ok = 0;
    //int decisionOps = 0;
    assert( Abc_NtkIsStrash(pNtk) );

    // cleanup the AIG
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);

    // start the managers resub
    pManCutRes = Abc_NtkManCutStart( nCutMax, 100000, 100000, 100000 );
    pManRes = Abc_ManResubStart( nCutMax, ABC_RS_DIV1_MAX );
    if ( nLevelsOdc > 0 )
    pManOdc = Abc_NtkDontCareAlloc( nCutMax, nLevelsOdc, fVerbose, fVeryVerbose );
    // start the managers refactor
    pManCutRef = Abc_NtkManCutStart( nNodeSizeMax, nConeSizeMax, 2, 1000 );
    pManRef = Abc_NtkManRefStart_1( nNodeSizeMax, nConeSizeMax, fUseDcs, fVerbose );
    pManRef->vLeaves   = Abc_NtkManCutReadCutLarge( pManCutRef );
    // start the managers rewrite
    pManRwr = Rwr_ManStart( 0 );
    if ( pManRwr == NULL )
        return 0;

    // compute the reverse levels if level update is requested
    if ( fUpdateLevel )
        Abc_NtkStartReverseLevels( pNtk, 0 );

    // 'Resub only'
    if ( Abc_NtkLatchNum(pNtk) ) {
        Abc_NtkForEachLatch(pNtk, pNode, i)
            pNode->pNext = (Abc_Obj_t *)pNode->pData;
    }
    // cut manager for rewrite
clk = Abc_Clock();
    pManCutRwr = Abc_NtkStartCutManForRewrite( pNtk );
Rwr_ManAddTimeCuts( pManRwr, Abc_Clock() - clk );
    pNtk->pManCut = pManCutRwr;

    if ( fVeryVerbose )
        Rwr_ScoresClean( pManRwr );

  // resynthesize each node once
  // resub
    pManRes->nNodesBeg = Abc_NtkNodeNum(pNtk);
  // rewrite
    pManRwr->nNodesBeg = Abc_NtkNodeNum(pNtk);
  // refactor
    pManRef->nNodesBeg = Abc_NtkNodeNum(pNtk);

    nNodes = Abc_NtkObjNumMax(pNtk);
    //printf("nNodes: %d\n", nNodes);
    //if (pGain_res) *pGain_res = Vec_IntAlloc(1);
    //if (pGain_ref) *pGain_ref = Vec_IntAlloc(1);
    //if (pGain_rwr) *pGain_rwr = Vec_IntAlloc(1);

    pProgress = Extra_ProgressBarStart( stdout, nNodes );
    //fpt = fopen("GNN_Embedding.csv", "w");
    f_el = fopen(edgelistFile, "w");
    f_feats = fopen(featFile, "w");

    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        int iterNode = pNode->Id;
        Abc_ObjForEachFanin(pNode, pFanin, fanin_i){
            fprintf(f_el, "%d %d\n", iterNode, Abc_ObjId(pFanin));
        }
        //printf("Nodes ID: %d\n", pNode->Id);
        rwr_ok = 0;
        ref_ok = 0;
        res_ok = 0;
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        // skip the constant node
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        // skip persistant nodes

        if ( Abc_NodeIsPersistant(pNode) )
        {   
            //fprintf(f_el, "%d %d\n", iterNode, Abc_ObjId(pFanin));
            fprintf(f_feats, "%d, %d, %d, %d, %d, %d, %d, %d\n", Abc_ObjFaninC0(pNode), Abc_ObjFaninC1(pNode), -1, -1, -1, -1, -1, -1);
            //fprintf(fpt, "%d, %d, %d, %d, %d, %d, %d, %d, %d,  %d\n", iterNode, Abc_ObjId(pFanin), Abc_ObjFaninC0(pNode), Abc_ObjFaninC1(pNode), -1, -1, -1, -1, -1, -1);
            continue;
        } 
        // skip the nodes with many fanouts
        if ( Abc_ObjFanoutNum(pNode) > 1000 )
        {
            //fprintf(f_el, "%d %d\n", iterNode, Abc_ObjId(pFanin));
            fprintf(f_feats, "%d, %d, %d, %d, %d, %d, %d, %d\n", Abc_ObjFaninC0(pNode), Abc_ObjFaninC1(pNode), -1, -1, -1, -1, -1, -1);
            //fprintf(fpt, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n", iterNode, Abc_ObjId(pFanin), Abc_ObjFaninC0(pNode), Abc_ObjFaninC1(pNode), -1, -1, -1, -1, -1, -1);
            continue;
        }
        // stop if all nodes have been tried once
        if ( i >= nNodes )
            break;
        
clk = Abc_Clock();

//Refactor
        vFanins = Abc_NodeFindCut( pManCutRef, pNode, fUseDcs );
pManRef->timeCut += Abc_Clock() - clk;
clk = Abc_Clock();
        pFFormRef = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros, fUseDcs, fVerbose );
        pFFormRef_zeros = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros_ref, fUseDcs, fVerbose );
pManRef->timeRes += Abc_Clock() - clk;
        if (! (pManRef->nLastGain < 0) ) {ref_ok = 1;}

// Resub
        // compute a reconvergence-driven cut
        vLeaves = Abc_NodeFindCut( pManCutRes, pNode, 0 );
//        vLeaves = Abc_CutFactorLarge( pNode, nCutMax );
pManRes->timeCut += Abc_Clock() - clk;
        // get the don't-cares
        if ( pManOdc )
        {
clk = Abc_Clock();
            Abc_NtkDontCareClear( pManOdc );
            Abc_NtkDontCareCompute( pManOdc, pNode, vLeaves, pManRes->pCareSet );
pManRes->timeTruth += Abc_Clock() - clk;
        }
        // evaluate this cut
clk = Abc_Clock();
        pFFormRes = Abc_ManResubEval( pManRes, pNode, vLeaves, nStepsMax, fUpdateLevel, fVerbose );
//        Vec_PtrFree( vLeaves );
//        Abc_ManResubCleanup( pManRes );
pManRes->timeRes += Abc_Clock() - clk;
        // put nGain in Vector
        if (! (pManRes->nLastGain < 0) ) {res_ok = 1;}

// Rewrite
        nGain = Rwr_NodeRewrite( pManRwr, pManCutRwr, pNode, fUpdateLevel, fUseZeros, fPlaceEnable );
        nGain_zeros = Rwr_NodeRewrite( pManRwr, pManCutRwr, pNode, fUpdateLevel, fUseZeros_rwr, fPlaceEnable );
        if (! (nGain < 0) ) {rwr_ok = 1;}

        //fprintf(f_el, "%d %d\n", iterNode, Abc_ObjId(pFanin));
        fprintf(f_feats, "%d, %d, %d, %d, %d, %d, %d, %d\n", Abc_ObjFaninC0(pNode), Abc_ObjFaninC1(pNode), rwr_ok, nGain, res_ok, pManRes->nLastGain, ref_ok, pManRef->nLastGain);
        //fprintf(fpt, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n", iterNode, Abc_ObjId(pFanin), Abc_ObjFaninC0(pNode), Abc_ObjFaninC1(pNode), rwr_ok, nGain, res_ok, pManRes->nLastGain, ref_ok, pManRef->nLastGain);
        //printf("Res Ochestration: %d\n", pManRes->nLastGain);
        //printf("Ref Ochestration: %d\n", pManRef->nLastGain);
        //printf("Rwr Ochestration: %d\n", nGain);

     //continue; 
     //}
     //continue; 
    }
    fclose(f_el);
    fclose(f_feats);
    //fclose(fpt);
    
    
    Extra_ProgressBarStop( pProgress );
// Rewrite
Rwr_ManAddTimeTotal( pManRwr, Abc_Clock() - clkStart );
    pManRwr->nNodesEnd = Abc_NtkNodeNum(pNtk);

// Resub
pManRes->timeTotal = Abc_Clock() - clkStart;
    pManRes->nNodesEnd = Abc_NtkNodeNum(pNtk);

// Refactor
pManRef->timeTotal = Abc_Clock() - clkStart;
    pManRef->nNodesEnd = Abc_NtkNodeNum(pNtk);

    // print statistics
    if ( fVerbose ){
        Abc_ManResubPrint( pManRes );
        Rwr_ManPrintStats( pManRwr );
        Abc_NtkManRefPrintStats_1( pManRef );
    }
    if ( fVeryVerbose )
        Rwr_ScoresReport( pManRwr );
    // delete the managers
    // resub
    Abc_ManResubStop( pManRes );
    Abc_NtkManCutStop( pManCutRes );
    // rewrite
    Rwr_ManStop( pManRwr );
    Cut_ManStop( pManCutRwr );
    pNtk->pManCut = NULL;
    // refactor
    Abc_NtkManCutStop( pManCutRef );
    Abc_NtkManRefStop_1( pManRef );

    if ( pManOdc ) Abc_NtkDontCareFree( pManOdc );

    // clean the data field
    Abc_NtkForEachObj( pNtk, pNode, i )
        pNode->pData = NULL;

    if ( Abc_NtkLatchNum(pNtk) ) {
        Abc_NtkForEachLatch(pNtk, pNode, i)
            pNode->pData = pNode->pNext, pNode->pNext = NULL;
    }

    // put the nodes into the DFS order and reassign their IDs
    Abc_NtkReassignIds( pNtk );
//    Abc_AigCheckFaninOrder( pNtk->pManFunc );

    // fix the levels
    if ( fUpdateLevel )
        Abc_NtkStopReverseLevels( pNtk );
    else
        Abc_NtkLevel( pNtk );
    // check
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Abc_NtkOchestraction: The network check has failed.\n" );
        return 0;
    }
s_ResubTime = Abc_Clock() - clkStart;
    return 1;
}

// orchestration with sudo random decision list
int Abc_NtkOrchRand( Abc_Ntk_t * pNtk, Vec_Int_t **pGain_rwr, Vec_Int_t **pGain_res,Vec_Int_t **pGain_ref, Vec_Int_t **DecisionMask, char * DecisionFile, int Rand_Seed, int fUseZeros_rwr, int fUseZeros_ref, int fPlaceEnable, int nCutMax, int nStepsMax, int nLevelsOdc, int fUpdateLevel, int fVerbose, int fVeryVerbose, int nNodeSizeMax, int nConeSizeMax, int fUseDcs )
{
    extern int           Dec_GraphUpdateNetwork( Abc_Obj_t * pRoot, Dec_Graph_t * pGraph, int fUpdateLevel, int nGain );
    ProgressBar * pProgress;
    // For resub
    Abc_ManRes_t * pManRes;
    Abc_ManCut_t * pManCutRes;
    Odc_Man_t * pManOdc = NULL;
    Dec_Graph_t * pFFormRes;
    Vec_Ptr_t * vLeaves;
    // For rewrite
    Cut_Man_t * pManCutRwr;
    Rwr_Man_t * pManRwr;
    Dec_Graph_t * pGraph;
    // For refactor
    Abc_ManRef_t * pManRef;
    Abc_ManCut_t * pManCutRef;
    Dec_Graph_t * pFFormRef;
    Vec_Ptr_t * vFanins;

    Abc_Obj_t * pNode;
    FILE *fpt;
    abctime clk, clkStart = Abc_Clock();
    int i, nNodes, nNodes_after, nGain, fCompl;
    int RetValue = 1;
    int ops_rwr = 0;
    int ops_res = 0;
    int ops_ref = 0;
    int ops_null = 0;
    int Valid_Len = 0;
    //Vec_Int_t *Valid_Ops;

    //clock_t begin= clock();
    assert( Abc_NtkIsStrash(pNtk) );

    // cleanup the AIG
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);

    // start the managers resub
    pManCutRes = Abc_NtkManCutStart( nCutMax, 100000, 100000, 100000 );
    pManRes = Abc_ManResubStart( nCutMax, ABC_RS_DIV1_MAX );
    if ( nLevelsOdc > 0 )
    pManOdc = Abc_NtkDontCareAlloc( nCutMax, nLevelsOdc, fVerbose, fVeryVerbose );
    // start the managers refactor
    pManCutRef = Abc_NtkManCutStart( nNodeSizeMax, nConeSizeMax, 2, 1000 );
    pManRef = Abc_NtkManRefStart_1( nNodeSizeMax, nConeSizeMax, fUseDcs, fVerbose );
    pManRef->vLeaves   = Abc_NtkManCutReadCutLarge( pManCutRef );
    // start the managers rewrite
    pManRwr = Rwr_ManStart( 0 );
    if ( pManRwr == NULL )
        return 0;

    // compute the reverse levels if level update is requested
    if ( fUpdateLevel )
        Abc_NtkStartReverseLevels( pNtk, 0 );

    // 'Resub only'
    
    if ( Abc_NtkLatchNum(pNtk) ) {
        Abc_NtkForEachLatch(pNtk, pNode, i)
            pNode->pNext = (Abc_Obj_t *)pNode->pData;
    }
    
    // cut manager for rewrite
clk = Abc_Clock();
    pManCutRwr = Abc_NtkStartCutManForRewrite( pNtk );
Rwr_ManAddTimeCuts( pManRwr, Abc_Clock() - clk );
    pNtk->pManCut = pManCutRwr;

    if ( fVeryVerbose )
        Rwr_ScoresClean( pManRwr );

  // resynthesize each node once
  // resub
    pManRes->nNodesBeg = Abc_NtkNodeNum(pNtk);
  // rewrite
    pManRwr->nNodesBeg = Abc_NtkNodeNum(pNtk);
  // refactor
    pManRef->nNodesBeg = Abc_NtkNodeNum(pNtk);

//clock_t resyn_end=clock();
//double resyn_time_spent = (double)(resyn_end-begin)/CLOCKS_PER_SEC;
//printf("time %f\n", resyn_time_spent);
    nNodes = Abc_NtkObjNumMax(pNtk);
    //printf("nNodes: %d\n", nNodes);
    //for(int i=0; i < nNodes; i++){printf("mask check: %d\n", (*DecisionMask)->pArray[i]);}
    //printf("mask size:%d", (**DecisionMask).nSize);
    if (pGain_res) *pGain_res = Vec_IntAlloc(1);
    if (pGain_ref) *pGain_ref = Vec_IntAlloc(1);
    if (pGain_rwr) *pGain_rwr = Vec_IntAlloc(1);
    Vec_Int_t  *Valid_Ops = Vec_IntAlloc(1);

    pProgress = Extra_ProgressBarStart( stdout, nNodes );
    fpt = fopen(DecisionFile, "w");

    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        //printf("Ochestration id: %d\n", pNode->Id);
        int iterNode = pNode->Id;
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        // skip the constant node
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        // stop if all nodes have been tried once
        if ( i >= nNodes )
            break;
        // skip persistant nodes
        if ( Abc_NodeIsPersistant(pNode) )
        {
            //fprintf(fpt, "%d, %s, %d\n", pNode->Id, "None" , -99);
            Vec_IntPush((*pGain_res), -99);
            Vec_IntPush((*pGain_ref), -99);
            Vec_IntPush((*pGain_rwr), -99);
            continue;
        } 
        // skip the nodes with many fanouts
        if ( Abc_ObjFanoutNum(pNode) > 1000 )
        {
            //fprintf(fpt, "%d, %s, %d\n", pNode->Id,"None", -99);
            Vec_IntPush((*pGain_res), -99);
            Vec_IntPush((*pGain_ref), -99);
            Vec_IntPush((*pGain_rwr), -99);
            continue;
        }
clk = Abc_Clock();

// Generate random operation
// check transformability of all three operations
    Vec_IntPush( (Valid_Ops), -1);
    nGain = Rwr_NodeRewrite( pManRwr, pManCutRwr, pNode, fUpdateLevel, fUseZeros_rwr, fPlaceEnable );
    Vec_IntPush( (*pGain_rwr), nGain);
    if (nGain > 0 || (nGain == 0 && fUseZeros_rwr))
    {
        Vec_IntPush( (Valid_Ops), 0);
    }
    vLeaves = Abc_NodeFindCut( pManCutRes, pNode, 0 ); 
    pManRes->timeCut += Abc_Clock() - clk;
    if ( pManOdc )
    {
clk = Abc_Clock();
        Abc_NtkDontCareClear( pManOdc );
        Abc_NtkDontCareCompute( pManOdc, pNode, vLeaves, pManRes->pCareSet );
pManRes->timeTruth += Abc_Clock() - clk;
    }
clk = Abc_Clock();
    pFFormRes = Abc_ManResubEval( pManRes, pNode, vLeaves, nStepsMax, fUpdateLevel, fVerbose );
pManRes->timeRes += Abc_Clock() - clk;
    Vec_IntPush((*pGain_res), pManRes->nLastGain);
    if (pManRes->nLastGain > 0)
    {
        if ( pFFormRes != NULL ){
        Vec_IntPush( (Valid_Ops), 1);
        }
    }
    
    vFanins = Abc_NodeFindCut( pManCutRef, pNode, fUseDcs );
pManRef->timeCut += Abc_Clock() - clk;
clk = Abc_Clock();
    pFFormRef = Abc_NodeRefactor_1( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros_ref, fUseDcs, fVerbose );
pManRef->timeRes += Abc_Clock() - clk;

    Vec_IntPush((*pGain_ref), pManRef->nLastGain);
    if (pManRef->nLastGain > 0 || (pManRef->nLastGain ==0 && fUseZeros_ref))
    {
         if ( pFFormRef != NULL ){
             Vec_IntPush( (Valid_Ops), 2);
         }
    }
    Valid_Len = (Valid_Ops)->nSize;
    //printf("The length of valid operations: %d\n", Valid_Len);

//Pick a random operations from valid ones
if (Rand_Seed == -1)
{
    srand(time(NULL));
}
else
{
    srand(Rand_Seed);
}

int r = rand() % Valid_Len;

    if ((Valid_Ops)->pArray[r] == -1){ 
        (*DecisionMask)->pArray[iterNode] = -1;
        ops_null++;
    Vec_IntZero(Valid_Ops); // reset updates
        continue;
    }
    else if ((Valid_Ops->pArray[r]) == 0){
    // apply rewrite
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);
        if ( fPlaceEnable )
            Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
        if ( fCompl ) Dec_GraphComplement( pGraph );
        (*DecisionMask)->pArray[iterNode] = 0;
        ops_rwr++;
    Vec_IntZero(Valid_Ops); // reset updates
        continue;
    }
    else if ((Valid_Ops->pArray[r] == 1)){
    // apply res
        pManRes->nTotalGain += pManRes->nLastGain;
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pFFormRes, fUpdateLevel, pManRes->nLastGain );
pManRes->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRes );
        (*DecisionMask)->pArray[iterNode] = 1;
        ops_res++;
    Vec_IntZero(Valid_Ops); // reset updates
        continue;
    }
    else if ((Valid_Ops->pArray[r] == 2)){
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFFormRef, fUpdateLevel, pManRef->nLastGain ) )
                 {
                     Dec_GraphFree( pFFormRef );
                     RetValue = -1;
                     break;
                 }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFFormRef );
        (*DecisionMask)->pArray[iterNode] = 2;
        ops_ref++;
    Vec_IntZero(Valid_Ops); // reset updates
        continue;
      }
    }
    //fwrite((**DecisionMask).pArray, sizeof(int), sizeof((**DecisionMask).pArray), fpt);
    for (int i = 0; i < (nNodes); i++){
        fprintf(fpt, "%d\n", (*DecisionMask)->pArray[i]);}
    fclose(fpt);
/*
    printf("size of vector %d\n", Valid_Len);
    printf("Nodes with rewrite: %d\n", ops_rwr);
    printf("Nodes with resub: %d\n", ops_res);
    printf("Nodes with refactor: %d\n", ops_ref);
    printf("Nodes without updates: %d\n", ops_null);
*/
    Extra_ProgressBarStop( pProgress );
// Rewrite
Rwr_ManAddTimeTotal( pManRwr, Abc_Clock() - clkStart );
    pManRwr->nNodesEnd = Abc_NtkNodeNum(pNtk);

// Resub
pManRes->timeTotal = Abc_Clock() - clkStart;
    pManRes->nNodesEnd = Abc_NtkNodeNum(pNtk);

// Refactor
pManRef->timeTotal = Abc_Clock() - clkStart;
    pManRef->nNodesEnd = Abc_NtkNodeNum(pNtk);

    // print statistics
    if ( fVerbose ){
        Abc_ManResubPrint( pManRes );
        Rwr_ManPrintStats( pManRwr );
        Abc_NtkManRefPrintStats_1( pManRef );
    }
    if ( fVeryVerbose )
        Rwr_ScoresReport( pManRwr );
    // delete the managers
    // resub
    Abc_ManResubStop( pManRes );
    Abc_NtkManCutStop( pManCutRes );
    // rewrite
    Rwr_ManStop( pManRwr );
    Cut_ManStop( pManCutRwr );
    pNtk->pManCut = NULL;
    // refactor
    Abc_NtkManCutStop( pManCutRef );
    Abc_NtkManRefStop_1( pManRef );

    if ( pManOdc ) Abc_NtkDontCareFree( pManOdc );

    // clean the data field
    Abc_NtkForEachObj( pNtk, pNode, i )
        pNode->pData = NULL;

    if ( Abc_NtkLatchNum(pNtk) ) {
        Abc_NtkForEachLatch(pNtk, pNode, i)
            pNode->pData = pNode->pNext, pNode->pNext = NULL;
    }

    // put the nodes into the DFS order and reassign their IDs
    Abc_NtkReassignIds( pNtk );
//    Abc_AigCheckFaninOrder( pNtk->pManFunc );

    // fix the levels
    if ( fUpdateLevel )
        Abc_NtkStopReverseLevels( pNtk );
    else
        Abc_NtkLevel( pNtk );
    // check
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Abc_NtkOchestraction: The network check has failed.\n" );
        return 0;
    }
    nNodes_after = Abc_NtkObjNumMax(pNtk);
    //printf("nNodes after optimization: %d\n", nNodes_after);
//s_ResubTime = Abc_Clock() - clkStart;
//clock_t end=clock();
//double time_spent = (double)(end-begin)/CLOCKS_PER_SEC;
//printf("time %f\n", time_spent);
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

