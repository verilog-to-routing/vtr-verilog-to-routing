/**CFile****************************************************************

  FileName    [fraSat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraSat.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include <math.h>
#include "fra.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Fra_SetActivityFactors( Fra_Man_t * p, Aig_Obj_t * pOld, Aig_Obj_t * pNew );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Runs equivalence test for the two nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_NodesAreEquiv( Fra_Man_t * p, Aig_Obj_t * pOld, Aig_Obj_t * pNew )
{
    int pLits[4], RetValue, RetValue1, nBTLimit;
    abctime clk;//, clk2 = Abc_Clock();
    int status;

    // make sure the nodes are not complemented
    assert( !Aig_IsComplement(pNew) );
    assert( !Aig_IsComplement(pOld) );
    assert( pNew != pOld );

    // if at least one of the nodes is a failed node, perform adjustments:
    // if the backtrack limit is small, simply skip this node
    // if the backtrack limit is > 10, take the quare root of the limit
    nBTLimit = p->pPars->nBTLimitNode;
    if ( !p->pPars->fSpeculate && p->pPars->nFramesK == 0 && (nBTLimit > 0 && (pOld->fMarkB || pNew->fMarkB)) )
    {
        p->nSatFails++;
        // fail immediately
//        return -1;
        if ( nBTLimit <= 10 )
            return -1;
        nBTLimit = (int)pow(nBTLimit, 0.7);
    }

    p->nSatCalls++;
    p->nSatCallsRecent++;

    // make sure the solver is allocated and has enough variables
    if ( p->pSat == NULL )
    {
        p->pSat = sat_solver_new();
        p->nSatVars = 1;
        sat_solver_setnvars( p->pSat, 1000 );
        // var 0 is reserved for const1 node - add the clause
        pLits[0] = toLit( 0 );
        sat_solver_addclause( p->pSat, pLits, pLits + 1 );
    }

    // if the nodes do not have SAT variables, allocate them
    Fra_CnfNodeAddToSolver( p, pOld, pNew );

    if ( p->pSat->qtail != p->pSat->qhead )
    {
        status = sat_solver_simplify(p->pSat);
        assert( status != 0 );
        assert( p->pSat->qtail == p->pSat->qhead );
    }

    // prepare variable activity
    if ( p->pPars->fConeBias )
        Fra_SetActivityFactors( p, pOld, pNew ); 

    // solve under assumptions
    // A = 1; B = 0     OR     A = 1; B = 1 
clk = Abc_Clock();
    pLits[0] = toLitCond( Fra_ObjSatNum(pOld), 0 );
    pLits[1] = toLitCond( Fra_ObjSatNum(pNew), pOld->fPhase == pNew->fPhase );
//Sat_SolverWriteDimacs( p->pSat, "temp.cnf", pLits, pLits + 2, 1 );
    RetValue1 = sat_solver_solve( p->pSat, pLits, pLits + 2, 
        (ABC_INT64_T)nBTLimit, (ABC_INT64_T)0, 
        p->nBTLimitGlobal, p->nInsLimitGlobal );
p->timeSat += Abc_Clock() - clk;
    if ( RetValue1 == l_False )
    {
p->timeSatUnsat += Abc_Clock() - clk;
        pLits[0] = lit_neg( pLits[0] );
        pLits[1] = lit_neg( pLits[1] );
        RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
        assert( RetValue );
        // continue solving the other implication
        p->nSatCallsUnsat++;
    }
    else if ( RetValue1 == l_True )
    {
p->timeSatSat += Abc_Clock() - clk;
        Fra_SmlSavePattern( p );
        p->nSatCallsSat++;
        return 0;
    }
    else // if ( RetValue1 == l_Undef )
    {
p->timeSatFail += Abc_Clock() - clk;
        // mark the node as the failed node
        if ( pOld != p->pManFraig->pConst1 ) 
            pOld->fMarkB = 1;
        pNew->fMarkB = 1;
        p->nSatFailsReal++;
        return -1;
    }

    // if the old node was constant 0, we already know the answer
    if ( pOld == p->pManFraig->pConst1 )
    {
        p->nSatProof++;
        return 1;
    }

    // solve under assumptions
    // A = 0; B = 1     OR     A = 0; B = 0 
clk = Abc_Clock();
    pLits[0] = toLitCond( Fra_ObjSatNum(pOld), 1 );
    pLits[1] = toLitCond( Fra_ObjSatNum(pNew), pOld->fPhase ^ pNew->fPhase );
    RetValue1 = sat_solver_solve( p->pSat, pLits, pLits + 2, 
        (ABC_INT64_T)nBTLimit, (ABC_INT64_T)0, 
        p->nBTLimitGlobal, p->nInsLimitGlobal );
p->timeSat += Abc_Clock() - clk;
    if ( RetValue1 == l_False )
    {
p->timeSatUnsat += Abc_Clock() - clk;
        pLits[0] = lit_neg( pLits[0] );
        pLits[1] = lit_neg( pLits[1] );
        RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
        assert( RetValue );
        p->nSatCallsUnsat++;
    }
    else if ( RetValue1 == l_True )
    {
p->timeSatSat += Abc_Clock() - clk;
        Fra_SmlSavePattern( p );
        p->nSatCallsSat++;
        return 0;
    }
    else // if ( RetValue1 == l_Undef )
    {
p->timeSatFail += Abc_Clock() - clk;
        // mark the node as the failed node
        pOld->fMarkB = 1;
        pNew->fMarkB = 1;
        p->nSatFailsReal++;
        return -1;
    }
/*
    // check BDD proof
    {
        int RetVal;
        ABC_PRT( "Sat", Abc_Clock() - clk2 );
        clk2 = Abc_Clock();
        RetVal = Fra_NodesAreEquivBdd( pOld, pNew );
//        printf( "%d ", RetVal );
        assert( RetVal );
        ABC_PRT( "Bdd", Abc_Clock() - clk2 );
        printf( "\n" );
    }
*/
    // return SAT proof
    p->nSatProof++;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Runs the result of test for pObj => pNew.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_NodesAreImp( Fra_Man_t * p, Aig_Obj_t * pOld, Aig_Obj_t * pNew, int fComplL, int fComplR )
{
    int pLits[4], RetValue, RetValue1, nBTLimit;
    abctime clk;//, clk2 = Abc_Clock();
    int status;

    // make sure the nodes are not complemented
    assert( !Aig_IsComplement(pNew) );
    assert( !Aig_IsComplement(pOld) );
    assert( pNew != pOld );

    // if at least one of the nodes is a failed node, perform adjustments:
    // if the backtrack limit is small, simply skip this node
    // if the backtrack limit is > 10, take the quare root of the limit
    nBTLimit = p->pPars->nBTLimitNode;
/*
    if ( !p->pPars->fSpeculate && p->pPars->nFramesK == 0 && (nBTLimit > 0 && (pOld->fMarkB || pNew->fMarkB)) )
    {
        p->nSatFails++;
        // fail immediately
//        return -1;
        if ( nBTLimit <= 10 )
            return -1;
        nBTLimit = (int)pow(nBTLimit, 0.7);
    }
*/
    p->nSatCalls++;

    // make sure the solver is allocated and has enough variables
    if ( p->pSat == NULL )
    {
        p->pSat = sat_solver_new();
        p->nSatVars = 1;
        sat_solver_setnvars( p->pSat, 1000 );
        // var 0 is reserved for const1 node - add the clause
        pLits[0] = toLit( 0 );
        sat_solver_addclause( p->pSat, pLits, pLits + 1 );
    }

    // if the nodes do not have SAT variables, allocate them
    Fra_CnfNodeAddToSolver( p, pOld, pNew );

    if ( p->pSat->qtail != p->pSat->qhead )
    {
        status = sat_solver_simplify(p->pSat);
        assert( status != 0 );
        assert( p->pSat->qtail == p->pSat->qhead );
    }

    // prepare variable activity
    if ( p->pPars->fConeBias )
        Fra_SetActivityFactors( p, pOld, pNew ); 

    // solve under assumptions
    // A = 1; B = 0     OR     A = 1; B = 1 
clk = Abc_Clock();
//    pLits[0] = toLitCond( Fra_ObjSatNum(pOld), 0 );
//    pLits[1] = toLitCond( Fra_ObjSatNum(pNew), pOld->fPhase == pNew->fPhase );
    pLits[0] = toLitCond( Fra_ObjSatNum(pOld),  fComplL );
    pLits[1] = toLitCond( Fra_ObjSatNum(pNew), !fComplR );
//Sat_SolverWriteDimacs( p->pSat, "temp.cnf", pLits, pLits + 2, 1 );
    RetValue1 = sat_solver_solve( p->pSat, pLits, pLits + 2, 
        (ABC_INT64_T)nBTLimit, (ABC_INT64_T)0, 
        p->nBTLimitGlobal, p->nInsLimitGlobal );
p->timeSat += Abc_Clock() - clk;
    if ( RetValue1 == l_False )
    {
p->timeSatUnsat += Abc_Clock() - clk;
        pLits[0] = lit_neg( pLits[0] );
        pLits[1] = lit_neg( pLits[1] );
        RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
        assert( RetValue );
        // continue solving the other implication
        p->nSatCallsUnsat++;
    }
    else if ( RetValue1 == l_True )
    {
p->timeSatSat += Abc_Clock() - clk;
        Fra_SmlSavePattern( p );
        p->nSatCallsSat++;
        return 0;
    }
    else // if ( RetValue1 == l_Undef )
    {
p->timeSatFail += Abc_Clock() - clk;
        // mark the node as the failed node
        if ( pOld != p->pManFraig->pConst1 ) 
            pOld->fMarkB = 1;
        pNew->fMarkB = 1;
        p->nSatFailsReal++;
        return -1;
    }
    // return SAT proof
    p->nSatProof++;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Runs the result of test for pObj => pNew.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_NodesAreClause( Fra_Man_t * p, Aig_Obj_t * pOld, Aig_Obj_t * pNew, int fComplL, int fComplR )
{
    int pLits[4], RetValue, RetValue1, nBTLimit;
    abctime clk;//, clk2 = Abc_Clock();
    int status;

    // make sure the nodes are not complemented
    assert( !Aig_IsComplement(pNew) );
    assert( !Aig_IsComplement(pOld) );
    assert( pNew != pOld );

    // if at least one of the nodes is a failed node, perform adjustments:
    // if the backtrack limit is small, simply skip this node
    // if the backtrack limit is > 10, take the quare root of the limit
    nBTLimit = p->pPars->nBTLimitNode;
/*
    if ( !p->pPars->fSpeculate && p->pPars->nFramesK == 0 && (nBTLimit > 0 && (pOld->fMarkB || pNew->fMarkB)) )
    {
        p->nSatFails++;
        // fail immediately
//        return -1;
        if ( nBTLimit <= 10 )
            return -1;
        nBTLimit = (int)pow(nBTLimit, 0.7);
    }
*/
    p->nSatCalls++;

    // make sure the solver is allocated and has enough variables
    if ( p->pSat == NULL )
    {
        p->pSat = sat_solver_new();
        p->nSatVars = 1;
        sat_solver_setnvars( p->pSat, 1000 );
        // var 0 is reserved for const1 node - add the clause
        pLits[0] = toLit( 0 );
        sat_solver_addclause( p->pSat, pLits, pLits + 1 );
    }

    // if the nodes do not have SAT variables, allocate them
    Fra_CnfNodeAddToSolver( p, pOld, pNew );

    if ( p->pSat->qtail != p->pSat->qhead )
    {
        status = sat_solver_simplify(p->pSat);
        assert( status != 0 );
        assert( p->pSat->qtail == p->pSat->qhead );
    }

    // prepare variable activity
    if ( p->pPars->fConeBias )
        Fra_SetActivityFactors( p, pOld, pNew ); 

    // solve under assumptions
    // A = 1; B = 0     OR     A = 1; B = 1 
clk = Abc_Clock();
//    pLits[0] = toLitCond( Fra_ObjSatNum(pOld), 0 );
//    pLits[1] = toLitCond( Fra_ObjSatNum(pNew), pOld->fPhase == pNew->fPhase );
    pLits[0] = toLitCond( Fra_ObjSatNum(pOld), !fComplL );
    pLits[1] = toLitCond( Fra_ObjSatNum(pNew), !fComplR );
//Sat_SolverWriteDimacs( p->pSat, "temp.cnf", pLits, pLits + 2, 1 );
    RetValue1 = sat_solver_solve( p->pSat, pLits, pLits + 2, 
        (ABC_INT64_T)nBTLimit, (ABC_INT64_T)0, 
        p->nBTLimitGlobal, p->nInsLimitGlobal );
p->timeSat += Abc_Clock() - clk;
    if ( RetValue1 == l_False )
    {
p->timeSatUnsat += Abc_Clock() - clk;
        pLits[0] = lit_neg( pLits[0] );
        pLits[1] = lit_neg( pLits[1] );
        RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
        assert( RetValue );
        // continue solving the other implication
        p->nSatCallsUnsat++;
    }
    else if ( RetValue1 == l_True )
    {
p->timeSatSat += Abc_Clock() - clk;
        Fra_SmlSavePattern( p );
        p->nSatCallsSat++;
        return 0;
    }
    else // if ( RetValue1 == l_Undef )
    {
p->timeSatFail += Abc_Clock() - clk;
        // mark the node as the failed node
        if ( pOld != p->pManFraig->pConst1 ) 
            pOld->fMarkB = 1;
        pNew->fMarkB = 1;
        p->nSatFailsReal++;
        return -1;
    }
    // return SAT proof
    p->nSatProof++;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Runs equivalence test for one node.]

  Description [Returns the fraiged node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_NodeIsConst( Fra_Man_t * p, Aig_Obj_t * pNew )
{
    int pLits[2], RetValue1, RetValue;
    abctime clk;

    // make sure the nodes are not complemented
    assert( !Aig_IsComplement(pNew) );
    assert( pNew != p->pManFraig->pConst1 );
    p->nSatCalls++;

    // make sure the solver is allocated and has enough variables
    if ( p->pSat == NULL )
    {
        p->pSat = sat_solver_new();
        p->nSatVars = 1;
        sat_solver_setnvars( p->pSat, 1000 );
        // var 0 is reserved for const1 node - add the clause
        pLits[0] = toLit( 0 );
        sat_solver_addclause( p->pSat, pLits, pLits + 1 );
    }

    // if the nodes do not have SAT variables, allocate them
    Fra_CnfNodeAddToSolver( p, NULL, pNew );

    // prepare variable activity
    if ( p->pPars->fConeBias )
        Fra_SetActivityFactors( p, NULL, pNew ); 

    // solve under assumptions
clk = Abc_Clock();
    pLits[0] = toLitCond( Fra_ObjSatNum(pNew), pNew->fPhase );
    RetValue1 = sat_solver_solve( p->pSat, pLits, pLits + 1, 
        (ABC_INT64_T)p->pPars->nBTLimitMiter, (ABC_INT64_T)0, 
        p->nBTLimitGlobal, p->nInsLimitGlobal );
p->timeSat += Abc_Clock() - clk;
    if ( RetValue1 == l_False )
    {
p->timeSatUnsat += Abc_Clock() - clk;
        pLits[0] = lit_neg( pLits[0] );
        RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 1 );
        assert( RetValue );
        // continue solving the other implication
        p->nSatCallsUnsat++;
    }
    else if ( RetValue1 == l_True )
    {
p->timeSatSat += Abc_Clock() - clk;
        if ( p->pPatWords )
            Fra_SmlSavePattern( p );
        p->nSatCallsSat++;
        return 0;
    }
    else // if ( RetValue1 == l_Undef )
    {
p->timeSatFail += Abc_Clock() - clk;
        // mark the node as the failed node
        pNew->fMarkB = 1;
        p->nSatFailsReal++;
        return -1;
    }

    // return SAT proof
    p->nSatProof++;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Sets variable activities in the cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_SetActivityFactors_rec( Fra_Man_t * p, Aig_Obj_t * pObj, int LevelMin, int LevelMax )
{
    Vec_Ptr_t * vFanins;
    Aig_Obj_t * pFanin;
    int i, Counter = 0;
    assert( !Aig_IsComplement(pObj) );
    assert( Fra_ObjSatNum(pObj) );
    // skip visited variables
    if ( Aig_ObjIsTravIdCurrent(p->pManFraig, pObj) )
        return 0;
    Aig_ObjSetTravIdCurrent(p->pManFraig, pObj);
    // add the PI to the list
    if ( pObj->Level <= (unsigned)LevelMin || Aig_ObjIsCi(pObj) )
        return 0;
    // set the factor of this variable
    // (LevelMax-LevelMin) / (pObj->Level-LevelMin) = p->pPars->dActConeBumpMax / ThisBump
    if ( p->pSat->factors == NULL )
        p->pSat->factors = ABC_CALLOC( double, p->pSat->cap );
    p->pSat->factors[Fra_ObjSatNum(pObj)] = p->pPars->dActConeBumpMax * (pObj->Level - LevelMin)/(LevelMax - LevelMin);
    veci_push(&p->pSat->act_vars, Fra_ObjSatNum(pObj));
    // explore the fanins
    vFanins = Fra_ObjFaninVec( pObj );
    Vec_PtrForEachEntry( Aig_Obj_t *, vFanins, pFanin, i )
        Counter += Fra_SetActivityFactors_rec( p, Aig_Regular(pFanin), LevelMin, LevelMax );
    return 1 + Counter;
}

/**Function*************************************************************

  Synopsis    [Sets variable activities in the cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_SetActivityFactors( Fra_Man_t * p, Aig_Obj_t * pOld, Aig_Obj_t * pNew )
{
    int LevelMin, LevelMax;
    abctime clk;
    assert( pOld || pNew );
clk = Abc_Clock(); 
    // reset the active variables
    veci_resize(&p->pSat->act_vars, 0);
    // prepare for traversal
    Aig_ManIncrementTravId( p->pManFraig );
    // determine the min and max level to visit
    assert( p->pPars->dActConeRatio > 0 && p->pPars->dActConeRatio < 1 );
    LevelMax = Abc_MaxInt( (pNew ? pNew->Level : 0), (pOld ? pOld->Level : 0) );
    LevelMin = (int)(LevelMax * (1.0 - p->pPars->dActConeRatio));
    // traverse
    if ( pOld && !Aig_ObjIsConst1(pOld) )
        Fra_SetActivityFactors_rec( p, pOld, LevelMin, LevelMax );
    if ( pNew && !Aig_ObjIsConst1(pNew) )
        Fra_SetActivityFactors_rec( p, pNew, LevelMin, LevelMax );
//Fra_PrintActivity( p );
p->timeTrav += Abc_Clock() - clk;
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

