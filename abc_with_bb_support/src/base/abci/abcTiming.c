/**CFile****************************************************************

  FileName    [abcTiming.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Computation of timing info for mapped circuits.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcTiming.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "main.h"
#include "mio.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Abc_ManTime_t_
{
    Abc_Time_t     tArrDef;
    Abc_Time_t     tReqDef;
    Vec_Ptr_t  *   vArrs;
    Vec_Ptr_t  *   vReqs;
};

// static functions
static Abc_ManTime_t *     Abc_ManTimeStart();
static void                Abc_ManTimeExpand( Abc_ManTime_t * p, int nSize, int fProgressive );
void                       Abc_NtkTimePrepare( Abc_Ntk_t * pNtk );

void                       Abc_NodeDelayTraceArrival( Abc_Obj_t * pNode );

// accessing the arrival and required times of a node
static inline Abc_Time_t * Abc_NodeArrival( Abc_Obj_t * pNode )  {  return pNode->pNtk->pManTime->vArrs->pArray[pNode->Id];  }
static inline Abc_Time_t * Abc_NodeRequired( Abc_Obj_t * pNode ) {  return pNode->pNtk->pManTime->vReqs->pArray[pNode->Id];  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the arrival time of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Time_t * Abc_NodeReadArrival( Abc_Obj_t * pNode )
{
    assert( pNode->pNtk->pManTime );
    return Abc_NodeArrival(pNode);
}

/**Function*************************************************************

  Synopsis    [Reads the arrival time of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Time_t * Abc_NodeReadRequired( Abc_Obj_t * pNode )
{
    assert( pNode->pNtk->pManTime );
    return Abc_NodeRequired(pNode);
}

/**Function*************************************************************

  Synopsis    [Reads the arrival time of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Time_t * Abc_NtkReadDefaultArrival( Abc_Ntk_t * pNtk )
{
    assert( pNtk->pManTime );
    return &pNtk->pManTime->tArrDef;
}

/**Function*************************************************************

  Synopsis    [Reads the arrival time of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Time_t * Abc_NtkReadDefaultRequired( Abc_Ntk_t * pNtk )
{
    assert( pNtk->pManTime );
    return &pNtk->pManTime->tReqDef;
}

/**Function*************************************************************

  Synopsis    [Sets the default arrival time for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTimeSetDefaultArrival( Abc_Ntk_t * pNtk, float Rise, float Fall )
{
    if ( Rise == 0.0 && Fall == 0.0 )
        return;
    if ( pNtk->pManTime == NULL )
        pNtk->pManTime = Abc_ManTimeStart();
    pNtk->pManTime->tArrDef.Rise  = Rise;
    pNtk->pManTime->tArrDef.Fall  = Fall;
    pNtk->pManTime->tArrDef.Worst = ABC_MAX( Rise, Fall );
}

/**Function*************************************************************

  Synopsis    [Sets the default arrival time for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTimeSetDefaultRequired( Abc_Ntk_t * pNtk, float Rise, float Fall )
{
    if ( Rise == 0.0 && Fall == 0.0 )
        return;
    if ( pNtk->pManTime == NULL )
        pNtk->pManTime = Abc_ManTimeStart();
    pNtk->pManTime->tReqDef.Rise  = Rise;
    pNtk->pManTime->tReqDef.Rise  = Fall;
    pNtk->pManTime->tReqDef.Worst = ABC_MAX( Rise, Fall );
}

/**Function*************************************************************

  Synopsis    [Sets the arrival time for an object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTimeSetArrival( Abc_Ntk_t * pNtk, int ObjId, float Rise, float Fall )
{
    Vec_Ptr_t * vTimes;
    Abc_Time_t * pTime;
    if ( pNtk->pManTime == NULL )
        pNtk->pManTime = Abc_ManTimeStart();
    if ( pNtk->pManTime->tArrDef.Rise == Rise && pNtk->pManTime->tArrDef.Fall == Fall )
        return;
    Abc_ManTimeExpand( pNtk->pManTime, ObjId + 1, 1 );
    // set the arrival time
    vTimes = pNtk->pManTime->vArrs;
    pTime = vTimes->pArray[ObjId];
    pTime->Rise  = Rise;
    pTime->Fall  = Rise;
    pTime->Worst = ABC_MAX( Rise, Fall );
}

/**Function*************************************************************

  Synopsis    [Sets the arrival time for an object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTimeSetRequired( Abc_Ntk_t * pNtk, int ObjId, float Rise, float Fall )
{
    Vec_Ptr_t * vTimes;
    Abc_Time_t * pTime;
    if ( pNtk->pManTime == NULL )
        pNtk->pManTime = Abc_ManTimeStart();
    if ( pNtk->pManTime->tReqDef.Rise == Rise && pNtk->pManTime->tReqDef.Fall == Fall )
        return;
    Abc_ManTimeExpand( pNtk->pManTime, ObjId + 1, 1 );
    // set the required time
    vTimes = pNtk->pManTime->vReqs;
    pTime = vTimes->pArray[ObjId];
    pTime->Rise  = Rise;
    pTime->Fall  = Rise;
    pTime->Worst = ABC_MAX( Rise, Fall );
}

/**Function*************************************************************

  Synopsis    [Finalizes the timing manager after setting arr/req times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTimeInitialize( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    Abc_Time_t ** ppTimes, * pTime;
    int i;
    if ( pNtk->pManTime == NULL )
        return;
    Abc_ManTimeExpand( pNtk->pManTime, Abc_NtkObjNumMax(pNtk), 0 );
    // set the default timing
    ppTimes = (Abc_Time_t **)pNtk->pManTime->vArrs->pArray;
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        pTime = ppTimes[pObj->Id];
        if ( pTime->Worst != -ABC_INFINITY )
            continue;
        *pTime = pNtk->pManTime->tArrDef;
    }
    // set the default timing
    ppTimes = (Abc_Time_t **)pNtk->pManTime->vReqs->pArray;
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        pTime = ppTimes[pObj->Id];
        if ( pTime->Worst != -ABC_INFINITY )
            continue;
        *pTime = pNtk->pManTime->tReqDef;
    }
    // set the 0 arrival times for latch outputs and constant nodes
    ppTimes = (Abc_Time_t **)pNtk->pManTime->vArrs->pArray;
    Abc_NtkForEachLatchOutput( pNtk, pObj, i )
    {
        pTime = ppTimes[pObj->Id];
        pTime->Fall = pTime->Rise = pTime->Worst = 0.0;
    }
}

/**Function*************************************************************

  Synopsis    [Prepares the timing manager for delay trace.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTimePrepare( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    Abc_Time_t ** ppTimes, * pTime;
    int i;
    // if there is no timing manager, allocate and initialize
    if ( pNtk->pManTime == NULL )
    {
        pNtk->pManTime = Abc_ManTimeStart();
        Abc_NtkTimeInitialize( pNtk );
        return;
    }
    // if timing manager is given, expand it if necessary
    Abc_ManTimeExpand( pNtk->pManTime, Abc_NtkObjNumMax(pNtk), 0 );
    // clean arrivals except for PIs
    ppTimes = (Abc_Time_t **)pNtk->pManTime->vArrs->pArray;
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        pTime = ppTimes[pObj->Id];
        pTime->Fall = pTime->Rise = pTime->Worst = -ABC_INFINITY;
    }
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        pTime = ppTimes[pObj->Id];
        pTime->Fall = pTime->Rise = pTime->Worst = -ABC_INFINITY;
    }
    // clean required except for POs
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_ManTime_t * Abc_ManTimeStart()
{
    Abc_ManTime_t * p;
    p = ALLOC( Abc_ManTime_t, 1 );
    memset( p, 0, sizeof(Abc_ManTime_t) );
    p->vArrs = Vec_PtrAlloc( 0 );
    p->vReqs = Vec_PtrAlloc( 0 );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ManTimeStop( Abc_ManTime_t * p )
{
    if ( p->vArrs->nSize > 0 )
    {
        free( p->vArrs->pArray[0] );
        Vec_PtrFree( p->vArrs );
    }
    if ( p->vReqs->nSize > 0 )
    {
        free( p->vReqs->pArray[0] );
        Vec_PtrFree( p->vReqs );
    }
    free( p );
}

/**Function*************************************************************

  Synopsis    [Duplicates the timing manager with the PI/PO timing info.]

  Description [The PIs/POs of the new network should be allocated.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ManTimeDup( Abc_Ntk_t * pNtkOld, Abc_Ntk_t * pNtkNew )
{
    Abc_Obj_t * pObj;
    Abc_Time_t ** ppTimesOld, ** ppTimesNew;
    int i;
    if ( pNtkOld->pManTime == NULL )
        return;
    assert( Abc_NtkPiNum(pNtkOld) == Abc_NtkPiNum(pNtkNew) );
    assert( Abc_NtkPoNum(pNtkOld) == Abc_NtkPoNum(pNtkNew) );
    assert( Abc_NtkLatchNum(pNtkOld) == Abc_NtkLatchNum(pNtkNew) );
    // create the new timing manager
    pNtkNew->pManTime = Abc_ManTimeStart();
    Abc_ManTimeExpand( pNtkNew->pManTime, Abc_NtkObjNumMax(pNtkNew), 0 );
    // set the default timing
    pNtkNew->pManTime->tArrDef = pNtkOld->pManTime->tArrDef;
    pNtkNew->pManTime->tReqDef = pNtkOld->pManTime->tReqDef;
    // set the CI timing
    ppTimesOld = (Abc_Time_t **)pNtkOld->pManTime->vArrs->pArray;
    ppTimesNew = (Abc_Time_t **)pNtkNew->pManTime->vArrs->pArray;
    Abc_NtkForEachCi( pNtkOld, pObj, i )
        *ppTimesNew[ Abc_NtkCi(pNtkNew,i)->Id ] = *ppTimesOld[ pObj->Id ];
    // set the CO timing
    ppTimesOld = (Abc_Time_t **)pNtkOld->pManTime->vReqs->pArray;
    ppTimesNew = (Abc_Time_t **)pNtkNew->pManTime->vReqs->pArray;
    Abc_NtkForEachCo( pNtkOld, pObj, i )
        *ppTimesNew[ Abc_NtkCo(pNtkNew,i)->Id ] = *ppTimesOld[ pObj->Id ];
}

/**Function*************************************************************

  Synopsis    [Expends the storage for timing information.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ManTimeExpand( Abc_ManTime_t * p, int nSize, int fProgressive )
{
    Vec_Ptr_t * vTimes;
    Abc_Time_t * ppTimes, * ppTimesOld, * pTime;
    int nSizeOld, nSizeNew, i;

    nSizeOld = p->vArrs->nSize;
    if ( nSizeOld >= nSize )
        return;
    nSizeNew = fProgressive? 2 * nSize : nSize;
    if ( nSizeNew < 100 )
        nSizeNew = 100;

    vTimes = p->vArrs;
    Vec_PtrGrow( vTimes, nSizeNew );
    vTimes->nSize = nSizeNew;
    ppTimesOld = ( nSizeOld == 0 )? NULL : vTimes->pArray[0];
    ppTimes = REALLOC( Abc_Time_t, ppTimesOld, nSizeNew );
    for ( i = 0; i < nSizeNew; i++ )
        vTimes->pArray[i] = ppTimes + i;
    for ( i = nSizeOld; i < nSizeNew; i++ )
    {
        pTime = vTimes->pArray[i];
        pTime->Rise  = -ABC_INFINITY;
        pTime->Fall  = -ABC_INFINITY;
        pTime->Worst = -ABC_INFINITY;    
    }

    vTimes = p->vReqs;
    Vec_PtrGrow( vTimes, nSizeNew );
    vTimes->nSize = nSizeNew;
    ppTimesOld = ( nSizeOld == 0 )? NULL : vTimes->pArray[0];
    ppTimes = REALLOC( Abc_Time_t, ppTimesOld, nSizeNew );
    for ( i = 0; i < nSizeNew; i++ )
        vTimes->pArray[i] = ppTimes + i;
    for ( i = nSizeOld; i < nSizeNew; i++ )
    {
        pTime = vTimes->pArray[i];
        pTime->Rise  = -ABC_INFINITY;
        pTime->Fall  = -ABC_INFINITY;
        pTime->Worst = -ABC_INFINITY;    
    }
}






/**Function*************************************************************

  Synopsis    [Sets the CI node levels according to the arrival info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkSetNodeLevelsArrival( Abc_Ntk_t * pNtkOld )
{
    Abc_Obj_t * pNodeOld, * pNodeNew;
    float tAndDelay;
    int i;
    if ( pNtkOld->pManTime == NULL )
        return;
    if ( Mio_LibraryReadNand2(Abc_FrameReadLibGen()) == NULL )
        return;
    tAndDelay = Mio_LibraryReadDelayNand2Max(Abc_FrameReadLibGen());
    Abc_NtkForEachPi( pNtkOld, pNodeOld, i )
    {
        pNodeNew = pNodeOld->pCopy;
        pNodeNew->Level = (int)(Abc_NodeArrival(pNodeOld)->Worst / tAndDelay);
    }
}

/**Function*************************************************************

  Synopsis    [Sets the CI node levels according to the arrival info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Time_t * Abc_NtkGetCiArrivalTimes( Abc_Ntk_t * pNtk )
{
    Abc_Time_t * p;
    Abc_Obj_t * pNode;
    int i;
    p = ALLOC( Abc_Time_t, Abc_NtkCiNum(pNtk) );
    memset( p, 0, sizeof(Abc_Time_t) * Abc_NtkCiNum(pNtk) );
    if ( pNtk->pManTime == NULL )
        return p;
    // set the PI arrival times
    Abc_NtkForEachPi( pNtk, pNode, i )
        p[i] = *Abc_NodeArrival(pNode);
    return p;
}


/**Function*************************************************************

  Synopsis    [Sets the CI node levels according to the arrival info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float * Abc_NtkGetCiArrivalFloats( Abc_Ntk_t * pNtk )
{
    float * p;
    Abc_Obj_t * pNode;
    int i;
    p = ALLOC( float, Abc_NtkCiNum(pNtk) );
    memset( p, 0, sizeof(float) * Abc_NtkCiNum(pNtk) );
    if ( pNtk->pManTime == NULL )
        return p;
    // set the PI arrival times
    Abc_NtkForEachPi( pNtk, pNode, i )
        p[i] = Abc_NodeArrival(pNode)->Worst;
    return p;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Abc_NtkDelayTrace( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode, * pDriver;
    Vec_Ptr_t * vNodes;
    Abc_Time_t * pTime;
    float tArrivalMax;
    int i;

    assert( Abc_NtkIsMappedLogic(pNtk) );

    Abc_NtkTimePrepare( pNtk );
    vNodes = Abc_NtkDfs( pNtk, 1 );
    for ( i = 0; i < vNodes->nSize; i++ )
        Abc_NodeDelayTraceArrival( vNodes->pArray[i] );
    Vec_PtrFree( vNodes );

    // get the latest arrival times
    tArrivalMax = -ABC_INFINITY;
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        pDriver = Abc_ObjFanin0(pNode);
        pTime   = Abc_NodeArrival(pDriver);
        if ( tArrivalMax < pTime->Worst )
            tArrivalMax = pTime->Worst;
    }
    return tArrivalMax;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeDelayTraceArrival( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanin;
    Abc_Time_t * pTimeIn, * pTimeOut;
    float tDelayBlockRise, tDelayBlockFall;
    Mio_PinPhase_t PinPhase;
    Mio_Pin_t * pPin;
    int i;

    // start the arrival time of the node
    pTimeOut = Abc_NodeArrival(pNode);
    pTimeOut->Rise = pTimeOut->Fall = -ABC_INFINITY; 
    // go through the pins of the gate
    pPin = Mio_GateReadPins(pNode->pData);
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        pTimeIn = Abc_NodeArrival(pFanin);
        // get the interesting parameters of this pin
        PinPhase = Mio_PinReadPhase(pPin);
        tDelayBlockRise = (float)Mio_PinReadDelayBlockRise( pPin );  
        tDelayBlockFall = (float)Mio_PinReadDelayBlockFall( pPin );  
        // compute the arrival times of the positive phase
        if ( PinPhase != MIO_PHASE_INV )  // NONINV phase is present
        {
            if ( pTimeOut->Rise < pTimeIn->Rise + tDelayBlockRise )
                pTimeOut->Rise = pTimeIn->Rise + tDelayBlockRise;
            if ( pTimeOut->Fall < pTimeIn->Fall + tDelayBlockFall )
                pTimeOut->Fall = pTimeIn->Fall + tDelayBlockFall;
        }
        if ( PinPhase != MIO_PHASE_NONINV )  // INV phase is present
        {
            if ( pTimeOut->Rise < pTimeIn->Fall + tDelayBlockRise )
                pTimeOut->Rise = pTimeIn->Fall + tDelayBlockRise;
            if ( pTimeOut->Fall < pTimeIn->Rise + tDelayBlockFall )
                pTimeOut->Fall = pTimeIn->Rise + tDelayBlockFall;
        }
        pPin = Mio_PinReadNext(pPin);
    }
    pTimeOut->Worst = ABC_MAX( pTimeOut->Rise, pTimeOut->Fall );
}




/**Function*************************************************************

  Synopsis    [Computes the level of the node using its fanin levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjLevelNew( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanin;
    int i, Level = 0;
    Abc_ObjForEachFanin( pObj, pFanin, i )
        Level = ABC_MAX( Level, Abc_ObjLevel(pFanin) );
    return Level + 1;
}

/**Function*************************************************************

  Synopsis    [Computes the reverse level of the node using its fanout levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjReverseLevelNew( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanout;
    int i, LevelCur, Level = 0;
    Abc_ObjForEachFanout( pObj, pFanout, i )
    {
        LevelCur = Abc_ObjReverseLevel( pFanout );
        Level = ABC_MAX( Level, LevelCur );
    }
    return Level + 1;
}

/**Function*************************************************************

  Synopsis    [Returns required level of the node.]

  Description [Converts the reverse levels of the node into its required 
  level as follows: ReqLevel(Node) = MaxLevels(Ntk) + 1 - LevelR(Node).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjRequiredLevel( Abc_Obj_t * pObj )
{
    Abc_Ntk_t * pNtk = pObj->pNtk;
    assert( pNtk->vLevelsR );
    return pNtk->LevelMax + 1 - Abc_ObjReverseLevel(pObj);
}

/**Function*************************************************************

  Synopsis    [Returns the reverse level of the node.]

  Description [The reverse level is the level of the node in reverse
  topological order, starting from the COs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjReverseLevel( Abc_Obj_t * pObj )
{
    Abc_Ntk_t * pNtk = pObj->pNtk;
    assert( pNtk->vLevelsR );
    Vec_IntFillExtra( pNtk->vLevelsR, pObj->Id + 1, 0 );
    return Vec_IntEntry(pNtk->vLevelsR, pObj->Id);
}

/**Function*************************************************************

  Synopsis    [Sets the reverse level of the node.]

  Description [The reverse level is the level of the node in reverse
  topological order, starting from the COs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ObjSetReverseLevel( Abc_Obj_t * pObj, int LevelR )
{
    Abc_Ntk_t * pNtk = pObj->pNtk;
    assert( pNtk->vLevelsR );
    Vec_IntFillExtra( pNtk->vLevelsR, pObj->Id + 1, 0 );
    Vec_IntWriteEntry( pNtk->vLevelsR, pObj->Id, LevelR );
}

/**Function*************************************************************

  Synopsis    [Prepares for the computation of required levels.]

  Description [This procedure should be called before the required times
  are used. It starts internal data structures, which records the level 
  from the COs of the network nodes in reverse topologogical order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkStartReverseLevels( Abc_Ntk_t * pNtk, int nMaxLevelIncrease )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj;
    int i;
    // remember the maximum number of direct levels
    pNtk->LevelMax = Abc_NtkLevel(pNtk) + nMaxLevelIncrease;
    // start the reverse levels
    pNtk->vLevelsR = Vec_IntAlloc( 0 );
    Vec_IntFill( pNtk->vLevelsR, 1 + Abc_NtkObjNumMax(pNtk), 0 );
    // compute levels in reverse topological order
    vNodes = Abc_NtkDfsReverse( pNtk );
    Vec_PtrForEachEntry( vNodes, pObj, i )
        Abc_ObjSetReverseLevel( pObj, Abc_ObjReverseLevelNew(pObj) );
    Vec_PtrFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Cleans the data structures used to compute required levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkStopReverseLevels( Abc_Ntk_t * pNtk )
{
    assert( pNtk->vLevelsR );
    Vec_IntFree( pNtk->vLevelsR );
    pNtk->vLevelsR = NULL;
    pNtk->LevelMax = 0;

}

/**Function*************************************************************

  Synopsis    [Incrementally updates level of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkUpdateLevel( Abc_Obj_t * pObjNew, Vec_Vec_t * vLevels )
{
    Abc_Obj_t * pFanout, * pTemp;
    int LevelOld, Lev, k, m;
    // check if level has changed
    LevelOld = Abc_ObjLevel(pObjNew);
    if ( LevelOld == Abc_ObjLevelNew(pObjNew) )
        return;
    // start the data structure for level update
    // we cannot fail to visit a node when using this structure because the 
    // nodes are stored by their _old_ levels, which are assumed to be correct
    Vec_VecClear( vLevels );
    Vec_VecPush( vLevels, LevelOld, pObjNew );
    pObjNew->fMarkA = 1;
    // recursively update level
    Vec_VecForEachEntryStart( vLevels, pTemp, Lev, k, LevelOld )
    {
        pTemp->fMarkA = 0;
        assert( Abc_ObjLevel(pTemp) == Lev );
        Abc_ObjSetLevel( pTemp, Abc_ObjLevelNew(pTemp) );
        // if the level did not change, no need to check the fanout levels
        if ( Abc_ObjLevel(pTemp) == Lev )
            continue;
        // schedule fanout for level update
        Abc_ObjForEachFanout( pTemp, pFanout, m )
        {
            if ( !Abc_ObjIsCo(pFanout) && !pFanout->fMarkA )
            {
                assert( Abc_ObjLevel(pFanout) >= Lev );
                Vec_VecPush( vLevels, Abc_ObjLevel(pFanout), pFanout );
                pFanout->fMarkA = 1;
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    [Incrementally updates level of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkUpdateReverseLevel( Abc_Obj_t * pObjNew, Vec_Vec_t * vLevels )
{
    Abc_Obj_t * pFanin, * pTemp;
    int LevelOld, LevFanin, Lev, k, m;
    // check if level has changed
    LevelOld = Abc_ObjReverseLevel(pObjNew);
    if ( LevelOld == Abc_ObjReverseLevelNew(pObjNew) )
        return;
    // start the data structure for level update
    // we cannot fail to visit a node when using this structure because the 
    // nodes are stored by their _old_ levels, which are assumed to be correct
    Vec_VecClear( vLevels );
    Vec_VecPush( vLevels, LevelOld, pObjNew );
    pObjNew->fMarkA = 1;
    // recursively update level
    Vec_VecForEachEntryStart( vLevels, pTemp, Lev, k, LevelOld )
    {
        pTemp->fMarkA = 0;
        LevelOld = Abc_ObjReverseLevel(pTemp); 
        assert( LevelOld == Lev );
        Abc_ObjSetReverseLevel( pTemp, Abc_ObjReverseLevelNew(pTemp) );
        // if the level did not change, no need to check the fanout levels
        if ( Abc_ObjReverseLevel(pTemp) == Lev )
            continue;
        // schedule fanins for level update
        Abc_ObjForEachFanin( pTemp, pFanin, m )
        {
            if ( !Abc_ObjIsCi(pFanin) && !pFanin->fMarkA )
            {
                LevFanin = Abc_ObjReverseLevel( pFanin );
                assert( LevFanin >= Lev );
                Vec_VecPush( vLevels, LevFanin, pFanin );
                pFanin->fMarkA = 1;
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    [Replaces the node and incrementally updates levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkUpdate( Abc_Obj_t * pObj, Abc_Obj_t * pObjNew, Vec_Vec_t * vLevels )
{
    // replace the old node by the new node
    pObjNew->Level = pObj->Level;
    Abc_ObjReplace( pObj, pObjNew );
    // update the level of the node
    Abc_NtkUpdateLevel( pObjNew, vLevels );
    Abc_ObjSetReverseLevel( pObjNew, 0 );
    Abc_NtkUpdateReverseLevel( pObjNew, vLevels );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


