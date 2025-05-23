/**CFile****************************************************************

  FileName    [saigGlaCba.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Gate level abstraction.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigGlaCba.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"
#include "sat/bsat/satSolver.h"
#include "sat/cnf/cnf.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Aig_Gla1Man_t_ Aig_Gla1Man_t;
struct Aig_Gla1Man_t_
{
    // user data
    Aig_Man_t *    pAig;
    int            nConfLimit;
    int            nFramesMax;
    int            fVerbose;
    // unrolling
    int            nFrames;
    Vec_Int_t *    vObj2Vec;   // maps obj ID into its vec ID
    Vec_Int_t *    vVec2Var;   // maps vec ID into its sat Var (nFrames per vec ID)
    Vec_Int_t *    vVar2Inf;   // maps sat Var into its frame and obj ID
    // abstraction
    Vec_Int_t *    vAssigned;  // collects objects whose SAT variables have been created
    Vec_Int_t *    vIncluded;  // maps obj ID into its status (0=unused; 1=included in abstraction)
    // components
    Vec_Int_t *    vPis;       // primary inputs
    Vec_Int_t *    vPPis;      // pseudo primary inputs
    Vec_Int_t *    vFlops;     // flops
    Vec_Int_t *    vNodes;     // nodes
    // CNF computation
    Vec_Ptr_t *    vLeaves;
    Vec_Ptr_t *    vVolume;
    Vec_Int_t *    vCover;
    Vec_Ptr_t *    vObj2Cnf;
    Vec_Int_t *    vLits;
    // SAT solver
    sat_solver *   pSat;
    // statistics
    clock_t        timeSat;
    clock_t        timeRef;
    clock_t        timeTotal;
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Adds constant to the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Aig_Gla1AddConst( sat_solver * pSat, int iVar, int fCompl )
{
    lit Lit = toLitCond( iVar, fCompl );
    if ( !sat_solver_addclause( pSat, &Lit, &Lit + 1 ) )
        return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Adds buffer to the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Aig_Gla1AddBuffer( sat_solver * pSat, int iVar0, int iVar1, int fCompl )
{
    lit Lits[2];

    Lits[0] = toLitCond( iVar0, 0 );
    Lits[1] = toLitCond( iVar1, !fCompl );
    if ( !sat_solver_addclause( pSat, Lits, Lits + 2 ) )
        return 0;

    Lits[0] = toLitCond( iVar0, 1 );
    Lits[1] = toLitCond( iVar1, fCompl );
    if ( !sat_solver_addclause( pSat, Lits, Lits + 2 ) )
        return 0;

    return 1;
}

/**Function*************************************************************

  Synopsis    [Adds buffer to the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Aig_Gla1AddNode( sat_solver * pSat, int iVar, int iVar0, int iVar1, int fCompl0, int fCompl1 )
{
    lit Lits[3];

    Lits[0] = toLitCond( iVar, 1 );
    Lits[1] = toLitCond( iVar0, fCompl0 );
    if ( !sat_solver_addclause( pSat, Lits, Lits + 2 ) )
        return 0;

    Lits[0] = toLitCond( iVar, 1 );
    Lits[1] = toLitCond( iVar1, fCompl1 );
    if ( !sat_solver_addclause( pSat, Lits, Lits + 2 ) )
        return 0;

    Lits[0] = toLitCond( iVar, 0 );
    Lits[1] = toLitCond( iVar0, !fCompl0 );
    Lits[2] = toLitCond( iVar1, !fCompl1 );
    if ( !sat_solver_addclause( pSat, Lits, Lits + 3 ) )
        return 0;

    return 1;
}

/**Function*************************************************************

  Synopsis    [Derives abstraction components (PIs, PPIs, flops, nodes).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_Gla1CollectAbstr( Aig_Gla1Man_t * p )
{
    Aig_Obj_t * pObj;
    int i, Entry;
/*
    // make sure every neighbor of included objects is assigned a variable
    Vec_IntForEachEntry( p->vIncluded, Entry, i )
    {
        if ( Entry == 0 )
            continue;
        assert( Entry == 1 );
        pObj = Aig_ManObj( p->pAig, i );
        if ( Vec_IntFind( p->vAssigned, Aig_ObjId(pObj) ) == -1 )
            printf( "Aig_Gla1CollectAbstr(): Object not found\n" );
        if ( Aig_ObjIsNode(pObj) )
        {
            if ( Vec_IntFind( p->vAssigned, Aig_ObjFaninId0(pObj) ) == -1 )
                printf( "Aig_Gla1CollectAbstr(): Node's fanin is not found\n" );
            if ( Vec_IntFind( p->vAssigned, Aig_ObjFaninId1(pObj) ) == -1 )
                printf( "Aig_Gla1CollectAbstr(): Node's fanin is not found\n" );
        }
        else if ( Saig_ObjIsLo(p->pAig, pObj) ) 
        {
            Aig_Obj_t * pObjLi;
            pObjLi = Saig_ObjLoToLi(p->pAig, pObj);
            if ( Vec_IntFind( p->vAssigned, Aig_ObjFaninId0(pObjLi) ) == -1 )
                printf( "Aig_Gla1CollectAbstr(): Flop's fanin is not found\n" );
        }
        else assert( Aig_ObjIsConst1(pObj) );
    }
*/
    Vec_IntClear( p->vPis );
    Vec_IntClear( p->vPPis );
    Vec_IntClear( p->vFlops );
    Vec_IntClear( p->vNodes );
    Vec_IntForEachEntryReverse( p->vAssigned, Entry, i )
    {
        pObj = Aig_ManObj( p->pAig, Entry );
        if ( Saig_ObjIsPi(p->pAig, pObj) )
            Vec_IntPush( p->vPis, Aig_ObjId(pObj) );
        else if ( !Vec_IntEntry(p->vIncluded, Aig_ObjId(pObj)) )
            Vec_IntPush( p->vPPis, Aig_ObjId(pObj) );
        else if ( Aig_ObjIsNode(pObj) )
            Vec_IntPush( p->vNodes, Aig_ObjId(pObj) );
        else if ( Saig_ObjIsLo(p->pAig, pObj) )
            Vec_IntPush( p->vFlops, Aig_ObjId(pObj) );
        else assert( Aig_ObjIsConst1(pObj) );
    }
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG manager recursively.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_Gla1DeriveAbs_rec( Aig_Man_t * pNew, Aig_Obj_t * pObj )
{
    if ( pObj->pData )
        return;
    assert( Aig_ObjIsNode(pObj) );
    Aig_Gla1DeriveAbs_rec( pNew, Aig_ObjFanin0(pObj) );
    Aig_Gla1DeriveAbs_rec( pNew, Aig_ObjFanin1(pObj) );
    pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
}

