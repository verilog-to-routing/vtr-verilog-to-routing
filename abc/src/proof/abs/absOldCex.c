/**CFile****************************************************************

  FileName    [saigAbsCba.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [CEX-based abstraction.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigAbsCba.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abs.h"
#include "sat/bmc/bmc.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// local manager
typedef struct Saig_ManCba_t_ Saig_ManCba_t;
struct Saig_ManCba_t_
{
    // user data
    Aig_Man_t * pAig;       // user's AIG
    Abc_Cex_t * pCex;       // user's CEX
    int         nInputs;    // the number of first inputs to skip
    int         fVerbose;   // verbose flag
    // unrolling
    Aig_Man_t * pFrames;    // unrolled timeframes
    Vec_Int_t * vMapPiF2A;  // mapping of frame PIs into real PIs
    // additional information
    Vec_Vec_t * vReg2Frame; // register to frame mapping
    Vec_Vec_t * vReg2Value; // register to value mapping
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Selects the best flops from the given array.]

  Description [Selects the best 'nFfsToSelect' flops among the array 
  'vAbsFfsToAdd' of flops that should be added to the abstraction.
  To this end, this procedure simulates the original AIG (pAig) using
  the given CEX (pAbsCex), which was detected for the abstraction.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_ManCbaFilterFlops( Aig_Man_t * pAig, Abc_Cex_t * pAbsCex, Vec_Int_t * vFlopClasses, Vec_Int_t * vAbsFfsToAdd, int nFfsToSelect )
{
    Aig_Obj_t * pObj, * pObjRi, * pObjRo;
    Vec_Int_t * vMapEntries, * vFlopCosts, * vFlopAddCosts, * vFfsToAddBest;
    int i, k, f, Entry, iBit, * pPerm;
    assert( Aig_ManRegNum(pAig) == Vec_IntSize(vFlopClasses) );
    assert( Vec_IntSize(vAbsFfsToAdd) > nFfsToSelect );
    // map previously abstracted flops into their original numbers
    vMapEntries = Vec_IntAlloc( Vec_IntSize(vFlopClasses) );
    Vec_IntForEachEntry( vFlopClasses, Entry, i )
        if ( Entry == 0 )
            Vec_IntPush( vMapEntries, i );
    // simulate one frame at a time
    assert( Saig_ManPiNum(pAig) + Vec_IntSize(vMapEntries) == pAbsCex->nPis );
    vFlopCosts = Vec_IntStart( Vec_IntSize(vMapEntries) );
    // initialize the flops
    Aig_ManCleanMarkB(pAig);
    Aig_ManConst1(pAig)->fMarkB = 1;
    Saig_ManForEachLo( pAig, pObj, i )
        pObj->fMarkB = 0;
    for ( f = 0; f < pAbsCex->iFrame; f++ )
    {
        // override the flop values according to the cex
        iBit = pAbsCex->nRegs + f * pAbsCex->nPis + Saig_ManPiNum(pAig);
        Vec_IntForEachEntry( vMapEntries, Entry, k )
            Saig_ManLo(pAig, Entry)->fMarkB = Abc_InfoHasBit(pAbsCex->pData, iBit + k);
        // simulate
        Aig_ManForEachNode( pAig, pObj, k )
            pObj->fMarkB = (Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj)) & 
                           (Aig_ObjFanin1(pObj)->fMarkB ^ Aig_ObjFaninC1(pObj));
        Aig_ManForEachCo( pAig, pObj, k )
            pObj->fMarkB = Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj);
        // transfer
        Saig_ManForEachLiLo( pAig, pObjRi, pObjRo, k )
            pObjRo->fMarkB = pObjRi->fMarkB;
        // compare
        iBit = pAbsCex->nRegs + (f + 1) * pAbsCex->nPis + Saig_ManPiNum(pAig);
        Vec_IntForEachEntry( vMapEntries, Entry, k )
            if ( Saig_ManLi(pAig, Entry)->fMarkB != (unsigned)Abc_InfoHasBit(pAbsCex->pData, iBit + k) )
                Vec_IntAddToEntry( vFlopCosts, k, 1 );
    }
//    Vec_IntForEachEntry( vFlopCosts, Entry, i )
//        printf( "%d ", Entry );
//    printf( "\n" );
    // remap the cost
    vFlopAddCosts = Vec_IntAlloc( Vec_IntSize(vAbsFfsToAdd) );
    Vec_IntForEachEntry( vAbsFfsToAdd, Entry, i )
        Vec_IntPush( vFlopAddCosts, -Vec_IntEntry(vFlopCosts, Entry) );
    // sort the flops
    pPerm = Abc_MergeSortCost( Vec_IntArray(vFlopAddCosts), Vec_IntSize(vFlopAddCosts) );
    // shrink the array
    vFfsToAddBest = Vec_IntAlloc( nFfsToSelect );
    for ( i = 0; i < nFfsToSelect; i++ )
    {
//        printf( "%d ", Vec_IntEntry(vFlopAddCosts, pPerm[i]) );
        Vec_IntPush( vFfsToAddBest, Vec_IntEntry(vAbsFfsToAdd, pPerm[i]) );
    }
//    printf( "\n" );
    // cleanup
    ABC_FREE( pPerm );
    Vec_IntFree( vMapEntries );
    Vec_IntFree( vFlopCosts );
    Vec_IntFree( vFlopAddCosts );
    Aig_ManCleanMarkB(pAig);
    // return the computed flops
    return vFfsToAddBest;
}


/**Function*************************************************************

  Synopsis    [Duplicate with literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManDupWithCubes( Aig_Man_t * pAig, Vec_Vec_t * vReg2Value )
{
    Vec_Int_t * vLevel;
    Aig_Man_t * pAigNew;
    Aig_Obj_t * pObj, * pMiter;
    int i, k, Lit;
    assert( pAig->nConstrs == 0 );
    // start the new manager
    pAigNew = Aig_ManStart( Aig_ManNodeNum(pAig) + Vec_VecSizeSize(vReg2Value) );
    pAigNew->pName = Abc_UtilStrsav( pAig->pName );
    // map the constant node
    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pAigNew );
    // create variables for PIs
    Aig_ManForEachCi( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pAigNew );
    // add internal nodes of this frame
    Aig_ManForEachNode( pAig, pObj, i )
        pObj->pData = Aig_And( pAigNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // create POs for cubes
    Vec_VecForEachLevelInt( vReg2Value, vLevel, i )
    {
        pMiter = Aig_ManConst1( pAigNew );
        Vec_IntForEachEntry( vLevel, Lit, k )
        {
            pObj = Saig_ManLi( pAig, Abc_Lit2Var(Lit) );
            pMiter = Aig_And( pAigNew, pMiter, Aig_NotCond(Aig_ObjChild0Copy(pObj), Abc_LitIsCompl(Lit)) );
        }
        Aig_ObjCreateCo( pAigNew, pMiter );
    }
    // transfer to register outputs
    Saig_ManForEachLi( pAig, pObj, i )
        Aig_ObjCreateCo( pAigNew, Aig_ObjChild0Copy(pObj) );
    // finalize
    Aig_ManCleanup( pAigNew );
    Aig_ManSetRegNum( pAigNew, Aig_ManRegNum(pAig) );
    return pAigNew;
}

/**Function*************************************************************

  Synopsis    [Maps array of frame PI IDs into array of additional PI IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_ManCbaReason2Inputs( Saig_ManCba_t * p, Vec_Int_t * vReasons )
{
    Vec_Int_t * vOriginal, * vVisited;
    int i, Entry;
    vOriginal = Vec_IntAlloc( Saig_ManPiNum(p->pAig) ); 
    vVisited = Vec_IntStart( Saig_ManPiNum(p->pAig) );
    Vec_IntForEachEntry( vReasons, Entry, i )
    {
        int iInput = Vec_IntEntry( p->vMapPiF2A, 2*Entry );
        assert( iInput >= p->nInputs && iInput < Aig_ManCiNum(p->pAig) );
        if ( Vec_IntEntry(vVisited, iInput) == 0 )
            Vec_IntPush( vOriginal, iInput - p->nInputs );
        Vec_IntAddToEntry( vVisited, iInput, 1 );
    }
    Vec_IntFree( vVisited );
    return vOriginal;
}

/**Function*************************************************************

  Synopsis    [Creates the counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Saig_ManCbaReason2Cex( Saig_ManCba_t * p, Vec_Int_t * vReasons )
{
    Abc_Cex_t * pCare;
    int i, Entry, iInput, iFrame;
    pCare = Abc_CexDup( p->pCex, p->pCex->nRegs );
    memset( pCare->pData, 0, sizeof(unsigned) * Abc_BitWordNum(pCare->nBits) );
    Vec_IntForEachEntry( vReasons, Entry, i )
    {
        assert( Entry >= 0 && Entry < Aig_ManCiNum(p->pFrames) );
        iInput = Vec_IntEntry( p->vMapPiF2A, 2*Entry );
        iFrame = Vec_IntEntry( p->vMapPiF2A, 2*Entry+1 );
        Abc_InfoSetBit( pCare->pData, pCare->nRegs + pCare->nPis * iFrame + iInput );
    }
/*
    for ( iFrame = 0; iFrame <= pCare->iFrame; iFrame++ )
    {
        int Count = 0;
        for ( i = 0; i < pCare->nPis; i++ )
            Count +=  Abc_InfoHasBit(pCare->pData, pCare->nRegs + pCare->nPis * iFrame + i);
        printf( "%d ", Count );
    }
printf( "\n" );
*/
    return pCare;
}

