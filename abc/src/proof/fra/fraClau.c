/**CFile****************************************************************

  FileName    [fraClau.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    [Induction with clause strengthening.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraClau.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fra.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver.h"

ABC_NAMESPACE_IMPL_START


/*
    This code is inspired by the paper: Aaron Bradley and Zohar Manna, 
    "Checking safety by inductive generalization of counterexamples to 
    induction", FMCAD '07.
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Cla_Man_t_    Cla_Man_t;
struct Cla_Man_t_
{
    // SAT solvers
    sat_solver *     pSatMain;
    sat_solver *     pSatTest;
    sat_solver *     pSatBmc;
    // CNF for the test solver
//    Cnf_Dat_t *      pCnfTest;
    // SAT variables
    Vec_Int_t *      vSatVarsMainCs;
    Vec_Int_t *      vSatVarsTestCs;
    Vec_Int_t *      vSatVarsTestNs;
    Vec_Int_t *      vSatVarsBmcNs;
    // helper variables
    int              nSatVarsTestBeg;
    int              nSatVarsTestCur;
    // counter-examples
    Vec_Int_t *      vCexMain0;
    Vec_Int_t *      vCexMain;
    Vec_Int_t *      vCexTest;
    Vec_Int_t *      vCexBase;
    Vec_Int_t *      vCexAssm;
    Vec_Int_t *      vCexBmc;
    // mapping of CS into NS var numbers
    int *            pMapCsMainToCsTest; 
    int *            pMapCsTestToCsMain; 
    int *            pMapCsTestToNsTest; 
    int *            pMapCsTestToNsBmc;  
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Saves variables corresponding to latch outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Fra_ClauSaveLatchVars( Aig_Man_t * pMan, Cnf_Dat_t * pCnf, int fCsVars )
{
    Vec_Int_t * vVars;
    Aig_Obj_t * pObjLo, * pObjLi;
    int i;
    vVars = Vec_IntAlloc( Aig_ManRegNum(pMan) );
    Aig_ManForEachLiLoSeq( pMan, pObjLi, pObjLo, i )
        Vec_IntPush( vVars, pCnf->pVarNums[fCsVars? pObjLo->Id : pObjLi->Id] );
    return vVars;
}

/**Function*************************************************************

  Synopsis    [Saves variables corresponding to latch outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Fra_ClauSaveOutputVars( Aig_Man_t * pMan, Cnf_Dat_t * pCnf )
{
    Vec_Int_t * vVars;
    Aig_Obj_t * pObj;
    int i;
    vVars = Vec_IntAlloc( Aig_ManCoNum(pMan) );
    Aig_ManForEachCo( pMan, pObj, i )
        Vec_IntPush( vVars, pCnf->pVarNums[pObj->Id] );
    return vVars;
}

/**Function*************************************************************

  Synopsis    [Saves variables corresponding to latch outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Fra_ClauSaveInputVars( Aig_Man_t * pMan, Cnf_Dat_t * pCnf, int nStarting )
{
    Vec_Int_t * vVars;
    Aig_Obj_t * pObj;
    int i;
    vVars = Vec_IntAlloc( Aig_ManCiNum(pMan) - nStarting );
    Aig_ManForEachCi( pMan, pObj, i )
    {
        if ( i < nStarting )
            continue;
        Vec_IntPush( vVars, pCnf->pVarNums[pObj->Id] );
    }
    return vVars;
}

/**Function*************************************************************

  Synopsis    [Saves variables corresponding to latch outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Fra_ClauCreateMapping( Vec_Int_t * vSatVarsFrom, Vec_Int_t * vSatVarsTo, int nVarsMax )
{
    int * pMapping, Var, i;
    assert( Vec_IntSize(vSatVarsFrom) == Vec_IntSize(vSatVarsTo) );
    pMapping = ABC_ALLOC( int, nVarsMax );
    for ( i = 0; i < nVarsMax; i++ )
        pMapping[i] = -1;
    Vec_IntForEachEntry( vSatVarsFrom, Var, i )
        pMapping[Var] = Vec_IntEntry(vSatVarsTo,i);
    return pMapping;
}


/**Function*************************************************************

  Synopsis    [Deletes the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClauStop( Cla_Man_t * p )
{
    ABC_FREE( p->pMapCsMainToCsTest );
    ABC_FREE( p->pMapCsTestToCsMain );
    ABC_FREE( p->pMapCsTestToNsTest );
    ABC_FREE( p->pMapCsTestToNsBmc  );
    Vec_IntFree( p->vSatVarsMainCs );
    Vec_IntFree( p->vSatVarsTestCs );
    Vec_IntFree( p->vSatVarsTestNs );
    Vec_IntFree( p->vSatVarsBmcNs );
    Vec_IntFree( p->vCexMain0 );
    Vec_IntFree( p->vCexMain );
    Vec_IntFree( p->vCexTest );
    Vec_IntFree( p->vCexBase );
    Vec_IntFree( p->vCexAssm );
    Vec_IntFree( p->vCexBmc  );
    if ( p->pSatMain ) sat_solver_delete( p->pSatMain );
    if ( p->pSatTest ) sat_solver_delete( p->pSatTest );
    if ( p->pSatBmc )  sat_solver_delete( p->pSatBmc );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Takes the AIG with the single output to be checked.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cla_Man_t * Fra_ClauStart( Aig_Man_t * pMan )
{
    Cla_Man_t * p;
    Cnf_Dat_t * pCnfMain;
    Cnf_Dat_t * pCnfTest;
    Cnf_Dat_t * pCnfBmc;
    Aig_Man_t * pFramesMain;
    Aig_Man_t * pFramesTest;
    Aig_Man_t * pFramesBmc;
    assert( Aig_ManCoNum(pMan) - Aig_ManRegNum(pMan) == 1 );

    // start the manager
    p = ABC_ALLOC( Cla_Man_t, 1 );
    memset( p, 0, sizeof(Cla_Man_t) );
    p->vCexMain0 = Vec_IntAlloc( Aig_ManRegNum(pMan) );
    p->vCexMain  = Vec_IntAlloc( Aig_ManRegNum(pMan) );
    p->vCexTest  = Vec_IntAlloc( Aig_ManRegNum(pMan) );
    p->vCexBase  = Vec_IntAlloc( Aig_ManRegNum(pMan) );
    p->vCexAssm  = Vec_IntAlloc( Aig_ManRegNum(pMan) );
    p->vCexBmc   = Vec_IntAlloc( Aig_ManRegNum(pMan) );

    // derive two timeframes to be checked
    pFramesMain = Aig_ManFrames( pMan, 2, 0, 1, 0, 0, NULL ); // nFrames, fInit, fOuts, fRegs
//Aig_ManShow( pFramesMain, 0, NULL );
    assert( Aig_ManCoNum(pFramesMain) == 2 );
    Aig_ObjChild0Flip( Aig_ManCo(pFramesMain, 0) ); // complement the first output
    pCnfMain = Cnf_DeriveSimple( pFramesMain, 0 );
//Cnf_DataWriteIntoFile( pCnfMain, "temp.cnf", 1 );
    p->pSatMain = (sat_solver *)Cnf_DataWriteIntoSolver( pCnfMain, 1, 0 );
/*
    {
        int i;
        Aig_Obj_t * pObj;
        Aig_ManForEachObj( pFramesMain, pObj, i )
            printf( "%d -> %d  \n", pObj->Id, pCnfMain->pVarNums[pObj->Id] );
        printf( "\n" );
    }
*/

    // derive one timeframe to be checked
    pFramesTest = Aig_ManFrames( pMan, 1, 0, 0, 1, 0, NULL );
    assert( Aig_ManCoNum(pFramesTest) == Aig_ManRegNum(pMan) );
    pCnfTest = Cnf_DeriveSimple( pFramesTest, Aig_ManRegNum(pMan) );
    p->pSatTest = (sat_solver *)Cnf_DataWriteIntoSolver( pCnfTest, 1, 0 );
    p->nSatVarsTestBeg = p->nSatVarsTestCur = sat_solver_nvars( p->pSatTest );

    // derive one timeframe to be checked for BMC
    pFramesBmc = Aig_ManFrames( pMan, 1, 1, 0, 1, 0, NULL );
