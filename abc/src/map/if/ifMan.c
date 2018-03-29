/**CFile****************************************************************

  FileName    [ifMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Mapping manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifMan.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static If_Obj_t * If_ManSetupObj( If_Man_t * p );

static void       If_ManCutSetRecycle( If_Man_t * p, If_Set_t * pSet ) { pSet->pNext = p->pFreeList; p->pFreeList = pSet;                            }
static If_Set_t * If_ManCutSetFetch( If_Man_t * p )                    { If_Set_t * pTemp = p->pFreeList; p->pFreeList = p->pFreeList->pNext; return pTemp; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Man_t * If_ManStart( If_Par_t * pPars )
{
    If_Man_t * p; int v;
    assert( !pPars->fUseDsd || !pPars->fUseTtPerm );
    // start the manager
    p = ABC_ALLOC( If_Man_t, 1 );
    memset( p, 0, sizeof(If_Man_t) );
    p->pPars    = pPars;
    p->fEpsilon = pPars->Epsilon;
    // allocate arrays for nodes
    p->vCis     = Vec_PtrAlloc( 100 );
    p->vCos     = Vec_PtrAlloc( 100 );
    p->vObjs    = Vec_PtrAlloc( 100 );
    p->vTemp    = Vec_PtrAlloc( 100 );
    p->vVisited = Vec_PtrAlloc( 100 );
    // prepare the memory manager
    if ( p->pPars->fTruth )
    {
        for ( v = 0; v <= p->pPars->nLutSize; v++ )
            p->nTruth6Words[v] = Abc_Truth6WordNum( v );
        for ( v = 6; v <= Abc_MaxInt(6,p->pPars->nLutSize); v++ )
            p->vTtMem[v] = Vec_MemAllocForTT( v, pPars->fUseTtPerm );
        for ( v = 0; v < 6; v++ )
            p->vTtMem[v] = p->vTtMem[6];
        if ( p->pPars->fDelayOpt || pPars->nGateSize > 0 )
        {
            for ( v = 6; v <= Abc_MaxInt(6,p->pPars->nLutSize); v++ )
                p->vTtIsops[v] = Vec_WecAlloc( 1000 );
            for ( v = 6; v <= Abc_MaxInt(6,p->pPars->nLutSize); v++ )
                Vec_WecInit( p->vTtIsops[v], 2 );
            for ( v = 0; v < 6; v++ )
                p->vTtIsops[v] = p->vTtIsops[6];
        }
        if ( pPars->fDelayOpt || pPars->nGateSize > 0 || pPars->fDsdBalance )
        {
            p->vCover = Vec_IntAlloc( 0 );
            p->vArray = Vec_IntAlloc( 1000 );
        }
    }
    p->nPermWords  = p->pPars->fUsePerm? If_CutPermWords( p->pPars->nLutSize ) : 0;
    p->nObjBytes   = sizeof(If_Obj_t) + sizeof(int) * (p->pPars->nLutSize + p->nPermWords);
    p->nCutBytes   = sizeof(If_Cut_t) + sizeof(int) * (p->pPars->nLutSize + p->nPermWords);
    p->nSetBytes   = sizeof(If_Set_t) + (sizeof(If_Cut_t *) + p->nCutBytes) * (p->pPars->nCutsMax + 1);
    p->pMemObj     = Mem_FixedStart( p->nObjBytes );
    // report expected memory usage
    if ( p->pPars->fVerbose )
        Abc_Print( 1, "K = %d. Memory (bytes): Truth = %4d. Cut = %4d. Obj = %4d. Set = %4d. CutMin = %s\n", 
            p->pPars->nLutSize, 8 * p->nTruth6Words[p->pPars->nLutSize], p->nCutBytes, p->nObjBytes, p->nSetBytes, p->pPars->fCutMin? "yes":"no" );
    // room for temporary truth tables
    p->puTemp[0] = p->pPars->fTruth? ABC_ALLOC( unsigned, 8 * p->nTruth6Words[p->pPars->nLutSize] ) : NULL;
    p->puTemp[1] = p->puTemp[0] + p->nTruth6Words[p->pPars->nLutSize]*2;
    p->puTemp[2] = p->puTemp[1] + p->nTruth6Words[p->pPars->nLutSize]*2;
    p->puTemp[3] = p->puTemp[2] + p->nTruth6Words[p->pPars->nLutSize]*2;
    p->puTempW   = p->pPars->fTruth? ABC_ALLOC( word, p->nTruth6Words[p->pPars->nLutSize] ) : NULL;
    if ( pPars->fUseDsd )
    {
        for ( v = 6; v <= Abc_MaxInt(6,p->pPars->nLutSize); v++ )
        {
            p->vTtDsds[v] = Vec_IntAlloc( 1000 );
            Vec_IntPush( p->vTtDsds[v], 0 );
            Vec_IntPush( p->vTtDsds[v], 2 );
            p->vTtPerms[v] = Vec_StrAlloc( 10000 );
            Vec_StrFill( p->vTtPerms[v], 2 * v, IF_BIG_CHAR );
            Vec_StrWriteEntry( p->vTtPerms[v], v, 0 );
        }
        for ( v = 0; v < 6; v++ )
        {
            p->vTtDsds[v]  = p->vTtDsds[6];
            p->vTtPerms[v] = p->vTtPerms[6];
        }
    }
    if ( pPars->fUseTtPerm )
    {
        p->vPairHash = Hash_IntManStart( 10000 );
        p->vPairPerms = Vec_StrAlloc( 10000 );
        Vec_StrFill( p->vPairPerms, p->pPars->nLutSize, 0 );
        p->vPairRes = Vec_IntAlloc( 1000 );
        Vec_IntPush( p->vPairRes, -1 );
        for ( v = 6; v <= Abc_MaxInt(6,p->pPars->nLutSize); v++ )
            p->vTtOccurs[v] = Vec_IntAlloc( 1000 );
        for ( v = 0; v < 6; v++ )
            p->vTtOccurs[v] = p->vTtOccurs[6];
        for ( v = 6; v <= Abc_MaxInt(6,p->pPars->nLutSize); v++ )
            Vec_IntPushTwo( p->vTtOccurs[v], 0, 0 );
    }
    if ( pPars->fUseCofVars )
    {
        for ( v = 6; v <= Abc_MaxInt(6,p->pPars->nLutSize); v++ )
        {
            p->vTtVars[v] = Vec_StrAlloc( 1000 );
            Vec_StrPush( p->vTtVars[v], 0 );
            Vec_StrPush( p->vTtVars[v], 0 );
        }
        for ( v = 0; v < 6; v++ )
            p->vTtVars[v]  = p->vTtVars[6];
    }
    if ( pPars->fUseAndVars )
    {
        for ( v = 6; v <= Abc_MaxInt(6,p->pPars->nLutSize); v++ )
        {
            p->vTtDecs[v] = Vec_IntAlloc( 1000 );
            Vec_IntPush( p->vTtDecs[v], 0 );
            Vec_IntPush( p->vTtDecs[v], 0 );
        }
        for ( v = 0; v < 6; v++ )
            p->vTtDecs[v]  = p->vTtDecs[6];
    }
    if ( pPars->fUseBat )
    {
//        abctime clk = Abc_Clock();
        extern int Bat_ManCellFuncLookup( void * pMan, unsigned * pTruth, int nVars, int nLeaves, char * pStr );
        extern void Bat_ManFuncSetupTable();
        pPars->pFuncCell = (int (*)  (If_Man_t *, unsigned *, int, int, char *))Bat_ManCellFuncLookup;
        Bat_ManFuncSetupTable();
//        Abc_PrintTime( 1, "Setup time", Abc_Clock() - clk );
    }
    // create the constant node
    p->pConst1   = If_ManSetupObj( p );
    p->pConst1->Type   = IF_CONST1;
    p->pConst1->fPhase = 1;
    p->nObjs[IF_CONST1]++;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManRestart( If_Man_t * p )
{
    ABC_FREE( p->pMemCi );
    Vec_PtrClear( p->vCis );
    Vec_PtrClear( p->vCos );
    Vec_PtrClear( p->vObjs );
    Vec_PtrClear( p->vTemp );
    Mem_FixedRestart( p->pMemObj );
    // create the constant node
    p->pConst1 = If_ManSetupObj( p );
    p->pConst1->Type = IF_CONST1;
    p->pConst1->fPhase = 1;
    // reset the counter of other nodes
    p->nObjs[IF_CI] = p->nObjs[IF_CO] = p->nObjs[IF_AND] = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManStop( If_Man_t * p )
{
    extern void If_ManCacheAnalize( If_Man_t * p );
    int i;
    if ( p->pPars->fVerbose && p->vCutData )
        If_ManCacheAnalize( p );
    if ( p->pPars->fVerbose && p->pPars->fTruth )
    {
        int nUnique = 0, nMemTotal = 0;
        for ( i = 6; i <= Abc_MaxInt(6,p->pPars->nLutSize); i++ )
            nUnique += Vec_MemEntryNum(p->vTtMem[i]);
        for ( i = 6; i <= Abc_MaxInt(6,p->pPars->nLutSize); i++ )
            nMemTotal += (int)Vec_MemMemory(p->vTtMem[i]);
        printf( "Unique truth tables = %d   Memory = %.2f MB   ", nUnique, 1.0 * nMemTotal / (1<<20) ); 
        Abc_PrintTime( 1, "Time", p->timeCache[4] );
        if ( p->nCacheMisses )
        {
            printf( "Cache hits = %d. Cache misses = %d  (%.2f %%)\n", p->nCacheHits, p->nCacheMisses, 100.0 * p->nCacheMisses / (p->nCacheHits + p->nCacheMisses) ); 
            Abc_PrintTime( 1, "Non-DSD   ", p->timeCache[0] );
            Abc_PrintTime( 1, "DSD hits  ", p->timeCache[1] );
            Abc_PrintTime( 1, "DSD misses", p->timeCache[2] );
            Abc_PrintTime( 1, "TOTAL     ", p->timeCache[0] + p->timeCache[1] + p->timeCache[2] );
            Abc_PrintTime( 1, "Canon     ", p->timeCache[3] );
        }
    }
    if ( p->pPars->fVerbose && p->nCutsUselessAll )
    {
        for ( i = 0; i <= 16; i++ )
            if ( p->nCutsUseless[i] )
                Abc_Print( 1, "Useless cuts %2d  = %9d  (out of %9d)  (%6.2f %%)\n", i, p->nCutsUseless[i], p->nCutsCount[i], 100.0*p->nCutsUseless[i]/Abc_MaxInt(p->nCutsCount[i],1) );
        Abc_Print( 1, "Useless cuts all = %9d  (out of %9d)  (%6.2f %%)\n", p->nCutsUselessAll, p->nCutsCountAll, 100.0*p->nCutsUselessAll/Abc_MaxInt(p->nCutsCountAll,1) );
    }
//    if ( p->pPars->fVerbose && p->nCuts5 )
//        Abc_Print( 1, "Statistics about 5-cuts: Total = %d  Non-decomposable = %d (%.2f %%)\n", p->nCuts5, p->nCuts5-p->nCuts5a, 100.0*(p->nCuts5-p->nCuts5a)/p->nCuts5 );
    if ( p->pIfDsdMan )
        p->pIfDsdMan = NULL;
    if ( p->pPars->fUseDsd && (p->nCountNonDec[0] || p->nCountNonDec[1]) )
        printf( "NonDec0 = %d.  NonDec1 = %d.\n", p->nCountNonDec[0], p->nCountNonDec[1] );
    Vec_IntFreeP( &p->vCoAttrs );
    Vec_PtrFree( p->vCis );
    Vec_PtrFree( p->vCos );
    Vec_PtrFree( p->vObjs );
    Vec_PtrFree( p->vTemp );
    Vec_IntFreeP( &p->vCover );
    Vec_IntFreeP( &p->vArray );
    Vec_WrdFreeP( &p->vAnds );
    Vec_WrdFreeP( &p->vAndGate );
    Vec_WrdFreeP( &p->vOrGate );
    Vec_PtrFreeP( &p->vObjsRev );
    Vec_PtrFreeP( &p->vLatchOrder );
    Vec_IntFreeP( &p->vLags );
    Vec_IntFreeP( &p->vDump );
    for ( i = 6; i <= Abc_MaxInt(6,p->pPars->nLutSize); i++ )
        Vec_IntFreeP( &p->vTtDsds[i] );
    for ( i = 6; i <= Abc_MaxInt(6,p->pPars->nLutSize); i++ )
        Vec_StrFreeP( &p->vTtPerms[i] );
    for ( i = 6; i <= Abc_MaxInt(6,p->pPars->nLutSize); i++ )
        Vec_StrFreeP( &p->vTtVars[i] );
    for ( i = 6; i <= Abc_MaxInt(6,p->pPars->nLutSize); i++ )
        Vec_IntFreeP( &p->vTtDecs[i] );
    Vec_IntFreeP( &p->vCutData );
    Vec_IntFreeP( &p->vPairRes );
    Vec_StrFreeP( &p->vPairPerms );
    Vec_PtrFreeP( &p->vVisited );
    if ( p->vPairHash )
        Hash_IntManStop( p->vPairHash );
    for ( i = 6; i <= Abc_MaxInt(6,p->pPars->nLutSize); i++ )
        Vec_MemHashFree( p->vTtMem[i] );
    for ( i = 6; i <= Abc_MaxInt(6,p->pPars->nLutSize); i++ )
        Vec_MemFreeP( &p->vTtMem[i] );
    for ( i = 6; i <= Abc_MaxInt(6,p->pPars->nLutSize); i++ )
        Vec_WecFreeP( &p->vTtIsops[i] );
    for ( i = 6; i <= Abc_MaxInt(6,p->pPars->nLutSize); i++ )
        Vec_IntFreeP( &p->vTtOccurs[i] );
    Mem_FixedStop( p->pMemObj, 0 );
    ABC_FREE( p->pMemCi );
    ABC_FREE( p->pMemAnd );
    ABC_FREE( p->puTemp[0] );
    ABC_FREE( p->puTempW );
    // free pars memory
    ABC_FREE( p->pPars->pTimesArr );
    ABC_FREE( p->pPars->pTimesReq );
    if ( p->pManTim )
        Tim_ManStop( p->pManTim );
    if ( p->vSwitching )
        Vec_IntFree( p->vSwitching );
    if ( p->pPars->fUseBat )
    {
        extern void Bat_ManFuncSetdownTable();
        Bat_ManFuncSetdownTable();
    }
    // hash table
//    if ( p->pPars->fVerbose && p->nTableEntries[0] )
//        printf( "Hash table 2:  Entries = %7d.  Size = %7d.\n", p->nTableEntries[0], p->nTableSize[0] );
//    if ( p->pPars->fVerbose && p->nTableEntries[1] )
//        printf( "Hash table 3:  Entries = %7d.  Size = %7d.\n", p->nTableEntries[1], p->nTableSize[1] );
    ABC_FREE( p->pHashTable[0] );
    ABC_FREE( p->pHashTable[1] );
    if ( p->pMemEntries )
        Mem_FixedStop( p->pMemEntries, 0 );
    ABC_FREE( p->pName );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Creates primary input.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Obj_t * If_ManCreateCi( If_Man_t * p )
{
    If_Obj_t * pObj;
    pObj = If_ManSetupObj( p );
    pObj->Type = IF_CI;
    pObj->IdPio = Vec_PtrSize( p->vCis );
    Vec_PtrPush( p->vCis, pObj );
    p->nObjs[IF_CI]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Creates primary output with the given driver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Obj_t * If_ManCreateCo( If_Man_t * p, If_Obj_t * pDriver )
{
    If_Obj_t * pObj;
    pObj = If_ManSetupObj( p );
    pObj->IdPio = Vec_PtrSize( p->vCos );
    Vec_PtrPush( p->vCos, pObj );
    pObj->Type = IF_CO;
    pObj->fCompl0 = If_IsComplement(pDriver); pDriver = If_Regular(pDriver);
    pObj->pFanin0 = pDriver; pDriver->nRefs++; 
    pObj->fPhase  = (pObj->fCompl0 ^ pDriver->fPhase);
    pObj->Level   = pDriver->Level;
    if ( p->nLevelMax < (int)pObj->Level )
        p->nLevelMax = (int)pObj->Level;
    p->nObjs[IF_CO]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Create the new node assuming it does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Obj_t * If_ManCreateAnd( If_Man_t * p, If_Obj_t * pFan0, If_Obj_t * pFan1 )
{
    If_Obj_t * pObj;
    // perform constant propagation
    if ( pFan0 == pFan1 )
        return pFan0;
    if ( pFan0 == If_Not(pFan1) )
        return If_Not(p->pConst1);
    if ( If_Regular(pFan0) == p->pConst1 )
        return pFan0 == p->pConst1 ? pFan1 : If_Not(p->pConst1);
    if ( If_Regular(pFan1) == p->pConst1 )
        return pFan1 == p->pConst1 ? pFan0 : If_Not(p->pConst1);
    // get memory for the new object
    pObj = If_ManSetupObj( p );
    pObj->Type    = IF_AND;
    pObj->fCompl0 = If_IsComplement(pFan0); pFan0 = If_Regular(pFan0);
    pObj->fCompl1 = If_IsComplement(pFan1); pFan1 = If_Regular(pFan1);
    pObj->pFanin0 = pFan0; pFan0->nRefs++; pFan0->nVisits++; pFan0->nVisitsCopy++;
    pObj->pFanin1 = pFan1; pFan1->nRefs++; pFan1->nVisits++; pFan1->nVisitsCopy++;
    pObj->fPhase  = (pObj->fCompl0 ^ pFan0->fPhase) & (pObj->fCompl1 ^ pFan1->fPhase);
    pObj->Level   = 1 + IF_MAX( pFan0->Level, pFan1->Level );
    if ( p->nLevelMax < (int)pObj->Level )
        p->nLevelMax = (int)pObj->Level;
    p->nObjs[IF_AND]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Create the new node assuming it does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Obj_t * If_ManCreateXor( If_Man_t * p, If_Obj_t * pFan0, If_Obj_t * pFan1 )
{
    If_Obj_t * pRes1, * pRes2;
    pRes1 = If_ManCreateAnd( p, If_Not(pFan0), pFan1 );
    pRes2 = If_ManCreateAnd( p, pFan0, If_Not(pFan1) );
    return If_Not( If_ManCreateAnd( p, If_Not(pRes1), If_Not(pRes2) ) );
}

/**Function*************************************************************

  Synopsis    [Create the new node assuming it does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Obj_t * If_ManCreateMux( If_Man_t * p, If_Obj_t * pFan0, If_Obj_t * pFan1, If_Obj_t * pCtrl )
{
    If_Obj_t * pRes1, * pRes2;
    pRes1 = If_ManCreateAnd( p, pFan0, If_Not(pCtrl) );
    pRes2 = If_ManCreateAnd( p, pFan1, pCtrl );
    return If_Not( If_ManCreateAnd( p, If_Not(pRes1), If_Not(pRes2) ) );
}

/**Function*************************************************************

  Synopsis    [Creates the choice node.]

  Description [Should be called after the equivalence class nodes are linked.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManCreateChoice( If_Man_t * p, If_Obj_t * pObj )
{
    If_Obj_t * pTemp;
    // mark the node as a representative if its class
    assert( pObj->fRepr == 0 );
    pObj->fRepr = 1;
    // update the level of this node (needed for correct required time computation)
    for ( pTemp = pObj; pTemp; pTemp = pTemp->pEquiv )
    {
        pObj->Level = IF_MAX( pObj->Level, pTemp->Level );
        pTemp->nVisits++; pTemp->nVisitsCopy++;
    }
    // mark the largest level
    if ( p->nLevelMax < (int)pObj->Level )
        p->nLevelMax = (int)pObj->Level;
    p->nChoices++;
}

/**Function*************************************************************

  Synopsis    [Prepares memory for one cutset.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManSetupSet( If_Man_t * p, If_Set_t * pSet )
{
    char * pArray;
    int i;
    pSet->nCuts = 0;
    pSet->nCutsMax = p->pPars->nCutsMax;
    pSet->ppCuts = (If_Cut_t **)(pSet + 1);
    pArray = (char *)pSet->ppCuts + sizeof(If_Cut_t *) * (pSet->nCutsMax+1);
    for ( i = 0; i <= pSet->nCutsMax; i++ )
    {
        pSet->ppCuts[i] = (If_Cut_t *)(pArray + i * p->nCutBytes); 
        If_CutSetup( p, pSet->ppCuts[i] );
    }
//    pArray += (pSet->nCutsMax + 1) * p->nCutBytes;
//    assert( ((char *)pArray) - ((char *)pSet) == p->nSetBytes );
}

/**Function*************************************************************

  Synopsis    [Prepares memory for one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManSetupCutTriv( If_Man_t * p, If_Cut_t * pCut, int ObjId )
{
    pCut->fCompl     = 0;
    pCut->nLimit     = p->pPars->nLutSize;
    pCut->nLeaves    = 1;
    pCut->pLeaves[0] = p->pPars->fLiftLeaves? (ObjId << 8) : ObjId;
    pCut->uSign      = If_ObjCutSign( pCut->pLeaves[0] );
    pCut->iCutFunc   = p->pPars->fUseTtPerm ? 3 : (p->pPars->fTruth ? 2: -1);
    pCut->uMaskFunc  = 0;
    assert( pCut->pLeaves[0] < p->vObjs->nSize );
}

/**Function*************************************************************

  Synopsis    [Prepares memory for the node with cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Obj_t * If_ManSetupObj( If_Man_t * p )
{
    If_Obj_t * pObj;
    // get memory for the object
    pObj = (If_Obj_t *)Mem_FixedEntryFetch( p->pMemObj );
    memset( pObj, 0, sizeof(If_Obj_t) );
    If_CutSetup( p, &pObj->CutBest );
    // assign ID and save 
    pObj->Id = Vec_PtrSize(p->vObjs);
    Vec_PtrPush( p->vObjs, pObj );
    // set the required times
    pObj->Required = IF_FLOAT_LARGE;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Prepares memory for one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManSetupCiCutSets( If_Man_t * p )
{
    If_Obj_t * pObj;
    int i;
    assert( p->pMemCi == NULL );
    // create elementary cuts for the CIs
    If_ManForEachCi( p, pObj, i )
        If_ManSetupCutTriv( p, &pObj->CutBest, pObj->Id );
    // create elementary cutsets for the CIs
    p->pMemCi = (If_Set_t *)ABC_ALLOC( char, If_ManCiNum(p) * (sizeof(If_Set_t) + sizeof(void *)) );
    If_ManForEachCi( p, pObj, i )
    {
        pObj->pCutSet = (If_Set_t *)((char *)p->pMemCi + i * (sizeof(If_Set_t) + sizeof(void *)));
        pObj->pCutSet->nCuts = 1;
        pObj->pCutSet->nCutsMax = p->pPars->nCutsMax;
        pObj->pCutSet->ppCuts = (If_Cut_t **)(pObj->pCutSet + 1);
        pObj->pCutSet->ppCuts[0] = &pObj->CutBest;
    }
}

/**Function*************************************************************

  Synopsis    [Prepares cutset of the node.]

  Description [Elementary cutset will be added last.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Set_t * If_ManSetupNodeCutSet( If_Man_t * p, If_Obj_t * pObj )
{
    assert( If_ObjIsAnd(pObj) );
    assert( pObj->pCutSet == NULL );
//    pObj->pCutSet = (If_Set_t *)Mem_FixedEntryFetch( p->pMemSet );
//    If_ManSetupSet( p, pObj->pCutSet );
    pObj->pCutSet = If_ManCutSetFetch( p );
    pObj->pCutSet->nCuts = 0;
    pObj->pCutSet->nCutsMax = p->pPars->nCutsMax;
    return pObj->pCutSet;
}

/**Function*************************************************************

  Synopsis    [Dereferences cutset of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManDerefNodeCutSet( If_Man_t * p, If_Obj_t * pObj )
{
    If_Obj_t * pFanin;
    assert( If_ObjIsAnd(pObj) );
    // consider the node
    assert( pObj->nVisits >= 0 );
    if ( pObj->nVisits == 0 )
    {
//        Mem_FixedEntryRecycle( p->pMemSet, (char *)pObj->pCutSet );
        If_ManCutSetRecycle( p, pObj->pCutSet );
        pObj->pCutSet = NULL;
    }
    // consider the first fanin
    pFanin = If_ObjFanin0(pObj);
    assert( pFanin->nVisits > 0 );
    if ( !If_ObjIsCi(pFanin) && --pFanin->nVisits == 0 )
    {
//        Mem_FixedEntryRecycle( p->pMemSet, (char *)pFanin->pCutSet );
        If_ManCutSetRecycle( p, pFanin->pCutSet );
        pFanin->pCutSet = NULL;
    }
    // consider the second fanin
    pFanin = If_ObjFanin1(pObj);
    assert( pFanin->nVisits > 0 );
    if ( !If_ObjIsCi(pFanin) && --pFanin->nVisits == 0 )
    {
//        Mem_FixedEntryRecycle( p->pMemSet, (char *)pFanin->pCutSet );
        If_ManCutSetRecycle( p, pFanin->pCutSet );
        pFanin->pCutSet = NULL;
    }
}

/**Function*************************************************************

  Synopsis    [Dereferences cutset of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManDerefChoiceCutSet( If_Man_t * p, If_Obj_t * pObj )
{
    If_Obj_t * pTemp;
    assert( If_ObjIsAnd(pObj) );
    assert( pObj->fRepr );
    assert( pObj->nVisits > 0 );
    // consider the nodes in the choice class
    for ( pTemp = pObj; pTemp; pTemp = pTemp->pEquiv )
    {
//        assert( pTemp == pObj || pTemp->nVisits == 1 );
        if ( --pTemp->nVisits == 0 )
        {
//            Mem_FixedEntryRecycle( p->pMemSet, (char *)pTemp->pCutSet );
            If_ManCutSetRecycle( p, pTemp->pCutSet );
            pTemp->pCutSet = NULL;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Dereferences cutset of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManSetupSetAll( If_Man_t * p, int nCrossCut )
{
    If_Set_t * pCutSet;
    int i, nCutSets;
    nCutSets = 128 + nCrossCut;
    p->pFreeList = p->pMemAnd = pCutSet = (If_Set_t *)ABC_ALLOC( char, nCutSets * p->nSetBytes );
    for ( i = 0; i < nCutSets; i++ )
    {
        If_ManSetupSet( p, pCutSet );
        if ( i == nCutSets - 1 )
            pCutSet->pNext = NULL;
        else
            pCutSet->pNext = (If_Set_t *)( (char *)pCutSet + p->nSetBytes );
        pCutSet = pCutSet->pNext;
    }
    assert( pCutSet == NULL );

    if ( p->pPars->fVerbose )
    {
        Abc_Print( 1, "Node = %7d.  Ch = %5d.  Total mem = %7.2f MB. Peak cut mem = %7.2f MB.\n", 
            If_ManAndNum(p), p->nChoices,
            1.0 * (p->nObjBytes + 2*sizeof(void *)) * If_ManObjNum(p) / (1<<20), 
            1.0 * p->nSetBytes * nCrossCut / (1<<20) );
    }
//    Abc_Print( 1, "Cross cut = %d.\n", nCrossCut );

}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

