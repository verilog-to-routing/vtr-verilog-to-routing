/**CFile****************************************************************

  FileName    [bmcLoad.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [Experiments with CNF loading.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcLoad.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"
#include "sat/bsat/satStore.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Bmc_Load_t_ Bmc_Load_t;  
struct Bmc_Load_t_
{
    Bmc_AndPar_t *   pPars;     // parameters
    Gia_Man_t *      pGia;      // unrolled AIG
    sat_solver *     pSat;      // SAT solvers
    Vec_Int_t *      vSat2Id;   // maps SAT var into its node
//    Vec_Int_t *      vCut;      // cut in terms of GIA IDs
//    Vec_Int_t *      vCnf;      // CNF for the cut
    int              nCallBacks1; 
    int              nCallBacks2; 
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Load CNF for the cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bmc_LoadGetSatVar( Bmc_Load_t * p, int Id )
{
    Gia_Obj_t * pObj = Gia_ManObj( p->pGia, Id );
    if ( pObj->Value == 0 )
    {
        pObj->Value = Vec_IntSize( p->vSat2Id );
        Vec_IntPush( p->vSat2Id, Id );
        sat_solver_setnvars( p->pSat, Vec_IntSize(p->vSat2Id) );
    }
    return pObj->Value;
}
int Bmc_LoadAddCnf( void * pMan, int iLit )
{
    Bmc_Load_t * p = (Bmc_Load_t *)pMan;
    int Lits[3], iVar = Abc_Lit2Var(iLit);
    Gia_Obj_t * pObj = Gia_ManObj( p->pGia, Vec_IntEntry(p->vSat2Id, iVar) );
    p->nCallBacks1++;
    if ( Gia_ObjIsCi(pObj) || Gia_ObjIsConst0(pObj) )
        return 0;
    assert( Gia_ObjIsAnd(pObj) );
    if ( (Abc_LitIsCompl(iLit) ? pObj->fMark1 : pObj->fMark0) )
        return 0;
    Lits[0] = Abc_LitNot(iLit);
    if ( Abc_LitIsCompl(iLit) )
    {
        Lits[1] = Abc_Var2Lit( Bmc_LoadGetSatVar(p, Gia_ObjFaninId0p(p->pGia, pObj)), !Gia_ObjFaninC0(pObj) );
        Lits[2] = Abc_Var2Lit( Bmc_LoadGetSatVar(p, Gia_ObjFaninId1p(p->pGia, pObj)), !Gia_ObjFaninC1(pObj) );
        sat_solver_clause_new( p->pSat, Lits, Lits + 3, 0 );
        pObj->fMark1 = 1;
    }
    else
    {
        Lits[1] = Abc_Var2Lit( Bmc_LoadGetSatVar(p, Gia_ObjFaninId0p(p->pGia, pObj)), Gia_ObjFaninC0(pObj) );
        sat_solver_clause_new( p->pSat, Lits, Lits + 2, 0 );
        Lits[1] = Abc_Var2Lit( Bmc_LoadGetSatVar(p, Gia_ObjFaninId1p(p->pGia, pObj)), Gia_ObjFaninC1(pObj) );
        sat_solver_clause_new( p->pSat, Lits, Lits + 2, 0 );
        pObj->fMark0 = 1;
    }
    p->nCallBacks2++;
    return 1;
}
int Bmc_LoadAddCnf_rec( Bmc_Load_t * p, int Id )
{
    int iVar = Bmc_LoadGetSatVar( p, Id );
    Gia_Obj_t * pObj = Gia_ManObj( p->pGia, Id );
    if ( Gia_ObjIsAnd(pObj) && !(pObj->fMark0 && pObj->fMark1) )
    {
        Bmc_LoadAddCnf( p, Abc_Var2Lit(iVar, 0) );
        Bmc_LoadAddCnf( p, Abc_Var2Lit(iVar, 1) );
        Bmc_LoadAddCnf_rec( p, Gia_ObjFaninId0(pObj, Id) );
        Bmc_LoadAddCnf_rec( p, Gia_ObjFaninId1(pObj, Id) );
    }
    return iVar;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bmc_Load_t * Bmc_LoadStart( Gia_Man_t * pGia )
{
    Bmc_Load_t * p;
    int Lit;
    Gia_ManSetPhase( pGia );
    Gia_ManCleanValue( pGia );
    Gia_ManCreateRefs( pGia );
    p = ABC_CALLOC( Bmc_Load_t, 1 );
    p->pGia      = pGia;
    p->pSat      = sat_solver_new();
    p->vSat2Id   = Vec_IntAlloc( 1000 );
    Vec_IntPush( p->vSat2Id, 0 );
    // create constant node
    Lit = Abc_Var2Lit( Bmc_LoadGetSatVar(p, 0), 1 );
    sat_solver_addclause( p->pSat, &Lit, &Lit + 1 );
    return p;
}
void Bmc_LoadStop( Bmc_Load_t * p )
{
    Vec_IntFree( p->vSat2Id );
    sat_solver_delete( p->pSat );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bmc_LoadTest( Gia_Man_t * pGia, int fLoadCnf, int fVerbose )
{
    int nConfLimit = 0;
    Bmc_Load_t * p;
    Gia_Obj_t * pObj;
    int i, status, Lit;
    abctime clk = Abc_Clock();
    // create the loading manager
    p = Bmc_LoadStart( pGia );
    // add callback for CNF loading
    if ( fLoadCnf )
    {
        p->pSat->pCnfMan  = p;
        p->pSat->pCnfFunc = Bmc_LoadAddCnf;
    }
    // solve SAT problem for each PO
    Gia_ManForEachPo( pGia, pObj, i )
    {
        if ( fLoadCnf )
            Lit = Abc_Var2Lit( Bmc_LoadGetSatVar(p, Gia_ObjFaninId0p(pGia, pObj)), Gia_ObjFaninC0(pObj) );
        else
            Lit = Abc_Var2Lit( Bmc_LoadAddCnf_rec(p, Gia_ObjFaninId0p(pGia, pObj)), Gia_ObjFaninC0(pObj) );
        if ( fVerbose )
        {
            printf( "Frame%4d :  ", i );
            printf( "Vars = %6d  ", Vec_IntSize(p->vSat2Id) );
            printf( "Clas = %6d  ", sat_solver_nclauses(p->pSat) );
        }
        status = sat_solver_solve( p->pSat, &Lit, &Lit + 1, (ABC_INT64_T)nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
        if ( fVerbose )
        {
            printf( "Conf = %6d  ", sat_solver_nconflicts(p->pSat) );
            if ( status == l_False )
                printf( "UNSAT  " );
            else if ( status == l_True )
                printf( "SAT    " );
            else // if ( status == l_Undec )
                printf( "UNDEC  " );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        }
    }
    printf( "Callbacks = %d.  Loadings = %d.\n", p->nCallBacks1, p->nCallBacks2 );
    Bmc_LoadStop( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

