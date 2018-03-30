/**CFile****************************************************************

  FileName    [giaUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Various utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaUtil.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "base/main/mainInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
#define NUMBER1  3716960521u
#define NUMBER2  2174103536u

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates a sequence or random numbers.]

  Description []
               
  SideEffects []

  SeeAlso     [http://www.codeproject.com/KB/recipes/SimpleRNG.aspx]

***********************************************************************/
unsigned Gia_ManRandom( int fReset )
{
    static unsigned int m_z = NUMBER1;
    static unsigned int m_w = NUMBER2;
    if ( fReset )
    {
        m_z = NUMBER1;
        m_w = NUMBER2;
    }
    m_z = 36969 * (m_z & 65535) + (m_z >> 16);
    m_w = 18000 * (m_w & 65535) + (m_w >> 16);
    return (m_z << 16) + m_w;
}
word Gia_ManRandomW( int fReset )
{ 
    return ((word)Gia_ManRandom(fReset) << 32) | ((word)Gia_ManRandom(fReset) << 0);
}



/**Function*************************************************************

  Synopsis    [Creates random info for the primary inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManRandomInfo( Vec_Ptr_t * vInfo, int iInputStart, int iWordStart, int iWordStop )
{
    unsigned * pInfo;
    int i, w;
    Vec_PtrForEachEntryStart( unsigned *, vInfo, pInfo, i, iInputStart )
        for ( w = iWordStart; w < iWordStop; w++ )
            pInfo[w] = Gia_ManRandom(0);
}


/**Function*************************************************************

  Synopsis    [Returns the time stamp.]

  Description [The file should be closed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Gia_TimeStamp()
{
    static char Buffer[100];
    char * TimeStamp;
    time_t ltime;
    // get the current time
    time( &ltime );
    TimeStamp = asctime( localtime( &ltime ) );
    TimeStamp[ strlen(TimeStamp) - 1 ] = 0;
    strcpy( Buffer, TimeStamp );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    [Returns the composite name of the file.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Gia_FileNameGenericAppend( char * pBase, char * pSuffix )
{
    static char Buffer[1000];
    char * pDot;
    strcpy( Buffer, pBase );
    if ( (pDot = strrchr( Buffer, '.' )) )
        *pDot = 0;
    strcat( Buffer, pSuffix );
    if ( (pDot = strrchr( Buffer, '\\' )) || (pDot = strrchr( Buffer, '/' )) )
        return pDot+1;
    return Buffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManIncrementTravId( Gia_Man_t * p )                     
{ 
    if ( p->pTravIds == NULL )
    {
        p->nTravIdsAlloc = Gia_ManObjNum(p) + 100; 
        p->pTravIds = ABC_CALLOC( int, p->nTravIdsAlloc ); 
        p->nTravIds = 0;  
    }
    while ( p->nTravIdsAlloc < Gia_ManObjNum(p) )
    {
        p->nTravIdsAlloc *= 2;
        p->pTravIds = ABC_REALLOC( int, p->pTravIds, p->nTravIdsAlloc );
        memset( p->pTravIds + p->nTravIdsAlloc/2, 0, sizeof(int) * p->nTravIdsAlloc/2 );
    }
    p->nTravIds++;                                                    
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCleanMark01( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        pObj->fMark0 = pObj->fMark1 = 0;
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSetMark0( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        pObj->fMark0 = 1;
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCleanMark0( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        pObj->fMark0 = 0;
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCheckMark0( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        assert( pObj->fMark0 == 0 );
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSetMark1( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        pObj->fMark1 = 1;
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCleanMark1( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        pObj->fMark1 = 0;
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCheckMark1( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        assert( pObj->fMark1 == 0 );
}

/**Function*************************************************************

  Synopsis    [Cleans the value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCleanValue( Gia_Man_t * p )  
{
    int i;
    for ( i = 0; i < p->nObjs; i++ )
        p->pObjs[i].Value = 0;
}

/**Function*************************************************************

  Synopsis    [Cleans the value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManFillValue( Gia_Man_t * p )  
{
    int i;
    for ( i = 0; i < p->nObjs; i++ )
        p->pObjs[i].Value = ~0;
}

/**Function*************************************************************

  Synopsis    [Sets the phase of one object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ObjSetPhase( Gia_Man_t * p, Gia_Obj_t * pObj )  
{
    if ( Gia_ObjIsAnd(pObj) )
    {
        int fPhase0 = Gia_ObjPhase(Gia_ObjFanin0(pObj)) ^ Gia_ObjFaninC0(pObj);
        int fPhase1 = Gia_ObjPhase(Gia_ObjFanin1(pObj)) ^ Gia_ObjFaninC1(pObj);
        if ( Gia_ObjIsMux(p, pObj) )
        {
            int fPhase2 = Gia_ObjPhase(Gia_ObjFanin2(p, pObj)) ^ Gia_ObjFaninC2(p, pObj);
            pObj->fPhase = (fPhase2 && fPhase1) || (!fPhase2 && fPhase0);
        }
        else if ( Gia_ObjIsXor(pObj) )
            pObj->fPhase = fPhase0 ^ fPhase1;
        else
            pObj->fPhase = fPhase0 & fPhase1;
    }
    else if ( Gia_ObjIsCo(pObj) )
        pObj->fPhase = (Gia_ObjPhase(Gia_ObjFanin0(pObj)) ^ Gia_ObjFaninC0(pObj));
    else
        pObj->fPhase = 0;
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSetPhase( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        Gia_ObjSetPhase( p, pObj );
}
void Gia_ManSetPhasePattern( Gia_Man_t * p, Vec_Int_t * vCiValues )  
{
    Gia_Obj_t * pObj;
    int i;
    assert( Gia_ManCiNum(p) == Vec_IntSize(vCiValues) );
    Gia_ManForEachObj( p, pObj, i )
        if ( Gia_ObjIsCi(pObj) )
            pObj->fPhase = Vec_IntEntry( vCiValues, Gia_ObjCioId(pObj) );
        else
            Gia_ObjSetPhase( p, pObj );
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSetPhase1( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachCi( p, pObj, i )
        pObj->fPhase = 1;
    Gia_ManForEachObj( p, pObj, i )
        if ( !Gia_ObjIsCi(pObj) )
            Gia_ObjSetPhase( p, pObj );
}

/**Function*************************************************************

  Synopsis    [Sets phases of the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCleanPhase( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        pObj->fPhase = 0;
}

/**Function*************************************************************

  Synopsis    [Returns the number of COs whose value is 1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCheckCoPhase( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, Counter = 0;
    Gia_ManForEachCo( p, pObj, i )
        Counter += pObj->fPhase;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Prepares copies for the model.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCleanLevels( Gia_Man_t * p, int Size )
{
    if ( p->vLevels == NULL )
        p->vLevels = Vec_IntAlloc( Size );
    Vec_IntFill( p->vLevels, Size, 0 );
}
/**Function*************************************************************

  Synopsis    [Prepares copies for the model.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCleanTruth( Gia_Man_t * p )
{
    if ( p->vTruths == NULL )
        p->vTruths = Vec_IntAlloc( Gia_ManObjNum(p) );
    Vec_IntFill( p->vTruths, Gia_ManObjNum(p), -1 );
}

/**Function*************************************************************

  Synopsis    [Assigns levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManLevelNum( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManCleanLevels( p, Gia_ManObjNum(p) );
    p->nLevels = 0;
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( !p->fGiaSimple && Gia_ObjIsBuf(pObj) )
            Gia_ObjSetBufLevel( p, pObj );
        else if ( Gia_ObjIsAnd(pObj) )
            Gia_ObjSetGateLevel( p, pObj );
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ObjSetCoLevel( p, pObj );
        else
            Gia_ObjSetLevel( p, pObj, 0 );
        p->nLevels = Abc_MaxInt( p->nLevels, Gia_ObjLevel(p, pObj) );
    }
    return p->nLevels;
}
float Gia_ManLevelAve( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i, Ave = 0;
    assert( p->vLevels );
    Gia_ManForEachCo( p, pObj, i )
        Ave += Gia_ObjLevel(p, pObj);
    return (float)Ave / Gia_ManCoNum(p);
}

/**Function*************************************************************

  Synopsis    [Assigns levels using CI level information.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManGetCiLevels( Gia_Man_t * p )  
{
    Vec_Int_t * vCiLevels;
    Gia_Obj_t * pObj;
    int i;
    if ( p->vLevels == NULL )
        return NULL;
    vCiLevels = Vec_IntAlloc( Gia_ManCiNum(p) );
    Gia_ManForEachCi( p, pObj, i )
        Vec_IntPush( vCiLevels, Gia_ObjLevel(p, pObj) );
    return vCiLevels;
}
int Gia_ManSetLevels( Gia_Man_t * p, Vec_Int_t * vCiLevels )  
{
    Gia_Obj_t * pObj;
    int i;
    if ( vCiLevels == NULL )
        return Gia_ManLevelNum( p );
    assert( Vec_IntSize(vCiLevels) == Gia_ManCiNum(p) );
    Gia_ManCleanLevels( p, Gia_ManObjNum(p) );
    p->nLevels = 0;
    Gia_ManForEachCi( p, pObj, i )
    {
        Gia_ObjSetLevel( p, pObj, Vec_IntEntry(vCiLevels, i) );
        p->nLevels = Abc_MaxInt( p->nLevels, Gia_ObjLevel(p, pObj) );
    }
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            Gia_ObjSetGateLevel( p, pObj );
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ObjSetCoLevel( p, pObj );
        else continue;
        p->nLevels = Abc_MaxInt( p->nLevels, Gia_ObjLevel(p, pObj) );
    }
    return p->nLevels;
}

/**Function*************************************************************

  Synopsis    [Compute reverse levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManReverseLevel( Gia_Man_t * p )  
{
    Vec_Int_t * vLevelRev;
    Gia_Obj_t * pObj;
    int i;
    vLevelRev = Vec_IntStart( Gia_ManObjNum(p) );
    Gia_ManForEachAndReverse( p, pObj, i )
    {
        int LevelR = Vec_IntEntry( vLevelRev, i );
        if ( Gia_ObjIsMux(p, pObj) )
        {
            Vec_IntUpdateEntry( vLevelRev, Gia_ObjFaninId0(pObj, i), LevelR + 2 );
            Vec_IntUpdateEntry( vLevelRev, Gia_ObjFaninId1(pObj, i), LevelR + 2 );
            Vec_IntUpdateEntry( vLevelRev, Gia_ObjFaninId2(p, i), LevelR + 2 );
        }
        else if ( Gia_ObjIsXor(pObj) )
        {
            Vec_IntUpdateEntry( vLevelRev, Gia_ObjFaninId0(pObj, i), LevelR + 2 );
            Vec_IntUpdateEntry( vLevelRev, Gia_ObjFaninId1(pObj, i), LevelR + 2 );
        }
        else if ( Gia_ObjIsBuf(pObj) )
        {
            Vec_IntUpdateEntry( vLevelRev, Gia_ObjFaninId0(pObj, i), LevelR );
        }
        else
        {
            Vec_IntUpdateEntry( vLevelRev, Gia_ObjFaninId0(pObj, i), LevelR + 1 );
            Vec_IntUpdateEntry( vLevelRev, Gia_ObjFaninId1(pObj, i), LevelR + 1 );
        }
    }
    return vLevelRev;
}

/**Function*************************************************************

  Synopsis    [Compute required levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManRequiredLevel( Gia_Man_t * p )  
{
    Vec_Int_t * vRequired;
    Gia_Obj_t * pObj;
    int i, LevelMax = 0;
    vRequired = Gia_ManReverseLevel( p );
    Gia_ManForEachCi( p, pObj, i )
        LevelMax = Abc_MaxInt( LevelMax, Vec_IntEntry(vRequired, Gia_ObjId(p, pObj)) );
    Gia_ManForEachObj( p, pObj, i )
        Vec_IntWriteEntry( vRequired, i, LevelMax - Vec_IntEntry(vRequired, i) );
    return vRequired;
}

/**Function*************************************************************

  Synopsis    [Compute slacks measured using the number of AIG levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManComputeSlacks( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, nLevels = Gia_ManLevelNum( p );
    Vec_Int_t * vLevelR = Gia_ManReverseLevel( p );
    Vec_Int_t * vSlacks = Vec_IntAlloc( Gia_ManObjNum(p) );
    Gia_ManForEachObj( p, pObj, i )
        Vec_IntPush( vSlacks, nLevels - Gia_ObjLevelId(p, i) - Vec_IntEntry(vLevelR, i) );
    assert( Vec_IntSize(vSlacks) == Gia_ManObjNum(p) );
    Vec_IntFree( vLevelR );
    return vSlacks;
}

/**Function*************************************************************

  Synopsis    [Assigns levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCreateValueRefs( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
    {
        pObj->Value = 0;
        if ( Gia_ObjIsAnd(pObj) )
        {
            Gia_ObjFanin0(pObj)->Value++;
            Gia_ObjFanin1(pObj)->Value++;
        }
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ObjFanin0(pObj)->Value++;
    }
}

/**Function*************************************************************

  Synopsis    [Assigns references.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCreateRefs( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int i;
    assert( p->pRefs == NULL );
    p->pRefs = ABC_CALLOC( int, Gia_ManObjNum(p) );
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
        {
            Gia_ObjRefFanin0Inc( p, pObj );
            Gia_ObjRefFanin1Inc( p, pObj );
            if ( Gia_ObjIsMuxId(p, i) )
                Gia_ObjRefFanin2Inc( p, pObj );
        }
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ObjRefFanin0Inc( p, pObj );
    }
}

/**Function*************************************************************

  Synopsis    [Assigns references.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Gia_ManCreateMuxRefs( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj, * pCtrl, * pFan0, * pFan1;
    int i, * pMuxRefs;
    pMuxRefs = ABC_CALLOC( int, Gia_ManObjNum(p) );
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Gia_ObjRecognizeExor( pObj, &pFan0, &pFan1 ) )
            continue;
        if ( !Gia_ObjIsMuxType(pObj) )
            continue;
        pCtrl = Gia_ObjRecognizeMux( pObj, &pFan0, &pFan1 );
        pMuxRefs[ Gia_ObjId(p, Gia_Regular(pCtrl)) ]++;
    }
    return pMuxRefs;
}

/**Function*************************************************************

  Synopsis    [Computes the maximum frontier size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDfsForCrossCut_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vNodes )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
    {
        Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
        return;
    }
    if ( Gia_ObjIsCo(pObj) )
    {
        Gia_ObjFanin0(pObj)->Value++;
        Gia_ManDfsForCrossCut_rec( p, Gia_ObjFanin0(pObj), vNodes );
        Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ObjFanin0(pObj)->Value++;
    Gia_ObjFanin1(pObj)->Value++;
    Gia_ManDfsForCrossCut_rec( p, Gia_ObjFanin0(pObj), vNodes );
    Gia_ManDfsForCrossCut_rec( p, Gia_ObjFanin1(pObj), vNodes );
    Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
}
Vec_Int_t * Gia_ManDfsForCrossCut( Gia_Man_t * p, int fReverse )
{
    Vec_Int_t * vNodes;
    Gia_Obj_t * pObj;
    int i;
    Gia_ManCleanValue( p );
    vNodes = Vec_IntAlloc( Gia_ManObjNum(p) );
    Gia_ManIncrementTravId( p );
    if ( fReverse )
    {
        Gia_ManForEachCoReverse( p, pObj, i )
            if ( !Gia_ObjIsConst0(Gia_ObjFanin0(pObj)) )
                Gia_ManDfsForCrossCut_rec( p, pObj, vNodes );
    }
    else
    {
        Gia_ManForEachCo( p, pObj, i )
            if ( !Gia_ObjIsConst0(Gia_ObjFanin0(pObj)) )
                Gia_ManDfsForCrossCut_rec( p, pObj, vNodes );
    }
    return vNodes;
}
int Gia_ManCrossCut( Gia_Man_t * p, int fReverse )
{
    Vec_Int_t * vNodes;
    Gia_Obj_t * pObj;
    int i, nCutCur = 0, nCutMax = 0;
    vNodes = Gia_ManDfsForCrossCut( p, fReverse );
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
    {
        if ( pObj->Value )
            nCutCur++;
        if ( nCutMax < nCutCur )
            nCutMax = nCutCur;
        if ( Gia_ObjIsAnd(pObj) )
        {
            if ( --Gia_ObjFanin0(pObj)->Value == 0 )
                nCutCur--;
            if ( --Gia_ObjFanin1(pObj)->Value == 0 )
                nCutCur--;
        }
        else if ( Gia_ObjIsCo(pObj) )
        {
            if ( --Gia_ObjFanin0(pObj)->Value == 0 )
                nCutCur--;
        }
    }
    Vec_IntFree( vNodes );
    Gia_ManForEachObj( p, pObj, i )
        assert( pObj->Value == 0 );
    return nCutMax;
}


/**Function*************************************************************

  Synopsis    [Collects PO Ids into one array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManCollectPoIds( Gia_Man_t * p )
{
    Vec_Int_t * vStart;
    int Entry, i;
    vStart = Vec_IntAlloc( Gia_ManPoNum(p) );
    Vec_IntForEachEntryStop( p->vCos, Entry, i, Gia_ManPoNum(p) )
        Vec_IntPush( vStart, Entry );
    return vStart;
}


/**Function*************************************************************

  Synopsis    [Returns 1 if the node is the root of MUX or EXOR/NEXOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ObjIsMuxType( Gia_Obj_t * pNode )
{
    Gia_Obj_t * pNode0, * pNode1;
    // check that the node is regular
    assert( !Gia_IsComplement(pNode) );
    // if the node is not AND, this is not MUX
    if ( !Gia_ObjIsAnd(pNode) || Gia_ObjIsBuf(pNode) )
        return 0;
    // if the children are not complemented, this is not MUX
    if ( !Gia_ObjFaninC0(pNode) || !Gia_ObjFaninC1(pNode) )
        return 0;
    // get children
    pNode0 = Gia_ObjFanin0(pNode);
    pNode1 = Gia_ObjFanin1(pNode);
    // if the children are not ANDs, this is not MUX
    if ( !Gia_ObjIsAnd(pNode0) || !Gia_ObjIsAnd(pNode1) )
        return 0;
    // otherwise the node is MUX iff it has a pair of equal grandchildren
    return (Gia_ObjFanin0(pNode0) == Gia_ObjFanin0(pNode1) && (Gia_ObjFaninC0(pNode0) ^ Gia_ObjFaninC0(pNode1))) || 
           (Gia_ObjFanin0(pNode0) == Gia_ObjFanin1(pNode1) && (Gia_ObjFaninC0(pNode0) ^ Gia_ObjFaninC1(pNode1))) ||
           (Gia_ObjFanin1(pNode0) == Gia_ObjFanin0(pNode1) && (Gia_ObjFaninC1(pNode0) ^ Gia_ObjFaninC0(pNode1))) ||
           (Gia_ObjFanin1(pNode0) == Gia_ObjFanin1(pNode1) && (Gia_ObjFaninC1(pNode0) ^ Gia_ObjFaninC1(pNode1)));
}


/**Function*************************************************************

  Synopsis    [Recognizes what nodes are inputs of the EXOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ObjRecognizeExor( Gia_Obj_t * pObj, Gia_Obj_t ** ppFan0, Gia_Obj_t ** ppFan1 )
{
    Gia_Obj_t * p0, * p1;
    assert( !Gia_IsComplement(pObj) );
    if ( !Gia_ObjIsAnd(pObj) || Gia_ObjIsBuf(pObj) )
        return 0;
    assert( Gia_ObjIsAnd(pObj) );
    p0 = Gia_ObjChild0(pObj);
    p1 = Gia_ObjChild1(pObj);
    if ( !Gia_IsComplement(p0) || !Gia_IsComplement(p1) )
        return 0;
    p0 = Gia_Regular(p0);
    p1 = Gia_Regular(p1);
    if ( !Gia_ObjIsAnd(p0) || !Gia_ObjIsAnd(p1) )
        return 0;
    if ( Gia_ObjFanin0(p0) != Gia_ObjFanin0(p1) || Gia_ObjFanin1(p0) != Gia_ObjFanin1(p1) )
        return 0;
    if ( Gia_ObjFaninC0(p0) == Gia_ObjFaninC0(p1) || Gia_ObjFaninC1(p0) == Gia_ObjFaninC1(p1) )
        return 0;
    if ( ppFan0 ) *ppFan0 = Gia_ObjChild0(p0);
    if ( ppFan1 ) *ppFan1 = Gia_ObjChild1(p0);
    return 1;
}

/**Function*************************************************************

  Synopsis    [Recognizes what nodes are control and data inputs of a MUX.]

  Description [If the node is a MUX, returns the control variable C.
  Assigns nodes T and E to be the then and else variables of the MUX. 
  Node C is never complemented. Nodes T and E can be complemented.
  This function also recognizes EXOR/NEXOR gates as MUXes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Obj_t * Gia_ObjRecognizeMux( Gia_Obj_t * pNode, Gia_Obj_t ** ppNodeT, Gia_Obj_t ** ppNodeE )
{
    Gia_Obj_t * pNode0, * pNode1;
    assert( !Gia_IsComplement(pNode) );
    assert( Gia_ObjIsMuxType(pNode) );
    // get children
    pNode0 = Gia_ObjFanin0(pNode);
    pNode1 = Gia_ObjFanin1(pNode);

    // find the control variable
    if ( Gia_ObjFanin1(pNode0) == Gia_ObjFanin1(pNode1) && (Gia_ObjFaninC1(pNode0) ^ Gia_ObjFaninC1(pNode1)) )
    {
//        if ( FrGia_IsComplement(pNode1->p2) )
        if ( Gia_ObjFaninC1(pNode0) )
        { // pNode2->p2 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild0(pNode1));//pNode2->p1);
            *ppNodeE = Gia_Not(Gia_ObjChild0(pNode0));//pNode1->p1);
            return Gia_ObjChild1(pNode1);//pNode2->p2;
        }
        else
        { // pNode1->p2 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild0(pNode0));//pNode1->p1);
            *ppNodeE = Gia_Not(Gia_ObjChild0(pNode1));//pNode2->p1);
            return Gia_ObjChild1(pNode0);//pNode1->p2;
        }
    }
    else if ( Gia_ObjFanin0(pNode0) == Gia_ObjFanin0(pNode1) && (Gia_ObjFaninC0(pNode0) ^ Gia_ObjFaninC0(pNode1)) )
    {
//        if ( FrGia_IsComplement(pNode1->p1) )
        if ( Gia_ObjFaninC0(pNode0) )
        { // pNode2->p1 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild1(pNode1));//pNode2->p2);
            *ppNodeE = Gia_Not(Gia_ObjChild1(pNode0));//pNode1->p2);
            return Gia_ObjChild0(pNode1);//pNode2->p1;
        }
        else
        { // pNode1->p1 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild1(pNode0));//pNode1->p2);
            *ppNodeE = Gia_Not(Gia_ObjChild1(pNode1));//pNode2->p2);
            return Gia_ObjChild0(pNode0);//pNode1->p1;
        }
    }
    else if ( Gia_ObjFanin0(pNode0) == Gia_ObjFanin1(pNode1) && (Gia_ObjFaninC0(pNode0) ^ Gia_ObjFaninC1(pNode1)) )
    {
//        if ( FrGia_IsComplement(pNode1->p1) )
        if ( Gia_ObjFaninC0(pNode0) )
        { // pNode2->p2 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild0(pNode1));//pNode2->p1);
            *ppNodeE = Gia_Not(Gia_ObjChild1(pNode0));//pNode1->p2);
            return Gia_ObjChild1(pNode1);//pNode2->p2;
        }
        else
        { // pNode1->p1 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild1(pNode0));//pNode1->p2);
            *ppNodeE = Gia_Not(Gia_ObjChild0(pNode1));//pNode2->p1);
            return Gia_ObjChild0(pNode0);//pNode1->p1;
        }
    }
    else if ( Gia_ObjFanin1(pNode0) == Gia_ObjFanin0(pNode1) && (Gia_ObjFaninC1(pNode0) ^ Gia_ObjFaninC0(pNode1)) )
    {
//        if ( FrGia_IsComplement(pNode1->p2) )
        if ( Gia_ObjFaninC1(pNode0) )
        { // pNode2->p1 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild1(pNode1));//pNode2->p2);
            *ppNodeE = Gia_Not(Gia_ObjChild0(pNode0));//pNode1->p1);
            return Gia_ObjChild0(pNode1);//pNode2->p1;
        }
        else
        { // pNode1->p2 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild0(pNode0));//pNode1->p1);
            *ppNodeE = Gia_Not(Gia_ObjChild1(pNode1));//pNode2->p2);
            return Gia_ObjChild1(pNode0);//pNode1->p2;
        }
    }
    assert( 0 ); // this is not MUX
    return NULL;
}
int Gia_ObjRecognizeMuxLits( Gia_Man_t * p, Gia_Obj_t * pNode, int * iLitT, int * iLitE )
{
    Gia_Obj_t * pNodeT, * pNodeE;
    Gia_Obj_t * pCtrl = Gia_ObjRecognizeMux( pNode, &pNodeT, &pNodeE );
    assert( pCtrl != NULL );
    *iLitT = Gia_Obj2Lit( p, pNodeT );
    *iLitE = Gia_Obj2Lit( p, pNodeE );
    return Gia_Obj2Lit( p, pCtrl );
}


/**Function*************************************************************

  Synopsis    [Dereferences the node's MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_NodeDeref_rec( Gia_Man_t * p, Gia_Obj_t * pNode )
{
    Gia_Obj_t * pFanin;
    int Counter = 0;
    if ( Gia_ObjIsCi(pNode) )
        return 0;
    assert( Gia_ObjIsAnd(pNode) );
    pFanin = Gia_ObjFanin0(pNode);
    assert( Gia_ObjRefNum(p, pFanin) > 0 );
    if ( Gia_ObjRefDec(p, pFanin) == 0 )
        Counter += Gia_NodeDeref_rec( p, pFanin );
    pFanin = Gia_ObjFanin1(pNode);
    assert( Gia_ObjRefNum(p, pFanin) > 0 );
    if ( Gia_ObjRefDec(p, pFanin) == 0 )
        Counter += Gia_NodeDeref_rec( p, pFanin );
    return Counter + 1;
}

/**Function*************************************************************

  Synopsis    [References the node's MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_NodeRef_rec( Gia_Man_t * p, Gia_Obj_t * pNode )
{
    Gia_Obj_t * pFanin;
    int Counter = 0;
    if ( Gia_ObjIsCi(pNode) )
        return 0;
    assert( Gia_ObjIsAnd(pNode) );
    pFanin = Gia_ObjFanin0(pNode);
    if ( Gia_ObjRefInc(p, pFanin) == 0 )
        Counter += Gia_NodeRef_rec( p, pFanin );
    pFanin = Gia_ObjFanin1(pNode);
    if ( Gia_ObjRefInc(p, pFanin) == 0 )
        Counter += Gia_NodeRef_rec( p, pFanin );
    return Counter + 1;
}


/**Function*************************************************************

  Synopsis    [References the node's MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManPoMffcSize( Gia_Man_t * p )
{
    Gia_ManCreateRefs( p );
    return Gia_NodeDeref_rec( p, Gia_ObjFanin0(Gia_ManPo(p, 0)) );
}

/**Function*************************************************************

  Synopsis    [Returns the number of internal nodes in the MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_NodeMffcSize( Gia_Man_t * p, Gia_Obj_t * pNode )
{
    int ConeSize1, ConeSize2;
    assert( !Gia_IsComplement(pNode) );
    assert( Gia_ObjIsCand(pNode) );
    ConeSize1 = Gia_NodeDeref_rec( p, pNode );
    ConeSize2 = Gia_NodeRef_rec( p, pNode );
    assert( ConeSize1 == ConeSize2 );
    assert( ConeSize1 >= 0 );
    return ConeSize1;
}

/**Function*************************************************************

  Synopsis    [Returns the number of internal nodes in the MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_NodeCollect_rec( Gia_Man_t * p, Gia_Obj_t * pNode, Vec_Int_t * vSupp )
{
    if ( Gia_ObjIsTravIdCurrent(p, pNode) )
        return;
    Gia_ObjSetTravIdCurrent(p, pNode);
    if ( Gia_ObjRefNum(p, pNode) || Gia_ObjIsCi(pNode) )
    {
        Vec_IntPush( vSupp, Gia_ObjId(p, pNode) );
        return;
    }
    assert( Gia_ObjIsAnd(pNode) );
    Gia_NodeCollect_rec( p, Gia_ObjFanin0(pNode), vSupp );
    Gia_NodeCollect_rec( p, Gia_ObjFanin1(pNode), vSupp );
}
int Gia_NodeMffcSizeSupp( Gia_Man_t * p, Gia_Obj_t * pNode, Vec_Int_t * vSupp )
{
    int ConeSize1, ConeSize2;
    assert( !Gia_IsComplement(pNode) );
    assert( Gia_ObjIsAnd(pNode) );
    Vec_IntClear( vSupp );
    Gia_ManIncrementTravId( p );
    ConeSize1 = Gia_NodeDeref_rec( p, pNode );
    Gia_NodeCollect_rec( p, Gia_ObjFanin0(pNode), vSupp );
    Gia_NodeCollect_rec( p, Gia_ObjFanin1(pNode), vSupp );
    ConeSize2 = Gia_NodeRef_rec( p, pNode );
    assert( ConeSize1 == ConeSize2 );
    assert( ConeSize1 >= 0 );
    return ConeSize1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if AIG has dangling nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManHasDangling( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, Counter = 0;
    Gia_ManForEachObj( p, pObj, i )
    {
        pObj->fMark0 = 0;
        if ( Gia_ObjIsCo(pObj) )
            Gia_ObjFanin0(pObj)->fMark0 = 1;
        else if ( Gia_ObjIsMux(p, pObj) )
        {
            Gia_ObjFanin0(pObj)->fMark0 = 1;
            Gia_ObjFanin1(pObj)->fMark0 = 1;
            Gia_ObjFanin2(p, pObj)->fMark0 = 1;
        }
        else if ( Gia_ObjIsAnd(pObj) )
        {
            Gia_ObjFanin0(pObj)->fMark0 = 1;
            Gia_ObjFanin1(pObj)->fMark0 = 1;
        }
    }
    Gia_ManForEachAnd( p, pObj, i )
        Counter += !pObj->fMark0;
    Gia_ManCleanMark0( p );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if AIG has dangling nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManMarkDangling( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, Counter = 0;
    Gia_ManForEachObj( p, pObj, i )
    {
        pObj->fMark0 = 0;
        if ( Gia_ObjIsAnd(pObj) )
        {
            Gia_ObjFanin0(pObj)->fMark0 = 1;
            Gia_ObjFanin1(pObj)->fMark0 = 1;
        }
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ObjFanin0(pObj)->fMark0 = 1;
    }
    Gia_ManForEachAnd( p, pObj, i )
        Counter += !pObj->fMark0;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if AIG has dangling nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManGetDangling( Gia_Man_t * p )
{
    Vec_Int_t * vDangles;
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
    {
        pObj->fMark0 = 0;
        if ( Gia_ObjIsAnd(pObj) )
        {
            Gia_ObjFanin0(pObj)->fMark0 = 1;
            Gia_ObjFanin1(pObj)->fMark0 = 1;
        }
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ObjFanin0(pObj)->fMark0 = 1;
    }
    vDangles = Vec_IntAlloc( 100 );
    Gia_ManForEachAnd( p, pObj, i )
        if ( !pObj->fMark0 )
            Vec_IntPush( vDangles, i );
    Gia_ManCleanMark0( p );
    return vDangles;
}
/**Function*************************************************************

  Synopsis    [Verbose printing of the AIG node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ObjPrint( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( pObj == NULL )
    {
        printf( "Object is NULL." );
        return;
    }
    if ( Gia_IsComplement(pObj) )
    {
        printf( "Compl " );
        pObj = Gia_Not(pObj);
    }
    assert( !Gia_IsComplement(pObj) );
    printf( "Obj %4d : ", Gia_ObjId(p, pObj) );
    if ( Gia_ObjIsConst0(pObj) )
        printf( "constant 0" );
    else if ( Gia_ObjIsPi(p, pObj) )
        printf( "PI" );
    else if ( Gia_ObjIsPo(p, pObj) )
        printf( "PO( %4d%s )", Gia_ObjFaninId0p(p, pObj), (Gia_ObjFaninC0(pObj)? "\'" : " ") );
    else if ( Gia_ObjIsCi(pObj) )
        printf( "RO( %4d%s )", Gia_ObjFaninId0p(p, Gia_ObjRoToRi(p, pObj)), (Gia_ObjFaninC0(Gia_ObjRoToRi(p, pObj))? "\'" : " ") );
    else if ( Gia_ObjIsCo(pObj) )
        printf( "RI( %4d%s )", Gia_ObjFaninId0p(p, pObj), (Gia_ObjFaninC0(pObj)? "\'" : " ") );
//    else if ( Gia_ObjIsBuf(pObj) )
//        printf( "BUF( %d%s )", Gia_ObjFaninId0p(p, pObj), (Gia_ObjFaninC0(pObj)? "\'" : " ") );
    else if ( Gia_ObjIsXor(pObj) )
        printf( "XOR( %4d%s, %4d%s )", 
            Gia_ObjFaninId0p(p, pObj), (Gia_ObjFaninC0(pObj)? "\'" : " "), 
            Gia_ObjFaninId1p(p, pObj), (Gia_ObjFaninC1(pObj)? "\'" : " ") );
    else if ( Gia_ObjIsMuxId(p, Gia_ObjId(p, pObj)) )
        printf( "MUX( %4d%s, %4d%s, %4d%s )", 
            Gia_ObjFaninId2p(p, pObj), (Gia_ObjFaninC2(p, pObj)? "\'" : " "), 
            Gia_ObjFaninId1p(p, pObj), (Gia_ObjFaninC1(pObj)? "\'" : " "), 
            Gia_ObjFaninId0p(p, pObj), (Gia_ObjFaninC0(pObj)? "\'" : " ") );
    else
        printf( "AND( %4d%s, %4d%s )", 
            Gia_ObjFaninId0p(p, pObj), (Gia_ObjFaninC0(pObj)? "\'" : " "), 
            Gia_ObjFaninId1p(p, pObj), (Gia_ObjFaninC1(pObj)? "\'" : " ") );
    if ( p->pRefs )
        printf( " (refs = %3d)", Gia_ObjRefNum(p, pObj) );
    if ( pObj->fMark0 )
        printf( " mark0" );
    if ( pObj->fMark1 )
        printf( " mark1" );
    if ( Gia_ManHasMapping(p) && Gia_ObjIsLut(p, Gia_ObjId(p, pObj)) )
    {
        int i, iFan;
        printf( " Cut = { " );
        Gia_LutForEachFanin( p, Gia_ObjId(p, pObj), iFan, i )
            printf( "%d ", iFan );
        printf( "}" );
    }
    if ( Gia_ManHasMapping2(p) && Gia_ObjIsLut2(p, Gia_ObjId(p, pObj)) )
    {
        int i, iFan;
        printf( " Cut = { " );
        Gia_LutForEachFanin2( p, Gia_ObjId(p, pObj), iFan, i )
            printf( "%d ", iFan );
        printf( "}" );
    }
    printf( "\n" );
/*
    if ( p->pRefs )
    {
        Gia_Obj_t * pFanout;
        int i;
        int iFan = -1; // Suppress "might be used uninitialized"
        printf( "\nFanouts:\n" );
        Gia_ObjForEachFanout( p, pObj, pFanout, iFan, i )
        {
            printf( "    " );
            printf( "Node %4d : ", Gia_ObjId(pFanout) );
            if ( Gia_ObjIsPo(pFanout) )
                printf( "PO( %4d%s )", Gia_ObjFanin0(pFanout)->Id, (Gia_ObjFaninC0(pFanout)? "\'" : " ") );
            else if ( Gia_ObjIsBuf(pFanout) )
                printf( "BUF( %d%s )", Gia_ObjFanin0(pFanout)->Id, (Gia_ObjFaninC0(pFanout)? "\'" : " ") );
            else
                printf( "AND( %4d%s, %4d%s )", 
                    Gia_ObjFanin0(pFanout)->Id, (Gia_ObjFaninC0(pFanout)? "\'" : " "), 
                    Gia_ObjFanin1(pFanout)->Id, (Gia_ObjFaninC1(pFanout)? "\'" : " ") );
            printf( "\n" );
        }
        return;
    }
*/
}
void Gia_ManPrint( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    printf( "GIA manager has %d ANDs, %d XORs, %d MUXes.\n", 
        Gia_ManAndNum(p) - Gia_ManXorNum(p) - Gia_ManMuxNum(p), Gia_ManXorNum(p), Gia_ManMuxNum(p) ); 
    Gia_ManForEachObj( p, pObj, i )
        Gia_ObjPrint( p, pObj );
}
void Gia_ManPrintCo_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( Gia_ObjIsAnd(pObj) )
    {
        Gia_ManPrintCo_rec( p, Gia_ObjFanin0(pObj) );
        Gia_ManPrintCo_rec( p, Gia_ObjFanin1(pObj) );
        if ( Gia_ObjIsMux(p, pObj) )
            Gia_ManPrintCo_rec( p, Gia_ObjFanin2(p, pObj) );
    }
    Gia_ObjPrint( p, pObj );
}
void Gia_ManPrintCo( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    assert( Gia_ObjIsCo(pObj) );
    printf( "TFI cone of CO number %d:\n", Gia_ObjCioId(pObj) );
    Gia_ManPrintCo_rec( p, Gia_ObjFanin0(pObj) );
    Gia_ObjPrint( p, pObj );
}

