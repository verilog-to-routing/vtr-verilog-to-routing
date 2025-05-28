/**CFile****************************************************************

  FileName    [sclBuffer.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Standard-cell library representation.]

  Synopsis    [Buffering algorithms.]

  Author      [Alan Mishchenko, Niklas Een]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 24, 2012.]

  Revision    [$Id: sclBuffer.c,v 1.0 2012/08/24 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sclSize.h"
#include "map/mio/mio.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define BUF_SCALE 1000

typedef struct Buf_Man_t_ Buf_Man_t;
struct Buf_Man_t_
{
    // parameters
    int            nFanMin;   // the smallest fanout count to consider
    int            nFanMax;   // the largest fanout count allowed off CP
    int            fBufPis;   // enables buffing of the combinational inputs
    // internal deta
    Abc_Ntk_t *    pNtk;      // logic network
    Vec_Int_t *    vOffsets;  // offsets into edge delays
    Vec_Int_t *    vEdges;    // edge delays
    Vec_Int_t *    vArr;      // arrival times
    Vec_Int_t *    vDep;      // departure times
    Vec_Flt_t *    vCounts;   // fanout counts
    Vec_Que_t *    vQue;      // queue by fanout count
    int            nObjStart; // the number of starting objects
    int            nObjAlloc; // the number of allocated objects
    int            DelayMax;  // maximum delay (percentage of inverter delay)
    float          DelayInv;  // inverter delay
    // sorting fanouts
    Vec_Int_t *    vOrder;    // ordering of fanouts
    Vec_Int_t *    vDelays;   // fanout delays
    Vec_Int_t *    vNonCrit;  // non-critical fanouts
    Vec_Int_t *    vTfCone;   // TFI/TFO cone of the node including the node
    Vec_Ptr_t *    vFanouts;  // temp storage for fanouts
    // statistics
    int            nSeparate;
    int            nDuplicate;
    int            nBranch0;
    int            nBranch1;
    int            nBranchCrit;
};

static inline int  Abc_BufNodeArr( Buf_Man_t * p, Abc_Obj_t * pObj )                     { return Vec_IntEntry( p->vArr, Abc_ObjId(pObj) );                                   }
static inline int  Abc_BufNodeDep( Buf_Man_t * p, Abc_Obj_t * pObj )                     { return Vec_IntEntry( p->vDep, Abc_ObjId(pObj) );                                   }
static inline void Abc_BufSetNodeArr( Buf_Man_t * p, Abc_Obj_t * pObj, int f )           { Vec_IntWriteEntry( p->vArr, Abc_ObjId(pObj), f );                                  }
static inline void Abc_BufSetNodeDep( Buf_Man_t * p, Abc_Obj_t * pObj, int f )           { Vec_IntWriteEntry( p->vDep, Abc_ObjId(pObj), f );                                  }
static inline int  Abc_BufEdgeDelay( Buf_Man_t * p, Abc_Obj_t * pObj, int i )            { return Vec_IntEntry( p->vEdges, Vec_IntEntry(p->vOffsets, Abc_ObjId(pObj)) + i );  }
static inline void Abc_BufSetEdgeDelay( Buf_Man_t * p, Abc_Obj_t * pObj, int i, int f )  { Vec_IntWriteEntry( p->vEdges, Vec_IntEntry(p->vOffsets, Abc_ObjId(pObj)) + i, f ); }
static inline int  Abc_BufNodeSlack( Buf_Man_t * p, Abc_Obj_t * pObj )                   { return p->DelayMax - Abc_BufNodeArr(p, pObj) - Abc_BufNodeDep(p, pObj);            }
static inline int  Abc_BufEdgeSlack( Buf_Man_t * p, Abc_Obj_t * pObj, Abc_Obj_t * pFan ) { return p->DelayMax - Abc_BufNodeArr(p, pObj) - Abc_BufNodeDep(p, pFan) - Abc_BufEdgeDelay(p, pFan, Abc_NodeFindFanin(pFan, pObj)); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Make sure fanins of gates are not duplicated.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclReportDupFanins( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pFanin, * pFanin2;
    int i, k, k2;
    Abc_NtkForEachNode( pNtk, pObj, i )
        Abc_ObjForEachFanin( pObj, pFanin, k )
            Abc_ObjForEachFanin( pObj, pFanin2, k2 )
                if ( k != k2 && pFanin == pFanin2 )
                    printf( "Node %d has dup fanin %d.\n", i, Abc_ObjId(pFanin) );    
}

/**Function*************************************************************

  Synopsis    [Removes buffers and inverters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_SclObjIsBufInv( Abc_Obj_t * pObj )
{
    return Abc_ObjIsNode(pObj) && Abc_ObjFaninNum(pObj) == 1;
}
int Abc_SclIsInv( Abc_Obj_t * pObj )
{
    assert( Abc_ObjIsNode(pObj) );
    return Mio_GateReadTruth((Mio_Gate_t *)pObj->pData) == ABC_CONST(0x5555555555555555);
}
int Abc_SclGetRealFaninLit( Abc_Obj_t * pObj )
{
    int iLit;
    if ( !Abc_SclObjIsBufInv(pObj) )
        return Abc_Var2Lit( Abc_ObjId(pObj), 0 );
    iLit = Abc_SclGetRealFaninLit( Abc_ObjFanin0(pObj) );
    return Abc_LitNotCond( iLit, Abc_SclIsInv(pObj) );
}
Abc_Ntk_t * Abc_SclUnBufferPerform( Abc_Ntk_t * pNtk, int fVerbose )
{
    Vec_Int_t * vLits;
    Abc_Obj_t * pObj, * pFanin, * pFaninNew;
    int i, k, iLit, nNodesOld = Abc_NtkObjNumMax(pNtk);
    // assign inverters
    vLits = Vec_IntStartFull( Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachNode( pNtk, pObj, i )
        if ( Abc_SclIsInv(pObj) && !Abc_SclObjIsBufInv(Abc_ObjFanin0(pObj)) )
            Vec_IntWriteEntry( vLits, Abc_ObjFaninId0(pObj), Abc_ObjId(pObj) );
    // transfer fanins
    Abc_NtkForEachNodeCo( pNtk, pObj, i )
    {
        if ( i >= nNodesOld )
            break;
        Abc_ObjForEachFanin( pObj, pFanin, k )
        {
            if ( !Abc_SclObjIsBufInv(pFanin) )
                continue;
            iLit = Abc_SclGetRealFaninLit( pFanin );
            pFaninNew = Abc_NtkObj( pNtk, Abc_Lit2Var(iLit) );
            if ( Abc_LitIsCompl(iLit) )
            {
                if ( Vec_IntEntry( vLits, Abc_Lit2Var(iLit) ) == -1 )
                {
                    pFaninNew = Abc_NtkCreateNodeInv( pNtk, pFaninNew );
                    Vec_IntWriteEntry( vLits, Abc_Lit2Var(iLit), Abc_ObjId(pFaninNew) );
                }
                else
                    pFaninNew = Abc_NtkObj( pNtk, Vec_IntEntry( vLits, Abc_Lit2Var(iLit) ) );
                assert( Abc_ObjFaninNum(pFaninNew) == 1 );
            }
            if ( pFanin != pFaninNew )
                Abc_ObjPatchFanin( pObj, pFanin, pFaninNew );
        }
    }
    Vec_IntFree( vLits );
    // duplicate network in topo order
    return Abc_NtkDupDfs( pNtk );
}

/**Function*************************************************************

  Synopsis    [Removes buffers and inverters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SclCountMaxPhases( Abc_Ntk_t * pNtk )
{
    Vec_Int_t * vPhLevel;
    Abc_Obj_t * pObj, * pFanin;
    int i, k, Max = 0, MaxAll = 0;
    vPhLevel = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachNodeCo( pNtk, pObj, i )
    {
        Max = 0;
        Abc_ObjForEachFanin( pObj, pFanin, k )
            Max = Abc_MaxInt( Max, Vec_IntEntry(vPhLevel, Abc_ObjId(pFanin)) + Abc_ObjFaninPhase(pObj, k) );
        Vec_IntWriteEntry( vPhLevel, i, Max );
        MaxAll = Abc_MaxInt( MaxAll, Max );
    }
    Vec_IntFree( vPhLevel );
    return MaxAll;
}
Abc_Ntk_t * Abc_SclBufferPhase( Abc_Ntk_t * pNtk, int fVerbose )
{
    Abc_Ntk_t * pNtkNew;
    Vec_Int_t * vInvs;
    Abc_Obj_t * pObj, * pFanin, * pFaninNew;
    int nNodesOld = Abc_NtkObjNumMax(pNtk);
    int i, k, Counter = 0, Counter2 = 0, Total = 0;
    assert( pNtk->vPhases != NULL );
    vInvs = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachNodeCo( pNtk, pObj, i )
    {
        if ( i >= nNodesOld )
            break;
        Abc_ObjForEachFanin( pObj, pFanin, k )
        {
            Total++;
            if ( !Abc_ObjFaninPhase(pObj, k) )
                continue;
            if ( Vec_IntEntry(vInvs, Abc_ObjId(pFanin)) == 0 || Abc_ObjIsCi(pFanin) ) // allow PIs to have high fanout - to be fixed later
            {
                pFaninNew = Abc_NtkCreateNodeInv( pNtk, pFanin );
                Vec_IntWriteEntry( vInvs, Abc_ObjId(pFanin), Abc_ObjId(pFaninNew) );
                Counter++;
            }
            pFaninNew = Abc_NtkObj( pNtk, Vec_IntEntry(vInvs, Abc_ObjId(pFanin)) );
            Abc_ObjPatchFanin( pObj, pFanin, pFaninNew );
            Counter2++;
        }
    }
    if ( fVerbose )
        printf( "Added %d inverters (%.2f %% fanins) (%.2f %% compl fanins).\n", 
            Counter, 100.0 * Counter / Total, 100.0 * Counter2 / Total );
    Vec_IntFree( vInvs );
    Vec_IntFillExtra( pNtk->vPhases, Abc_NtkObjNumMax(pNtk), 0 );
    // duplicate network in topo order
    vInvs = pNtk->vPhases;
    pNtk->vPhases = NULL;
    pNtkNew = Abc_NtkDupDfs( pNtk );
    pNtk->vPhases = vInvs;
    return pNtkNew;
}
Abc_Ntk_t * Abc_SclUnBufferPhase( Abc_Ntk_t * pNtk, int fVerbose )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pFanin, * pFaninNew;
    int i, k, iLit, Counter = 0, Total = 0;
    assert( pNtk->vPhases == NULL );
    pNtk->vPhases = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachNodeCo( pNtk, pObj, i )
    {
        if ( Abc_SclObjIsBufInv(pObj) )
            continue;
        Abc_ObjForEachFanin( pObj, pFanin, k )
        {
            Total++;
            iLit = Abc_SclGetRealFaninLit( pFanin );
            pFaninNew = Abc_NtkObj( pNtk, Abc_Lit2Var(iLit) );
            if ( pFaninNew == pFanin )
                continue;
            // skip fanins which are already fanins of the node
            if ( Abc_NodeFindFanin( pObj, pFaninNew ) >= 0 )
                continue;
            Abc_ObjPatchFanin( pObj, pFanin, pFaninNew );
            if ( Abc_LitIsCompl(iLit) )
                Abc_ObjFaninFlipPhase( pObj, k ), Counter++;
        }
    }
    if ( fVerbose )
        printf( "Saved %d (%.2f %%) fanin phase bits.  ", Counter, 100.0 * Counter / Total );
    // duplicate network in topo order
    pNtkNew = Abc_NtkDupDfs( pNtk );
    if ( fVerbose )
        printf( "Max depth = %d.\n", Abc_SclCountMaxPhases(pNtkNew) );
    Abc_SclReportDupFanins( pNtkNew );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Make sure the network is in topo order without dangling nodes.]

  Description [Returns 1 iff the network is fine.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SclCheckNtk( Abc_Ntk_t * p, int fVerbose )
{
    Abc_Obj_t * pObj, * pFanin;
    int i, k, fFlag = 1;
    Abc_NtkIncrementTravId( p );        
    Abc_NtkForEachCi( p, pObj, i )
        Abc_NodeSetTravIdCurrent( pObj );
    Abc_NtkForEachNode( p, pObj, i )
    {
        Abc_ObjForEachFanin( pObj, pFanin, k )
            if ( !Abc_NodeIsTravIdCurrent( pFanin ) )
                printf( "obj %d and its fanin %d are not in the topo order\n", Abc_ObjId(pObj), Abc_ObjId(pFanin) ), fFlag = 0;
        Abc_NodeSetTravIdCurrent( pObj );
        if ( Abc_ObjIsBarBuf(pObj) )
            continue;
        if ( Abc_ObjFanoutNum(pObj) == 0 )
            printf( "node %d has no fanout\n", Abc_ObjId(pObj) ), fFlag = 0;
        if ( !fFlag )
            break;
    }
    if ( fFlag && fVerbose )
        printf( "The network is in topo order and no dangling nodes.\n" );
    return fFlag;
}

/**Function*************************************************************

  Synopsis    [Performs buffering of the mapped network (old code).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeInvUpdateFanPolarity( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanout;
    int i;
    assert( Abc_ObjFaninNum(pObj) == 0 || Abc_SclObjIsBufInv(pObj) );
    Abc_ObjForEachFanout( pObj, pFanout, i )
    {
        assert( Abc_ObjFaninNum(pFanout) > 0 );
        if ( Abc_SclObjIsBufInv(pFanout) )
            Abc_NodeInvUpdateFanPolarity( pFanout );
        else
            Abc_ObjFaninFlipPhase( pFanout, Abc_NodeFindFanin(pFanout, pObj) );
    }
}
void Abc_NodeInvUpdateObjFanoutPolarity( Abc_Obj_t * pObj, Abc_Obj_t * pFanout )
{
    if ( Abc_SclObjIsBufInv(pFanout) )
        Abc_NodeInvUpdateFanPolarity( pFanout );
    else
        Abc_ObjFaninFlipPhase( pFanout, Abc_NodeFindFanin(pFanout, pObj) );
}
int Abc_NodeCompareLevels( Abc_Obj_t ** pp1, Abc_Obj_t ** pp2 )
{
    int Diff = Abc_ObjLevel(*pp1) - Abc_ObjLevel(*pp2);
    if ( Diff < 0 )
        return -1;
    if ( Diff > 0 ) 
        return 1;
    Diff = (*pp1)->Id - (*pp2)->Id; // needed to make qsort() platform-infependent
    if ( Diff < 0 )
        return -1;
    if ( Diff > 0 ) 
        return 1;
    return 0; 
}
int Abc_SclComputeReverseLevel( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanout;
    int i, Level = 0;
    Abc_ObjForEachFanout( pObj, pFanout, i )
        Level = Abc_MaxInt( Level, pFanout->Level );
    return Level + 1;
}
Abc_Obj_t * Abc_SclPerformBufferingOne( Abc_Obj_t * pObj, int Degree, int fUseInvs, int fVerbose )
{
    Vec_Ptr_t * vFanouts;
    Abc_Obj_t * pBuffer, * pFanout;
    int i, Degree0 = Degree;
    assert( Abc_ObjFanoutNum(pObj) > Degree );
    // collect fanouts and sort by reverse level
    vFanouts = Vec_PtrAlloc( Abc_ObjFanoutNum(pObj) );
    Abc_NodeCollectFanouts( pObj, vFanouts );
    Vec_PtrSort( vFanouts, (int (*)(const void *, const void *))Abc_NodeCompareLevels );
    // select the first Degree fanouts
    if ( fUseInvs )
        pBuffer = Abc_NtkCreateNodeInv( pObj->pNtk, NULL );
    else
        pBuffer = Abc_NtkCreateNodeBuf( pObj->pNtk, NULL );
    // check if it is possible to not increase level
    if ( Vec_PtrSize(vFanouts) < 2 * Degree )
    {
        Abc_Obj_t * pFanPrev = (Abc_Obj_t *)Vec_PtrEntry(vFanouts, Vec_PtrSize(vFanouts)-1-Degree);
        Abc_Obj_t * pFanThis = (Abc_Obj_t *)Vec_PtrEntry(vFanouts, Degree-1);
        Abc_Obj_t * pFanLast = (Abc_Obj_t *)Vec_PtrEntryLast(vFanouts);
        if ( Abc_ObjLevel(pFanThis) == Abc_ObjLevel(pFanLast) &&
             Abc_ObjLevel(pFanPrev) <  Abc_ObjLevel(pFanThis) )
        {
            // find the first one whose level is the same as last
            Vec_PtrForEachEntry( Abc_Obj_t *, vFanouts, pFanout, i )
                if ( Abc_ObjLevel(pFanout) == Abc_ObjLevel(pFanLast) )
                    break;
            assert( i < Vec_PtrSize(vFanouts) );
            if ( i > 1 )
                Degree = i;
        }
        // make the last two more well-balanced
        if ( Degree == Degree0 && Degree > Vec_PtrSize(vFanouts) - Degree )
            Degree = Vec_PtrSize(vFanouts)/2 + (Vec_PtrSize(vFanouts) & 1);
        assert( Degree <= Degree0 );
    }
    // select fanouts
    Vec_PtrForEachEntryStop( Abc_Obj_t *, vFanouts, pFanout, i, Degree )
        Abc_ObjPatchFanin( pFanout, pObj, pBuffer );
    if ( fVerbose )
    {
        printf( "%5d : ", Abc_ObjId(pObj) );
        Vec_PtrForEachEntry( Abc_Obj_t *, vFanouts, pFanout, i )
            printf( "%d%s ", Abc_ObjLevel(pFanout), i == Degree-1 ? "  " : "" );
        printf( "\n" );
    }
    Vec_PtrFree( vFanouts );
    Abc_ObjAddFanin( pBuffer, pObj );
    pBuffer->Level = Abc_SclComputeReverseLevel( pBuffer );
    if ( fUseInvs )
        Abc_NodeInvUpdateFanPolarity( pBuffer );
    return pBuffer;
}
void Abc_SclPerformBuffering_rec( Abc_Obj_t * pObj, int DegreeR, int Degree, int fUseInvs, int fVerbose )
{
    Vec_Ptr_t * vFanouts;
    Abc_Obj_t * pBuffer;
    Abc_Obj_t * pFanout;
    int i, nOldFanNum;
    if ( Abc_NodeIsTravIdCurrent( pObj ) )
        return;
    Abc_NodeSetTravIdCurrent( pObj );
    pObj->Level = 0;
    if ( Abc_ObjIsCo(pObj) )
        return;
    assert( Abc_ObjIsCi(pObj) || Abc_ObjIsNode(pObj) );
    // buffer fanouts and collect reverse levels
    Abc_ObjForEachFanout( pObj, pFanout, i )
        Abc_SclPerformBuffering_rec( pFanout, DegreeR, Degree, fUseInvs, fVerbose );
    // perform buffering as long as needed
    nOldFanNum = Abc_ObjFanoutNum(pObj);
    while ( Abc_ObjFanoutNum(pObj) > Degree )
        Abc_SclPerformBufferingOne( pObj, Degree, fUseInvs, fVerbose );
    // add yet another level of buffers
    if ( DegreeR && nOldFanNum > DegreeR )
    {
        if ( fUseInvs )
            pBuffer = Abc_NtkCreateNodeInv( pObj->pNtk, NULL );
        else
            pBuffer = Abc_NtkCreateNodeBuf( pObj->pNtk, NULL );
        vFanouts = Vec_PtrAlloc( Abc_ObjFanoutNum(pObj) );
        Abc_NodeCollectFanouts( pObj, vFanouts );
        Vec_PtrForEachEntry( Abc_Obj_t *, vFanouts, pFanout, i )
            Abc_ObjPatchFanin( pFanout, pObj, pBuffer );
        Vec_PtrFree( vFanouts );
        Abc_ObjAddFanin( pBuffer, pObj );
        pBuffer->Level = Abc_SclComputeReverseLevel( pBuffer );
        if ( fUseInvs )
            Abc_NodeInvUpdateFanPolarity( pBuffer );
    }
    // compute the new level of the node
    pObj->Level = Abc_SclComputeReverseLevel( pObj );
}
Abc_Ntk_t * Abc_SclPerformBuffering( Abc_Ntk_t * p, int DegreeR, int Degree, int fUseInvs, int fVerbose )
{
    Vec_Int_t * vCiLevs;
    Abc_Ntk_t * pNew;
    Abc_Obj_t * pObj;
    int i;
    assert( Abc_NtkHasMapping(p) );
    if ( fUseInvs )
    {
        printf( "Warning!!! Using inverters instead of buffers.\n" );
        if ( p->vPhases == NULL )
            printf( "The phases are not given. The result will not verify.\n" );
    }
    // remember CI levels
    vCiLevs = Vec_IntAlloc( Abc_NtkCiNum(p) );
    Abc_NtkForEachCi( p, pObj, i )
        Vec_IntPush( vCiLevs, Abc_ObjLevel(pObj) );
    // perform buffering
    Abc_NtkIncrementTravId( p );        
    Abc_NtkForEachCi( p, pObj, i )
        Abc_SclPerformBuffering_rec( pObj, DegreeR, Degree, fUseInvs, fVerbose );
    // recompute logic levels
    Abc_NtkForEachCi( p, pObj, i )
        pObj->Level = Vec_IntEntry( vCiLevs, i );
    Abc_NtkForEachNode( p, pObj, i )
        Abc_ObjLevelNew( pObj );
    Vec_IntFree( vCiLevs );
    // if phases are present
    if ( p->vPhases )
        Vec_IntFillExtra( p->vPhases, Abc_NtkObjNumMax(p), 0 );
    // duplication in topo order
    pNew = Abc_NtkDupDfs( p );
    Abc_SclCheckNtk( pNew, fVerbose );
//    Abc_NtkDelete( pNew );
    return pNew;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Abc_BufComputeArr( Buf_Man_t * p, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanin;
    int i;
    float DelayF, Delay = -ABC_INFINITY;
    Abc_ObjForEachFanin( pObj, pFanin, i )
    {
        if ( Vec_IntEntry(p->vOffsets, Abc_ObjId(pObj)) == -ABC_INFINITY )
            continue;
        DelayF = Abc_BufNodeArr(p, pFanin) + Abc_BufEdgeDelay(p, pObj, i);
        if ( Delay < DelayF )
            Delay = DelayF;
    }
    Abc_BufSetNodeArr( p, pObj, Delay );
    return Delay;
}
float Abc_BufComputeDep( Buf_Man_t * p, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanout;
    int i;
    float DelayF, Delay = -ABC_INFINITY;
    Abc_ObjForEachFanout( pObj, pFanout, i )
    {
        if ( Vec_IntEntry(p->vOffsets, Abc_ObjId(pFanout)) == -ABC_INFINITY )
            continue;
        DelayF = Abc_BufNodeDep(p, pFanout) + Abc_BufEdgeDelay(p, pFanout, Abc_NodeFindFanin(pFanout, pObj));
        if ( Delay < DelayF )
            Delay = DelayF;
    }
    Abc_BufSetNodeDep( p, pObj, Delay );
    return Delay;
}
void Abc_BufUpdateGlobal( Buf_Man_t * p )
{
    Abc_Obj_t * pObj;
    int i;
    p->DelayMax = 0;
    Abc_NtkForEachCo( p->pNtk, pObj, i )
        p->DelayMax = Abc_MaxInt( p->DelayMax, Abc_BufNodeArr(p, Abc_ObjFanin0(pObj)) );
}
void Abc_BufCreateEdges( Buf_Man_t * p, Abc_Obj_t * pObj )
{
    int k;
    Mio_Gate_t * pGate = Abc_ObjIsCo(pObj) ? NULL : (Mio_Gate_t *)pObj->pData;
    Vec_IntWriteEntry( p->vOffsets, Abc_ObjId(pObj), Vec_IntSize(p->vEdges) );
    for ( k = 0; k < Abc_ObjFaninNum(pObj); k++ )
        Vec_IntPush( p->vEdges, pGate ? (int)(1.0 * BUF_SCALE * Mio_GateReadPinDelay(pGate, k) / p->DelayInv) : 0 );
}
void Abc_BufAddToQue( Buf_Man_t * p, Abc_Obj_t * pObj )
{
    if ( Abc_ObjFanoutNum(pObj) < p->nFanMin || (!p->fBufPis && Abc_ObjIsCi(pObj)) )
        return;
    Vec_FltWriteEntry( p->vCounts, Abc_ObjId(pObj), Abc_ObjFanoutNum(pObj) );
    if ( Vec_QueIsMember(p->vQue, Abc_ObjId(pObj)) )
        Vec_QueUpdate( p->vQue, Abc_ObjId(pObj) );
    else
        Vec_QuePush( p->vQue, Abc_ObjId(pObj) );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_BufCollectTfoCone_rec( Abc_Obj_t * pNode, Vec_Int_t * vNodes )
{
    Abc_Obj_t * pNext;
    int i;
    if ( Abc_NodeIsTravIdCurrent( pNode ) )
        return;
    Abc_NodeSetTravIdCurrent( pNode );
    if ( Abc_ObjIsCo(pNode) )
        return;
    assert( Abc_ObjIsCi(pNode) || Abc_ObjIsNode(pNode) );
    Abc_ObjForEachFanout( pNode, pNext, i )
        Abc_BufCollectTfoCone_rec( pNext, vNodes );
    if ( Abc_ObjIsNode(pNode) )
        Vec_IntPush( vNodes, Abc_ObjId(pNode) );
}
void Abc_BufCollectTfoCone( Buf_Man_t * p, Abc_Obj_t * pObj )
{
    Vec_IntClear( p->vTfCone );
    Abc_NtkIncrementTravId( p->pNtk );
    Abc_BufCollectTfoCone_rec( pObj, p->vTfCone );
}
void Abc_BufUpdateArr( Buf_Man_t * p, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pNext;
    int i, Delay;
//    assert( Abc_ObjIsNode(pObj) );
    Abc_BufCollectTfoCone( p, pObj );
    Vec_IntReverseOrder( p->vTfCone );
    Abc_NtkForEachObjVec( p->vTfCone, p->pNtk, pNext, i )
    {
        Delay = Abc_BufComputeArr( p, pNext );
        p->DelayMax = Abc_MaxInt( p->DelayMax, Delay );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_BufCollectTfiCone_rec( Abc_Obj_t * pNode, Vec_Int_t * vNodes )
{
    Abc_Obj_t * pNext;
    int i;
    if ( Abc_NodeIsTravIdCurrent( pNode ) )
        return;
    Abc_NodeSetTravIdCurrent( pNode );
    if ( Abc_ObjIsCi(pNode) )
        return;
    assert( Abc_ObjIsNode(pNode) );
    Abc_ObjForEachFanin( pNode, pNext, i )
        Abc_BufCollectTfiCone_rec( pNext, vNodes );
    Vec_IntPush( vNodes, Abc_ObjId(pNode) );
}
void Abc_BufCollectTfiCone( Buf_Man_t * p, Abc_Obj_t * pObj )
{
    Vec_IntClear( p->vTfCone );
    Abc_NtkIncrementTravId( p->pNtk );
    Abc_BufCollectTfiCone_rec( pObj, p->vTfCone );
}
void Abc_BufUpdateDep( Buf_Man_t * p, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pNext;
    int i, Delay;
//    assert( Abc_ObjIsNode(pObj) );
    Abc_BufCollectTfiCone( p, pObj );
    Vec_IntReverseOrder( p->vTfCone );
    Abc_NtkForEachObjVec( p->vTfCone, p->pNtk, pNext, i )
    {
        Delay = Abc_BufComputeDep( p, pNext );
        p->DelayMax = Abc_MaxInt( p->DelayMax, Delay );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Buf_Man_t * Buf_ManStart( Abc_Ntk_t * pNtk, int FanMin, int FanMax, int fBufPis )
{
    Buf_Man_t * p;
    Abc_Obj_t * pObj;
    Vec_Ptr_t * vNodes;
    int i;
    p = ABC_CALLOC( Buf_Man_t, 1 );
    p->pNtk      = pNtk;
    p->nFanMin   = FanMin;
    p->nFanMax   = FanMax;
    p->fBufPis   = fBufPis;
    // allocate arrays
    p->nObjStart = Abc_NtkObjNumMax(p->pNtk);
    p->nObjAlloc = (6 * Abc_NtkObjNumMax(p->pNtk) / 3) + 100;
    p->vOffsets  = Vec_IntAlloc( p->nObjAlloc );
    p->vArr      = Vec_IntAlloc( p->nObjAlloc );
    p->vDep      = Vec_IntAlloc( p->nObjAlloc );
    p->vCounts   = Vec_FltAlloc( p->nObjAlloc );
    p->vQue      = Vec_QueAlloc( p->nObjAlloc );
    Vec_IntFill( p->vOffsets, p->nObjAlloc, -ABC_INFINITY );
    Vec_IntFill( p->vArr,     p->nObjAlloc, 0 );
    Vec_IntFill( p->vDep,     p->nObjAlloc, 0 );
    Vec_FltFill( p->vCounts,  p->nObjAlloc, -ABC_INFINITY );
    Vec_QueSetPriority( p->vQue, Vec_FltArrayP(p->vCounts) );
    // collect edge delays
    p->DelayInv  = Mio_GateReadPinDelay( Mio_LibraryReadInv((Mio_Library_t *)pNtk->pManFunc), 0 );
    p->vEdges    = Vec_IntAlloc( 1000 );
    // create edges
    vNodes = Abc_NtkDfs( p->pNtk, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        Abc_BufCreateEdges( p, pObj );
    Abc_NtkForEachCo( p->pNtk, pObj, i )
        Abc_BufCreateEdges( p, pObj );
    // derive delays
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        Abc_BufComputeArr( p, pObj );
    Vec_PtrForEachEntryReverse( Abc_Obj_t *, vNodes, pObj, i )
        Abc_BufComputeDep( p, pObj );
    Abc_BufUpdateGlobal( p );
//    Abc_NtkForEachNode( p->pNtk, pObj, i )
//        printf( "%4d : %4d %4d\n", i, Abc_BufNodeArr(p, pObj), Abc_BufNodeDep(p, pObj) );
    // create fanout queue
//    Abc_NtkForEachCi( p->pNtk, pObj, i )
//        Abc_BufAddToQue( p, pObj );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        Abc_BufAddToQue( p, pObj );
    Vec_PtrFree( vNodes );
    p->vDelays  = Vec_IntAlloc( 100 );
    p->vOrder   = Vec_IntAlloc( 100 );
    p->vNonCrit = Vec_IntAlloc( 100 );
    p->vTfCone  = Vec_IntAlloc( 100 );
    p->vFanouts = Vec_PtrAlloc( 100 );
    return p;
}
void Buf_ManStop( Buf_Man_t * p )
{
    printf( "Sep = %d. Dup = %d. Br0 = %d. Br1 = %d. BrC = %d.  ", 
        p->nSeparate, p->nDuplicate, p->nBranch0, p->nBranch1, p->nBranchCrit );
    printf( "Orig = %d. Add = %d. Rem = %d.\n", 
        p->nObjStart, Abc_NtkObjNumMax(p->pNtk) - p->nObjStart, 
        p->nObjAlloc - Abc_NtkObjNumMax(p->pNtk) );
    Vec_PtrFree( p->vFanouts );
    Vec_IntFree( p->vTfCone );
    Vec_IntFree( p->vNonCrit );
    Vec_IntFree( p->vDelays );
    Vec_IntFree( p->vOrder );
    Vec_IntFree( p->vOffsets );
    Vec_IntFree( p->vEdges );
    Vec_IntFree( p->vArr );
    Vec_IntFree( p->vDep );
//    Vec_QueCheck( p->vQue );
    Vec_QueFree( p->vQue );
    Vec_FltFree( p->vCounts );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_BufSortByDelay( Buf_Man_t * p, int iPivot )
{
    Abc_Obj_t * pObj, * pFanout;
    int i, Slack, * pOrder;
    Vec_IntClear( p->vDelays );
    pObj = Abc_NtkObj( p->pNtk, iPivot );
    Abc_ObjForEachFanout( pObj, pFanout, i )
    {
        Slack = Abc_BufEdgeSlack(p, pObj, pFanout);
        assert( Slack >= 0 );
        Vec_IntPush( p->vDelays, Abc_MaxInt(0, Slack) );
    }
    pOrder = Abc_QuickSortCost( Vec_IntArray(p->vDelays), Vec_IntSize(p->vDelays), 0 );
    Vec_IntClear( p->vOrder );
    for ( i = 0; i < Vec_IntSize(p->vDelays); i++ )
        Vec_IntPush( p->vOrder, Abc_ObjId(Abc_ObjFanout(pObj, pOrder[i])) );
    ABC_FREE( pOrder );
//    for ( i = 0; i < Vec_IntSize(p->vDelays); i++ )
//        printf( "%5d - %5d   ", Vec_IntEntry(p->vOrder, i), Abc_BufEdgeSlack(p, pObj, Abc_NtkObj(p->pNtk, Vec_IntEntry(p->vOrder, i))) );
    return p->vOrder;        
}
void Abc_BufPrintOne( Buf_Man_t * p, int iPivot )
{
    Abc_Obj_t * pObj, * pFanout;
    Vec_Int_t * vOrder;
    int i, Slack;
    pObj = Abc_NtkObj( p->pNtk, iPivot );
    vOrder = Abc_BufSortByDelay( p, iPivot );
    printf( "Node %5d  Fi = %d  Fo = %3d  Lev = %3d : {", iPivot, Abc_ObjFaninNum(pObj), Abc_ObjFanoutNum(pObj), Abc_ObjLevel(pObj) );
    Abc_NtkForEachObjVec( vOrder, p->pNtk, pFanout, i )
    {
        Slack = Abc_BufEdgeSlack( p, pObj, pFanout );
        printf( " %d(%d)", Abc_ObjId(pFanout), Slack );
    }
    printf( " }\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_BufReplaceBufsByInvs( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pInv;
    int i, Counter = 0;
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        if ( !Abc_NodeIsBuf(pObj) )
            continue;
        assert( pObj->pData == Mio_LibraryReadBuf((Mio_Library_t *)pNtk->pManFunc) );
        pObj->pData = Mio_LibraryReadInv((Mio_Library_t *)pNtk->pManFunc);
        pInv = Abc_NtkCreateNodeInv( pNtk, Abc_ObjFanin0(pObj) );
        Abc_ObjPatchFanin( pObj, Abc_ObjFanin0(pObj), pInv );
        Counter++;
    }
    printf( "Replaced %d buffers by invertor pairs.\n", Counter );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_BufComputeAverage( Buf_Man_t * p, int iPivot, Vec_Int_t * vOrder )
{
    Abc_Obj_t * pObj, * pFanout;
    int i, Average = 0;
    pObj = Abc_NtkObj( p->pNtk, iPivot );
    Abc_NtkForEachObjVec( vOrder, p->pNtk, pFanout, i )
        Average += Abc_BufEdgeSlack( p, pObj, pFanout );
    return Average / Vec_IntSize(vOrder);
}
Abc_Obj_t * Abc_BufFindNonBuffDriver( Buf_Man_t * p, Abc_Obj_t * pObj )
{
    return (Abc_ObjIsNode(pObj) && Abc_NodeIsBuf(pObj)) ? Abc_BufFindNonBuffDriver(p, Abc_ObjFanin0(pObj)) : pObj;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_BufCountNonCritical( Buf_Man_t * p, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanout;
    int i;
    Vec_IntClear( p->vNonCrit );
    Abc_ObjForEachFanout( pObj, pFanout, i )
        if ( Abc_BufEdgeSlack( p, pObj, pFanout ) > 7*BUF_SCALE/2 )
            Vec_IntPush( p->vNonCrit, Abc_ObjId(pFanout) );
    return Vec_IntSize(p->vNonCrit);
}
void Abc_BufPerformOne( Buf_Man_t * p, int iPivot, int fSkipDup, int fVerbose )
{
    Abc_Obj_t * pObj, * pFanout;
    int i, j, nCrit, nNonCrit;
//    int DelayMax = p->DelayMax;
    assert( Abc_NtkObjNumMax(p->pNtk) + 30 < p->nObjAlloc );
    pObj     = Abc_NtkObj( p->pNtk, iPivot );
//    assert( Vec_FltEntry(p->vCounts, iPivot) == (float)Abc_ObjFanoutNum(pObj) );
    nNonCrit = Abc_BufCountNonCritical( p, pObj );
    nCrit    = Abc_ObjFanoutNum(pObj) - nNonCrit;
if ( fVerbose )
{
//Abc_BufPrintOne( p, iPivot );
printf( "ObjId = %6d : %-10s   FI = %d. FO =%4d.  Crit =%4d.  ", 
    Abc_ObjId(pObj), Mio_GateReadName((Mio_Gate_t *)pObj->pData), Abc_ObjFaninNum(pObj), Abc_ObjFanoutNum(pObj), nCrit );
}
    // consider three cases
    if ( nCrit > 0 && nNonCrit > 1 )
    {
        // (1) both critical and non-critical are present - split them by adding buffer
        Abc_Obj_t * pBuffer = Abc_NtkCreateNodeBuf( p->pNtk, pObj );
        Abc_NtkForEachObjVec( p->vNonCrit, p->pNtk, pFanout, i )
            Abc_ObjPatchFanin( pFanout, pObj, pBuffer );
        // update timing
        Abc_BufCreateEdges( p, pBuffer );
        Abc_BufUpdateArr( p, pBuffer );
        Abc_BufUpdateDep( p, pBuffer );
        Abc_BufAddToQue( p, pObj );
        Abc_BufAddToQue( p, pBuffer );
        Abc_SclTimeIncUpdateLevel( pBuffer );
        p->nSeparate++;
if ( fVerbose )
printf( "Adding buffer\n" );
    }
    else if ( !fSkipDup && nCrit > 0 && Abc_ObjIsNode(pObj) && Abc_ObjFanoutNum(pObj) > p->nFanMin )//&& Abc_ObjLevel(pObj) < 4 )//&& Abc_ObjFaninNum(pObj) < 2 )
    {
        // (2) only critical are present - duplicate
        Abc_Obj_t * pClone = Abc_NtkDupObj( p->pNtk, pObj, 0 );
        Abc_ObjForEachFanin( pObj, pFanout, i )
            Abc_ObjAddFanin( pClone, pFanout );
        Abc_NodeCollectFanouts( pObj, p->vFanouts );
        Vec_PtrForEachEntryStop( Abc_Obj_t *, p->vFanouts, pFanout, i, Vec_PtrSize(p->vFanouts)/2 )
            Abc_ObjPatchFanin( pFanout, pObj, pClone );
        // update timing
        Abc_BufCreateEdges( p, pClone );
        Abc_BufSetNodeArr( p, pClone, Abc_BufNodeArr(p, pObj) );
        Abc_BufUpdateDep( p, pObj );
        Abc_BufUpdateDep( p, pClone );
        Abc_BufAddToQue( p, pObj );
        Abc_BufAddToQue( p, pClone );
        Abc_ObjForEachFanin( pObj, pFanout, i )
            Abc_BufAddToQue( p, pFanout );
        Abc_SclTimeIncUpdateLevel( pClone );
        p->nDuplicate++;
//        printf( "Duplicating %s on level %d\n", Mio_GateReadName((Mio_Gate_t *)pObj->pData), Abc_ObjLevel(pObj) );
if ( fVerbose )
printf( "Duplicating node\n" );
    }
    else if ( (nCrit > 0 && Abc_ObjFanoutNum(pObj) > 8) || Abc_ObjFanoutNum(pObj) > p->nFanMax )
    {
        // (2) only critical or only non-critical - add buffer/inverter tree
        int nDegree, n1Degree, n1Number, nFirst;
        int iFirstBuf = Abc_NtkObjNumMax( p->pNtk );
//        nDegree  = Abc_MinInt( 3, (int)pow(Abc_ObjFanoutNum(pObj), 0.34) );
        nDegree  = Abc_MinInt( 10, (int)pow(Abc_ObjFanoutNum(pObj), 0.5) );
        n1Degree = Abc_ObjFanoutNum(pObj) / nDegree + 1;
        n1Number = Abc_ObjFanoutNum(pObj) % nDegree;
        nFirst   = n1Degree * n1Number;
        p->nBranchCrit += (nCrit > 0);
        // create inverters
        Abc_NodeCollectFanouts( pObj, p->vFanouts );
        if ( Abc_ObjIsNode(pObj) && Abc_NodeIsBuf(pObj) )
        {
            p->nBranch0++;
            pObj->pData = Mio_LibraryReadInv((Mio_Library_t *)p->pNtk->pManFunc);
            Abc_BufSetEdgeDelay( p, pObj, 0, BUF_SCALE );
            assert( Abc_NodeIsInv(pObj) );
            for ( i = 0; i < nDegree; i++ )
                Abc_NtkCreateNodeInv( p->pNtk, pObj );
if ( fVerbose )
printf( "Adding %d inverters\n", nDegree );
        }
        else
        {
            p->nBranch1++;
            for ( i = 0; i < nDegree; i++ )
                Abc_NtkCreateNodeBuf( p->pNtk, pObj );
if ( fVerbose )
printf( "Adding %d buffers\n", nDegree );
        }
        // connect inverters
        Vec_PtrForEachEntry( Abc_Obj_t *, p->vFanouts, pFanout, i )
        {
            j = (i < nFirst) ? i/n1Degree : n1Number + ((i - nFirst)/(n1Degree - 1));
            assert( j >= 0 && j < nDegree );
            Abc_ObjPatchFanin( pFanout, pObj, Abc_NtkObj(p->pNtk, iFirstBuf + j) );
        }
        // update timing
        for ( i = 0; i < nDegree; i++ )
            Abc_BufCreateEdges( p, Abc_NtkObj(p->pNtk, iFirstBuf + i) );
        Abc_BufUpdateArr( p, pObj );
        for ( i = 0; i < nDegree; i++ )
            Abc_BufComputeDep( p, Abc_NtkObj(p->pNtk, iFirstBuf + i) );
        Abc_BufUpdateDep( p, pObj );
        for ( i = 0; i < nDegree; i++ )
            Abc_BufAddToQue( p, Abc_NtkObj(p->pNtk, iFirstBuf + i) );
        for ( i = 0; i < nDegree; i++ )
            Abc_SclTimeIncUpdateLevel( Abc_NtkObj(p->pNtk, iFirstBuf + i) );
    }
    else
    {
if ( fVerbose )
printf( "Doing nothing\n" );
    }
//    if ( DelayMax != p->DelayMax )
//        printf( "%d (%.2f)  ", p->DelayMax, 1.0 * p->DelayMax * p->DelayInv / BUF_SCALE );
}
Abc_Ntk_t * Abc_SclBufPerform( Abc_Ntk_t * pNtk, int FanMin, int FanMax, int fBufPis, int fSkipDup, int fVerbose )
{
    Abc_Ntk_t * pNew;
    Buf_Man_t * p = Buf_ManStart( pNtk, FanMin, FanMax, fBufPis );
    int i, Limit = ABC_INFINITY;
    Abc_NtkLevel( pNtk );
//    if ( Abc_NtkNodeNum(pNtk) < 1000 )
//        fSkipDup = 1;
    for ( i = 0; i < Limit && Vec_QueSize(p->vQue); i++ )
        Abc_BufPerformOne( p, Vec_QuePop(p->vQue), fSkipDup, fVerbose );
    Buf_ManStop( p );
//    Abc_BufReplaceBufsByInvs( pNtk );
    // duplicate network in topo order
    pNew = Abc_NtkDupDfs( pNtk );
    Abc_SclCheckNtk( pNew, fVerbose );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

