/**CFile****************************************************************

  FileName    [bmcBmcAnd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [Memory-efficient BMC engine]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcBmcAnd.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"
#include "aig/gia/giaAig.h"
#include "sat/bsat/satStore.h"
#include "sat/cnf/cnf.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Bmc_Mna_t_ Bmc_Mna_t; 
struct Bmc_Mna_t_
{
    Gia_Man_t *         pFrames;  // time frames
    Cnf_Dat_t *         pCnf;     // CNF derived for the timeframes
    Vec_Int_t *         vPiMap;   // maps unrolled GIA PIs into user GIA PIs
    Vec_Int_t *         vId2Var;  // maps GIA IDs into SAT vars
    Vec_Int_t *         vInputs;  // inputs of the cone
    Vec_Int_t *         vOutputs; // outputs of the cone
    Vec_Int_t *         vNodes;   // internal nodes of the cone
    sat_solver *        pSat;     // SAT solver
    int                 nSatVars; // the counter of SAT variables
    abctime             clkStart; // starting time
};

static inline int Gia_ManTerSimInfoGet( unsigned * pInfo, int i )
{
    return 3 & (pInfo[i >> 4] >> ((i & 15) << 1));
}
static inline void Gia_ManTerSimInfoSet( unsigned * pInfo, int i, int Value )
{
    assert( Value >= GIA_ZER && Value <= GIA_UND );
    Value ^= Gia_ManTerSimInfoGet( pInfo, i );
    pInfo[i >> 4] ^= (Value << ((i & 15) << 1));
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs ternary simulation of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Bmc_MnaTernary( Gia_Man_t * p, int nFrames, int nFramesAdd, int fVerbose, int * iFirst )
{
    Vec_Ptr_t * vStates;
    unsigned * pState;
    int nStateWords = Abc_BitWordNum( 2*Gia_ManCoNum(p) );
    Gia_Obj_t * pObj, * pObjRo;
    int f, i, Count[4];
    abctime clk = Abc_Clock();
    Gia_ManConst0(p)->Value = GIA_ZER;
    Gia_ManForEachPi( p, pObj, i )
        pObj->Value = GIA_UND;
    Gia_ManForEachRi( p, pObj, i )
        pObj->Value = GIA_ZER;
    *iFirst = -1;
    vStates = Vec_PtrAlloc( 100 );
    for ( f = 0; ; f++ )
    {
        // if frames are given, break at frames
        if ( nFrames && f == nFrames )
            break;
        // if frames are not given, break after nFramesAdd from the first x-valued
        if ( !nFrames && *iFirst >= 0 && f == *iFirst + nFramesAdd )
            break;
        // aassign CI values
        Gia_ManForEachRiRo( p, pObj, pObjRo, i )
            pObjRo->Value = pObj->Value;
        Gia_ManForEachAnd( p, pObj, i )
            pObj->Value = Gia_XsimAndCond( Gia_ObjFanin0(pObj)->Value, Gia_ObjFaninC0(pObj), Gia_ObjFanin1(pObj)->Value, Gia_ObjFaninC1(pObj) );
        // compute and save CO values
        pState = ABC_ALLOC( unsigned, nStateWords );
        Gia_ManForEachCo( p, pObj, i )
        {
            pObj->Value = Gia_XsimNotCond( Gia_ObjFanin0(pObj)->Value, Gia_ObjFaninC0(pObj) );
            Gia_ManTerSimInfoSet( pState, i, pObj->Value );
            if ( *iFirst == -1 && i < Gia_ManPoNum(p) && pObj->Value == GIA_UND )
                *iFirst = f;
        }
        Vec_PtrPush( vStates, pState );
        // print statistics
        if ( !fVerbose )
            continue;
        Count[0] = Count[1] = Count[2] = Count[3] = 0;
        Gia_ManForEachRi( p, pObj, i )
            Count[pObj->Value]++;
        printf( "%5d : 0 =%7d    1 =%7d    x =%7d    all =%7d   out = %s\n", 
            f, Count[GIA_ZER], Count[GIA_ONE], Count[GIA_UND], Gia_ManRegNum(p), 
            Gia_ManPo(p, 0)->Value == GIA_UND ? "x" : "0" );  
    }
//    assert( Vec_PtrSize(vStates) == nFrames );
    if ( fVerbose )
        printf( "Finished %d frames. First x-valued PO is in frame %d.  ", nFrames, *iFirst );
    if ( fVerbose )
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return vStates;
}


/**Function*************************************************************

  Synopsis    [Collect AIG nodes for the group of POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bmc_MnaCollect_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vNodes, unsigned * pState )
{
    if ( pObj->fPhase )
        return;
    pObj->fPhase = 1;
    if ( Gia_ObjIsAnd(pObj) )
    {
        Bmc_MnaCollect_rec( p, Gia_ObjFanin0(pObj), vNodes, pState );
        Bmc_MnaCollect_rec( p, Gia_ObjFanin1(pObj), vNodes, pState );
        pObj->Value = Gia_XsimAndCond( Gia_ObjFanin0(pObj)->Value, Gia_ObjFaninC0(pObj), Gia_ObjFanin1(pObj)->Value, Gia_ObjFaninC1(pObj) );
    }
    else if ( Gia_ObjIsRo(p, pObj) )
        pObj->Value = pState ? Gia_ManTerSimInfoGet( pState, Gia_ObjCioId(Gia_ObjRoToRi(p, pObj)) ) : GIA_ZER;
    else if ( Gia_ObjIsPi(p, pObj) )
        pObj->Value = GIA_UND;
    else assert( 0 );
    Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
}
void Bmc_MnaCollect( Gia_Man_t * p, Vec_Int_t * vCos, Vec_Int_t * vNodes, unsigned * pState )
{
    Gia_Obj_t * pObj;
    int i;
    Vec_IntClear( vNodes );
    Gia_ManConst0(p)->fPhase = 1;
    Gia_ManConst0(p)->Value = GIA_ZER;
    Gia_ManForEachObjVec( vCos, p, pObj, i )
    {
        assert( Gia_ObjIsCo(pObj) );
        Bmc_MnaCollect_rec( p, Gia_ObjFanin0(pObj), vNodes, pState );
        pObj->Value = Gia_XsimNotCond( Gia_ObjFanin0(pObj)->Value, Gia_ObjFaninC0(pObj) );
        assert( pObj->Value == GIA_UND );
    }
}

/**Function*************************************************************

  Synopsis    [Select related logic cones for the COs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bmc_MnaSelect_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vLeaves )
{
    if ( !pObj->fPhase )
        return;
    pObj->fPhase = 0;
    assert( pObj->Value == GIA_UND );
    if ( Gia_ObjIsAnd(pObj) )
    {
        if ( Gia_ObjFanin0(pObj)->Value == GIA_UND )
            Bmc_MnaSelect_rec( p, Gia_ObjFanin0(pObj), vLeaves );
        else assert( Gia_ObjFanin0(pObj)->Value + Gia_ObjFaninC0(pObj) == GIA_ONE );
        if ( Gia_ObjFanin1(pObj)->Value == GIA_UND )
            Bmc_MnaSelect_rec( p, Gia_ObjFanin1(pObj), vLeaves );
        else assert( Gia_ObjFanin1(pObj)->Value + Gia_ObjFaninC1(pObj) == GIA_ONE );
    }
    else if ( Gia_ObjIsRo(p, pObj) )
        Vec_IntPush( vLeaves, Gia_ObjId(p, Gia_ObjRoToRi(p, pObj)) );
}
void Bmc_MnaSelect( Gia_Man_t * p, Vec_Int_t * vCos, Vec_Int_t * vNodes, Vec_Int_t * vLeaves )
{
    Gia_Obj_t * pObj;
    int i;
    Vec_IntClear( vLeaves );
    Gia_ManForEachObjVec( vCos, p, pObj, i )
        Bmc_MnaSelect_rec( p, Gia_ObjFanin0(pObj), vLeaves );
    Gia_ManConst0(p)->fPhase = 0;
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
        pObj->fPhase = 0;
}

/**Function*************************************************************

  Synopsis    [Build AIG for the selected cones.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bmc_MnaBuild_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Gia_Man_t * pNew, Vec_Int_t * vMap, Vec_Int_t * vPiMap )
{
    if ( !pObj->fPhase )
        return;
    pObj->fPhase = 0;
    assert( pObj->Value == GIA_UND );
    if ( Gia_ObjIsAnd(pObj) )
    {
        int iLit0 = 1, iLit1 = 1;
        if ( Gia_ObjFanin0(pObj)->Value == GIA_UND )
            Bmc_MnaBuild_rec( p, Gia_ObjFanin0(pObj), pNew, vMap, vPiMap );
        if ( Gia_ObjFanin1(pObj)->Value == GIA_UND )
            Bmc_MnaBuild_rec( p, Gia_ObjFanin1(pObj), pNew, vMap, vPiMap );
        if ( Gia_ObjFanin0(pObj)->Value == GIA_UND )
            iLit0 = Abc_LitNotCond( Vec_IntEntry(vMap, Gia_ObjFaninId0p(p, pObj)), Gia_ObjFaninC0(pObj) );
        if ( Gia_ObjFanin1(pObj)->Value == GIA_UND )
            iLit1 = Abc_LitNotCond( Vec_IntEntry(vMap, Gia_ObjFaninId1p(p, pObj)), Gia_ObjFaninC1(pObj) );
        Vec_IntWriteEntry( vMap, Gia_ObjId(p, pObj), Gia_ManHashAnd(pNew, iLit0, iLit1) );
    }
    else if ( Gia_ObjIsRo(p, pObj) )
        assert( Vec_IntEntry(vMap, Gia_ObjId(p, pObj)) != -1 );
    else if ( Gia_ObjIsPi(p, pObj) )
    {
        Vec_IntPush( vPiMap, Gia_ObjCioId(pObj) );
        Vec_IntWriteEntry( vMap, Gia_ObjId(p, pObj), Gia_ManAppendCi(pNew) );
    }
    else assert( 0 );
}
void Bmc_MnaBuild( Gia_Man_t * p, Vec_Int_t * vCos, Vec_Int_t * vNodes, Gia_Man_t * pNew, Vec_Int_t * vMap, Vec_Int_t * vPiMap )
{
    Gia_Obj_t * pObj;
    int i, iLit;
    Gia_ManForEachObjVec( vCos, p, pObj, i )
    {
        assert( Gia_ObjIsCo(pObj) );
        Bmc_MnaBuild_rec( p, Gia_ObjFanin0(pObj), pNew, vMap, vPiMap );
        iLit = Abc_LitNotCond( Vec_IntEntry(vMap, Gia_ObjFaninId0p(p, pObj)), Gia_ObjFaninC0(pObj) );
        Vec_IntWriteEntry( vMap, Gia_ObjId(p, pObj), iLit );
    }
    Gia_ManConst0(p)->fPhase = 0;
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
        pObj->fPhase = 0;
}


/**Function*************************************************************

  Synopsis    [Compute the first non-trivial timeframe.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManBmcUnroll( Gia_Man_t * pGia, int nFramesMax, int nFramesAdd, int fVerbose, Vec_Int_t ** pvPiMap )
{
    Gia_Obj_t * pObj;
    Gia_Man_t * pNew, * pTemp;
    Vec_Ptr_t * vStates, * vBegins;
    Vec_Int_t * vRoots, * vCone, * vLeaves, * vMap;
    unsigned * pStateF, * pStateP;
    int f, i, iFirst;
    Gia_ManCleanPhase( pGia );
    vCone = Vec_IntAlloc( 1000 );
    vLeaves = Vec_IntAlloc( 1000 );
    // perform ternary simulation
    vStates = Bmc_MnaTernary( pGia, nFramesMax, nFramesAdd, fVerbose, &iFirst );
    // go backward
    *pvPiMap = Vec_IntAlloc( 1000 );
    vBegins = Vec_PtrStart( Vec_PtrSize(vStates) );
    for ( f = Vec_PtrSize(vStates) - 1; f >= 0; f-- )
    {
        // get ternary states
        pStateF = (unsigned *)Vec_PtrEntry(vStates, f);
        pStateP = f ? (unsigned *)Vec_PtrEntry(vStates, f-1) : 0;
        // collect roots of this frame
        vRoots = Vec_IntAlloc( 100 );
        Gia_ManForEachPo( pGia, pObj, i )
            if ( Gia_ManTerSimInfoGet( pStateF, Gia_ObjCioId(pObj) ) == GIA_UND )
                Vec_IntPush( vRoots, Gia_ObjId(pGia, pObj) );
        // add leaves from the previous frame
        Vec_IntAppend( vRoots, vLeaves );
        Vec_PtrWriteEntry( vBegins, f, vRoots );
        // find the cone
        Bmc_MnaCollect( pGia, vRoots, vCone, pStateP ); // computes vCone
        Bmc_MnaSelect( pGia, vRoots, vCone, vLeaves );  // computes vLeaves 
        if ( fVerbose )
        printf( "Frame %4d :  Roots = %6d  Leaves = %6d  Cone = %6d\n", 
            f, Vec_IntSize(vRoots), Vec_IntSize(vLeaves), Vec_IntSize(vCone) );
        if ( Vec_IntSize(vLeaves) == 0 )
            break;
        // it is possible that some of the POs are still ternary... 
    }
    assert( f >= 0 );
    // go forward
    vMap = Vec_IntStartFull( Gia_ManObjNum(pGia) );
    pNew = Gia_ManStart( 10000 );
    pNew->pName = Abc_UtilStrsav( pGia->pName );
    Gia_ManHashStart( pNew );
    for ( f = 0; f < Vec_PtrSize(vStates); f++ )
    {
        vRoots = (Vec_Int_t *)Vec_PtrEntry( vBegins, f );
        if ( vRoots == NULL )
        {
            Gia_ManForEachPo( pGia, pObj, i )
                Gia_ManAppendCo( pNew, 0 );
            continue;
        }
        // get ternary states
        pStateF = (unsigned *)Vec_PtrEntry(vStates, f);
        pStateP = f ? (unsigned *)Vec_PtrEntry(vStates, f-1) : 0;
        // clean POs
        Gia_ManForEachPo( pGia, pObj, i )
            Vec_IntWriteEntry( vMap, Gia_ObjId(pGia, pObj), 0 );
        // find the cone
        Vec_IntPush( *pvPiMap, -f-1 );
        Bmc_MnaCollect( pGia, vRoots, vCone, pStateP );   // computes vCone
        Bmc_MnaBuild( pGia, vRoots, vCone, pNew, vMap, *pvPiMap );  // computes pNew       
        if ( fVerbose )
        printf( "Frame %4d :  Roots = %6d  Leaves = %6d  Cone = %6d\n", 
            f, Vec_IntSize(vRoots), Vec_IntSize(vLeaves), Vec_IntSize(vCone) );
        // create POs
        Gia_ManForEachPo( pGia, pObj, i )
            Gia_ManAppendCo( pNew, Vec_IntEntry(vMap, Gia_ObjId(pGia, pObj)) );
        // set a new map
        Gia_ManForEachObjVec( vRoots, pGia, pObj, i )
            if ( Gia_ObjIsRi(pGia, pObj) )
                Vec_IntWriteEntry( vMap, Gia_ObjId(pGia, Gia_ObjRiToRo(pGia, pObj)), Vec_IntEntry(vMap, Gia_ObjId(pGia, pObj)) );
//            else if ( Gia_ObjIsPo(pGia, pObj) )
//                Gia_ManAppendCo( pNew, Vec_IntEntry(vMap, Gia_ObjId(pGia, pObj)) );
//            else assert( 0 );
    }
    Gia_ManHashStop( pNew );
    Vec_VecFree( (Vec_Vec_t *)vBegins );
    Vec_PtrFreeFree( vStates );
    Vec_IntFree( vLeaves );
    Vec_IntFree( vCone );
    Vec_IntFree( vMap );
    // cleanup
//    Gia_ManPrintStats( pNew, NULL );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
//    Gia_ManPrintStats( pNew, NULL );
    return pNew;
}
 


/**Function*************************************************************

  Synopsis    [BMC manager manipulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bmc_Mna_t * Bmc_MnaAlloc()
{
    Bmc_Mna_t * p;
    p = ABC_CALLOC( Bmc_Mna_t, 1 );
    p->vId2Var   = Vec_IntAlloc( 0 );
    p->vInputs   = Vec_IntAlloc( 1000 );
    p->vOutputs  = Vec_IntAlloc( 1000 );
    p->vNodes    = Vec_IntAlloc( 10000 );
    p->pSat      = sat_solver_new();
    p->nSatVars  = 1;
    p->clkStart  = Abc_Clock();
    sat_solver_setnvars( p->pSat, 1000 );
    return p;
}
void Bmc_MnaFree( Bmc_Mna_t * p )
{
    Cnf_DataFree( p->pCnf );
    Vec_IntFreeP( &p->vPiMap );
    Vec_IntFreeP( &p->vId2Var );
    Vec_IntFreeP( &p->vInputs );
    Vec_IntFreeP( &p->vOutputs );
    Vec_IntFreeP( &p->vNodes );
    sat_solver_delete( p->pSat );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Derives GIA for the given cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManBmcDupCone( Gia_Man_t * p, Vec_Int_t * vIns, Vec_Int_t * vNodes, Vec_Int_t * vOuts )
{
    Gia_Man_t * pNew;
    Vec_Int_t * vTempIn, * vTempNode;
    Gia_Obj_t * pObj;
    int i;
    // save values
    vTempIn = Vec_IntAlloc( Vec_IntSize(vIns) );
    Gia_ManForEachObjVec( vIns, p, pObj, i )
        Vec_IntPush( vTempIn, pObj->Value );
    // save values
    vTempNode = Vec_IntAlloc( Vec_IntSize(vNodes) );
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
        Vec_IntPush( vTempNode, pObj->Value );
    // derive new GIA
    pNew = Gia_ManDupFromVecs( p, vIns, vNodes, vOuts, 0 );
    // reset values
    Gia_ManForEachObjVec( vIns, p, pObj, i )
        pObj->Value = Vec_IntEntry( vTempIn, i );
    // reset values
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
        pObj->Value = Vec_IntEntry( vTempNode, i );
    // reset values
    Gia_ManForEachObjVec( vOuts, p, pObj, i )
        pObj->Value = 0;
    Vec_IntFree( vTempIn );
    Vec_IntFree( vTempNode );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Derives GIA for the given cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManBmcAssignVarIds( Bmc_Mna_t * p, Vec_Int_t * vIns, Vec_Int_t * vUsed, Vec_Int_t * vOuts )
{
    int i, iObj = 0, VarC0 = p->nSatVars++;
    Vec_IntForEachEntry( vIns, iObj, i )
        if ( Vec_IntEntry( p->vId2Var, iObj ) == 0 )
            Vec_IntWriteEntry( p->vId2Var, iObj, p->nSatVars++ );
    Vec_IntForEachEntryReverse( vUsed, iObj, i )
    {
        assert( Vec_IntEntry( p->vId2Var, iObj ) == 0 );
        Vec_IntWriteEntry( p->vId2Var, iObj, p->nSatVars++ );
    }
    Vec_IntForEachEntry( vOuts, iObj, i )
    {
        assert( Vec_IntEntry( p->vId2Var, iObj ) == 0 );
        Vec_IntWriteEntry( p->vId2Var, iObj, p->nSatVars++ );
    }
    // extend the SAT solver
    if ( p->nSatVars > sat_solver_nvars(p->pSat) )
        sat_solver_setnvars( p->pSat, p->nSatVars );
    return VarC0;
}

/**Function*************************************************************

  Synopsis    [Derives CNF for the given cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManBmcAddCnf( Bmc_Mna_t * p, Gia_Man_t * pGia, Vec_Int_t * vIns, Vec_Int_t * vNodes, Vec_Int_t * vOuts )
{
    Gia_Man_t * pNew = Gia_ManBmcDupCone( pGia, vIns, vNodes, vOuts );
    Aig_Man_t * pAig = Gia_ManToAigSimple( pNew );
    Cnf_Dat_t * pCnf = Cnf_Derive( pAig, Aig_ManCoNum(pAig) );
    Vec_Int_t * vUsed, * vMap;
    Gia_Obj_t * pObj;
    int i, iObj = 0, VarC0;
    // collect used variables
    vUsed = Vec_IntAlloc( pCnf->nVars - Vec_IntSize(vIns) - Vec_IntSize(vOuts) );
    Gia_ManForEachAnd( pNew, pObj, i )
        if ( pCnf->pVarNums[i] >= 0 )
            Vec_IntPush( vUsed, Vec_IntEntry(vNodes, i - Vec_IntSize(vIns) - 1) );
    // assign variable IDs
    VarC0 = Gia_ManBmcAssignVarIds( p, vIns, vUsed, vOuts );
    Vec_IntFree( vUsed );
    // create variable map from CNF vars into SAT vars
    vMap = Vec_IntStartFull( pCnf->nVars );
    assert( pCnf->pVarNums[0] > 0 );
    Vec_IntWriteEntry( vMap, pCnf->pVarNums[0], VarC0 );
    Gia_ManForEachObj1( pNew, pObj, i )
    {
        if ( pCnf->pVarNums[i] < 0 )
            continue;
        assert( pCnf->pVarNums[i] >= 0 && pCnf->pVarNums[i] < pCnf->nVars );
        if ( Gia_ObjIsCi(pObj) )
            iObj = Vec_IntEntry( vIns, i - 1 );
        else if ( Gia_ObjIsAnd(pObj) )
            iObj = Vec_IntEntry( vNodes, i - Vec_IntSize(vIns) - 1 );
        else if ( Gia_ObjIsCo(pObj) )
            iObj = Vec_IntEntry( vOuts, i - Vec_IntSize(vIns) - Vec_IntSize(vNodes) - 1 );
        else assert( 0 );
        assert( Vec_IntEntry(p->vId2Var, iObj) > 0 );
        Vec_IntWriteEntry( vMap, pCnf->pVarNums[i], Vec_IntEntry(p->vId2Var, iObj) );
    }
//Vec_IntPrint( vMap );
    // remap CNF
    for ( i = 0; i < pCnf->nLiterals; i++ )
    {
        assert( pCnf->pClauses[0][i] > 1 && pCnf->pClauses[0][i] < 2 * pCnf->nVars );
        pCnf->pClauses[0][i] = Abc_Lit2LitV( Vec_IntArray(vMap), pCnf->pClauses[0][i] );
    }
    Vec_IntFree( vMap );
    // add clauses
    for ( i = 0; i < pCnf->nClauses; i++ )
    {
/*
        int v;
        for ( v = 0; v < pCnf->pClauses[i+1] - pCnf->pClauses[i]; v++ )
            printf( "%d ", pCnf->pClauses[i][v] );
        printf( "\n" );
*/
        if ( !sat_solver_addclause( p->pSat, pCnf->pClauses[i], pCnf->pClauses[i+1] ) )
            break;
    }
    if ( i < pCnf->nClauses )
        printf( "SAT solver became UNSAT after adding clauses.\n" );
    Aig_ManStop( pAig );
    Cnf_DataFree( pCnf );
    Gia_ManStop( pNew );
}

