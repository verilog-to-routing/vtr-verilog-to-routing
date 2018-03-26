/**CFile****************************************************************

  FileName    [fraHot.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    [Computing and using one-hotness conditions.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraHot.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fra.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline int  Fra_RegToLit( int n, int c )   { return c? -n-1 : n+1;       }
static inline int  Fra_LitReg( int n )            { return (n>0)? n-1 : -n-1;   }
static inline int  Fra_LitSign( int n )           { return (n<0);               }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation info is composed of all zeros.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_OneHotNodeIsConst( Fra_Sml_t * pSeq, Aig_Obj_t * pObj )
{
    unsigned * pSims;
    int i;
    pSims = Fra_ObjSim(pSeq, pObj->Id);
    for ( i = pSeq->nWordsPref; i < pSeq->nWordsTotal; i++ )
        if ( pSims[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation infos are equal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_OneHotNodesAreEqual( Fra_Sml_t * pSeq, Aig_Obj_t * pObj0, Aig_Obj_t * pObj1 )
{
    unsigned * pSims0, * pSims1;
    int i;
    pSims0 = Fra_ObjSim(pSeq, pObj0->Id);
    pSims1 = Fra_ObjSim(pSeq, pObj1->Id);
    for ( i = pSeq->nWordsPref; i < pSeq->nWordsTotal; i++ )
        if ( pSims0[i] != pSims1[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if implications holds.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_OneHotNodesAreClause( Fra_Sml_t * pSeq, Aig_Obj_t * pObj1, Aig_Obj_t * pObj2, int fCompl1, int fCompl2 )
{
    unsigned * pSim1, * pSim2;
    int k;
    pSim1 = Fra_ObjSim(pSeq, pObj1->Id);
    pSim2 = Fra_ObjSim(pSeq, pObj2->Id);
    if ( fCompl1 && fCompl2 )
    {
        for ( k = pSeq->nWordsPref; k < pSeq->nWordsTotal; k++ )
            if ( pSim1[k] & pSim2[k] )
                return 0;
    }
    else if ( fCompl1 )
    {
        for ( k = pSeq->nWordsPref; k < pSeq->nWordsTotal; k++ )
            if ( pSim1[k] & ~pSim2[k] )
                return 0;
    }
    else if ( fCompl2 )
    {
        for ( k = pSeq->nWordsPref; k < pSeq->nWordsTotal; k++ )
            if ( ~pSim1[k] & pSim2[k] )
                return 0;
    }
    else
        assert( 0 );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes one-hot implications.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Fra_OneHotCompute( Fra_Man_t * p, Fra_Sml_t * pSim )
{
    int fSkipConstEqu = 1;
    Vec_Int_t * vOneHots;
    Aig_Obj_t * pObj1, * pObj2;
    int i, k;
    int nTruePis = Aig_ManCiNum(pSim->pAig) - Aig_ManRegNum(pSim->pAig);
    assert( pSim->pAig == p->pManAig );
    vOneHots = Vec_IntAlloc( 100 );
    Aig_ManForEachLoSeq( pSim->pAig, pObj1, i )
    {
        if ( fSkipConstEqu && Fra_OneHotNodeIsConst(pSim, pObj1) )
            continue;
        assert( i-nTruePis >= 0 );
//        Aig_ManForEachLoSeq( pSim->pAig, pObj2, k )
//        Vec_PtrForEachEntryStart( Aig_Obj_t *, pSim->pAig->vPis, pObj2, k, Aig_ManCiNum(p)-Aig_ManRegNum(p) )
        Vec_PtrForEachEntryStart( Aig_Obj_t *, pSim->pAig->vCis, pObj2, k, i+1 )
        {
            if ( fSkipConstEqu && Fra_OneHotNodeIsConst(pSim, pObj2) )
                continue;
            if ( fSkipConstEqu && Fra_OneHotNodesAreEqual( pSim, pObj1, pObj2 ) )
                continue;
            assert( k-nTruePis >= 0 );
            if ( Fra_OneHotNodesAreClause( pSim, pObj1, pObj2, 1, 1 ) )
            {
                Vec_IntPush( vOneHots, Fra_RegToLit(i-nTruePis, 1) );
                Vec_IntPush( vOneHots, Fra_RegToLit(k-nTruePis, 1) );
                continue;
            }
            if ( Fra_OneHotNodesAreClause( pSim, pObj1, pObj2, 0, 1 ) )
            {
                Vec_IntPush( vOneHots, Fra_RegToLit(i-nTruePis, 0) );
                Vec_IntPush( vOneHots, Fra_RegToLit(k-nTruePis, 1) );
                continue;
            }
            if ( Fra_OneHotNodesAreClause( pSim, pObj1, pObj2, 1, 0 ) )
            {
                Vec_IntPush( vOneHots, Fra_RegToLit(i-nTruePis, 1) );
                Vec_IntPush( vOneHots, Fra_RegToLit(k-nTruePis, 0) );
                continue;
            }
        }
    }
    return vOneHots;
}

/**Function*************************************************************

  Synopsis    [Assumes one-hot implications in the SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

**********************************************************************/
void Fra_OneHotAssume( Fra_Man_t * p, Vec_Int_t * vOneHots )
{
    Aig_Obj_t * pObj1, * pObj2;
    int i, Out1, Out2, pLits[2];
    int nPiNum = Aig_ManCiNum(p->pManFraig) - Aig_ManRegNum(p->pManFraig);
    assert( p->pPars->nFramesK == 1 ); // add to only one frame
    for ( i = 0; i < Vec_IntSize(vOneHots); i += 2 )
    {
        Out1 = Vec_IntEntry( vOneHots, i );
        Out2 = Vec_IntEntry( vOneHots, i+1 );
        if ( Out1 == 0 && Out2 == 0 )
            continue;
        pObj1 = Aig_ManCi( p->pManFraig, nPiNum + Fra_LitReg(Out1) );
        pObj2 = Aig_ManCi( p->pManFraig, nPiNum + Fra_LitReg(Out2) );
        pLits[0] = toLitCond( Fra_ObjSatNum(pObj1), Fra_LitSign(Out1) );
        pLits[1] = toLitCond( Fra_ObjSatNum(pObj2), Fra_LitSign(Out2) );
        // add constraint to solver
        if ( !sat_solver_addclause( p->pSat, pLits, pLits + 2 ) )
        {
            printf( "Fra_OneHotAssume(): Adding clause makes SAT solver unsat.\n" );
            sat_solver_delete( p->pSat );
            p->pSat = NULL;
            return;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Checks one-hot implications.]

  Description []
               
  SideEffects []

  SeeAlso     []

**********************************************************************/
void Fra_OneHotCheck( Fra_Man_t * p, Vec_Int_t * vOneHots )
{
    Aig_Obj_t * pObj1, * pObj2;
    int RetValue, i, Out1, Out2;
    int nTruePos = Aig_ManCoNum(p->pManFraig) - Aig_ManRegNum(p->pManFraig);
    for ( i = 0; i < Vec_IntSize(vOneHots); i += 2 )
    {
        Out1 = Vec_IntEntry( vOneHots, i );
        Out2 = Vec_IntEntry( vOneHots, i+1 );
        if ( Out1 == 0 && Out2 == 0 )
            continue;
        pObj1 = Aig_ManCo( p->pManFraig, nTruePos + Fra_LitReg(Out1) );
        pObj2 = Aig_ManCo( p->pManFraig, nTruePos + Fra_LitReg(Out2) );
        RetValue = Fra_NodesAreClause( p, pObj1, pObj2, Fra_LitSign(Out1), Fra_LitSign(Out2) );
        if ( RetValue != 1 )
        {
            p->pCla->fRefinement = 1;
            if ( RetValue == 0 )
                Fra_SmlResimulate( p );
            if ( Vec_IntEntry(vOneHots, i) != 0 )
                printf( "Fra_OneHotCheck(): Clause is not refined!\n" );
            assert( Vec_IntEntry(vOneHots, i) == 0 );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Removes those implications that no longer hold.]

  Description [Returns 1 if refinement has happened.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_OneHotRefineUsingCex( Fra_Man_t * p, Vec_Int_t * vOneHots )
{
    Aig_Obj_t * pObj1, * pObj2;
    int i, Out1, Out2, RetValue = 0;
    int nPiNum = Aig_ManCiNum(p->pManAig) - Aig_ManRegNum(p->pManAig);
    assert( p->pSml->pAig == p->pManAig );
    for ( i = 0; i < Vec_IntSize(vOneHots); i += 2 )
    {
        Out1 = Vec_IntEntry( vOneHots, i );
        Out2 = Vec_IntEntry( vOneHots, i+1 );
        if ( Out1 == 0 && Out2 == 0 )
            continue;
        // get the corresponding nodes
        pObj1 = Aig_ManCi( p->pManAig, nPiNum + Fra_LitReg(Out1) );
        pObj2 = Aig_ManCi( p->pManAig, nPiNum + Fra_LitReg(Out2) );
        // check if implication holds using this simulation info
        if ( !Fra_OneHotNodesAreClause( p->pSml, pObj1, pObj2, Fra_LitSign(Out1), Fra_LitSign(Out2) ) )
        {
            Vec_IntWriteEntry( vOneHots, i, 0 );
            Vec_IntWriteEntry( vOneHots, i+1, 0 );
            RetValue = 1;
        }
    }
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Removes those implications that no longer hold.]

  Description [Returns 1 if refinement has happened.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_OneHotCount( Fra_Man_t * p, Vec_Int_t * vOneHots )
{
    int i, Out1, Out2, Counter = 0;
    for ( i = 0; i < Vec_IntSize(vOneHots); i += 2 )
    {
        Out1 = Vec_IntEntry( vOneHots, i );
        Out2 = Vec_IntEntry( vOneHots, i+1 );
        if ( Out1 == 0 && Out2 == 0 )
            continue;
        Counter++;
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Estimates the coverage of state space by clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_OneHotEstimateCoverage( Fra_Man_t * p, Vec_Int_t * vOneHots )
{
    int nSimWords = (1<<14);
    int nRegs = Aig_ManRegNum(p->pManAig);
    Vec_Ptr_t * vSimInfo;
    unsigned * pSim1, * pSim2, * pSimTot;
    int i, w, Out1, Out2, nCovered, Counter = 0;
    abctime clk = Abc_Clock();

    // generate random sim-info at register outputs
    vSimInfo = Vec_PtrAllocSimInfo( nRegs + 1, nSimWords );
//    srand( 0xAABBAABB );
    Aig_ManRandom(1);
    for ( i = 0; i < nRegs; i++ )
    {
        pSim1 = (unsigned *)Vec_PtrEntry( vSimInfo, i );
        for ( w = 0; w < nSimWords; w++ )
            pSim1[w] = Fra_ObjRandomSim();
    }
    pSimTot = (unsigned *)Vec_PtrEntry( vSimInfo, nRegs );

    // collect simulation info
    memset( pSimTot, 0, sizeof(unsigned) * nSimWords );
    for ( i = 0; i < Vec_IntSize(vOneHots); i += 2 )
    {
        Out1 = Vec_IntEntry( vOneHots, i );
        Out2 = Vec_IntEntry( vOneHots, i+1 );
        if ( Out1 == 0 && Out2 == 0 )
            continue;
//printf( "(%c%d,%c%d) ", 
//Fra_LitSign(Out1)? '-': '+', Fra_LitReg(Out1), 
//Fra_LitSign(Out2)? '-': '+', Fra_LitReg(Out2) ); 
        Counter++;
        pSim1 = (unsigned *)Vec_PtrEntry( vSimInfo, Fra_LitReg(Out1) );
        pSim2 = (unsigned *)Vec_PtrEntry( vSimInfo, Fra_LitReg(Out2) );
        if ( Fra_LitSign(Out1) && Fra_LitSign(Out2) )
            for ( w = 0; w < nSimWords; w++ )
                pSimTot[w] |=  pSim1[w] &  pSim2[w];
        else if ( Fra_LitSign(Out1) )
            for ( w = 0; w < nSimWords; w++ )
                pSimTot[w] |=  pSim1[w] & ~pSim2[w];
        else if ( Fra_LitSign(Out2) )
            for ( w = 0; w < nSimWords; w++ )
                pSimTot[w] |= ~pSim1[w] &  pSim2[w];
        else
            assert( 0 );
    }
//printf( "\n" );
    // count the total number of patterns contained in the don't-care
    nCovered = 0;
    for ( w = 0; w < nSimWords; w++ )
        nCovered += Aig_WordCountOnes( pSimTot[w] );
    Vec_PtrFree( vSimInfo );
    // print the result
    printf( "Care states ratio = %f. ", 1.0 * (nSimWords * 32 - nCovered) / (nSimWords * 32) );
    printf( "(%d out of %d patterns)  ", nSimWords * 32 - nCovered, nSimWords * 32 );
    ABC_PRT( "Time", Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    [Creates one-hotness EXDC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Fra_OneHotCreateExdc( Fra_Man_t * p, Vec_Int_t * vOneHots )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj1, * pObj2, * pObj;
    int i, Out1, Out2, nTruePis;
    pNew = Aig_ManStart( Vec_IntSize(vOneHots)/2 );
//    for ( i = 0; i < Aig_ManRegNum(p->pManAig); i++ )
//        Aig_ObjCreateCi(pNew);
    Aig_ManForEachCi( p->pManAig, pObj, i )
        Aig_ObjCreateCi(pNew);
    nTruePis = Aig_ManCiNum(p->pManAig) - Aig_ManRegNum(p->pManAig);
    for ( i = 0; i < Vec_IntSize(vOneHots); i += 2 )
    {
        Out1 = Vec_IntEntry( vOneHots, i );
        Out2 = Vec_IntEntry( vOneHots, i+1 );
        if ( Out1 == 0 && Out2 == 0 )
            continue;
        pObj1 = Aig_ManCi( pNew, nTruePis + Fra_LitReg(Out1) );
        pObj2 = Aig_ManCi( pNew, nTruePis + Fra_LitReg(Out2) );
        pObj1 = Aig_NotCond( pObj1, Fra_LitSign(Out1) );
        pObj2 = Aig_NotCond( pObj2, Fra_LitSign(Out2) );
        pObj  = Aig_Or( pNew, pObj1, pObj2 );
        Aig_ObjCreateCo( pNew, pObj );
    }
    Aig_ManCleanup(pNew);
//    printf( "Created AIG with %d nodes and %d outputs.\n", Aig_ManNodeNum(pNew), Aig_ManCoNum(pNew) );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Assumes one-hot implications in the SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

**********************************************************************/
void Fra_OneHotAddKnownConstraint( Fra_Man_t * p, Vec_Ptr_t * vOnehots )
{
    Vec_Int_t * vGroup;
    Aig_Obj_t * pObj1, * pObj2;
    int k, i, j, Out1, Out2, pLits[2];
    //
    // these constrants should be added to different timeframes!
    // (also note that PIs follow first - then registers)
    //
    Vec_PtrForEachEntry( Vec_Int_t *, vOnehots, vGroup, k )
    {
        Vec_IntForEachEntry( vGroup, Out1, i )
        Vec_IntForEachEntryStart( vGroup, Out2, j, i+1 )
        {
            pObj1 = Aig_ManCi( p->pManFraig, Out1 );
            pObj2 = Aig_ManCi( p->pManFraig, Out2 );
            pLits[0] = toLitCond( Fra_ObjSatNum(pObj1), 1 );
            pLits[1] = toLitCond( Fra_ObjSatNum(pObj2), 1 );
            // add constraint to solver
            if ( !sat_solver_addclause( p->pSat, pLits, pLits + 2 ) )
            {
                printf( "Fra_OneHotAddKnownConstraint(): Adding clause makes SAT solver unsat.\n" );
                sat_solver_delete( p->pSat );
                p->pSat = NULL;
                return;
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

