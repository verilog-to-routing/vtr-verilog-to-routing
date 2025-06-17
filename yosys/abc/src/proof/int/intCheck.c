/**CFile****************************************************************

  FileName    [intCheck.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Interpolation engine.]

  Synopsis    [Procedures to perform incremental inductive check.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 24, 2008.]

  Revision    [$Id: intCheck.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "intInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// checking manager
struct Inter_Check_t_
{
    int           nFramesK;     // the number of timeframes (K=1 for simple induction)
    int           nVars;        // the current number of variables in the solver
    Aig_Man_t *   pFrames;      // unrolled timeframes
    Cnf_Dat_t *   pCnf;         // CNF of unrolled timeframes 
    sat_solver *  pSat;         // SAT solver 
    Vec_Int_t *   vOrLits;      // OR vars in each time frame (total number is the number nFrames)
    Vec_Int_t *   vAndLits;     // AND vars in the last timeframe (total number is the number of interpolants)
    Vec_Int_t *   vAssLits;     // assumptions (the union of the two)
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Create timeframes of the manager for interpolation.]

  Description [The resulting manager is combinational. The primary inputs
  corresponding to register outputs are ordered first.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Inter_ManUnrollFrames( Aig_Man_t * pAig, int nFrames )
{
    Aig_Man_t * pFrames;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i, f;
    assert( Saig_ManRegNum(pAig) > 0 );
    pFrames = Aig_ManStart( Aig_ManNodeNum(pAig) * nFrames );
    // map the constant node
    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pFrames );
    // create variables for register outputs
    Saig_ManForEachLo( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pFrames );
    // add timeframes
    for ( f = 0; f < nFrames; f++ )
    {
        // create PI nodes for this frame
        Saig_ManForEachPi( pAig, pObj, i )
            pObj->pData = Aig_ObjCreateCi( pFrames );
        // add internal nodes of this frame
        Aig_ManForEachNode( pAig, pObj, i )
            pObj->pData = Aig_And( pFrames, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
        // save register inputs
        Saig_ManForEachLi( pAig, pObj, i )
            pObj->pData = Aig_ObjChild0Copy(pObj);
        // transfer to register outputs
        Saig_ManForEachLiLo(  pAig, pObjLi, pObjLo, i )
        {
            pObjLo->pData = pObjLi->pData;
            Aig_ObjCreateCo( pFrames, (Aig_Obj_t *)pObjLo->pData );
        }
    }
    Aig_ManCleanup( pFrames );
    return pFrames;
}

/**Function*************************************************************

  Synopsis    [This procedure sets default values of interpolation parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Inter_Check_t * Inter_CheckStart( Aig_Man_t * pTrans, int nFramesK )
{
    Inter_Check_t * p;
    // create solver
    p = ABC_CALLOC( Inter_Check_t, 1 );
    p->vOrLits  = Vec_IntAlloc( 100 );
    p->vAndLits = Vec_IntAlloc( 100 );
    p->vAssLits = Vec_IntAlloc( 100 );
    // generate the timeframes 
    p->pFrames = Inter_ManUnrollFrames( pTrans, nFramesK );
    assert( Aig_ManCiNum(p->pFrames) == nFramesK * Saig_ManPiNum(pTrans) + Saig_ManRegNum(pTrans) );
    assert( Aig_ManCoNum(p->pFrames) == nFramesK * Saig_ManRegNum(pTrans) );
    // convert to CNF
    p->pCnf = Cnf_Derive( p->pFrames, Aig_ManCoNum(p->pFrames) ); 
    p->pSat = (sat_solver *)Cnf_DataWriteIntoSolver( p->pCnf, 1, 0 );
    // assign parameters
    p->nFramesK = nFramesK;
    p->nVars    = p->pCnf->nVars;
    return p;
}

/**Function*************************************************************

  Synopsis    [This procedure sets default values of interpolation parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Inter_CheckStop( Inter_Check_t * p )
{
    if ( p == NULL )
        return;
    Vec_IntFree( p->vOrLits );
    Vec_IntFree( p->vAndLits );
    Vec_IntFree( p->vAssLits );
    Cnf_DataFree( p->pCnf );
    Aig_ManStop( p->pFrames );
    sat_solver_delete( p->pSat );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Creates one OR-gate: A + B = C.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Inter_CheckAddOrGate( Inter_Check_t * p, int iVarA, int iVarB, int iVarC )
{
    int RetValue, pLits[3];
    // add A => C   or   !A + C
    pLits[0] = toLitCond(iVarA, 1);
    pLits[1] = toLitCond(iVarC, 0);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
    assert( RetValue );
    // add B => C   or   !B + C
    pLits[0] = toLitCond(iVarB, 1);
    pLits[1] = toLitCond(iVarC, 0);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
    assert( RetValue );
    // add !A & !B => !C  or A + B + !C
    pLits[0] = toLitCond(iVarA, 0);
    pLits[1] = toLitCond(iVarB, 0);
    pLits[2] = toLitCond(iVarC, 1);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );
}

/**Function*************************************************************

  Synopsis    [Creates equality: A = B.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Inter_CheckAddEqual( Inter_Check_t * p, int iVarA, int iVarB )
{
    int RetValue, pLits[3];
    // add A => B   or   !A + B
    pLits[0] = toLitCond(iVarA, 1);
    pLits[1] = toLitCond(iVarB, 0);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
    assert( RetValue );
    // add B => A   or   !B + A
    pLits[0] = toLitCond(iVarB, 1);
    pLits[1] = toLitCond(iVarA, 0);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
    assert( RetValue );
}

/**Function*************************************************************

  Synopsis    [Perform the checking.]

  Description [Returns 1 if the check has passed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Inter_CheckPerform( Inter_Check_t * p, Cnf_Dat_t * pCnfInt, abctime nTimeNewOut )
{
    Aig_Obj_t * pObj, * pObj2;
    int i, f, VarA, VarB, RetValue, Entry, status;
    int nRegs = Aig_ManCiNum(pCnfInt->pMan);
    assert( Aig_ManCoNum(p->pCnf->pMan) == p->nFramesK * nRegs );
    assert( Aig_ManCoNum(pCnfInt->pMan) == 1 );

    // set runtime limit
    if ( nTimeNewOut )
        sat_solver_set_runtime_limit( p->pSat, nTimeNewOut );

    // add clauses to the SAT solver
    Cnf_DataLift( pCnfInt, p->nVars );
    for ( f = 0; f <= p->nFramesK; f++ )
    {
        // add clauses to the solver
        for ( i = 0; i < pCnfInt->nClauses; i++ )
        {
            RetValue = sat_solver_addclause( p->pSat, pCnfInt->pClauses[i], pCnfInt->pClauses[i+1] );
            assert( RetValue );
        }
        // add equality clauses for the flop variables
        Aig_ManForEachCi( pCnfInt->pMan, pObj, i )
        {
            pObj2 = f ? Aig_ManCo(p->pFrames, i + (f-1) * nRegs) : Aig_ManCi(p->pFrames, i);
            Inter_CheckAddEqual( p, pCnfInt->pVarNums[pObj->Id], p->pCnf->pVarNums[pObj2->Id] );
        }
        // add final clauses
        if ( f < p->nFramesK )
        {
            if ( f == Vec_IntSize(p->vOrLits) ) // find time here
            {
                // add literal to this frame
                VarB = pCnfInt->pVarNums[ Aig_ManCo(pCnfInt->pMan, 0)->Id ];
                Vec_IntPush( p->vOrLits, VarB );
            }
            else
            {
                // add OR gate for this frame
                VarA = Vec_IntEntry( p->vOrLits, f );
                VarB = pCnfInt->pVarNums[ Aig_ManCo(pCnfInt->pMan, 0)->Id ];
                Inter_CheckAddOrGate( p, VarA, VarB, p->nVars + pCnfInt->nVars );
                Vec_IntWriteEntry( p->vOrLits, f, p->nVars + pCnfInt->nVars ); // using var ID!
            }
        }
        else
        {
            // add AND gate for this frame
            VarB = pCnfInt->pVarNums[ Aig_ManCo(pCnfInt->pMan, 0)->Id ];
            Vec_IntPush( p->vAndLits, VarB );
        }
        // update variable IDs
        Cnf_DataLift( pCnfInt, pCnfInt->nVars + 1 );
        p->nVars += pCnfInt->nVars + 1;
    }
    Cnf_DataLift( pCnfInt, -p->nVars );
    assert( Vec_IntSize(p->vOrLits) == p->nFramesK );

    // collect the assumption literals
    Vec_IntClear( p->vAssLits );
    Vec_IntForEachEntry( p->vOrLits, Entry, i )
        Vec_IntPush( p->vAssLits, toLitCond(Entry, 0) );
    Vec_IntForEachEntry( p->vAndLits, Entry, i )
        Vec_IntPush( p->vAssLits, toLitCond(Entry, 1) );
/*
    if ( pCnfInt->nLiterals == 3635 )
    {
        int s = 0;
    }
*/
    // call the SAT solver
    status = sat_solver_solve( p->pSat, Vec_IntArray(p->vAssLits), 
        Vec_IntArray(p->vAssLits) + Vec_IntSize(p->vAssLits), 
        (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );

    return status == l_False;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

