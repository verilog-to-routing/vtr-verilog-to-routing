/**CFile****************************************************************

  FileName    [saigRefSat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [SAT based refinement of a counter-example.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigRefSat.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abs.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// local manager
typedef struct Saig_RefMan_t_ Saig_RefMan_t;
struct Saig_RefMan_t_
{
    // user data
    Aig_Man_t * pAig;       // user's AIG
    Abc_Cex_t * pCex;       // user's CEX
    int         nInputs;    // the number of first inputs to skip
    int         fVerbose;   // verbose flag
    // unrolling
    Aig_Man_t * pFrames;    // unrolled timeframes
    Vec_Int_t * vMapPiF2A;  // mapping of frame PIs into real PIs
};

// performs ternary simulation
extern int Saig_ManSimDataInit( Aig_Man_t * p, Abc_Cex_t * pCex, Vec_Ptr_t * vSimInfo, Vec_Int_t * vRes );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Maps array of frame PI IDs into array of original PI IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_RefManReason2Inputs( Saig_RefMan_t * p, Vec_Int_t * vReasons )
{
    Vec_Int_t * vOriginal, * vVisited;
    int i, Entry;
    vOriginal = Vec_IntAlloc( Saig_ManPiNum(p->pAig) ); 
    vVisited = Vec_IntStart( Saig_ManPiNum(p->pAig) );
    Vec_IntForEachEntry( vReasons, Entry, i )
    {
        int iInput = Vec_IntEntry( p->vMapPiF2A, 2*Entry );
        assert( iInput >= 0 && iInput < Aig_ManCiNum(p->pAig) );
        if ( Vec_IntEntry(vVisited, iInput) == 0 )
            Vec_IntPush( vOriginal, iInput );
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
Abc_Cex_t * Saig_RefManReason2Cex( Saig_RefMan_t * p, Vec_Int_t * vReasons )
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
    return pCare;
}

/**Function*************************************************************

  Synopsis    [Returns reasons for the property to fail.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_RefManFindReason_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Int_t * vPrios, Vec_Int_t * vReasons )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Aig_ObjIsCi(pObj) )
    {
        Vec_IntPush( vReasons, Aig_ObjCioId(pObj) );
        return;
    }
    assert( Aig_ObjIsNode(pObj) );
    if ( pObj->fPhase )
    {
        Saig_RefManFindReason_rec( p, Aig_ObjFanin0(pObj), vPrios, vReasons );
        Saig_RefManFindReason_rec( p, Aig_ObjFanin1(pObj), vPrios, vReasons );
    }
    else
    {
        int fPhase0 = Aig_ObjFaninC0(pObj) ^ Aig_ObjFanin0(pObj)->fPhase;
        int fPhase1 = Aig_ObjFaninC1(pObj) ^ Aig_ObjFanin1(pObj)->fPhase;
        assert( !fPhase0 || !fPhase1 );
        if ( !fPhase0 && fPhase1 )
            Saig_RefManFindReason_rec( p, Aig_ObjFanin0(pObj), vPrios, vReasons );
        else if ( fPhase0 && !fPhase1 )
            Saig_RefManFindReason_rec( p, Aig_ObjFanin1(pObj), vPrios, vReasons );
        else 
        {
            int iPrio0 = Vec_IntEntry( vPrios, Aig_ObjFaninId0(pObj) );
            int iPrio1 = Vec_IntEntry( vPrios, Aig_ObjFaninId1(pObj) );
            if ( iPrio0 <= iPrio1 )
                Saig_RefManFindReason_rec( p, Aig_ObjFanin0(pObj), vPrios, vReasons );
            else
                Saig_RefManFindReason_rec( p, Aig_ObjFanin1(pObj), vPrios, vReasons );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Returns reasons for the property to fail.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_RefManFindReason( Saig_RefMan_t * p )
{
    Aig_Obj_t * pObj;
    Vec_Int_t * vPrios, * vPi2Prio, * vReasons;
    int i, CountPrios;

    vPi2Prio = Vec_IntStartFull( Saig_ManPiNum(p->pAig) );
    vPrios   = Vec_IntStartFull( Aig_ManObjNumMax(p->pFrames) );

    // set PI values according to CEX
    CountPrios = 0;
    Aig_ManConst1(p->pFrames)->fPhase = 1;
    Aig_ManForEachCi( p->pFrames, pObj, i )
    {
        int iInput = Vec_IntEntry( p->vMapPiF2A, 2*i );
        int iFrame = Vec_IntEntry( p->vMapPiF2A, 2*i+1 );
        pObj->fPhase = Abc_InfoHasBit( p->pCex->pData, p->pCex->nRegs + p->pCex->nPis * iFrame + iInput );
        // assign priority
        if ( Vec_IntEntry(vPi2Prio, iInput) == ~0 )
            Vec_IntWriteEntry( vPi2Prio, iInput, CountPrios++ );
//        Vec_IntWriteEntry( vPrios, Aig_ObjId(pObj), Vec_IntEntry(vPi2Prio, iInput) );
        Vec_IntWriteEntry( vPrios, Aig_ObjId(pObj), i );
    }
//    printf( "Priority numbers = %d.\n", CountPrios );
    Vec_IntFree( vPi2Prio );

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
    assert( (int)Aig_ObjFanin0(pObj)->fPhase == Aig_ObjFaninC0(pObj) );

    // select the reason
    vReasons = Vec_IntAlloc( 100 );
    Aig_ManIncrementTravId( p->pFrames );
    if ( !Aig_ObjIsConst1(Aig_ObjFanin0(pObj)) )
        Saig_RefManFindReason_rec( p->pFrames, Aig_ObjFanin0(pObj), vPrios, vReasons );
    Vec_IntFree( vPrios );
    return vReasons;
}


/**Function*************************************************************

  Synopsis    [Collect nodes in the unrolled timeframes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManUnrollCollect_rec( Aig_Man_t * pAig, Aig_Obj_t * pObj, Vec_Int_t * vObjs, Vec_Int_t * vRoots )
{
    if ( Aig_ObjIsTravIdCurrent(pAig, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(pAig, pObj);
    if ( Aig_ObjIsCo(pObj) )
        Saig_ManUnrollCollect_rec( pAig, Aig_ObjFanin0(pObj), vObjs, vRoots );
    else if ( Aig_ObjIsNode(pObj) )
    {
        Saig_ManUnrollCollect_rec( pAig, Aig_ObjFanin0(pObj), vObjs, vRoots );
        Saig_ManUnrollCollect_rec( pAig, Aig_ObjFanin1(pObj), vObjs, vRoots );
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
Aig_Man_t * Saig_ManUnrollWithCex( Aig_Man_t * pAig, Abc_Cex_t * pCex, int nInputs, Vec_Int_t ** pvMapPiF2A )
{
    Aig_Man_t * pFrames;     // unrolled timeframes
    Vec_Vec_t * vFrameCos;   // the list of COs per frame
    Vec_Vec_t * vFrameObjs;  // the list of objects per frame
    Vec_Int_t * vRoots, * vObjs;
    Aig_Obj_t * pObj;
    int i, f;
    // sanity checks
    assert( Saig_ManPiNum(pAig) == pCex->nPis );
    assert( Saig_ManRegNum(pAig) == pCex->nRegs );
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
            Saig_ManUnrollCollect_rec( pAig, pObj, 
                Vec_VecEntryInt(vFrameObjs, f),
                (Vec_Int_t *)(f ? Vec_VecEntry(vFrameCos, f-1) : NULL) );
    }

    // derive unrolled timeframes
    pFrames = Aig_ManStart( 10000 );
    pFrames->pName = Abc_UtilStrsav( pAig->pName );
    pFrames->pSpec = Abc_UtilStrsav( pAig->pSpec );
    // initialize the flops 
    Saig_ManForEachLo( pAig, pObj, i )
        pObj->pData = Aig_NotCond( Aig_ManConst1(pFrames), !Abc_InfoHasBit(pCex->pData, i) );
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
            Saig_ObjLiToLo( pAig, pObj )->pData = pObj->pData;
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
Saig_RefMan_t * Saig_RefManStart( Aig_Man_t * pAig, Abc_Cex_t * pCex, int nInputs, int fVerbose )
{
    Saig_RefMan_t * p;
    p = ABC_CALLOC( Saig_RefMan_t, 1 );
    p->pAig = pAig;
    p->pCex = pCex;
    p->nInputs = nInputs;
    p->fVerbose = fVerbose;
    p->pFrames = Saig_ManUnrollWithCex( pAig, pCex, nInputs, &p->vMapPiF2A );
    return p;
}

/**Function*************************************************************

  Synopsis    [Destroys refinement manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_RefManStop( Saig_RefMan_t * p )
{
    Aig_ManStopP( &p->pFrames );
    Vec_IntFreeP( &p->vMapPiF2A );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Sets phase bits in the timeframe AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_RefManSetPhases( Saig_RefMan_t * p, Abc_Cex_t * pCare, int fValue1 )
{
    Aig_Obj_t * pObj;
    int i, iFrame, iInput;
    Aig_ManConst1( p->pFrames )->fPhase = 1;
    Aig_ManForEachCi( p->pFrames, pObj, i )
    {
        iInput = Vec_IntEntry( p->vMapPiF2A, 2*i );
        iFrame = Vec_IntEntry( p->vMapPiF2A, 2*i+1 );
        pObj->fPhase = Abc_InfoHasBit( p->pCex->pData, p->pCex->nRegs + p->pCex->nPis * iFrame + iInput );
        // update value if it is a don't-care
        if ( pCare && !Abc_InfoHasBit( pCare->pData, p->pCex->nRegs + p->pCex->nPis * iFrame + iInput ) )
            pObj->fPhase = fValue1;
    }
    Aig_ManForEachNode( p->pFrames, pObj, i )
        pObj->fPhase = ( Aig_ObjFanin0(pObj)->fPhase ^ Aig_ObjFaninC0(pObj) )
                     & ( Aig_ObjFanin1(pObj)->fPhase ^ Aig_ObjFaninC1(pObj) );
    Aig_ManForEachCo( p->pFrames, pObj, i )
        pObj->fPhase = ( Aig_ObjFanin0(pObj)->fPhase ^ Aig_ObjFaninC0(pObj) );
    pObj = Aig_ManCo( p->pFrames, 0 );
    return pObj->fPhase;
}

/**Function*************************************************************

  Synopsis    [Tries to remove literals from abstraction.]

  Description [The literals are sorted more desirable first.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Saig_RefManOrderLiterals( Saig_RefMan_t * p, Vec_Int_t * vVar2PiId, Vec_Int_t * vAssumps )
{
    Vec_Vec_t * vLits;
    Vec_Int_t * vVar2New;
    int i, Entry, iInput, iFrame;
    // collect literals
    vLits = Vec_VecAlloc( 100 );
    vVar2New = Vec_IntStartFull( Saig_ManPiNum(p->pAig) );
    Vec_IntForEachEntry( vAssumps, Entry, i )
    {
        int iPiNum = Vec_IntEntry( vVar2PiId, lit_var(Entry) );
        assert( iPiNum >= 0 && iPiNum < Aig_ManCiNum(p->pFrames) );
        iInput = Vec_IntEntry( p->vMapPiF2A, 2*iPiNum );
        iFrame = Vec_IntEntry( p->vMapPiF2A, 2*iPiNum+1 );
//        Abc_InfoSetBit( pCare->pData, pCare->nRegs + pCare->nPis * iFrame + iInput );
        if ( Vec_IntEntry( vVar2New, iInput ) == ~0 )
            Vec_IntWriteEntry( vVar2New, iInput, Vec_VecSize(vLits) );
        Vec_VecPushInt( vLits, Vec_IntEntry( vVar2New, iInput ), Entry );
    }
    Vec_IntFree( vVar2New );
    return vLits;
}


/**Function*************************************************************

  Synopsis    [Generate the care set using SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Saig_RefManCreateCex( Saig_RefMan_t * p, Vec_Int_t * vVar2PiId, Vec_Int_t * vAssumps )
{
    Abc_Cex_t * pCare;
    int i, Entry, iInput, iFrame;
    // create counter-example
    pCare = Abc_CexDup( p->pCex, p->pCex->nRegs );
    memset( pCare->pData, 0, sizeof(unsigned) * Abc_BitWordNum(pCare->nBits) );
    Vec_IntForEachEntry( vAssumps, Entry, i )
    {
        int iPiNum = Vec_IntEntry( vVar2PiId, lit_var(Entry) );
        assert( iPiNum >= 0 && iPiNum < Aig_ManCiNum(p->pFrames) );
        iInput = Vec_IntEntry( p->vMapPiF2A, 2*iPiNum );
        iFrame = Vec_IntEntry( p->vMapPiF2A, 2*iPiNum+1 );
        Abc_InfoSetBit( pCare->pData, pCare->nRegs + pCare->nPis * iFrame + iInput );
    }
    return pCare;
}

/**Function*************************************************************

  Synopsis    [Generate the care set using SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Saig_RefManRunSat( Saig_RefMan_t * p, int fNewOrder )
{
    int nConfLimit = 1000000;
    Abc_Cex_t * pCare;
    Cnf_Dat_t * pCnf;
    sat_solver * pSat;
    Aig_Obj_t * pObj;
    Vec_Vec_t * vLits = NULL;
    Vec_Int_t * vAssumps, * vVar2PiId;
    int i, k, Entry, RetValue;//, f = 0, Counter = 0;
    int nCoreLits, * pCoreLits;
    abctime clk = Abc_Clock();
    // create CNF
    assert( Aig_ManRegNum(p->pFrames) == 0 );
//    pCnf = Cnf_Derive( p->pFrames, 0 ); // too slow
    pCnf = Cnf_DeriveSimple( p->pFrames, 0 );
    RetValue = Saig_RefManSetPhases( p, NULL, 0 );
    if ( RetValue )
    {
        printf( "Constructed frames are incorrect.\n" );
        Cnf_DataFree( pCnf );
        return NULL;
    }
    Cnf_DataTranformPolarity( pCnf, 0 );
    // create SAT solver
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    if ( pSat == NULL )
    {
        Cnf_DataFree( pCnf );
        return NULL;
    }
//Abc_PrintTime( 1, "Preparing", Abc_Clock() - clk );
    // look for a true counter-example
    if ( p->nInputs > 0 )
    {
        RetValue = sat_solver_solve( pSat, NULL, NULL, 
            (ABC_INT64_T)nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
        if ( RetValue == l_False )
        {
            printf( "The problem is trivially UNSAT. The CEX is real.\n" );
            // create counter-example
            pCare = Abc_CexDup( p->pCex, p->pCex->nRegs );
            memset( pCare->pData, 0, sizeof(unsigned) * Abc_BitWordNum(pCare->nBits) );
            return pCare;
        }
        // the problem is SAT - it is expected
    }
    // create assumptions
    vVar2PiId = Vec_IntStartFull( pCnf->nVars );
    vAssumps = Vec_IntAlloc( Aig_ManCiNum(p->pFrames) );
    Aig_ManForEachCi( p->pFrames, pObj, i )
    {
//        RetValue = Abc_InfoHasBit( p->pCex->pData, p->pCex->nRegs + p->pCex->nPis * iFrame + iInput );
//        Vec_IntPush( vAssumps, toLitCond( pCnf->pVarNums[Aig_ObjId(pObj)], !RetValue ) );
        Vec_IntPush( vAssumps, toLitCond( pCnf->pVarNums[Aig_ObjId(pObj)], 1 ) );
        Vec_IntWriteEntry( vVar2PiId, pCnf->pVarNums[Aig_ObjId(pObj)], i );
    }

    // reverse the order of assumptions
//    if ( fNewOrder )
//    Vec_IntReverseOrder( vAssumps );

    if ( fNewOrder )
    {
        // create literals
        vLits = Saig_RefManOrderLiterals( p, vVar2PiId, vAssumps );
        // sort literals
        Vec_VecSort( vLits, 1 );
        // save literals
        Vec_IntClear( vAssumps );
        Vec_VecForEachEntryInt( vLits, Entry, i, k )
            Vec_IntPush( vAssumps, Entry );

        for ( i = 0; i < Vec_VecSize(vLits); i++ )
            printf( "%d ", Vec_IntSize( Vec_VecEntryInt(vLits, i) ) );
        printf( "\n" );

        if ( p->fVerbose )
            printf( "Total PIs = %d. Essential PIs = %d.\n", 
                Saig_ManPiNum(p->pAig) - p->nInputs, Vec_VecSize(vLits) );
    }

    // solve
clk = Abc_Clock();
    RetValue = sat_solver_solve( pSat, Vec_IntArray(vAssumps), Vec_IntArray(vAssumps) + Vec_IntSize(vAssumps), 
        (ABC_INT64_T)nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
//Abc_PrintTime( 1, "Solving", Abc_Clock() - clk );
    if ( RetValue != l_False )
    {
        if ( RetValue == l_True )
            printf( "Internal Error!!! The resulting problem is SAT.\n" );
        else
            printf( "Internal Error!!! SAT solver timed out.\n" );
        Cnf_DataFree( pCnf );
        sat_solver_delete( pSat );
        Vec_IntFree( vAssumps );
        Vec_IntFree( vVar2PiId );
        return NULL;
    }
    assert( RetValue == l_False ); // UNSAT

    // get relevant SAT literals
    nCoreLits = sat_solver_final( pSat, &pCoreLits );
    assert( nCoreLits > 0 );
    if ( p->fVerbose )
    printf( "AnalizeFinal selected %d assumptions (out of %d). Conflicts = %d.\n", 
        nCoreLits, Vec_IntSize(vAssumps), (int)pSat->stats.conflicts );

    // save literals
    Vec_IntClear( vAssumps );
    for ( i = 0; i < nCoreLits; i++ )
        Vec_IntPush( vAssumps, pCoreLits[i] );


    // create literals
    vLits = Saig_RefManOrderLiterals( p, vVar2PiId, vAssumps );
    // sort literals
//    Vec_VecSort( vLits, 0 );
    // save literals
    Vec_IntClear( vAssumps );
    Vec_VecForEachEntryInt( vLits, Entry, i, k )
        Vec_IntPush( vAssumps, Entry );

//    for ( i = 0; i < Vec_VecSize(vLits); i++ )
//        printf( "%d ", Vec_IntSize( Vec_VecEntryInt(vLits, i) ) );
//    printf( "\n" );

    if ( p->fVerbose )
        printf( "Total PIs = %d. Essential PIs = %d.\n", 
            Saig_ManPiNum(p->pAig) - p->nInputs, Vec_VecSize(vLits) );
/*
    // try assumptions in different order
    RetValue = sat_solver_solve( pSat, Vec_IntArray(vAssumps), Vec_IntArray(vAssumps) + Vec_IntSize(vAssumps), 
        (ABC_INT64_T)nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    printf( "Assumpts = %2d. Intermediate instance is %5s. Conflicts = %2d.\n", 
        Vec_IntSize(vAssumps), (RetValue == l_False ? "UNSAT" : "SAT"), (int)pSat->stats.conflicts );

    // create different sets of assumptions
    Counter = Vec_VecSize(vLits);
    for ( f = 0; f < Vec_VecSize(vLits); f++ )
    {
        Vec_IntClear( vAssumps );
        Vec_VecForEachEntryInt( vLits, Entry, i, k )
            if ( i != f )
                Vec_IntPush( vAssumps, Entry );

        // try the new assumptions
        RetValue = sat_solver_solve( pSat, Vec_IntArray(vAssumps), Vec_IntArray(vAssumps) + Vec_IntSize(vAssumps), 
            (ABC_INT64_T)nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
        printf( "Assumpts = %2d. Intermediate instance is %5s. Conflicts = %2d.\n", 
            Vec_IntSize(vAssumps), RetValue == l_False ? "UNSAT" : "SAT", (int)pSat->stats.conflicts );
        if ( RetValue != l_False )
            continue;

        // UNSAT - remove literals
        Vec_IntClear( Vec_VecEntryInt(vLits, f) );
        Counter--;
    }

    for ( i = 0; i < Vec_VecSize(vLits); i++ )
        printf( "%d ", Vec_IntSize( Vec_VecEntryInt(vLits, i) ) );
    printf( "\n" );

    if ( p->fVerbose )
        printf( "Total PIs = %d. Essential PIs = %d.\n", 
            Saig_ManPiNum(p->pAig) - p->nInputs, Counter );

    // save literals
    Vec_IntClear( vAssumps );
    Vec_VecForEachEntryInt( vLits, Entry, i, k )
        Vec_IntPush( vAssumps, Entry );
*/
    // create counter-example
    pCare = Saig_RefManCreateCex( p, vVar2PiId, vAssumps );

    // cleanup
    Cnf_DataFree( pCnf );
    sat_solver_delete( pSat );
    Vec_IntFree( vAssumps );
    Vec_IntFree( vVar2PiId );
    Vec_VecFreeP( &vLits );

    // verify counter-example
    RetValue = Saig_RefManSetPhases( p, pCare, 0 );
    if ( RetValue )
        printf( "Reduced CEX verification has failed.\n" );
    RetValue = Saig_RefManSetPhases( p, pCare, 1 );
    if ( RetValue )
        printf( "Reduced CEX verification has failed.\n" );
    return pCare;
}