//Aig_ManShow( pFramesBmc, 0, NULL );
    assert( Aig_ManCoNum(pFramesBmc) == Aig_ManRegNum(pMan) );
    pCnfBmc = Cnf_DeriveSimple( pFramesBmc, Aig_ManRegNum(pMan) );
    p->pSatBmc = (sat_solver *)Cnf_DataWriteIntoSolver( pCnfBmc, 1, 0 );

    // create variable sets
    p->vSatVarsMainCs = Fra_ClauSaveInputVars( pFramesMain, pCnfMain, 2 * (Aig_ManCiNum(pMan)-Aig_ManRegNum(pMan)) );
    p->vSatVarsTestCs = Fra_ClauSaveLatchVars( pFramesTest, pCnfTest, 1 );
    p->vSatVarsTestNs = Fra_ClauSaveLatchVars( pFramesTest, pCnfTest, 0 );
    p->vSatVarsBmcNs  = Fra_ClauSaveOutputVars( pFramesBmc, pCnfBmc );
    assert( Vec_IntSize(p->vSatVarsTestCs) == Vec_IntSize(p->vSatVarsMainCs) );
    assert( Vec_IntSize(p->vSatVarsTestCs) == Vec_IntSize(p->vSatVarsBmcNs) );

    // create mapping of CS into NS vars
    p->pMapCsMainToCsTest = Fra_ClauCreateMapping( p->vSatVarsMainCs, p->vSatVarsTestCs, Aig_ManObjNumMax(pFramesMain) );
    p->pMapCsTestToCsMain = Fra_ClauCreateMapping( p->vSatVarsTestCs, p->vSatVarsMainCs, Aig_ManObjNumMax(pFramesTest) );
    p->pMapCsTestToNsTest = Fra_ClauCreateMapping( p->vSatVarsTestCs, p->vSatVarsTestNs, Aig_ManObjNumMax(pFramesTest) );
    p->pMapCsTestToNsBmc  = Fra_ClauCreateMapping( p->vSatVarsTestCs, p->vSatVarsBmcNs,  Aig_ManObjNumMax(pFramesTest) );

    // cleanup
    Cnf_DataFree( pCnfMain );
    Cnf_DataFree( pCnfTest );
    Cnf_DataFree( pCnfBmc );
    Aig_ManStop( pFramesMain );
    Aig_ManStop( pFramesTest );
    Aig_ManStop( pFramesBmc );
    if ( p->pSatMain == NULL || p->pSatTest == NULL || p->pSatBmc == NULL )
    {
        Fra_ClauStop( p );
        return NULL;
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Splits off second half and returns it as a new vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Vec_Int_t * Vec_IntSplitHalf( Vec_Int_t * vVec )
{
    Vec_Int_t * vPart;
    int Entry, i;
    assert( Vec_IntSize(vVec) > 1 );
    vPart = Vec_IntAlloc( Vec_IntSize(vVec) / 2 + 1 );
    Vec_IntForEachEntryStart( vVec, Entry, i, Vec_IntSize(vVec) / 2 )
        Vec_IntPush( vPart, Entry );
    Vec_IntShrink( vVec, Vec_IntSize(vVec) / 2 );
    return vPart;
}

/**Function*************************************************************

  Synopsis    [Complements all literals in the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Vec_IntComplement( Vec_Int_t * vVec )
{
    int i;
    for ( i = 0; i < Vec_IntSize(vVec); i++ )
        vVec->pArray[i] = lit_neg( vVec->pArray[i] );
}

/**Function*************************************************************

  Synopsis    [Checks if the property holds. Returns counter-example if not.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClauCheckProperty( Cla_Man_t * p, Vec_Int_t * vCex )
{
    int nBTLimit = 0;
    int RetValue, iVar, i;
    sat_solver_act_var_clear( p->pSatMain );
    RetValue = sat_solver_solve( p->pSatMain, NULL, NULL, (ABC_INT64_T)nBTLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    Vec_IntClear( vCex );
    if ( RetValue == l_False )
        return 1;
    assert( RetValue == l_True );
    Vec_IntForEachEntry( p->vSatVarsMainCs, iVar, i )
        Vec_IntPush( vCex, sat_solver_var_literal(p->pSatMain, iVar) );
/*
    {
        int i;
        for (i = 0; i < p->pSatMain->size; i++)
            printf( "%d=%d ", i, p->pSatMain->model.ptr[i] == l_True );
        printf( "\n" );
    }
*/
    return 0;
}

