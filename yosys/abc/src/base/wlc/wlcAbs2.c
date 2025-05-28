/**CFile****************************************************************

  FileName    [wlcAbs2.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Abstraction for word-level networks.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: wlcAbs2.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wlc.h"
#include "proof/pdr/pdr.h"
#include "aig/gia/giaAig.h"
#include "sat/bmc/bmc.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Mark operators that meet the abstraction criteria.]

  Description [This procedure returns the array of objects (vLeaves) that 
  should be abstracted because of their high bit-width. It uses input array (vUnmark)
  to not abstract those objects that have been refined in the previous rounds.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Vec_Bit_t * Wlc_NtkAbsMarkOpers( Wlc_Ntk_t * p, Wlc_Par_t * pPars, Vec_Bit_t * vUnmark, int fVerbose )
{
    Vec_Bit_t * vLeaves = Vec_BitStart( Wlc_NtkObjNumMax(p) );
    Wlc_Obj_t * pObj; int i, Count[4] = {0};
    Wlc_NtkForEachObj( p, pObj, i )
    {
        if ( vUnmark && Vec_BitEntry(vUnmark, i) ) // not allow this object to be abstracted away
            continue;
        if ( pObj->Type == WLC_OBJ_ARI_ADD || pObj->Type == WLC_OBJ_ARI_SUB || pObj->Type == WLC_OBJ_ARI_MINUS )
        {
            if ( Wlc_ObjRange(pObj) >= pPars->nBitsAdd )
                Vec_BitWriteEntry( vLeaves, Wlc_ObjId(p, pObj), 1 ), Count[0]++;
            continue;
        }
        if ( pObj->Type == WLC_OBJ_ARI_MULTI || pObj->Type == WLC_OBJ_ARI_DIVIDE || pObj->Type == WLC_OBJ_ARI_REM || pObj->Type == WLC_OBJ_ARI_MODULUS )
        {
            if ( Wlc_ObjRange(pObj) >= pPars->nBitsMul )
                Vec_BitWriteEntry( vLeaves, Wlc_ObjId(p, pObj), 1 ), Count[1]++;
            continue;
        }
        if ( pObj->Type == WLC_OBJ_MUX )
        {
            if ( Wlc_ObjRange(pObj) >= pPars->nBitsMux )
                Vec_BitWriteEntry( vLeaves, Wlc_ObjId(p, pObj), 1 ), Count[2]++;
            continue;
        }
        if ( Wlc_ObjIsCi(pObj) && !Wlc_ObjIsPi(pObj) )
        {
            if ( Wlc_ObjRange(pObj) >= pPars->nBitsFlop )
                Vec_BitWriteEntry( vLeaves, Wlc_ObjId(p, pObj), 1 ), Count[3]++;
            continue;
        }
    }
    if ( fVerbose )
        printf( "Abstraction engine marked %d adds/subs, %d muls/divs, %d muxes, and %d flops to be abstracted away.\n", Count[0], Count[1], Count[2], Count[3] );
    return vLeaves;
}

/**Function*************************************************************

  Synopsis    [Marks nodes to be included in the abstracted network.]

  Description [Marks all objects that will be included in the abstracted model.  
  Stops at the objects (vLeaves) that are abstracted away. Returns three arrays:
  a subset of original PIs (vPisOld), a subset of pseudo-PIs (vPisNew) and the
  set of flops present as flops in the abstracted network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Wlc_NtkAbsMarkNodes_rec( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, Vec_Bit_t * vLeaves, Vec_Int_t * vPisOld, Vec_Int_t * vPisNew, Vec_Int_t * vFlops )
{
    int i, iFanin;
    if ( pObj->Mark )
        return;
    pObj->Mark = 1;
    if ( Vec_BitEntry(vLeaves, Wlc_ObjId(p, pObj)) )
    {
        assert( !Wlc_ObjIsPi(pObj) );
        Vec_IntPush( vPisNew, Wlc_ObjId(p, pObj) );
        return;
    }
    if ( Wlc_ObjIsCi(pObj) )
    {
        if ( Wlc_ObjIsPi(pObj) )
            Vec_IntPush( vPisOld, Wlc_ObjId(p, pObj) );
        else
            Vec_IntPush( vFlops, Wlc_ObjId(p, pObj) );
        return;
    }
    Wlc_ObjForEachFanin( pObj, iFanin, i )
        Wlc_NtkAbsMarkNodes_rec( p, Wlc_NtkObj(p, iFanin), vLeaves, vPisOld, vPisNew, vFlops );
}
static void Wlc_NtkAbsMarkNodes( Wlc_Ntk_t * p, Vec_Bit_t * vLeaves, Vec_Int_t * vPisOld, Vec_Int_t * vPisNew, Vec_Int_t * vFlops )
{
    Wlc_Obj_t * pObj;
    int i, Count = 0;
    Wlc_NtkCleanMarks( p );
    Wlc_NtkForEachCo( p, pObj, i )
        Wlc_NtkAbsMarkNodes_rec( p, pObj, vLeaves, vPisOld, vPisNew, vFlops );
    Wlc_NtkForEachObjVec( vFlops, p, pObj, i )
        Wlc_NtkAbsMarkNodes_rec( p, Wlc_ObjFo2Fi(p, pObj), vLeaves, vPisOld, vPisNew, vFlops );
    Wlc_NtkForEachObj( p, pObj, i )
        Count += pObj->Mark;
//    printf( "Collected %d old PIs, %d new PIs, %d flops, and %d other objects.\n", 
//        Vec_IntSize(vPisOld), Vec_IntSize(vPisNew), Vec_IntSize(vFlops), 
//        Count - Vec_IntSize(vPisOld) - Vec_IntSize(vPisNew) - Vec_IntSize(vFlops) );
    Vec_IntSort( vPisOld, 0 );
    Vec_IntSort( vPisNew, 0 );
    Vec_IntSort( vFlops, 0 );
    Wlc_NtkCleanMarks( p );
}

/**Function*************************************************************

  Synopsis    [Derive word-level abstracted model based on the parameter values.]

  Description [Retuns the word-level abstracted network and the set of pseudo-PIs 
  (vPisNew), which were created during abstraction. If the abstraction is
  satisfiable, some of the pseudo-PIs will be un-abstracted. These pseudo-PIs
  and their MFFC cones will be listed in the array (vUnmark), which will
  force the abstraction to not stop at these pseudo-PIs in the future.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Wlc_Ntk_t * Wlc_NtkAbs( Wlc_Ntk_t * p, Wlc_Par_t * pPars, Vec_Bit_t * vUnmark, Vec_Int_t ** pvPisNew, int fVerbose )
{
    Wlc_Ntk_t * pNtkNew = NULL;
    Vec_Int_t * vPisOld = Vec_IntAlloc( 100 );
    Vec_Int_t * vPisNew = Vec_IntAlloc( 100 );
    Vec_Int_t * vFlops  = Vec_IntAlloc( 100 );
    Vec_Bit_t * vLeaves = Wlc_NtkAbsMarkOpers( p, pPars, vUnmark, fVerbose );
    Wlc_NtkAbsMarkNodes( p, vLeaves, vPisOld, vPisNew, vFlops );
    Vec_BitFree( vLeaves );
    pNtkNew = Wlc_NtkDupDfsAbs( p, vPisOld, vPisNew, vFlops );
    Vec_IntFree( vPisOld );
    Vec_IntFree( vFlops );
    if ( pvPisNew )
        *pvPisNew = vPisNew;
    else
        Vec_IntFree( vPisNew );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Find what objects need to be un-abstracted.]

  Description [Returns a subset of pseudo-PIs (vPisNew), which will be 
  prevented from being abstracted in the future rounds of abstraction.
  The AIG manager (pGia) is a bit-level view of the abstracted model.
  The counter-example (pCex) is used to find waht PPIs to refine.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Vec_Int_t * Wlc_NtkAbsRefinement( Wlc_Ntk_t * p, Gia_Man_t * pGia, Abc_Cex_t * pCex, Vec_Int_t * vPisNew )
{
    Vec_Int_t * vRefine = Vec_IntAlloc( 100 );
    Abc_Cex_t * pCexCare;
    Wlc_Obj_t * pObj; 
    // count the number of bit-level PPIs and map them into word-level objects they were derived from
    int f, i, b, nRealPis, nPpiBits = 0;
    Vec_Int_t * vMap = Vec_IntStartFull( pCex->nPis );
    Wlc_NtkForEachObjVec( vPisNew, p, pObj, i )
        for ( b = 0; b < Wlc_ObjRange(pObj); b++ )
            Vec_IntWriteEntry( vMap, nPpiBits++, Wlc_ObjId(p, pObj) );
    // since PPIs are ordered last, the previous bits are real PIs
    nRealPis = pCex->nPis - nPpiBits;
    // find the care-set
    pCexCare = Bmc_CexCareMinimizeAig( pGia, nRealPis, pCex, 1, 0, 0 );
    assert( pCexCare->nPis == pCex->nPis );
    // detect care PPIs
    for ( f = 0; f <= pCexCare->iFrame; f++ )
        for ( i = nRealPis; i < pCexCare->nPis; i++ )
            if ( Abc_InfoHasBit(pCexCare->pData, pCexCare->nRegs + pCexCare->nPis * f + i) )
                Vec_IntPushUniqueOrder( vRefine, Vec_IntEntry(vMap, i-nRealPis) );
    Abc_CexFree( pCexCare );
    Vec_IntFree( vMap );
    if ( Vec_IntSize(vRefine) == 0 )// real CEX
        Vec_IntFreeP( &vRefine );
    return vRefine;
}

/**Function*************************************************************

  Synopsis    [Mark MFFC cones of the un-abstracted objects.]

  Description [The MFFC cones of the objects in vRefine are traversed
  and all their nodes are marked in vUnmark.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Wlc_NtkNodeDeref_rec( Wlc_Ntk_t * p, Wlc_Obj_t * pNode, Vec_Bit_t * vUnmark )
{
    int i, Fanin, Counter = 1;
    if ( Wlc_ObjIsCi(pNode) )
        return 0;
    Vec_BitWriteEntry( vUnmark, Wlc_ObjId(p, pNode), 1 );
    Wlc_ObjForEachFanin( pNode, Fanin, i )
    {
        Vec_IntAddToEntry( &p->vRefs, Fanin, -1 );
        if ( Vec_IntEntry(&p->vRefs, Fanin) == 0 )
            Counter += Wlc_NtkNodeDeref_rec( p, Wlc_NtkObj(p, Fanin), vUnmark );
    }
    return Counter;
}
static int Wlc_NtkNodeRef_rec( Wlc_Ntk_t * p, Wlc_Obj_t * pNode )
{
    int i, Fanin, Counter = 1;
    if ( Wlc_ObjIsCi(pNode) )
        return 0;
    Wlc_ObjForEachFanin( pNode, Fanin, i )
    {
        if ( Vec_IntEntry(&p->vRefs, Fanin) == 0 )
            Counter += Wlc_NtkNodeRef_rec( p, Wlc_NtkObj(p, Fanin) );
        Vec_IntAddToEntry( &p->vRefs, Fanin, 1 );
    }
    return Counter;
}
static int Wlc_NtkMarkMffc( Wlc_Ntk_t * p, Wlc_Obj_t * pNode, Vec_Bit_t * vUnmark )
{
    int Count1, Count2;
    // if this is a flop output, compute MFFC of the corresponding flop input
    while ( Wlc_ObjIsCi(pNode) )
    {
        Vec_BitWriteEntry( vUnmark, Wlc_ObjId(p, pNode), 1 );
        pNode = Wlc_ObjFo2Fi(p, pNode);
    }
    assert( !Wlc_ObjIsCi(pNode) );
    // dereference the node (and set the bits in vUnmark)
    Count1 = Wlc_NtkNodeDeref_rec( p, pNode, vUnmark );
    // reference it back
    Count2 = Wlc_NtkNodeRef_rec( p, pNode );
    assert( Count1 == Count2 );
    return Count1;
}
static int Wlc_NtkRemoveFromAbstraction( Wlc_Ntk_t * p, Vec_Int_t * vRefine, Vec_Bit_t * vUnmark )
{
    Wlc_Obj_t * pObj; int i, nNodes = 0;
    if ( Vec_IntSize(&p->vRefs) == 0 )
        Wlc_NtkSetRefs( p );
    Wlc_NtkForEachObjVec( vRefine, p, pObj, i )
        nNodes += Wlc_NtkMarkMffc( p, pObj, vUnmark );
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Performs abstraction.]

  Description [Derives initial abstraction based on user-specified
  parameter values, which tell what is the smallest bit-width of a
  primitive that is being abstracted away.  Currently only add/sub,
  mul/div, mux, and flop are supported with individual parameters.
  The second step is to refine the initial abstraction until the
  point when the property is proved.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wlc_NtkAbsCore2( Wlc_Ntk_t * p, Wlc_Par_t * pPars )
{
    abctime clk = Abc_Clock();
    int nIters, nNodes, nDcFlops, RetValue = -1;
    // start the bitmap to mark objects that cannot be abstracted because of refinement
    // currently, this bitmap is empty because abstraction begins without refinement
    Vec_Bit_t * vUnmark = Vec_BitStart( Wlc_NtkObjNumMax(p) );
    // set up parameters to run PDR
    Pdr_Par_t PdrPars, * pPdrPars = &PdrPars;
    Pdr_ManSetDefaultParams( pPdrPars );
    pPdrPars->fUseAbs    = 1;   // use 'pdr -t'  (on-the-fly abstraction)
    pPdrPars->fCtgs      = 1;   // use 'pdr -nc' (improved generalization)
    pPdrPars->fSkipDown  = 0;   // use 'pdr -nc' (improved generalization)
    //pPdrPars->nRestLimit = 500; // reset queue or proof-obligations when it gets larger than this
    pPdrPars->fVerbose   = pPars->fPdrVerbose;
    // perform refinement iterations
    for ( nIters = 1; nIters < pPars->nIterMax; nIters++ )
    {
        Aig_Man_t * pAig;
        Abc_Cex_t * pCex;
        Vec_Int_t * vPisNew, * vRefine;  
        Gia_Man_t * pGia, * pTemp;
        Wlc_Ntk_t * pAbs;

        if ( pPars->fVerbose )
            printf( "\nIteration %d:\n", nIters );

        // get abstracted GIA and the set of pseudo-PIs (vPisNew)
        pAbs = Wlc_NtkAbs( p, pPars, vUnmark, &vPisNew, pPars->fVerbose );
        pGia = Wlc_NtkBitBlast( pAbs, NULL );

        // if the abstraction has flops with DC-init state,
        // new PIs were introduced by bit-blasting at the end of the PI list
        // here we move these variables to be *before* PPIs, because
        // PPIs are supposed to be at the end of the PI list for refinement
        nDcFlops = Wlc_NtkDcFlopNum(pAbs);
        if ( nDcFlops > 0 ) // DC-init flops are present
        {
            pGia = Gia_ManPermuteInputs( pTemp = pGia, Wlc_NtkCountObjBits(p, vPisNew), nDcFlops );
            Gia_ManStop( pTemp );
        }
        // if the word-level outputs have to be XORs, this is a place to do it
        if ( pPars->fXorOutput )
        {
            pGia = Gia_ManTransformMiter2( pTemp = pGia );
            Gia_ManStop( pTemp );
        }
        if ( pPars->fVerbose )
        {
            printf( "Derived abstraction with %d objects and %d PPIs. Bit-blasted AIG stats are:\n", Wlc_NtkObjNum(pAbs), Vec_IntSize(vPisNew) ); 
            Gia_ManPrintStats( pGia, NULL );
        }
        Wlc_NtkFree( pAbs );

        // try to prove abstracted GIA by converting it to AIG and calling PDR
        pAig = Gia_ManToAigSimple( pGia );
        RetValue = Pdr_ManSolve( pAig, pPdrPars );
        pCex = pAig->pSeqModel; pAig->pSeqModel = NULL;
        Aig_ManStop( pAig );

        // consider outcomes
        if ( pCex == NULL ) 
        {
            assert( RetValue ); // proved or undecided
            Gia_ManStop( pGia );
            Vec_IntFree( vPisNew );
            break;
        }

        // perform refinement
        vRefine = Wlc_NtkAbsRefinement( p, pGia, pCex, vPisNew );
        Gia_ManStop( pGia );
        Vec_IntFree( vPisNew );
        if ( vRefine == NULL ) // real CEX
        {
            Abc_CexFree( pCex ); // return CEX in the future
            break;
        }

        // update the set of objects to be un-abstracted
        nNodes = Wlc_NtkRemoveFromAbstraction( p, vRefine, vUnmark );
        if ( pPars->fVerbose )
            printf( "Refinement of CEX in frame %d came up with %d un-abstacted PPIs, whose MFFCs include %d objects.\n", pCex->iFrame, Vec_IntSize(vRefine), nNodes );
        Vec_IntFree( vRefine );
        Abc_CexFree( pCex );
    }
    Vec_BitFree( vUnmark );
    // report the result
    if ( pPars->fVerbose )
        printf( "\n" );
    printf( "Abstraction " );
    if ( RetValue == 0 )
        printf( "resulted in a real CEX" );
    else if ( RetValue == 1 )
        printf( "is successfully proved" );
    else 
        printf( "timed out" );
    printf( " after %d iterations. ", nIters );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