/**Function*************************************************************

  Synopsis    []

  Description []               

  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_RefManRefineWithSat( Saig_RefMan_t * p, Vec_Int_t * vAigPis )
{
    int nConfLimit = 1000000;
    Cnf_Dat_t * pCnf;
    sat_solver * pSat;
    Aig_Obj_t * pObj;
    Vec_Vec_t * vLits;
    Vec_Int_t * vReasons, * vAssumps, * vVisited, * vVar2PiId;
    int i, k, f, Entry, RetValue, Counter;

    // create CNF and SAT solver
    pCnf = Cnf_DeriveSimple( p->pFrames, 0 );
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    if ( pSat == NULL )
    {
        Cnf_DataFree( pCnf );
        return NULL;
    }

    // mark used AIG inputs
    vVisited = Vec_IntStart( Saig_ManPiNum(p->pAig) );
    Vec_IntForEachEntry( vAigPis, Entry, i )
    {
        assert( Entry >= 0 && Entry < Aig_ManCiNum(p->pAig) );
        Vec_IntWriteEntry( vVisited, Entry, 1 );
    }

    // create assumptions
    vVar2PiId = Vec_IntStartFull( pCnf->nVars );
    vAssumps = Vec_IntAlloc( Aig_ManCiNum(p->pFrames) );
    Aig_ManForEachCi( p->pFrames, pObj, i )
    {
        int iInput = Vec_IntEntry( p->vMapPiF2A, 2*i );
        int iFrame = Vec_IntEntry( p->vMapPiF2A, 2*i+1 );
        if ( Vec_IntEntry(vVisited, iInput) == 0 )
            continue;
        RetValue = Abc_InfoHasBit( p->pCex->pData, p->pCex->nRegs + p->pCex->nPis * iFrame + iInput );
        Vec_IntPush( vAssumps, toLitCond( pCnf->pVarNums[Aig_ObjId(pObj)], !RetValue ) );
//        Vec_IntPush( vAssumps, toLitCond( pCnf->pVarNums[Aig_ObjId(pObj)], 1 ) );
        Vec_IntWriteEntry( vVar2PiId, pCnf->pVarNums[Aig_ObjId(pObj)], i );
    }
    Vec_IntFree( vVisited );

    // try assumptions in different order
    RetValue = sat_solver_solve( pSat, Vec_IntArray(vAssumps), Vec_IntArray(vAssumps) + Vec_IntSize(vAssumps), 
        (ABC_INT64_T)nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    printf( "Assumpts = %2d. Intermediate instance is %5s. Conflicts = %2d.\n", 
        Vec_IntSize(vAssumps), (RetValue == l_False ? "UNSAT" : "SAT"), (int)pSat->stats.conflicts );

/*
    // AnalizeFinal does not work because it implications propagate directly
    // and SAT solver does not kick in (the number of conflicts in 0).

    // count the number of lits in the unsat core
    {
        int nCoreLits, * pCoreLits;
        nCoreLits = sat_solver_final( pSat, &pCoreLits );
        assert( nCoreLits > 0 );

        // count the number of flops
        vVisited = Vec_IntStart( Saig_ManPiNum(p->pAig) );
        for ( i = 0; i < nCoreLits; i++ )
        {
            int iPiNum = Vec_IntEntry( vVar2PiId, lit_var(pCoreLits[i]) );
            int iInput = Vec_IntEntry( p->vMapPiF2A, 2*iPiNum );
            int iFrame = Vec_IntEntry( p->vMapPiF2A, 2*iPiNum+1 );
            Vec_IntWriteEntry( vVisited, iInput, 1 );
        }
        // count the number of entries
        Counter = 0;
        Vec_IntForEachEntry( vVisited, Entry, i )
            Counter += Entry;
        Vec_IntFree( vVisited );

//        if ( p->fVerbose )
        printf( "AnalizeFinal: Assumptions %d (out of %d). Essential PIs = %d. Conflicts = %d.\n", 
            nCoreLits, Vec_IntSize(vAssumps), Counter, (int)pSat->stats.conflicts );
    }
*/

    // derive literals
    vLits = Saig_RefManOrderLiterals( p, vVar2PiId, vAssumps );
    for ( i = 0; i < Vec_VecSize(vLits); i++ )
        printf( "%d ", Vec_IntSize( Vec_VecEntryInt(vLits, i) ) );
    printf( "\n" );

    // create different sets of assumptions
    Counter = Vec_VecSize(vLits);
    for ( f = 0; f < Vec_VecSize(vLits); f++ )
    {
        Vec_IntClear( vAssumps );
        Vec_VecForEachEntryInt( vLits, Entry, i, k )
            if ( i != f )
                Vec_IntPush( vAssumps, Entry );

        // try the new assumptions
        RetValue = sat_solver_solve( pSat, Vec_IntArray(vAssumps), Vec_IntArray(vAssumps) + Vec_IntSize(vAssumps), 
            (ABC_INT64_T)nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
        printf( "Assumpts = %2d. Intermediate instance is %5s. Conflicts = %2d.\n", 
            Vec_IntSize(vAssumps), RetValue == l_False ? "UNSAT" : "SAT", (int)pSat->stats.conflicts );
        if ( RetValue != l_False )
            continue;

        // UNSAT - remove literals
        Vec_IntClear( Vec_VecEntryInt(vLits, f) );
        Counter--;
    }

    for ( i = 0; i < Vec_VecSize(vLits); i++ )
        printf( "%d ", Vec_IntSize( Vec_VecEntryInt(vLits, i) ) );
    printf( "\n" );

    // create assumptions
    Vec_IntClear( vAssumps );
    Vec_VecForEachEntryInt( vLits, Entry, i, k )
        Vec_IntPush( vAssumps, Entry );

    // try assumptions in different order
    RetValue = sat_solver_solve( pSat, Vec_IntArray(vAssumps), Vec_IntArray(vAssumps) + Vec_IntSize(vAssumps), 
        (ABC_INT64_T)nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    printf( "Assumpts = %2d. Intermediate instance is %5s. Conflicts = %2d.\n", 
        Vec_IntSize(vAssumps), (RetValue == l_False ? "UNSAT" : "SAT"), (int)pSat->stats.conflicts );

//    if ( p->fVerbose )
//        printf( "Total PIs = %d. Essential PIs = %d.\n", 
//            Saig_ManPiNum(p->pAig) - p->nInputs, Counter );


    // transform assumptions into reasons
    vReasons = Vec_IntAlloc( 100 );
    Vec_IntForEachEntry( vAssumps, Entry, i )
    {
        int iPiNum = Vec_IntEntry( vVar2PiId, lit_var(Entry) );
        assert( iPiNum >= 0 && iPiNum < Aig_ManCiNum(p->pFrames) );
        Vec_IntPush( vReasons, iPiNum );
    }

    // cleanup
    Cnf_DataFree( pCnf );
    sat_solver_delete( pSat );
    Vec_IntFree( vAssumps );
    Vec_IntFree( vVar2PiId );
    Vec_VecFreeP( &vLits );

    return vReasons;
}

/**Function*************************************************************

  Synopsis    [SAT-based refinement of the counter-example.]

  Description [The first parameter (nInputs) indicates how many first 
  primary inputs to skip without considering as care candidates.]
               

  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Saig_ManFindCexCareBits( Aig_Man_t * pAig, Abc_Cex_t * pCex, int nInputs, int fNewOrder, int fVerbose )
{
    Saig_RefMan_t * p;
    Vec_Int_t * vReasons;
    Abc_Cex_t * pCare;
    abctime clk = Abc_Clock();

    clk = Abc_Clock();
    p = Saig_RefManStart( pAig, pCex, nInputs, fVerbose );
    vReasons = Saig_RefManFindReason( p );

if ( fVerbose )
Aig_ManPrintStats( p->pFrames );

//    if ( fVerbose )
    {
        Vec_Int_t * vRes = Saig_RefManReason2Inputs( p, vReasons );
        printf( "Frame PIs = %4d (essential = %4d)   AIG PIs = %4d (essential = %4d)   ",
            Aig_ManCiNum(p->pFrames), Vec_IntSize(vReasons), 
            Saig_ManPiNum(p->pAig) - p->nInputs, Vec_IntSize(vRes) );
ABC_PRT( "Time", Abc_Clock() - clk );

        Vec_IntFree( vRes );

/*
        ////////////////////////////////////
        Vec_IntFree( vReasons );
        vReasons = Saig_RefManRefineWithSat( p, vRes );
        ////////////////////////////////////

        Vec_IntFree( vRes );
        vRes = Saig_RefManReason2Inputs( p, vReasons );
        printf( "Frame PIs = %4d (essential = %4d)   AIG PIs = %4d (essential = %4d)   ",
            Aig_ManCiNum(p->pFrames), Vec_IntSize(vReasons), 
            Saig_ManPiNum(p->pAig) - p->nInputs, Vec_IntSize(vRes) );

        Vec_IntFree( vRes );
ABC_PRT( "Time", Abc_Clock() - clk );
*/
    }

    pCare = Saig_RefManReason2Cex( p, vReasons );
    Vec_IntFree( vReasons );
    Saig_RefManStop( p );