/**Function*************************************************************

  Synopsis    [Checks if the clause holds using BMC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClauCheckBmc( Cla_Man_t * p, Vec_Int_t * vClause )
{
    int nBTLimit = 0;
    int RetValue;
    RetValue = sat_solver_solve( p->pSatBmc, Vec_IntArray(vClause), Vec_IntArray(vClause) + Vec_IntSize(vClause), 
        (ABC_INT64_T)nBTLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    if ( RetValue == l_False )
        return 1;
    assert( RetValue == l_True );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Lifts the clause to depend on NS variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClauRemapClause( int * pMap, Vec_Int_t * vClause, Vec_Int_t * vRemapped, int fInv )
{
    int iLit, i;
    Vec_IntClear( vRemapped );
    Vec_IntForEachEntry( vClause, iLit, i )
    {
        assert( pMap[lit_var(iLit)] >= 0 );
        iLit = toLitCond( pMap[lit_var(iLit)], lit_sign(iLit) ^ fInv );
        Vec_IntPush( vRemapped, iLit );
    }
}

/**Function*************************************************************

  Synopsis    [Checks if the clause holds. Returns counter example if not.]

  Description [Uses test SAT solver.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClauCheckClause( Cla_Man_t * p, Vec_Int_t * vClause, Vec_Int_t * vCex )
{
    int nBTLimit = 0;
    int RetValue, iVar, i;
    // complement literals
    Vec_IntPush( vClause, toLit( p->nSatVarsTestCur++ ) ); // helper positive
    Vec_IntComplement( vClause ); // helper negative (the clause is C v h')
    // add the clause
    RetValue = sat_solver_addclause( p->pSatTest, Vec_IntArray(vClause), Vec_IntArray(vClause) + Vec_IntSize(vClause) );
    assert( RetValue == 1 );
    // complement all literals
    Vec_IntPop( vClause );  // helper removed
    Vec_IntComplement( vClause ); 
    // create the assumption in terms of NS variables
    Fra_ClauRemapClause( p->pMapCsTestToNsTest, vClause, p->vCexAssm, 0 );
    // add helper literals
    for ( i = p->nSatVarsTestBeg; i < p->nSatVarsTestCur - 1; i++ )
        Vec_IntPush( p->vCexAssm, toLitCond(i,1) ); // other helpers negative
    Vec_IntPush( p->vCexAssm, toLitCond(i,0) ); // positive helper
    // try to solve
    RetValue = sat_solver_solve( p->pSatTest, Vec_IntArray(p->vCexAssm), Vec_IntArray(p->vCexAssm) + Vec_IntSize(p->vCexAssm), 
        (ABC_INT64_T)nBTLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    if ( vCex )
        Vec_IntClear( vCex );
    if ( RetValue == l_False )
        return 1;
    assert( RetValue == l_True );
    if ( vCex )
    {
        Vec_IntForEachEntry( p->vSatVarsTestCs, iVar, i )
            Vec_IntPush( vCex, sat_solver_var_literal(p->pSatTest, iVar) );
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Reduces the counter-example by removing complemented literals.]

  Description [Removes literals from vMain that differ from those in the  
  counter-example (vNew). Relies on the fact that the PI variables are
  assigned in the increasing order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClauReduceClause( Vec_Int_t * vMain, Vec_Int_t * vNew )
{
    int LitM, LitN, VarM, VarN, i, j, k;
    assert( Vec_IntSize(vMain) <= Vec_IntSize(vNew) );
    for ( i = j = k = 0; i < Vec_IntSize(vMain) && j < Vec_IntSize(vNew); )
    {
        LitM = Vec_IntEntry( vMain, i );
        LitN = Vec_IntEntry( vNew, j );
        VarM = lit_var( LitM );
        VarN = lit_var( LitN );
        if ( VarM < VarN )
        {
            assert( 0 );
        }
        else if ( VarM > VarN )
        {
            j++;
        }
        else // if ( VarM == VarN )
        {
            i++;
            j++;
            if ( LitM == LitN )
                Vec_IntWriteEntry( vMain, k++, LitM );
        }
    }
    assert( i == Vec_IntSize(vMain) );
    Vec_IntShrink( vMain, k );
}

/**Function*************************************************************

  Synopsis    [Computes the minimal invariant that holds.]

  Description [On entrace, vBasis does not hold, vBasis+vExtra holds but
  is not minimal. On exit, vBasis is unchanged, vBasis+vExtra is minimal.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClauMinimizeClause_rec( Cla_Man_t * p, Vec_Int_t * vBasis, Vec_Int_t * vExtra )
{
    Vec_Int_t * vExtra2;
    int nSizeOld;
    if ( Vec_IntSize(vExtra) == 1 )
        return;
    nSizeOld = Vec_IntSize( vBasis );
    vExtra2 = Vec_IntSplitHalf( vExtra );

    // try the first half
    Vec_IntAppend( vBasis, vExtra );
    if ( Fra_ClauCheckClause( p, vBasis, NULL ) )
    {
        Vec_IntShrink( vBasis, nSizeOld );
        Fra_ClauMinimizeClause_rec( p, vBasis, vExtra );
        return;
    }
    Vec_IntShrink( vBasis, nSizeOld );

    // try the second half
    Vec_IntAppend( vBasis, vExtra2 );
    if ( Fra_ClauCheckClause( p, vBasis, NULL ) )
    {
        Vec_IntShrink( vBasis, nSizeOld );
        Fra_ClauMinimizeClause_rec( p, vBasis, vExtra2 );
        return;
    }
//    Vec_IntShrink( vBasis, nSizeOld );

    // find the smallest with the second half added
    Fra_ClauMinimizeClause_rec( p, vBasis, vExtra );
    Vec_IntShrink( vBasis, nSizeOld );
    Vec_IntAppend( vBasis, vExtra );
    // find the smallest with the second half added
    Fra_ClauMinimizeClause_rec( p, vBasis, vExtra2 );
    Vec_IntShrink( vBasis, nSizeOld );
    Vec_IntAppend( vExtra, vExtra2 );
    Vec_IntFree( vExtra2 );
}

/**Function*************************************************************

  Synopsis    [Minimizes the clauses using a simple method.]

  Description [The input and output clause are in vExtra.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClauMinimizeClause( Cla_Man_t * p, Vec_Int_t * vBasis, Vec_Int_t * vExtra )
{
    int iLit, iLit2, i, k;
    Vec_IntForEachEntryReverse( vExtra, iLit, i )
    {
        // copy literals without the given one
        Vec_IntClear( vBasis );
        Vec_IntForEachEntry( vExtra, iLit2, k )
            if ( k != i )
                Vec_IntPush( vBasis, iLit2 );
        // try whether it is inductive
        if ( !Fra_ClauCheckClause( p, vBasis, NULL ) )
            continue;
        // the clause is inductive
        // remove the literal
        for ( k = i; k < Vec_IntSize(vExtra)-1; k++ )
            Vec_IntWriteEntry( vExtra, k, Vec_IntEntry(vExtra,k+1) );
        Vec_IntShrink( vExtra, Vec_IntSize(vExtra)-1 );
    }
}

/**Function*************************************************************

  Synopsis    [Prints the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClauPrintClause( Vec_Int_t * vSatCsVars, Vec_Int_t * vCex )
{
    int LitM, VarM, VarN, i, j, k;
    assert( Vec_IntSize(vCex) <= Vec_IntSize(vSatCsVars) );
    for ( i = j = k = 0; i < Vec_IntSize(vCex) && j < Vec_IntSize(vSatCsVars); )
    {
        LitM = Vec_IntEntry( vCex, i );
        VarM = lit_var( LitM );
        VarN = Vec_IntEntry( vSatCsVars, j );
        if ( VarM < VarN )
        {
            assert( 0 );
        }
        else if ( VarM > VarN )
        {
            j++;
            printf( "-" );
        }
        else // if ( VarM == VarN )
        {
            i++;
            j++;
            printf( "%d", !lit_sign(LitM) );
        }
    }
    assert( i == Vec_IntSize(vCex) );
}

/**Function*************************************************************

  Synopsis    [Takes the AIG with the single output to be checked.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_Clau( Aig_Man_t * pMan, int nIters, int fVerbose, int fVeryVerbose )
{
    Cla_Man_t * p;
    int Iter, RetValue, fFailed, i;
    assert( Aig_ManCoNum(pMan) - Aig_ManRegNum(pMan) == 1 );
    // create the manager
    p = Fra_ClauStart( pMan );
    if ( p == NULL )
    {
        printf( "The property is trivially inductive.\n" );
        return 1;
    }
    // generate counter-examples and expand them
    for ( Iter = 0; !Fra_ClauCheckProperty( p, p->vCexMain0 ) && Iter < nIters; Iter++ )
    {
        if ( fVerbose )
            printf( "%4d : ", Iter );
        // remap clause into the test manager
        Fra_ClauRemapClause( p->pMapCsMainToCsTest, p->vCexMain0, p->vCexMain, 0 );
        if ( fVerbose && fVeryVerbose )
            Fra_ClauPrintClause( p->vSatVarsTestCs, p->vCexMain );
        // the main counter-example is in p->vCexMain
        // intermediate counter-examples are in p->vCexTest
        // generate the reduced counter-example to the inductive property
        fFailed = 0;
        for ( i = 0; !Fra_ClauCheckClause( p, p->vCexMain, p->vCexTest ); i++ )
        {
            Fra_ClauReduceClause( p->vCexMain, p->vCexTest );
            Fra_ClauRemapClause( p->pMapCsTestToNsBmc, p->vCexMain, p->vCexBmc, 0 );

//            if ( !Fra_ClauCheckBmc(p, p->vCexBmc) )
            if ( Vec_IntSize(p->vCexMain) < 1 )
            {
                Vec_IntComplement( p->vCexMain0 ); 
                RetValue = sat_solver_addclause( p->pSatMain, Vec_IntArray(p->vCexMain0), Vec_IntArray(p->vCexMain0) + Vec_IntSize(p->vCexMain0) );
                if ( RetValue == 0 )
                {
                    printf( "\nProperty is proved after %d iterations.\n", Iter+1 );
                    return 0;
                }
                fFailed = 1;
                break;
            }
        }
        if ( fFailed )
        {
            if ( fVerbose )
                printf( " Reducing failed after %d iterations (BMC failed).\n", i );
            continue;
        }
        if ( Vec_IntSize(p->vCexMain) == 0 )
        {
            if ( fVerbose )
                printf( " Reducing failed after %d iterations (nothing left).\n", i );
            continue;
        }
        if ( fVerbose )
            printf( "  " );
        if ( fVerbose && fVeryVerbose )
            Fra_ClauPrintClause( p->vSatVarsTestCs, p->vCexMain );
        if ( fVerbose )
            printf( " LitsInd = %3d.  ", Vec_IntSize(p->vCexMain) );
        // minimize the inductive property
        Vec_IntClear( p->vCexBase );
        if ( Vec_IntSize(p->vCexMain) > 1 )
//        Fra_ClauMinimizeClause_rec( p, p->vCexBase, p->vCexMain );
            Fra_ClauMinimizeClause( p, p->vCexBase, p->vCexMain );
        assert( Vec_IntSize(p->vCexMain) > 0 );
        if ( fVerbose && fVeryVerbose )
            Fra_ClauPrintClause( p->vSatVarsTestCs, p->vCexMain );
        if ( fVerbose )
            printf( " LitsRed = %3d.  ", Vec_IntSize(p->vCexMain) );
        if ( fVerbose )
            printf( "\n" );
        // add the clause to the solver
        Fra_ClauRemapClause( p->pMapCsTestToCsMain, p->vCexMain, p->vCexAssm, 1 );
        RetValue = sat_solver_addclause( p->pSatMain, Vec_IntArray(p->vCexAssm), Vec_IntArray(p->vCexAssm) + Vec_IntSize(p->vCexAssm) );
        if ( RetValue == 0 )
        {
            Iter++;
            break;
        }
        if ( p->pSatMain->qtail != p->pSatMain->qhead )
        {
            RetValue = sat_solver_simplify(p->pSatMain);
            assert( RetValue != 0 );
            assert( p->pSatMain->qtail == p->pSatMain->qhead );
        }
    }

    // report the results
    if ( Iter == nIters )
    {
        printf( "Property is not proved after %d iterations.\n", nIters );
        return 0;
    }
    printf( "Property is proved after %d iterations.\n", Iter );
    Fra_ClauStop( p );
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

