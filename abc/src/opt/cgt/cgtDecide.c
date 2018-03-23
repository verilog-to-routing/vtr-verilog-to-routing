/**CFile****************************************************************

  FileName    [cgtMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Clock gating package.]

  Synopsis    [Decide what gate to use for what flop.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cgtMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cgtInt.h"
#include "proof/ssw/sswInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern int Ssw_SmlCheckXorImplication( Ssw_Sml_t * p, Aig_Obj_t * pObjLi, Aig_Obj_t * pObjLo, Aig_Obj_t * pCand );
extern int Ssw_SmlCountXorImplication( Ssw_Sml_t * p, Aig_Obj_t * pObjLi, Aig_Obj_t * pObjLo, Aig_Obj_t * pCand );
extern int Ssw_SmlCountEqual( Ssw_Sml_t * p, Aig_Obj_t * pObjLi, Aig_Obj_t * pObjLo );
extern int Ssw_SmlNodeCountOnesReal( Ssw_Sml_t * p, Aig_Obj_t * pObj );
extern int Ssw_SmlNodeCountOnesRealVec( Ssw_Sml_t * p, Vec_Ptr_t * vObjs );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Collects POs in the transitive fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cgt_ManCollectFanoutPos_rec( Aig_Man_t * pAig, Aig_Obj_t * pObj, Vec_Ptr_t * vFanout )
{
    Aig_Obj_t * pFanout;
    int f, iFanout = -1;
    if ( Aig_ObjIsTravIdCurrent(pAig, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(pAig, pObj);
    if ( Aig_ObjIsCo(pObj) )
    {
        Vec_PtrPush( vFanout, pObj );
        return;
    }
    Aig_ObjForEachFanout( pAig, pObj, pFanout, iFanout, f )
        Cgt_ManCollectFanoutPos_rec( pAig, pFanout, vFanout );
}

/**Function*************************************************************

  Synopsis    [Collects POs in the transitive fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cgt_ManCollectFanoutPos( Aig_Man_t * pAig, Aig_Obj_t * pObj, Vec_Ptr_t * vFanout )
{
    Vec_PtrClear( vFanout );
    Aig_ManIncrementTravId( pAig );
    Cgt_ManCollectFanoutPos_rec( pAig, pObj, vFanout );
}

/**Function*************************************************************

  Synopsis    [Checks if all PO fanouts can be gated by this node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cgt_ManCheckGateComplete( Aig_Man_t * pAig, Vec_Vec_t * vGatesAll, Aig_Obj_t * pGate, Vec_Ptr_t * vFanout )
{
    Vec_Ptr_t * vGates;
    Aig_Obj_t * pObj;
    int i;
    Vec_PtrForEachEntry( Aig_Obj_t *, vFanout, pObj, i )
    {
        if ( Saig_ObjIsPo(pAig, pObj) )
            return 0;
        vGates = Vec_VecEntry( vGatesAll, Aig_ObjCioId(pObj) - Saig_ManPoNum(pAig) );
        if ( Vec_PtrFind( vGates, pGate ) == -1 )
            return 0;            
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes the set of complete clock gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Cgt_ManCompleteGates( Aig_Man_t * pAig, Vec_Vec_t * vGatesAll, int nOdcMax, int fVerbose )
{
    Vec_Ptr_t * vFanout, * vGatesFull;
    Aig_Obj_t * pGate, * pGateR;
    int i, k;
    vFanout    = Vec_PtrAlloc( 100 );
    vGatesFull = Vec_PtrAlloc( 100 );
    Vec_VecForEachEntry( Aig_Obj_t *, vGatesAll, pGate, i, k )
    {
        pGateR = Aig_Regular(pGate);
        if ( pGateR->fMarkA )
            continue;
        pGateR->fMarkA = 1;
        Cgt_ManCollectFanoutPos( pAig, pGateR, vFanout );
        if ( Cgt_ManCheckGateComplete( pAig, vGatesAll, pGate, vFanout ) )
            Vec_PtrPush( vGatesFull, pGate );
    }
    Vec_PtrFree( vFanout );
    Vec_VecForEachEntry( Aig_Obj_t *, vGatesAll, pGate, i, k )
        Aig_Regular(pGate)->fMarkA = 0;
    return vGatesFull;
}

/**Function*************************************************************

  Synopsis    [Calculates coverage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Cgt_ManComputeCoverage( Aig_Man_t * pAig, Vec_Vec_t * vGates )
{
    int nFrames = 32;
    int nWords  =  1;
    Ssw_Sml_t * pSml;
    Vec_Ptr_t * vOne;
    int i, nTransSaved = 0;
    pSml = Ssw_SmlSimulateSeq( pAig, 0, nFrames, nWords );
    Vec_VecForEachLevel( vGates, vOne, i )
        nTransSaved += Ssw_SmlNodeCountOnesRealVec( pSml, vOne );
    Ssw_SmlStop( pSml );
    return (float)100.0*nTransSaved/32/nFrames/nWords/Vec_VecSize(vGates);
}

/**Function*************************************************************

  Synopsis    [Chooses what clock-gate to use for this register.]

  Description [Currently uses the naive approach: For each register, 
  choose the clock gate, which covers most of the transitions.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Cgt_ManDecideSimple( Aig_Man_t * pAig, Vec_Vec_t * vGatesAll, int nOdcMax, int fVerbose )
{
    int nFrames = 32;
    int nWords  =  1;
    Ssw_Sml_t * pSml;
    Vec_Vec_t * vGates;
    Vec_Ptr_t * vCands;
    Aig_Obj_t * pObjLi, * pObjLo, * pCand, * pCandBest;
    int i, k, nHitsCur, nHitsMax, Counter = 0;
    abctime clk = Abc_Clock();
    int nTransTotal = 0, nTransSaved = 0;
    vGates = Vec_VecStart( Saig_ManRegNum(pAig) );
    pSml = Ssw_SmlSimulateSeq( pAig, 0, nFrames, nWords );
    Saig_ManForEachLiLo( pAig, pObjLi, pObjLo, i )
    {
        nHitsMax = 0;
        pCandBest = NULL;
        vCands = Vec_VecEntry( vGatesAll, i );
        Vec_PtrForEachEntry( Aig_Obj_t *, vCands, pCand, k )
        {
            // check if this is indeed a clock-gate
            if ( nOdcMax == 0 && !Ssw_SmlCheckXorImplication( pSml, pObjLi, pObjLo, pCand ) )
                printf( "Clock gate candidate is invalid!\n" );
            // find its characteristic number
            nHitsCur = Ssw_SmlNodeCountOnesReal( pSml, pCand );
            if ( nHitsMax < nHitsCur )
            {
                nHitsMax = nHitsCur;
                pCandBest = pCand;
            }
        }
        if ( pCandBest != NULL )
        {
            Vec_VecPush( vGates, i, pCandBest );
            Counter++;
            nTransSaved += nHitsMax;
        }
        nTransTotal += 32 * nFrames * nWords;
    }
    Ssw_SmlStop( pSml );
    if ( fVerbose )
    {
        printf( "Gating signals = %6d. Gated flops = %6d. (Total flops = %6d.)\n", 
            Vec_VecSizeSize(vGatesAll), Counter, Saig_ManRegNum(pAig) );
//        printf( "Gated transitions = %5.2f %%. (%5.2f %%.) ", 
//            100.0*nTransSaved/nTransTotal, Cgt_ManComputeCoverage(pAig, vGates) );
        printf( "Gated transitions = %5.2f %%. ", Cgt_ManComputeCoverage(pAig, vGates) );
        ABC_PRT( "Time", Abc_Clock() - clk );
    }
/*
    {
        Vec_Ptr_t * vCompletes;
        vCompletes = Cgt_ManCompleteGates( pAig, vGatesAll, nOdcMax, fVerbose );
        printf( "Complete gates = %d. \n", Vec_PtrSize(vCompletes) );
        Vec_PtrFree( vCompletes );
    }
*/
    return vGates;
}