if ( fVerbose )
Abc_CexPrintStats( pCex );
if ( fVerbose )
Abc_CexPrintStats( pCare );

    return pCare;
}

/**Function*************************************************************

  Synopsis    [Returns the array of PIs for flops that should not be absracted.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_ManExtendCounterExampleTest3( Aig_Man_t * pAig, int iFirstFlopPi, Abc_Cex_t * pCex, int fVerbose )
{
    Saig_RefMan_t * p;
    Vec_Int_t * vRes, * vReasons;
    abctime clk;
    if ( Saig_ManPiNum(pAig) != pCex->nPis )
    {
        printf( "Saig_ManExtendCounterExampleTest3(): The PI count of AIG (%d) does not match that of cex (%d).\n", 
            Aig_ManCiNum(pAig), pCex->nPis );
        return NULL;
    }

clk = Abc_Clock();

    p = Saig_RefManStart( pAig, pCex, iFirstFlopPi, fVerbose );
    vReasons = Saig_RefManFindReason( p );
    vRes = Saig_RefManReason2Inputs( p, vReasons );

//    if ( fVerbose )
    {
        printf( "Frame PIs = %4d (essential = %4d)   AIG PIs = %4d (essential = %4d)   ",
            Aig_ManCiNum(p->pFrames), Vec_IntSize(vReasons), 
            Saig_ManPiNum(p->pAig) - p->nInputs, Vec_IntSize(vRes) );
ABC_PRT( "Time", Abc_Clock() - clk );
    }

/*
    ////////////////////////////////////
    Vec_IntFree( vReasons );
    vReasons = Saig_RefManRefineWithSat( p, vRes );
    ////////////////////////////////////

    // derive new result
    Vec_IntFree( vRes );
    vRes = Saig_RefManReason2Inputs( p, vReasons );
//    if ( fVerbose )
    {
        printf( "Frame PIs = %4d (essential = %4d)   AIG PIs = %4d (essential = %4d)   ",
            Aig_ManCiNum(p->pFrames), Vec_IntSize(vReasons), 
            Saig_ManPiNum(p->pAig) - p->nInputs, Vec_IntSize(vRes) );
ABC_PRT( "Time", Abc_Clock() - clk );
    }
*/

    Vec_IntFree( vReasons );
    Saig_RefManStop( p );
    return vRes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