void Gia_ManPrintCollect_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vNodes )
{
    if ( Vec_IntFind(vNodes, Gia_ObjId(p, pObj)) >= 0 )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManPrintCollect_rec( p, Gia_ObjFanin0(pObj), vNodes );
    Gia_ManPrintCollect_rec( p, Gia_ObjFanin1(pObj), vNodes );
    if ( Gia_ObjIsMux(p, pObj) )
        Gia_ManPrintCollect_rec( p, Gia_ObjFanin2(p, pObj), vNodes );
    Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
}
void Gia_ManPrintCone( Gia_Man_t * p, Gia_Obj_t * pObj, int * pLeaves, int nLeaves, Vec_Int_t * vNodes )
{
    int i;
    Vec_IntClear( vNodes );
    for ( i = 0; i < nLeaves; i++ )
        Vec_IntPush( vNodes, pLeaves[i] );
    Gia_ManPrintCollect_rec( p, pObj, vNodes );
    printf( "GIA logic cone for node %d:\n", Gia_ObjId(p, pObj) );
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
        Gia_ObjPrint( p, pObj );
}
void Gia_ManPrintConeMulti( Gia_Man_t * p, Vec_Int_t * vObjs, Vec_Int_t * vLeaves, Vec_Int_t * vNodes )
{
    Gia_Obj_t * pObj;
    int i;
    Vec_IntClear( vNodes );
    Vec_IntAppend( vNodes, vLeaves );
    Gia_ManForEachObjVec( vObjs, p, pObj, i )
        Gia_ManPrintCollect_rec( p, pObj, vNodes );
    printf( "GIA logic cone for %d nodes:\n", Vec_IntSize(vObjs) );
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
        Gia_ObjPrint( p, pObj );
}