/**Function*************************************************************

  Synopsis    [Returns reasons for the property to fail.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManCbaFindReason_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Int_t * vPrios, Vec_Int_t * vReasons )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Aig_ObjIsConst1(pObj) )
        return;
    if ( Aig_ObjIsCi(pObj) )
    {
        Vec_IntPush( vReasons, Aig_ObjCioId(pObj) );
        return;
    }
    assert( Aig_ObjIsNode(pObj) );
    if ( pObj->fPhase )
    {
        Saig_ManCbaFindReason_rec( p, Aig_ObjFanin0(pObj), vPrios, vReasons );
        Saig_ManCbaFindReason_rec( p, Aig_ObjFanin1(pObj), vPrios, vReasons );
    }
    else
    {
        int fPhase0 = Aig_ObjFaninC0(pObj) ^ Aig_ObjFanin0(pObj)->fPhase;
        int fPhase1 = Aig_ObjFaninC1(pObj) ^ Aig_ObjFanin1(pObj)->fPhase;
        assert( !fPhase0 || !fPhase1 );
        if ( !fPhase0 && fPhase1 )
            Saig_ManCbaFindReason_rec( p, Aig_ObjFanin0(pObj), vPrios, vReasons );
        else if ( fPhase0 && !fPhase1 )
            Saig_ManCbaFindReason_rec( p, Aig_ObjFanin1(pObj), vPrios, vReasons );
        else 
        {
            int iPrio0 = Vec_IntEntry( vPrios, Aig_ObjFaninId0(pObj) );
            int iPrio1 = Vec_IntEntry( vPrios, Aig_ObjFaninId1(pObj) );
            if ( iPrio0 <= iPrio1 )
                Saig_ManCbaFindReason_rec( p, Aig_ObjFanin0(pObj), vPrios, vReasons );
            else
                Saig_ManCbaFindReason_rec( p, Aig_ObjFanin1(pObj), vPrios, vReasons );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Returns reasons for the property to fail.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_ManCbaFindReason( Saig_ManCba_t * p )
{
    Aig_Obj_t * pObj;
    Vec_Int_t * vPrios, * vReasons;
    int i;

    // set PI values according to CEX
    vPrios = Vec_IntStartFull( Aig_ManObjNumMax(p->pFrames) );
    Aig_ManConst1(p->pFrames)->fPhase = 1;
    Aig_ManForEachCi( p->pFrames, pObj, i )
    {
        int iInput = Vec_IntEntry( p->vMapPiF2A, 2*i );
        int iFrame = Vec_IntEntry( p->vMapPiF2A, 2*i+1 );
        pObj->fPhase = Abc_InfoHasBit( p->pCex->pData, p->pCex->nRegs + p->pCex->nPis * iFrame + iInput );
        Vec_IntWriteEntry( vPrios, Aig_ObjId(pObj), i );
    }

    // traverse and set the priority
    Aig_ManForEachNode( p->pFrames, pObj, i )
    {
        int fPhase0 = Aig_ObjFaninC0(pObj) ^ Aig_ObjFanin0(pObj)->fPhase;
        int fPhase1 = Aig_ObjFaninC1(pObj) ^ Aig_ObjFanin1(pObj)->fPhase;
        int iPrio0  = Vec_IntEntry( vPrios, Aig_ObjFaninId0(pObj) );
        int iPrio1  = Vec_IntEntry( vPrios, Aig_ObjFaninId1(pObj) );
        pObj->fPhase = fPhase0 && fPhase1;
        if ( fPhase0 && fPhase1 ) // both are one
            Vec_IntWriteEntry( vPrios, Aig_ObjId(pObj), Abc_MaxInt(iPrio0, iPrio1) );
        else if ( !fPhase0 && fPhase1 ) 
            Vec_IntWriteEntry( vPrios, Aig_ObjId(pObj), iPrio0 );
        else if ( fPhase0 && !fPhase1 )
            Vec_IntWriteEntry( vPrios, Aig_ObjId(pObj), iPrio1 );
        else // both are zero
            Vec_IntWriteEntry( vPrios, Aig_ObjId(pObj), Abc_MinInt(iPrio0, iPrio1) );
    }
    // check the property output
    pObj = Aig_ManCo( p->pFrames, 0 );
    pObj->fPhase = Aig_ObjFaninC0(pObj) ^ Aig_ObjFanin0(pObj)->fPhase;
    assert( !pObj->fPhase );

    // select the reason
    vReasons = Vec_IntAlloc( 100 );
    Aig_ManIncrementTravId( p->pFrames );
    Saig_ManCbaFindReason_rec( p->pFrames, Aig_ObjFanin0(pObj), vPrios, vReasons );
    Vec_IntFree( vPrios );
//    assert( !Aig_ObjIsTravIdCurrent(p->pFrames, Aig_ManConst1(p->pFrames)) );
    return vReasons;
}


/**Function*************************************************************

  Synopsis    [Collect nodes in the unrolled timeframes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManCbaUnrollCollect_rec( Aig_Man_t * pAig, Aig_Obj_t * pObj, Vec_Int_t * vObjs, Vec_Int_t * vRoots )
{
    if ( Aig_ObjIsTravIdCurrent(pAig, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(pAig, pObj);
    if ( Aig_ObjIsCo(pObj) )
        Saig_ManCbaUnrollCollect_rec( pAig, Aig_ObjFanin0(pObj), vObjs, vRoots );
    else if ( Aig_ObjIsNode(pObj) )
    {
        Saig_ManCbaUnrollCollect_rec( pAig, Aig_ObjFanin0(pObj), vObjs, vRoots );
        Saig_ManCbaUnrollCollect_rec( pAig, Aig_ObjFanin1(pObj), vObjs, vRoots );
    }
    if ( vRoots && Saig_ObjIsLo( pAig, pObj ) )
        Vec_IntPush( vRoots, Aig_ObjId( Saig_ObjLoToLi(pAig, pObj) ) );
    Vec_IntPush( vObjs, Aig_ObjId(pObj) );
}

/**Function*************************************************************

  Synopsis    [Derive unrolled timeframes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManCbaUnrollWithCex( Aig_Man_t * pAig, Abc_Cex_t * pCex, int nInputs, Vec_Int_t ** pvMapPiF2A, Vec_Vec_t ** pvReg2Frame )
{
    Aig_Man_t * pFrames;     // unrolled timeframes
    Vec_Vec_t * vFrameCos;   // the list of COs per frame
    Vec_Vec_t * vFrameObjs;  // the list of objects per frame
    Vec_Int_t * vRoots, * vObjs;
    Aig_Obj_t * pObj;
    int i, f;
    // sanity checks
    assert( Saig_ManPiNum(pAig) == pCex->nPis );
//    assert( Saig_ManRegNum(pAig) == pCex->nRegs );
    assert( pCex->iPo >= 0 && pCex->iPo < Saig_ManPoNum(pAig) );

    // map PIs of the unrolled frames into PIs of the original design
    *pvMapPiF2A = Vec_IntAlloc( 1000 );

    // collect COs and Objs visited in each frame
    vFrameCos  = Vec_VecStart( pCex->iFrame+1 );
    vFrameObjs = Vec_VecStart( pCex->iFrame+1 );
    // initialized the topmost frame
    pObj = Aig_ManCo( pAig, pCex->iPo );
    Vec_VecPushInt( vFrameCos, pCex->iFrame, Aig_ObjId(pObj) );
    for ( f = pCex->iFrame; f >= 0; f-- )
    {
        // collect nodes starting from the roots
        Aig_ManIncrementTravId( pAig );
        vRoots = Vec_VecEntryInt( vFrameCos, f );
        Aig_ManForEachObjVec( vRoots, pAig, pObj, i )
            Saig_ManCbaUnrollCollect_rec( pAig, pObj, 
                Vec_VecEntryInt(vFrameObjs, f),
                (Vec_Int_t *)(f ? Vec_VecEntry(vFrameCos, f-1) : NULL) );
    }

    // derive unrolled timeframes
    pFrames = Aig_ManStart( 10000 );
    pFrames->pName = Abc_UtilStrsav( pAig->pName );
    pFrames->pSpec = Abc_UtilStrsav( pAig->pSpec );
    // initialize the flops 
    if ( Saig_ManRegNum(pAig) == pCex->nRegs )
    {
        Saig_ManForEachLo( pAig, pObj, i )
            pObj->pData = Aig_NotCond( Aig_ManConst1(pFrames), !Abc_InfoHasBit(pCex->pData, i) );
    }
    else // this is the case when synthesis was applied, assume all-0 init state
    {
        Saig_ManForEachLo( pAig, pObj, i )
            pObj->pData = Aig_NotCond( Aig_ManConst1(pFrames), 1 );
    }
    // iterate through the frames
    for ( f = 0; f <= pCex->iFrame; f++ )
    {
        // construct
        vObjs = Vec_VecEntryInt( vFrameObjs, f );
        Aig_ManForEachObjVec( vObjs, pAig, pObj, i )
        {
            if ( Aig_ObjIsNode(pObj) )
                pObj->pData = Aig_And( pFrames, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
            else if ( Aig_ObjIsCo(pObj) )
                pObj->pData = Aig_ObjChild0Copy(pObj);
            else if ( Aig_ObjIsConst1(pObj) )
                pObj->pData = Aig_ManConst1(pFrames);
            else if ( Saig_ObjIsPi(pAig, pObj) )
            {
                if ( Aig_ObjCioId(pObj) < nInputs )
                {
                    int iBit = pCex->nRegs + f * pCex->nPis + Aig_ObjCioId(pObj);
                    pObj->pData = Aig_NotCond( Aig_ManConst1(pFrames), !Abc_InfoHasBit(pCex->pData, iBit) );
                }
                else
                {
                    pObj->pData = Aig_ObjCreateCi( pFrames );
                    Vec_IntPush( *pvMapPiF2A, Aig_ObjCioId(pObj) );
                    Vec_IntPush( *pvMapPiF2A, f );
                }
            }
        }
        if ( f == pCex->iFrame )
            break;
        // transfer
        vRoots = Vec_VecEntryInt( vFrameCos, f );
        Aig_ManForEachObjVec( vRoots, pAig, pObj, i )
        {
            Saig_ObjLiToLo( pAig, pObj )->pData = pObj->pData;
            if ( *pvReg2Frame )
            {
                Vec_VecPushInt( *pvReg2Frame, f, Aig_ObjId(pObj) );             // record LO
                Vec_VecPushInt( *pvReg2Frame, f, Aig_ObjToLit((Aig_Obj_t *)pObj->pData) ); // record its literal
            }
        }
    }
    // create output
    pObj = Aig_ManCo( pAig, pCex->iPo );
    Aig_ObjCreateCo( pFrames, Aig_Not((Aig_Obj_t *)pObj->pData) );
    Aig_ManSetRegNum( pFrames, 0 );
    // cleanup
    Vec_VecFree( vFrameCos );
    Vec_VecFree( vFrameObjs );
    // finallize
    Aig_ManCleanup( pFrames );
    // return
    return pFrames;
}

/**Function*************************************************************

  Synopsis    [Creates refinement manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Saig_ManCba_t * Saig_ManCbaStart( Aig_Man_t * pAig, Abc_Cex_t * pCex, int nInputs, int fVerbose )
{
    Saig_ManCba_t * p;
    p = ABC_CALLOC( Saig_ManCba_t, 1 );
    p->pAig = pAig;
    p->pCex = pCex;
    p->nInputs = nInputs;
    p->fVerbose = fVerbose;
    return p;
}

/**Function*************************************************************

  Synopsis    [Destroys refinement manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManCbaStop( Saig_ManCba_t * p )
{
    Vec_VecFreeP( &p->vReg2Frame );
    Vec_VecFreeP( &p->vReg2Value );
    Aig_ManStopP( &p->pFrames );
    Vec_IntFreeP( &p->vMapPiF2A );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Destroys refinement manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManCbaShrink( Saig_ManCba_t * p )
{
    Aig_Man_t * pManNew;
    Aig_Obj_t * pObjLi, * pObjFrame;
    Vec_Int_t * vLevel, * vLevel2;
    int f, k, ObjId, Lit;
    // assuming that important objects are labeled in Saig_ManCbaFindReason()
    Vec_VecForEachLevelInt( p->vReg2Frame, vLevel, f )
    {
        Vec_IntForEachEntryDouble( vLevel, ObjId, Lit, k )
        {
            pObjFrame = Aig_ManObj( p->pFrames, Abc_Lit2Var(Lit) );
            if ( pObjFrame == NULL || (!Aig_ObjIsConst1(pObjFrame) && !Aig_ObjIsTravIdCurrent(p->pFrames, pObjFrame)) )
                continue;
            pObjLi = Aig_ManObj( p->pAig, ObjId );
            assert( Saig_ObjIsLi(p->pAig, pObjLi) );
            Vec_VecPushInt( p->vReg2Value, f, Abc_Var2Lit( Aig_ObjCioId(pObjLi) - Saig_ManPoNum(p->pAig), Abc_LitIsCompl(Lit) ^ !pObjFrame->fPhase ) );
        }
    }
    // print statistics
    Vec_VecForEachLevelInt( p->vReg2Frame, vLevel, k )
    {
        vLevel2 = Vec_VecEntryInt( p->vReg2Value, k );
        printf( "Level = %4d   StateBits = %4d (%6.2f %%)  CareBits = %4d (%6.2f %%)\n", k, 
            Vec_IntSize(vLevel)/2, 100.0 * (Vec_IntSize(vLevel)/2) / Aig_ManRegNum(p->pAig),
            Vec_IntSize(vLevel2),  100.0 * Vec_IntSize(vLevel2) / Aig_ManRegNum(p->pAig) );
    }
    // try reducing the frames
    pManNew = Saig_ManDupWithCubes( p->pAig, p->vReg2Value );
//    Ioa_WriteAiger( pManNew, "aigcube.aig", 0, 0 );
    Aig_ManStop( pManNew );
}

static inline void Saig_ObjCexMinSet0( Aig_Obj_t * pObj ) { pObj->fMarkA = 1; pObj->fMarkB = 0;    }
static inline void Saig_ObjCexMinSet1( Aig_Obj_t * pObj ) { pObj->fMarkA = 0; pObj->fMarkB = 1;    }
static inline void Saig_ObjCexMinSetX( Aig_Obj_t * pObj ) { pObj->fMarkA = 1; pObj->fMarkB = 1;    }

static inline int  Saig_ObjCexMinGet0( Aig_Obj_t * pObj ) { return  pObj->fMarkA && !pObj->fMarkB; }
static inline int  Saig_ObjCexMinGet1( Aig_Obj_t * pObj ) { return !pObj->fMarkA &&  pObj->fMarkB; }
static inline int  Saig_ObjCexMinGetX( Aig_Obj_t * pObj ) { return  pObj->fMarkA &&  pObj->fMarkB; }

static inline int  Saig_ObjCexMinGet0Fanin0( Aig_Obj_t * pObj ) { return (Saig_ObjCexMinGet1(Aig_ObjFanin0(pObj)) && Aig_ObjFaninC0(pObj)) || (Saig_ObjCexMinGet0(Aig_ObjFanin0(pObj)) && !Aig_ObjFaninC0(pObj)); }
static inline int  Saig_ObjCexMinGet1Fanin0( Aig_Obj_t * pObj ) { return (Saig_ObjCexMinGet0(Aig_ObjFanin0(pObj)) && Aig_ObjFaninC0(pObj)) || (Saig_ObjCexMinGet1(Aig_ObjFanin0(pObj)) && !Aig_ObjFaninC0(pObj)); }

static inline int  Saig_ObjCexMinGet0Fanin1( Aig_Obj_t * pObj ) { return (Saig_ObjCexMinGet1(Aig_ObjFanin1(pObj)) && Aig_ObjFaninC1(pObj)) || (Saig_ObjCexMinGet0(Aig_ObjFanin1(pObj)) && !Aig_ObjFaninC1(pObj)); }
static inline int  Saig_ObjCexMinGet1Fanin1( Aig_Obj_t * pObj ) { return (Saig_ObjCexMinGet0(Aig_ObjFanin1(pObj)) && Aig_ObjFaninC1(pObj)) || (Saig_ObjCexMinGet1(Aig_ObjFanin1(pObj)) && !Aig_ObjFaninC1(pObj)); }

static inline void Saig_ObjCexMinSim( Aig_Obj_t * pObj )
{
    if ( Aig_ObjIsAnd(pObj) )
    {
        if ( Saig_ObjCexMinGet0Fanin0(pObj) || Saig_ObjCexMinGet0Fanin1(pObj) )
            Saig_ObjCexMinSet0( pObj );
        else if ( Saig_ObjCexMinGet1Fanin0(pObj) && Saig_ObjCexMinGet1Fanin1(pObj) )
            Saig_ObjCexMinSet1( pObj );
        else 
            Saig_ObjCexMinSetX( pObj );
    }
    else if ( Aig_ObjIsCo(pObj) )
    {
        if ( Saig_ObjCexMinGet0Fanin0(pObj) )
            Saig_ObjCexMinSet0( pObj );
        else if ( Saig_ObjCexMinGet1Fanin0(pObj) )
            Saig_ObjCexMinSet1( pObj );
        else 
            Saig_ObjCexMinSetX( pObj );
    }
    else assert( 0 );
}

static inline void Saig_ObjCexMinPrint( Aig_Obj_t * pObj )
{
    if ( Saig_ObjCexMinGet0(pObj) )
        printf( "0" );
    else if ( Saig_ObjCexMinGet1(pObj) )
        printf( "1" );
    else if ( Saig_ObjCexMinGetX(pObj) )
        printf( "X" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManCexVerifyUsingTernary( Aig_Man_t * pAig, Abc_Cex_t * pCex, Abc_Cex_t * pCare )
{
    Aig_Obj_t * pObj, * pObjRi, * pObjRo;
    int i, f, iBit = 0;
    assert( pCex->iFrame == pCare->iFrame );
    assert( pCex->nBits  == pCare->nBits );
    assert( pCex->iPo < Saig_ManPoNum(pAig) );
    Saig_ObjCexMinSet1( Aig_ManConst1(pAig) );
    // set flops to the init state
    Saig_ManForEachLo( pAig, pObj, i )
    {
        assert( !Abc_InfoHasBit(pCex->pData, iBit) );
        assert( !Abc_InfoHasBit(pCare->pData, iBit) );
//        if ( Abc_InfoHasBit(pCare->pData, iBit++) )
            Saig_ObjCexMinSet0( pObj );
//        else
//            Saig_ObjCexMinSetX( pObj );
    }
    iBit = pCex->nRegs;
    for ( f = 0; f <= pCex->iFrame; f++ )
    {
        // init inputs
        Saig_ManForEachPi( pAig, pObj, i )
        {
            if ( Abc_InfoHasBit(pCare->pData, iBit++) )
            {
                if ( Abc_InfoHasBit(pCex->pData, iBit-1) )
                    Saig_ObjCexMinSet1( pObj );
                else
                    Saig_ObjCexMinSet0( pObj );
            }
            else
                Saig_ObjCexMinSetX( pObj );
        }
        // simulate internal nodes
        Aig_ManForEachNode( pAig, pObj, i )
            Saig_ObjCexMinSim( pObj );
        // simulate COs
        Aig_ManForEachCo( pAig, pObj, i )
            Saig_ObjCexMinSim( pObj );
/*
        Aig_ManForEachObj( pAig, pObj, i )
        {
            Aig_ObjPrint(pAig, pObj);
            printf( "  Value = " );
            Saig_ObjCexMinPrint( pObj );
            printf( "\n" );
        }
*/
        // transfer
        Saig_ManForEachLiLo( pAig, pObjRi, pObjRo, i )
            pObjRo->fMarkA = pObjRi->fMarkA,
            pObjRo->fMarkB = pObjRi->fMarkB;
    }
    assert( iBit == pCex->nBits );
    return Saig_ObjCexMinGet1( Aig_ManCo( pAig, pCex->iPo ) );
}

