/**CFile****************************************************************

  FileName    [abcRec.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Record of semi-canonical AIG subgraphs.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcRec.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "if.h"
#include "kit.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Abc_ManRec_t_ Abc_ManRec_t;
struct Abc_ManRec_t_
{
    Abc_Ntk_t *       pNtk;          // the record
    Vec_Ptr_t *       vTtElems;      // the elementary truth tables
    Vec_Ptr_t *       vTtNodes;      // the node truth tables
    Abc_Obj_t **      pBins;         // hash table mapping truth tables into nodes
    int               nBins;         // the number of allocated bins
    int               nVars;         // the number of variables
    int               nVarsInit;     // the number of variables requested initially
    int               nWords;        // the number of TT words
    int               nCuts;         // the max number of cuts to use
    // temporaries
    int *             pBytes;        // temporary storage for minterms
    int *             pMints;        // temporary storage for minterm counters
    unsigned *        pTemp1;        // temporary truth table
    unsigned *        pTemp2;        // temporary truth table
    Vec_Ptr_t *       vNodes;        // the temporary nodes
    Vec_Ptr_t *       vTtTemps;      // the truth tables for the internal nodes of the cut
    Vec_Ptr_t *       vLabels;       // temporary storage for AIG node labels
    Vec_Str_t *       vCosts;        // temporary storage for costs
    Vec_Int_t *       vMemory;       // temporary memory for truth tables
    // statistics
    int               nTried;        // the number of cuts tried
    int               nFilterSize;   // the number of same structures
    int               nFilterRedund; // the number of same structures
    int               nFilterVolume; // the number of same structures
    int               nFilterTruth;  // the number of same structures
    int               nFilterError;  // the number of same structures
    int               nFilterSame;   // the number of same structures
    int               nAdded;        // the number of subgraphs added
    int               nAddedFuncs;   // the number of functions added
    // rewriting
    int               nFunsFound;    // the found functions
    int               nFunsNotFound; // the missing functions 
    // runtime
    int               timeCollect;   // the runtime to canonicize
    int               timeTruth;     // the runtime to canonicize
    int               timeCanon;     // the runtime to canonicize
    int               timeOther;     // the runtime to canonicize
    int               timeTotal;     // the runtime to canonicize
};

// the truth table is canonicized in such a way that for (00000) its value is 0

static Abc_Obj_t ** Abc_NtkRecTableLookup( Abc_ManRec_t * p, unsigned * pTruth, int nVars );
static int          Abc_NtkRecComputeTruth( Abc_Obj_t * pObj, Vec_Ptr_t * vTtNodes, int nVars );
static int          Abc_NtkRecAddCutCheckCycle_rec( Abc_Obj_t * pRoot, Abc_Obj_t * pObj );

static Abc_ManRec_t * s_pMan = NULL;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the record for the given network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRecIsRunning()
{
    return s_pMan != NULL;
}

/**Function*************************************************************

  Synopsis    [Starts the record for the given network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRecVarNum()
{
    return (s_pMan != NULL)? s_pMan->nVars : -1;
}

/**Function*************************************************************

  Synopsis    [Starts the record for the given network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkRecMemory()
{
    return s_pMan->vMemory;
}

/**Function*************************************************************

  Synopsis    [Starts the record for the given network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRecStart( Abc_Ntk_t * pNtk, int nVars, int nCuts )
{
    Abc_ManRec_t * p;
    Abc_Obj_t * pObj, ** ppSpot;
    char Buffer[10];
    unsigned * pTruth;
    int i, RetValue;
    int clkTotal = clock(), clk;

    assert( s_pMan == NULL );
    if ( pNtk == NULL )
    {
        assert( nVars > 2 && nVars <= 16 );
        pNtk = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
        pNtk->pName = Extra_UtilStrsav( "record" );
    }
    else
    {
        if ( Abc_NtkGetChoiceNum(pNtk) > 0 )
        {
            printf( "The starting record should be a network without choice nodes.\n" );
            return;
        }
        if ( Abc_NtkPiNum(pNtk) > 16 )
        {
            printf( "The starting record should be a network with no more than %d primary inputs.\n", 16 );
            return;
        }
        if ( Abc_NtkPiNum(pNtk) > nVars )
            printf( "The starting record has %d inputs (warning only).\n", Abc_NtkPiNum(pNtk) );
        pNtk = Abc_NtkDup( pNtk );
    }
    // create the primary inputs
    for ( i = Abc_NtkPiNum(pNtk); i < nVars; i++ )
    {
        pObj = Abc_NtkCreatePi( pNtk );
        Buffer[0] = 'a' + i;
        Buffer[1] = 0;
        Abc_ObjAssignName( pObj, Buffer, NULL );
    }
    Abc_NtkCleanCopy( pNtk );
    Abc_NtkCleanEquiv( pNtk );

    // start the manager
    p = ALLOC( Abc_ManRec_t, 1 );
    memset( p, 0, sizeof(Abc_ManRec_t) );
    p->pNtk = pNtk;
    p->nVars = Abc_NtkPiNum(pNtk);
    p->nWords = Kit_TruthWordNum( p->nVars );
    p->nCuts = nCuts;
    p->nVarsInit = nVars;

    // create elementary truth tables
    p->vTtElems = Vec_PtrAlloc( 0 ); assert( p->vTtElems->pArray == NULL );
    p->vTtElems->nSize = p->nVars;
    p->vTtElems->nCap = p->nVars;
    p->vTtElems->pArray = (void *)Extra_TruthElementary( p->nVars );        

    // allocate room for node truth tables
    if ( Abc_NtkObjNum(pNtk) > (1<<14) )
        p->vTtNodes = Vec_PtrAllocSimInfo( 2 * Abc_NtkObjNum(pNtk), p->nWords );
    else
        p->vTtNodes = Vec_PtrAllocSimInfo( 1<<14, p->nWords );

    // create hash table
    p->nBins = 50011;
    p->pBins = ALLOC( Abc_Obj_t *, p->nBins );
    memset( p->pBins, 0, sizeof(Abc_Obj_t *) * p->nBins );

    // set elementary tables
    Kit_TruthFill( Vec_PtrEntry(p->vTtNodes, 0), p->nVars );
    Abc_NtkForEachPi( pNtk, pObj, i )
        Kit_TruthCopy( Vec_PtrEntry(p->vTtNodes, pObj->Id), Vec_PtrEntry(p->vTtElems, i), p->nVars );

    // compute the tables
clk = clock();
    Abc_AigForEachAnd( pNtk, pObj, i )
    {
        RetValue = Abc_NtkRecComputeTruth( pObj, p->vTtNodes, p->nVars );
        assert( RetValue );
    }
p->timeTruth += clock() - clk;

    // insert the PO nodes into the table
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        p->nTried++;
        p->nAdded++;

        pObj = Abc_ObjFanin0(pObj);
        pTruth = Vec_PtrEntry( p->vTtNodes, pObj->Id );

        if ( pTruth[0] == 1128481603 )
        {
            int x = 0;
        }

        // add the resulting truth table to the hash table 
        ppSpot = Abc_NtkRecTableLookup( p, pTruth, p->nVars );
        assert( pObj->pEquiv == NULL );
        assert( pObj->pCopy == NULL );
        if ( *ppSpot == NULL )
        {
            p->nAddedFuncs++;
            *ppSpot = pObj;
        }
        else
        {
            pObj->pEquiv = (*ppSpot)->pEquiv;
            (*ppSpot)->pEquiv = (Hop_Obj_t *)pObj;
            if ( !Abc_NtkRecAddCutCheckCycle_rec(*ppSpot, pObj) )
                printf( "Loop!\n" );
        }
    }

    // temporaries
    p->pBytes = ALLOC( int, 4*p->nWords );
    p->pMints = ALLOC( int, 2*p->nVars );
    p->pTemp1 = ALLOC( unsigned, p->nWords );
    p->pTemp2 = ALLOC( unsigned, p->nWords );
    p->vNodes = Vec_PtrAlloc( 100 );
    p->vTtTemps = Vec_PtrAllocSimInfo( 64, p->nWords );
    p->vMemory = Vec_IntAlloc( Abc_TruthWordNum(p->nVars) * 1000 );

    // set the manager
    s_pMan = p;
p->timeTotal += clock() - clkTotal;
}

/**Function*************************************************************

  Synopsis    [Returns the given record.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRecStop()
{
    assert( s_pMan != NULL );
    if ( s_pMan->pNtk )
        Abc_NtkDelete( s_pMan->pNtk );
    Vec_PtrFree( s_pMan->vTtNodes );
    Vec_PtrFree( s_pMan->vTtElems );
    free( s_pMan->pBins );

    // temporaries
    free( s_pMan->pBytes );
    free( s_pMan->pMints );
    free( s_pMan->pTemp1 );
    free( s_pMan->pTemp2 );
    Vec_PtrFree( s_pMan->vNodes );
    Vec_PtrFree( s_pMan->vTtTemps );
    if ( s_pMan->vLabels )
        Vec_PtrFree( s_pMan->vLabels );
    if ( s_pMan->vCosts )
        Vec_StrFree( s_pMan->vCosts );
    Vec_IntFree( s_pMan->vMemory );

    free( s_pMan );
    s_pMan = NULL;
}

/**Function*************************************************************

  Synopsis    [Returns the given record.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkRecUse()
{
    Abc_ManRec_t * p = s_pMan;
    Abc_Ntk_t * pNtk = p->pNtk;
    assert( p != NULL );
    Abc_NtkRecPs();
    p->pNtk = NULL;
    Abc_NtkRecStop();
    return pNtk;
}

static inline void Abc_ObjSetMax( Abc_Obj_t * pObj, int Value ) { assert( pObj->Level < 0xff ); pObj->Level = (Value << 8) | (pObj->Level & 0xff); }
static inline void Abc_ObjClearMax( Abc_Obj_t * pObj )          { pObj->Level = (pObj->Level & 0xff); }
static inline int  Abc_ObjGetMax( Abc_Obj_t * pObj )            { return (pObj->Level >> 8) & 0xff; }

/**Function*************************************************************

  Synopsis    [Print statistics about the current record.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRecPs()
{
    int Counter, Counters[17] = {0};
    int CounterS, CountersS[17] = {0};
    Abc_ManRec_t * p = s_pMan;
    Abc_Ntk_t * pNtk = p->pNtk;
    Abc_Obj_t * pObj, * pEntry, * pTemp;
    int i;

    // set the max PI number
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_ObjSetMax( pObj, i+1 );
    Abc_AigForEachAnd( pNtk, pObj, i )
        Abc_ObjSetMax( pObj, ABC_MAX( Abc_ObjGetMax(Abc_ObjFanin0(pObj)), Abc_ObjGetMax(Abc_ObjFanin1(pObj)) ) );
    // go through the table
    Counter = CounterS = 0;
    for ( i = 0; i < p->nBins; i++ )
    for ( pEntry = p->pBins[i]; pEntry; pEntry = pEntry->pCopy )
    {
        Counters[ Abc_ObjGetMax(pEntry) ]++;
        Counter++;
        for ( pTemp = pEntry; pTemp; pTemp = (Abc_Obj_t *)pTemp->pEquiv )
        {
            assert( Abc_ObjGetMax(pTemp) == Abc_ObjGetMax(pEntry) );
            CountersS[ Abc_ObjGetMax(pTemp) ]++;
            CounterS++;
        }
    }
//    printf( "Functions = %d. Expected = %d.\n", Counter, p->nAddedFuncs );
//    printf( "Subgraphs = %d. Expected = %d.\n", CounterS, p->nAdded );
    assert( Counter == p->nAddedFuncs );
    assert( CounterS == p->nAdded );

    // clean
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        Abc_ObjClearMax( pObj );
    }

    printf( "The record with %d AND nodes in %d subgraphs for %d functions with %d inputs:\n", 
        Abc_NtkNodeNum(pNtk), Abc_NtkPoNum(pNtk), p->nAddedFuncs, Abc_NtkPiNum(pNtk) );
    for ( i = 0; i <= 16; i++ )
    {
        if ( Counters[i] )
            printf( "Inputs = %2d.  Funcs = %8d.  Subgrs = %8d.  Ratio = %6.2f.\n", i, Counters[i], CountersS[i], 1.0*CountersS[i]/Counters[i] );
    }

    printf( "Subgraphs tried                             = %8d. (%6.2f %%)\n", p->nTried,        !p->nTried? 0 : 100.0*p->nTried/p->nTried );
    printf( "Subgraphs filtered by support size          = %8d. (%6.2f %%)\n", p->nFilterSize,   !p->nTried? 0 : 100.0*p->nFilterSize/p->nTried );
    printf( "Subgraphs filtered by structural redundancy = %8d. (%6.2f %%)\n", p->nFilterRedund, !p->nTried? 0 : 100.0*p->nFilterRedund/p->nTried );
    printf( "Subgraphs filtered by volume                = %8d. (%6.2f %%)\n", p->nFilterVolume, !p->nTried? 0 : 100.0*p->nFilterVolume/p->nTried );
    printf( "Subgraphs filtered by TT redundancy         = %8d. (%6.2f %%)\n", p->nFilterTruth,  !p->nTried? 0 : 100.0*p->nFilterTruth/p->nTried );
    printf( "Subgraphs filtered by error                 = %8d. (%6.2f %%)\n", p->nFilterError,  !p->nTried? 0 : 100.0*p->nFilterError/p->nTried );
    printf( "Subgraphs filtered by isomorphism           = %8d. (%6.2f %%)\n", p->nFilterSame,   !p->nTried? 0 : 100.0*p->nFilterSame/p->nTried );
    printf( "Subgraphs added                             = %8d. (%6.2f %%)\n", p->nAdded,        !p->nTried? 0 : 100.0*p->nAdded/p->nTried );
    printf( "Functions added                             = %8d. (%6.2f %%)\n", p->nAddedFuncs,   !p->nTried? 0 : 100.0*p->nAddedFuncs/p->nTried );

    p->timeOther = p->timeTotal - p->timeCollect - p->timeTruth - p->timeCanon;
    PRTP( "Collecting nodes ", p->timeCollect, p->timeTotal );
    PRTP( "Computing truth  ", p->timeTruth, p->timeTotal );
    PRTP( "Canonicizing     ", p->timeCanon, p->timeTotal );
    PRTP( "Other            ", p->timeOther, p->timeTotal );
    PRTP( "TOTAL            ", p->timeTotal, p->timeTotal );
    if ( p->nFunsFound )
    printf( "During rewriting found = %d and not found = %d functions.\n", p->nFunsFound, p->nFunsNotFound );
}

/**Function*************************************************************

  Synopsis    [Filters the current record.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRecFilter( int iVar, int iPlus )
{
}

/**Function*************************************************************

  Synopsis    [Returns the hash key.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Abc_NtkRecTableHash( unsigned * pTruth, int nVars, int nBins, int * pPrimes )
{
    int i, nWords = Kit_TruthWordNum( nVars );
    unsigned uHash = 0;
    for ( i = 0; i < nWords; i++ )
        uHash ^= pTruth[i] * pPrimes[i & 0x7];
    return uHash % nBins;
}

/**Function*************************************************************

  Synopsis    [Returns the given record.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t ** Abc_NtkRecTableLookup( Abc_ManRec_t * p, unsigned * pTruth, int nVars )
{
    static int s_Primes[10] = { 1291, 1699, 2357, 4177, 5147, 5647, 6343, 7103, 7873, 8147 };
    Abc_Obj_t ** ppSpot, * pEntry;
    ppSpot = p->pBins + Abc_NtkRecTableHash( pTruth, nVars, p->nBins, s_Primes );
    for ( pEntry = *ppSpot; pEntry; ppSpot = &pEntry->pCopy, pEntry = pEntry->pCopy )
        if ( Kit_TruthIsEqualWithPhase(Vec_PtrEntry(p->vTtNodes, pEntry->Id), pTruth, nVars) )
            return ppSpot;
    return ppSpot;
}

/**Function*************************************************************

  Synopsis    [Computes the truth table of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRecComputeTruth( Abc_Obj_t * pObj, Vec_Ptr_t * vTtNodes, int nVars )
{
    unsigned * pTruth, * pTruth0, * pTruth1;
    int RetValue;
    assert( Abc_ObjIsNode(pObj) );
    pTruth  = Vec_PtrEntry( vTtNodes, pObj->Id );
    pTruth0 = Vec_PtrEntry( vTtNodes, Abc_ObjFaninId0(pObj) );
    pTruth1 = Vec_PtrEntry( vTtNodes, Abc_ObjFaninId1(pObj) );
    Kit_TruthAndPhase( pTruth, pTruth0, pTruth1, nVars, Abc_ObjFaninC0(pObj), Abc_ObjFaninC1(pObj) );
    assert( (pTruth[0] & 1) == pObj->fPhase );
    RetValue = ((pTruth[0] & 1) == pObj->fPhase);
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Performs renoding as technology mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRecAdd( Abc_Ntk_t * pNtk )
{
    extern Abc_Ntk_t * Abc_NtkIf( Abc_Ntk_t * pNtk, If_Par_t * pPars );
    extern int Abc_NtkRecAddCut( If_Man_t * pIfMan, If_Obj_t * pRoot, If_Cut_t * pCut );

    If_Par_t Pars, * pPars = &Pars;
    Abc_Ntk_t * pNtkNew;
    int clk = clock();

    if ( Abc_NtkGetChoiceNum( pNtk ) )
        printf( "Performing renoding with choices.\n" );

    // set defaults
    memset( pPars, 0, sizeof(If_Par_t) );
    // user-controlable paramters
    pPars->nLutSize    =  s_pMan->nVarsInit;
    pPars->nCutsMax    =  s_pMan->nCuts;
    pPars->nFlowIters  =  0;
    pPars->nAreaIters  =  0;
    pPars->DelayTarget = -1;
    pPars->fPreprocess =  0;
    pPars->fArea       =  1;
    pPars->fFancy      =  0;
    pPars->fExpRed     =  0; 
    pPars->fLatchPaths =  0;
    pPars->fSeqMap     =  0;
    pPars->fVerbose    =  0;
    // internal parameters
    pPars->fTruth      =  0;
    pPars->fUsePerm    =  0; 
    pPars->nLatches    =  0;
    pPars->pLutLib     =  NULL; // Abc_FrameReadLibLut();
    pPars->pTimesArr   =  NULL; 
    pPars->pTimesArr   =  NULL;   
    pPars->fUseBdds    =  0;
    pPars->fUseSops    =  0;
    pPars->fUseCnfs    =  0;
    pPars->fUseMv      =  0;
    pPars->pFuncCost   =  NULL;
    pPars->pFuncUser   =  Abc_NtkRecAddCut;

    // perform recording
    pNtkNew = Abc_NtkIf( pNtk, pPars );
    Abc_NtkDelete( pNtkNew );
s_pMan->timeTotal += clock() - clk;

//    if ( !Abc_NtkCheck( s_pMan->pNtk ) )
//        printf( "Abc_NtkRecAdd: The network check has failed.\n" );
}

/**Function*************************************************************

  Synopsis    [Adds the cut function to the internal storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRecCollectNodes_rec( If_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    if ( pNode->fMark )
        return;
    pNode->fMark = 1;
    assert( If_ObjIsAnd(pNode) );
    Abc_NtkRecCollectNodes_rec( If_ObjFanin0(pNode), vNodes );
    Abc_NtkRecCollectNodes_rec( If_ObjFanin1(pNode), vNodes );
    Vec_PtrPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    [Adds the cut function to the internal storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRecCollectNodes( If_Man_t * pIfMan, If_Obj_t * pRoot, If_Cut_t * pCut, Vec_Ptr_t * vNodes )
{
    If_Obj_t * pLeaf;
    int i, RetValue = 1;

    // collect the internal nodes of the cut
    Vec_PtrClear( vNodes );
    If_CutForEachLeaf( pIfMan, pCut, pLeaf, i )
    {
        Vec_PtrPush( vNodes, pLeaf );
        assert( pLeaf->fMark == 0 );
        pLeaf->fMark = 1;
    }

    // collect other nodes
    Abc_NtkRecCollectNodes_rec( pRoot, vNodes );

    // check if there are leaves, such that both of their fanins are marked
    // this indicates a redundant cut
    If_CutForEachLeaf( pIfMan, pCut, pLeaf, i )
    {
        if ( !If_ObjIsAnd(pLeaf) )
            continue;
        if ( If_ObjFanin0(pLeaf)->fMark && If_ObjFanin1(pLeaf)->fMark )
        {
            RetValue = 0;
            break;
        }
    }

    // clean the mark
    Vec_PtrForEachEntry( vNodes, pLeaf, i )
        pLeaf->fMark = 0;
/*
    if ( pRoot->Id == 2639 )
    {
        // print the cut
        Vec_PtrForEachEntry( vNodes, pLeaf, i )
        {
            if ( If_ObjIsAnd(pLeaf) )
                printf( "%4d = %c%4d & %c%4d\n", pLeaf->Id, 
                    (If_ObjFaninC0(pLeaf)? '-':'+'), If_ObjFanin0(pLeaf)->Id, 
                    (If_ObjFaninC1(pLeaf)? '-':'+'), If_ObjFanin1(pLeaf)->Id ); 
            else
                printf( "%4d = pi\n", pLeaf->Id ); 
        }
        printf( "\n" );
    }
*/
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Computes truth tables of nodes in the cut.]

  Description [Returns 0 if the TT does not depend on some cut variables.
  Or if the TT can be expressed simpler using other nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRecCutTruth( Vec_Ptr_t * vNodes, int nLeaves, Vec_Ptr_t * vTtTemps, Vec_Ptr_t * vTtElems )
{
    unsigned * pSims, * pSims0, * pSims1;
    unsigned * pTemp = s_pMan->pTemp2;
    unsigned uWord;
    If_Obj_t * pObj, * pObj2, * pRoot;
    int i, k, nLimit, nInputs = s_pMan->nVars;

    assert( Vec_PtrSize(vNodes) > nLeaves );

    // set the elementary truth tables and compute the truth tables of the nodes
    Vec_PtrForEachEntry( vNodes, pObj, i )
    {
        pObj->pCopy = Vec_PtrEntry(vTtTemps, i);
        pSims = (unsigned *)pObj->pCopy;
        if ( i < nLeaves )
        {
            Kit_TruthCopy( pSims, Vec_PtrEntry(vTtElems, i), nInputs );
            continue;
        }
        assert( If_ObjIsAnd(pObj) );
        // get hold of the simulation information
        pSims0 = (unsigned *)If_ObjFanin0(pObj)->pCopy;
        pSims1 = (unsigned *)If_ObjFanin1(pObj)->pCopy;
        // simulate the node
        Kit_TruthAndPhase( pSims, pSims0, pSims1, nInputs, If_ObjFaninC0(pObj), If_ObjFaninC1(pObj) );
    }

    // check the support size
    pRoot = Vec_PtrEntryLast( vNodes );
    pSims = (unsigned *)pRoot->pCopy;
    if ( Kit_TruthSupport(pSims, nInputs) != Kit_BitMask(nLeaves) )
        return 0;

    // make sure none of the nodes has the same simulation info as the output
    // check pairwise comparisons
    nLimit = Vec_PtrSize(vNodes) - 1;
    Vec_PtrForEachEntryStop( vNodes, pObj, i, nLimit )
    {
        pSims0 = (unsigned *)pObj->pCopy;
        if ( Kit_TruthIsEqualWithPhase(pSims, pSims0, nInputs) )
            return 0;
        Vec_PtrForEachEntryStop( vNodes, pObj2, k, i )
        {
            if ( (If_ObjFanin0(pRoot) == pObj && If_ObjFanin1(pRoot) == pObj2) ||
                 (If_ObjFanin1(pRoot) == pObj && If_ObjFanin0(pRoot) == pObj2) )
                 continue;
            pSims1 = (unsigned *)pObj2->pCopy;

            uWord = pSims0[0] & pSims1[0];
            if ( pSims[0] == uWord || pSims[0] == ~uWord )
            {
                Kit_TruthAndPhase( pTemp, pSims0, pSims1, nInputs, 0, 0 );
                if ( Kit_TruthIsEqualWithPhase(pSims, pTemp, nInputs) )
                    return 0;
            }

            uWord = pSims0[0] & ~pSims1[0];
            if ( pSims[0] == uWord || pSims[0] == ~uWord )
            {
                Kit_TruthAndPhase( pTemp, pSims0, pSims1, nInputs, 0, 1 );
                if ( Kit_TruthIsEqualWithPhase(pSims, pTemp, nInputs) )
                    return 0;
            }

            uWord = ~pSims0[0] & pSims1[0];
            if ( pSims[0] == uWord || pSims[0] == ~uWord )
            {
                Kit_TruthAndPhase( pTemp, pSims0, pSims1, nInputs, 1, 0 );
                if ( Kit_TruthIsEqualWithPhase(pSims, pTemp, nInputs) )
                    return 0;
            }

            uWord = ~pSims0[0] & ~pSims1[0];
            if ( pSims[0] == uWord || pSims[0] == ~uWord )
            {
                Kit_TruthAndPhase( pTemp, pSims0, pSims1, nInputs, 1, 1 );
                if ( Kit_TruthIsEqualWithPhase(pSims, pTemp, nInputs) )
                    return 0;
            }
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Adds the cut function to the internal storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRecAddCutCheckCycle_rec( Abc_Obj_t * pRoot, Abc_Obj_t * pObj )
{
    assert( pRoot->Level > 0 );
    if ( pObj->Level < pRoot->Level )
        return 1;
    if ( pObj == pRoot )
        return 0;
    if ( !Abc_NtkRecAddCutCheckCycle_rec(pRoot, Abc_ObjFanin0(pObj)) )
        return 0;
    if ( !Abc_NtkRecAddCutCheckCycle_rec(pRoot, Abc_ObjFanin1(pObj)) )
        return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Adds the cut function to the internal storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRecAddCut( If_Man_t * pIfMan, If_Obj_t * pRoot, If_Cut_t * pCut )
{
    static int s_MaxSize[16] = { 0 };
    char Buffer[40], Name[20], Truth[20];
    char pCanonPerm[16];
    Abc_Obj_t * pObj, * pFanin0, * pFanin1, ** ppSpot, * pObjPo;
    Abc_Ntk_t * pAig = s_pMan->pNtk;
    If_Obj_t * pIfObj;
    Vec_Ptr_t * vNodes = s_pMan->vNodes;
    unsigned * pInOut = s_pMan->pTemp1;
    unsigned * pTemp = s_pMan->pTemp2;
    unsigned * pTruth;
    int i, RetValue, nNodes, nNodesBeg, nInputs = s_pMan->nVars, nLeaves = If_CutLeaveNum(pCut);
    unsigned uCanonPhase;
    int clk;

    if ( pRoot->Id == 2639 )
    {
        int y = 0;
    }

    assert( nInputs <= 16 );
    assert( nInputs == (int)pCut->nLimit );
    s_pMan->nTried++;

    // skip small cuts
    if ( nLeaves < 3 )
    {
        s_pMan->nFilterSize++;
        return 1;
    }

    // collect internal nodes and skip redundant cuts
clk = clock();
    RetValue = Abc_NtkRecCollectNodes( pIfMan, pRoot, pCut, vNodes );
s_pMan->timeCollect += clock() - clk;
    if ( !RetValue )
    {
        s_pMan->nFilterRedund++;
        return 1;
    }

    // skip cuts with very large volume
    if ( Vec_PtrSize(vNodes) > nLeaves + 3*(nLeaves-1) + s_MaxSize[nLeaves] )
    {
        s_pMan->nFilterVolume++;
        return 1;
    }

    // compute truth table and skip the redundant structures
clk = clock();
    RetValue = Abc_NtkRecCutTruth( vNodes, nLeaves, s_pMan->vTtTemps, s_pMan->vTtElems );
s_pMan->timeTruth += clock() - clk;
    if ( !RetValue )
    {
        s_pMan->nFilterTruth++;
        return 1;
    }

    // copy the truth table
    Kit_TruthCopy( pInOut, (unsigned *)pRoot->pCopy, nInputs );

    // set permutation
    for ( i = 0; i < nInputs; i++ )
        pCanonPerm[i] = i;

    // semi-canonicize the truth table
clk = clock();
    uCanonPhase = Kit_TruthSemiCanonicize( pInOut, pTemp, nInputs, pCanonPerm, (short *)s_pMan->pMints );
s_pMan->timeCanon += clock() - clk;
    // pCanonPerm and uCanonPhase show what was the variable corresponding to each var in the current truth

    // go through the variables in the new truth table
    for ( i = 0; i < nLeaves; i++ )
    {
        // get hold of the corresponding leaf
        pIfObj = If_ManObj( pIfMan, pCut->pLeaves[pCanonPerm[i]] );
        // get hold of the corresponding new node
        pObj = Abc_NtkPi( pAig, i );
        pObj = Abc_ObjNotCond( pObj, (uCanonPhase & (1 << i)) );
        // map them
        pIfObj->pCopy = pObj;
/*
        if ( pRoot->Id == 2639 )
        {
            unsigned uSupp;
            printf( "Node %6d : ", pIfObj->Id );
            printf( "Support " );
            uSupp = Kit_TruthSupport(Vec_PtrEntry( s_pMan->vTtNodes, Abc_ObjRegular(pObj)->Id ), nInputs);
            Extra_PrintBinary( stdout, &uSupp, nInputs ); 
            printf( "   " );
            Extra_PrintBinary( stdout, Vec_PtrEntry( s_pMan->vTtNodes, Abc_ObjRegular(pObj)->Id ), 1<<6 ); 
            printf( "\n" );
        }
*/
    }

    // build the node and compute its truth table
    nNodesBeg = Abc_NtkObjNumMax( pAig );
    Vec_PtrForEachEntryStart( vNodes, pIfObj, i, nLeaves )
    {
        pFanin0 = Abc_ObjNotCond( If_ObjFanin0(pIfObj)->pCopy, If_ObjFaninC0(pIfObj) );
        pFanin1 = Abc_ObjNotCond( If_ObjFanin1(pIfObj)->pCopy, If_ObjFaninC1(pIfObj) );

        nNodes = Abc_NtkObjNumMax( pAig );
        pObj = Abc_AigAnd( pAig->pManFunc, pFanin0, pFanin1 );
        assert( !Abc_ObjIsComplement(pObj) );
        pIfObj->pCopy = pObj;

        if ( pObj->Id == nNodes )
        {
            // increase storage for truth tables
            if ( Vec_PtrSize(s_pMan->vTtNodes) <= pObj->Id )
                Vec_PtrDoubleSimInfo(s_pMan->vTtNodes);
            // compute the truth table
            RetValue = Abc_NtkRecComputeTruth( pObj, s_pMan->vTtNodes, nInputs );
            if ( RetValue == 0 )
            {
                s_pMan->nFilterError++;
                printf( "T" );
                return 1;
            }
        }
    }

    pTruth = Vec_PtrEntry( s_pMan->vTtNodes, pObj->Id );
    if ( Kit_TruthSupport(pTruth, nInputs) != Kit_BitMask(nLeaves) )
    {
        s_pMan->nFilterError++;
        printf( "S" );
        return 1;
    }

    // compare the truth tables
    if ( !Kit_TruthIsEqualWithPhase( pTruth, pInOut, nInputs ) )
    {
        s_pMan->nFilterError++;
        printf( "F" );
        return 1;
    }
//    Extra_PrintBinary( stdout, pInOut, 8 ); printf( "\n" );

    // if not new nodes were added and the node has a CO fanout
    if ( nNodesBeg == Abc_NtkObjNumMax(pAig) && Abc_NodeFindCoFanout(pObj) != NULL )
    {
        s_pMan->nFilterSame++;
        return 1;
    }
    s_pMan->nAdded++;

    // create PO for this node
    pObjPo = Abc_NtkCreatePo(pAig);
    Abc_ObjAddFanin( pObjPo, pObj );

    // assign the name to this PO
    sprintf( Name, "%d_%06d", nLeaves, Abc_NtkPoNum(pAig) );
    if ( (nInputs <= 6) && 0 )
    {
        Extra_PrintHexadecimalString( Truth, pInOut, nInputs );
        sprintf( Buffer, "%s_%s", Name, Truth );
    }
    else
    {
        sprintf( Buffer, "%s", Name );
    }
    Abc_ObjAssignName( pObjPo, Buffer, NULL );

    // add the resulting truth table to the hash table 
    ppSpot = Abc_NtkRecTableLookup( s_pMan, pTruth, nInputs );
    assert( pObj->pEquiv == NULL );
    assert( pObj->pCopy == NULL );
    if ( *ppSpot == NULL )
    {
        s_pMan->nAddedFuncs++;
        *ppSpot = pObj;
    }
    else
    {
        pObj->pEquiv = (*ppSpot)->pEquiv;
        (*ppSpot)->pEquiv = (Hop_Obj_t *)pObj;
        if ( !Abc_NtkRecAddCutCheckCycle_rec(*ppSpot, pObj) )
            printf( "Loop!\n" );
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    [Labels the record AIG with the corresponding new AIG nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkRecStrashNodeLabel_rec( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pObj, int fBuild, Vec_Ptr_t * vLabels )
{
    Abc_Obj_t * pFanin0New, * pFanin1New, * pLabel;
    assert( !Abc_ObjIsComplement(pObj) );
    // if this node is already visited, skip
    if ( Abc_NodeIsTravIdCurrent( pObj ) )
        return Vec_PtrEntry( vLabels, pObj->Id );
    assert( Abc_ObjIsNode(pObj) );
    // mark the node as visited
    Abc_NodeSetTravIdCurrent( pObj );
    // label the fanins
    pFanin0New = Abc_NtkRecStrashNodeLabel_rec( pNtkNew, Abc_ObjFanin0(pObj), fBuild, vLabels );
    pFanin1New = Abc_NtkRecStrashNodeLabel_rec( pNtkNew, Abc_ObjFanin1(pObj), fBuild, vLabels );
    // label the node if possible
    pLabel = NULL;
    if ( pFanin0New && pFanin1New )
    {
        pFanin0New = Abc_ObjNotCond( pFanin0New, Abc_ObjFaninC0(pObj) );
        pFanin1New = Abc_ObjNotCond( pFanin1New, Abc_ObjFaninC1(pObj) );
        if ( fBuild )
            pLabel = Abc_AigAnd( pNtkNew->pManFunc, pFanin0New, pFanin1New );
        else
            pLabel = Abc_AigAndLookup( pNtkNew->pManFunc, pFanin0New, pFanin1New );
    }
    Vec_PtrWriteEntry( vLabels, pObj->Id, pLabel );
    return pLabel;
}

/**Function*************************************************************

  Synopsis    [Counts the area of the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRecStrashNodeCount_rec( Abc_Obj_t * pObj, Vec_Str_t * vCosts, Vec_Ptr_t * vLabels )
{
    int Cost0, Cost1;
    if ( Vec_PtrEntry( vLabels, pObj->Id ) )
        return 0;
    assert( Abc_ObjIsNode(pObj) );
    // if this node is already visited, skip
    if ( Abc_NodeIsTravIdCurrent( pObj ) )
        return Vec_StrEntry( vCosts, pObj->Id );
    // mark the node as visited
    Abc_NodeSetTravIdCurrent( pObj );
    // count for the fanins
    Cost0 = Abc_NtkRecStrashNodeCount_rec( Abc_ObjFanin0(pObj), vCosts, vLabels );
    Cost1 = Abc_NtkRecStrashNodeCount_rec( Abc_ObjFanin1(pObj), vCosts, vLabels );
    Vec_StrWriteEntry( vCosts, pObj->Id, (char)(Cost0 + Cost1 + 1) );
    return Cost0 + Cost1 + 1;
}

/**Function*************************************************************

  Synopsis    [Strashes the given node using its local function.]

  Description [Assumes that the fanins are already strashed.
  Returns 0 if the function is not found in the table.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRecStrashNode( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pObj, unsigned * pTruth, int nVars )
{
    char pCanonPerm[16];
    Abc_Ntk_t * pAig = s_pMan->pNtk;
    unsigned * pInOut = s_pMan->pTemp1;
    unsigned * pTemp = s_pMan->pTemp2;
    unsigned * pTruthRec;
    Abc_Obj_t * pCand, * pCandMin, * pLeaf, * pFanin, ** ppSpot;
    unsigned uCanonPhase;
    int i, nLeaves, CostMin, Cost, nOnes, fCompl;

    // check if the record works
    nLeaves = Abc_ObjFaninNum(pObj);
    assert( nLeaves >= 3 && nLeaves <= s_pMan->nVars );
    pFanin = Abc_ObjFanin0(pObj);
    assert( Abc_ObjRegular(pFanin->pCopy)->pNtk == pNtkNew );
    assert( s_pMan != NULL );
    assert( nVars == s_pMan->nVars );

    // copy the truth table
    Kit_TruthCopy( pInOut, pTruth, nVars );

    // set permutation
    for ( i = 0; i < nVars; i++ )
        pCanonPerm[i] = i;

    // canonicize the truth table
    uCanonPhase = Kit_TruthSemiCanonicize( pInOut, pTemp, nVars, pCanonPerm, (short *)s_pMan->pMints );

    // get hold of the curresponding class
    ppSpot = Abc_NtkRecTableLookup( s_pMan, pInOut, nVars );
    if ( *ppSpot == NULL )
    {
        s_pMan->nFunsNotFound++;        
//        printf( "The class of a function with %d inputs is not found.\n", nLeaves );
        return 0;
    }
    s_pMan->nFunsFound++;        

    // make sure the truth table is the same
    pTruthRec = Vec_PtrEntry( s_pMan->vTtNodes, (*ppSpot)->Id );
    if ( !Kit_TruthIsEqualWithPhase( pTruthRec, pInOut, nVars ) )
    {
        assert( 0 );
        return 0;
    }


    // allocate storage for costs
    if ( s_pMan->vLabels && Vec_PtrSize(s_pMan->vLabels) < Abc_NtkObjNumMax(pAig) )
    {
        Vec_PtrFree( s_pMan->vLabels );
        s_pMan->vLabels = NULL;
    }
    if ( s_pMan->vLabels == NULL )
        s_pMan->vLabels = Vec_PtrStart( Abc_NtkObjNumMax(pAig) );

    // go through the variables in the new truth table
    Abc_NtkIncrementTravId( pAig );
    for ( i = 0; i < nLeaves; i++ )
    {
        // get hold of the corresponding fanin
        pFanin = Abc_ObjFanin( pObj, pCanonPerm[i] )->pCopy;
        pFanin = Abc_ObjNotCond( pFanin, (uCanonPhase & (1 << i)) );
        // label the PI of the AIG subgraphs with this fanin
        pLeaf = Abc_NtkPi( pAig, i );
        Vec_PtrWriteEntry( s_pMan->vLabels, pLeaf->Id, pFanin );
        Abc_NodeSetTravIdCurrent( pLeaf );
    }

    // go through the candidates - and recursively label them
    for ( pCand = *ppSpot; pCand; pCand = (Abc_Obj_t *)pCand->pEquiv )
        Abc_NtkRecStrashNodeLabel_rec( pNtkNew, pCand, 0, s_pMan->vLabels );


    // allocate storage for costs
    if ( s_pMan->vCosts && Vec_StrSize(s_pMan->vCosts) < Abc_NtkObjNumMax(pAig) )
    {
        Vec_StrFree( s_pMan->vCosts );
        s_pMan->vCosts = NULL;
    }
    if ( s_pMan->vCosts == NULL )
        s_pMan->vCosts = Vec_StrStart( Abc_NtkObjNumMax(pAig) );

    // find the best subgraph
    CostMin = ABC_INFINITY;
    pCandMin = NULL;
    for ( pCand = *ppSpot; pCand; pCand = (Abc_Obj_t *)pCand->pEquiv )
    {
        // label the leaves
        Abc_NtkIncrementTravId( pAig );
        // count the number of non-labeled nodes
        Cost = Abc_NtkRecStrashNodeCount_rec( pCand, s_pMan->vCosts, s_pMan->vLabels );
        if ( CostMin > Cost )
        {
//            printf( "%d ", Cost );
            CostMin = Cost;
            pCandMin = pCand;
        }
    }
//    printf( "\n" );
    assert( pCandMin != NULL );
    if ( pCandMin == NULL )
        return 0;


    // label the leaves
    Abc_NtkIncrementTravId( pAig );
    for ( i = 0; i < nLeaves; i++ )
        Abc_NodeSetTravIdCurrent( Abc_NtkPi(pAig, i) );

    // implement the subgraph
    pObj->pCopy = Abc_NtkRecStrashNodeLabel_rec( pNtkNew, pCandMin, 1, s_pMan->vLabels );
    assert( Abc_ObjRegular(pObj->pCopy)->pNtk == pNtkNew );

    // determine phase difference
    nOnes = Kit_TruthCountOnes(pTruth, nVars);
    fCompl = (nOnes > (1<< nVars)/2);
//    assert( fCompl == ((uCanonPhase & (1 << nVars)) > 0) );

    nOnes = Kit_TruthCountOnes(pTruthRec, nVars);
    fCompl ^= (nOnes > (1<< nVars)/2);
    // complement
    pObj->pCopy = Abc_ObjNotCond( pObj->pCopy, fCompl );
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


