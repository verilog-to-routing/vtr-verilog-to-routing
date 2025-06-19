/**CFile****************************************************************

  FileName    [giaSatoko.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Interface to Satoko solver.]

  Author      [Alan Mishchenko, Bruno Schmitt]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaSatoko.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "sat/cnf/cnf.h"
#include "misc/extra/extra.h"
#include "sat/satoko/satoko.h"

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
void Gia_ManCollectVars_rec( int Var, Vec_Int_t * vMapping, Vec_Int_t * vRes, Vec_Bit_t * vVisit )
{
    int * pCut, i;
    if ( Vec_BitEntry(vVisit, Var) )
        return;
    Vec_BitWriteEntry(vVisit, Var, 1);
    if ( Vec_IntEntry(vMapping, Var) ) // primary input or constant 0
    {
        pCut = Vec_IntEntryP( vMapping, Vec_IntEntry(vMapping, Var) );
        for ( i = 1; i <= pCut[0]; i++ )
            Gia_ManCollectVars_rec( pCut[i], vMapping, vRes, vVisit );
    }
    Vec_IntPush( vRes, Var );        
}
Vec_Int_t * Gia_ManCollectVars( int Root, Vec_Int_t * vMapping, int nVars )
{
    Vec_Int_t * vRes = Vec_IntAlloc( 100 );
    Vec_Bit_t * vVisit = Vec_BitStart( nVars );
    assert( Vec_IntEntry(vMapping, Root) );
    Gia_ManCollectVars_rec( Root, vMapping, vRes, vVisit );
    Vec_BitFree( vVisit );
    return vRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSatokoReport( int iOutput, int status, abctime clk )
{
    if ( iOutput >= 0 )
        Abc_Print( 1, "Output %6d : ", iOutput );
    else
        Abc_Print( 1, "Total: " );

    if ( status == SATOKO_UNDEC )
        Abc_Print( 1, "UNDECIDED      " );
    else if ( status == SATOKO_SAT )
        Abc_Print( 1, "SATISFIABLE    " );
    else
        Abc_Print( 1, "UNSATISFIABLE  " );

    Abc_PrintTime( 1, "Time", clk );
}
satoko_t * Gia_ManSatokoFromDimacs( char * pFileName, satoko_opts_t * opts )
{
    satoko_t * pSat = satoko_create();
    char * pBuffer = Extra_FileReadContents( pFileName );
    Vec_Int_t * vLits = Vec_IntAlloc( 100 );
    char * pTemp; int fComp, Var, VarMax = 0;
    for ( pTemp = pBuffer; *pTemp; pTemp++ )
    {
        if ( *pTemp == 'c' || *pTemp == 'p' ) 
        {
            while ( *pTemp != '\n' )
                pTemp++;
            continue;
        }
        while ( *pTemp == ' ' || *pTemp == '\t' || *pTemp == '\r' || *pTemp == '\n' )
            pTemp++;
        fComp = 0;
        if ( *pTemp == '-' )
            fComp = 1, pTemp++;
        if ( *pTemp == '+' )
            pTemp++;
        Var = atoi( pTemp );
        if ( Var == 0 ) {
            if ( Vec_IntSize(vLits) > 0 ) 
            {
                satoko_setnvars( pSat, VarMax+1 );
                if ( !satoko_add_clause( pSat, Vec_IntArray(vLits), Vec_IntSize(vLits) ) )
                {
                    satoko_destroy(pSat);
                    Vec_IntFree( vLits );
                    ABC_FREE( pBuffer );
                    return NULL;
                }
                Vec_IntClear( vLits );
            }
        }
        else 
        {
            Var--;
            VarMax = Abc_MaxInt( VarMax, Var );
            Vec_IntPush( vLits, Abc_Var2Lit(Var, fComp) );
        }
        while ( *pTemp >= '0' && *pTemp <= '9' )
            pTemp++;
    }
    ABC_FREE( pBuffer );
    Vec_IntFree( vLits );
    return pSat;
}
void Gia_ManSatokoDimacs( char * pFileName, satoko_opts_t * opts )
{
    abctime clk = Abc_Clock();  
    int status = SATOKO_UNSAT;
    satoko_t * pSat = Gia_ManSatokoFromDimacs( pFileName, opts );
    if ( pSat )
    {
        status = satoko_solve( pSat );
        satoko_destroy( pSat );
    }
    Gia_ManSatokoReport( -1, status, Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
satoko_t * Gia_ManSatokoInit( Cnf_Dat_t * pCnf, satoko_opts_t * opts )
{
    satoko_t * pSat = satoko_create();
    int i;
    //sat_solver_setnvars( pSat, p->nVars );
    for ( i = 0; i < pCnf->nClauses; i++ )
    {
        if ( !satoko_add_clause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1]-pCnf->pClauses[i] ) )
        {
            satoko_destroy( pSat );
            return NULL;
        }
    }
    satoko_configure(pSat, opts);
    return pSat;
}
satoko_t * Gia_ManSatokoCreate( Gia_Man_t * p, satoko_opts_t * opts )
{
    Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p, 8, 0, 1, 0, 0 );
    satoko_t * pSat = Gia_ManSatokoInit( pCnf, opts );
    int status = pSat ? satoko_simplify(pSat) : SATOKO_OK;
    Cnf_DataFree( pCnf );
    if ( status == SATOKO_OK )
        return pSat;
    satoko_destroy( pSat );
    return NULL;
}
int Gia_ManSatokoCallOne( Gia_Man_t * p, satoko_opts_t * opts, int iOutput )
{
    abctime clk = Abc_Clock();
    satoko_t * pSat;
    int status = SATOKO_UNSAT, Cost = 0;
    pSat = Gia_ManSatokoCreate( p, opts );
    if ( pSat )
    {
        status = satoko_solve( pSat );
        Cost = satoko_stats(pSat)->n_conflicts;
        satoko_destroy( pSat );
    }
    Gia_ManSatokoReport( iOutput, status, Abc_Clock() - clk );
    return Cost;
}
void Gia_ManSatokoCall( Gia_Man_t * p, satoko_opts_t * opts, int fSplit, int fIncrem )
{
    int fUseCone = 1;
    Gia_Man_t * pOne;
    Gia_Obj_t * pRoot;
    Vec_Int_t * vCone;
    int i, iLit, status;
    if ( fIncrem )
    {
        abctime clk = Abc_Clock();
        Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p, 8, 0, 0, fUseCone, 0 );
        satoko_t * pSat = Gia_ManSatokoInit( pCnf, opts );
        Gia_ManForEachCo( p, pRoot, i )
        {
            abctime clk = Abc_Clock();
            iLit = Abc_Var2Lit( i+1, 0 );
            satoko_assump_push( pSat, iLit );
            if ( fUseCone )
            {
                vCone = Gia_ManCollectVars( i+1, pCnf->vMapping, pCnf->nVars );
                satoko_mark_cone( pSat, Vec_IntArray(vCone), Vec_IntSize(vCone) );
                printf( "Cone has %6d vars (out of %6d).  ", Vec_IntSize(vCone), pCnf->nVars );
                status = satoko_solve( pSat );
                satoko_unmark_cone( pSat, Vec_IntArray(vCone), Vec_IntSize(vCone) );
                Vec_IntFree( vCone );
            }
            else
            {
                status = satoko_solve( pSat );
            }
            satoko_assump_pop( pSat );
            Gia_ManSatokoReport( i, status, Abc_Clock() - clk );
        }
        Cnf_DataFree( pCnf );
        satoko_destroy( pSat );
        Abc_PrintTime( 1, "Total time", Abc_Clock() - clk );
        return;
    }
    if ( fSplit )
    {
        abctime clk = Abc_Clock();
        Gia_ManForEachCo( p, pRoot, i )
        {
            pOne = Gia_ManDupDfsCone( p, pRoot );
            Gia_ManSatokoCallOne( pOne, opts, i );
            Gia_ManStop( pOne );
        }
        Abc_PrintTime( 1, "Total time", Abc_Clock() - clk );
        return;
    }
    Gia_ManSatokoCallOne( p, opts, -1 );
}    


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