/**Function*************************************************************

  Synopsis    [SAT-based refinement of the counter-example.]

  Description [The first parameter (nInputs) indicates how many first 
  primary inputs to skip without considering as care candidates.]
               

  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Saig_ManCbaFindCexCareBits( Aig_Man_t * pAig, Abc_Cex_t * pCex, int nInputs, int fVerbose )
{
    Saig_ManCba_t * p;
    Vec_Int_t * vReasons;
    Abc_Cex_t * pCare;
    abctime clk = Abc_Clock();

    clk = Abc_Clock();
    p = Saig_ManCbaStart( pAig, pCex, nInputs, fVerbose );

//    p->vReg2Frame = Vec_VecStart( pCex->iFrame );
//    p->vReg2Value = Vec_VecStart( pCex->iFrame );
    p->pFrames = Saig_ManCbaUnrollWithCex( pAig, pCex, nInputs, &p->vMapPiF2A, &p->vReg2Frame );
    vReasons = Saig_ManCbaFindReason( p );
    if ( p->vReg2Frame )
        Saig_ManCbaShrink( p );


//if ( fVerbose )
//Aig_ManPrintStats( p->pFrames );

    if ( fVerbose )
    {
        Vec_Int_t * vRes = Saig_ManCbaReason2Inputs( p, vReasons );
        printf( "Frame PIs = %4d (essential = %4d)   AIG PIs = %4d (essential = %4d)   ",
            Aig_ManCiNum(p->pFrames), Vec_IntSize(vReasons), 
            Saig_ManPiNum(p->pAig) - p->nInputs, Vec_IntSize(vRes) );
        Vec_IntFree( vRes );
ABC_PRT( "Time", Abc_Clock() - clk );
    }

    pCare = Saig_ManCbaReason2Cex( p, vReasons );
    Vec_IntFree( vReasons );
    Saig_ManCbaStop( p );

if ( fVerbose )
{
printf( "Real " );
Abc_CexPrintStats( pCex );
}
if ( fVerbose )
{
printf( "Care " );
Abc_CexPrintStats( pCare );
}
/*
    // verify the reduced counter-example using ternary simulation
    if ( !Saig_ManCexVerifyUsingTernary( pAig, pCex, pCare ) )
        printf( "Saig_ManCbaFindCexCareBits(): Minimized counter-example verification has failed!!!\n" );
    else if ( fVerbose )
        printf( "Saig_ManCbaFindCexCareBits(): Minimized counter-example verification is successful.\n" );
*/
    Aig_ManCleanMarkAB( pAig );
    return pCare;
}

