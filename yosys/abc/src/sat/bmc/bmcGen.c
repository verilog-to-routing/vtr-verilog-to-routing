/**CFile****************************************************************

  FileName    [bmcGen.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [Generating satisfying assignments.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcGen.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satStore.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word * Gia_ManMoObj( Gia_Man_t * p, int iObj )
{
    return Vec_WrdEntryP( p->vSims, iObj * p->nSimWords );
}
static inline void Gia_ManMoSetCi( Gia_Man_t * p, int iObj )
{
    int w;
    word * pSims = Gia_ManMoObj( p, iObj );
    for ( w = 0; w < p->nSimWords; w++ )
        pSims[w] = Gia_ManRandomW( 0 );
}
static inline void Gia_ManMoSimAnd( Gia_Man_t * p, int iObj )
{
    int w;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    word * pSims  = Gia_ManMoObj( p, iObj );
    word * pSims0 = Gia_ManMoObj( p, Gia_ObjFaninId0(pObj, iObj) );
    word * pSims1 = Gia_ManMoObj( p, Gia_ObjFaninId1(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) )
    {
        if (  Gia_ObjFaninC1(pObj) )
            for ( w = 0; w < p->nSimWords; w++ )
                pSims[w] = ~(pSims0[w] | pSims1[w]);
        else 
            for ( w = 0; w < p->nSimWords; w++ )
                pSims[w] = ~pSims0[w] & pSims1[w];
    }
    else 
    {
        if (  Gia_ObjFaninC1(pObj) )
            for ( w = 0; w < p->nSimWords; w++ )
                pSims[w] = pSims0[w] & ~pSims1[w];
        else 
            for ( w = 0; w < p->nSimWords; w++ )
                pSims[w] = pSims0[w] & pSims1[w];
    }
}
static inline void Gia_ManMoSetCo( Gia_Man_t * p, int iObj )
{
    int w;
    Gia_Obj_t * pObj  = Gia_ManObj( p, iObj );
    word * pSims  = Gia_ManMoObj( p, iObj );
    word * pSims0 = Gia_ManMoObj( p, Gia_ObjFaninId0(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) )
    {
        for ( w = 0; w < p->nSimWords; w++ )
            pSims[w] = ~pSims0[w];
    }
    else 
    {
        for ( w = 0; w < p->nSimWords; w++ )
            pSims[w] = pSims0[w];
    }
}
void Gia_ManMoFindSimulate( Gia_Man_t * p, int nWords )
{
    int i, iObj;
    Gia_ManRandomW( 1 );
    p->nSimWords = nWords;
    if ( p->vSims )
        Vec_WrdFill( p->vSims, nWords * Gia_ManObjNum(p), 0 );
    else
        p->vSims = Vec_WrdStart( nWords * Gia_ManObjNum(p) );
    Gia_ManForEachCiId( p, iObj, i )
        Gia_ManMoSetCi( p, iObj );
    Gia_ManForEachAndId( p, iObj )
        Gia_ManMoSimAnd( p, iObj );
    Gia_ManForEachCoId( p, iObj, i )
        Gia_ManMoSetCo( p, iObj );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManTestSatEnum( Gia_Man_t * p )
{
    abctime clk = Abc_Clock(), clk2, clkTotal = 0;
    Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p, 8, 0, 0, 0, 0 );
    sat_solver * pSat = (sat_solver *)Cnf_DataWriteIntoSolver(pCnf, 1, 0);
    int i, v, status, iLit, nWords = 1, iOutVar = 1, Count = 0;
    Vec_Int_t * vVars = Vec_IntAlloc( 1000 );
    word * pSimInfo;

    // add literals to the solver
    iLit  = Abc_Var2Lit( iOutVar, 0 ); 
    status = sat_solver_addclause( pSat, &iLit, &iLit + 1 );
    assert( status );

    // simulate the AIG
    Gia_ManMoFindSimulate( p, nWords );

    // print outputs
    pSimInfo = Gia_ManMoObj( p, Gia_ObjId(p, Gia_ManCo(p, 0)) );
    for ( i = 0; i < 64*nWords; i++ )
        printf( "%d", Abc_InfoHasBit( (unsigned *)pSimInfo, i ) );
    printf( "\n" );

    // iterate through the assignments
    for ( i = 0; i < 64*nWords; i++ )
    {
        Vec_IntClear( vVars );
        for ( v = 0; v < Gia_ManObjNum(p); v++ )
        {
            if ( pCnf->pVarNums[v] == -1 )
                continue;
            pSimInfo = Gia_ManMoObj( p, v );
            if ( !Abc_InfoHasBit( (unsigned *)pSimInfo, i ) )
                continue;
            Vec_IntPush( vVars, pCnf->pVarNums[v] );
        }
        //sat_solver_act_var_clear( pSat );
        //sat_solver_set_polarity( pSat, Vec_IntArray(vVars), Vec_IntSize(vVars) );

        clk2 = Abc_Clock();
        status = sat_solver_solve( pSat, NULL, NULL, 0, 0, 0, 0 );
        clkTotal += Abc_Clock() - clk2;
        printf( "%c", status == l_True ? '+' : '-' );
        if ( status == l_True )
            Count++;
    }
    printf( "\n" );
    printf( "Finished generating %d assignments.  ", Count );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    Abc_PrintTime( 1, "SAT solver time", clkTotal );

    Vec_WrdFreeP( &p->vSims );
    Vec_IntFree( vVars );
    sat_solver_delete( pSat );
    Cnf_DataFree( pCnf );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