/**Function*************************************************************

  Synopsis    [Computes the set of complete clock gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Cgt_ManDecideArea( Aig_Man_t * pAig, Vec_Vec_t * vGatesAll, int nOdcMax, int fVerbose )
{
    Vec_Vec_t * vGates;
    Vec_Ptr_t * vCompletes, * vOne;
    Aig_Obj_t * pGate;
    int i, k, Counter = 0;
    abctime clk = Abc_Clock();
    // derive and label complete gates
    vCompletes = Cgt_ManCompleteGates( pAig, vGatesAll, nOdcMax, fVerbose );
    // label complete gates
    Vec_PtrForEachEntry( Aig_Obj_t *, vCompletes, pGate, i )
        Aig_Regular(pGate)->fMarkA = 1;
    // select only complete gates
    vGates = Vec_VecStart( Saig_ManRegNum(pAig) );
    Vec_VecForEachEntry( Aig_Obj_t *, vGatesAll, pGate, i, k )
        if ( Aig_Regular(pGate)->fMarkA )
            Vec_VecPush( vGates, i, pGate );
    // unlabel complete gates
    Vec_PtrForEachEntry( Aig_Obj_t *, vCompletes, pGate, i )
        Aig_Regular(pGate)->fMarkA = 0;
    // count the number of gated flops
    Vec_VecForEachLevel( vGates, vOne, i )
    {
        Counter += (int)(Vec_PtrSize(vOne) > 0);
//        printf( "%d ", Vec_PtrSize(vOne) );
    }
//    printf( "\n" );
    if ( fVerbose )
    {
        printf( "Gating signals = %6d. Gated flops = %6d. (Total flops = %6d.)\n", 
            Vec_VecSizeSize(vGatesAll), Counter, Saig_ManRegNum(pAig) );
        printf( "Complete gates = %6d. Gated transitions = %5.2f %%. ", 
            Vec_PtrSize(vCompletes), Cgt_ManComputeCoverage(pAig, vGates) );
        ABC_PRT( "Time", Abc_Clock() - clk );
    }
    Vec_PtrFree( vCompletes );
    return vGates;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

