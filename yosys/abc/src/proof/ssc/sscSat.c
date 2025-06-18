/**CFile****************************************************************

  FileName    [sscSat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT sweeping under constraints.]

  Synopsis    [SAT procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 29, 2008.]

  Revision    [$Id: sscSat.c,v 1.00 2008/07/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sscInt.h"
#include "sat/cnf/cnf.h"
#include "aig/gia/giaAig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline int Ssc_ObjSatLit( Ssc_Man_t * p, int Lit ) { return Abc_Var2Lit( Ssc_ObjSatVar(p, Abc_Lit2Var(Lit)), Abc_LitIsCompl(Lit) ); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Addes clauses to the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Gia_ManAddClausesMux( Ssc_Man_t * p, Gia_Obj_t * pNode )
{
    Gia_Obj_t * pNodeI, * pNodeT, * pNodeE;
    int pLits[4], LitF, LitI, LitT, LitE, RetValue;
    assert( !Gia_IsComplement( pNode ) );
    assert( Gia_ObjIsMuxType( pNode ) );
    // get nodes (I = if, T = then, E = else)
    pNodeI = Gia_ObjRecognizeMux( pNode, &pNodeT, &pNodeE );
    // get the Litiable numbers
    LitF = Ssc_ObjSatLit( p, Gia_Obj2Lit(p->pFraig,pNode) );
    LitI = Ssc_ObjSatLit( p, Gia_Obj2Lit(p->pFraig,pNodeI) );
    LitT = Ssc_ObjSatLit( p, Gia_Obj2Lit(p->pFraig,pNodeT) );
    LitE = Ssc_ObjSatLit( p, Gia_Obj2Lit(p->pFraig,pNodeE) );

    // f = ITE(i, t, e)

    // i' + t' + f
    // i' + t  + f'
    // i  + e' + f
    // i  + e  + f'

    // create four clauses
    pLits[0] = Abc_LitNotCond(LitI, 1);
    pLits[1] = Abc_LitNotCond(LitT, 1);
    pLits[2] = Abc_LitNotCond(LitF, 0);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );
    pLits[0] = Abc_LitNotCond(LitI, 1);
    pLits[1] = Abc_LitNotCond(LitT, 0);
    pLits[2] = Abc_LitNotCond(LitF, 1);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );
    pLits[0] = Abc_LitNotCond(LitI, 0);
    pLits[1] = Abc_LitNotCond(LitE, 1);
    pLits[2] = Abc_LitNotCond(LitF, 0);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );
    pLits[0] = Abc_LitNotCond(LitI, 0);
    pLits[1] = Abc_LitNotCond(LitE, 0);
    pLits[2] = Abc_LitNotCond(LitF, 1);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );

    // two additional clauses
    // t' & e' -> f'
    // t  & e  -> f

    // t  + e   + f'
    // t' + e'  + f

    if ( LitT == LitE )
    {
//        assert( fCompT == !fCompE );
        return;
    }

    pLits[0] = Abc_LitNotCond(LitT, 0);
    pLits[1] = Abc_LitNotCond(LitE, 0);
    pLits[2] = Abc_LitNotCond(LitF, 1);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );
    pLits[0] = Abc_LitNotCond(LitT, 1);
    pLits[1] = Abc_LitNotCond(LitE, 1);
    pLits[2] = Abc_LitNotCond(LitF, 0);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );
}

/**Function*************************************************************

  Synopsis    [Addes clauses to the solver.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Gia_ManAddClausesSuper( Ssc_Man_t * p, Gia_Obj_t * pNode, Vec_Int_t * vSuper )
{
    int i, RetValue, Lit, LitNode, pLits[2];
    assert( !Gia_IsComplement(pNode) );
    assert( Gia_ObjIsAnd( pNode ) );
    // suppose AND-gate is A & B = C
    // add !A => !C   or   A + !C
    // add !B => !C   or   B + !C
    LitNode = Ssc_ObjSatLit( p, Gia_Obj2Lit(p->pFraig,pNode) );
    Vec_IntForEachEntry( vSuper, Lit, i )
    {
        pLits[0] = Ssc_ObjSatLit( p, Lit );
        pLits[1] = Abc_LitNot( LitNode );
        RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
        assert( RetValue );
        // update literals
        Vec_IntWriteEntry( vSuper, i, Abc_LitNot(pLits[0]) );
    }
    // add A & B => C   or   !A + !B + C
    Vec_IntPush( vSuper, LitNode );
    RetValue = sat_solver_addclause( p->pSat, Vec_IntArray(vSuper), Vec_IntArray(vSuper) + Vec_IntSize(vSuper) );
    assert( RetValue );
    (void) RetValue;
}


/**Function*************************************************************

  Synopsis    [Collects the supergate.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Ssc_ManCollectSuper_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSuper )
{
    // stop at complements, PIs, and MUXes
    if ( Gia_IsComplement(pObj) || Gia_ObjIsCi(pObj) || Gia_ObjIsMuxType(pObj) )
    {
        Vec_IntPushUnique( vSuper, Gia_Obj2Lit(p, pObj) );
        return;
    }
    Ssc_ManCollectSuper_rec( p, Gia_ObjChild0(pObj), vSuper );
    Ssc_ManCollectSuper_rec( p, Gia_ObjChild1(pObj), vSuper );
}
static void Ssc_ManCollectSuper( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSuper )
{
    assert( !Gia_IsComplement(pObj) );
    assert( Gia_ObjIsAnd(pObj) );
    Vec_IntClear( vSuper );
    Ssc_ManCollectSuper_rec( p, Gia_ObjChild0(pObj), vSuper );
    Ssc_ManCollectSuper_rec( p, Gia_ObjChild1(pObj), vSuper );
}

/**Function*************************************************************

  Synopsis    [Updates the solver clause database.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Ssc_ManCnfAddToFrontier( Ssc_Man_t * p, int Id, Vec_Int_t * vFront )
{
    Gia_Obj_t * pObj;
    assert( Id > 0 );
    if ( Ssc_ObjSatVar(p, Id) )
        return;
    pObj = Gia_ManObj( p->pFraig, Id );
    Ssc_ObjSetSatVar( p, Id, p->nSatVars++ );
    sat_solver_setnvars( p->pSat, p->nSatVars + 100 );
    if ( Gia_ObjIsAnd(pObj) )
        Vec_IntPush( vFront, Id );
}
static void Ssc_ManCnfNodeAddToSolver( Ssc_Man_t * p, int NodeId )
{
    Gia_Obj_t * pNode;
    int i, k, Id, Lit;
    abctime clk;
    assert( NodeId > 0 );
    // quit if CNF is ready
    if ( Ssc_ObjSatVar(p, NodeId) )
        return;
clk = Abc_Clock();
    // start the frontier
    Vec_IntClear( p->vFront );
    Ssc_ManCnfAddToFrontier( p, NodeId, p->vFront );
    // explore nodes in the frontier
    Gia_ManForEachObjVec( p->vFront, p->pFraig, pNode, i )
    {
        // create the supergate
        assert( Ssc_ObjSatVar(p, Gia_ObjId(p->pFraig, pNode)) );
        if ( Gia_ObjIsMuxType(pNode) )
        {
            Vec_IntClear( p->vFanins );
            Vec_IntPushUnique( p->vFanins, Gia_ObjFaninId0p( p->pFraig, Gia_ObjFanin0(pNode) ) );
            Vec_IntPushUnique( p->vFanins, Gia_ObjFaninId0p( p->pFraig, Gia_ObjFanin1(pNode) ) );
            Vec_IntPushUnique( p->vFanins, Gia_ObjFaninId1p( p->pFraig, Gia_ObjFanin0(pNode) ) );
            Vec_IntPushUnique( p->vFanins, Gia_ObjFaninId1p( p->pFraig, Gia_ObjFanin1(pNode) ) );
            Vec_IntForEachEntry( p->vFanins, Id, k )
                Ssc_ManCnfAddToFrontier( p, Id, p->vFront );
            Gia_ManAddClausesMux( p, pNode );
        }
        else
        {
            Ssc_ManCollectSuper( p->pFraig, pNode, p->vFanins );
            Vec_IntForEachEntry( p->vFanins, Lit, k )
                Ssc_ManCnfAddToFrontier( p, Abc_Lit2Var(Lit), p->vFront );
            Gia_ManAddClausesSuper( p, pNode, p->vFanins );
        }
        assert( Vec_IntSize(p->vFanins) > 1 );
    }
p->timeCnfGen += Abc_Clock() - clk;
}


/**Function*************************************************************

  Synopsis    [Starts the SAT solver for constraints.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssc_ManStartSolver( Ssc_Man_t * p )
{
    Aig_Man_t * pMan = Gia_ManToAigSimple( p->pFraig );
    Cnf_Dat_t * pDat = Cnf_Derive( pMan, 0 );
    Gia_Obj_t * pObj;
    sat_solver * pSat;
    int i, status;
    assert( p->pSat == NULL && p->vId2Var == NULL );
    assert( Aig_ManObjNumMax(pMan) == Gia_ManObjNum(p->pFraig) );
    Aig_ManStop( pMan );
    // save variable mapping
    p->nSatVarsPivot = p->nSatVars = pDat->nVars;
    p->vId2Var = Vec_IntStart( Gia_ManCandNum(p->pAig) + Gia_ManCandNum(p->pCare) + 10 ); // mapping of each node into its SAT var
    p->vVar2Id = Vec_IntStart( Gia_ManCandNum(p->pAig) + Gia_ManCandNum(p->pCare) + 10 ); // mapping of each SAT var into its node
    Ssc_ObjSetSatVar( p, 0, pDat->pVarNums[0] );
    Gia_ManForEachCi( p->pFraig, pObj, i )
    {
        int iObj = Gia_ObjId( p->pFraig, pObj );
        Ssc_ObjSetSatVar( p, iObj, pDat->pVarNums[iObj] );
    }
//Cnf_DataWriteIntoFile( pDat, "dump.cnf", 1, NULL, NULL );
    // start the SAT solver
    pSat = sat_solver_new();
    sat_solver_setnvars( pSat, pDat->nVars + 1000 );
    for ( i = 0; i < pDat->nClauses; i++ )
    {
        if ( !sat_solver_addclause( pSat, pDat->pClauses[i], pDat->pClauses[i+1] ) )
        {
            Cnf_DataFree( pDat );
            sat_solver_delete( pSat );
            return;
        }
    }
    Cnf_DataFree( pDat );
    status = sat_solver_simplify( pSat );
    if ( status == 0 )
    {
        sat_solver_delete( pSat );
        return;
    }
    p->pSat = pSat;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssc_ManCollectSatPattern( Ssc_Man_t * p, Vec_Int_t * vPattern )
{
    Gia_Obj_t * pObj;
    int i;
    Vec_IntClear( vPattern );
    Gia_ManForEachCi( p->pFraig, pObj, i )
        Vec_IntPush( vPattern, sat_solver_var_value(p->pSat, Ssc_ObjSatVar(p, Gia_ObjId(p->pFraig, pObj))) );
}
Vec_Int_t * Ssc_ManFindPivotSat( Ssc_Man_t * p )
{
    Vec_Int_t * vInit;
    int status = sat_solver_solve( p->pSat, NULL, NULL, p->pPars->nBTLimit, 0, 0, 0 );
    if ( status == l_False )
        return (Vec_Int_t *)(ABC_PTRINT_T)1;
    if ( status == l_Undef )
        return NULL;
    assert( status == l_True );
    vInit = Vec_IntAlloc( Gia_ManCiNum(p->pFraig) );
    Ssc_ManCollectSatPattern( p, vInit );
    return vInit;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssc_ManCheckEquivalence( Ssc_Man_t * p, int iRepr, int iNode, int fCompl )
{
    int pLitsSat[2], RetValue;
    abctime clk;
    assert( iRepr != iNode );
    if ( iRepr > iNode )
        return l_Undef;
    assert( iRepr < iNode );
//    if ( p->nTimeOut )
//        sat_solver_set_runtime_limit( p->pSat, p->nTimeOut * CLOCKS_PER_SEC + Abc_Clock() );

    // create CNF
    if ( iRepr )
    Ssc_ManCnfNodeAddToSolver( p, iRepr );
    Ssc_ManCnfNodeAddToSolver( p, iNode );
    sat_solver_compress( p->pSat );

    // order the literals
    pLitsSat[0] = Abc_Var2Lit( Ssc_ObjSatVar(p, iRepr), 0 );
    pLitsSat[1] = Abc_Var2Lit( Ssc_ObjSatVar(p, iNode), fCompl ^ (int)(iRepr > 0) );

    // solve under assumptions
    // A = 1; B = 0
    clk = Abc_Clock();
    RetValue = sat_solver_solve( p->pSat, pLitsSat, pLitsSat + 2, (ABC_INT64_T)p->pPars->nBTLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    if ( RetValue == l_False )
    {
        pLitsSat[0] = Abc_LitNot( pLitsSat[0] ); // compl
        pLitsSat[1] = Abc_LitNot( pLitsSat[1] ); // compl
        RetValue = sat_solver_addclause( p->pSat, pLitsSat, pLitsSat + 2 );
        assert( RetValue );
        p->timeSatUnsat += Abc_Clock() - clk;
    }
    else if ( RetValue == l_True )
    {
        Ssc_ManCollectSatPattern( p, p->vPattern );
        p->timeSatSat += Abc_Clock() - clk;
        return l_True;
    }
    else // if ( RetValue1 == l_Undef )
    {
        p->timeSatUndec += Abc_Clock() - clk;
        return l_Undef;
    }

    // if the old node was constant 0, we already know the answer
    if ( iRepr == 0 )
        return l_False;

    // solve under assumptions
    // A = 0; B = 1
    clk = Abc_Clock();
    RetValue = sat_solver_solve( p->pSat, pLitsSat, pLitsSat + 2, (ABC_INT64_T)p->pPars->nBTLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    if ( RetValue == l_False )
    {
        pLitsSat[0] = Abc_LitNot( pLitsSat[0] );
        pLitsSat[1] = Abc_LitNot( pLitsSat[1] );
        RetValue = sat_solver_addclause( p->pSat, pLitsSat, pLitsSat + 2 );
        assert( RetValue );
        p->timeSatUnsat += Abc_Clock() - clk;
    }
    else if ( RetValue == l_True )
    {
        Ssc_ManCollectSatPattern( p, p->vPattern );
        p->timeSatSat += Abc_Clock() - clk;
        return l_True;
    }
    else // if ( RetValue1 == l_Undef )
    {
        p->timeSatUndec += Abc_Clock() - clk;
        return l_Undef;
    }
    return l_False;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