/**Function*************************************************************

  Synopsis    [Derives abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_Gla1DeriveAbs( Aig_Gla1Man_t * p )
{ 
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj;
    int i, RetValue;
    assert( Saig_ManPoNum(p->pAig) == 1 );
    // start the new manager
    pNew = Aig_ManStart( 5000 );
    pNew->pName = Abc_UtilStrsav( p->pAig->pName );
    // create constant
    Aig_ManCleanData( p->pAig );
    Aig_ManConst1(p->pAig)->pData = Aig_ManConst1(pNew);
    // create PIs
    Aig_ManForEachObjVec( p->vPis, p->pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi(pNew);
    // create additional PIs
    Aig_ManForEachObjVec( p->vPPis, p->pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi(pNew);
    // create ROs
    Aig_ManForEachObjVec( p->vFlops, p->pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi(pNew);
    // create internal nodes
    Aig_ManForEachObjVec( p->vNodes, p->pAig, pObj, i )
//        pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
        Aig_Gla1DeriveAbs_rec( pNew, pObj );
    // create PO
    Saig_ManForEachPo( p->pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    // create RIs
    Aig_ManForEachObjVec( p->vFlops, p->pAig, pObj, i )
    {
        assert( Saig_ObjIsLo(p->pAig, pObj) );
        pObj = Saig_ObjLoToLi( p->pAig, pObj );
        pObj->pData = Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    }
    Aig_ManSetRegNum( pNew, Vec_IntSize(p->vFlops) );
    // clean up
    RetValue = Aig_ManCleanup( pNew );
    if ( RetValue > 0 )
        printf( "Aig_Gla1DeriveAbs(): Internal error! Object count mismatch.\n" );
    assert( RetValue == 0 );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Finds existing SAT variable or creates a new one.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Aig_Gla1FetchVecId( Aig_Gla1Man_t * p, Aig_Obj_t * pObj )
{
    int i, iVecId;
    iVecId = Vec_IntEntry( p->vObj2Vec, Aig_ObjId(pObj) );
    if ( iVecId == 0 )
    {
        iVecId = Vec_IntSize( p->vVec2Var ) / p->nFrames;
        for ( i = 0; i < p->nFrames; i++ )
            Vec_IntPush( p->vVec2Var, 0 );
        Vec_IntWriteEntry( p->vObj2Vec, Aig_ObjId(pObj), iVecId );
        Vec_IntPushOrderReverse( p->vAssigned, Aig_ObjId(pObj) );
    }
    return iVecId;
}

/**Function*************************************************************

  Synopsis    [Finds existing SAT variable or creates a new one.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Aig_Gla1FetchVar( Aig_Gla1Man_t * p, Aig_Obj_t * pObj, int k )
{
    int iVecId, iSatVar;
    assert( k < p->nFrames );
    iVecId  = Aig_Gla1FetchVecId( p, pObj );
    iSatVar = Vec_IntEntry( p->vVec2Var, iVecId * p->nFrames + k );
    if ( iSatVar == 0 )
    {
        iSatVar = Vec_IntSize( p->vVar2Inf ) / 2;
        Vec_IntPush( p->vVar2Inf, Aig_ObjId(pObj) );
        Vec_IntPush( p->vVar2Inf, k );
        Vec_IntWriteEntry( p->vVec2Var, iVecId * p->nFrames + k, iSatVar );
        sat_solver_setnvars( p->pSat, iSatVar + 1 );
    }
    return iSatVar;
}

/**Function*************************************************************

  Synopsis    [Adds CNF for the given object in the given frame.]

  Description [Returns 0, if the solver becames UNSAT.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_Gla1ObjAddToSolver( Aig_Gla1Man_t * p, Aig_Obj_t * pObj, int k )
{
    if ( k == p->nFrames )
    {
        int i, j, nVecIds = Vec_IntSize( p->vVec2Var ) / p->nFrames;
        Vec_Int_t * vVec2VarNew = Vec_IntAlloc( 4 * nVecIds * p->nFrames );
        for ( i = 0; i < nVecIds; i++ )
        {
            for ( j = 0; j < p->nFrames; j++ )
                Vec_IntPush( vVec2VarNew, Vec_IntEntry( p->vVec2Var, i * p->nFrames + j ) );
            for ( j = 0; j < p->nFrames; j++ )
                Vec_IntPush( vVec2VarNew, i ? 0 : -1 );
        }
        Vec_IntFree( p->vVec2Var );
        p->vVec2Var = vVec2VarNew;
        p->nFrames *= 2;
    }
    assert( k < p->nFrames );
    assert( Vec_IntEntry(p->vIncluded, Aig_ObjId(pObj)) );
    if ( Aig_ObjIsConst1(pObj) )
        return Aig_Gla1AddConst( p->pSat, Aig_Gla1FetchVar(p, pObj, k), 0 );
    if ( Saig_ObjIsLo(p->pAig, pObj) )
    {
        Aig_Obj_t * pObjLi = Saig_ObjLoToLi(p->pAig, pObj);
        if ( k == 0 )
        {
            Aig_Gla1FetchVecId( p, Aig_ObjFanin0(pObjLi) );
            return Aig_Gla1AddConst( p->pSat, Aig_Gla1FetchVar(p, pObj, k), 1 );
        }
        return Aig_Gla1AddBuffer( p->pSat, Aig_Gla1FetchVar(p, pObj, k), 
                   Aig_Gla1FetchVar(p, Aig_ObjFanin0(pObjLi), k-1), 
                   Aig_ObjFaninC0(pObjLi) );
    }
    else
    { 
        Vec_Int_t * vClauses;
        int i, Entry;
        assert( Aig_ObjIsNode(pObj) );
        if ( p->vObj2Cnf == NULL )
            return Aig_Gla1AddNode( p->pSat, Aig_Gla1FetchVar(p, pObj, k), 
                       Aig_Gla1FetchVar(p, Aig_ObjFanin0(pObj), k), 
                       Aig_Gla1FetchVar(p, Aig_ObjFanin1(pObj), k), 
                       Aig_ObjFaninC0(pObj), Aig_ObjFaninC1(pObj) );
        // derive clauses
        assert( pObj->fMarkA );
        vClauses = (Vec_Int_t *)Vec_PtrEntry( p->vObj2Cnf, Aig_ObjId(pObj) );
        if ( vClauses == NULL )
        {
            Vec_PtrWriteEntry( p->vObj2Cnf, Aig_ObjId(pObj), (vClauses = Vec_IntAlloc(16)) );
            Cnf_ComputeClauses( p->pAig, pObj, p->vLeaves, p->vVolume, NULL, p->vCover, vClauses );
        }
        // derive variables
        Cnf_CollectLeaves( pObj, p->vLeaves, 0 );
        Vec_PtrForEachEntry( Aig_Obj_t *, p->vLeaves, pObj, i )
            Aig_Gla1FetchVar( p, pObj, k );
        // translate clauses
        assert( Vec_IntSize(vClauses) >= 2 );
        assert( Vec_IntEntry(vClauses, 0) == 0 );
        Vec_IntForEachEntry( vClauses, Entry, i )
        {
            if ( Entry == 0 )
            {
                Vec_IntClear( p->vLits );
                continue;
            }
            Vec_IntPush( p->vLits, (Entry & 1) ^ (2 * Aig_Gla1FetchVar(p, Aig_ManObj(p->pAig, Entry >> 1), k)) );
            if ( i == Vec_IntSize(vClauses) - 1 || Vec_IntEntry(vClauses, i+1) == 0 )
            {
                if ( !sat_solver_addclause( p->pSat, Vec_IntArray(p->vLits), Vec_IntArray(p->vLits)+Vec_IntSize(p->vLits) ) )
                    return 0;
            }
        }       
        return 1;
    }
}

/**Function*************************************************************

  Synopsis    [Returns the array of neighbors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_Gla1CollectAssigned( Aig_Gla1Man_t * p, Vec_Int_t * vGateClasses )
{
    Aig_Obj_t * pObj;
    int i, Entry;
    Vec_IntForEachEntryReverse( vGateClasses, Entry, i )
    {
        if ( Entry == 0 )
            continue;
        assert( Entry == 1 );
        pObj = Aig_ManObj( p->pAig, i );
        Aig_Gla1FetchVecId( p, pObj );
        if ( Aig_ObjIsNode(pObj) )
        {
            Aig_Gla1FetchVecId( p, Aig_ObjFanin0(pObj) );
            Aig_Gla1FetchVecId( p, Aig_ObjFanin1(pObj) );
        }
        else if ( Saig_ObjIsLo(p->pAig, pObj) )
            Aig_Gla1FetchVecId( p, Aig_ObjFanin0(Saig_ObjLoToLi(p->pAig, pObj)) );
        else assert( Aig_ObjIsConst1(pObj) );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Gla1Man_t * Aig_Gla1ManStart( Aig_Man_t * pAig, Vec_Int_t * vGateClassesOld, int fNaiveCnf )
{
    Aig_Gla1Man_t * p;
    int i;

    p = ABC_CALLOC( Aig_Gla1Man_t, 1 );
    p->pAig      = pAig;
    p->nFrames   = 32;

    // unrolling
    p->vObj2Vec  = Vec_IntStart( Aig_ManObjNumMax(pAig) );
    p->vVec2Var  = Vec_IntAlloc( 1 << 20 );
    p->vVar2Inf  = Vec_IntAlloc( 1 << 20 );

    // skip first vector ID
    for ( i = 0; i < p->nFrames; i++ )
        Vec_IntPush( p->vVec2Var, -1 );
    // skip  first SAT variable
    Vec_IntPush( p->vVar2Inf, -1 );
    Vec_IntPush( p->vVar2Inf, -1 );

    // abstraction
    p->vAssigned = Vec_IntAlloc( 1000 );
    if ( vGateClassesOld )
    {
        p->vIncluded = Vec_IntDup( vGateClassesOld );
        Aig_Gla1CollectAssigned( p, vGateClassesOld );
        assert( fNaiveCnf );
    }
    else
        p->vIncluded = Vec_IntStart( Aig_ManObjNumMax(pAig) );

    // components
    p->vPis      = Vec_IntAlloc( 1000 );
    p->vPPis     = Vec_IntAlloc( 1000 );
    p->vFlops    = Vec_IntAlloc( 1000 );
    p->vNodes    = Vec_IntAlloc( 1000 );

    // CNF computation
    if ( !fNaiveCnf )
    {
        p->vLeaves   = Vec_PtrAlloc( 100 );
        p->vVolume   = Vec_PtrAlloc( 100 );
        p->vCover    = Vec_IntAlloc( 1 << 16 );
        p->vObj2Cnf  = Vec_PtrStart( Aig_ManObjNumMax(pAig) );
        p->vLits     = Vec_IntAlloc( 100 );
        Cnf_DeriveFastMark( pAig );
    }

    // start the SAT solver
    p->pSat = sat_solver_new();
    sat_solver_setnvars( p->pSat, 256 );
    return p;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_Gla1ManStop( Aig_Gla1Man_t * p )
{
    Vec_IntFreeP( &p->vObj2Vec );
    Vec_IntFreeP( &p->vVec2Var );
    Vec_IntFreeP( &p->vVar2Inf );

    Vec_IntFreeP( &p->vAssigned );
    Vec_IntFreeP( &p->vIncluded );

    Vec_IntFreeP( &p->vPis );
    Vec_IntFreeP( &p->vPPis );
    Vec_IntFreeP( &p->vFlops );
    Vec_IntFreeP( &p->vNodes );

    if ( p->vObj2Cnf )
    {
        Vec_PtrFreeP( &p->vLeaves );
        Vec_PtrFreeP( &p->vVolume );
        Vec_IntFreeP( &p->vCover );
        Vec_VecFreeP( (Vec_Vec_t **)&p->vObj2Cnf );
        Vec_IntFreeP( &p->vLits );
        Aig_ManCleanMarkA( p->pAig );
    }

    if ( p->pSat )
        sat_solver_delete( p->pSat );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Aig_Gla1DeriveCex( Aig_Gla1Man_t * p, int iFrame )
{
    Abc_Cex_t * pCex;
    Aig_Obj_t * pObj;
    int i, f, iVecId, iSatId;
    pCex = Abc_CexAlloc( Vec_IntSize(p->vFlops), Vec_IntSize(p->vPis) + Vec_IntSize(p->vPPis), iFrame+1 );
    pCex->iFrame = iFrame;
    Aig_ManForEachObjVec( p->vPis, p->pAig, pObj, i )
    {
        iVecId = Vec_IntEntry( p->vObj2Vec, Aig_ObjId(pObj) );
        assert( iVecId > 0 );
        for ( f = 0; f <= iFrame; f++ )
        {
            iSatId = Vec_IntEntry( p->vVec2Var, iVecId * p->nFrames + f );
            if ( iSatId == 0 )
                continue;
            assert( iSatId > 0 );
            if ( sat_solver_var_value(p->pSat, iSatId) )
                Abc_InfoSetBit( pCex->pData, pCex->nRegs + f * pCex->nPis + i );
        }
    }
    Aig_ManForEachObjVec( p->vPPis, p->pAig, pObj, i )
    {
        iVecId = Vec_IntEntry( p->vObj2Vec, Aig_ObjId(pObj) );
        assert( iVecId > 0 );
        for ( f = 0; f <= iFrame; f++ )
        {
            iSatId = Vec_IntEntry( p->vVec2Var, iVecId * p->nFrames + f );
            if ( iSatId == 0 )
                continue;
            assert( iSatId > 0 );
            if ( sat_solver_var_value(p->pSat, iSatId) )
                Abc_InfoSetBit( pCex->pData, pCex->nRegs + f * pCex->nPis + Vec_IntSize(p->vPis) + i );
        }
    }
    return pCex;
}

/**Function*************************************************************

  Synopsis    [Prints current abstraction statistics.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_Gla1PrintAbstr( Aig_Gla1Man_t * p, int f, int r, int v, int c )
{
    if ( r == 0 )
        printf( "== %3d ==", f );
    else
        printf( "         " );
    printf( " %4d  PI =%6d  PPI =%6d  FF =%6d  Node =%6d  Var =%7d  Conf =%6d\n", 
        r, Vec_IntSize(p->vPis), Vec_IntSize(p->vPPis), Vec_IntSize(p->vFlops), Vec_IntSize(p->vNodes), v, c );
}

/**Function*************************************************************

  Synopsis    [Prints current abstraction statistics.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_Gla1ExtendIncluded( Aig_Gla1Man_t * p )
{
    Aig_Obj_t * pObj;
    int i, k;
    Aig_ManForEachNode( p->pAig, pObj, i )
    {
        if ( !Vec_IntEntry( p->vIncluded, i ) )
            continue;
        Cnf_ComputeClauses( p->pAig, pObj, p->vLeaves, p->vVolume, NULL, p->vCover, p->vNodes );
        Vec_PtrForEachEntry( Aig_Obj_t *, p->vVolume, pObj, k )
        {
            assert( Aig_ObjId(pObj) <= i );
            Vec_IntWriteEntry( p->vIncluded, Aig_ObjId(pObj), 1 );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Performs gate-level localization abstraction.]

  Description [Returns array of objects included in the abstraction. This array
  may contain only const1, flop outputs, and internal nodes, that is, objects
  that should have clauses added to the SAT solver.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Aig_Gla1ManPerform( Aig_Man_t * pAig, Vec_Int_t * vGateClassesOld, int nStart, int nFramesMax, int nConfLimit, int TimeLimit, int fNaiveCnf, int fVerbose, int * piFrame )
{
    Vec_Int_t * vResult = NULL;
    Aig_Gla1Man_t * p;
    Aig_Man_t * pAbs;
    Aig_Obj_t * pObj;
    Abc_Cex_t * pCex;
    Vec_Int_t * vPPiRefine;
    int f, g, r, i, iSatVar, Lit, Entry, RetValue;
    int nConfBef, nConfAft;
    clock_t clk, clkTotal = clock();
    clock_t nTimeToStop = TimeLimit ? TimeLimit * CLOCKS_PER_SEC + clock(): 0;
    assert( Saig_ManPoNum(pAig) == 1 );

    if ( nFramesMax == 0 )
        nFramesMax = ABC_INFINITY;

    if ( fVerbose )
    {
        if ( TimeLimit )
            printf( "Abstracting from frame %d to frame %d with timeout %d sec.\n", nStart, nFramesMax, TimeLimit );
        else
            printf( "Abstracting from frame %d to frame %d with no timeout.\n", nStart, nFramesMax );
    }

    // start the solver
    p = Aig_Gla1ManStart( pAig, vGateClassesOld, fNaiveCnf );
    p->nFramesMax = nFramesMax;
    p->nConfLimit = nConfLimit;
    p->fVerbose   = fVerbose;

    // include constant node
    Vec_IntWriteEntry( p->vIncluded, 0, 1 );
    Aig_Gla1FetchVecId( p, Aig_ManConst1(pAig) );

    // set runtime limit
    if ( TimeLimit )
        sat_solver_set_runtime_limit( p->pSat, nTimeToStop );

    // iterate over the timeframes
    for ( f = 0; f < nFramesMax; f++ )
    {
        // initialize abstraction in this timeframe
        Aig_ManForEachObjVec( p->vAssigned, pAig, pObj, i )
            if ( Vec_IntEntry(p->vIncluded, Aig_ObjId(pObj)) )
                if ( !Aig_Gla1ObjAddToSolver( p, pObj, f ) )
                    printf( "Error!  SAT solver became UNSAT.\n" );

        // skip checking if we are not supposed to
        if ( f < nStart )
            continue;

        // create output literal to represent property failure
        pObj    = Aig_ManCo( pAig, 0 );
        iSatVar = Aig_Gla1FetchVar( p, Aig_ObjFanin0(pObj), f );
        Lit     = toLitCond( iSatVar, Aig_ObjFaninC0(pObj) );

        // try solving the abstraction
        Aig_Gla1CollectAbstr( p );
        for ( r = 0; r < ABC_INFINITY; r++ )
        {
            // try to find a counter-example
            clk = clock();
            nConfBef = p->pSat->stats.conflicts;
            RetValue = sat_solver_solve( p->pSat, &Lit, &Lit + 1, 
                (ABC_INT64_T)nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
            nConfAft = p->pSat->stats.conflicts;
            p->timeSat += clock() - clk;
            if ( RetValue != l_True )
            {
                if ( fVerbose )
                {
                    if ( r == 0 )
                        printf( "== %3d ==", f );
                    else
                        printf( "         " );
                    if ( TimeLimit && clock() > nTimeToStop )
                        printf( "       SAT solver timed out after %d seconds.\n", TimeLimit );
                    else if ( RetValue != l_False )
                        printf( "       SAT solver returned UNDECIDED after %5d conflicts.\n", nConfAft - nConfBef );
                    else
                    {
                        printf( "       SAT solver returned UNSAT after %5d conflicts.  ", nConfAft - nConfBef );
                        Abc_PrintTime( 1, "Total time", clock() - clkTotal );
                    }
                }
                break;
            }
            clk = clock();
            // derive abstraction
            pAbs = Aig_Gla1DeriveAbs( p );
            // derive counter-example
            pCex = Aig_Gla1DeriveCex( p, f );
            // verify the counter-example
            RetValue = Saig_ManVerifyCex( pAbs, pCex );
            if ( RetValue == 0 )
                printf( "Error!  CEX is invalid.\n" );
            // perform refinement
            vPPiRefine = Saig_ManCbaFilterInputs( pAbs, Vec_IntSize(p->vPis), pCex, 0 );
            Vec_IntForEachEntry( vPPiRefine, Entry, i )
            {
                pObj = Aig_ManObj( pAig, Vec_IntEntry(p->vPPis, Entry) );
                assert( Aig_ObjIsNode(pObj) || Saig_ObjIsLo(p->pAig, pObj) );
                assert( Vec_IntEntry( p->vIncluded, Aig_ObjId(pObj) ) == 0 );
                Vec_IntWriteEntry( p->vIncluded, Aig_ObjId(pObj), 1 );
                for ( g = 0; g <= f; g++ )
                    if ( !Aig_Gla1ObjAddToSolver( p, pObj, g ) )
                        printf( "Error!  SAT solver became UNSAT.\n" );
            }
            if ( Vec_IntSize(vPPiRefine) == 0 )
            {
                Vec_IntFreeP( &p->vIncluded );
                Vec_IntFree( vPPiRefine );
                Aig_ManStop( pAbs );
                Abc_CexFree( pCex );
                break;
            }
            Vec_IntFree( vPPiRefine );
            Aig_ManStop( pAbs );
            Abc_CexFree( pCex );
            p->timeRef += clock() - clk;

            // prepare abstraction
            Aig_Gla1CollectAbstr( p );
            if ( fVerbose )
                Aig_Gla1PrintAbstr( p, f, r, p->pSat->size, nConfAft - nConfBef );
        }
        if ( RetValue != l_False )
            break;
    }
    p->timeTotal = clock() - clkTotal;
    if ( f == nFramesMax )
        printf( "Finished %d frames without exceeding conflict limit (%d).\n", f, nConfLimit );
    else if ( p->vIncluded == NULL )
        printf( "The problem is SAT in frame %d. The CEX is currently not produced.\n", f );
    else
        printf( "Ran out of conflict limit (%d) at frame %d.\n", nConfLimit, f );
    *piFrame = f;
    // print stats
    if ( fVerbose )
    {
        ABC_PRTP( "Sat   ", p->timeSat,   p->timeTotal );
        ABC_PRTP( "Ref   ", p->timeRef,   p->timeTotal );
        ABC_PRTP( "Total ", p->timeTotal, p->timeTotal );
    }
    // prepare return value
    if ( !fNaiveCnf && p->vIncluded )
        Aig_Gla1ExtendIncluded( p );
    vResult = p->vIncluded;  p->vIncluded = NULL;
    Aig_Gla1ManStop( p );
    return vResult;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

