/**CFile****************************************************************

  FileName    [cecSat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Combinational equivalence checking.]

  Synopsis    [Detection of structural isomorphism.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cecSat.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig/gia/gia.h"
#include "misc/util/utilTruth.h"
#include "sat/glucose/AbcGlucose.h"
#include "cec.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// sweeping manager
typedef struct Cec3_Par_t_ Cec3_Par_t;
struct Cec3_Par_t_
{
    int              nSimWords;     // simulation words
    int              nSimRounds;    // simulation rounds
    int              nItersMax;     // max number of iterations
    int              nConfLimit;    // SAT solver conflict limit
    int              fIsMiter;      // this is a miter
    int              fUseCones;     // use logic cones
    int              fVeryVerbose;  // verbose stats
    int              fVerbose;      // verbose stats
};

// SAT solving manager
typedef struct Cec3_Man_t_ Cec3_Man_t;
struct Cec3_Man_t_
{
    Cec3_Par_t *     pPars;          // parameters
    Gia_Man_t *      pAig;           // user's AIG
    Gia_Man_t *      pNew;           // internal AIG
    // SAT solving
    bmcg_sat_solver* pSat;           // SAT solver
    Vec_Ptr_t *      vFrontier;      // CNF construction
    Vec_Ptr_t *      vFanins;        // CNF construction
    Vec_Wrd_t *      vSims;          // CI simulation info
    Vec_Int_t *      vNodesNew;      // nodes
    Vec_Int_t *      vSatVars;       // nodes
    Vec_Int_t *      vObjSatPairs;   // nodes
    Vec_Int_t *      vCexTriples;    // nodes
    // statistics
    int              nPatterns;
    int              nSatSat;
    int              nSatUnsat;
    int              nSatUndec;
    abctime          timeSatSat;
    abctime          timeSatUnsat;
    abctime          timeSatUndec;
    abctime          timeSim;
    abctime          timeRefine;
    abctime          timeExtra;
    abctime          timeStart;
};

static inline int    Cec3_ObjSatId( Gia_Man_t * p, Gia_Obj_t * pObj )             { return Gia_ObjCopy2Array(p, Gia_ObjId(p, pObj));                                                     }
static inline int    Cec3_ObjSetSatId( Gia_Man_t * p, Gia_Obj_t * pObj, int Num ) { assert(Cec3_ObjSatId(p, pObj) == -1); Gia_ObjSetCopy2Array(p, Gia_ObjId(p, pObj), Num); return Num;  }
static inline void   Cec3_ObjCleanSatId( Gia_Man_t * p, Gia_Obj_t * pObj )        { assert(Cec3_ObjSatId(p, pObj) != -1); Gia_ObjSetCopy2Array(p, Gia_ObjId(p, pObj), -1);               }

static inline void   satoko_mark_cone( bmcg_sat_solver * p, int * pVars, int nVars )   {}
static inline void   satoko_unmark_cone( bmcg_sat_solver * p, int * pVars, int nVars ) {}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Sets parameter defaults.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec3_SetDefaultParams( Cec3_Par_t * p )
{
    memset( p, 0, sizeof(Cec3_Par_t) );
    p->nSimWords      =      12;    // simulation words
    p->nSimRounds     =       4;    // simulation rounds
    p->nItersMax      =      10;    // max number of iterations
    p->nConfLimit     =    1000;    // conflict limit at a node
    p->fIsMiter       =       0;    // this is a miter
    p->fUseCones      =       0;    // use logic cones
    p->fVeryVerbose   =       0;    // verbose stats
    p->fVerbose       =       0;    // verbose stats
}  

/**Function*************************************************************

  Synopsis    [Adds clauses to the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec3_AddClausesMux( Gia_Man_t * p, Gia_Obj_t * pNode, bmcg_sat_solver * pSat )
{
    int fPolarFlip = 0;
    Gia_Obj_t * pNodeI, * pNodeT, * pNodeE;
    int pLits[4], RetValue, VarF, VarI, VarT, VarE, fCompT, fCompE;

    assert( !Gia_IsComplement( pNode ) );
    assert( pNode->fMark0 );
    // get nodes (I = if, T = then, E = else)
    pNodeI = Gia_ObjRecognizeMux( pNode, &pNodeT, &pNodeE );
    // get the variable numbers
    VarF = Cec3_ObjSatId(p, pNode);
    VarI = Cec3_ObjSatId(p, pNodeI);
    VarT = Cec3_ObjSatId(p, Gia_Regular(pNodeT));
    VarE = Cec3_ObjSatId(p, Gia_Regular(pNodeE));
    // get the complementation flags
    fCompT = Gia_IsComplement(pNodeT);
    fCompE = Gia_IsComplement(pNodeE);

    // f = ITE(i, t, e)

    // i' + t' + f
    // i' + t  + f'
    // i  + e' + f
    // i  + e  + f'

    // create four clauses
    pLits[0] = Abc_Var2Lit(VarI, 1);
    pLits[1] = Abc_Var2Lit(VarT, 1^fCompT);
    pLits[2] = Abc_Var2Lit(VarF, 0);
    if ( fPolarFlip )
    {
        if ( pNodeI->fPhase )               pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeT)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = bmcg_sat_solver_addclause( pSat, pLits, 3 );
    assert( RetValue );
    pLits[0] = Abc_Var2Lit(VarI, 1);
    pLits[1] = Abc_Var2Lit(VarT, 0^fCompT);
    pLits[2] = Abc_Var2Lit(VarF, 1);
    if ( fPolarFlip )
    {
        if ( pNodeI->fPhase )               pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeT)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = bmcg_sat_solver_addclause( pSat, pLits, 3 );
    assert( RetValue );
    pLits[0] = Abc_Var2Lit(VarI, 0);
    pLits[1] = Abc_Var2Lit(VarE, 1^fCompE);
    pLits[2] = Abc_Var2Lit(VarF, 0);
    if ( fPolarFlip )
    {
        if ( pNodeI->fPhase )               pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeE)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = bmcg_sat_solver_addclause( pSat, pLits, 3 );
    assert( RetValue );
    pLits[0] = Abc_Var2Lit(VarI, 0);
    pLits[1] = Abc_Var2Lit(VarE, 0^fCompE);
    pLits[2] = Abc_Var2Lit(VarF, 1);
    if ( fPolarFlip )
    {
        if ( pNodeI->fPhase )               pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeE)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = bmcg_sat_solver_addclause( pSat, pLits, 3 );
    assert( RetValue );

    // two additional clauses
    // t' & e' -> f'
    // t  & e  -> f 

    // t  + e   + f'
    // t' + e'  + f 

    if ( VarT == VarE )
    {
//        assert( fCompT == !fCompE );
        return;
    }

    pLits[0] = Abc_Var2Lit(VarT, 0^fCompT);
    pLits[1] = Abc_Var2Lit(VarE, 0^fCompE);
    pLits[2] = Abc_Var2Lit(VarF, 1);
    if ( fPolarFlip )
    {
        if ( Gia_Regular(pNodeT)->fPhase )  pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeE)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = bmcg_sat_solver_addclause( pSat, pLits, 3 );
    assert( RetValue );
    pLits[0] = Abc_Var2Lit(VarT, 1^fCompT);
    pLits[1] = Abc_Var2Lit(VarE, 1^fCompE);
    pLits[2] = Abc_Var2Lit(VarF, 0);
    if ( fPolarFlip )
    {
        if ( Gia_Regular(pNodeT)->fPhase )  pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeE)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = bmcg_sat_solver_addclause( pSat, pLits, 3 );
    assert( RetValue );
}
void Cec3_AddClausesSuper( Gia_Man_t * p, Gia_Obj_t * pNode, Vec_Ptr_t * vSuper, bmcg_sat_solver * pSat )
{
    int fPolarFlip = 0;
    Gia_Obj_t * pFanin;
    int * pLits, nLits, RetValue, i;
    assert( !Gia_IsComplement(pNode) );
    assert( Gia_ObjIsAnd( pNode ) );
    // create storage for literals
    nLits = Vec_PtrSize(vSuper) + 1;
    pLits = ABC_ALLOC( int, nLits );
    // suppose AND-gate is A & B = C
    // add !A => !C   or   A + !C
    Vec_PtrForEachEntry( Gia_Obj_t *, vSuper, pFanin, i )
    {
        pLits[0] = Abc_Var2Lit(Cec3_ObjSatId(p, Gia_Regular(pFanin)), Gia_IsComplement(pFanin));
        pLits[1] = Abc_Var2Lit(Cec3_ObjSatId(p, pNode), 1);
        if ( fPolarFlip )
        {
            if ( Gia_Regular(pFanin)->fPhase )  pLits[0] = Abc_LitNot( pLits[0] );
            if ( pNode->fPhase )                pLits[1] = Abc_LitNot( pLits[1] );
        }
        RetValue = bmcg_sat_solver_addclause( pSat, pLits, 2 );
        assert( RetValue );
    }
    // add A & B => C   or   !A + !B + C
    Vec_PtrForEachEntry( Gia_Obj_t *, vSuper, pFanin, i )
    {
        pLits[i] = Abc_Var2Lit(Cec3_ObjSatId(p, Gia_Regular(pFanin)), !Gia_IsComplement(pFanin));
        if ( fPolarFlip )
        {
            if ( Gia_Regular(pFanin)->fPhase )  pLits[i] = Abc_LitNot( pLits[i] );
        }
    }
    pLits[nLits-1] = Abc_Var2Lit(Cec3_ObjSatId(p, pNode), 0);
    if ( fPolarFlip )
    {
        if ( pNode->fPhase )  pLits[nLits-1] = Abc_LitNot( pLits[nLits-1] );
    }
    RetValue = bmcg_sat_solver_addclause( pSat, pLits, nLits );
    assert( RetValue );
    ABC_FREE( pLits );
}

/**Function*************************************************************

  Synopsis    [Adds clauses and returns CNF variable of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec3_CollectSuper_rec( Gia_Obj_t * pObj, Vec_Ptr_t * vSuper, int fFirst, int fUseMuxes )
{
    // if the new node is complemented or a PI, another gate begins
    if ( Gia_IsComplement(pObj) || Gia_ObjIsCi(pObj) || 
         (!fFirst && Gia_ObjValue(pObj) > 1) || 
         (fUseMuxes && pObj->fMark0) )
    {
        Vec_PtrPushUnique( vSuper, pObj );
        return;
    }
    // go through the branches
    Cec3_CollectSuper_rec( Gia_ObjChild0(pObj), vSuper, 0, fUseMuxes );
    Cec3_CollectSuper_rec( Gia_ObjChild1(pObj), vSuper, 0, fUseMuxes );
}
void Cec3_CollectSuper( Gia_Obj_t * pObj, int fUseMuxes, Vec_Ptr_t * vSuper )
{
    assert( !Gia_IsComplement(pObj) );
    assert( !Gia_ObjIsCi(pObj) );
    Vec_PtrClear( vSuper );
    Cec3_CollectSuper_rec( pObj, vSuper, 1, fUseMuxes );
}
void Cec3_ObjAddToFrontier( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Ptr_t * vFrontier, bmcg_sat_solver * pSat )
{
    assert( !Gia_IsComplement(pObj) );
    assert( !Gia_ObjIsConst0(pObj) );
    if ( Cec3_ObjSatId(p, pObj) >= 0 )
        return;
    assert( Cec3_ObjSatId(p, pObj) == -1 );
    Cec3_ObjSetSatId( p, pObj, bmcg_sat_solver_addvar(pSat) );
    if ( Gia_ObjIsAnd(pObj) )
        Vec_PtrPush( vFrontier, pObj );
}
int Cec3_ObjGetCnfVar( Cec3_Man_t * p, int iObj )
{ 
    Gia_Obj_t * pNode, * pFanin;
    Gia_Obj_t * pObj = Gia_ManObj(p->pNew, iObj);
    int i, k, fUseMuxes = 1;
    // quit if CNF is ready
    if ( Cec3_ObjSatId(p->pNew,pObj) >= 0 )
        return Cec3_ObjSatId(p->pNew,pObj);
    assert( iObj > 0 );
    if ( Gia_ObjIsCi(pObj) )
        return Cec3_ObjSetSatId( p->pNew, pObj, bmcg_sat_solver_addvar(p->pSat) );
    assert( Gia_ObjIsAnd(pObj) );
    // start the frontier
    Vec_PtrClear( p->vFrontier );
    Cec3_ObjAddToFrontier( p->pNew, pObj, p->vFrontier, p->pSat );
    // explore nodes in the frontier
    Vec_PtrForEachEntry( Gia_Obj_t *, p->vFrontier, pNode, i )
    {
        // create the supergate
        assert( Cec3_ObjSatId(p->pNew,pNode) >= 0 );
        if ( fUseMuxes && pNode->fMark0 )
        {
            Vec_PtrClear( p->vFanins );
            Vec_PtrPushUnique( p->vFanins, Gia_ObjFanin0( Gia_ObjFanin0(pNode) ) );
            Vec_PtrPushUnique( p->vFanins, Gia_ObjFanin0( Gia_ObjFanin1(pNode) ) );
            Vec_PtrPushUnique( p->vFanins, Gia_ObjFanin1( Gia_ObjFanin0(pNode) ) );
            Vec_PtrPushUnique( p->vFanins, Gia_ObjFanin1( Gia_ObjFanin1(pNode) ) );
            Vec_PtrForEachEntry( Gia_Obj_t *, p->vFanins, pFanin, k )
                Cec3_ObjAddToFrontier( p->pNew, Gia_Regular(pFanin), p->vFrontier, p->pSat );
            Cec3_AddClausesMux( p->pNew, pNode, p->pSat );
        }
        else
        {
            Cec3_CollectSuper( pNode, fUseMuxes, p->vFanins );
            Vec_PtrForEachEntry( Gia_Obj_t *, p->vFanins, pFanin, k )
                Cec3_ObjAddToFrontier( p->pNew, Gia_Regular(pFanin), p->vFrontier, p->pSat );
            Cec3_AddClausesSuper( p->pNew, pNode, p->vFanins, p->pSat );
        }
        assert( Vec_PtrSize(p->vFanins) > 1 );
    }
    return Cec3_ObjSatId(p->pNew,pObj);
}



/**Function*************************************************************

  Synopsis    [Internal simulation APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word * Cec3_ObjSim( Gia_Man_t * p, int iObj )
{
    return Vec_WrdEntryP( p->vSims, p->nSimWords * iObj );
}
static inline void Cec3_ObjSimSetInputBit( Gia_Man_t * p, int iObj, int Bit )
{
    word * pSim = Cec3_ObjSim( p, iObj );
    if ( Abc_InfoHasBit( (unsigned*)pSim, p->iPatsPi ) != Bit )
        Abc_InfoXorBit( (unsigned*)pSim, p->iPatsPi );
}
static inline void Cec3_ObjSimRo( Gia_Man_t * p, int iObj )
{
    int w;
    word * pSimRo = Cec3_ObjSim( p, iObj );
    word * pSimRi = Cec3_ObjSim( p, Gia_ObjRoToRiId(p, iObj) );
    for ( w = 0; w < p->nSimWords; w++ )
        pSimRo[w] = pSimRi[w];
}
static inline void Cec3_ObjSimCo( Gia_Man_t * p, int iObj )
{
    int w;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    word * pSimCo  = Cec3_ObjSim( p, iObj );
    word * pSimDri = Cec3_ObjSim( p, Gia_ObjFaninId0(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSimCo[w] = ~pSimDri[w];
    else
        for ( w = 0; w < p->nSimWords; w++ )
            pSimCo[w] =  pSimDri[w];
}
static inline void Cec3_ObjSimAnd( Gia_Man_t * p, int iObj )
{
    int w;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    word * pSim  = Cec3_ObjSim( p, iObj );
    word * pSim0 = Cec3_ObjSim( p, Gia_ObjFaninId0(pObj, iObj) );
    word * pSim1 = Cec3_ObjSim( p, Gia_ObjFaninId1(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = ~pSim0[w] & ~pSim1[w];
    else if ( Gia_ObjFaninC0(pObj) && !Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = ~pSim0[w] & pSim1[w];
    else if ( !Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = pSim0[w] & ~pSim1[w];
    else
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = pSim0[w] & pSim1[w];
}
static inline int Cec3_ObjSimEqual( Gia_Man_t * p, int iObj0, int iObj1 )
{
    int w;
    word * pSim0 = Cec3_ObjSim( p, iObj0 );
    word * pSim1 = Cec3_ObjSim( p, iObj1 );
    if ( (pSim0[0] & 1) == (pSim1[0] & 1) )
    {
        for ( w = 0; w < p->nSimWords; w++ )
            if ( pSim0[w] != pSim1[w] )
                return 0;
        return 1;
    }
    else
    {
        for ( w = 0; w < p->nSimWords; w++ )
            if ( pSim0[w] != ~pSim1[w] )
                return 0;
        return 1;
    }
}
static inline void Cec3_ObjSimCi( Gia_Man_t * p, int iObj )
{
    int w;
    word * pSim = Cec3_ObjSim( p, iObj );
    for ( w = 0; w < p->nSimWords; w++ )
        pSim[w] = Gia_ManRandomW( 0 );
    pSim[0] <<= 1;
}
void Cec3_ManSimulateCis( Gia_Man_t * p )
{
    int i, Id;
    Gia_ManForEachCiId( p, Id, i )
        Cec3_ObjSimCi( p, Id );
    p->iPatsPi = 0;
}
Abc_Cex_t * Cec3_ManDeriveCex( Gia_Man_t * p, int iOut, int iPat )
{
    Abc_Cex_t * pCex;
    int i, Id;
    pCex = Abc_CexAlloc( 0, Gia_ManCiNum(p), 1 );
    pCex->iPo = iOut;
    if ( iPat == -1 )
        return pCex;
    Gia_ManForEachCiId( p, Id, i )
        if ( Abc_InfoHasBit((unsigned *)Cec3_ObjSim(p, Id), iPat) )
            Abc_InfoSetBit( pCex->pData, i );
    return pCex;
}
int Cec3_ManSimulateCos( Gia_Man_t * p )
{
    int i, Id;
    // check outputs and generate CEX if they fail
    Gia_ManForEachCoId( p, Id, i )
    {
        Cec3_ObjSimCo( p, Id );
        if ( Cec3_ObjSimEqual(p, Id, 0) )
            continue;
        p->pCexSeq = Cec3_ManDeriveCex( p, i, Abc_TtFindFirstBit2(Cec3_ObjSim(p, Id), p->nSimWords) );
        return 0;
    }
    return 1;
}
void Cec3_ManSaveCis( Gia_Man_t * p )
{
    int w, i, Id;
    assert( p->vSimsPi != NULL );
    for ( w = 0; w < p->nSimWords; w++ )
        Gia_ManForEachCiId( p, Id, i )
            Vec_WrdPush( p->vSimsPi, Cec3_ObjSim(p, Id)[w] );
}
int Cec3_ManSimulate( Gia_Man_t * p, Vec_Int_t * vTriples, Cec3_Man_t * pMan )
{
    extern void Cec3_ManSimClassRefineOne( Gia_Man_t * p, int iRepr );
    abctime clk = Abc_Clock();
    Gia_Obj_t * pObj; 
    int i, iRepr, iObj, Entry, Count = 0;
    //Cec3_ManSaveCis( p );
    Gia_ManForEachAnd( p, pObj, i )
        Cec3_ObjSimAnd( p, i );
    pMan->timeSim += Abc_Clock() - clk;
    if ( p->pReprs == NULL )
        return 0;
    if ( vTriples )
    {
        Vec_IntForEachEntryTriple( vTriples, iRepr, iObj, Entry, i )
        {
            word * pSim0 = Cec3_ObjSim( p, iRepr );
            word * pSim1 = Cec3_ObjSim( p, iObj );
            int iPat     = Abc_Lit2Var(Entry);
            int fPhase   = Abc_LitIsCompl(Entry);
            if ( (fPhase ^ Abc_InfoHasBit((unsigned *)pSim0, iPat)) == Abc_InfoHasBit((unsigned *)pSim1, iPat) )
            {
                //printf( "ERROR:  Pattern %d did not disprove pair %d and %d.\n", iPat, iRepr, iObj );
                Count++;
            }
        }
    }
    clk = Abc_Clock();
    Gia_ManForEachClass0( p, i )
        Cec3_ManSimClassRefineOne( p, i );
    pMan->timeRefine += Abc_Clock() - clk;
    return Count;
}
void Cec3_ManSimAlloc( Gia_Man_t * p, int nWords )
{
    Vec_WrdFreeP( &p->vSims );
    Vec_WrdFreeP( &p->vSimsPi );
    p->vSims   = Vec_WrdStart( Gia_ManObjNum(p) * nWords );
    p->vSimsPi = Vec_WrdAlloc( Gia_ManCiNum(p) * nWords * 4 ); // storage for CI patterns
    p->nSimWords = nWords;
}


/**Function*************************************************************

  Synopsis    [Computes hash key of the simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec3_ManSimHashKey( word * pSim, int nSims, int nTableSize )
{
    static int s_Primes[16] = { 
        1291, 1699, 1999, 2357, 2953, 3313, 3907, 4177, 
        4831, 5147, 5647, 6343, 6899, 7103, 7873, 8147 };
    unsigned uHash = 0, * pSimU = (unsigned *)pSim;
    int i, nSimsU = 2 * nSims;
    if ( pSimU[0] & 1 )
        for ( i = 0; i < nSimsU; i++ )
            uHash ^= ~pSimU[i] * s_Primes[i & 0xf];
    else
        for ( i = 0; i < nSimsU; i++ )
            uHash ^= pSimU[i] * s_Primes[i & 0xf];
    return (int)(uHash % nTableSize);

}

/**Function*************************************************************

  Synopsis    [Creating initial equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec3_ManSimClassRefineOne( Gia_Man_t * p, int iRepr )
{
    int iObj, iPrev = iRepr, iPrev2, iRepr2;
    Gia_ClassForEachObj1( p, iRepr, iRepr2 )
        if ( Cec3_ObjSimEqual(p, iRepr, iRepr2) )
            iPrev = iRepr2;
        else
            break;
    if ( iRepr2 <= 0 ) // no refinement
        return;
    // relink remaining nodes of the class
    // nodes that are equal to iRepr, remain in the class of iRepr
    // nodes that are not equal to iRepr, move to the class of iRepr2
    Gia_ObjSetRepr( p, iRepr2, GIA_VOID );
    iPrev2 = iRepr2;
    for ( iObj = Gia_ObjNext(p, iRepr2); iObj > 0; iObj = Gia_ObjNext(p, iObj) )
    {
        if ( Cec3_ObjSimEqual(p, iRepr, iObj) ) // remains with iRepr
        {
            Gia_ObjSetNext( p, iPrev, iObj );
            iPrev = iObj;
        }
        else // moves to iRepr2
        {
            Gia_ObjSetRepr( p, iObj, iRepr2 );
            Gia_ObjSetNext( p, iPrev2, iObj );
            iPrev2 = iObj;
        }
    }
    Gia_ObjSetNext( p, iPrev, -1 );
    Gia_ObjSetNext( p, iPrev2, -1 );
}
void Cec3_ManCreateClasses( Gia_Man_t * p, Cec3_Man_t * pMan )
{
    abctime clk;
    Gia_Obj_t * pObj;
    int nWords = p->nSimWords;
    int * pTable, nTableSize, i, Key;
    // allocate representation
    ABC_FREE( p->pReprs );
    ABC_FREE( p->pNexts );
    p->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(p) );
    p->pNexts = ABC_FALLOC( int, Gia_ManObjNum(p) );
    // hash each node by its simulation info
    nTableSize = Abc_PrimeCudd( Gia_ManObjNum(p) );
    pTable = ABC_FALLOC( int, nTableSize );
    Gia_ManForEachObj( p, pObj, i )
    {
        p->pReprs[i].iRepr = GIA_VOID;
        if ( Gia_ObjIsCo(pObj) )
            continue;
        Key = Cec3_ManSimHashKey( Cec3_ObjSim(p, i), nWords, nTableSize );
        assert( Key >= 0 && Key < nTableSize );
        if ( pTable[Key] == -1 )
            pTable[Key] = i;
        else
            Gia_ObjSetRepr( p, i, pTable[Key] );
    }
    // create classes
    for ( i = Gia_ManObjNum(p) - 1; i >= 0; i-- )
    {
        int iRepr = Gia_ObjRepr(p, i);
        if ( iRepr == GIA_VOID )
            continue;
        Gia_ObjSetNext( p, i, Gia_ObjNext(p, iRepr) );
        Gia_ObjSetNext( p, iRepr, i );
    }
    ABC_FREE( pTable );
    clk = Abc_Clock();
    Gia_ManForEachClass0( p, i )
        Cec3_ManSimClassRefineOne( p, i );
    pMan->timeRefine += Abc_Clock() - clk;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cec3_Man_t * Cec3_ManCreate( Gia_Man_t * pAig, Cec3_Par_t * pPars )
{
    Cec3_Man_t * p;
    Gia_Obj_t * pObj; int i;
    //satoko_opts_t Pars;
    //assert( Gia_ManRegNum(pAig) == 0 );
    p = ABC_CALLOC( Cec3_Man_t, 1 );
    memset( p, 0, sizeof(Cec3_Man_t) );
    p->timeStart    = Abc_Clock();
    p->pPars        = pPars;
    p->pAig         = pAig;
    // create new manager
    p->pNew         = Gia_ManStart( Gia_ManObjNum(pAig) );
    Gia_ManFillValue( pAig );
    Gia_ManConst0(pAig)->Value = 0;
    Gia_ManForEachCi( pAig, pObj, i )
        pObj->Value = Gia_ManAppendCi( p->pNew );
    Gia_ManHashAlloc( p->pNew );
    Vec_IntFill( &p->pNew->vCopies2, Gia_ManObjNum(p->pNew), -1 );
    // SAT solving
    //memset( &Pars, 0, sizeof(satoko_opts_t) );
    p->pSat         = bmcg_sat_solver_start();
    p->vFrontier    = Vec_PtrAlloc( 1000 );
    p->vFanins      = Vec_PtrAlloc( 100 );
    p->vNodesNew    = Vec_IntAlloc( 100 );
    p->vSatVars     = Vec_IntAlloc( 100 );
    p->vObjSatPairs = Vec_IntAlloc( 100 );
    p->vCexTriples  = Vec_IntAlloc( 100 );
    //Pars.conf_limit = pPars->nConfLimit;
    //satoko_configure(p->pSat, &Pars);
    // remember pointer to the solver in the AIG manager
    pAig->pData     = p->pSat;
    return p;
}
void Cec3_ManDestroy( Cec3_Man_t * p )
{
    if ( p->pPars->fVerbose ) 
    {
        abctime timeTotal = Abc_Clock() - p->timeStart;
        abctime timeSat   = p->timeSatSat + p->timeSatUnsat + p->timeSatUndec;
        abctime timeOther = timeTotal - timeSat - p->timeSim - p->timeRefine - p->timeExtra;
//        Abc_Print( 1, "%d\n", p->Num );
        ABC_PRTP( "SAT solving", timeSat,          timeTotal );
        ABC_PRTP( "  sat      ", p->timeSatSat,    timeTotal );
        ABC_PRTP( "  unsat    ", p->timeSatUnsat,  timeTotal );
        ABC_PRTP( "  fail     ", p->timeSatUndec,  timeTotal );
        ABC_PRTP( "Simulation ", p->timeSim,       timeTotal );
        ABC_PRTP( "Refinement ", p->timeRefine,    timeTotal );
        ABC_PRTP( "Rollback   ", p->timeExtra,     timeTotal );
        ABC_PRTP( "Other      ", timeOther,        timeTotal );
        ABC_PRTP( "TOTAL      ", timeTotal,        timeTotal );
        fflush( stdout );
    }

    Vec_WrdFreeP( &p->pAig->vSims );
    //Vec_WrdFreeP( &p->pAig->vSimsPi );
    Gia_ManCleanMark01( p->pAig );
    bmcg_sat_solver_stop( p->pSat );
    Gia_ManStopP( &p->pNew );
    Vec_PtrFreeP( &p->vFrontier );
    Vec_PtrFreeP( &p->vFanins );
    Vec_IntFreeP( &p->vNodesNew );
    Vec_IntFreeP( &p->vSatVars );
    Vec_IntFreeP( &p->vObjSatPairs );
    Vec_IntFreeP( &p->vCexTriples );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Verify counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec3_ManVerify_rec( Gia_Man_t * p, int iObj, bmcg_sat_solver * pSat )
{
    int Value0, Value1;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    if ( iObj == 0 ) return 0;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return pObj->fMark1;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    if ( Gia_ObjIsCi(pObj) )
//        return pObj->fMark1 = satoko_var_polarity(pSat, Cec3_ObjSatId(p, pObj)) == SATOKO_LIT_TRUE;
        return pObj->fMark1 = bmcg_sat_solver_read_cex_varvalue(pSat, Cec3_ObjSatId(p, pObj));
    assert( Gia_ObjIsAnd(pObj) );
    Value0 = Cec3_ManVerify_rec( p, Gia_ObjFaninId0(pObj, iObj), pSat ) ^ Gia_ObjFaninC0(pObj);
    Value1 = Cec3_ManVerify_rec( p, Gia_ObjFaninId1(pObj, iObj), pSat ) ^ Gia_ObjFaninC1(pObj);
    return pObj->fMark1 = Value0 & Value1;
}
void Cec3_ManVerify( Gia_Man_t * p, int iObj0, int iObj1, int fPhase, bmcg_sat_solver * pSat )
{
//    int val0 = satoko_var_polarity(pSat, Cec3_ObjSatId(p, Gia_ManObj(p, iObj0))) == SATOKO_LIT_TRUE;
//    int val1 = satoko_var_polarity(pSat, Cec3_ObjSatId(p, Gia_ManObj(p, iObj1))) == SATOKO_LIT_TRUE;
    int Value0, Value1;
    Gia_ManIncrementTravId( p );
    Value0 = Cec3_ManVerify_rec( p, iObj0, pSat );
    Value1 = Cec3_ManVerify_rec( p, iObj1, pSat );
    if ( (Value0 ^ Value1) == fPhase )
        printf( "CEX verification FAILED for obj %d and obj %d.\n", iObj0, iObj1 );
//    else
//        printf( "CEX verification succeeded for obj %d and obj %d.\n", iObj0, iObj1 );;
}

/**Function*************************************************************

  Synopsis    [Internal simulation APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec3_ManCollect_rec( Cec3_Man_t * p, int iObj )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjIsTravIdCurrentId(p->pNew, iObj) )
        return;
    Gia_ObjSetTravIdCurrentId(p->pNew, iObj);
    pObj = Gia_ManObj( p->pNew, iObj );
    if ( Cec3_ObjSatId(p->pNew, pObj) >= 0 )
    {
        Vec_IntPush( p->vNodesNew, iObj );
        Vec_IntPush( p->vSatVars, Cec3_ObjSatId(p->pNew, pObj) );
    }
    if ( !iObj )
        return;
    if ( Gia_ObjIsAnd(pObj) )
    {
        Cec3_ManCollect_rec( p, Gia_ObjFaninId0(pObj, iObj) );
        Cec3_ManCollect_rec( p, Gia_ObjFaninId1(pObj, iObj) );
    }
    else
    {
        assert( Cec3_ObjSatId(p->pNew, pObj) >= 0 );
        Vec_IntPushTwo( p->vObjSatPairs, Gia_ManCiIdToId(p->pAig, Gia_ObjCioId(pObj)), Cec3_ObjSatId(p->pNew, pObj) ); // SAT var
    }
}
int Cec3_ManSolveTwo( Cec3_Man_t * p, int iObj0, int iObj1, int fPhase )
{
    Gia_Obj_t * pObj;
    int status, i, iVar0, iVar1, Lits[2];
    if (iObj1 < iObj0) 
        iObj1 ^= iObj0, iObj0 ^= iObj1, iObj1 ^= iObj0;
    assert( iObj0 < iObj1 );
    assert( p->pPars->fUseCones || bmcg_sat_solver_varnum(p->pSat) == 0 );
    if ( !iObj0 && Cec3_ObjSatId(p->pNew, Gia_ManConst0(p->pNew)) == -1 )
        Cec3_ObjSetSatId( p->pNew, Gia_ManConst0(p->pNew), bmcg_sat_solver_addvar(p->pSat) );
    iVar0 = Cec3_ObjGetCnfVar( p, iObj0 );
    iVar1 = Cec3_ObjGetCnfVar( p, iObj1 );
    // collect inputs and internal nodes
    Vec_IntClear( p->vNodesNew );
    Vec_IntClear( p->vSatVars );
    Vec_IntClear( p->vObjSatPairs );
    Gia_ManIncrementTravId( p->pNew );
    Cec3_ManCollect_rec( p, iObj0 );
    Cec3_ManCollect_rec( p, iObj1 );
//printf( "%d ", Vec_IntSize(p->vNodesNew) );
    // solve direct
    if ( p->pPars->fUseCones )  satoko_mark_cone( p->pSat, Vec_IntArray(p->vSatVars), Vec_IntSize(p->vSatVars) );
//    satoko_assump_push( p->pSat, Abc_Var2Lit(iVar0, 1) );
//    satoko_assump_push( p->pSat, Abc_Var2Lit(iVar1, fPhase) );
//    status = satoko_solve( p->pSat );
//    satoko_assump_pop( p->pSat );
//    satoko_assump_pop( p->pSat );
    Lits[0] = Abc_Var2Lit(iVar0, 1);
    Lits[1] = Abc_Var2Lit(iVar1, fPhase);
    bmcg_sat_solver_set_conflict_budget( p->pSat, p->pPars->nConfLimit );
    status = bmcg_sat_solver_solve( p->pSat, Lits, 2 );
    if ( status == GLUCOSE_UNSAT && iObj0 > 0 )
    {
        // solve reverse
//        satoko_assump_push( p->pSat, Abc_Var2Lit(iVar0, 0) );
//        satoko_assump_push( p->pSat, Abc_Var2Lit(iVar1, !fPhase) );
//        status = satoko_solve( p->pSat );
//        satoko_assump_pop( p->pSat );
//        satoko_assump_pop( p->pSat );
        Lits[0] = Abc_Var2Lit(iVar0, 0);
        Lits[1] = Abc_Var2Lit(iVar1, !fPhase);
        bmcg_sat_solver_set_conflict_budget( p->pSat, p->pPars->nConfLimit );
        status = bmcg_sat_solver_solve( p->pSat, Lits, 2 );
    }
    if ( p->pPars->fUseCones )  satoko_unmark_cone( p->pSat, Vec_IntArray(p->vSatVars), Vec_IntSize(p->vSatVars) );
    //if ( status == SATOKO_SAT )
    //    Cec3_ManVerify( p->pNew, iObj0, iObj1, fPhase, p->pSat );
    if ( p->pPars->fUseCones )
        return status;
    Gia_ManForEachObjVec( p->vNodesNew, p->pNew, pObj, i )
        Cec3_ObjCleanSatId( p->pNew, pObj );
    return status;
}

int Cec3_ManSweepNode( Cec3_Man_t * p, int iObj )
{
    abctime clk = Abc_Clock();
    int i, IdAig, IdSat, status, RetValue = 1;
    Gia_Obj_t * pObj = Gia_ManObj( p->pAig, iObj );
    Gia_Obj_t * pRepr = Gia_ObjReprObj( p->pAig, iObj );
    int fCompl = Abc_LitIsCompl(pObj->Value) ^ Abc_LitIsCompl(pRepr->Value) ^ pObj->fPhase ^ pRepr->fPhase;
    status = Cec3_ManSolveTwo( p, Abc_Lit2Var(pRepr->Value), Abc_Lit2Var(pObj->Value), fCompl );
    if ( status == GLUCOSE_SAT )
    {
        p->nSatSat++;
        p->nPatterns++;
        p->pAig->iPatsPi = (p->pAig->iPatsPi == 64 * p->pAig->nSimWords - 1) ? 1 : p->pAig->iPatsPi + 1;
        assert( p->pAig->iPatsPi > 0 && p->pAig->iPatsPi < 64 * p->pAig->nSimWords );
        Vec_IntForEachEntryDouble( p->vObjSatPairs, IdAig, IdSat, i )
//            Cec3_ObjSimSetInputBit( p->pAig, IdAig, satoko_var_polarity(p->pSat, IdSat) == SATOKO_LIT_TRUE );
            Cec3_ObjSimSetInputBit( p->pAig, IdAig, bmcg_sat_solver_read_cex_varvalue(p->pSat, IdSat) );
        p->timeSatSat += Abc_Clock() - clk;
        RetValue = 0;
    }
    else if ( status == GLUCOSE_UNSAT )
    {
        p->nSatUnsat++;
        pObj->Value = Abc_LitNotCond( pRepr->Value, fCompl );
        Gia_ObjSetProved( p->pAig, iObj );
        p->timeSatUnsat += Abc_Clock() - clk;
        RetValue = 1;
    }
    else 
    {
        p->nSatUndec++;
        assert( status == GLUCOSE_UNDEC );
        Gia_ObjSetFailed( p->pAig, iObj );
        p->timeSatUndec += Abc_Clock() - clk;
        RetValue = 2;
    }
    if ( p->pPars->fUseCones )
        return RetValue;
    clk = Abc_Clock();
    bmcg_sat_solver_reset( p->pSat );
    p->timeExtra += Abc_Clock() - clk;
//    satoko_stats(p->pSat)->n_conflicts = 0;
    return RetValue;
}
void Cec3_ManPrintStats( Gia_Man_t * p, Cec3_Par_t * pPars, Cec3_Man_t * pMan )
{
    if ( !pPars->fVerbose )
        return;
    printf( "S =%5d ", pMan ? pMan->nSatSat   : 0 );
    printf( "U =%5d ", pMan ? pMan->nSatUnsat : 0 );
    printf( "F =%5d ", pMan ? pMan->nSatUndec : 0 );
    Gia_ManEquivPrintClasses( p, pPars->fVeryVerbose, 0 );
}
int Cec3_ManPerformSweeping( Gia_Man_t * p, Cec3_Par_t * pPars, Gia_Man_t ** ppNew )
{
    Cec3_Man_t * pMan = Cec3_ManCreate( p, pPars ); 
    Gia_Obj_t * pObj, * pRepr, * pObjNew; 
    int i, Iter, fDisproved = 1;

    // check if any output trivially fails under all-0 pattern
    Gia_ManRandomW( 1 );
    Gia_ManSetPhase( p );
    if ( pPars->fIsMiter ) 
    {
        Gia_ManForEachCo( p, pObj, i )
            if ( pObj->fPhase )
            {
                p->pCexSeq = Cec3_ManDeriveCex( p, i, -1 );
                goto finalize;
            }
    }

    // simulate one round and create classes
    Cec3_ManSimAlloc( p, pPars->nSimWords );
    Cec3_ManSimulateCis( p );
    Cec3_ManSimulate( p, NULL, pMan );
    if ( pPars->fIsMiter && !Cec3_ManSimulateCos(p) ) // cex detected
        goto finalize;
    Cec3_ManCreateClasses( p, pMan );
    Cec3_ManPrintStats( p, pPars, pMan );

    // perform additinal simulation
    for ( i = 0; i < pPars->nSimRounds; i++ )
    {
        Cec3_ManSimulateCis( p );
        Cec3_ManSimulate( p, NULL, pMan );
        if ( pPars->fIsMiter && !Cec3_ManSimulateCos(p) ) // cex detected
            goto finalize;
        Cec3_ManPrintStats( p, pPars, pMan );
    }
    // perform sweeping
    //pMan = Cec3_ManCreate( p, pPars );
    for ( Iter = 0; fDisproved && Iter < pPars->nItersMax; Iter++ )
    {
        fDisproved = 0;
        pMan->nPatterns = 0;
        Cec3_ManSimulateCis( p );
        Vec_IntClear( pMan->vCexTriples );
        Gia_ManForEachAnd( p, pObj, i )
        {
            if ( ~pObj->Value || Gia_ObjFailed(p, i) ) // skip swept nodes and failed nodes
                continue;
            if ( !~Gia_ObjFanin0(pObj)->Value || !~Gia_ObjFanin1(pObj)->Value ) // skip fanouts of non-swept nodes
                continue;
            assert( !Gia_ObjProved(p, i) && !Gia_ObjFailed(p, i) );
            // duplicate the node
            pObj->Value = Gia_ManHashAnd( pMan->pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
            if ( Vec_IntSize(&pMan->pNew->vCopies2) == Abc_Lit2Var(pObj->Value) )
            {
                pObjNew = Gia_ManObj( pMan->pNew, Abc_Lit2Var(pObj->Value) );
                pObjNew->fMark0 = Gia_ObjIsMuxType( pObjNew );
                Gia_ObjSetPhase( pMan->pNew, pObjNew );
                Vec_IntPush( &pMan->pNew->vCopies2, -1 );
            }
            assert( Vec_IntSize(&pMan->pNew->vCopies2) == Gia_ManObjNum(pMan->pNew) );
            pRepr = Gia_ObjReprObj( p, i );
            if ( pRepr == NULL || !~pRepr->Value )
                continue;
            if ( Abc_Lit2Var(pObj->Value) == Abc_Lit2Var(pRepr->Value) )
            {
                assert( (pObj->Value ^ pRepr->Value) == (pObj->fPhase ^ pRepr->fPhase) );
                Gia_ObjSetProved( p, i );
                continue;
            }
            if ( Cec3_ManSweepNode(pMan, i) )
            {
                if ( Gia_ObjProved(p, i) )
                    pObj->Value = Abc_LitNotCond( pRepr->Value, pObj->fPhase ^ pRepr->fPhase );
                continue;
            }
            pObj->Value = ~0;
            Vec_IntPushThree( pMan->vCexTriples, Gia_ObjId(p, pRepr), i, Abc_Var2Lit(p->iPatsPi, pObj->fPhase ^ pRepr->fPhase) );
            fDisproved = 1;
        }
        if ( fDisproved )
        {
            int Fails = Cec3_ManSimulate( p, pMan->vCexTriples, pMan );
            if ( Fails && pPars->fVerbose )
                printf( "Failed to resimulate %d times with pattern = %d  (total = %d).\n", Fails, pMan->nPatterns, pPars->nSimWords * 64 );
            if ( pPars->fIsMiter && !Cec3_ManSimulateCos(p) ) // cex detected
                break;
        }
        Cec3_ManPrintStats( p, pPars, pMan );
    }
    // finish the AIG, if it is not finished
    if ( ppNew )
    {
        Gia_ManForEachAnd( p, pObj, i )
            if ( !~pObj->Value ) 
                pObj->Value = Gia_ManHashAnd( pMan->pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        Gia_ManForEachCo( p, pObj, i )
            pObj->Value = Gia_ManAppendCo( pMan->pNew, Gia_ObjFanin0Copy(pObj) );
        *ppNew = Gia_ManCleanup( pMan->pNew );
        (*ppNew)->pName = Abc_UtilStrsav( p->pName );
        (*ppNew)->pSpec = Abc_UtilStrsav( p->pSpec );
    }
finalize:
    Cec3_ManDestroy( pMan );
    //Gia_ManEquivPrintClasses( p, 1, 0 );
    return p->pCexSeq ? 0 : 1;
}
Gia_Man_t * Cec3_ManSimulateTest( Gia_Man_t * p, Cec_ParFra_t * pPars0 )
{
    Gia_Man_t * pNew = NULL;
    //abctime clk = Abc_Clock();
    Cec3_Par_t Pars, * pPars = &Pars;
    Cec3_SetDefaultParams( pPars );
    // set resource limits
//    pPars->nSimWords  = pPars0->nWords;     // simulation words
//    pPars->nSimRounds = pPars0->nRounds;    // simulation rounds
//    pPars->nItersMax  = pPars0->nItersMax;  // max number of iterations
    pPars->nConfLimit = pPars0->nBTLimit;   // conflict limit at a node
    pPars->fUseCones  = pPars0->fUseCones;
    pPars->fVerbose   = pPars0->fVerbose;
//    Gia_ManComputeGiaEquivs( p, 100000, 0 );
//    Gia_ManEquivPrintClasses( p, 1, 0 );
    Cec3_ManPerformSweeping( p, pPars, &pNew );
    //Abc_PrintTime( 1, "SAT sweeping time", Abc_Clock() - clk );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