/**Function*************************************************************

  Synopsis    [Returns the array of PIs for flops that should not be absracted.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_ManCbaFilterInputs( Aig_Man_t * pAig, int iFirstFlopPi, Abc_Cex_t * pCex, int fVerbose )
{
    Saig_ManCba_t * p;
    Vec_Int_t * vRes, * vReasons;
    abctime clk;
    if ( Saig_ManPiNum(pAig) != pCex->nPis )
    {
        printf( "Saig_ManCbaFilterInputs(): The PI count of AIG (%d) does not match that of cex (%d).\n", 
            Aig_ManCiNum(pAig), pCex->nPis );
        return NULL;
    }

clk = Abc_Clock();
    p = Saig_ManCbaStart( pAig, pCex, iFirstFlopPi, fVerbose );
    p->pFrames = Saig_ManCbaUnrollWithCex( pAig, pCex, iFirstFlopPi, &p->vMapPiF2A, &p->vReg2Frame );
    vReasons = Saig_ManCbaFindReason( p );
    vRes = Saig_ManCbaReason2Inputs( p, vReasons );
    if ( fVerbose )
    {
        printf( "Frame PIs = %4d (essential = %4d)   AIG PIs = %4d (essential = %4d)   ",
            Aig_ManCiNum(p->pFrames), Vec_IntSize(vReasons), 
            Saig_ManPiNum(p->pAig) - p->nInputs, Vec_IntSize(vRes) );
ABC_PRT( "Time", Abc_Clock() - clk );
    }

    Vec_IntFree( vReasons );
    Saig_ManCbaStop( p );
    return vRes;
}




/**Function*************************************************************

  Synopsis    [Checks the abstracted model for a counter-example.]

  Description [Returns the array of abstracted flops that should be added
  to the abstraction.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_ManCbaPerform( Aig_Man_t * pAbs, int nInputs, Saig_ParBmc_t * pPars )
{
    Vec_Int_t * vAbsFfsToAdd;
    int RetValue;
    abctime clk = Abc_Clock();
//    assert( pAbs->nRegs > 0 );
    // perform BMC
    RetValue = Saig_ManBmcScalable( pAbs, pPars );
    if ( RetValue == -1 ) // time out - nothing to add
    {
        printf( "Resource limit is reached during BMC.\n" );
        assert( pAbs->pSeqModel == NULL );
        return Vec_IntAlloc( 0 );
    }
    if ( pAbs->pSeqModel == NULL )
    {
        printf( "BMC did not detect a CEX with the given depth.\n" );
        return Vec_IntAlloc( 0 );
    }
    if ( pPars->fVerbose )
        Abc_CexPrintStats( pAbs->pSeqModel );
    // CEX is detected - refine the flops
    vAbsFfsToAdd = Saig_ManCbaFilterInputs( pAbs, nInputs, pAbs->pSeqModel, pPars->fVerbose );
    if ( Vec_IntSize(vAbsFfsToAdd) == 0 )
    {
        Vec_IntFree( vAbsFfsToAdd );
        return NULL;
    }
    if ( pPars->fVerbose )
    {
        printf( "Adding %d registers to the abstraction (total = %d).  ", 
            Vec_IntSize(vAbsFfsToAdd), Aig_ManRegNum(pAbs)+Vec_IntSize(vAbsFfsToAdd) );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    return vAbsFfsToAdd;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

