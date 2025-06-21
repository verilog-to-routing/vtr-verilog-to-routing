/**CFile****************************************************************

  FileName    [gia.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Structural detection of enables, sets and resets.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: gia.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Collects the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_CollectSuper_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSuper )
{
    // if the new node is complemented or a PI, another gate begins
    if ( Gia_IsComplement(pObj) || Gia_ObjIsCi(pObj) )
    {
        Vec_IntPushUnique( vSuper, Gia_ObjId(p, Gia_Regular(pObj)) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    // go through the branches
    Gia_CollectSuper_rec( p, Gia_ObjChild0(pObj), vSuper );
    Gia_CollectSuper_rec( p, Gia_ObjChild1(pObj), vSuper );
}

/**Function*************************************************************

  Synopsis    [Collects the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_CollectSuper( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSuper )
{
    assert( !Gia_IsComplement(pObj) );
    Vec_IntClear( vSuper );
//    Gia_CollectSuper_rec( p, pObj, vSuper );
    if ( Gia_ObjIsAnd(pObj) )
    {
        Vec_IntPushUnique( vSuper, Gia_ObjId(p, Gia_ObjFanin0(pObj)) );
        Vec_IntPushUnique( vSuper, Gia_ObjId(p, Gia_ObjFanin1(pObj)) );
    }
    else
        Vec_IntPushUnique( vSuper, Gia_ObjId(p, Gia_Regular(pObj)) );

}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintSignals( Gia_Man_t * p, int * pFreq, char * pStr )
{
    Vec_Int_t * vObjs;
    int i, Counter = 0, nTotal = 0;
    vObjs = Vec_IntAlloc( 100 );
    for ( i = 0; i < Gia_ManObjNum(p); i++ )
        if ( pFreq[i] > 1 )
        {
            nTotal += pFreq[i];
            Counter++;
        }
    printf( "%s (total = %d  driven = %d)\n", pStr, Counter, nTotal );
    Counter = 0;
    for ( i = 0; i < Gia_ManObjNum(p); i++ )
        if ( pFreq[i] > 10 )
        {
            printf( "%3d :   Obj = %6d   Refs = %6d   Freq = %6d\n", 
                ++Counter, i, Gia_ObjRefNum(p, Gia_ManObj(p,i)), pFreq[i] );
            Vec_IntPush( vObjs, i );
        }
    Vec_IntFree( vObjs );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDetectSeqSignals( Gia_Man_t * p, int fSetReset, int fVerbose )
{
    Vec_Int_t * vSuper;
    Gia_Obj_t * pFlop, * pObjC, * pObj0, * pObj1, * pNode, * pTemp;
    int i, k, Ent, * pSets, * pResets, * pEnables;
    int nHaveSetReset = 0, nHaveEnable = 0;
    assert( Gia_ManRegNum(p) > 0 );
    pSets    = ABC_CALLOC( int, Gia_ManObjNum(p) );
    pResets  = ABC_CALLOC( int, Gia_ManObjNum(p) );
    pEnables = ABC_CALLOC( int, Gia_ManObjNum(p) );
    vSuper   = Vec_IntAlloc( 100 );
    Gia_ManForEachRi( p, pFlop, i )
    {
        pNode = Gia_ObjFanin0(pFlop);
        if ( !Gia_ObjIsAnd(pNode) )
            continue;
        // detect sets/resets
        Gia_CollectSuper( p, pNode, vSuper );
        if ( Gia_ObjFaninC0(pFlop) )
            Vec_IntForEachEntry( vSuper, Ent, k )
                pSets[Ent]++;
        else
            Vec_IntForEachEntry( vSuper, Ent, k )
                pResets[Ent]++;
        // detect enables
        if ( !Gia_ObjIsMuxType(pNode) )
            continue;
        pObjC = Gia_ObjRecognizeMux( pNode, &pObj0, &pObj1 );
        pTemp = Gia_ObjRiToRo( p, pFlop );
        if ( Gia_Regular(pObj0) != pTemp && Gia_Regular(pObj1) != pTemp )
            continue;
        if ( !Gia_ObjFaninC0(pFlop) )
        {
            pObj0 = Gia_Not(pObj0);
            pObj1 = Gia_Not(pObj1);
        }
        if ( Gia_IsComplement(pObjC) )
        {
            pObjC = Gia_Not(pObjC);
            pTemp = pObj0;
            pObj0 = pObj1;
            pObj1 = pTemp;
        }
        // detect controls
//        Gia_CollectSuper( p, pObjC, vSuper );
//        Vec_IntForEachEntry( vSuper, Ent, k )
//            pEnables[Ent]++;
        pEnables[Gia_ObjId(p, pObjC)]++;
        nHaveEnable++;
    }
    Gia_ManForEachRi( p, pFlop, i )
    {
        pNode = Gia_ObjFanin0(pFlop);
        if ( !Gia_ObjIsAnd(pNode) )
            continue;
        // detect sets/resets
        Gia_CollectSuper( p, pNode, vSuper );
        Vec_IntForEachEntry( vSuper, Ent, k )
            if ( pSets[Ent] > 1 || pResets[Ent] > 1 )
            {
                nHaveSetReset++;
                break;
            }
    }
    Vec_IntFree( vSuper );
    ABC_FREE( p->pRefs );
    Gia_ManCreateRefs( p );
    if ( fVerbose )
    {
        printf( "Flops with set/reset = %6d. Flops with enable = %6d.\n", nHaveSetReset, nHaveEnable );
        if ( fSetReset )
        {
            Gia_ManPrintSignals( p, pSets, "Set signals" );
            Gia_ManPrintSignals( p, pResets, "Reset signals" );
        }
        Gia_ManPrintSignals( p, pEnables, "Enable signals" );
    }
    ABC_FREE( p->pRefs );
    ABC_FREE( pSets );
    ABC_FREE( pResets );
    ABC_FREE( pEnables );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManDetectSeqSignalsWithFanout( Gia_Man_t * p, int nFanMax, int fVerbose )
{
    Vec_Int_t * vResult;
    Vec_Int_t * vSuper;
    Gia_Obj_t * pFlop, * pObjC, * pObj0, * pObj1, * pNode, * pTemp;
    int i, k, Ent, * pSets, * pResets, * pEnables;
    int nHaveSetReset = 0, nHaveEnable = 0;
    assert( Gia_ManRegNum(p) > 0 );
    pSets    = ABC_CALLOC( int, Gia_ManObjNum(p) );
    pResets  = ABC_CALLOC( int, Gia_ManObjNum(p) );
    pEnables = ABC_CALLOC( int, Gia_ManObjNum(p) );
    vSuper   = Vec_IntAlloc( 100 );
    Gia_ManForEachRi( p, pFlop, i )
    {
        pNode = Gia_ObjFanin0(pFlop);
        if ( !Gia_ObjIsAnd(pNode) )
            continue;
        // detect sets/resets
        Gia_CollectSuper( p, pNode, vSuper );
        if ( Gia_ObjFaninC0(pFlop) )
            Vec_IntForEachEntry( vSuper, Ent, k )
                pSets[Ent]++;
        else
            Vec_IntForEachEntry( vSuper, Ent, k )
                pResets[Ent]++;
        // detect enables
        if ( !Gia_ObjIsMuxType(pNode) )
            continue;
        pObjC = Gia_ObjRecognizeMux( pNode, &pObj0, &pObj1 );
        pTemp = Gia_ObjRiToRo( p, pFlop );
        if ( Gia_Regular(pObj0) != pTemp && Gia_Regular(pObj1) != pTemp )
            continue;
        if ( !Gia_ObjFaninC0(pFlop) )
        {
            pObj0 = Gia_Not(pObj0);
            pObj1 = Gia_Not(pObj1);
        }
        if ( Gia_IsComplement(pObjC) )
        {
            pObjC = Gia_Not(pObjC);
            pTemp = pObj0;
            pObj0 = pObj1;
            pObj1 = pTemp;
        }
        // detect controls
//        Gia_CollectSuper( p, pObjC, vSuper );
//        Vec_IntForEachEntry( vSuper, Ent, k )
//            pEnables[Ent]++;
        pEnables[Gia_ObjId(p, pObjC)]++;
        nHaveEnable++;
    }
    Gia_ManForEachRi( p, pFlop, i )
    {
        pNode = Gia_ObjFanin0(pFlop);
        if ( !Gia_ObjIsAnd(pNode) )
            continue;
        // detect sets/resets
        Gia_CollectSuper( p, pNode, vSuper );
        Vec_IntForEachEntry( vSuper, Ent, k )
            if ( pSets[Ent] > 1 || pResets[Ent] > 1 )
            {
                nHaveSetReset++;
                break;
            }
    }
    Vec_IntFree( vSuper );
    vResult  = Vec_IntAlloc( 100 );
    for ( i = 1; i < Gia_ManObjNum(p); i++ )
        if ( pSets[i] > nFanMax )
        {
            if ( fVerbose )
                printf( "Adding set signal %d related to %d flops.\n", i, pSets[i] );
            Vec_IntPushUnique( vResult, i );
        }
    for ( i = 1; i < Gia_ManObjNum(p); i++ )
        if ( pResets[i] > nFanMax )
        {
            if ( fVerbose )
                printf( "Adding reset signal %d related to %d flops.\n", i, pResets[i] );
            Vec_IntPushUnique( vResult, i );
        }
    for ( i = 1; i < Gia_ManObjNum(p); i++ )
        if ( pEnables[i] > nFanMax )
        {
            if ( fVerbose )
                printf( "Adding enable signal %d related to %d flops.\n", i, pEnables[i] );
            Vec_IntPushUnique( vResult, i );
        }
    ABC_FREE( pSets );
    ABC_FREE( pResets );
    ABC_FREE( pEnables );
    return vResult;
}


/**Function*************************************************************

  Synopsis    [Transfers attributes from the original one to the final one.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManTransferFrames( Gia_Man_t * pAig, Gia_Man_t * pFrames, int nFrames, Gia_Man_t * pNew, Vec_Int_t * vSigs )
{
    Vec_Int_t * vSigsNew;
    Gia_Obj_t * pObj, * pObjF;
    int k, f;
    vSigsNew = Vec_IntAlloc( 100 );
    Gia_ManForEachObjVec( vSigs, pAig, pObj, k )
    {
        assert( Gia_ObjIsCand(pObj) );
        for ( f = 0; f < nFrames; f++ )
        {
            pObjF = Gia_ManObj( pFrames, Abc_Lit2Var(Gia_ObjCopyF( pAig, f, pObj )) );
            if ( pObjF->Value && ~pObjF->Value )
                Vec_IntPushUnique( vSigsNew, Abc_Lit2Var(pObjF->Value) );
        }
    }
    return vSigsNew;    
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManUnrollInit( Gia_Man_t * p, int nFrames )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj, * pObjRi, * pObjRo;
    int f, i;
    Vec_IntFill( &p->vCopies, nFrames * Gia_ManObjNum(p), -1 );
    pNew = Gia_ManStart( nFrames * Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachRo( p, pObj, i )
        Gia_ObjSetCopyF( p, 0, pObj, 0 );
    for ( f = 0; f < nFrames; f++ )
    {
        Gia_ObjSetCopyF( p, f, Gia_ManConst0(p), 0 );
        Gia_ManForEachPi( p, pObj, i )
            Gia_ObjSetCopyF( p, f, pObj, Gia_ManAppendCi(pNew) );
        Gia_ManForEachAnd( p, pObj, i )
            Gia_ObjSetCopyF( p, f, pObj, Gia_ManHashAnd(pNew, Gia_ObjFanin0CopyF(p, f, pObj), Gia_ObjFanin1CopyF(p, f, pObj)) );
        Gia_ManForEachCo( p, pObj, i )
            Gia_ObjSetCopyF( p, f, pObj, Gia_ObjFanin0CopyF(p, f, pObj) );
        Gia_ManForEachPo( p, pObj, i )
            Gia_ManAppendCo( pNew, Gia_ObjCopyF(p, f, pObj) );
        if ( f == nFrames - 1 )
            break;
        Gia_ManForEachRiRo( p, pObjRi, pObjRo, i )
            Gia_ObjSetCopyF( p, f+1, pObjRo, Gia_ObjCopyF(p, f, pObjRi) );
    }
    Gia_ManHashStop( pNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Unrolls initialized timeframes while cofactoring some vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManUnrollAndCofactor( Gia_Man_t * p, int nFrames, int nFanMax, int fVerbose )
{
    Vec_Int_t * vCofSigs, * vTemp;
    Gia_Man_t * pAig, * pFrames, * pNew;
    // compute initialized timeframes
    pFrames  = Gia_ManUnrollInit( p, nFrames );
    pAig     = Gia_ManCleanup( pFrames );
    // compute and remap set/reset/enable signals
    vCofSigs = Gia_ManDetectSeqSignalsWithFanout( p, nFanMax, fVerbose );
    vCofSigs = Gia_ManTransferFrames( p, pFrames, nFrames, pAig, vTemp = vCofSigs );
    Vec_IntFree( vTemp );
    Gia_ManStop( pFrames );
    Vec_IntErase( &p->vCopies );
    // cofactor all these variables
    pNew = Gia_ManDupCofAllInt( pAig, vCofSigs, fVerbose );
    Vec_IntFree( vCofSigs );
    Gia_ManStop( pAig );
    return pNew;
}



/**Function*************************************************************

  Synopsis    [Transform seq circuits with enables by removing enables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManRemoveEnables2( Gia_Man_t * p )
{
    Gia_Man_t * pNew, * pAux; 
    Gia_Obj_t * pTemp, * pObjC, * pObj0, * pObj1, * pFlopIn, * pFlopOut;
    Gia_Obj_t * pThis, * pNode;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pThis, i )
        pThis->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( p, pThis, i )
        pThis->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pThis), Gia_ObjFanin1Copy(pThis) );
    Gia_ManForEachPo( p, pThis, i )
        pThis->Value = Gia_ObjFanin0Copy(pThis);
    Gia_ManForEachRi( p, pFlopIn, i )
    {
        pNode = Gia_ObjFanin0(pFlopIn);
        if ( !Gia_ObjIsMuxType(pNode) )
        {
            printf( "Cannot recognize enable of flop %d.\n", i );
            continue;
        }
        pObjC = Gia_ObjRecognizeMux( pNode, &pObj1, &pObj0 );
        pFlopOut = Gia_ObjRiToRo( p, pFlopIn );
        if ( Gia_Regular(pObj0) != pFlopOut && Gia_Regular(pObj1) != pFlopOut )
        {
            printf( "Cannot recognize self-loop of enable flop %d.\n", i );
            continue;
        }
        if ( !Gia_ObjFaninC0(pFlopIn) )
        {
            pObj0 = Gia_Not(pObj0);
            pObj1 = Gia_Not(pObj1);
        }
        if ( Gia_IsComplement(pObjC) )
        {
            pObjC = Gia_Not(pObjC);
            pTemp = pObj0;
            pObj0 = pObj1;
            pObj1 = pTemp;
        }
        if ( Gia_Regular(pObj0) == pFlopOut )
        {
//            printf( "FlopIn compl = %d. FlopOut is d0. Complement = %d.\n", 
//                Gia_ObjFaninC0(pFlopIn), Gia_IsComplement(pObj0) );
            pFlopIn->Value = Abc_LitNotCond(Gia_Regular(pObj1)->Value, !Gia_IsComplement(pObj1));
        }
        else if ( Gia_Regular(pObj1) == pFlopOut )
        {
//            printf( "FlopIn compl = %d. FlopOut is d1. Complement = %d.\n", 
//                Gia_ObjFaninC0(pFlopIn), Gia_IsComplement(pObj1) );
            pFlopIn->Value = Abc_LitNotCond(Gia_Regular(pObj0)->Value, !Gia_IsComplement(pObj0));
        }
    }
    Gia_ManForEachCo( p, pThis, i )
        Gia_ManAppendCo( pNew, pThis->Value );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew = Gia_ManCleanup( pAux = pNew );
    Gia_ManStop( pAux );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Transform seq circuits with enables by removing enables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManRemoveEnables( Gia_Man_t * p )
{
    Vec_Ptr_t * vCtrls, * vDatas;
    Vec_Int_t * vFlopClasses;
    Gia_Man_t * pNew, * pAux; 
    Gia_Obj_t * pFlopIn, * pFlopOut, * pDriver, * pFan0, * pFan1, * pCtrl = NULL, * pData, * pObj;
    int i, iClass, fCompl, Counter = 0;
    vCtrls = Vec_PtrAlloc( 100 );
    Vec_PtrPush( vCtrls, NULL );
    vDatas = Vec_PtrAlloc( Gia_ManRegNum(p) );
    vFlopClasses = Vec_IntAlloc( Gia_ManRegNum(p) );
    Gia_ManForEachRi( p, pFlopIn, i )
    {
        fCompl = Gia_ObjFaninC0(pFlopIn);
        pDriver = Gia_ObjFanin0(pFlopIn);
        if ( !Gia_ObjIsAnd(pDriver) )
        {
            printf( "The flop driver %d is not a node.\n", i );
            Vec_PtrPush( vDatas, NULL );
            Vec_IntPush( vFlopClasses, 0 );
            Counter++;
            continue;
        }
        if ( !Gia_ObjFaninC0(pDriver) || !Gia_ObjFaninC1(pDriver) )
        {
            printf( "The flop driver %d is not an OR gate.\n", i );
            Vec_PtrPush( vDatas, NULL );
            Vec_IntPush( vFlopClasses, 0 );
            Counter++;
            continue;
        }
        pFan0 = Gia_ObjFanin0(pDriver);
        pFan1 = Gia_ObjFanin1(pDriver);
        if ( !Gia_ObjIsAnd(pFan0) || !Gia_ObjIsAnd(pFan1) )
        {
            printf( "The flop driver fanin %d is not a node.\n", i );
            Vec_PtrPush( vDatas, NULL );
            Vec_IntPush( vFlopClasses, 0 );
            Counter++;
            continue;
        }
        pFlopOut = Gia_ObjRiToRo( p, pFlopIn );
        pFlopOut = Gia_NotCond( pFlopOut, !fCompl );
        if ( Gia_ObjChild0(pFan0) != pFlopOut && Gia_ObjChild1(pFan0) != pFlopOut && 
             Gia_ObjChild0(pFan1) != pFlopOut && Gia_ObjChild1(pFan1) != pFlopOut )
        {
            printf( "The flop %d does not have a self-loop.\n", i );
            Vec_PtrPush( vDatas, NULL );
            Vec_IntPush( vFlopClasses, 0 );
            Counter++;
            continue;
        }
        pData = NULL;
        if ( Gia_ObjChild0(pFan0) == pFlopOut )
        {
            pCtrl = Gia_Not( Gia_ObjChild1(pFan0) );
            if ( Gia_ObjFanin0(pFan1) == Gia_Regular(pCtrl) )
                pData = Gia_ObjChild1(pFan1);
            else
                pData = Gia_ObjChild0(pFan1);
        }
        else if ( Gia_ObjChild1(pFan0) == pFlopOut )
        {
            pCtrl = Gia_Not( Gia_ObjChild0(pFan0) );
            if ( Gia_ObjFanin0(pFan1) == Gia_Regular(pCtrl) )
                pData = Gia_ObjChild1(pFan1);
            else
                pData = Gia_ObjChild0(pFan1);
        }
        else if ( Gia_ObjChild0(pFan1) == pFlopOut )
        {
            pCtrl = Gia_Not( Gia_ObjChild1(pFan1) );
            if ( Gia_ObjFanin0(pFan0) == Gia_Regular(pCtrl) )
                pData = Gia_ObjChild1(pFan0);
            else
                pData = Gia_ObjChild0(pFan0);
        }
        else if ( Gia_ObjChild1(pFan1) == pFlopOut )
        {
            pCtrl = Gia_Not( Gia_ObjChild0(pFan1) );
            if ( Gia_ObjFanin0(pFan0) == Gia_Regular(pCtrl) )
                pData = Gia_ObjChild1(pFan0);
            else
                pData = Gia_ObjChild0(pFan0);
        }
        else assert( 0 );
        if ( Vec_PtrFind( vCtrls, pCtrl ) == -1 )
            Vec_PtrPush( vCtrls, pCtrl );
        iClass = Vec_PtrFind( vCtrls, pCtrl );
        pData = Gia_NotCond( pData, !fCompl );
        Vec_PtrPush( vDatas, pData );
        Vec_IntPush( vFlopClasses, iClass );
    }
    assert( Vec_PtrSize( vDatas ) == Gia_ManRegNum(p) );
    assert( Vec_IntSize( vFlopClasses ) == Gia_ManRegNum(p) );
    printf( "Detected %d classes.\n", Vec_PtrSize(vCtrls) - (Counter == 0) );
    Vec_PtrFree( vCtrls );
    

    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Gia_ManAppendCi( pNew );
        else if ( Gia_ObjIsPo(p, pObj) )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    Gia_ManForEachRi( p, pObj, i )
    {
        pData = (Gia_Obj_t *)Vec_PtrEntry(vDatas, i);
        if ( pData == NULL )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        else
            pObj->Value = Gia_ManAppendCo( pNew, Abc_LitNotCond(Gia_Regular(pData)->Value, Gia_IsComplement(pData)) );
    }
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    Vec_PtrFree( vDatas );


    pNew = Gia_ManCleanup( pAux = pNew );
    Gia_ManStop( pAux );
    pNew->vFlopClasses = vFlopClasses;
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