void Gia_ManPrintCollect2_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vNodes )
{
    if ( Vec_IntFind(vNodes, Gia_ObjId(p, pObj)) >= 0 )
        return;
    if ( Gia_ObjIsCo(pObj) || Gia_ObjIsAnd(pObj) )
        Gia_ManPrintCollect2_rec( p, Gia_ObjFanin0(pObj), vNodes );
    if ( Gia_ObjIsAnd(pObj) )
        Gia_ManPrintCollect2_rec( p, Gia_ObjFanin1(pObj), vNodes );
    if ( Gia_ObjIsMux(p, pObj) )
        Gia_ManPrintCollect2_rec( p, Gia_ObjFanin2(p, pObj), vNodes );
    Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
}
void Gia_ManPrintCone2( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    Vec_Int_t * vNodes;
    int i;
    vNodes = Vec_IntAlloc( 100 );
    Gia_ManPrintCollect2_rec( p, pObj, vNodes );
    printf( "GIA logic cone for node %d:\n", Gia_ObjId(p, pObj) );
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
        Gia_ObjPrint( p, pObj );
    Vec_IntFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Complements the constraint outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManInvertConstraints( Gia_Man_t * pAig )
{
    Gia_Obj_t * pObj;
    int i;
    if ( Gia_ManConstrNum(pAig) == 0 )
        return;
    Gia_ManForEachPo( pAig, pObj, i )
        if ( i >= Gia_ManPoNum(pAig) - Gia_ManConstrNum(pAig) )
            Gia_ObjFlipFaninC0( pObj );
}
void Gia_ManInvertPos( Gia_Man_t * pAig )
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachPo( pAig, pObj, i )
        Gia_ObjFlipFaninC0( pObj );
}

