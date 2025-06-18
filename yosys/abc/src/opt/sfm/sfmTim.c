/**CFile****************************************************************

  FileName    [sfmTim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based optimization using internal don't-cares.]

  Synopsis    [Timing manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: sfmTim.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sfmInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Sfm_Tim_t_
{
    // external
    Mio_Library_t *   pLib;        // library
    Scl_Con_t *       pExt;        // external timing
    Abc_Ntk_t *       pNtk;        // mapped network
    int               Delay;       // the largest delay
    int               DeltaCrit;   // critical delay delta
    // timing info
    Vec_Int_t         vTimArrs;    // arrivals (rise/fall)
    Vec_Int_t         vTimReqs;    // required (rise/fall)
    // incremental timing
    Vec_Wec_t         vLevels;     // levels
    // critical path
    Vec_Int_t         vPath;       // critical path
    Vec_Wrd_t         vSortData;   // node priority order
};

static inline int * Sfm_TimArrId( Sfm_Tim_t * p, int Id )                    { return Vec_IntEntryP( &p->vTimArrs,  Abc_Var2Lit(Id, 0) );               }
static inline int * Sfm_TimReqId( Sfm_Tim_t * p, int Id )                    { return Vec_IntEntryP( &p->vTimReqs,  Abc_Var2Lit(Id, 0) );               }

static inline int * Sfm_TimArr( Sfm_Tim_t * p, Abc_Obj_t * pNode )           { return Vec_IntEntryP( &p->vTimArrs,  Abc_Var2Lit(Abc_ObjId(pNode), 0) ); }
static inline int * Sfm_TimReq( Sfm_Tim_t * p, Abc_Obj_t * pNode )           { return Vec_IntEntryP( &p->vTimReqs,  Abc_Var2Lit(Abc_ObjId(pNode), 0) ); }

static inline int   Sfm_TimArrMaxId( Sfm_Tim_t * p, int Id )                 { int * a = Sfm_TimArrId(p, Id); return Abc_MaxInt(a[0], a[1]);            }

static inline int   Sfm_TimArrMax( Sfm_Tim_t * p, Abc_Obj_t * pNode )        { int * a = Sfm_TimArr(p, pNode); return Abc_MaxInt(a[0], a[1]);           }
static inline void  Sfm_TimSetReq( Sfm_Tim_t * p, Abc_Obj_t * pNode, int t ) { int * r = Sfm_TimReq(p, pNode); r[0] = r[1] = t;                         }
static inline int   Sfm_TimSlack( Sfm_Tim_t * p, Abc_Obj_t * pNode )         { int * r = Sfm_TimReq(p, pNode), * a = Sfm_TimArr(p, pNode); return Abc_MinInt(r[0]-a[0], r[1]-a[1]); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Sfm_TimEdgeArrival( Sfm_Tim_t * p, Mio_Pin_t * pPin, int * pTimeIn, int * pTimeOut )
{
    Mio_PinPhase_t PinPhase = Mio_PinReadPhase(pPin);
    int tDelayBlockRise = Scl_Flt2Int(Mio_PinReadDelayBlockRise(pPin));  
    int tDelayBlockFall = Scl_Flt2Int(Mio_PinReadDelayBlockFall(pPin));  
    if ( PinPhase != MIO_PHASE_INV )  // NONINV phase is present
    {
        pTimeOut[0] = Abc_MaxInt( pTimeOut[0], pTimeIn[0] + tDelayBlockRise );
        pTimeOut[1] = Abc_MaxInt( pTimeOut[1], pTimeIn[1] + tDelayBlockFall );
    }
    if ( PinPhase != MIO_PHASE_NONINV )  // INV phase is present
    {
        pTimeOut[0] = Abc_MaxInt( pTimeOut[0], pTimeIn[1] + tDelayBlockRise );
        pTimeOut[1] = Abc_MaxInt( pTimeOut[1], pTimeIn[0] + tDelayBlockFall );
    }
}
static inline void Sfm_TimGateArrival( Sfm_Tim_t * p, Mio_Gate_t * pGate, int ** pTimesIn, int * pTimeOut )
{
    Mio_Pin_t * pPin;  int i = 0;
    pTimeOut[0] = pTimeOut[1] = 0;
    Mio_GateForEachPin( pGate, pPin )
        Sfm_TimEdgeArrival( p, pPin, pTimesIn[i++], pTimeOut );
    assert( i == Mio_GateReadPinNum(pGate) );
}
static inline void Sfm_TimNodeArrival( Sfm_Tim_t * p, Abc_Obj_t * pNode )
{
    int i, iFanin, * pTimesIn[6];
    int * pTimeOut = Sfm_TimArr(p, pNode);
    assert( Abc_ObjFaninNum(pNode) <= 6 );
    Abc_ObjForEachFaninId( pNode, iFanin, i )
        pTimesIn[i] = Sfm_TimArrId( p, iFanin );
    Sfm_TimGateArrival( p, (Mio_Gate_t *)pNode->pData, pTimesIn, pTimeOut );
}

static inline void Sfm_TimEdgeRequired( Sfm_Tim_t * p, Mio_Pin_t * pPin, int * pTimeIn, int * pTimeOut )
{
    Mio_PinPhase_t PinPhase = Mio_PinReadPhase(pPin);
    int tDelayBlockRise = Scl_Flt2Int(Mio_PinReadDelayBlockRise(pPin));  
    int tDelayBlockFall = Scl_Flt2Int(Mio_PinReadDelayBlockFall(pPin));  
    if ( PinPhase != MIO_PHASE_INV )  // NONINV phase is present
    {
        pTimeIn[0] = Abc_MinInt( pTimeIn[0], pTimeOut[0] - tDelayBlockRise );
        pTimeIn[1] = Abc_MinInt( pTimeIn[1], pTimeOut[1] - tDelayBlockFall );
    }
    if ( PinPhase != MIO_PHASE_NONINV )  // INV phase is present
    {
        pTimeIn[0] = Abc_MinInt( pTimeIn[0], pTimeOut[1] - tDelayBlockRise );
        pTimeIn[1] = Abc_MinInt( pTimeIn[1], pTimeOut[0] - tDelayBlockFall );
    }
}
static inline void Sfm_TimGateRequired( Sfm_Tim_t * p, Mio_Gate_t * pGate, int ** pTimesIn, int * pTimeOut )
{
    Mio_Pin_t * pPin;  int i = 0;
    Mio_GateForEachPin( pGate, pPin )
        Sfm_TimEdgeRequired( p, pPin, pTimesIn[i++], pTimeOut );
    assert( i == Mio_GateReadPinNum(pGate) );
}
void Sfm_TimNodeRequired( Sfm_Tim_t * p, Abc_Obj_t * pNode )
{
    int i, iFanin, * pTimesIn[6];
    int * pTimeOut = Sfm_TimReq(p, pNode);
    assert( Abc_ObjFaninNum(pNode) <= 6 );
    Abc_ObjForEachFaninId( pNode, iFanin, i )
        pTimesIn[i] = Sfm_TimReqId( p, iFanin );
    Sfm_TimGateRequired( p, (Mio_Gate_t *)pNode->pData, pTimesIn, pTimeOut );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sfm_TimCriticalPath_int( Sfm_Tim_t * p, Abc_Obj_t * pObj, Vec_Int_t * vPath, int SlackMax )
{
    Abc_Obj_t * pNext; int i;
    if ( Abc_NodeIsTravIdCurrent( pObj ) )
        return;
    Abc_NodeSetTravIdCurrent( pObj );
    assert( Abc_ObjIsNode(pObj) );
    Abc_ObjForEachFanin( pObj, pNext, i )
    {
        if ( Abc_ObjIsCi(pNext) || Abc_ObjFaninNum(pNext) == 0 )
            continue;
        assert( Abc_ObjIsNode(pNext) );
        if ( Sfm_TimSlack(p, pNext) <= SlackMax )
            Sfm_TimCriticalPath_int( p, pNext, vPath, SlackMax );
    }
    if ( Abc_ObjFaninNum(pObj) > 0 )
        Vec_IntPush( vPath, Abc_ObjId(pObj) );
}
int Sfm_TimCriticalPath( Sfm_Tim_t * p, int Window )
{
    int i, SlackMax = p->Delay * Window / 100;
    Abc_Obj_t * pObj, * pNext; 
    Vec_IntClear( &p->vPath );
    Abc_NtkIncrementTravId( p->pNtk ); 
    Abc_NtkForEachCo( p->pNtk, pObj, i )
    {
        pNext = Abc_ObjFanin0(pObj);
        if ( Abc_ObjIsCi(pNext) || Abc_ObjFaninNum(pNext) == 0 )
            continue;
        assert( Abc_ObjIsNode(pNext) );
        if ( Sfm_TimSlack(p, pNext) <= SlackMax )
            Sfm_TimCriticalPath_int( p, pNext, &p->vPath, SlackMax );
    }
    return Vec_IntSize(&p->vPath);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_TimTrace( Sfm_Tim_t * p )
{
    Abc_Obj_t * pObj; int i, Delay = 0;
    Vec_Ptr_t * vNodes = Abc_NtkDfs( p->pNtk, 1 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        Sfm_TimNodeArrival( p, pObj );
    Abc_NtkForEachCo( p->pNtk, pObj, i )
        Delay = Abc_MaxInt( Delay, Sfm_TimArrMax(p, Abc_ObjFanin0(pObj)) );
    Vec_IntFill( &p->vTimReqs, 2*Abc_NtkObjNumMax(p->pNtk), ABC_INFINITY );
    Abc_NtkForEachCo( p->pNtk, pObj, i )
        Sfm_TimSetReq( p, Abc_ObjFanin0(pObj), Delay );
    Vec_PtrForEachEntryReverse( Abc_Obj_t *, vNodes, pObj, i )
        Sfm_TimNodeRequired( p, pObj );
    Vec_PtrFree( vNodes );
    return Delay;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sfm_Tim_t * Sfm_TimStart( Mio_Library_t * pLib, Scl_Con_t * pExt, Abc_Ntk_t * pNtk, int DeltaCrit )
{
    Sfm_Tim_t * p = ABC_CALLOC( Sfm_Tim_t, 1 );
    p->pLib = pLib;
    p->pExt = pExt;
    p->pNtk = pNtk;
    Vec_IntFill( &p->vTimArrs,  3*Abc_NtkObjNumMax(pNtk), 0 );
    Vec_IntFill( &p->vTimReqs,  3*Abc_NtkObjNumMax(pNtk), 0 );
    p->Delay = Sfm_TimTrace( p );
    assert( DeltaCrit > 0 && DeltaCrit < Scl_Flt2Int(1000.0) );
    p->DeltaCrit = DeltaCrit;
    return p;
}
void Sfm_TimStop( Sfm_Tim_t * p )
{
    Vec_IntErase( &p->vTimArrs );
    Vec_IntErase( &p->vTimReqs );
    Vec_WecErase( &p->vLevels );
    Vec_IntErase( &p->vPath );
    Vec_WrdErase( &p->vSortData );
    ABC_FREE( p );
}
int Sfm_TimReadNtkDelay( Sfm_Tim_t * p )
{
    return p->Delay;
}
int Sfm_TimReadObjDelay( Sfm_Tim_t * p, int iObj )
{
    return Sfm_TimArrMaxId(p, iObj);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sfm_TimTest( Abc_Ntk_t * pNtk )
{
    Mio_Library_t * pLib = (Mio_Library_t *)pNtk->pManFunc;
    Sfm_Tim_t * p = Sfm_TimStart( pLib, NULL, pNtk, 100 );
    printf( "Max delay = %.2f.  Path = %d (%d).\n", Scl_Int2Flt(p->Delay), Sfm_TimCriticalPath(p, 1), Abc_NtkNodeNum(p->pNtk) );
    Sfm_TimStop( p );
}

/**Function*************************************************************

  Synopsis    [Levelized structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Sfm_TimUpdateClean( Sfm_Tim_t * p )
{
    Vec_Int_t * vLevel;
    Abc_Obj_t * pObj;
    int i, k;
    Vec_WecForEachLevel( &p->vLevels, vLevel, i )
    {
        Abc_NtkForEachObjVec( vLevel, p->pNtk, pObj, k )
        {
            assert( pObj->fMarkC == 1 );
            pObj->fMarkC = 0;
        }
        Vec_IntClear( vLevel );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sfm_TimUpdateTiming( Sfm_Tim_t * p, Vec_Int_t * vTimeNodes )
{
    assert( Vec_IntSize(vTimeNodes) > 0 && Vec_IntSize(vTimeNodes) <= 2 );
    Vec_IntFillExtra( &p->vTimArrs, 2*Abc_NtkObjNumMax(p->pNtk), 0 );
    Vec_IntFillExtra( &p->vTimReqs, 2*Abc_NtkObjNumMax(p->pNtk), 0 );
    p->Delay = Sfm_TimTrace( p );
}

/**Function*************************************************************

  Synopsis    [Sort an array of nodes using their max arrival times.]

  Description [Returns the number of new divisor nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_TimSortArrayByArrival( Sfm_Tim_t * p, Vec_Int_t * vNodes, int iPivot )
{
    word Entry; 
    int i, Id, Time, nDivNew = -1; 
    int MaxDelay = ABC_INFINITY/2+Sfm_TimArrMaxId(p, iPivot);
    assert( p->DeltaCrit > 0 );
    // collect nodes
    Vec_WrdClear( &p->vSortData );
    Vec_IntForEachEntry( vNodes, Id, i )
    {
        Time = Sfm_TimArrMaxId( p, Id );
        assert( -ABC_INFINITY/2 < Time && Time < ABC_INFINITY/2 );
        Vec_WrdPush( &p->vSortData, ((word)Id << 32) | (ABC_INFINITY/2+Time) );
    }
    // sort nodes by delay
    Abc_QuickSort3( Vec_WrdArray(&p->vSortData), Vec_WrdSize(&p->vSortData), 0 );
    // collect sorted nodes and find place where divisors end
    Vec_IntClear( vNodes );
    Vec_WrdForEachEntry( &p->vSortData, Entry, i )
    {
        Vec_IntPush( vNodes, (int)(Entry >> 32) );
        if ( nDivNew == -1 && ((int)Entry) + p->DeltaCrit > MaxDelay )
            nDivNew = i;
    }
    return nDivNew;
}

/**Function*************************************************************

  Synopsis    [Priority of nodes to try remapping for delay.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_TimPriorityNodes( Sfm_Tim_t * p, Vec_Int_t * vCands, int Window )
{
    Vec_Int_t * vLevel;
    Abc_Obj_t * pObj;
    int i, k;
    assert( Window >= 0 && Window <= 100 );
    // collect critical path
    Sfm_TimCriticalPath( p, Window );
    // add nodes to the levelized structure
    Sfm_TimUpdateClean( p );
    Abc_NtkForEachObjVec( &p->vPath, p->pNtk, pObj, i )
    {
        assert( pObj->fMarkC == 0 );
        pObj->fMarkC = 1;
        Vec_WecPush( &p->vLevels, Abc_ObjLevel(pObj), Abc_ObjId(pObj) );
    }
    // prioritize nodes by expected gain
    Vec_WecSort( &p->vLevels, 0 );
    Vec_IntClear( vCands );
    Vec_WecForEachLevel( &p->vLevels, vLevel, i )
        Abc_NtkForEachObjVec( vLevel, p->pNtk, pObj, k )
            if ( !pObj->fMarkA )
                Vec_IntPush( vCands, Abc_ObjId(pObj) );
//    printf( "Path = %5d   Cand = %5d\n", Vec_IntSize(&p->vPath) );
    return Vec_IntSize(vCands) > 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if node is relatively non-critical compared to the pivot.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_TimNodeIsNonCritical( Sfm_Tim_t * p, Abc_Obj_t * pPivot, Abc_Obj_t * pNode )
{
    return Sfm_TimArrMax(p, pNode) + p->DeltaCrit <= Sfm_TimArrMax(p, pPivot);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_TimEvalRemapping( Sfm_Tim_t * p, Vec_Int_t * vFanins, Vec_Int_t * vMap, Mio_Gate_t * pGate1, char * pFans1, Mio_Gate_t * pGate2, char * pFans2 )
{
    int TimeOut[2][2];
    int * pTimesIn1[6], * pTimesIn2[6];
    int i, nFanins1, nFanins2;
    // process the first gate
    nFanins1 = Mio_GateReadPinNum( pGate1 );
    for ( i = 0; i < nFanins1; i++ )
        pTimesIn1[i] = Sfm_TimArrId( p, Vec_IntEntry(vMap, Vec_IntEntry(vFanins, (int)pFans1[i])) );
    Sfm_TimGateArrival( p, pGate1, pTimesIn1, TimeOut[0] );
    if ( pGate2 == NULL )
        return Abc_MaxInt(TimeOut[0][0], TimeOut[0][1]);
    // process the second gate
    nFanins2 = Mio_GateReadPinNum( pGate2 );
    for ( i = 0; i < nFanins2; i++ )
        if ( (int)pFans2[i] == 16 )
            pTimesIn2[i] = TimeOut[0];
        else
            pTimesIn2[i] = Sfm_TimArrId( p, Vec_IntEntry(vMap, Vec_IntEntry(vFanins, (int)pFans2[i])) );
    Sfm_TimGateArrival( p, pGate2, pTimesIn2, TimeOut[1] );
    return Abc_MaxInt(TimeOut[1][0], TimeOut[1][1]);
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