/**Function*************************************************************

  Synopsis    [Collects new nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManBmcAddCone_rec( Bmc_Mna_t * p, Gia_Obj_t * pObj )
{
    int iObj;
    if ( pObj->fMark0 )
        return;
    pObj->fMark0 = 1;
    iObj = Gia_ObjId( p->pFrames, pObj );
    if ( Gia_ObjIsAnd(pObj) && Vec_IntEntry(p->vId2Var, iObj) == 0 )
    {
        Gia_ManBmcAddCone_rec( p, Gia_ObjFanin0(pObj) );
        Gia_ManBmcAddCone_rec( p, Gia_ObjFanin1(pObj) );
        Vec_IntPush( p->vNodes, iObj );
    }
    else
        Vec_IntPush( p->vInputs, iObj );
}
void Gia_ManBmcAddCone( Bmc_Mna_t * p, int iStart, int iStop )
{
    Gia_Obj_t * pObj;
    int i;
    Vec_IntClear( p->vNodes );
    Vec_IntClear( p->vInputs );
    Vec_IntClear( p->vOutputs );
    Vec_IntFillExtra( p->vId2Var, Gia_ManObjNum(p->pFrames), 0 );
    for ( i = iStart; i < iStop; i++ )
    {
        pObj = Gia_ManPo(p->pFrames, i);
        if ( Gia_ObjChild0(pObj) == Gia_ManConst0(p->pFrames) )
            continue;
        Gia_ManBmcAddCone_rec( p, Gia_ObjFanin0(pObj) );
        Vec_IntPush( p->vOutputs, Gia_ObjId(p->pFrames, pObj) );
    }
    // clean attributes and create new variables
    Gia_ManForEachObjVec( p->vNodes, p->pFrames, pObj, i )
        pObj->fMark0 = 0;
    Gia_ManForEachObjVec( p->vInputs, p->pFrames, pObj, i )
        pObj->fMark0 = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManBmcCheckOutputs( Gia_Man_t * pFrames, int iStart, int iStop )
{
    int i;
    for ( i = iStart; i < iStop; i++ )
        if ( Gia_ObjChild0(Gia_ManPo(pFrames, i)) != Gia_ManConst0(pFrames) )
            return 0;
    return 1;
}
int Gia_ManBmcFindFirst( Gia_Man_t * pFrames )
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachPo( pFrames, pObj, i )
        if ( Gia_ObjChild0(pObj) != Gia_ManConst0(pFrames) )
            return i;
    return -1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManBmcPerform_Unr( Gia_Man_t * pGia, Bmc_AndPar_t * pPars )
{
    Unr_Man_t * pUnroll;
    Bmc_Mna_t * p;
    int nFramesMax = pPars->nFramesMax ? pPars->nFramesMax : ABC_INFINITY;
    int f, i=0, Lit, status, RetValue = -2;
    p = Bmc_MnaAlloc();
    pUnroll = Unr_ManUnrollStart( pGia, pPars->fVeryVerbose );
    for ( f = 0; f < nFramesMax; f++ )
    {
        p->pFrames = Unr_ManUnrollFrame( pUnroll, f );
        if ( !Gia_ManBmcCheckOutputs( p->pFrames, f * Gia_ManPoNum(pGia), (f+1) * Gia_ManPoNum(pGia) ) )
        {
            // create another slice
            Gia_ManBmcAddCone( p, f * Gia_ManPoNum(pGia), (f+1) * Gia_ManPoNum(pGia) );
            // create CNF in the SAT solver
            Gia_ManBmcAddCnf( p, p->pFrames, p->vInputs, p->vNodes, p->vOutputs );
            // try solving the outputs
            for ( i = f * Gia_ManPoNum(pGia); i < (f+1) * Gia_ManPoNum(pGia); i++ )
            {
                Gia_Obj_t * pObj = Gia_ManPo(p->pFrames, i);
                if ( Gia_ObjChild0(pObj) == Gia_ManConst0(p->pFrames) )
                    continue;
                Lit = Abc_Var2Lit( Vec_IntEntry(p->vId2Var, Gia_ObjId(p->pFrames, pObj)), 0 );
                status = sat_solver_solve( p->pSat, &Lit, &Lit + 1, (ABC_INT64_T)pPars->nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
                if ( status == l_False ) // unsat
                    continue;
                if ( status == l_True ) // sat
                    RetValue = 0;
                if ( status == l_Undef ) // undecided
                    RetValue = -1;
                break;
            }
        }
        // report statistics
        if ( pPars->fVerbose )
        {
            printf( "%4d :  PI =%9d.  AIG =%9d.  Var =%8d.  In =%6d.  And =%9d.  Cla =%9d.  Conf =%9d.  Mem =%7.1f MB   ", 
                f, Gia_ManPiNum(p->pFrames), Gia_ManAndNum(p->pFrames), 
                p->nSatVars-1, Vec_IntSize(p->vInputs), Vec_IntSize(p->vNodes), 
                sat_solver_nclauses(p->pSat), sat_solver_nconflicts(p->pSat), Gia_ManMemory(p->pFrames)/(1<<20) );
            Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
        }
        if ( RetValue != -2 )
        {
            if ( RetValue == -1 )
                printf( "SAT solver reached conflict/runtime limit in frame %d.\n", f );
            else
            {
                printf( "Output %d of miter \"%s\" was asserted in frame %d.  ", 
                    i - f * Gia_ManPoNum(pGia), Gia_ManName(pGia), f );
                Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
            }
            break;
        }
    }
    if ( RetValue == -2 )
        RetValue = -1;
    // dump unfolded frames
    if ( pPars->fDumpFrames )
    {
        p->pFrames = Gia_ManCleanup( p->pFrames );
        Gia_AigerWrite( p->pFrames, "frames.aig", 0, 0, 0 );
        printf( "Dumped unfolded frames into file \"frames.aig\".\n" );
        Gia_ManStop( p->pFrames );
    }
    // cleanup
    Unr_ManFree( pUnroll );
    Bmc_MnaFree( p );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Generate counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Gia_ManBmcCexGen( Bmc_Mna_t * pMan, Gia_Man_t * p, int iOut )
{
    Abc_Cex_t * pCex;
    int i, iObjId, iSatVar, iOrigPi;
    int iFramePi = 0, iFrame = -1;
    pCex = Abc_CexAlloc( Gia_ManRegNum(p), Gia_ManPiNum(p), iOut / Gia_ManPoNum(p) + 1 );
    pCex->iFrame = iOut / Gia_ManPoNum(p);
    pCex->iPo = iOut % Gia_ManPoNum(p);
    // fill in the input values
    Vec_IntForEachEntry( pMan->vPiMap, iOrigPi, i )
    {
        if ( iOrigPi < 0 )
        {
            iFrame = -iOrigPi-1;
            continue;
        }
        // iOrigPi in iFrame of pGia has PI index iFramePi in pMan->pFrames,
        iObjId = Gia_ObjId( pMan->pFrames, Gia_ManPi(pMan->pFrames, iFramePi) );
        iSatVar = Vec_IntEntry( pMan->vId2Var, iObjId );
        if ( sat_solver_var_value(pMan->pSat, iSatVar) )
            Abc_InfoSetBit( pCex->pData, Gia_ManRegNum(p) + Gia_ManPiNum(p) * iFrame + iOrigPi );
        iFramePi++;
    }
    assert( iFramePi == Gia_ManPiNum(pMan->pFrames) );
    return pCex;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManBmcPerform_old_cnf( Gia_Man_t * pGia, Bmc_AndPar_t * pPars )
{
    Bmc_Mna_t * p;
    int nFramesMax, f, i=0, Lit, status, RetValue = -2;
    abctime clk = Abc_Clock();
    p = Bmc_MnaAlloc();
    p->pFrames = Gia_ManBmcUnroll( pGia, pPars->nFramesMax, pPars->nFramesAdd, pPars->fVeryVerbose, &p->vPiMap );
    nFramesMax = Gia_ManPoNum(p->pFrames) / Gia_ManPoNum(pGia);
    if ( pPars->fVerbose )
    {
        printf( "Unfolding for %d frames with first non-trivial PO %d.  ", nFramesMax, Gia_ManBmcFindFirst(p->pFrames) );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    if ( pPars->fUseSynth )
    {
        Gia_Man_t * pTemp = p->pFrames;
        p->pFrames = Gia_ManAigSyn2( pTemp, 1, 0, 0, 0, 0, pPars->fVerbose, 0 );
        Gia_ManStop( pTemp );
    }
    else if ( pPars->fVerbose )
        Gia_ManPrintStats( p->pFrames, NULL );
    if ( pPars->fDumpFrames )
    {
        Gia_AigerWrite( p->pFrames, "frames.aig", 0, 0, 0 );
        printf( "Dumped unfolded frames into file \"frames.aig\".\n" );
    }
    for ( f = 0; f < nFramesMax; f++ )
    {
        if ( !Gia_ManBmcCheckOutputs( p->pFrames, f * Gia_ManPoNum(pGia), (f+1) * Gia_ManPoNum(pGia) ) )
        {
            // create another slice
            Gia_ManBmcAddCone( p, f * Gia_ManPoNum(pGia), (f+1) * Gia_ManPoNum(pGia) );
            // create CNF in the SAT solver
            Gia_ManBmcAddCnf( p, p->pFrames, p->vInputs, p->vNodes, p->vOutputs );
            // try solving the outputs
            for ( i = f * Gia_ManPoNum(pGia); i < (f+1) * Gia_ManPoNum(pGia); i++ )
            {
                Gia_Obj_t * pObj = Gia_ManPo(p->pFrames, i);
                if ( Gia_ObjChild0(pObj) == Gia_ManConst0(p->pFrames) )
                    continue;
                Lit = Abc_Var2Lit( Vec_IntEntry(p->vId2Var, Gia_ObjId(p->pFrames, pObj)), 0 );
                status = sat_solver_solve( p->pSat, &Lit, &Lit + 1, (ABC_INT64_T)pPars->nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
                if ( status == l_False ) // unsat
                    continue;
                if ( status == l_True ) // sat
                    RetValue = 0;
                if ( status == l_Undef ) // undecided
                    RetValue = -1;
                break;
            }
            // report statistics
            if ( pPars->fVerbose )
            {
                printf( "%4d :  PI =%9d.  AIG =%9d.  Var =%8d.  In =%6d.  And =%9d.  Cla =%9d.  Conf =%9d.  Mem =%7.1f MB   ", 
                    f, Gia_ManPiNum(p->pFrames), Gia_ManAndNum(p->pFrames), 
                    p->nSatVars-1, Vec_IntSize(p->vInputs), Vec_IntSize(p->vNodes), 
                    sat_solver_nclauses(p->pSat), sat_solver_nconflicts(p->pSat), Gia_ManMemory(p->pFrames)/(1<<20) );
                Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
            }
        }
        if ( RetValue != -2 )
        {
            if ( RetValue == -1 )
                printf( "SAT solver reached conflict/runtime limit in frame %d.\n", f );
            else
            {
                ABC_FREE( pGia->pCexSeq );
                pGia->pCexSeq = Gia_ManBmcCexGen( p, pGia, i );
                printf( "Output %d of miter \"%s\" was asserted in frame %d.  ", 
                    i - f * Gia_ManPoNum(pGia), Gia_ManName(pGia), f );
                Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
            }
            break;
        }
        pPars->iFrame = f;
    }
    if ( RetValue == -2 )
        RetValue = -1;
    // cleanup
    Gia_ManStop( p->pFrames );
    Bmc_MnaFree( p );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManBmcAddCnfNew_rec( Bmc_Mna_t * p, Gia_Obj_t * pObj )
{
    int iObj = Gia_ObjId( p->pFrames, pObj );
    if ( Vec_IntEntry(p->vId2Var, iObj) > 0 )
        return;
    Vec_IntWriteEntry(p->vId2Var, iObj, 1);
    if ( Gia_ObjIsAnd(pObj) && p->pCnf->pObj2Count[iObj] == -1 )
    {
        Gia_ManBmcAddCnfNew_rec( p, Gia_ObjFanin0(pObj) );
        Gia_ManBmcAddCnfNew_rec( p, Gia_ObjFanin1(pObj) );
        return;
    }
    Vec_IntWriteEntry(p->vId2Var, iObj, p->nSatVars++);
    if ( Gia_ObjIsAnd(pObj) || Gia_ObjIsPo(p->pFrames, pObj) )
    {
        int i, nClas, iCla;
        Gia_ManBmcAddCnfNew_rec( p, Gia_ObjFanin0(pObj) );
        if ( Gia_ObjIsAnd(pObj) )
        Gia_ManBmcAddCnfNew_rec( p, Gia_ObjFanin1(pObj) );
        // extend the SAT solver
        if ( p->nSatVars > sat_solver_nvars(p->pSat) )
            sat_solver_setnvars( p->pSat, p->nSatVars );
        // add clauses
        nClas = p->pCnf->pObj2Count[iObj];
        iCla  = p->pCnf->pObj2Clause[iObj];
        for ( i = 0; i < nClas; i++ )
        {
            int nLits, pLits[10];
            int * pClauseThis = p->pCnf->pClauses[iCla+i];
            int * pClauseNext = p->pCnf->pClauses[iCla+i+1];
            for ( nLits = 0; pClauseThis + nLits < pClauseNext; nLits++ )
            {
                if ( pClauseThis[nLits] < 2 )
                    printf( "\n\n\nError in CNF generation:  Constant literal!\n\n\n" );
                assert( pClauseThis[nLits] > 1 && pClauseThis[nLits] < 2*Gia_ManObjNum(p->pFrames) );
                pLits[nLits] = Abc_Lit2LitV( Vec_IntArray(p->vId2Var), pClauseThis[nLits] );
            }
            assert( nLits <= 9 );
            if ( !sat_solver_addclause( p->pSat, pLits, pLits + nLits ) )
                break;
        }
        if ( i < nClas )
            printf( "SAT solver became UNSAT after adding clauses.\n" );
    }
    else assert( Gia_ObjIsCi(pObj) );
}
void Gia_ManBmcAddCnfNew( Bmc_Mna_t * p, int iStart, int iStop )
{
    Gia_Obj_t * pObj;
    int i;
    for ( i = iStart; i < iStop; i++ )
    {
        pObj = Gia_ManPo(p->pFrames, i);
        if ( Gia_ObjFanin0(pObj) == Gia_ManConst0(p->pFrames) )
            continue;
        Gia_ManBmcAddCnfNew_rec( p, pObj );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Cnf_Dat_t * Cnf_DeriveGia( Gia_Man_t * p )
{
    Aig_Man_t * pAig = Gia_ManToAigSimple( p );
    Cnf_Dat_t * pCnf = Cnf_DeriveOther( pAig, 1 );
    Aig_ManStop( pAig );
    return pCnf;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManBmcPerformInt( Gia_Man_t * pGia, Bmc_AndPar_t * pPars )
{
    Bmc_Mna_t * p;
    Gia_Man_t * pTemp;
    int nFramesMax, f, i=0, Lit = 1, status, RetValue = -2;
    abctime clk = Abc_Clock();
    p = Bmc_MnaAlloc();
    sat_solver_set_runtime_limit( p->pSat, pPars->nTimeOut ? pPars->nTimeOut * CLOCKS_PER_SEC + Abc_Clock(): 0 );
    p->pFrames = Gia_ManBmcUnroll( pGia, pPars->nFramesMax, pPars->nFramesAdd, pPars->fVeryVerbose, &p->vPiMap );
    nFramesMax = Gia_ManPoNum(p->pFrames) / Gia_ManPoNum(pGia);
    if ( pPars->fVerbose )
    {
        printf( "Unfolding for %d frames with first non-trivial PO %d.  ", nFramesMax, Gia_ManBmcFindFirst(p->pFrames) );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    if ( pPars->fUseSynth )
    {
        p->pFrames = Gia_ManAigSyn2( pTemp = p->pFrames, 1, 0, 0, 0, 0, pPars->fVerbose, 0 );  Gia_ManStop( pTemp );
    }
    else if ( pPars->fVerbose )
        Gia_ManPrintStats( p->pFrames, NULL );
    if ( pPars->fDumpFrames )
    {
        Gia_AigerWrite( p->pFrames, "frames.aig", 0, 0, 0 );
        printf( "Dumped unfolded frames into file \"frames.aig\".\n" );
    }
    if ( pPars->fUseOldCnf )
        p->pCnf = Cnf_DeriveGia( p->pFrames );
    else
    {
//        p->pFrames = Jf_ManDeriveCnf( pTemp = p->pFrames, 1 );  Gia_ManStop( pTemp );
//        p->pCnf = (Cnf_Dat_t *)p->pFrames->pData; p->pFrames->pData = NULL;
        p->pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p->pFrames, pPars->nLutSize, 1, 0, 0, pPars->fVerbose );
    }
    Vec_IntFillExtra( p->vId2Var, Gia_ManObjNum(p->pFrames), 0 );
    // create clauses for constant node
//    sat_solver_addclause( p->pSat, &Lit, &Lit + 1 );
    for ( f = 0; f < nFramesMax; f++ )
    {
        if ( !Gia_ManBmcCheckOutputs( p->pFrames, f * Gia_ManPoNum(pGia), (f+1) * Gia_ManPoNum(pGia) ) )
        {
            // create another slice
            Gia_ManBmcAddCnfNew( p, f * Gia_ManPoNum(pGia), (f+1) * Gia_ManPoNum(pGia) );
            // try solving the outputs
            for ( i = f * Gia_ManPoNum(pGia); i < (f+1) * Gia_ManPoNum(pGia); i++ )
            {
                Gia_Obj_t * pObj = Gia_ManPo(p->pFrames, i);
                if ( Gia_ObjChild0(pObj) == Gia_ManConst0(p->pFrames) )
                    continue;
                if ( Gia_ObjChild0(pObj) == Gia_ManConst1(p->pFrames) )
                {
                    printf( "Output %d is trivially SAT.\n", i );
                    continue;
                }
                Lit = Abc_Var2Lit( Vec_IntEntry(p->vId2Var, Gia_ObjId(p->pFrames, pObj)), 0 );
                status = sat_solver_solve( p->pSat, &Lit, &Lit + 1, (ABC_INT64_T)pPars->nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
                if ( status == l_False ) // unsat
                    continue;
                if ( status == l_True )  // sat
                    RetValue = 0;
                if ( status == l_Undef ) // undecided
                    RetValue = -1;
                break;
            }
            // report statistics
            if ( pPars->fVerbose )
            {
                printf( "%4d :  PI =%9d.  AIG =%9d.  Var =%8d.  In =%6d.  And =%9d.  Cla =%9d.  Conf =%9d.  Mem =%7.1f MB   ", 
                    f, Gia_ManPiNum(p->pFrames), Gia_ManAndNum(p->pFrames), 
                    p->nSatVars-1, Vec_IntSize(p->vInputs), Vec_IntSize(p->vNodes), 
                    sat_solver_nclauses(p->pSat), sat_solver_nconflicts(p->pSat), Gia_ManMemory(p->pFrames)/(1<<20) );
                Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
            }
        }
        if ( RetValue != -2 )
        {
            if ( RetValue == -1 )
                printf( "SAT solver reached conflict/runtime limit in frame %d.\n", f );
            else
            {
                ABC_FREE( pGia->pCexSeq );
                pGia->pCexSeq = Gia_ManBmcCexGen( p, pGia, i );
                printf( "Output %d of miter \"%s\" was asserted in frame %d.  ", 
                    i - f * Gia_ManPoNum(pGia), Gia_ManName(pGia), f );
                Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
            }
            break;
        }
        pPars->iFrame = f;
    }
    if ( RetValue == -2 )
        RetValue = -1;
    // cleanup
    Gia_ManStop( p->pFrames );
    Bmc_MnaFree( p );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManBmcPerform( Gia_Man_t * pGia, Bmc_AndPar_t * pPars )
{
    abctime TimeToStop = pPars->nTimeOut ? pPars->nTimeOut * CLOCKS_PER_SEC + Abc_Clock() : 0;
    if ( pPars->nFramesAdd == 0 )
        return Gia_ManBmcPerformInt( pGia, pPars );
    // iterate over the engine until we read the global timeout
    assert( pPars->nTimeOut >= 0 );
    while ( 1 )
    {
        if ( TimeToStop && TimeToStop < Abc_Clock() )
            return -1;
        if ( Gia_ManBmcPerformInt( pGia, pPars ) == 0 )
            return 0;
        // set the new runtime limit
        if ( pPars->nTimeOut )
        {
            pPars->nTimeOut = Abc_MinInt( pPars->nTimeOut-1, (int)((TimeToStop - Abc_Clock()) / CLOCKS_PER_SEC) );
            if ( pPars->nTimeOut <= 0 )
                return -1;
        }
        else
            return -1;
        // set the new frames limit
        pPars->nFramesAdd *= 2;
    }
    return -1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