/**Function*************************************************************

  Synopsis    [Testing the speedup due to grouping POs into batches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCollectObjs_rec( Gia_Man_t * p, int iObjId, Vec_Int_t * vObjs, int Limit )
{
    Gia_Obj_t * pObj;
    if ( Vec_IntSize(vObjs) == Limit )
        return;
    if ( Gia_ObjIsTravIdCurrentId(p, iObjId) )
        return;
    Gia_ObjSetTravIdCurrentId(p, iObjId);
    pObj = Gia_ManObj( p, iObjId );
    if ( Gia_ObjIsAnd(pObj) )
    {
        Gia_ManCollectObjs_rec( p, Gia_ObjFaninId0p(p, pObj), vObjs, Limit );
        if ( Vec_IntSize(vObjs) == Limit )
            return;
        Gia_ManCollectObjs_rec( p, Gia_ObjFaninId1p(p, pObj), vObjs, Limit );
        if ( Vec_IntSize(vObjs) == Limit )
            return;
    }
    Vec_IntPush( vObjs, iObjId );
}
unsigned * Gia_ManComputePoTruthTables( Gia_Man_t * p, int nBytesMax )
{
    int nVars = Gia_ManPiNum(p);
    int nTruthWords = Abc_TruthWordNum( nVars );
    int nTruths = nBytesMax / (sizeof(unsigned) * nTruthWords);
    int nTotalNodes = 0, nRounds = 0;
    Vec_Int_t * vObjs;
    Gia_Obj_t * pObj;
    abctime clk = Abc_Clock();
    int i;
    printf( "Var = %d. Words = %d. Truths = %d.\n", nVars, nTruthWords, nTruths );
    vObjs = Vec_IntAlloc( nTruths );
    Gia_ManIncrementTravId( p );
    Gia_ManForEachPo( p, pObj, i )
    {
        Gia_ManCollectObjs_rec( p, Gia_ObjFaninId0p(p, pObj), vObjs, nTruths );
        if ( Vec_IntSize(vObjs) == nTruths )
        {
            nRounds++;
//            printf( "%d ", i );
            nTotalNodes += Vec_IntSize( vObjs );
            Vec_IntClear( vObjs );
            Gia_ManIncrementTravId( p );
        }
    }
//    printf( "\n" );
    nTotalNodes += Vec_IntSize( vObjs );
    Vec_IntFree( vObjs );

    printf( "Rounds = %d. Objects = %d. Total = %d.   ", nRounds, Gia_ManObjNum(p), nTotalNodes );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

    return NULL;
}


/**Function*************************************************************

  Synopsis    [Returns 1 if the manager are structural identical.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCompare( Gia_Man_t * p1, Gia_Man_t * p2 )
{
    Gia_Obj_t * pObj1, * pObj2;
    int i;
    if ( Gia_ManObjNum(p1) != Gia_ManObjNum(p2) )
    {
        printf( "AIGs have different number of objects.\n" );
        return 0;
    }
    Gia_ManCleanValue( p1 );
    Gia_ManCleanValue( p2 );
    Gia_ManForEachObj( p1, pObj1, i )
    {
        pObj2 = Gia_ManObj( p2, i );
        if ( memcmp( pObj1, pObj2, sizeof(Gia_Obj_t) ) )
        {
            printf( "Objects %d are different.\n", i );
            return 0;
        }
        if ( p1->pReprs && p2->pReprs )
        {
            if ( memcmp( &p1->pReprs[i], &p2->pReprs[i], sizeof(Gia_Rpr_t) ) )
            {
                printf( "Representatives of objects %d are different.\n", i );
                return 0;
            }
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Marks nodes that appear as faninis of other nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManMarkFanoutDrivers( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
    {
        pObj->fMark0 = 0;
        if ( Gia_ObjIsAnd(pObj) )
        {
            Gia_ObjFanin0(pObj)->fMark0 = 1;
            Gia_ObjFanin1(pObj)->fMark0 = 1;
        }
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ObjFanin0(pObj)->fMark0 = 1;
    }
}


/**Function*************************************************************

  Synopsis    [Swaps PO number 0 with PO number i.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSwapPos( Gia_Man_t * p, int i )
{
    int Lit0, LitI;
    assert( i >= 0 && i < Gia_ManPoNum(p) );
    if ( i == 0 )
        return;
    Lit0 = Gia_ObjFaninLit0p( p, Gia_ManPo(p, 0) );
    LitI = Gia_ObjFaninLit0p( p, Gia_ManPo(p, i) );
    Gia_ManPatchCoDriver( p, 0, LitI );
    Gia_ManPatchCoDriver( p, i, Lit0 );
}

/**Function*************************************************************

  Synopsis    [Save/load value from file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManSaveValue( Gia_Man_t * p )
{
    Vec_Int_t * vValues;
    Gia_Obj_t * pObj;
    int i;
    vValues = Vec_IntAlloc( Gia_ManObjNum(p) );
    Gia_ManForEachObj( p, pObj, i )
        Vec_IntPush( vValues, pObj->Value );
    return vValues;
}
void Gia_ManLoadValue( Gia_Man_t * p, Vec_Int_t * vValues )
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        pObj->Value = Vec_IntEntry(vValues, i);
}


/**Function*************************************************************

  Synopsis    [Returns the array containing the first fanout of each object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManFirstFanouts( Gia_Man_t * p )
{
    Vec_Int_t * vFans = Vec_IntStart( Gia_ManObjNum(p) );
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
        {
            if ( Vec_IntEntry(vFans, Gia_ObjFaninId0p(p, pObj)) == 0 )
                Vec_IntWriteEntry(vFans, Gia_ObjFaninId0p(p, pObj), i);
            if ( Vec_IntEntry(vFans, Gia_ObjFaninId1p(p, pObj)) == 0 )
                Vec_IntWriteEntry(vFans, Gia_ObjFaninId1p(p, pObj), i);
            if ( Gia_ObjIsMuxId(p, i) && Vec_IntEntry(vFans, Gia_ObjFaninId2p(p, pObj)) == 0 )
                Vec_IntWriteEntry(vFans, Gia_ObjFaninId2p(p, pObj), i);
        }
        else if ( Gia_ObjIsCo(pObj) )
        {
            if ( Vec_IntEntry(vFans, Gia_ObjFaninId0p(p, pObj)) == 0 )
                Vec_IntWriteEntry(vFans, Gia_ObjFaninId0p(p, pObj), i);
        }
    }
    return vFans;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if AIG has choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManHasChoices_very_old( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, Counter1 = 0, Counter2 = 0;
    int nFailNoRepr = 0;
    int nFailHaveRepr = 0;
    int nChoiceNodes = 0;
    int nChoices = 0;
    if ( p->pReprs == NULL || p->pNexts == NULL )
        return 0;
    // check if there are any representatives
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Gia_ObjReprObj( p, Gia_ObjId(p, pObj) ) )
        {
//            printf( "%d ", i );
            Counter1++;
        }
//        if ( Gia_ObjNext( p, Gia_ObjId(p, pObj) ) )
//            Counter2++;
    }
//    printf( "\n" );
    Gia_ManForEachObj( p, pObj, i )
    {
//        if ( Gia_ObjReprObj( p, Gia_ObjId(p, pObj) ) )
//            Counter1++;
        if ( Gia_ObjNext( p, Gia_ObjId(p, pObj) ) )
        {
//            printf( "%d ", i );
            Counter2++;
        }
    }
//    printf( "\n" );
    if ( Counter1 == 0 )
    {
        printf( "Warning: AIG has repr data-strucure but not reprs.\n" );
        return 0;
    }
    printf( "%d nodes have reprs.\n", Counter1 );
    printf( "%d nodes have nexts.\n", Counter2 );
    // check if there are any internal nodes without fanout
    // make sure all nodes without fanout have representatives
    // make sure all nodes with fanout have no representatives
    ABC_FREE( p->pRefs );
    Gia_ManCreateRefs( p );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( Gia_ObjRefNum(p, pObj) == 0 )
        {
            if ( Gia_ObjReprObj( p, Gia_ObjId(p, pObj) ) == NULL )
                nFailNoRepr++;
            else
                nChoices++;
        }
        else
        {
            if ( Gia_ObjReprObj( p, Gia_ObjId(p, pObj) ) != NULL )
                nFailHaveRepr++;
            if ( Gia_ObjNextObj( p, Gia_ObjId(p, pObj) ) != NULL )
                nChoiceNodes++;
        }
        if ( Gia_ObjReprObj( p, i ) )
            assert( Gia_ObjRepr(p, i) < i );
    }
    if ( nChoices == 0 )
        return 0;
    if ( nFailNoRepr )
    {
        printf( "Gia_ManHasChoices_very_old(): Error: %d internal nodes have no fanout and no repr.\n", nFailNoRepr );
//        return 0;
    }
    if ( nFailHaveRepr )
    {
        printf( "Gia_ManHasChoices_very_old(): Error: %d internal nodes have both fanout and repr.\n", nFailHaveRepr );
//        return 0;
    }
//    printf( "Gia_ManHasChoices_very_old(): AIG has %d choice nodes with %d choices.\n", nChoiceNodes, nChoices );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Proving multi-output properties.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManGroupProve( Gia_Man_t * pInit, char * pCommLine, int nGroupSize, int fVerbose )
{
    Abc_Frame_t * pAbc = Abc_FrameReadGlobalFrame();
    Gia_Man_t * p = Gia_ManDup( pInit );
    Gia_Man_t * pGroup;
    Vec_Int_t * vOuts;
    Vec_Int_t * vOutMap;
    Vec_Ptr_t * vCexes;
    int i, k, nGroupCur, nGroups;
    abctime clk, timeComm = 0;
    abctime timeStart = Abc_Clock();
    // pre-conditions
    assert( nGroupSize > 0 );
    assert( pCommLine != NULL );
    assert( p->nConstrs == 0 );
    Abc_Print( 1, "RUNNING MultiProve: Group size = %d. Command line = \"%s\".\n", nGroupSize, pCommLine );
    // create output map
    vOuts   = Vec_IntStartNatural( Gia_ManPoNum(p) );
    vOutMap = Vec_IntAlloc( Gia_ManPoNum(p) );
    vCexes  = Vec_PtrAlloc( Gia_ManPoNum(p) );
    nGroups = Gia_ManPoNum(p) / nGroupSize + (int)((Gia_ManPoNum(p) % nGroupSize) > 0);
    for ( i = 0; i < nGroups; i++ )
    {
        // derive the group
        nGroupCur = ((i + 1) * nGroupSize < Gia_ManPoNum(p)) ? nGroupSize : Gia_ManPoNum(p) - i * nGroupSize;
        pGroup = Gia_ManDupCones( p, Vec_IntArray(vOuts) + i * nGroupSize, nGroupCur, 0 );
        Abc_Print( 1, "GROUP %4d : %4d <= PoId < %4d : ", i, i * nGroupSize, i * nGroupSize + nGroupCur );
        // set the current GIA
        Abc_FrameUpdateGia( pAbc, pGroup );
        // solve the group
        clk = Abc_Clock();
        Cmd_CommandExecute( pAbc, pCommLine );
        timeComm += Abc_Clock() - clk;
        // get the solution status
        if ( nGroupSize == 1 )
        {
            Vec_IntPush( vOutMap, Abc_FrameReadProbStatus(pAbc) );
            Vec_PtrPush( vCexes, Abc_FrameReadCex(pAbc) );
        }
        else // if ( nGroupSize > 1 )  
        {
            Vec_Int_t * vStatusCur = Abc_FrameReadPoStatuses( pAbc );
            Vec_Ptr_t * vCexesCur = Abc_FrameReadCexVec( pAbc );
            assert( vStatusCur != NULL ); // only works for "bmc3" and "pdr"
//            assert( vCexesCur != NULL );
            for ( k = 0; k < nGroupCur; k++ )
            {
                Vec_IntPush( vOutMap, Vec_IntEntry(vStatusCur, k) );
                Vec_PtrPush( vCexes, vCexesCur ? Vec_PtrEntry(vCexesCur, k) : NULL );
            }
        }
    }
    assert( Vec_PtrSize(vCexes) == Gia_ManPoNum(p) );
    assert( Vec_IntSize(vOutMap) == Gia_ManPoNum(p) );
    // set CEXes
    if ( Vec_PtrCountZero(vCexes) < Vec_PtrSize(vCexes) )
        Abc_FrameReplaceCexVec( pAbc, &vCexes );
    else  // there is no CEXes
        Vec_PtrFree( vCexes );
    // report the result
    Abc_Print( 1, "SUMMARY:  " );
    Abc_Print( 1, "Properties = %6d. ", Gia_ManPoNum(p) );
    Abc_Print( 1, "UNSAT = %6d. ",      Vec_IntCountEntry(vOutMap, 1) );
    Abc_Print( 1, "SAT = %6d. ",        Vec_IntCountEntry(vOutMap, 0) );
    Abc_Print( 1, "UNDEC = %6d. ",      Vec_IntCountEntry(vOutMap, -1) );
    Abc_Print( 1, "\n" );
    Abc_PrintTime( 1, "Command time", timeComm );
    Abc_PrintTime( 1, "Total time  ", Abc_Clock() - timeStart );
    // cleanup
    Vec_IntFree( vOuts );
    Gia_ManStop( p );
    return vOutMap;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManPoXSim( Gia_Man_t * p, int nFrames, int fVerbose )
{
    Vec_Int_t * vRes;
    Gia_Obj_t * pObj;
    int f, k, nLeft = Gia_ManPoNum(p);
    vRes = Vec_IntAlloc( Gia_ManPoNum(p) );
    Vec_IntFill( vRes, Gia_ManPoNum(p), nFrames );
    Gia_ObjTerSimSet0( Gia_ManConst0(p) );
    Gia_ManForEachRi( p, pObj, k )
        Gia_ObjTerSimSet0( pObj );
    for ( f = 0; f < nFrames; f++ )
    {
        Gia_ManForEachPi( p, pObj, k )
            Gia_ObjTerSimSetX( pObj );
        Gia_ManForEachRo( p, pObj, k )
            Gia_ObjTerSimRo( p, pObj );
        Gia_ManForEachAnd( p, pObj, k )
            Gia_ObjTerSimAnd( pObj );
        Gia_ManForEachCo( p, pObj, k )
            Gia_ObjTerSimCo( pObj );
        if ( fVerbose )
        {
            Gia_ManForEachPo( p, pObj, k )
                Gia_ObjTerSimPrint( pObj );
            printf( "\n" );
        }
        Gia_ManForEachPo( p, pObj, k )
            if ( Vec_IntEntry(vRes, k) == nFrames && Gia_ObjTerSimGetX(pObj) )
                Vec_IntWriteEntry(vRes, k, f), nLeft--;
        if ( nLeft == 0 )
            break;
    }
    if ( fVerbose )
    {
        if ( nLeft == 0 )
            printf( "Simulation converged after %d frames.\n", f+1 );
        else
            printf( "Simulation terminated after %d frames.\n", nFrames );
    }        
//    Vec_IntPrint( vRes );
    return vRes;
}

#define MAX_LUT_SIZE 8
typedef struct Gia_MapLut_t_
{
    int        Type;          // node type: PI=1, PO=2, LUT=3
    int        Out;           // ID
    int        StartId;       // -1
    int        nFans;         // fanin count
    float      Delay;         // 0.0
    int        pFans[MAX_LUT_SIZE];  // fanin IDs
    unsigned   pTruth[MAX_LUT_SIZE<6?1:(1<<(MAX_LUT_SIZE-5))]; // the truth table
} Gia_MapLut_t;

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_AigerWriteLut( Gia_Man_t * p, char * pFileName )
{
    Gia_Obj_t * pObj;
    int i, k, iFan, iLut = 0;
    int LutSizeMax = Gia_ManLutSizeMax( p );
    int nUints = Abc_TruthWordNum(LutSizeMax);
    int nLuts = 1 + Gia_ManCiNum(p) + Gia_ManCoNum(p) + Gia_ManLutNum(p);
    Gia_MapLut_t * pLuts = ABC_CALLOC( Gia_MapLut_t, nLuts );
    Vec_Wrd_t * vTruths = Vec_WrdStart( Gia_ManObjNum(p) );
    assert( LutSizeMax <= 6 );
    // set obj numbers
    // constant
    pLuts->Type = 3;
    memset( pLuts->pTruth, 0xFF, sizeof(unsigned) * nUints );
    Gia_ManFillValue(p);
    Gia_ManConst0(p)->Value = pLuts[iLut].Out = Abc_Var2Lit( iLut, 0 );
    iLut++;
    // inputs
    Gia_ManForEachCi( p, pObj, i )
    {
        pLuts[iLut].Type = 1;
        memset( pLuts[iLut].pTruth, 0xAA, sizeof(unsigned) * nUints );
        pObj->Value = pLuts[iLut].Out = Abc_Var2Lit( iLut, 0 );
        iLut++;
    }
    // nodes
    Gia_ManForEachObj( p, pObj, i )
        if ( i && Gia_ObjIsLut(p, i) )
        {
            pLuts[iLut].Type = 3;
            Gia_LutForEachFanin( p, i, iFan, k )
                pLuts[iLut].pFans[k] = Gia_ManObj(p, iFan)->Value;
            pLuts[iLut].nFans = k;
            *(word *)pLuts[iLut].pTruth = Gia_LutComputeTruth6(p, i, vTruths);
            pObj->Value = pLuts[iLut].Out = Abc_Var2Lit( iLut, 0 );
            iLut++;
        }
    // outputs
    Gia_ManForEachCo( p, pObj, i )
    {
        pLuts[iLut].Type = 2;
        pLuts[iLut].pFans[0] = Gia_ObjFanin0(pObj)->Value;
        if ( Gia_ObjFaninC0(pObj) ^ Gia_ObjIsConst0(Gia_ObjFanin0(pObj)) )
            memset( pLuts[iLut].pTruth, 0x55, sizeof(unsigned) * nUints );
        else
            memset( pLuts[iLut].pTruth, 0xAA, sizeof(unsigned) * nUints );
        pLuts[iLut].nFans = 1;
        pObj->Value = pLuts[iLut].Out = Abc_Var2Lit( iLut, 0 );
        iLut++;
    }
    assert( iLut == nLuts );
    // dump into a file
    {
        FILE * pFile = fopen( pFileName, "wb" );
        if ( pFile == NULL )
            printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        else
        {
            int nSize1 = nLuts * sizeof(Gia_MapLut_t);
            int nSize2 = fwrite( pLuts, 1, nSize1, pFile );
            assert( nSize1 == nSize2 );
            printf( "Successfully dumped %d bytes of binary data.\n", nSize1 );
        }
        fclose( pFile );
    }
    ABC_FREE( pLuts );
    Vec_WrdFree( vTruths );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_DumpLutSizeDistrib( Gia_Man_t * p, char * pFileName )
{
    FILE * pTable = fopen( pFileName, "a+" );
    int i, Counts[10] = {0};
    Gia_ManForEachLut( p, i )
        if ( Gia_ObjLutSize(p, i) > 0 && Gia_ObjLutSize(p, i) < 10 )
            Counts[ Gia_ObjLutSize(p, i) ]++;
    fprintf( pTable, "%s", p->pName );
    for ( i = 1; i < 10; i++ )
        fprintf( pTable, " %d", Counts[i] );
    fprintf( pTable, "\n" );
    fclose( pTable );
}

/**Function*************************************************************

  Synopsis    [Check if two logic cones have overlap.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCheckSuppMark_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( pObj->fMark0 )
        return;
    pObj->fMark0 = 1;
    if ( Gia_ObjIsCi(pObj) )
        return;
    Gia_ManCheckSuppMark_rec( p, Gia_ObjFanin0(pObj) );
    Gia_ManCheckSuppMark_rec( p, Gia_ObjFanin1(pObj) );
}
void Gia_ManCheckSuppUnmark_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( !pObj->fMark0 )
        return;
    pObj->fMark0 = 0;
    if ( Gia_ObjIsCi(pObj) )
        return;
    Gia_ManCheckSuppUnmark_rec( p, Gia_ObjFanin0(pObj) );
    Gia_ManCheckSuppUnmark_rec( p, Gia_ObjFanin1(pObj) );
}
int Gia_ManCheckSupp_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( pObj->fMark0 )
        return 1;
    if ( Gia_ObjIsCi(pObj) )
        return 0;
    if ( Gia_ManCheckSupp_rec( p, Gia_ObjFanin0(pObj) ) )
        return 1;
    return Gia_ManCheckSupp_rec( p, Gia_ObjFanin1(pObj) );
}
int Gia_ManCheckSuppOverlap( Gia_Man_t * p, int iNode1, int iNode2 )
{
    int Result;
    if ( iNode1 == 0 || iNode2 == 0 )
        return 0;
    Gia_ManCheckSuppMark_rec( p, Gia_ManObj(p, iNode1) );
    Result = Gia_ManCheckSupp_rec( p, Gia_ManObj(p, iNode2) );
    Gia_ManCheckSuppUnmark_rec( p, Gia_ManObj(p, iNode1) );
    return Result;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

