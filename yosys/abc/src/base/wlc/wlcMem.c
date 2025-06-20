/**CFile****************************************************************

  FileName    [wlcMem.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Memory abstraction.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: wlcMem.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wlc.h"
#include "sat/bsat/satStore.h"
#include "proof/pdr/pdr.h"
#include "proof/pdr/pdrInt.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Memory blasting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_NtkMemBlast_rec( Wlc_Ntk_t * pNew, Wlc_Ntk_t * p, int iObj, Vec_Int_t * vFanins )
{
    Wlc_Obj_t * pObj;
    int i, iFanin;
    if ( Wlc_ObjCopy(p, iObj) )
        return;
    pObj = Wlc_NtkObj( p, iObj );
    Wlc_ObjForEachFanin( pObj, iFanin, i )
        Wlc_NtkMemBlast_rec( pNew, p, iFanin, vFanins );
    if ( pObj->Type == WLC_OBJ_WRITE )
    {
        Vec_Int_t * vTemp = Vec_IntAlloc( 1 );
        Vec_Int_t * vBits = Vec_IntAlloc( 100 );
        Wlc_Obj_t * pMem  = Wlc_ObjFanin0(p, pObj);
        Wlc_Obj_t * pAddr = Wlc_ObjFanin1(p, pObj);
        Wlc_Obj_t * pData = Wlc_ObjFanin2(p, pObj);
        int DataW = Wlc_ObjRange(pData);
        int AddrW = Wlc_ObjRange(pAddr);
        int nRegs = 1 << AddrW, iObjNew, iObjDec;
        assert( nRegs * DataW == Wlc_ObjRange(pMem) );
        assert( Wlc_ObjRange(pObj) == Wlc_ObjRange(pMem) );
        // create decoder
        iObjDec = Wlc_ObjAlloc( pNew, WLC_OBJ_DEC, 0, nRegs-1, 0 );
        Vec_IntFill( vTemp, 1, Wlc_ObjCopy(p, Wlc_ObjId(p, pAddr)) );
        Wlc_ObjAddFanins( pNew, Wlc_NtkObj(pNew, iObjDec), vTemp );
        // create decoder bits
        for ( i = 0; i < nRegs; i++ )
        {
            int iObj2 = Wlc_ObjAlloc( pNew, WLC_OBJ_BIT_SELECT, 0, i, i );
            Vec_IntFill( vTemp, 1, iObjDec );
            Vec_IntPushTwo( vTemp, i, i );
            Wlc_ObjAddFanins( pNew, Wlc_NtkObj(pNew, iObj2), vTemp );
            Vec_IntPush( vBits, iObj2 );
        }
        // create data words
        Vec_IntClear( vFanins );
        for ( i = 0; i < nRegs; i++ )
        {
            int iObj2 = Wlc_ObjAlloc( pNew, WLC_OBJ_BIT_SELECT, 0, i*DataW+DataW-1, i*DataW );
            Vec_IntFill( vTemp, 1, Wlc_ObjCopy(p, Wlc_ObjId(p, pMem)) );
            Vec_IntPushTwo( vTemp, i*DataW+DataW-1, i*DataW );
            Wlc_ObjAddFanins( pNew, Wlc_NtkObj(pNew, iObj2), vTemp );
            Vec_IntPush( vFanins, iObj2 );
        }
        // create MUXes of data words controlled by decoder bits
        for ( i = 0; i < nRegs; i++ )
        {
            int iObj2 = Wlc_ObjAlloc( pNew, WLC_OBJ_MUX, 0, DataW-1, 0 );
            Vec_IntFill( vTemp, 1, Vec_IntEntry(vBits, i) );
            Vec_IntPushTwo( vTemp, Wlc_ObjCopy(p, Wlc_ObjId(p, pData)), Vec_IntEntry(vFanins, i) );
            Wlc_ObjAddFanins( pNew, Wlc_NtkObj(pNew, iObj2), vTemp );
            Vec_IntWriteEntry( vFanins, i, iObj2 );
        }
        // concatenate the results
        iObjNew = Wlc_ObjAlloc( pNew, WLC_OBJ_BIT_CONCAT, 0, nRegs*DataW-1, 0 );
        Wlc_ObjAddFanins( pNew, Wlc_NtkObj(pNew, iObjNew), vFanins );
        Wlc_ObjSetCopy( p, iObj, iObjNew );
        Vec_IntFree( vTemp );
        Vec_IntFree( vBits );
    }
    else if ( pObj->Type == WLC_OBJ_READ )
    {
        Vec_Int_t * vTemp = Vec_IntAlloc( 1 );
        Wlc_Obj_t * pMem  = Wlc_ObjFanin0(p, pObj);
        Wlc_Obj_t * pAddr = Wlc_ObjFanin1(p, pObj);
        int DataW = Wlc_ObjRange(pObj);
        int AddrW = Wlc_ObjRange(pAddr);
        int nRegs = 1 << AddrW, iObjNew;
        assert( nRegs * DataW == Wlc_ObjRange(pMem) );
        Vec_IntClear( vFanins );
        Vec_IntPush( vFanins, Wlc_ObjCopy(p, Wlc_ObjId(p, pAddr)) );
        for ( i = 0; i < nRegs; i++ )
        {
            int iObj2 = Wlc_ObjAlloc( pNew, WLC_OBJ_BIT_SELECT, 0, i*DataW+DataW-1, i*DataW );
            Vec_IntFill( vTemp, 1, Wlc_ObjCopy(p, Wlc_ObjId(p, pMem)) );
            Vec_IntPushTwo( vTemp, i*DataW+DataW-1, i*DataW );
            Wlc_ObjAddFanins( pNew, Wlc_NtkObj(pNew, iObj2), vTemp );
            Vec_IntPush( vFanins, iObj2 );
        }
        iObjNew = Wlc_ObjAlloc( pNew, WLC_OBJ_MUX, 0, DataW-1, 0 );
        Wlc_ObjAddFanins( pNew, Wlc_NtkObj(pNew, iObjNew), vFanins );
        Wlc_ObjSetCopy( p, iObj, iObjNew );
        Vec_IntFree( vTemp );
    }
    else
        Wlc_ObjDup( pNew, p, iObj, vFanins );
}
Wlc_Ntk_t * Wlc_NtkMemBlast( Wlc_Ntk_t * p )
{
    Wlc_Ntk_t * pNew;
    Wlc_Obj_t * pObj;
    Vec_Int_t * vFanins;
    int i;
    Wlc_NtkCleanCopy( p );
    vFanins = Vec_IntAlloc( 100 );
    pNew = Wlc_NtkAlloc( p->pName, p->nObjsAlloc );
    pNew->fSmtLib = p->fSmtLib;
    pNew->fAsyncRst = p->fAsyncRst;
    pNew->fMemPorts = p->fMemPorts;
    pNew->fEasyFfs = p->fEasyFfs;
    Wlc_NtkForEachCi( p, pObj, i )
        Wlc_ObjDup( pNew, p, Wlc_ObjId(p, pObj), vFanins );
    Wlc_NtkForEachCo( p, pObj, i )
        Wlc_NtkMemBlast_rec( pNew, p, Wlc_ObjId(p, pObj), vFanins );
    Wlc_NtkForEachCo( p, pObj, i )
        Wlc_ObjSetCo( pNew, Wlc_ObjCopyObj(pNew, p, pObj), pObj->fIsFi );
    if ( p->vInits )
    pNew->vInits = Vec_IntDup( p->vInits );
    if ( p->pInits )
    pNew->pInits = Abc_UtilStrsav( p->pInits );
    Vec_IntFree( vFanins );
    if ( p->pSpec )
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Collect memory nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Wlc_NtkCollectMemSizes( Wlc_Ntk_t * p )
{
    Wlc_Obj_t * pObj; int i;
    Vec_Int_t * vMemSizes = Vec_IntAlloc( 10 );
    Wlc_NtkForEachObj( p, pObj, i )
    {
        if ( Wlc_ObjType(pObj) != WLC_OBJ_WRITE && Wlc_ObjType(pObj) != WLC_OBJ_READ )
            continue;
        pObj = Wlc_ObjFanin( p, pObj, 0 );
        Vec_IntPushUnique( vMemSizes, Wlc_ObjRange(pObj) );
    }
    return vMemSizes;
}
// remove PIs without fanout, non-driven object and their fanouts
int Wlc_ObjCheckIsEmpty_rec( Wlc_Ntk_t * p, Wlc_Obj_t * pObj )
{
    int k, iFanin;
    if ( Wlc_ObjType(pObj) == 0 )
        return 1;
    if ( Wlc_ObjIsPi(pObj) )
        return Vec_IntEntry(&p->vRefs, Wlc_ObjId(p, pObj)) == 0;
    else if ( Wlc_ObjIsCi(pObj) )
        return 0;
    Wlc_ObjForEachFanin( pObj, iFanin, k )
        if ( !Wlc_ObjCheckIsEmpty_rec(p, Wlc_NtkObj(p, iFanin)) )
            return 0;
    return 1;
}
Vec_Int_t * Wlc_NtkCleanObjects( Wlc_Ntk_t * p, Vec_Int_t * vObjs )
{
    Wlc_Obj_t * pObj; int i;
    Vec_Int_t * vMemObjs = Vec_IntAlloc( 10 );
    Wlc_NtkSetRefs( p );
    Wlc_NtkForEachObjVec( vObjs, p, pObj, i )
    {
        //printf( "Considering %d (%s)\n", Wlc_ObjId(p, pObj), Wlc_ObjName(p, Wlc_ObjId(p, pObj)) );
        if ( !Wlc_ObjCheckIsEmpty_rec(p, pObj) )
            Vec_IntPush( vMemObjs, Wlc_ObjId(p, pObj) );
    }
    return vMemObjs;
}
Vec_Int_t * Wlc_NtkCollectMemory( Wlc_Ntk_t * p, int fClean )
{
    Wlc_Obj_t * pObj; int i;
    Vec_Int_t * vMemSizes = Wlc_NtkCollectMemSizes( p );
    Vec_Int_t * vMemObjs = Vec_IntAlloc( 10 );
    Wlc_NtkForEachObj( p, pObj, i )
    {
        if ( Wlc_ObjType(pObj) == WLC_OBJ_WRITE || Wlc_ObjType(pObj) == WLC_OBJ_READ )
            Vec_IntPush( vMemObjs, i );
        else if ( Vec_IntFind(vMemSizes, Wlc_ObjRange(pObj)) >= 0 )
            Vec_IntPush( vMemObjs, i );
    }
    Vec_IntFree( vMemSizes );
    Vec_IntSort( vMemObjs, 0 );
    //Wlc_NtkPrintNodeArray( p, vMemObjs ); 
    if ( fClean )
    {
        Vec_Int_t * vTemp;
        vMemObjs = Wlc_NtkCleanObjects( p, vTemp = vMemObjs );
        Vec_IntFree( vTemp );
    }
    return vMemObjs;
}
void Wlc_NtkPrintMemory( Wlc_Ntk_t * p )
{
    Vec_Int_t * vMemory;
    vMemory = Wlc_NtkCollectMemory( p, 1 );
    printf( "Memory subsystem is composed of the following objects:\n" );
    Wlc_NtkPrintNodeArray( p, vMemory );
    Vec_IntFree( vMemory );
}

/**Function*************************************************************

  Synopsis    [Collect fanins of memory subsystem.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Wlc_NtkCollectMemFanins( Wlc_Ntk_t * p, Vec_Int_t * vMemObjs )
{
    Wlc_Obj_t * pObj; int i, k, iFanin;
    Vec_Int_t * vMemFanins = Vec_IntAlloc( 100 );
    Wlc_NtkForEachObjVec( vMemObjs, p, pObj, i )
    {
        if ( Wlc_ObjType(pObj) == WLC_OBJ_MUX )
            Vec_IntPush( vMemFanins, Wlc_ObjFaninId0(pObj) );
        else if ( Wlc_ObjType(pObj) == WLC_OBJ_READ || Wlc_ObjType(pObj) == WLC_OBJ_WRITE )
        {
            Wlc_ObjForEachFanin( pObj, iFanin, k )
                if ( k > 0 )
                    Vec_IntPush( vMemFanins, iFanin );
        }
    }
    return vMemFanins;
}

/**Function*************************************************************

  Synopsis    [Abstract memory nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wlc_CountDcs( char * pInit )
{
    int Count = 0;
    for ( ; *pInit; pInit++ )
        Count += (*pInit == 'x' || *pInit == 'X');
    return Count;
}
int Wlc_NtkDupOneObject( Wlc_Ntk_t * pNew, Wlc_Ntk_t * p, Wlc_Obj_t * pObj, int TypeNew, Vec_Int_t * vFanins )
{
    int iObj = Wlc_ObjId(p, pObj);
    unsigned Type = pObj->Type;
    int iObjNew, nFanins = Wlc_ObjFaninNum(pObj);
    int iObjCopy = Wlc_ObjCopy(p, iObj);
    pObj->Type = TypeNew;
    pObj->nFanins = 0;
    iObjNew = Wlc_ObjDup( pNew, p, iObj, vFanins );
    pObj->Type = Type;
    pObj->nFanins = (unsigned)nFanins;
    if ( TypeNew == WLC_OBJ_FO )
    {
        Vec_IntPush( pNew->vInits, -Wlc_ObjRange(pObj) );
        Wlc_ObjSetCopy( p, iObj, iObjCopy );
    }
    return iObjNew;
}
void Wlc_NtkDupOneBuffer( Wlc_Ntk_t * pNew, Wlc_Ntk_t * p, Wlc_Obj_t * pObj, int iFanin, Vec_Int_t * vFanins, int fIsFi )
{
    int iObj = Wlc_ObjAlloc( pNew, WLC_OBJ_BUF, pObj->Signed, pObj->End, pObj->Beg );
    Wlc_Obj_t * pObjNew = Wlc_NtkObj( pNew, iObj );
    Vec_IntFill( vFanins, 1, iFanin );
    Wlc_ObjAddFanins( pNew, pObjNew, vFanins );
    Wlc_ObjSetCo( pNew, pObjNew, fIsFi );
}

void Wlc_NtkAbsAddToNodeFrames( Vec_Int_t * vNodeFrames, Vec_Int_t * vLevel ) 
{
    int i, Entry;
    Vec_IntForEachEntry( vLevel, Entry, i )
        Vec_IntPushUnique( vNodeFrames, Entry );
    Vec_IntSort( vNodeFrames, 0 );
}
Vec_Int_t * Wlc_NtkAbsCreateFlopOutputs( Wlc_Ntk_t * pNew, Wlc_Ntk_t * p, Vec_Int_t * vNodeFrames, Vec_Int_t * vFanins ) 
{
    Vec_Int_t * vNewObjs = Vec_IntAlloc( 2*Vec_IntSize(vNodeFrames) );
    Wlc_Obj_t * pObj, * pFanin = NULL;
    int i, Entry, iObj, iFrame;
    Vec_IntForEachEntry( vNodeFrames, Entry, i )
    {
        iObj   = Entry >> 11;
        iFrame = (Entry >> 1) & 0x3FF;
        pObj   = Wlc_NtkObj( p, iObj );
        // address
        if ( Wlc_ObjType(pObj) == WLC_OBJ_MUX )
            pFanin = Wlc_ObjFanin0(p, pObj);
        else if ( Wlc_ObjType(pObj) == WLC_OBJ_READ || Wlc_ObjType(pObj) == WLC_OBJ_WRITE )
            pFanin = Wlc_ObjFanin1(p, pObj);
        else assert( 0 );
        Vec_IntPush( vNewObjs, Wlc_NtkDupOneObject(pNew, p, pFanin, WLC_OBJ_FO, vFanins) );
        // data
        if ( Wlc_ObjType(pObj) == WLC_OBJ_MUX )
            pFanin = NULL;
        else if ( Wlc_ObjType(pObj) == WLC_OBJ_READ )
            pFanin = pObj;
        else if ( Wlc_ObjType(pObj) == WLC_OBJ_WRITE )
            pFanin = Wlc_ObjFanin2(p, pObj);
        else assert( 0 );
        Vec_IntPush( vNewObjs, pFanin ? Wlc_NtkDupOneObject(pNew, p, pFanin, WLC_OBJ_FO, vFanins) : 0 );
    }
    assert( Vec_IntSize(vNewObjs) == 2 * Vec_IntSize(vNodeFrames) );
    return vNewObjs;
}
void Wlc_NtkAbsCreateFlopInputs( Wlc_Ntk_t * pNew, Wlc_Ntk_t * p, Vec_Int_t * vNodeFrames, Vec_Int_t * vFanins, Vec_Int_t * vNewObjs, Wlc_Obj_t * pCounter, int AdderBits ) 
{
    Wlc_Obj_t * pObj, * pFanin, * pFlop, * pCond, * pMux, * pConst;
    int i, n, Entry, Value, iObj, iFrame;
    Vec_IntForEachEntry( vNodeFrames, Entry, i )
    {
        Value  = Entry & 1;
        iObj   = Entry >> 11;
        iFrame = (Entry >> 1) & 0x3FF;
        pObj   = Wlc_NtkObj( p, iObj );
        for ( n = 0; n < 2; n++ ) // n=0 -- address   n=1 -- data
        {
            pFlop = Wlc_NtkObj( pNew, Vec_IntEntry(vNewObjs, 2*i+n) );
            if ( Wlc_ObjType(pObj) == WLC_OBJ_WRITE )
                pFanin = Wlc_ObjCopyObj( pNew, p, n ? Wlc_ObjFanin2(p, pObj) : Wlc_ObjFanin1(p, pObj) );
            else if ( Wlc_ObjType(pObj) == WLC_OBJ_READ )
                pFanin = n ? Wlc_NtkObj(pNew, Wlc_ObjCopy(p, iObj)) : Wlc_ObjCopyObj( pNew, p, Wlc_ObjFanin1(p, pObj) );
            else if ( Wlc_ObjType(pObj) == WLC_OBJ_MUX )
            {
                if ( n ) continue;
                pFanin = Wlc_ObjCopyObj( pNew, p, Wlc_ObjFanin0(p, pObj) );
                if ( Value )
                {
                    Vec_IntFill( vFanins, 1, Wlc_ObjId(pNew, pFanin) );
                    pFanin = Wlc_NtkObj(pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_BIT_NOT, 0, 0, 0));
                    Wlc_ObjAddFanins( pNew, pFanin, vFanins );
                }
            }
            else assert( 0 );
            assert( Wlc_ObjRange(pFlop) == Wlc_ObjRange(pFanin) );
            // create constant
            pConst = Wlc_NtkObj(pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_CONST, 0, AdderBits-1, 0));
            Vec_IntFill( vFanins, 1, iFrame );
            Wlc_ObjAddFanins( pNew, pConst, vFanins );
            // create equality
            pCond = Wlc_NtkObj(pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_COMP_EQU, 0, 0, 0));
            Vec_IntFillTwo( vFanins, 2, Wlc_ObjId(pNew, pConst), Wlc_ObjId(pNew, pCounter) );
            Wlc_ObjAddFanins( pNew, pCond, vFanins );
            // create MUX
            pMux = Wlc_NtkObj(pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_MUX, 0, Wlc_ObjRange(pFlop)-1, 0));
            Vec_IntClear( vFanins );
            Vec_IntPush( vFanins, Wlc_ObjId(pNew, pCond) );
            Vec_IntPush( vFanins, Wlc_ObjId(pNew, pFlop) );
            Vec_IntPush( vFanins, Wlc_ObjId(pNew, pFanin) );
            Wlc_ObjAddFanins( pNew, pMux, vFanins );
            Wlc_ObjSetCo( pNew, pMux, 1 );
        }
    }
}
void Wlc_NtkAbsCreateLogic( Wlc_Ntk_t * pNew, Wlc_Ntk_t * p, Vec_Int_t * vNodeFrames, Vec_Int_t * vFanins, Vec_Int_t * vNewObjs, Vec_Wec_t * vConstrs, Wlc_Obj_t * pConstrOut ) 
{
    Vec_Int_t * vTrace; 
    Vec_Int_t * vBitConstr = Vec_IntAlloc( 100 );
    Vec_Int_t * vLevConstr = Vec_IntAlloc( 100 );
    Wlc_Obj_t * pAddrs[2], * pDatas[2], * pCond, * pConstr = NULL;
    int i, k, Entry, Index[2], iFrameLast, iFrameThis;
    assert( Vec_IntSize(vNewObjs) == 2 * Vec_IntSize(vNodeFrames) );
    Vec_WecForEachLevel( vConstrs, vTrace, i )
    {
        if ( Vec_IntSize(vTrace) == 0 ) 
            continue;
        assert( Vec_IntSize(vTrace) >= 2 );
        Vec_IntClear( vLevConstr );

        iFrameThis = (Vec_IntEntry(vTrace, 0)  >> 1) & 0x3FF;
        iFrameLast = (Vec_IntEntryLast(vTrace) >> 1) & 0x3FF;

        Index[0] = Vec_IntFind( vNodeFrames, Vec_IntEntry(vTrace, 0) );
        Index[1] = Vec_IntFind( vNodeFrames, Vec_IntEntryLast(vTrace) );
        assert( Index[0] >= 0 && Index[1] >= 0 );

        pAddrs[0] = Wlc_NtkObj( pNew, Vec_IntEntry(vNewObjs, 2*Index[0]) );
        pAddrs[1] = Wlc_NtkObj( pNew, Vec_IntEntry(vNewObjs, 2*Index[1]) );

        pDatas[0] = Wlc_NtkObj( pNew, Vec_IntEntry(vNewObjs, 2*Index[0]+1) );
        pDatas[1] = Wlc_NtkObj( pNew, Vec_IntEntry(vNewObjs, 2*Index[1]+1) );

        // redirect the source in the last time frame to point to FOs
        if ( iFrameThis == iFrameLast )
        {
            pAddrs[0] = Wlc_ObjFo2Fi(pNew, pAddrs[0]);
            pDatas[0] = Wlc_ObjFo2Fi(pNew, pDatas[0]);
        }

        // redirect the sink in the last time frame to point to FOs
        pAddrs[1] = Wlc_ObjFo2Fi(pNew, pAddrs[1]);
        pDatas[1] = Wlc_ObjFo2Fi(pNew, pDatas[1]);

        // equal addresses
        pCond = Wlc_NtkObj(pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_COMP_EQU, 0, 0, 0));
        Vec_IntFillTwo( vFanins, 2, Wlc_ObjId(pNew, pAddrs[0]), Wlc_ObjId(pNew, pAddrs[1]) );
        Wlc_ObjAddFanins( pNew, pCond, vFanins );
        Vec_IntPush( vLevConstr, Wlc_ObjId(pNew, pCond) );

        // different datas
        pCond = Wlc_NtkObj(pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_COMP_NOTEQU, 0, 0, 0));
        Vec_IntFillTwo( vFanins, 2, Wlc_ObjId(pNew, pDatas[0]), Wlc_ObjId(pNew, pDatas[1]) );
        Wlc_ObjAddFanins( pNew, pCond, vFanins );
        Vec_IntPush( vLevConstr, Wlc_ObjId(pNew, pCond) );

        Vec_IntForEachEntryStartStop( vTrace, Entry, k, 1, Vec_IntSize(vTrace)-1 )
        {
            iFrameThis = (Entry >> 1) & 0x3FF;
            Index[0]   = Vec_IntFind( vNodeFrames, Entry );            
            assert( Index[0] >= 0 );
            pAddrs[0]  = Wlc_NtkObj( pNew, Vec_IntEntry(vNewObjs, 2*Index[0]) );
            // redirect the middle in the last time frame to point to FOs
            if ( iFrameThis == iFrameLast )
                pAddrs[0] = Wlc_ObjFo2Fi(pNew, pAddrs[0]);
            if ( Vec_IntEntry(vNewObjs, 2*Index[0]+1) == 0 ) // MUX
            {
                assert( Wlc_ObjType(Wlc_NtkObj(p, Entry >> 11)) == WLC_OBJ_MUX );
                Vec_IntPush( vLevConstr, Wlc_ObjId(pNew, pAddrs[0]) );
            }
            else // different addresses
            {
                assert( Wlc_ObjRange(pAddrs[0]) == Wlc_ObjRange(pAddrs[1]) );
                pCond = Wlc_NtkObj(pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_COMP_NOTEQU, 0, 0, 0));
                Vec_IntFillTwo( vFanins, 2, Wlc_ObjId(pNew, pAddrs[0]), Wlc_ObjId(pNew, pAddrs[1]) );
                Wlc_ObjAddFanins( pNew, pCond, vFanins );
                Vec_IntPush( vLevConstr, Wlc_ObjId(pNew, pCond) );
            }
        }

        // create last output
        pConstr = Wlc_NtkObj( pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_BIT_CONCAT, 0, Vec_IntSize(vLevConstr)-1, 0) );
        Wlc_ObjAddFanins( pNew, pConstr, vLevConstr );
        Vec_IntFill( vFanins, 1, Wlc_ObjId(pNew, pConstr) );
        pConstr = Wlc_NtkObj(pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_REDUCT_AND, 0, 0, 0));
        Wlc_ObjAddFanins( pNew, pConstr, vFanins );

        // save the result
        Vec_IntPush( vBitConstr, Wlc_ObjId(pNew, pConstr) );
    }
    if ( Vec_IntSize(vBitConstr) > 0 )
    {
        // create last output
        pConstr = Wlc_NtkObj(pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_BIT_CONCAT, 0, Vec_IntSize(vBitConstr)-1, 0));
        Wlc_ObjAddFanins( pNew, pConstr, vBitConstr );
        // create last output
        Vec_IntFill( vFanins, 1, Wlc_ObjId(pNew, pConstr) );
        pConstr = Wlc_NtkObj(pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_REDUCT_OR, 0, 0, 0));
        Wlc_ObjAddFanins( pNew, pConstr, vFanins );
    }
    else
    {
        pConstr = Wlc_NtkObj(pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_CONST, 0, 0, 0));
        Vec_IntFill( vFanins, 1, 0 ); // const0 - always true
        Wlc_ObjAddFanins( pNew, pConstr, vFanins );
    }
    // cleanup
    Vec_IntFree( vBitConstr );
    Vec_IntFree( vLevConstr );
    // add the constraint as fanin to the output
    Vec_IntFill( vFanins, 1, Wlc_ObjId(pNew, pConstr) );
    Wlc_ObjAddFanins( pNew, pConstrOut, vFanins );
}
Wlc_Ntk_t * Wlc_NtkAbstractMemory( Wlc_Ntk_t * p, Vec_Int_t * vMemObjs, Vec_Int_t * vMemFanins, int * piFirstMemPi, int * piFirstCi, int * piFirstMemCi, Vec_Wec_t * vConstrs, Vec_Int_t * vNodeFrames )
{
    Wlc_Ntk_t * pNew, * pTemp;
    Wlc_Obj_t * pObj, * pCounter, * pConst, * pAdder, * pConstr = NULL;
    Vec_Int_t * vNewObjs = NULL;
    Vec_Int_t * vFanins  = Vec_IntAlloc( 100 );
    int i, Po0, Po1, AdderBits = 16, nBits = 0;

    // mark memory nodes
    Wlc_NtkCleanMarks( p );
    Wlc_NtkForEachObjVec( vMemObjs, p, pObj, i )
        pObj->Mark = 1;

    // start new network
    Wlc_NtkCleanCopy( p );
    pNew = Wlc_NtkAlloc( p->pName, p->nObjsAlloc + 1000 );
    pNew->fSmtLib = p->fSmtLib;
    pNew->fAsyncRst = p->fAsyncRst;
    pNew->fMemPorts = p->fMemPorts;
    pNew->fEasyFfs = p->fEasyFfs;
    pNew->vInits = Vec_IntAlloc( 100 );

    // duplicate PIs
    Wlc_NtkForEachPi( p, pObj, i )
        if ( !pObj->Mark )
        {
            Wlc_ObjDup( pNew, p, Wlc_ObjId(p, pObj), vFanins );
            nBits += Wlc_ObjRange(pObj);
        }

    // create new PIs
    *piFirstMemPi = nBits;
    Wlc_NtkForEachObjVec( vMemObjs, p, pObj, i )
        if ( Wlc_ObjType(pObj) == WLC_OBJ_READ )
        {
            Wlc_NtkDupOneObject( pNew, p, pObj, WLC_OBJ_PI, vFanins );
            nBits += Wlc_ObjRange(pObj);
        }

    // duplicate flop outputs
    *piFirstCi = nBits;
    Wlc_NtkForEachCi( p, pObj, i )
        if ( !Wlc_ObjIsPi(pObj) && !pObj->Mark )
        {
            Wlc_ObjDup( pNew, p, Wlc_ObjId(p, pObj), vFanins );
            Vec_IntPush( pNew->vInits, Vec_IntEntry(p->vInits, i-Wlc_NtkPiNum(p)) );
            nBits += Wlc_ObjRange(pObj);
        }

    // create flop for time-frame counter 
    pCounter = Wlc_NtkObj(pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_FO, 0, AdderBits-1, 0));
    Vec_IntPush( pNew->vInits, -AdderBits );
    nBits += AdderBits;

    // create flops for memory objects
    *piFirstMemCi = nBits;
    if ( vMemFanins )
        Wlc_NtkForEachObjVec( vMemFanins, p, pObj, i )
            Wlc_NtkDupOneObject( pNew, p, pObj, WLC_OBJ_FO, vFanins );

    // create flops for constraint objects
    if ( vConstrs )
        vNewObjs = Wlc_NtkAbsCreateFlopOutputs( pNew, p, vNodeFrames, vFanins );

    // duplicate objects
    Wlc_NtkForEachObj( p, pObj, i )
        if ( !Wlc_ObjIsCi(pObj) && !pObj->Mark )
            Wlc_ObjDup( pNew, p, Wlc_ObjId(p, pObj), vFanins );

    if ( Vec_IntSize(&p->vPoPairs) )
    {
        // create miter for the PO pairs
        Vec_IntForEachEntryDouble( &p->vPoPairs, Po0, Po1, i )
        {
            int iCopy0 = Wlc_ObjCopy(p, Wlc_ObjId(p, Wlc_NtkPo(p, Po0)));
            int iCopy1 = Wlc_ObjCopy(p, Wlc_ObjId(p, Wlc_NtkPo(p, Po1)));
            int iObj = Wlc_ObjAlloc( pNew, WLC_OBJ_COMP_NOTEQU, 0, 0, 0 );
            Wlc_Obj_t * pObjNew = Wlc_NtkObj( pNew, iObj );
            Vec_IntFillTwo( vFanins, 1, iCopy0, iCopy1 );
            Wlc_ObjAddFanins( pNew, pObjNew, vFanins );
            Wlc_ObjSetCo( pNew, pObjNew, 0 );
        }
        printf( "Memory abstraction created %d miter outputs.\n", Wlc_NtkPoNum(pNew) );
        Wlc_NtkForEachCo( p, pObj, i )
            if ( pObj->fIsFi && !pObj->Mark )
                Wlc_ObjSetCo( pNew, Wlc_ObjCopyObj(pNew, p, pObj), pObj->fIsFi );

        // create constraint output
        if ( vConstrs )
            Wlc_ObjSetCo( pNew, (pConstr = Wlc_NtkObj(pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_BUF, 0, 0, 0))), 0 );
    }
    else
    {
        // duplicate POs
        Wlc_NtkForEachPo( p, pObj, i )
            if ( !pObj->Mark )
                Wlc_ObjSetCo( pNew, Wlc_ObjCopyObj(pNew, p, pObj), pObj->fIsFi );
        // create constraint output
        if ( vConstrs )
            Wlc_ObjSetCo( pNew, (pConstr = Wlc_NtkObj(pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_BUF, 0, 0, 0))), 0 );
        // duplicate FIs
        Wlc_NtkForEachCo( p, pObj, i )
            if ( !Wlc_ObjIsPo(pObj) && !pObj->Mark )
                Wlc_ObjSetCo( pNew, Wlc_ObjCopyObj(pNew, p, pObj), pObj->fIsFi );
    }

    // create time-frame counter
    pConst = Wlc_NtkObj(pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_CONST, 0, AdderBits-1, 0));
    Vec_IntFill( vFanins, 1, 1 );
    Wlc_ObjAddFanins( pNew, pConst, vFanins );

    pAdder = Wlc_NtkObj(pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_ARI_ADD, 0, AdderBits-1, 0));
    Vec_IntFillTwo( vFanins, 2, Wlc_ObjId(pNew, pCounter), Wlc_ObjId(pNew, pConst) );
    Wlc_ObjAddFanins( pNew, pAdder, vFanins );
    Wlc_ObjSetCo( pNew, pAdder, 1 );

    // create new flop inputs
    if ( vMemFanins )
        Wlc_NtkForEachObjVec( vMemFanins, p, pObj, i )
            Wlc_NtkDupOneBuffer( pNew, p, pObj, Wlc_ObjCopy(p, Wlc_ObjId(p, pObj)), vFanins, 1 );

    // create flop inputs for constraint objects
    if ( vConstrs )
        Wlc_NtkAbsCreateFlopInputs( pNew, p, vNodeFrames, vFanins, vNewObjs, pCounter, AdderBits );

    // create constraint logic nodes
    if ( vConstrs )
        Wlc_NtkAbsCreateLogic( pNew, p, vNodeFrames, vFanins, vNewObjs, vConstrs, pConstr );

    // append init states
    pNew->pInits = Wlc_PrsConvertInitValues( pNew );
    if ( p->pSpec )
        pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    //Wlc_NtkTransferNames( pNew, p );
    Vec_IntFree( vFanins );
    Vec_IntFreeP( &vNewObjs );
    Wlc_NtkCleanMarks( p );
    // derive topological order
    pNew = Wlc_NtkDupDfs( pTemp = pNew, 0, 1 );
    Wlc_NtkFree( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Derives values of memory subsystem objects under the CEX.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Wlc_NtkDeriveFirstTotal( Wlc_Ntk_t * p, Vec_Int_t * vMemObjs, Vec_Int_t * vMemFanins, int iFirstMemPi, int iFirstMemCi, int fVerbose )
{
    Vec_Int_t * vFirstTotal = Vec_IntStart( 3 * Vec_IntSize(vMemObjs) ); // contains pairs of (first CO bit: range) for each input/output of a node
    Wlc_Obj_t * pObj, * pFanin; int i, k, iFanin, nMemFanins = 0;
    Wlc_NtkForEachObjVec( vMemObjs, p, pObj, i )
    {
        if ( Wlc_ObjType(pObj) == WLC_OBJ_MUX )
        {
            //Vec_IntPush( vMemFanins, Wlc_ObjFaninId0(pObj) );
            pFanin = Wlc_ObjFanin0(p, pObj);
            assert( Wlc_ObjRange(pFanin) == 1 );
            Vec_IntWriteEntry( vFirstTotal, 3*i, (iFirstMemCi << 10) | Wlc_ObjRange(pFanin) );
            iFirstMemCi += Wlc_ObjRange(pFanin);
            nMemFanins++;
        }
        else if ( Wlc_ObjType(pObj) == WLC_OBJ_READ || Wlc_ObjType(pObj) == WLC_OBJ_WRITE )
        {
            Wlc_ObjForEachFanin( pObj, iFanin, k )
                if ( k > 0 )
                {
                    //Vec_IntPush( vMemFanins, iFanin );
                    pFanin = Wlc_NtkObj(p, iFanin);
                    Vec_IntWriteEntry( vFirstTotal, 3*i + k, (iFirstMemCi << 10) | Wlc_ObjRange(pFanin) );
                    iFirstMemCi += Wlc_ObjRange(pFanin);
                    nMemFanins++;
                }
            if ( Wlc_ObjType(pObj) == WLC_OBJ_READ )
            {
                Vec_IntWriteEntry( vFirstTotal, 3*i + 2, (iFirstMemPi << 10) | Wlc_ObjRange(pObj) );
                iFirstMemPi += Wlc_ObjRange(pObj);
            }
        }
    }
    assert( nMemFanins == Vec_IntSize(vMemFanins) );
    if ( fVerbose )
    Vec_IntForEachEntry( vFirstTotal, iFanin, i )
    {
        printf( "Obj %5d  Fanin %5d : ", i/3, i%3 );
        printf( "%16s : %d(%d)\n", Wlc_ObjName(p, Vec_IntEntry(vMemObjs, i/3)), iFanin >> 10, iFanin & 0x3FF );
    }
    return vFirstTotal;
}
int Wlc_NtkCexResim( Gia_Man_t * pAbs, Abc_Cex_t * p, Vec_Int_t * vFirstTotal, int iBit, Vec_Wrd_t * vRes, int iFrame )
{
    Gia_Obj_t * pObj, * pObjRi, * pObjRo; 
    int k, b, Entry, First, nBits; word Value;
    // assuming that flop outputs have been assigned in the previous timeframe
    // assign primary inputs
    Gia_ManForEachPi( pAbs, pObj, k )
        pObj->fMark0 = Abc_InfoHasBit(p->pData, iBit++);
    // simulate
    Gia_ManForEachAnd( pAbs, pObj, k )
        pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & 
                       (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj));
    // transfer to combinational outputs
    Gia_ManForEachCo( pAbs, pObj, k )
        pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
    // transfer values to flop outputs
    Gia_ManForEachRiRo( pAbs, pObjRi, pObjRo, k )
        pObjRo->fMark0 = pObjRi->fMark0;
    // at this point PIs and FOs area assigned according to this time-frame
    // collect values
    Vec_IntForEachEntry( vFirstTotal, Entry, k )
    {
        if ( Entry == 0 )
        {
            Vec_WrdWriteEntry( vRes, Vec_IntSize(vFirstTotal)*iFrame + k, ~(word)0 );
            continue;
        }
        First = Entry >> 10;
        assert( First < Gia_ManCiNum(pAbs) );
        nBits = Entry & 0x3FF;
        assert( nBits <= 64 );
        Value = 0;
        for ( b = 0; b < nBits; b++ )
            if ( Gia_ManCi(pAbs, First+b)->fMark0 )
                Value |= ((word)1) << b;
        Vec_WrdWriteEntry( vRes, Vec_IntSize(vFirstTotal)*iFrame + k, Value );
    }
    return iBit;
}
Vec_Wrd_t * Wlc_NtkConvertCex( Vec_Int_t * vFirstTotal, Gia_Man_t * pAbs, Abc_Cex_t * pCex, int fVerbose )
{
    Vec_Wrd_t * vValues = Vec_WrdStartFull( Vec_IntSize(vFirstTotal) * (pCex->iFrame + 1) );
    int k, f, nBits = pCex->nRegs; word Value;
    Gia_ManCleanMark0(pAbs); // init FOs to zero
    for ( f = 0; f <= pCex->iFrame; f++ )
        nBits = Wlc_NtkCexResim( pAbs, pCex, vFirstTotal, nBits, vValues, f );
    if ( fVerbose )
    Vec_WrdForEachEntry( vValues, Value, k )
    {
        if ( k % Vec_IntSize(vFirstTotal) == 0 )
            printf( "Frame %d:\n", k/Vec_IntSize(vFirstTotal) );
        printf( "Obj %5d  Fanin %5d : ", k/3, k%3 );
        Extra_PrintBinary( stdout, (unsigned *)&Value, 32 );
        printf( "\n" );
    }
    return vValues;
}

/**Function*************************************************************

  Synopsis    [Derives the reason for failure in terms of memory values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_NtkTrace_rec( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, int iFrame, Vec_Int_t * vMemObjs, Vec_Wrd_t * vValues, word ValueA, Vec_Int_t * vRes )
{
    int iObj = Wlc_ObjId(p, pObj);
    int iNum = Wlc_ObjCopy( p, iObj );
    assert( iObj == Vec_IntEntry(vMemObjs, iNum) );
    assert( iFrame >= 0 );
    if ( Wlc_ObjIsPi(pObj) ) 
        Vec_IntPush( vRes, (iObj << 11) | (iFrame << 1) );
    else if ( Wlc_ObjIsCi(pObj) && iFrame == 0 ) 
    {
        int PiId = Vec_IntEntry(p->vInits, Wlc_ObjCiId(pObj)-Wlc_NtkPiNum(p) );
        int iPiObj = Wlc_ObjId( p, Wlc_NtkPi(p, PiId) );
        Vec_IntPush( vRes, (iPiObj << 11) | (iFrame << 1) );
    }
    else if ( Wlc_ObjIsCi(pObj) )
        Wlc_NtkTrace_rec( p, Wlc_ObjFo2Fi(p, pObj), iFrame - 1, vMemObjs, vValues, ValueA, vRes );
    else if ( Wlc_ObjType(pObj) == WLC_OBJ_BUF )
        Wlc_NtkTrace_rec( p, Wlc_ObjFanin0(p, pObj), iFrame, vMemObjs, vValues, ValueA, vRes );
    else if ( Wlc_ObjType(pObj) == WLC_OBJ_MUX )
    {
        int Index = 3*(iFrame*Vec_IntSize(vMemObjs) + iNum);
        int Value = (int)Vec_WrdEntry( vValues, Index );
        assert( Value == 0 || Value == 1 );
        Wlc_NtkTrace_rec( p, Value ? Wlc_ObjFanin2(p, pObj) : Wlc_ObjFanin1(p, pObj), iFrame, vMemObjs, vValues, ValueA, vRes );
        Vec_IntPush( vRes, (iObj << 11) | (iFrame << 1) | Value );
    }
    else if ( Wlc_ObjType(pObj) == WLC_OBJ_WRITE )
    {
        int Index = 3*(iFrame*Vec_IntSize(vMemObjs) + iNum);
        if ( Vec_WrdEntry(vValues, Index + 1) != ValueA ) // address 
            Wlc_NtkTrace_rec( p, Wlc_ObjFanin0(p, pObj), iFrame, vMemObjs, vValues, ValueA, vRes );
        Vec_IntPush( vRes, (iObj << 11) | (iFrame << 1) );
    }
    else assert( 0 );
}
Vec_Int_t * Wlc_NtkTrace( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, int iFrame, Vec_Int_t * vMemObjs, Vec_Wrd_t * vValues )
{
    int iObj = Wlc_ObjId( p, pObj );
    int iNum = Wlc_ObjCopy( p, iObj );
    Vec_Int_t * vTrace = Vec_IntAlloc( 10 );
    assert( Wlc_ObjType(pObj) == WLC_OBJ_READ );
    assert( iObj == Vec_IntEntry(vMemObjs, iNum) );
    Wlc_NtkTrace_rec( p, Wlc_ObjFanin0(p, pObj), iFrame, vMemObjs, vValues, Vec_WrdEntry(vValues, 3*(iFrame*Vec_IntSize(vMemObjs) + iNum) + 1), vTrace );
    Vec_IntPush( vTrace, (iObj << 11) | (iFrame << 1) );
    return vTrace;
}
int Wlc_NtkTraceCheckConfict( Wlc_Ntk_t * p, Vec_Int_t * vTrace, Vec_Int_t * vMemObjs, Vec_Wrd_t * vValues )
{
    Wlc_Obj_t * pObjLast, * pObjFirst; 
    int iObjLast = Vec_IntEntryLast(vTrace) >> 11;
    int iFrmLast =(Vec_IntEntryLast(vTrace) >> 1) & 0x3FF;
    int iNumLast = Wlc_ObjCopy( p, iObjLast );
    int iIndLast = 3*(iFrmLast*Vec_IntSize(vMemObjs) + iNumLast);
    int iObjFirst = Vec_IntEntry(vTrace, 0) >> 11;
    int iFrmFirst =(Vec_IntEntry(vTrace, 0) >> 1) & 0x3FF;
    int iNumFirst = Wlc_ObjCopy( p, iObjFirst );
    int iIndFirst = 3*(iFrmFirst*Vec_IntSize(vMemObjs) + iNumFirst);
    assert( Vec_IntSize(vTrace) >= 2 );
    assert( iObjLast  == Vec_IntEntry(vMemObjs, iNumLast) );
    assert( iObjFirst == Vec_IntEntry(vMemObjs, iNumFirst) );
    pObjLast  = Wlc_NtkObj(p, iObjLast);
    pObjFirst = Wlc_NtkObj(p, iObjFirst);
    assert( Wlc_ObjType(pObjLast) == WLC_OBJ_READ );
    assert( Wlc_ObjType(pObjFirst) == WLC_OBJ_WRITE || Wlc_ObjIsPi(pObjFirst) );
    if ( Wlc_ObjIsPi(pObjFirst) )
        return 0;
    assert( Vec_WrdEntry(vValues, iIndLast + 1) == Vec_WrdEntry(vValues, iIndFirst + 1) ); // equal addr
    return Vec_WrdEntry(vValues, iIndLast + 2) != Vec_WrdEntry(vValues, iIndFirst + 2); // diff data
}
Vec_Int_t * Wlc_NtkFindConflict( Wlc_Ntk_t * p, Vec_Int_t * vMemObjs, Vec_Wrd_t * vValues, int nFrames )
{
    Wlc_Obj_t * pObj; int i, f, Entry;
    Vec_Wec_t * vTraces = Vec_WecAlloc( 100 );
    Vec_Int_t * vTrace, * vTrace1, * vTrace2;
    assert( 3 * nFrames * Vec_IntSize(vMemObjs) == Vec_WrdSize(vValues) );
    Wlc_NtkCleanCopy( p );
    Vec_IntForEachEntry( vMemObjs, Entry, i )
        Wlc_ObjSetCopy( p, Entry, i );
    for ( f = 0; f < nFrames; f++ )
    {
        Wlc_NtkForEachObjVec( vMemObjs, p, pObj, i )
        {
            if ( Wlc_ObjType(pObj) != WLC_OBJ_READ )
                continue;
            vTrace = Wlc_NtkTrace( p, pObj, f, vMemObjs, vValues );
            if ( Wlc_NtkTraceCheckConfict( p, vTrace, vMemObjs, vValues ) )
            {
                Vec_WecFree( vTraces );
                return vTrace;
            }
            Vec_WecPushLevel( vTraces );
            Vec_IntAppend( Vec_WecEntryLast(vTraces), vTrace );
            Vec_IntFree( vTrace );
        }
    }
    // check if there are any common read addresses
    Vec_WecForEachLevel( vTraces, vTrace1, i )
    Vec_WecForEachLevelStop( vTraces, vTrace2, f, i )
        if ( Vec_IntEntry(vTrace1, 0) == Vec_IntEntry(vTrace2, 0) )
        {
            int iObj1 = Vec_IntEntryLast(vTrace1) >> 11;
            int iFrm1 =(Vec_IntEntryLast(vTrace1) >> 1) & 0x3FF;
            int iNum1 = Wlc_ObjCopy( p, iObj1 );
            int iInd1 = 3*(iFrm1*Vec_IntSize(vMemObjs) + iNum1);

            int iObj2 = Vec_IntEntryLast(vTrace2) >> 11;
            int iFrm2 =(Vec_IntEntryLast(vTrace2) >> 1) & 0x3FF;
            int iNum2 = Wlc_ObjCopy( p, iObj2 );
            int iInd2 = 3*(iFrm2*Vec_IntSize(vMemObjs) + iNum2);

            assert( iObj1 == Vec_IntEntry(vMemObjs, iNum1) );
            assert( iObj2 == Vec_IntEntry(vMemObjs, iNum2) );
            if ( Vec_WrdEntry(vValues, iInd1 + 1) == Vec_WrdEntry(vValues, iInd2 + 1) &&  // equal address
                 Vec_WrdEntry(vValues, iInd1 + 2) != Vec_WrdEntry(vValues, iInd2 + 2) )   // diff data
            {
                vTrace = Vec_IntAlloc( 100 );
                Vec_IntPush( vTrace, Vec_IntPop(vTrace1) );
                Vec_IntForEachEntryStart( vTrace1, Entry, i, 1 )
                    Vec_IntPushUnique( vTrace, Entry );
                Vec_IntForEachEntryStart( vTrace2, Entry, i, 1 )
                    Vec_IntPushUnique( vTrace, Entry );
                Vec_WecFree( vTraces );
                return vTrace;
            }
        }
    Vec_WecFree( vTraces );
    return NULL;
}
void Wlc_NtkPrintConflict( Wlc_Ntk_t * p, Vec_Int_t * vTrace )
{
    int Entry, i;
    printf( "Memory semantics failure trace:\n" );
    Vec_IntForEachEntry( vTrace, Entry, i )
        printf( "%3d: entry %9d : obj %5d with name %16s in frame %d\n", i, Entry, Entry >> 11, Wlc_ObjName(p, Entry>>11), (Entry >> 1) & 0x3FF );
}
void Wlc_NtkPrintCex( Wlc_Ntk_t * p, Wlc_Ntk_t * pAbs, Abc_Cex_t * pCex )
{
    Wlc_Obj_t * pObj; int i, k, f, nBits = pCex ? pCex->nRegs : 0;
    if ( pCex == NULL )
    {
        printf( "The CEX is NULL.\n" );
        return;
    }
    for ( f = 0; f <= pCex->iFrame; f++ )
    {
        printf( "Frame%02d ", f );
        Wlc_NtkForEachPi( pAbs, pObj, i )
        {
            printf( "PI%d:", i );
            //printf( "%d(%d):", nBits, Wlc_ObjRange(pObj) );
            for ( k = 0; k < Wlc_ObjRange(pObj); k++ )
                printf( "%d", Abc_InfoHasBit(pCex->pData,  nBits++) );
            printf( " " );
        }
        printf( "FF:" );
        for ( k = 0; nBits < pCex->nPis; k++ )
            printf( "%d", Abc_InfoHasBit(pCex->pData, nBits++) );
        printf( "\n" );
    }

}

/**Function*************************************************************

  Synopsis    [Perform abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wlc_Ntk_t * Wlc_NtkMemAbstractTest( Wlc_Ntk_t * p )
{
    int iFirstMemPi, iFirstCi, iFirstMemCi, nDcBits;
    Vec_Int_t * vRefine;
    Vec_Int_t * vMemObjs    = Wlc_NtkCollectMemory( p, 0 );
    Vec_Int_t * vMemFanins  = Wlc_NtkCollectMemFanins( p, vMemObjs );

    Vec_Wec_t * vRefines    = Vec_WecAlloc( 100 );
    Vec_Int_t * vNodeFrames = Vec_IntAlloc( 100 );
    Wlc_Ntk_t * pNewFull;

    vRefine     = Vec_WecPushLevel(vRefines);
    Vec_IntPush( vRefine,  (11 << 11) | 0 );
    Vec_IntPush( vRefine,  (10 << 11) | 0 );
    Vec_IntPush( vRefine,  ( 8 << 11) | 0 );
    Vec_IntPush( vRefine,  ( 9 << 11) | 0 );
    Wlc_NtkAbsAddToNodeFrames( vNodeFrames, vRefine );

//    pNewFull    = Wlc_NtkAbstractMemory( p, vMemObjs, vMemFanins, &iFirstMemPi, &iFirstCi, &iFirstMemCi, NULL, NULL );
    pNewFull    = Wlc_NtkAbstractMemory( p, vMemObjs, NULL, &iFirstMemPi, &iFirstCi, &iFirstMemCi, vRefines, vNodeFrames );

    Vec_WecFree( vRefines );
    Vec_IntFree( vNodeFrames );

    nDcBits     = Wlc_CountDcs( pNewFull->pInits );
    printf( "iFirstMemPi = %d  iFirstCi = %d  iFirstMemCi = %d  nDcBits = %d\n", iFirstMemPi, iFirstCi, iFirstMemCi, nDcBits );

    Vec_IntFreeP( &vMemObjs );
    Vec_IntFreeP( &vMemFanins );
    return pNewFull;
}

int Wlc_NtkMemAbstract( Wlc_Ntk_t * p, int nIterMax, int fDumpAbs, int fPdrVerbose, int fVerbose )
{
    abctime clk = Abc_Clock();
    Wlc_Ntk_t * pNewFull, * pNew; Aig_Man_t * pAig, * pTempAig;
    Gia_Man_t * pAbsFull, * pAbs;
    Abc_Cex_t * pCex = NULL;  Vec_Wrd_t * vValues = NULL; 
    Vec_Wec_t * vRefines = Vec_WecAlloc( 100 );
    Vec_Int_t * vNodeFrames = Vec_IntAlloc( 100 );
    Vec_Int_t * vMemObjs, * vMemFanins, * vFirstTotal, * vRefine; 
    int RetValue = -1, iFirstMemPi, iFirstCi, iFirstMemCi, nDcBits, nIters;

    vMemObjs    = Wlc_NtkCollectMemory( p, 0 );
    vMemFanins  = Wlc_NtkCollectMemFanins( p, vMemObjs );
    pNewFull    = Wlc_NtkAbstractMemory( p, vMemObjs, vMemFanins, &iFirstMemPi, &iFirstCi, &iFirstMemCi, NULL, NULL );
    nDcBits     = Wlc_CountDcs( pNewFull->pInits );
    vFirstTotal = Wlc_NtkDeriveFirstTotal( p, vMemObjs, vMemFanins, iFirstMemPi, nDcBits + iFirstMemCi, fVerbose );

    pAbsFull    = Wlc_NtkBitBlast( pNewFull, NULL );
    assert( Gia_ManPiNum(pAbsFull) == iFirstCi + nDcBits );
    Wlc_NtkFree( pNewFull );

    // perform abstraction
    for ( nIters = 0; nIters < nIterMax; nIters++ )
    {
        // set up parameters to run PDR
        Pdr_Par_t PdrPars, * pPdrPars = &PdrPars;
        Pdr_ManSetDefaultParams( pPdrPars );
        pPdrPars->fUseAbs    = 0;   // use 'pdr -t'  (on-the-fly abstraction)
        pPdrPars->fVerbose   = fVerbose;

        // derive specific abstraction
        pNew = Wlc_NtkAbstractMemory( p, vMemObjs, NULL, &iFirstMemPi, &iFirstCi, &iFirstMemCi, vRefines, vNodeFrames );
        pAbs = Wlc_NtkBitBlast( pNew, NULL );
        // simplify the AIG
        //pAbs = Gia_ManSeqStructSweep( pGiaTemp = pAbs, 1, 1, 0 );
        //Gia_ManStop( pGiaTemp );

        // roll in the constraints
        pAig = Gia_ManToAigSimple( pAbs );
        Gia_ManStop( pAbs );
        pAig->nConstrs = 1;
        pAig = Saig_ManDupFoldConstrsFunc( pTempAig = pAig, 0, 0, 1 );
        Aig_ManStop( pTempAig );
        pAbs = Gia_ManFromAigSimple( pAig );
        Aig_ManStop( pAig );

        // try to prove abstracted GIA by converting it to AIG and calling PDR
        pAig = Gia_ManToAigSimple( pAbs );
        RetValue = Pdr_ManSolve( pAig, pPdrPars );
        pCex = pAig->pSeqModel; pAig->pSeqModel = NULL;
        Aig_ManStop( pAig );

        if ( fVerbose )
            printf( "\nITERATIONS %d:\n", nIters );
        if ( fVerbose )
            Wlc_NtkPrintCex( p, pNew, pCex );
        Wlc_NtkFree( pNew );

        if ( fDumpAbs )
        {
            char * pFileName = "mem_abs.aig";
            Gia_AigerWrite( pAbs, pFileName, 0, 0, 0 );
            printf( "Iteration %3d: Dumped abstraction in file \"%s\" after finding CEX in frame %d.\n", nIters, pFileName, pCex ? pCex->iFrame : -1 );
        }

        // check if proved or undecided
        if ( pCex == NULL ) 
        {
            assert( RetValue );
            Gia_ManStop( pAbs );
            break;
        }

        // analyze counter-example
        vValues = Wlc_NtkConvertCex( vFirstTotal, pAbsFull, pCex, fVerbose );
        Gia_ManStop( pAbs );
        vRefine = Wlc_NtkFindConflict( p, vMemObjs, vValues, pCex->iFrame + 1 );
        Vec_WrdFree( vValues );
        if ( vRefine == NULL ) // cannot refine
            break;
        Abc_CexFreeP( &pCex );

        // save refinement for the future
        if ( fVerbose )
            Wlc_NtkPrintConflict( p, vRefine );
        Vec_WecPushLevel( vRefines );
        Vec_IntAppend( Vec_WecEntryLast(vRefines), vRefine );
        Wlc_NtkAbsAddToNodeFrames( vNodeFrames, vRefine );
        Vec_IntFree( vRefine );
    }
    // cleanup
    Gia_ManStop( pAbsFull );
    Vec_WecFree( vRefines );
    Vec_IntFreeP( &vMemObjs );
    Vec_IntFreeP( &vMemFanins );
    Vec_IntFreeP( &vFirstTotal );
    Vec_IntFreeP( &vNodeFrames );

    // report the result
    if ( fVerbose )
        printf( "\n" );
    printf( "Abstraction " );
    if ( RetValue == 0 && pCex )
        printf( "resulted in a real CEX in frame %d", pCex->iFrame );
    else if ( RetValue == 1 )
        printf( "is successfully proved" );
    else 
        printf( "timed out" );
    printf( " after %d iterations. ", nIters );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    Abc_CexFreeP( &pCex ); // return CEX in the future
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Collect visited objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wlc_NtkExploreMem2_rec( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, Vec_Int_t * vCollect, int nFrames )
{
    int k, iFanin, nVisited = 0;
    if ( pObj->Mark == 0 )
        return 0;
    if ( Wlc_ObjType(pObj) == WLC_OBJ_PI || (Wlc_ObjType(pObj) == WLC_OBJ_FO && nFrames == 0) )
    {
        Vec_IntPushTwo( vCollect, Wlc_ObjId(p, pObj), nFrames );
        return 1;
    }
    if ( Wlc_ObjType(pObj) == WLC_OBJ_FO )
        return Wlc_NtkExploreMem2_rec( p, Wlc_ObjFo2Fi(p, pObj), vCollect, nFrames-1 );
    Wlc_ObjForEachFanin( pObj, iFanin, k )
        nVisited += Wlc_NtkExploreMem2_rec( p, Wlc_NtkObj(p, iFanin), vCollect, nFrames );
    Vec_IntPushTwo( vCollect, Wlc_ObjId(p, pObj), nFrames );
    return nVisited + 1;
}
void Wlc_NtkExploreMem2( Wlc_Ntk_t * p, int nFrames )
{
    Wlc_Obj_t * pObj; int i, k, Entry, Frame, nVisited;
    Vec_Int_t * vCollect = Vec_IntStart( 1000 );
    Vec_Int_t * vMemObjsClean = Wlc_NtkCollectMemory( p, 1 );
    Wlc_NtkCleanMarks( p );
    Wlc_NtkForEachObjVec( vMemObjsClean, p, pObj, i )
        pObj->Mark = 1;
    Wlc_NtkForEachObjVec( vMemObjsClean, p, pObj, i )
    {
        if ( Wlc_ObjType(pObj) != WLC_OBJ_READ )
            continue;
        Vec_IntClear( vCollect );
        nVisited = Wlc_NtkExploreMem2_rec( p, pObj, vCollect, nFrames );
        printf( "Obj %6d : ", Wlc_ObjId(p, pObj) );
        printf( "Visit = %6d  ", nVisited );
        printf( "Pair = %6d  ", Vec_IntSize(vCollect)/2 );
        if ( Vec_IntSize(vCollect)/2 < 10 )
            Vec_IntForEachEntryDouble( vCollect, Entry, Frame, k )
                printf( "%d(%d) ", Entry, Frame );
        printf( "\n" );
    }
    Vec_IntFree( vMemObjsClean );
    Vec_IntFree( vCollect );
    Wlc_NtkCleanMarks( p );
}

/**Function*************************************************************

  Synopsis    [Collect memory flops reachable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_NtkExploreMem_rec( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, Vec_Int_t * vCollect, int nFrames )
{
    int k, iFanin;
    if ( pObj->Mark == 0 )
        return;
    if ( Wlc_ObjType(pObj) == WLC_OBJ_PI || (Wlc_ObjType(pObj) == WLC_OBJ_FO && nFrames == 0) )
    {
        Vec_IntPushUnique( vCollect, Wlc_ObjId(p, pObj) );
        return;
    }
    if ( Wlc_ObjType(pObj) == WLC_OBJ_FO )
    {
        Wlc_NtkExploreMem_rec( p, Wlc_ObjFo2Fi(p, pObj), vCollect, nFrames-1 );
        return;
    }
    Wlc_ObjForEachFanin( pObj, iFanin, k )
        Wlc_NtkExploreMem_rec( p, Wlc_NtkObj(p, iFanin), vCollect, nFrames );
}
void Wlc_NtkExploreMem( Wlc_Ntk_t * p, int nFrames )
{
    Wlc_Obj_t * pObj; int i, k, iObj;
    Vec_Int_t * vCollect = Vec_IntStart( 1000 );
    Vec_Int_t * vMemObjsClean = Wlc_NtkCollectMemory( p, 1 );
    Wlc_NtkCleanMarks( p );
    Wlc_NtkForEachObjVec( vMemObjsClean, p, pObj, i )
        pObj->Mark = 1;
    Wlc_NtkForEachObjVec( vMemObjsClean, p, pObj, i )
    {
        if ( Wlc_ObjType(pObj) != WLC_OBJ_READ )
            continue;
        Vec_IntClear( vCollect );
        Wlc_NtkExploreMem_rec( p, pObj, vCollect, nFrames );
        printf( "Read port %6d : ", Wlc_ObjId(p, pObj) );
        printf( "Inputs = %6d  ", Vec_IntSize(vCollect) );
        Vec_IntForEachEntry( vCollect, iObj, k )
            printf( "%d(%s) ", iObj, Wlc_ObjName(p, iObj) );
        printf( "\n" );
    }
    Vec_IntFree( vMemObjsClean );
    Vec_IntFree( vCollect );
    Wlc_NtkCleanMarks( p );
}


/**Function*************************************************************

  Synopsis    [For each READ find reachable CIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Wlc_NtkFindReachablePiFo( Wlc_Ntk_t * p, Vec_Int_t * vMemObjsClean, int nFrames )
{
    Vec_Int_t * vRes = Vec_IntAlloc( 100 );
    Wlc_Obj_t * pObjR, * pObjI, * pObj; 
    int i, j, k, f, fFanin;
    Wlc_NtkForEachObj( p, pObj, i )
        pObj->Mark2 = 0;
    // go through read ports
    Wlc_NtkForEachObjVec( vMemObjsClean, p, pObjR, i ) if ( Wlc_ObjIsRead(pObjR) )
    {
        // go through memory CIs
        Wlc_NtkForEachObjVec( vMemObjsClean, p, pObjI, j ) if ( Wlc_ObjIsCi(pObjI) )
        {
            // propagate bit from CI to READ
            pObjI->Mark2 = 1;
            Wlc_NtkForEachObjVec( vMemObjsClean, p, pObj, k )
            {
                if ( pObj == pObjI )
                    continue;
                assert( pObj->Mark2 == 0 );
                Wlc_ObjForEachFanin( pObj, fFanin, f )
                    pObj->Mark2 |= Wlc_NtkObj(p, fFanin)->Mark2;
            }
            if ( pObjR->Mark2 )
                Vec_IntPushThree( vRes, Wlc_ObjId(p, pObjR), Wlc_ObjId(p, pObjI), -1 );
            Wlc_NtkForEachObjVec( vMemObjsClean, p, pObj, k )
                pObj->Mark2 = 0;
        }
    }
    Wlc_NtkForEachObj( p, pObj, i )
        assert( pObj->Mark2 == 0 );
    return vRes;
}
Vec_Int_t * Wlc_NtkExtractCisForThisRead( Vec_Int_t * vReachReadCi, int iRead )
{
    int k;
    Vec_Int_t * vRes = Vec_IntAlloc( 100 );
    for ( k = 0; k < Vec_IntSize(vReachReadCi)/3; k++ )
    {
        if ( iRead != Vec_IntEntry( vReachReadCi, 3*k ) )
            continue;
        Vec_IntPush( vRes, Vec_IntEntry( vReachReadCi, 3*k+1 ) );
        Vec_IntPush( vRes, Vec_IntEntry( vReachReadCi, 3*k+2 ) );
    }
    return vRes;
}
Vec_Int_t * Wlc_NtkCollectOneType( Wlc_Ntk_t * p, Vec_Int_t * vMemObjsClean, int Type1, int Type2 )
{
    Wlc_Obj_t * pObj; int i;
    Vec_Int_t * vRes = Vec_IntAlloc( 100 );
    Wlc_NtkForEachObjVec( vMemObjsClean, p, pObj, i )
        if ( Wlc_ObjType(pObj) == Type1 || Wlc_ObjType(pObj) == Type2 )
            Vec_IntPush( vRes, Wlc_ObjId(p, pObj) );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Create memory constraints.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_NtkCreateMemoryConstr( Wlc_Ntk_t * pNew, Wlc_Ntk_t * p, Vec_Int_t * vMemObjsClean, Vec_Int_t * vReachReadCi )
{
    Vec_Int_t * vReads  = Wlc_NtkCollectOneType( p, vMemObjsClean, WLC_OBJ_READ, -1 );
    Vec_Int_t * vObjCis = Wlc_NtkCollectOneType( p, vMemObjsClean, WLC_OBJ_PI, WLC_OBJ_FO );
    Vec_Int_t * vFanins = Vec_IntAlloc( 10 );
    Wlc_Obj_t * pObjR, * pObj, * pCond; 
    int i, k, iObjCi, iObjNew = -1;
    // go through read ports
    Wlc_NtkForEachObjVec( vReads, p, pObjR, i )
    {
        // ABC_READ  ( .mem_in(mem_fi1), .addr(raddr), .data(read1) ) ;
        // ABC_WRITE ( .mem_in(mem_fo1), .addr(waddr), .data(data), .mem_out(mem_fi1) ) ;
        // initialize CI related to the read port
        Vec_Int_t * vLocalCis = Wlc_NtkExtractCisForThisRead( vReachReadCi, Wlc_ObjId(p, pObjR) );
        Wlc_NtkForEachObjVec( vObjCis, p, pObj, k )
            Wlc_ObjSetCopy( p, Wlc_ObjId(p, pObj), -1 );
        Vec_IntForEachEntryDouble( vLocalCis, iObjCi, iObjNew, k )
            Wlc_ObjSetCopy( p, iObjCi, iObjNew );
        Vec_IntFree( vLocalCis );
        // implement the nodes
        Wlc_NtkForEachObjVec( vMemObjsClean, p, pObj, k )
        {
            if ( Wlc_ObjIsRead(pObj) || Wlc_ObjIsCi(pObj) )
                continue;
            Wlc_ObjSetCopy( p, Wlc_ObjId(p, pObj), -1 );
            if ( Wlc_ObjIsWrite(pObj) )
            {
                if ( Wlc_ObjCopy(p, Wlc_ObjFaninId0(pObj)) == -1 )
                    continue;
                if ( Wlc_ObjRange(pObjR) != Wlc_ObjRange(Wlc_ObjFanin2(p, pObj)) )
                    continue;
                // create equality
                pCond = Wlc_NtkObj( pNew, Wlc_ObjAlloc(pNew, WLC_OBJ_COMP_EQU, 0, 0, 0) );
                Vec_IntFillTwo( vFanins, 2, Wlc_ObjCopy(p, Wlc_ObjFaninId1(pObjR)), Wlc_ObjCopy(p, Wlc_ObjFaninId1(pObj)) );
                //printf( "%d : ", Wlc_ObjId(p,pObj) ), Vec_IntPrint( vFanins );
                Wlc_ObjAddFanins( pNew, pCond, vFanins );
                // create MUX
                iObjNew = Wlc_ObjAlloc( pNew, WLC_OBJ_MUX, 0, Wlc_ObjRange(pObjR)-1, 0 );
                Vec_IntFill( vFanins, 1, Wlc_ObjId(pNew, pCond) );
                Vec_IntPush( vFanins, Wlc_ObjCopy(p, Wlc_ObjFaninId2(pObj)) );
                Vec_IntPush( vFanins, Wlc_ObjCopy(p, Wlc_ObjFaninId0(pObj)) );
                Wlc_ObjAddFanins( pNew, Wlc_NtkObj(pNew, iObjNew), vFanins );
            }
            else if ( Wlc_ObjIsMux(pObj) )
            {
                int iFanin0New = Wlc_ObjCopy(p, Wlc_ObjFaninId1(pObj));
                int iFanin1New = Wlc_ObjCopy(p, Wlc_ObjFaninId2(pObj));
                if ( iFanin0New == -1 || iFanin1New == -1 )
                    continue;
                //Wlc_NtkPrintNode( pNew, Wlc_NtkObj(pNew, iFanin0New) );
                assert( Wlc_ObjRange(pObjR) == Wlc_ObjRange(Wlc_NtkObj(pNew, iFanin0New)) );
                assert( Wlc_ObjRange(pObjR) == Wlc_ObjRange(Wlc_NtkObj(pNew, iFanin1New)) );
                iObjNew = Wlc_ObjAlloc( pNew, WLC_OBJ_MUX, 0, Wlc_ObjRange(pObjR)-1, 0 );
                Vec_IntFill( vFanins, 1, Wlc_ObjCopy(p, Wlc_ObjFaninId0(pObj)) );
                Vec_IntPush( vFanins, iFanin0New );
                Vec_IntPush( vFanins, iFanin1New );
                Wlc_ObjAddFanins( pNew, Wlc_NtkObj(pNew, iObjNew), vFanins );
            }
            else if ( Wlc_ObjIsBuf(pObj) )
            {
                iObjNew = Wlc_ObjCopy( p, Wlc_ObjFaninId0(pObj) );
                if ( iObjNew == -1 )
                    continue;
            }
            else assert( 0 );
            Wlc_ObjSetCopy( p, Wlc_ObjId(p, pObj), iObjNew );
        }
        // extract fanin
        iObjNew = Wlc_ObjCopy( p, Wlc_ObjFaninId0(pObjR) );
        assert( iObjNew != -1 );
        Vec_IntFill( vFanins, 1, iObjNew );
        // add it as buffer fanin
        iObjNew = Wlc_ObjCopy( p, Wlc_ObjId(p, pObjR) );
        assert( Wlc_NtkObj(pNew, iObjNew)->Type == WLC_OBJ_BUF );
        Wlc_ObjAddFanins( pNew, Wlc_NtkObj(pNew, iObjNew), vFanins );
    }
    Wlc_NtkForEachObjVec( vObjCis, p, pObj, k )
        Wlc_ObjSetCopy( p, Wlc_ObjId(p, pObj), -1 );
    Vec_IntFree( vFanins );
    Vec_IntFree( vReads );
    Vec_IntFree( vObjCis );
}

/**Function*************************************************************

  Synopsis    [Perform abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wlc_Ntk_t * Wlc_NtkAbstractMem( Wlc_Ntk_t * p, int nFrames, int fVerbose )
{
    Vec_Int_t * vMemObjsAll   = Wlc_NtkCollectMemory( p, 0 );
    Vec_Int_t * vMemObjsClean = Wlc_NtkCollectMemory( p, 1 );
    Vec_Int_t * vReachReadCi  = Wlc_NtkFindReachablePiFo( p, vMemObjsClean, nFrames );
    Wlc_Ntk_t * pNew, * pTemp;
    Wlc_Obj_t * pObj;
    Vec_Int_t * vFanins  = Vec_IntAlloc( 100 );
    int i, Po0, Po1, iObjNew;

    assert( nFrames == 0 );

    // mark memory nodes
    Wlc_NtkCleanMarks( p );
    Wlc_NtkForEachObjVec( vMemObjsAll, p, pObj, i )
        pObj->Mark = 1;

    // start new network
    Wlc_NtkCleanCopy( p );
    pNew = Wlc_NtkAlloc( p->pName, p->nObjsAlloc + Vec_IntSize(vMemObjsClean) * nFrames * 10 );
    pNew->fSmtLib   = p->fSmtLib;
    pNew->fAsyncRst = p->fAsyncRst;
    pNew->fMemPorts = p->fMemPorts;
    pNew->fEasyFfs  = p->fEasyFfs;
    pNew->vInits    = Vec_IntAlloc( 100 );

    // duplicate PIs
    Wlc_NtkForEachPi( p, pObj, i )
        if ( !pObj->Mark )
            Wlc_ObjDup( pNew, p, Wlc_ObjId(p, pObj), vFanins );

    // create new PIs
    assert( Vec_IntSize(vReachReadCi) % 3 == 0 );
    for ( i = 0; i < Vec_IntSize(vReachReadCi)/3; i++ )
    {
        pObj = Wlc_NtkObj( p, Vec_IntEntry( vReachReadCi, 3*i ) );
        assert( Wlc_ObjIsRead(pObj) );
        iObjNew = Wlc_NtkDupOneObject( pNew, p, pObj, WLC_OBJ_PI, vFanins );
        Vec_IntWriteEntry( vReachReadCi, 3*i+2, iObjNew );
    }
    //Vec_IntPrint( vReachReadCi );

    // duplicate flop outputs
    Wlc_NtkForEachCi( p, pObj, i )
        if ( !Wlc_ObjIsPi(pObj) && !pObj->Mark )
        {
            Wlc_ObjDup( pNew, p, Wlc_ObjId(p, pObj), vFanins );
            Vec_IntPush( pNew->vInits, Vec_IntEntry(p->vInits, i-Wlc_NtkPiNum(p)) );
        }

/*
    // create flops for memory objects
    Wlc_NtkForEachObjVec( vMemFanins, p, pObj, i )
        for ( k = 0; k < nFrames; k++ )
            Wlc_NtkDupOneObject( pNew, p, pObj, WLC_OBJ_FO, vFanins );
*/

    // create buffers for read ports
    Wlc_NtkForEachObjVec( vMemObjsClean, p, pObj, i )
    {
        if ( !Wlc_ObjIsRead(pObj) )
            continue;
        iObjNew = Wlc_ObjAlloc( pNew, WLC_OBJ_BUF, pObj->Signed, pObj->End, pObj->Beg );
        Wlc_ObjSetCopy( p, Wlc_ObjId(p, pObj), iObjNew );
    }

    // duplicate objects
    Wlc_NtkForEachObj( p, pObj, i )
        if ( !Wlc_ObjIsCi(pObj) && !pObj->Mark )
            Wlc_ObjDup( pNew, p, Wlc_ObjId(p, pObj), vFanins );

    // create memory constraints
    Wlc_NtkCreateMemoryConstr( pNew, p, vMemObjsClean, vReachReadCi );

    // create outpus
    if ( Vec_IntSize(&p->vPoPairs) )
    {
        // create miter for the PO pairs
        Vec_IntForEachEntryDouble( &p->vPoPairs, Po0, Po1, i )
        {
            int iCopy0 = Wlc_ObjCopy(p, Wlc_ObjId(p, Wlc_NtkPo(p, Po0)));
            int iCopy1 = Wlc_ObjCopy(p, Wlc_ObjId(p, Wlc_NtkPo(p, Po1)));
            int iObj = Wlc_ObjAlloc( pNew, WLC_OBJ_COMP_NOTEQU, 0, 0, 0 );
            Wlc_Obj_t * pObjNew = Wlc_NtkObj( pNew, iObj );
            Vec_IntFillTwo( vFanins, 1, iCopy0, iCopy1 );
            Wlc_ObjAddFanins( pNew, pObjNew, vFanins );
            Wlc_ObjSetCo( pNew, pObjNew, 0 );
        }
        printf( "Memory abstraction created %d miter outputs.\n", Wlc_NtkPoNum(pNew) );
        Wlc_NtkForEachCo( p, pObj, i )
            if ( pObj->fIsFi && !pObj->Mark )
                Wlc_ObjSetCo( pNew, Wlc_ObjCopyObj(pNew, p, pObj), pObj->fIsFi );
    }
    else
    {
        // duplicate POs
        Wlc_NtkForEachPo( p, pObj, i )
            if ( !pObj->Mark )
                Wlc_ObjSetCo( pNew, Wlc_ObjCopyObj(pNew, p, pObj), pObj->fIsFi );
        // duplicate FIs
        Wlc_NtkForEachCo( p, pObj, i )
            if ( !Wlc_ObjIsPo(pObj) && !pObj->Mark )
                Wlc_ObjSetCo( pNew, Wlc_ObjCopyObj(pNew, p, pObj), pObj->fIsFi );
    }

/*
    // create new flop inputs
    Wlc_NtkForEachObjVec( vMemFanins, p, pObj, i )
        for ( k = 0; k < nFrames; k++ )
            Wlc_NtkDupOneBuffer( pNew, p, pObj, Wlc_ObjCopy(p, Wlc_ObjId(p, pObj))-(nFrames-1-k), vFanins, 1 );
*/

    // append init states
    pNew->pInits = Wlc_PrsConvertInitValues( pNew );
    if ( p->pSpec )
        pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    if ( Wlc_NtkHasNameId(p) )
        Wlc_NtkTransferNames( pNew, p );
    Vec_IntFree( vFanins );
    Vec_IntFree( vMemObjsAll );
    Vec_IntFree( vMemObjsClean );
    Vec_IntFree( vReachReadCi );
    Wlc_NtkCleanMarks( p );
    // derive topological order
//printf( "Objects before = %d.\n", Wlc_NtkObjNum(pNew) );
    pNew = Wlc_NtkDupDfs( pTemp = pNew, 0, 1 );
    Wlc_NtkFree( pTemp );
//printf( "Objects after = %d.\n", Wlc_NtkObjNum(pNew) );
    return pNew;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

