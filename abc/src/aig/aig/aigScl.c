/**CFile****************************************************************

  FileName    [aigScl.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Sequential cleanup.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigScl.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Remaps the manager.]

  Description [Map in the array specifies for each CI node the node that
  should be used after remapping.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManRemap( Aig_Man_t * p, Vec_Ptr_t * vMap )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj, * pObjMapped;
    int i, nTruePis;
    // create the new manager
    pNew = Aig_ManStart( Aig_ManObjNumMax(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    pNew->nAsserts = p->nAsserts;
    pNew->nConstrs = p->nConstrs;
    pNew->nBarBufs = p->nBarBufs;
    assert( p->vFlopNums == NULL || Vec_IntSize(p->vFlopNums) == p->nRegs );
    if ( p->vFlopNums )
        pNew->vFlopNums = Vec_IntDup( p->vFlopNums );
    if ( p->vFlopReprs )
        pNew->vFlopReprs = Vec_IntDup( p->vFlopReprs );
    // create the PIs
    Aig_ManCleanData( p );
    Aig_ManConst1(p)->pData = Aig_ManConst1(pNew);
    Aig_ManForEachCi( p, pObj, i )
        pObj->pData = Aig_ObjCreateCi(pNew);
    // implement the mapping
    nTruePis = Aig_ManCiNum(p)-Aig_ManRegNum(p);
    if ( p->vFlopReprs )
    {
        Aig_ManForEachLoSeq( p, pObj, i )
            pObj->pNext = (Aig_Obj_t *)(long)Vec_IntEntry( p->vFlopNums, i-nTruePis );
    }
    Aig_ManForEachCi( p, pObj, i )
    {
        pObjMapped = (Aig_Obj_t *)Vec_PtrEntry( vMap, i );
        pObj->pData = Aig_NotCond( (Aig_Obj_t *)Aig_Regular(pObjMapped)->pData, Aig_IsComplement(pObjMapped) );
        if ( pNew->vFlopReprs && i >= nTruePis && pObj != pObjMapped )
        {
            Vec_IntPush( pNew->vFlopReprs, Aig_ObjCioId(pObj) );
            if ( Aig_ObjIsConst1( Aig_Regular(pObjMapped) ) )
                Vec_IntPush( pNew->vFlopReprs, -1 );
            else
            {
                assert( !Aig_IsComplement(pObjMapped) );
                assert( Aig_ObjIsCi(pObjMapped) );
                assert( Aig_ObjCioId(pObj) != Aig_ObjCioId(pObjMapped) );
                Vec_IntPush( pNew->vFlopReprs, Aig_ObjCioId(pObjMapped) );
            }
        }
    }
    if ( p->vFlopReprs )
    {
        Aig_ManForEachLoSeq( p, pObj, i )
            pObj->pNext = NULL;
    }
    // duplicate internal nodes
    Aig_ManForEachObj( p, pObj, i )
        if ( Aig_ObjIsBuf(pObj) )
            pObj->pData = Aig_ObjChild0Copy(pObj);
        else if ( Aig_ObjIsNode(pObj) )
            pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // add the POs
    Aig_ManForEachCo( p, pObj, i )
        Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    assert( Aig_ManNodeNum(p) >= Aig_ManNodeNum(pNew) );
    Aig_ManSetRegNum( pNew, Aig_ManRegNum(p) );
    // check the resulting network
    if ( !Aig_ManCheck(pNew) )
        printf( "Aig_ManRemap(): The check has failed.\n" );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Returns the number of dangling nodes removed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManSeqCleanup_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    // collect latch input corresponding to unmarked PI (latch output)
    if ( Aig_ObjIsCi(pObj) )
    {
        Vec_PtrPush( vNodes, pObj->pNext );
        return;
    }
    if ( Aig_ObjIsCo(pObj) || Aig_ObjIsBuf(pObj) )
    {
        Aig_ManSeqCleanup_rec( p, Aig_ObjFanin0(pObj), vNodes );
        return;
    }
    assert( Aig_ObjIsNode(pObj) );
    Aig_ManSeqCleanup_rec( p, Aig_ObjFanin0(pObj), vNodes );
    Aig_ManSeqCleanup_rec( p, Aig_ObjFanin1(pObj), vNodes );
}

/**Function*************************************************************

  Synopsis    [Returns the number of dangling nodes removed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManSeqCleanup( Aig_Man_t * p )
{
    Vec_Ptr_t * vNodes, * vCis, * vCos;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i, nTruePis, nTruePos;
//    assert( Aig_ManBufNum(p) == 0 );

    // mark the PIs
    Aig_ManIncrementTravId( p );
    Aig_ObjSetTravIdCurrent( p, Aig_ManConst1(p) );
    Aig_ManForEachPiSeq( p, pObj, i )
        Aig_ObjSetTravIdCurrent( p, pObj );

    // prepare to collect nodes reachable from POs
    vNodes = Vec_PtrAlloc( 100 );
    Aig_ManForEachPoSeq( p, pObj, i )
        Vec_PtrPush( vNodes, pObj );

    // remember latch inputs in latch outputs
    Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
        pObjLo->pNext = pObjLi;
    // mark the nodes reachable from these nodes
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        Aig_ManSeqCleanup_rec( p, pObj, vNodes );
    assert( Vec_PtrSize(vNodes) <= Aig_ManCoNum(p) );
    // clean latch output pointers
    Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
        pObjLo->pNext = NULL;

    // if some latches are removed, update PIs/POs
    if ( Vec_PtrSize(vNodes) < Aig_ManCoNum(p) )
    {
        if ( p->vFlopNums )
        {
            int nTruePos = Aig_ManCoNum(p)-Aig_ManRegNum(p);
            int iNum, k = 0;
            Aig_ManForEachCo( p, pObj, i )
                if ( i >= nTruePos && Aig_ObjIsTravIdCurrent(p, pObj) )
                {
                    iNum = Vec_IntEntry( p->vFlopNums, i - nTruePos );
                    Vec_IntWriteEntry( p->vFlopNums, k++, iNum );
                }
            assert( k == Vec_PtrSize(vNodes) - nTruePos );
            Vec_IntShrink( p->vFlopNums, k );
        }
        // collect new CIs/COs
        vCis = Vec_PtrAlloc( Aig_ManCiNum(p) );
        Aig_ManForEachCi( p, pObj, i )
            if ( Aig_ObjIsTravIdCurrent(p, pObj) )
                Vec_PtrPush( vCis, pObj );
            else
            {
                Vec_PtrWriteEntry( p->vObjs, pObj->Id, NULL );
//                Aig_ManRecycleMemory( p, pObj );
            }
        vCos = Vec_PtrAlloc( Aig_ManCoNum(p) );
        Aig_ManForEachCo( p, pObj, i )
            if ( Aig_ObjIsTravIdCurrent(p, pObj) )
                Vec_PtrPush( vCos, pObj );
            else
            {
                Aig_ObjDisconnect( p, pObj );
                Vec_PtrWriteEntry( p->vObjs, pObj->Id, NULL );
//                Aig_ManRecycleMemory( p, pObj );
            }
        // remember the number of true PIs/POs
        nTruePis = Aig_ManCiNum(p) - Aig_ManRegNum(p);
        nTruePos = Aig_ManCoNum(p) - Aig_ManRegNum(p);
        // set the new number of registers
        p->nRegs -= Aig_ManCoNum(p) - Vec_PtrSize(vNodes);
        // create new PIs/POs
        assert( Vec_PtrSize(vCis) == nTruePis + p->nRegs );
        assert( Vec_PtrSize(vCos) == nTruePos + p->nRegs );
        Vec_PtrFree( p->vCis );    p->vCis = vCis;
        Vec_PtrFree( p->vCos );    p->vCos = vCos;
        p->nObjs[AIG_OBJ_CI] = Vec_PtrSize( p->vCis );
        p->nObjs[AIG_OBJ_CO] = Vec_PtrSize( p->vCos );
                
    }
    Vec_PtrFree( vNodes );
    p->nTruePis = Aig_ManCiNum(p) - Aig_ManRegNum(p); 
    p->nTruePos = Aig_ManCoNum(p) - Aig_ManRegNum(p); 
    Aig_ManSetCioIds( p );
    // remove dangling nodes
    return Aig_ManCleanup( p );
}

/**Function*************************************************************

  Synopsis    [Returns the number of dangling nodes removed.]

  Description [This cleanup procedure is different in that 
  it removes logic but does not remove the dangling latches.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManSeqCleanupBasic( Aig_Man_t * p )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i;
//    assert( Aig_ManBufNum(p) == 0 );

    // mark the PIs
    Aig_ManIncrementTravId( p );
    Aig_ObjSetTravIdCurrent( p, Aig_ManConst1(p) );
    Aig_ManForEachPiSeq( p, pObj, i )
        Aig_ObjSetTravIdCurrent( p, pObj );

    // prepare to collect nodes reachable from POs
    vNodes = Vec_PtrAlloc( 100 );
    Aig_ManForEachPoSeq( p, pObj, i )
        Vec_PtrPush( vNodes, pObj );

    // remember latch inputs in latch outputs
    Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
        pObjLo->pNext = pObjLi;
    // mark the nodes reachable from these nodes
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        Aig_ManSeqCleanup_rec( p, pObj, vNodes );
    assert( Vec_PtrSize(vNodes) <= Aig_ManCoNum(p) );
    // clean latch output pointers
    Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
        pObjLo->pNext = NULL;

    // if some latches are removed, update PIs/POs
    if ( Vec_PtrSize(vNodes) < Aig_ManCoNum(p) )
    {
        // add constant drivers to the dangling latches
        Aig_ManForEachCo( p, pObj, i )
            if ( !Aig_ObjIsTravIdCurrent(p, pObj) )
                Aig_ObjPatchFanin0( p, pObj, Aig_ManConst0(p) );
    }
    Vec_PtrFree( vNodes );
    // remove dangling nodes
    return Aig_ManCleanup( p );
}

/**Function*************************************************************

  Synopsis    [Returns the number of dangling nodes removed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManCountMergeRegs( Aig_Man_t * p )
{
    Aig_Obj_t * pObj, * pFanin;
    int i, Counter = 0, Const0 = 0, Const1 = 0;
    Aig_ManIncrementTravId( p );
    Aig_ManForEachLiSeq( p, pObj, i )
    {
        pFanin = Aig_ObjFanin0(pObj);
        if ( Aig_ObjIsConst1(pFanin) )
        {
            if ( Aig_ObjFaninC0(pObj) )
                Const0++;
            else
                Const1++;
        }
        if ( Aig_ObjIsTravIdCurrent(p, pFanin) )
            continue;
        Aig_ObjSetTravIdCurrent(p, pFanin);
        Counter++;
    }
    printf( "Regs = %d. Fanins = %d. Const0 = %d. Const1 = %d.\n", 
        Aig_ManRegNum(p), Counter, Const0, Const1 );
    return 0;
}


/**Function*************************************************************

  Synopsis    [Checks how many latches can be reduced.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManReduceLachesCount( Aig_Man_t * p )
{
    Aig_Obj_t * pObj, * pFanin;
    int i, Counter = 0, Diffs = 0;
    assert( Aig_ManRegNum(p) > 0 );
    Aig_ManForEachObj( p, pObj, i )
        assert( !pObj->fMarkA && !pObj->fMarkB );
    Aig_ManForEachLiSeq( p, pObj, i )
    {
        pFanin = Aig_ObjFanin0(pObj);
        if ( Aig_ObjFaninC0(pObj) )
        {
            if ( pFanin->fMarkB )
                Counter++;
            else
                pFanin->fMarkB = 1;
        }
        else
        {
            if ( pFanin->fMarkA )
                Counter++;
            else
                pFanin->fMarkA = 1;
        }
    }
    // count fanins that have both attributes
    Aig_ManForEachLiSeq( p, pObj, i )
    {
        pFanin = Aig_ObjFanin0(pObj);
        Diffs += pFanin->fMarkA && pFanin->fMarkB;
        pFanin->fMarkA = pFanin->fMarkB = 0;
    }
//    printf( "Diffs = %d.\n", Diffs );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Reduces the latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManReduceLachesOnce( Aig_Man_t * p )
{
    Vec_Ptr_t * vMap;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo, * pFanin;
    int * pMapping, i;
    // start mapping by adding the true PIs
    vMap = Vec_PtrAlloc( Aig_ManCiNum(p) );
    Aig_ManForEachPiSeq( p, pObj, i )
        Vec_PtrPush( vMap, pObj );
    // create mapping of fanin nodes into the corresponding latch outputs
    pMapping = ABC_FALLOC( int, 2 * Aig_ManObjNumMax(p) );
    Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
    {
        pFanin = Aig_ObjFanin0(pObjLi);
        if ( Aig_ObjFaninC0(pObjLi) )
        {
            if ( pFanin->fMarkB )
            {
                Vec_PtrPush( vMap, Aig_ManLo(p, pMapping[2*pFanin->Id + 1]) );
            }
            else
            {
                pFanin->fMarkB = 1;
                pMapping[2*pFanin->Id + 1] = i;
                Vec_PtrPush( vMap, pObjLo );
            }
        }
        else
        {
            if ( pFanin->fMarkA )
            {
                Vec_PtrPush( vMap, Aig_ManLo(p, pMapping[2*pFanin->Id]) );
            }
            else
            {
                pFanin->fMarkA = 1;
                pMapping[2*pFanin->Id] = i;
                Vec_PtrPush( vMap, pObjLo );
            }
        }
    }
    ABC_FREE( pMapping );
    Aig_ManForEachLiSeq( p, pObj, i )
    {
        pFanin = Aig_ObjFanin0(pObj);
        pFanin->fMarkA = pFanin->fMarkB = 0;
    }
    return vMap;
}

/**Function*************************************************************

  Synopsis    [Reduces the latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManReduceLaches( Aig_Man_t * p, int fVerbose )
{
    Aig_Man_t * pTemp;
    Vec_Ptr_t * vMap;
    int nSaved, nCur;
    if ( fVerbose )
        printf( "Performing combinational register sweep:\n" );
    for ( nSaved = 0; (nCur = Aig_ManReduceLachesCount(p)); nSaved += nCur )
    {
//        printf( "Reducible = %d\n", nCur );
        vMap = Aig_ManReduceLachesOnce( p );
        p = Aig_ManRemap( pTemp = p, vMap );
        Vec_PtrFree( vMap );
        Aig_ManSeqCleanup( p );
        if ( fVerbose )
            Aig_ManReportImprovement( pTemp, p );
        Aig_ManStop( pTemp );
        if ( p->nRegs == 0 )
            break;
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Computes strongly connected components of registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManComputeSccs( Aig_Man_t * p )
{
    Vec_Ptr_t * vSupports, * vMatrix, * vMatrix2;
    Vec_Int_t * vSupp, * vSupp2, * vComp;
    char * pVarsTot;
    int i, k, m, iOut, iIn, nComps;
    if ( Aig_ManRegNum(p) == 0 )
    {
        printf( "The network is combinational.\n" );
        return;
    }
    // get structural supports for each output
    vSupports = Aig_ManSupports( p );
    // transforms the supports into the latch dependency matrix
    vMatrix = Vec_PtrStart( Aig_ManRegNum(p) );
    Vec_PtrForEachEntry( Vec_Int_t *, vSupports, vSupp, i )
    {
        // skip true POs
        iOut = Vec_IntPop( vSupp );
        iOut -= Aig_ManCoNum(p) - Aig_ManRegNum(p);
        if ( iOut < 0 )
            continue;
        // remove PIs
        m = 0;
        Vec_IntForEachEntry( vSupp, iIn, k )
        {
            iIn -= Aig_ManCiNum(p) - Aig_ManRegNum(p);
            if ( iIn < 0 )
                continue;
            assert( iIn < Aig_ManRegNum(p) );
            Vec_IntWriteEntry( vSupp, m++, iIn );
        }
        Vec_IntShrink( vSupp, m );
        // store support in the matrix
        assert( iOut < Aig_ManRegNum(p) );
        Vec_PtrWriteEntry( vMatrix, iOut, vSupp );
    }
    // create the reverse matrix
    vMatrix2 = Vec_PtrAlloc( Aig_ManRegNum(p) );
    for ( i = 0; i < Aig_ManRegNum(p); i++ )
        Vec_PtrPush( vMatrix2, Vec_IntAlloc(8) );
    Vec_PtrForEachEntry( Vec_Int_t *, vMatrix, vSupp, i )
    {
        Vec_IntForEachEntry( vSupp, iIn, k )
        {
            vSupp2 = (Vec_Int_t *)Vec_PtrEntry( vMatrix2, iIn );
            Vec_IntPush( vSupp2, i );
        }
    } 

    // detect strongly connected components
    vComp = Vec_IntAlloc( Aig_ManRegNum(p) );
    pVarsTot = ABC_ALLOC( char, Aig_ManRegNum(p) );
    memset( pVarsTot, 0, Aig_ManRegNum(p) * sizeof(char) );
    for ( nComps = 0; ; nComps++ )
    {
        Vec_IntClear( vComp );
        // get the first support
        for ( iOut = 0; iOut < Aig_ManRegNum(p); iOut++ )
            if ( pVarsTot[iOut] == 0 )
                break;
        if ( iOut == Aig_ManRegNum(p) )
            break;
        pVarsTot[iOut] = 1;
        Vec_IntPush( vComp, iOut );
        Vec_IntForEachEntry( vComp, iOut, i )
        {
            vSupp = (Vec_Int_t *)Vec_PtrEntry( vMatrix, iOut );
            Vec_IntForEachEntry( vSupp, iIn, k )
            {
                if ( pVarsTot[iIn] )
                    continue;
                pVarsTot[iIn] = 1;
                Vec_IntPush( vComp, iIn );
            }
            vSupp2 = (Vec_Int_t *)Vec_PtrEntry( vMatrix2, iOut );
            Vec_IntForEachEntry( vSupp2, iIn, k )
            {
                if ( pVarsTot[iIn] )
                    continue;
                pVarsTot[iIn] = 1;
                Vec_IntPush( vComp, iIn );
            }
        }
        if ( Vec_IntSize(vComp) == Aig_ManRegNum(p) )
        {
            printf( "There is only one SCC of registers in this network.\n" );
            break;
        }
        printf( "SCC #%d contains %5d registers.\n", nComps+1, Vec_IntSize(vComp) );
    }
    ABC_FREE( pVarsTot );
    Vec_IntFree( vComp );
    Vec_PtrFree( vMatrix );
    Vec_VecFree( (Vec_Vec_t *)vMatrix2 );
    Vec_VecFree( (Vec_Vec_t *)vSupports );
}

/**Function*************************************************************

  Synopsis    [Performs partitioned register sweep.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManSclPart( Aig_Man_t * pAig, int fLatchConst, int fLatchEqual, int fVerbose ) 
{
    Vec_Ptr_t * vResult;
    Vec_Int_t * vPart;
    int i, nCountPis, nCountRegs;
    int * pMapBack;
    Aig_Man_t * pTemp, * pNew;
    int nClasses;

    if ( pAig->vClockDoms ) 
    {
        vResult = Vec_PtrAlloc( 100 );
        Vec_PtrForEachEntry( Vec_Int_t *, (Vec_Ptr_t *)pAig->vClockDoms, vPart, i )
            Vec_PtrPush( vResult, Vec_IntDup(vPart) );
    } 
    else
        vResult = Aig_ManRegPartitionSimple( pAig, 0, 0 );

    Aig_ManReprStart( pAig, Aig_ManObjNumMax(pAig) );
    Vec_PtrForEachEntry( Vec_Int_t *, vResult, vPart, i ) 
    {
        pTemp = Aig_ManRegCreatePart( pAig, vPart, &nCountPis, &nCountRegs, &pMapBack );
        Aig_ManSetRegNum( pTemp, pTemp->nRegs );
        if (nCountPis>0) 
        {
            pNew = Aig_ManScl( pTemp, fLatchConst, fLatchEqual, 0, -1, -1, fVerbose, 0 );
            nClasses = Aig_TransferMappedClasses( pAig, pTemp, pMapBack );
            if ( fVerbose )
                printf( "%3d : Reg = %4d. PI = %4d. (True = %4d. Regs = %4d.) And = %5d. It = %3d. Cl = %5d\n",
                        i, Vec_IntSize(vPart), Aig_ManCiNum(pTemp)-Vec_IntSize(vPart), nCountPis, nCountRegs, Aig_ManNodeNum(pTemp), 0, nClasses );
            Aig_ManStop( pNew );
        }
        Aig_ManStop( pTemp );
        ABC_FREE( pMapBack );
    }
    pNew = Aig_ManDupRepr( pAig, 0 );
    Aig_ManSeqCleanup( pNew );
    Vec_VecFree( (Vec_Vec_t*)vResult );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManScl( Aig_Man_t * pAig, int fLatchConst, int fLatchEqual, int fUseMvSweep, int nFramesSymb, int nFramesSatur, int fVerbose, int fVeryVerbose )
{
    extern void Saig_ManReportUselessRegisters( Aig_Man_t * pAig );
    extern int Saig_ManReportComplements( Aig_Man_t * p );

    Aig_Man_t * pAigInit, * pAigNew;
    Aig_Obj_t * pFlop1, * pFlop2;
    int i, Entry1, Entry2, nTruePis;//, nRegs;

    if ( pAig->vClockDoms && Vec_VecSize(pAig->vClockDoms) > 0 )
        return Aig_ManSclPart( pAig, fLatchConst, fLatchEqual, fVerbose);

    // store the original AIG
    assert( pAig->vFlopNums == NULL );
    pAigInit = pAig;
    pAig = Aig_ManDupSimple( pAig );
    // create storage for latch numbers
    pAig->vFlopNums = Vec_IntStartNatural( pAig->nRegs );
    pAig->vFlopReprs = Vec_IntAlloc( 100 );
    Aig_ManSeqCleanup( pAig );
    if ( fLatchConst && pAig->nRegs )
        pAig = Aig_ManConstReduce( pAig, fUseMvSweep, nFramesSymb, nFramesSatur, fVerbose, fVeryVerbose );
    if ( fLatchEqual && pAig->nRegs )
        pAig = Aig_ManReduceLaches( pAig, fVerbose );
    // translate pairs into reprs
    nTruePis = Aig_ManCiNum(pAigInit)-Aig_ManRegNum(pAigInit);
    Aig_ManReprStart( pAigInit, Aig_ManObjNumMax(pAigInit) );
    Vec_IntForEachEntry( pAig->vFlopReprs, Entry1, i )
    {
        Entry2 = Vec_IntEntry( pAig->vFlopReprs, ++i ); 
        pFlop1 = Aig_ManCi( pAigInit, nTruePis + Entry1 );
        pFlop2 = (Entry2 == -1)? Aig_ManConst1(pAigInit) : Aig_ManCi( pAigInit, nTruePis + Entry2 );
        assert( pFlop1 != pFlop2 );
        if ( pFlop1->Id > pFlop2->Id )
            pAigInit->pReprs[pFlop1->Id] = pFlop2;
        else
            pAigInit->pReprs[pFlop2->Id] = pFlop1;
    }
    Aig_ManStop( pAig );
//    Aig_ManSeqCleanup( pAigInit );
    pAigNew = Aig_ManDupRepr( pAigInit, 0 );
    Aig_ManSeqCleanup( pAigNew );

//    Saig_ManReportUselessRegisters( pAigNew );
    if ( Aig_ManRegNum(pAigNew) == 0 )
        return pAigNew;
//    nRegs = Saig_ManReportComplements( pAigNew );
//    if ( nRegs ) 
//    printf( "The number of complemented registers = %d.\n", nRegs );
    return pAigNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

