/**CFile****************************************************************

  FileName    [abcLut.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Superchoicing for K-LUTs.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcLut.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "opt/cut/cut.h"

ABC_NAMESPACE_IMPL_START

#define LARGE_LEVEL 1000000

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define SCL_LUT_MAX          6   // the maximum LUT size
#define SCL_VARS_MAX        15   // the maximum number of variables
#define SCL_NODE_MAX      1000   // the maximum number of nodes

typedef struct Abc_ManScl_t_ Abc_ManScl_t;
struct Abc_ManScl_t_
{
    // paramers
    int                nLutSize;    // the LUT size
    int                nCutSizeMax; // the max number of leaves of the cone
    int                nNodesMax;   // the max number of divisors in the cone
    int                nWords;      // the number of machine words in sim info
    // structural representation of the cone
    Vec_Ptr_t *        vLeaves;     // leaves of the cut
    Vec_Ptr_t *        vVolume;     // volume of the cut
    int                pBSet[SCL_VARS_MAX]; // bound set
    // functional representation of the cone
    unsigned *         uTruth;      // truth table of the cone
    // representation of truth tables
    unsigned **        uVars;       // elementary truth tables
    unsigned **        uSims;       // truth tables of the nodes
    unsigned **        uCofs;       // truth tables of the cofactors
};

static Vec_Ptr_t * s_pLeaves = NULL;

static Cut_Man_t * Abc_NtkStartCutManForScl( Abc_Ntk_t * pNtk, int nLutSize );
static Abc_ManScl_t * Abc_ManSclStart( int nLutSize, int nCutSizeMax, int nNodesMax );
static void Abc_ManSclStop( Abc_ManScl_t * p );
static void Abc_NodeLutMap( Cut_Man_t * pManCuts, Abc_Obj_t * pObj );

static Abc_Obj_t * Abc_NodeSuperChoiceLut( Abc_ManScl_t * pManScl, Abc_Obj_t * pObj );
static int Abc_NodeDecomposeStep( Abc_ManScl_t * pManScl );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs superchoicing for K-LUTs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkSuperChoiceLut( Abc_Ntk_t * pNtk, int nLutSize, int nCutSizeMax, int fVerbose )
{
    ProgressBar * pProgress;
    Abc_ManCut_t * pManCut;
    Abc_ManScl_t * pManScl;
    Cut_Man_t * pManCuts;
    Abc_Obj_t * pObj, * pFanin, * pObjTop;
    int i, LevelMax, nNodes;
    int nNodesTried, nNodesDec, nNodesExist, nNodesUsed;

    assert( Abc_NtkIsSopLogic(pNtk) );
    if ( nLutSize < 3 || nLutSize > SCL_LUT_MAX )
    {
        printf( "LUT size (%d) does not belong to the interval: 3 <= LUT size <= %d\n", nLutSize, SCL_LUT_MAX );
        return 0;
    }
    if ( nCutSizeMax <= nLutSize || nCutSizeMax > SCL_VARS_MAX )
    {
        printf( "Cut size (%d) does not belong to the interval: LUT size (%d) < Cut size <= %d\n", nCutSizeMax, nLutSize, SCL_VARS_MAX );
        return 0;
    }

    assert( nLutSize <= SCL_LUT_MAX );
    assert( nCutSizeMax <= SCL_VARS_MAX );
    nNodesTried = nNodesDec = nNodesExist = nNodesUsed = 0;

    // set the delays of the CIs
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->Level = 0;

//Abc_NtkLevel( pNtk );
 
    // start the managers
    pManScl = Abc_ManSclStart( nLutSize, nCutSizeMax, 1000 );
    pManCuts = Abc_NtkStartCutManForScl( pNtk, nLutSize );
    pManCut = Abc_NtkManCutStart( nCutSizeMax, 100000, 100000, 100000 );
    s_pLeaves = Abc_NtkManCutReadCutSmall( pManCut );
    pManScl->vVolume = Abc_NtkManCutReadVisited( pManCut );

    // process each internal node (assuming topological order of nodes!!!)
    nNodes = Abc_NtkObjNumMax(pNtk);
    pProgress = Extra_ProgressBarStart( stdout, nNodes );
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
//        if ( i != nNodes-1 )
//            continue;
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        if ( i >= nNodes )
            break;
        if ( Abc_ObjFaninNum(pObj) != 2 )
            continue;
        nNodesTried++;

        // map this node using regular cuts
//        pObj->Level = 0;
        Abc_NodeLutMap( pManCuts, pObj );
        // compute the cut
        pManScl->vLeaves = Abc_NodeFindCut( pManCut, pObj, 0 );
        if ( Vec_PtrSize(pManScl->vLeaves) <= nLutSize )
            continue;
        // get the volume of the cut
        if ( Vec_PtrSize(pManScl->vVolume) > SCL_NODE_MAX )
            continue;
        nNodesDec++;

        // decompose the cut
        pObjTop = Abc_NodeSuperChoiceLut( pManScl, pObj );
        if ( pObjTop == NULL )
            continue;
        nNodesExist++;

        // if there is no delay improvement, skip; otherwise, update level
        if ( pObjTop->Level >= pObj->Level )
        {
            Abc_NtkDeleteObj_rec( pObjTop, 1 );
            continue;
        }
        pObj->Level = pObjTop->Level;
        nNodesUsed++;
    }
    Extra_ProgressBarStop( pProgress );

    // delete the managers
    Abc_ManSclStop( pManScl );
    Abc_NtkManCutStop( pManCut );
    Cut_ManStop( pManCuts );

    // get the largest arrival time
    LevelMax = 0;
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        pFanin = Abc_ObjFanin0( pObj );
        // skip inv/buf
        if ( Abc_ObjFaninNum(pFanin) == 1 )
            pFanin = Abc_ObjFanin0( pFanin );
        // get the new level
        LevelMax = Abc_MaxInt( LevelMax, (int)pFanin->Level );
    }

    if ( fVerbose )
    printf( "Try = %d. Dec = %d. Exist = %d. Use = %d. SUPER = %d levels of %d-LUTs.\n", 
        nNodesTried, nNodesDec, nNodesExist, nNodesUsed, LevelMax, nLutSize );
//    if ( fVerbose )
//    printf( "The network is superchoiced for %d levels of %d-LUTs.\n", LevelMax, nLutSize );

    // clean the data field
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->pNext = NULL;

    // check
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Abc_NtkSuperChoiceLut: The network check has failed.\n" );
        return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Performs LUT mapping of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeLutMap( Cut_Man_t * pManCuts, Abc_Obj_t * pObj )
{
    Cut_Cut_t * pCut;
    Abc_Obj_t * pFanin;
    int i, DelayMax;
    pCut = (Cut_Cut_t *)Abc_NodeGetCutsRecursive( pManCuts, pObj, 0, 0 );
    assert( pCut != NULL );
    assert( pObj->Level == 0 );
    // go through the cuts
    pObj->Level = LARGE_LEVEL;
    for ( pCut = pCut->pNext; pCut; pCut = pCut->pNext )
    {
        DelayMax = 0;
        for ( i = 0; i < (int)pCut->nLeaves; i++ )
        {
            pFanin = Abc_NtkObj( pObj->pNtk, pCut->pLeaves[i] );
//            assert( Abc_ObjIsCi(pFanin) || pFanin->Level > 0 ); // should hold if node ordering is topological
            if ( DelayMax < (int)pFanin->Level )
                DelayMax = pFanin->Level;
        }
        if ( (int)pObj->Level > DelayMax )
            pObj->Level = DelayMax;
    }
    assert( pObj->Level < LARGE_LEVEL );
    pObj->Level++;
//    printf( "%d(%d) ", pObj->Id, pObj->Level );
}

/**Function*************************************************************

  Synopsis    [Starts the cut manager for rewriting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cut_Man_t * Abc_NtkStartCutManForScl( Abc_Ntk_t * pNtk, int nLutSize )
{
    static Cut_Params_t Params, * pParams = &Params;
    Cut_Man_t * pManCut;
    Abc_Obj_t * pObj;
    int i;
    // start the cut manager
    memset( pParams, 0, sizeof(Cut_Params_t) );
    pParams->nVarsMax  = nLutSize; // the max cut size ("k" of the k-feasible cuts)
    pParams->nKeepMax  = 500;   // the max number of cuts kept at a node
    pParams->fTruth    = 0;     // compute truth tables
    pParams->fFilter   = 1;     // filter dominated cuts
    pParams->fSeq      = 0;     // compute sequential cuts
    pParams->fDrop     = 0;     // drop cuts on the fly
    pParams->fVerbose  = 0;     // the verbosiness flag
    pParams->nIdsMax   = Abc_NtkObjNumMax( pNtk );
    pManCut = Cut_ManStart( pParams );
    if ( pParams->fDrop )
        Cut_ManSetFanoutCounts( pManCut, Abc_NtkFanoutCounts(pNtk) );
    // set cuts for PIs
    Abc_NtkForEachCi( pNtk, pObj, i )
        if ( Abc_ObjFanoutNum(pObj) > 0 )
            Cut_NodeSetTriv( pManCut, pObj->Id );
    return pManCut;
}

/**Function*************************************************************

  Synopsis    [Starts the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_ManScl_t * Abc_ManSclStart( int nLutSize, int nCutSizeMax, int nNodesMax )
{
    Abc_ManScl_t * p;
    int i, k;
    assert( sizeof(unsigned) == 4 );
    p = ABC_ALLOC( Abc_ManScl_t, 1 );
    memset( p, 0, sizeof(Abc_ManScl_t) );
    p->nLutSize    = nLutSize;
    p->nCutSizeMax = nCutSizeMax;
    p->nNodesMax   = nNodesMax;
    p->nWords      = Extra_TruthWordNum(nCutSizeMax);
    // allocate simulation info
    p->uVars = (unsigned **)Extra_ArrayAlloc( nCutSizeMax, p->nWords, 4 );
    p->uSims = (unsigned **)Extra_ArrayAlloc( nNodesMax, p->nWords, 4 );
    p->uCofs = (unsigned **)Extra_ArrayAlloc( 2 << nLutSize, p->nWords, 4 );
    memset( p->uVars[0], 0, nCutSizeMax * p->nWords * 4 );
    // assign elementary truth tables
    for ( k = 0; k < p->nCutSizeMax; k++ )
        for ( i = 0; i < p->nWords * 32; i++ )
            if ( i & (1 << k) )
                p->uVars[k][i>>5] |= (1 << (i&31));
    // other data structures
//    p->vBound = Vec_IntAlloc( nCutSizeMax );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ManSclStop( Abc_ManScl_t * p )
{
//    Vec_IntFree( p->vBound );
    ABC_FREE( p->uVars );
    ABC_FREE( p->uSims );
    ABC_FREE( p->uCofs );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Performs superchoicing for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Abc_NodeSuperChoiceTruth( Abc_ManScl_t * pManScl )
{
    Abc_Obj_t * pObj;
    unsigned * puData0, * puData1, * puData = NULL;
    char * pSop;
    int i, k;
    // set elementary truth tables
    Vec_PtrForEachEntry( Abc_Obj_t *, pManScl->vLeaves, pObj, i )
        pObj->pNext = (Abc_Obj_t *)pManScl->uVars[i];
    // compute truth tables for internal nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, pManScl->vVolume, pObj, i )
    {
        // set storage for the node's simulation info
        pObj->pNext = (Abc_Obj_t *)pManScl->uSims[i];
        // get pointer to the simulation info
        puData  = (unsigned *)pObj->pNext;
        puData0 = (unsigned *)Abc_ObjFanin0(pObj)->pNext;
        puData1 = (unsigned *)Abc_ObjFanin1(pObj)->pNext;
        // simulate
        pSop = (char *)pObj->pData;
        if ( pSop[0] == '0' && pSop[1] == '0' )
            for ( k = 0; k < pManScl->nWords; k++ )
                puData[k] = ~puData0[k] & ~puData1[k];
        else if ( pSop[0] == '0' )
            for ( k = 0; k < pManScl->nWords; k++ )
                puData[k] = ~puData0[k] & puData1[k];
        else if ( pSop[1] == '0' )
            for ( k = 0; k < pManScl->nWords; k++ )
                puData[k] = puData0[k] & ~puData1[k];
        else 
            for ( k = 0; k < pManScl->nWords; k++ )
                puData[k] = puData0[k] & puData1[k];
    }
    return puData;
}

/**Function*************************************************************

  Synopsis    [Performs superchoicing for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeSuperChoiceCollect2_rec( Abc_Obj_t * pObj, Vec_Ptr_t * vVolume )
{
    if ( pObj->fMarkC )
        return;
    pObj->fMarkC = 1;
    assert( Abc_ObjFaninNum(pObj) == 2 );
    Abc_NodeSuperChoiceCollect2_rec( Abc_ObjFanin0(pObj), vVolume );
    Abc_NodeSuperChoiceCollect2_rec( Abc_ObjFanin1(pObj), vVolume );
    Vec_PtrPush( vVolume, pObj );
}

/**Function*************************************************************

  Synopsis    [Performs superchoicing for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeSuperChoiceCollect2( Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vVolume )
{
    Abc_Obj_t * pObj;
    int i;
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pObj, i )
        pObj->fMarkC = 1;
    Vec_PtrClear( vVolume );
    Abc_NodeSuperChoiceCollect2_rec( pRoot, vVolume );
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pObj, i )
        pObj->fMarkC = 0;
    Vec_PtrForEachEntry( Abc_Obj_t *, vVolume, pObj, i )
        pObj->fMarkC = 0;
}

/**Function*************************************************************

  Synopsis    [Performs superchoicing for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeSuperChoiceCollect_rec( Abc_Obj_t * pObj, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vVolume )
{
    if ( pObj->fMarkB )
    {
        Vec_PtrPush( vLeaves, pObj );
        pObj->fMarkB = 0;
    }
    if ( pObj->fMarkC )
        return;
    pObj->fMarkC = 1;
    assert( Abc_ObjFaninNum(pObj) == 2 );
    Abc_NodeSuperChoiceCollect_rec( Abc_ObjFanin0(pObj), vLeaves, vVolume );
    Abc_NodeSuperChoiceCollect_rec( Abc_ObjFanin1(pObj), vLeaves, vVolume );
    Vec_PtrPush( vVolume, pObj );
}

/**Function*************************************************************

  Synopsis    [Performs superchoicing for one node.]

  Description [Orders the leaves topologically.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeSuperChoiceCollect( Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vVolume )
{
    Abc_Obj_t * pObj;
    int i, nLeaves;
    nLeaves = Vec_PtrSize(vLeaves);
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pObj, i )
        pObj->fMarkB = pObj->fMarkC = 1;
    Vec_PtrClear( vVolume );
    Vec_PtrClear( vLeaves );
    Abc_NodeSuperChoiceCollect_rec( pRoot, vLeaves, vVolume );
    assert( Vec_PtrSize(vLeaves) == nLeaves );
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pObj, i )
        pObj->fMarkC = 0;
    Vec_PtrForEachEntry( Abc_Obj_t *, vVolume, pObj, i )
        pObj->fMarkC = 0;
}

/**Function*************************************************************

  Synopsis    [Performs superchoicing for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeLeavesRemove( Vec_Ptr_t * vLeaves, unsigned uPhase, int nVars )
{
    int i;
    for ( i = nVars - 1; i >= 0; i-- )
        if ( uPhase & (1 << i) )
            Vec_PtrRemove( vLeaves, Vec_PtrEntry(vLeaves, i) );
}

/**Function*************************************************************

  Synopsis    [Performs superchoicing for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeGetLevel( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanin;
    int i, Level;
    Level = 0;
    Abc_ObjForEachFanin( pObj, pFanin, i )
        Level = Abc_MaxInt( Level, (int)pFanin->Level );
    return Level + 1;
}

/**Function*************************************************************

  Synopsis    [Performs superchoicing for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeSuperChoiceLut( Abc_ManScl_t * p, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanin, * pObjNew;
    int i, nVars, uSupport, nSuppVars;
    // collect the cone using DFS (excluding leaves)
    Abc_NodeSuperChoiceCollect2( pObj, p->vLeaves, p->vVolume );
    assert( Vec_PtrEntryLast(p->vVolume) == pObj );  
    // compute the truth table
    p->uTruth = Abc_NodeSuperChoiceTruth( p );
    // get the support of this truth table
    nVars = Vec_PtrSize(p->vLeaves);
    uSupport = Extra_TruthSupport(p->uTruth, nVars);
    nSuppVars = Extra_WordCountOnes(uSupport);
    assert( nSuppVars <= nVars );
    if ( nSuppVars == 0 )
    {
        pObj->Level = 0;
        return NULL;
    }
    if ( nSuppVars == 1 )
    {
        // find the variable
        for ( i = 0; i < nVars; i++ )
            if ( uSupport & (1 << i) )
                break;
        assert( i < nVars );
        pFanin = (Abc_Obj_t *)Vec_PtrEntry( p->vLeaves, i );
        pObj->Level = pFanin->Level;
        return NULL;
    }
    // support-minimize the truth table
    if ( nSuppVars != nVars )
    {
        Extra_TruthShrink( p->uCofs[0], p->uTruth, nSuppVars, nVars, uSupport );
        Extra_TruthCopy( p->uTruth, p->uCofs[0], nVars );
        Abc_NodeLeavesRemove( p->vLeaves, ((1 << nVars) - 1) & ~uSupport, nVars );
    }
//    return NULL;
    // decompose the truth table recursively
    while ( Vec_PtrSize(p->vLeaves) > p->nLutSize )
        if ( !Abc_NodeDecomposeStep( p ) )
        {
            Vec_PtrForEachEntry( Abc_Obj_t *, p->vLeaves, pFanin, i )
                if ( Abc_ObjIsNode(pFanin) && Abc_ObjFanoutNum(pFanin) == 0 )
                    Abc_NtkDeleteObj_rec( pFanin, 1 );
            return NULL;
        }
    // create the topmost node
    pObjNew = Abc_NtkCreateNode( pObj->pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vLeaves, pFanin, i )
        Abc_ObjAddFanin( pObjNew, pFanin );
    // create the function
    pObjNew->pData = Abc_SopCreateFromTruth( (Mem_Flex_t *)pObj->pNtk->pManFunc, Vec_PtrSize(p->vLeaves), p->uTruth ); // need ISOP
    pObjNew->Level = Abc_NodeGetLevel( pObjNew );
    return pObjNew;
}

/**Function*************************************************************

  Synopsis    [Procedure used for sorting the nodes in increasing order of levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeCompareLevelsInc( int * pp1, int * pp2 )
{
    Abc_Obj_t * pNode1, * pNode2;
    pNode1 = (Abc_Obj_t *)Vec_PtrEntry(s_pLeaves, *pp1);
    pNode2 = (Abc_Obj_t *)Vec_PtrEntry(s_pLeaves, *pp2);
    if ( pNode1->Level < pNode2->Level )
        return -1;
    if ( pNode1->Level > pNode2->Level ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Selects the earliest arriving nodes from the array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeDecomposeSort( Abc_Obj_t ** pLeaves, int nVars, int * pBSet, int nLutSize )
{
    Abc_Obj_t * pTemp[SCL_VARS_MAX];
    int i, k, kBest, LevelMin;
    assert( nLutSize < nVars );
    assert( nVars <= SCL_VARS_MAX );
    // copy nodes into the internal storage
//    printf( "(" );
    for ( i = 0; i < nVars; i++ )
    {
        pTemp[i] = pLeaves[i];
//        printf( " %d", pLeaves[i]->Level );
    }
//    printf( " )\n" );
    // choose one node at a time
    for ( i = 0; i < nLutSize; i++ )
    {
        kBest = -1;
        LevelMin = LARGE_LEVEL;
        for ( k = 0; k < nVars; k++ )
            if ( pTemp[k] && LevelMin > (int)pTemp[k]->Level )
            {
                LevelMin = pTemp[k]->Level;
                kBest = k;
            }
        pBSet[i] = kBest;
        pTemp[kBest] = NULL;
    }
}

/**Function*************************************************************

  Synopsis    [Performs superchoicing for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeDecomposeStep( Abc_ManScl_t * p )
{
    static char pCofClasses[1<<SCL_LUT_MAX][1<<SCL_LUT_MAX];
    static char nCofClasses[1<<SCL_LUT_MAX];
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pObjNew, * pFanin, * pNodesNew[SCL_LUT_MAX];
    unsigned * pTruthCof, * pTruthClass, * pTruth, uPhase;
    int i, k, c, v, w, nVars, nVarsNew, nClasses, nCofs;
    // set the network
    pNtk = ((Abc_Obj_t *)Vec_PtrEntry(p->vLeaves, 0))->pNtk;
    // find the earliest nodes
    nVars = Vec_PtrSize(p->vLeaves);
    assert( nVars > p->nLutSize );
/*
    for ( v = 0; v < nVars; v++ )
        p->pBSet[v] = v;
    qsort( (void *)p->pBSet, (size_t)nVars, sizeof(int), 
            (int (*)(const void *, const void *)) Abc_NodeCompareLevelsInc );
*/
    Abc_NodeDecomposeSort( (Abc_Obj_t **)Vec_PtrArray(p->vLeaves), Vec_PtrSize(p->vLeaves), p->pBSet, p->nLutSize );
    assert( ((Abc_Obj_t *)Vec_PtrEntry(p->vLeaves, p->pBSet[0]))->Level <=
        ((Abc_Obj_t *)Vec_PtrEntry(p->vLeaves, p->pBSet[1]))->Level );
    // cofactor w.r.t. the selected variables
    Extra_TruthCopy( p->uCofs[1], p->uTruth, nVars );
    c = 2;
    for ( v = 0; v < p->nLutSize; v++ )
        for ( k = 0; k < (1<<v); k++ )
        {
            Extra_TruthCopy( p->uCofs[c], p->uCofs[c/2], nVars );
            Extra_TruthCopy( p->uCofs[c+1], p->uCofs[c/2], nVars );
            Extra_TruthCofactor0( p->uCofs[c], nVars, p->pBSet[v] );
            Extra_TruthCofactor1( p->uCofs[c+1], nVars, p->pBSet[v] );
            c += 2;
        }
    assert( c == (2 << p->nLutSize) );
    // count unique cofactors
    nClasses = 0;
    nCofs = (1 << p->nLutSize);
    for ( i = 0; i < nCofs; i++ )
    {
        pTruthCof = p->uCofs[ nCofs + i ];
        for ( k = 0; k < nClasses; k++ )
        {
            pTruthClass = p->uCofs[ nCofs + pCofClasses[k][0] ];
            if ( Extra_TruthIsEqual( pTruthCof, pTruthClass, nVars ) )
            {
                pCofClasses[k][(int)nCofClasses[k]++ ] = i;
                break;
            }
        }
        if ( k != nClasses )
            continue;
        // not found
        pCofClasses[nClasses][0] = i;
        nCofClasses[nClasses] = 1;
        nClasses++;
        if ( nClasses > nCofs/2 )
            return 0;
    }
    // the number of cofactors is acceptable
    nVarsNew = Abc_Base2Log( nClasses );
    assert( nVarsNew < p->nLutSize );
    // create the remainder truth table
    // for each class of cofactors, multiply cofactor truth table by its code
    Extra_TruthClear( p->uTruth, nVars );
    for ( k = 0; k < nClasses; k++ )
    {
        pTruthClass = p->uCofs[ nCofs + pCofClasses[k][0] ];
        for ( v = 0; v < nVarsNew; v++ )
            if ( k & (1 << v) )
                Extra_TruthAnd( pTruthClass, pTruthClass, p->uVars[p->pBSet[v]], nVars );
            else
                Extra_TruthSharp( pTruthClass, pTruthClass, p->uVars[p->pBSet[v]], nVars );
        Extra_TruthOr( p->uTruth, p->uTruth, pTruthClass, nVars );
    }
    // create nodes
    pTruth = p->uCofs[0];
    for ( v = 0; v < nVarsNew; v++ )
    {
        Extra_TruthClear( pTruth, p->nLutSize );
        for ( k = 0; k < nClasses; k++ )
            if ( k & (1 << v) )
                for ( i = 0; i < nCofClasses[k]; i++ )
                {
                    pTruthCof = p->uCofs[1];
                    Extra_TruthFill( pTruthCof, p->nLutSize );
                    for ( w = 0; w < p->nLutSize; w++ )
                        if ( pCofClasses[k][i] & (1 << (p->nLutSize-1-w)) )
                            Extra_TruthAnd( pTruthCof, pTruthCof, p->uVars[w], p->nLutSize );
                        else
                            Extra_TruthSharp( pTruthCof, pTruthCof, p->uVars[w], p->nLutSize );
                    Extra_TruthOr( pTruth, pTruth, pTruthCof, p->nLutSize );
                }
        // implement the node
        pObjNew = Abc_NtkCreateNode( pNtk );
        for ( i = 0; i < p->nLutSize; i++ )
        {
            pFanin = (Abc_Obj_t *)Vec_PtrEntry( p->vLeaves, p->pBSet[i] );
            Abc_ObjAddFanin( pObjNew, pFanin );
        }
        // create the function
        pObjNew->pData = Abc_SopCreateFromTruth( (Mem_Flex_t *)pNtk->pManFunc, p->nLutSize, pTruth ); // need ISOP
        pObjNew->Level = Abc_NodeGetLevel( pObjNew );
        pNodesNew[v] = pObjNew;
    }
    // put the new nodes back into the list
    for ( v = 0; v < nVarsNew; v++ )
        Vec_PtrWriteEntry( p->vLeaves, p->pBSet[v], pNodesNew[v] );
    // compute the variables that should be removed
    uPhase = 0;
    for ( v = nVarsNew; v < p->nLutSize; v++ )
        uPhase |= (1 << p->pBSet[v]);
    // remove entries from the array
    Abc_NodeLeavesRemove( p->vLeaves, uPhase, nVars );
    // update truth table
    Extra_TruthShrink( p->uCofs[0], p->uTruth, nVars - p->nLutSize + nVarsNew, nVars, ((1 << nVars) - 1) & ~uPhase );
    Extra_TruthCopy( p->uTruth, p->uCofs[0], nVars );
    assert( !Extra_TruthVarInSupport( p->uTruth, nVars, nVars - p->nLutSize + nVarsNew ) );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Performs specialized mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static word s__Truths6[6] = {
    ABC_CONST(0xAAAAAAAAAAAAAAAA),
    ABC_CONST(0xCCCCCCCCCCCCCCCC),
    ABC_CONST(0xF0F0F0F0F0F0F0F0),
    ABC_CONST(0xFF00FF00FF00FF00),
    ABC_CONST(0xFFFF0000FFFF0000),
    ABC_CONST(0xFFFFFFFF00000000)
};
word Abc_ObjComputeTruth( Abc_Obj_t * pObj, Vec_Int_t * vSupp )
{
    int Index; word t0, t1, tc;
    assert( Vec_IntSize(vSupp) <= 6 );
    if ( (Index = Vec_IntFind(vSupp, Abc_ObjId(pObj))) >= 0 )
        return s__Truths6[Index];
    assert( Abc_ObjIsNode(pObj) );
    if ( Abc_ObjFaninNum(pObj) == 0 )
        return Abc_NodeIsConst0(pObj) ? (word)0 : ~(word)0;
    assert( Abc_ObjFaninNum(pObj) == 3 );
    t0 = Abc_ObjComputeTruth( Abc_ObjFanin(pObj, 2), vSupp );
    t1 = Abc_ObjComputeTruth( Abc_ObjFanin(pObj, 1), vSupp );
    tc = Abc_ObjComputeTruth( Abc_ObjFanin(pObj, 0), vSupp );
    return (tc & t1) | (~tc & t0);
}
Abc_Obj_t * Abc_NtkSpecialMap_rec( Abc_Ntk_t * pNew, Abc_Obj_t * pObj, Vec_Wec_t * vSupps, Vec_Int_t * vCover )
{
    if ( pObj->pCopy )
        return pObj->pCopy;
    if ( Abc_ObjFaninNum(pObj) == 0 )
        return NULL;
    assert( Abc_ObjFaninNum(pObj) == 3 );
    if ( pObj->fMarkA || pObj->fMarkB )
    {
        Abc_Obj_t * pFan0 = Abc_NtkSpecialMap_rec( pNew, Abc_ObjFanin(pObj, 2), vSupps, vCover );
        Abc_Obj_t * pFan1 = Abc_NtkSpecialMap_rec( pNew, Abc_ObjFanin(pObj, 1), vSupps, vCover );
        Abc_Obj_t * pFanC = Abc_NtkSpecialMap_rec( pNew, Abc_ObjFanin(pObj, 0), vSupps, vCover );
        if ( pFan0 == NULL )
            pFan0 = Abc_NodeIsConst0(Abc_ObjFanin(pObj, 2)) ? Abc_NtkCreateNodeConst0(pNew) : Abc_NtkCreateNodeConst1(pNew);
        if ( pFan1 == NULL )
            pFan1 = Abc_NodeIsConst0(Abc_ObjFanin(pObj, 1)) ? Abc_NtkCreateNodeConst0(pNew) : Abc_NtkCreateNodeConst1(pNew);
        pObj->pCopy = Abc_NtkCreateNodeMux( pNew, pFanC, pFan1, pFan0 );
        pObj->pCopy->fMarkA = pObj->fMarkA;
        pObj->pCopy->fMarkB = pObj->fMarkB;
    }
    else
    {
        Abc_Obj_t * pTemp; int i; word Truth;
        Vec_Int_t * vSupp = Vec_WecEntry( vSupps, Abc_ObjId(pObj) );
        Abc_NtkForEachObjVec( vSupp, pObj->pNtk, pTemp, i )
            Abc_NtkSpecialMap_rec( pNew, pTemp, vSupps, vCover );
        pObj->pCopy = Abc_NtkCreateNode( pNew );   
        Abc_NtkForEachObjVec( vSupp, pObj->pNtk, pTemp, i )
            Abc_ObjAddFanin( pObj->pCopy, pTemp->pCopy );
        Truth = Abc_ObjComputeTruth( pObj, vSupp );
        pObj->pCopy->pData = Abc_SopCreateFromTruthIsop( (Mem_Flex_t *)pNew->pManFunc, Vec_IntSize(vSupp), &Truth, vCover );
        assert( Abc_SopGetVarNum((char *)pObj->pCopy->pData) == Vec_IntSize(vSupp) );
    }
    return pObj->pCopy;
}
Abc_Ntk_t * Abc_NtkSpecialMapping( Abc_Ntk_t * pNtk, int fVerbose )
{
    Abc_Ntk_t * pNtkNew;
    Vec_Int_t * vCover = Vec_IntAlloc( 1 << 16 );
    Vec_Wec_t * vSupps = Vec_WecStart( Abc_NtkObjNumMax(pNtk) );
    Abc_Obj_t * pObj, * pFan0, * pFan1, * pFanC; int i, Count[2] = {0};
    Abc_NtkForEachCi( pNtk, pObj, i )
        Vec_IntPush( Vec_WecEntry(vSupps, i), i );
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        Vec_Int_t * vSupp = Vec_WecEntry(vSupps, i);
        if ( Abc_ObjFaninNum(pObj) == 0 )
            continue;
        assert( Abc_ObjFaninNum(pObj) == 3 );
        pFan0 = Abc_ObjFanin( pObj, 2 );
        pFan1 = Abc_ObjFanin( pObj, 1 );
        pFanC = Abc_ObjFanin0( pObj );
        assert( Abc_ObjIsCi(pFanC) );
        if ( pFan0->fMarkA && pFan1->fMarkA )
        {
            pObj->fMarkB = 1;
            Vec_IntPush( vSupp, Abc_ObjId(pObj) );
            continue;
        }
        Vec_IntTwoMerge2( Vec_WecEntry(vSupps, Abc_ObjId(pFan0)), Vec_WecEntry(vSupps, Abc_ObjId(pFan1)), vSupp );
        assert( Vec_IntFind(vSupp, Abc_ObjId(pFanC)) == -1 );
        Vec_IntPushOrder( vSupp, Abc_ObjId(pFanC) );
        if ( Vec_IntSize(vSupp) <= 6 )
            continue;
        Vec_IntClear( vSupp );
        if ( !pFan0->fMarkA && !pFan1->fMarkA )
        {
            pObj->fMarkA = 1;
            Vec_IntPush( vSupp, Abc_ObjId(pObj) );
        }
        else
        {
            Vec_IntPushOrder( vSupp, Abc_ObjId(pFan0) );
            Vec_IntPushOrder( vSupp, Abc_ObjId(pFan1) );
            Vec_IntPushOrder( vSupp, Abc_ObjId(pFanC) );
        }
    }

    if ( fVerbose )
    {
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        printf( "Node %4d : ", i );
        if ( pObj->fMarkA )
            printf( " MarkA  " );
        else
            printf( "        " );
        if ( pObj->fMarkB )
            printf( " MarkB  " );
        else
            printf( "        " );
        Vec_IntPrint( Vec_WecEntry(vSupps, i) );
    }
    }

    Abc_NtkCleanCopy( pNtk );
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_SOP );
    Abc_NtkForEachCo( pNtk, pObj, i )
        if ( Abc_ObjFaninNum(Abc_ObjFanin0(pObj)) == 0 )
            Abc_ObjFanin0(pObj)->pCopy = Abc_NodeIsConst0(Abc_ObjFanin0(pObj)) ? Abc_NtkCreateNodeConst0(pNtkNew) : Abc_NtkCreateNodeConst1(pNtkNew);
        else
            Abc_NtkSpecialMap_rec( pNtkNew, Abc_ObjFanin0(pObj), vSupps, vCover );
    Abc_NtkFinalize( pNtk, pNtkNew );
    Abc_NtkCleanMarkAB( pNtk );
    Vec_WecFree( vSupps );
    Vec_IntFree( vCover );

    Abc_NtkForEachNode( pNtkNew, pObj, i )
    {
        Count[0] += pObj->fMarkA,
        Count[1] += pObj->fMarkB;
        pObj->fPersist = pObj->fMarkA | pObj->fMarkB;
        pObj->fMarkA = pObj->fMarkB = 0;
    }
    //printf( "Total = %3d.  Nodes = %3d. MarkA = %3d. MarkB = %3d.\n", Abc_NtkNodeNum(pNtkNew), 
    //    Abc_NtkNodeNum(pNtkNew) - Count[0] - Count[1], Count[0], Count[1] );

    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkSpecialMapping: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END



