/**CFile****************************************************************

  FileName    [giaCCof.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Backward reachability using circuit cofactoring.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaCCof.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "sat/bsat/satSolver.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Ccf_Man_t_ Ccf_Man_t; // manager
struct Ccf_Man_t_
{
    // user data
    Gia_Man_t *  pGia;       // single-output AIG manager
    int          nFrameMax;  // maximum number of frames
    int          nConfMax;   // maximum number of conflicts
    int          nTimeMax;   // maximum runtime in seconds
    int          fVerbose;   // verbose flag
    // internal data
    void *       pUnr;       // unrolling manager
    Gia_Man_t *  pFrames;    // unrolled timeframes
    Vec_Int_t *  vCopies;    // copy pointers of the AIG
    sat_solver * pSat;       // SAT solver
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Create manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ccf_Man_t * Ccf_ManStart( Gia_Man_t * pGia, int nFrameMax, int nConfMax, int nTimeMax, int fVerbose )
{
    static Gia_ParFra_t Pars, * pPars = &Pars;
    Ccf_Man_t * p;
    assert( nFrameMax > 0 );
    p = ABC_CALLOC( Ccf_Man_t, 1 );
    p->pGia      = pGia;
    p->nFrameMax = nFrameMax;    
    p->nConfMax  = nConfMax;
    p->nTimeMax  = nTimeMax;
    p->fVerbose  = fVerbose;
    // create unrolling manager
    memset( pPars, 0, sizeof(Gia_ParFra_t) );
    pPars->fVerbose     = fVerbose;
    pPars->nFrames      = nFrameMax;
    pPars->fSaveLastLit = 1;
    p->pUnr = Gia_ManUnrollStart( pGia, pPars );
    p->vCopies = Vec_IntAlloc( 1000 );
    // internal data
    p->pSat = sat_solver_new();
//    sat_solver_setnvars( p->pSat, 10000 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Delete manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ccf_ManStop( Ccf_Man_t * p )
{
    Vec_IntFree( p->vCopies );
    Gia_ManUnrollStop( p->pUnr );
    sat_solver_delete( p->pSat );
    Gia_ManStopP( &p->pFrames );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Extends the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCofExtendSolver( Ccf_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    // add SAT clauses
    for ( i = sat_solver_nvars(p->pSat); i < Gia_ManObjNum(p->pFrames); i++ )
    {
        pObj = Gia_ManObj( p->pFrames, i );
        if ( Gia_ObjIsAnd(pObj) )
            sat_solver_add_and( p->pSat, i, 
                Gia_ObjFaninId0(pObj, i), 
                Gia_ObjFaninId1(pObj, i), 
                Gia_ObjFaninC0(pObj), 
                Gia_ObjFaninC1(pObj), 0 ); 
    }
    sat_solver_setnvars( p->pSat, Gia_ManObjNum(p->pFrames) );
}

static inline int Gia_Obj0Copy( Vec_Int_t * vCopies, int Fan0, int fCompl0 )  
{ return Abc_LitNotCond( Vec_IntEntry(vCopies, Fan0), fCompl0 ); }

static inline int Gia_Obj1Copy( Vec_Int_t * vCopies, int Fan1, int fCompl1 )  
{ return Abc_LitNotCond( Vec_IntEntry(vCopies, Fan1), fCompl1 ); }
 
/**Function*************************************************************

  Synopsis    [Cofactor the circuit w.r.t. the given assignment.]

  Description [Assumes that the solver has just returned SAT.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCofOneDerive_rec( Ccf_Man_t * p, int Id )
{
    Gia_Obj_t * pObj;
    int Res;
    if ( Vec_IntEntry(p->vCopies, Id) != -1 )
        return;
    pObj = Gia_ManObj(p->pFrames, Id);
    assert( Gia_ObjIsCi(pObj) || Gia_ObjIsAnd(pObj) );
    if ( Gia_ObjIsAnd(pObj) )
    {
        int fCompl0 = Gia_ObjFaninC0(pObj);
        int fCompl1 = Gia_ObjFaninC1(pObj);
        int Fan0 = Gia_ObjFaninId0p(p->pFrames, pObj);
        int Fan1 = Gia_ObjFaninId1p(p->pFrames, pObj);
        Gia_ManCofOneDerive_rec( p, Fan0 );
        Gia_ManCofOneDerive_rec( p, Fan1 );
        Res = Gia_ManHashAnd( p->pFrames, 
            Gia_Obj0Copy(p->vCopies, Fan0, fCompl0), 
            Gia_Obj1Copy(p->vCopies, Fan1, fCompl1) );
    }
    else if ( Gia_ObjCioId(pObj) >= Gia_ManRegNum(p->pGia) ) // PI
        Res = sat_solver_var_value( p->pSat, Id );
    else
        Res = Abc_Var2Lit( Id, 0 );
    Vec_IntWriteEntry( p->vCopies, Id, Res );
}

/**Function*************************************************************

  Synopsis    [Cofactor the circuit w.r.t. the given assignment.]

  Description [Assumes that the solver has just returned SAT.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCofOneDerive( Ccf_Man_t * p, int LitProp )
{
    int LitOut;
    // derive the cofactor of the property node
    Vec_IntFill( p->vCopies, Gia_ManObjNum(p->pFrames), -1 );
    Gia_ManCofOneDerive_rec( p, Abc_Lit2Var(LitProp) );
    LitOut = Vec_IntEntry( p->vCopies, Abc_Lit2Var(LitProp) );
    LitOut = Abc_LitNotCond( LitOut, Abc_LitIsCompl(LitProp) );
    // add new PO for the cofactor
    Gia_ManAppendCo( p->pFrames, LitOut );
    // add SAT clauses
    Gia_ManCofExtendSolver( p );
    // return negative literal of the cofactor
    return Abc_LitNot(LitOut);
} 

/**Function*************************************************************

  Synopsis    [Enumerates backward reachable states.]

  Description [Return -1 if resource limit is reached. Returns 1 
  if computation converged (there is no more reachable states).
  Returns 0 if no more states to enumerate.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCofGetReachable( Ccf_Man_t * p, int Lit )
{
    int ObjPrev = 0, ConfPrev = 0;
    int Count = 0, LitOut, RetValue;
    abctime clk;
    // try solving for the first time and quit if converged
    RetValue = sat_solver_solve( p->pSat, &Lit, &Lit + 1, p->nConfMax, 0, 0, 0 );
    if ( RetValue == l_False )
        return 1;
    // iterate circuit cofactoring
    while ( RetValue == l_True )
    {
        clk = Abc_Clock();
        // derive cofactor
        LitOut = Gia_ManCofOneDerive( p, Lit );
        // add the blocking clause
        RetValue = sat_solver_addclause( p->pSat, &LitOut, &LitOut + 1 );
        assert( RetValue );
        // try solving again
        RetValue = sat_solver_solve( p->pSat, &Lit, &Lit + 1, p->nConfMax, 0, 0, 0 );
        // derive cofactors
        if ( p->fVerbose ) 
        {
            printf( "%3d : AIG =%7d  Conf =%7d.  ", Count++, 
                Gia_ManObjNum(p->pFrames) - ObjPrev, sat_solver_nconflicts(p->pSat) - ConfPrev );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
            ObjPrev = Gia_ManObjNum(p->pFrames);
            ConfPrev = sat_solver_nconflicts(p->pSat);
        }
    }
    if ( RetValue == l_Undef )
        return -1;
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManCofTest( Gia_Man_t * pGia, int nFrameMax, int nConfMax, int nTimeMax, int fVerbose )
{ 
    Gia_Man_t * pNew;
    Ccf_Man_t * p;
    Gia_Obj_t * pObj;
    int f, i, Lit, RetValue = -1, fFailed = 0;
    abctime nTimeToStop = Abc_Clock() + nTimeMax * CLOCKS_PER_SEC;
    abctime clk = Abc_Clock();
    assert( Gia_ManPoNum(pGia) == 1 );

    // create reachability manager
    p = Ccf_ManStart( pGia, nFrameMax, nConfMax, nTimeMax, fVerbose );

    // set runtime limit
    if ( nTimeMax )
        sat_solver_set_runtime_limit( p->pSat, nTimeToStop );

    // perform backward image computation
    for ( f = 0; f < nFrameMax; f++ )
    {
        if ( fVerbose ) 
            printf( "ITER %3d :\n", f );
        // add to the mapping of nodes
        p->pFrames = (Gia_Man_t *)Gia_ManUnrollAdd( p->pUnr, f+1 );
        // add SAT clauses
        Gia_ManCofExtendSolver( p );
        // return output literal
        Lit = Gia_ManUnrollLastLit( p->pUnr );
        // derives cofactors of the property literal till all states are blocked
        RetValue = Gia_ManCofGetReachable( p, Lit );
        if ( RetValue )
            break;

        // check the property output
        Gia_ManSetPhase( p->pFrames );
        Gia_ManForEachPo( p->pFrames, pObj, i )
            if ( pObj->fPhase )
            {
                printf( "Property failed in frame %d.\n", f );
                fFailed = 1;
                break;
            }
        if ( i < Gia_ManPoNum(p->pFrames) )
            break;
    }

    // report the result
    if ( nTimeToStop && Abc_Clock() > nTimeToStop )
        printf( "Runtime limit (%d sec) is reached after %d frames.  ", nTimeMax, f );
    else if ( f == nFrameMax )
        printf( "Completed %d frames without converging.  ", f );
    else if ( RetValue == 1 )
        printf( "Backward reachability converged after %d iterations.  ", f-1 );
    else if ( RetValue == -1 )
        printf( "Conflict limit or timeout is reached after %d frames.  ", f-1 );
    Abc_PrintTime( 1, "Runtime", Abc_Clock() - clk );

    if ( !fFailed && RetValue == 1 )
        printf( "Property holds.\n" );
    else if ( !fFailed )
        printf( "Property is undecided.\n" );

    // get the resulting AIG manager
    Gia_ManHashStop( p->pFrames );
    pNew = p->pFrames;  p->pFrames = NULL;
    Ccf_ManStop( p );

    // cleanup
//    if ( fVerbose )
//        Gia_ManPrintStats( pNew, 0 );
    pNew = Gia_ManCleanup( pGia = pNew );
    Gia_ManStop( pGia );
//    if ( fVerbose )
//        Gia_ManPrintStats( pNew, 0 );
    return pNew;   
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

