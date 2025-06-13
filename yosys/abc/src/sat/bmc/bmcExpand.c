/**CFile****************************************************************

  FileName    [bmcExpand.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [Expanding cubes against the offset.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcExpand.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satStore.h"
#include "base/abc/abc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// iterator thought the cubes
#define Bmc_SopForEachCube( pSop, nVars, pCube )  for ( pCube = (pSop); *pCube; pCube += (nVars) + 3 )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Expands cubes against the offset given as an AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjExpandCubesTry( Vec_Str_t * vSop, sat_solver * pSat, Vec_Int_t * vVars )
{
    extern int Bmc_CollapseExpandRound( sat_solver * pSat, sat_solver * pSatOn, Vec_Int_t * vLits, Vec_Int_t * vNums, Vec_Int_t * vTemp, int nBTLimit, int fCanon, int fOnOffSetLit );

    char * pCube, * pSop = Vec_StrArray(vSop);
    int nCubes = Abc_SopGetCubeNum(pSop);
    int nVars = Abc_SopGetVarNum(pSop);

    Vec_Int_t * vLits = Vec_IntAlloc( nVars );
    Vec_Int_t * vTemp = Vec_IntAlloc( nVars );

    assert( nVars == Vec_IntSize(vVars) );
    assert( Vec_StrSize(vSop) == nCubes * (nVars + 3) + 1 );
    Bmc_SopForEachCube( pSop, nVars, pCube )
    {
        int k, Entry;
        // collect literals and clean cube
        Vec_IntFill( vLits, nVars, -1 );
        for ( k = 0; k < nVars; k++ )
        {
            if ( pCube[k] == '-' )
                continue;
            Vec_IntWriteEntry( vLits, k, Abc_Var2Lit(Vec_IntEntry(vVars, k), pCube[k] == '0') );
            pCube[k] = '-';
        }
        // expand cube
        Bmc_CollapseExpandRound( pSat, NULL, vLits, NULL, vTemp, 0, 0, -1 );
        // insert literals
        Vec_IntForEachEntry( vLits, Entry, k )
            if ( Entry != -1 )
                pCube[k] = '1' - Abc_LitIsCompl(Entry);
    }

    Vec_IntFree( vLits );
    Vec_IntFree( vTemp );
    return nCubes;
}
int Abc_ObjExpandCubes( Vec_Str_t * vSop, Gia_Man_t * p, int nVars )
{
    extern int Bmc_CollapseIrredundantFull( Vec_Str_t * vSop, int nCubes, int nVars );

    int fReverse = 0;
    Vec_Int_t * vVars = Vec_IntAlloc( nVars );
    Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p, 8, 0, 0, 0, 0 );
    sat_solver * pSat = (sat_solver *)Cnf_DataWriteIntoSolver(pCnf, 1, 0);
    int v, n, iLit, status, nCubesNew, iCiVarBeg = sat_solver_nvars(pSat) - nVars;

    // check that on-set/off-set is sat
    int iOutVar = 1;
    for ( n = 0; n < 2; n++ )
    {
        iLit  = Abc_Var2Lit( iOutVar, n ); // n=0 => F=1   n=1 => F=0
        status = sat_solver_solve( pSat, &iLit, &iLit + 1, 0, 0, 0, 0 );
        if ( status == l_False )
        {
            Vec_StrClear( vSop );
            Vec_StrPrintStr( vSop, n ? " 1\n" : " 0\n" );
            Vec_StrPush( vSop, '\0' );
            return 1;
        }
    }

    // add literals to the solver
    iLit  = Abc_Var2Lit( iOutVar, 1 ); 
    status = sat_solver_addclause( pSat, &iLit, &iLit + 1 );
    assert( status );

    // collect variables
    if ( fReverse )
        for ( v = nVars - 1; v >= 0; v-- )
            Vec_IntPush( vVars, iCiVarBeg + v );
    else
        for ( v = 0; v < nVars; v++ )
            Vec_IntPush( vVars, iCiVarBeg + v );

    nCubesNew = Abc_ObjExpandCubesTry( vSop, pSat, vVars );

    sat_solver_delete( pSat );
    Cnf_DataFree( pCnf );
    Vec_IntFree( vVars );
    if ( nCubesNew > 1 )
        Bmc_CollapseIrredundantFull( vSop, nCubesNew, nVars );
    return 0;
}
void Abc_NtkExpandCubes( Abc_Ntk_t * pNtk, Gia_Man_t * pGia, int fVerbose )
{
    Gia_Man_t * pNew;
    Abc_Obj_t * pObj; int i;
    Vec_Str_t * vSop = Vec_StrAlloc( 1000 );
    assert( Abc_NtkIsSopLogic(pNtk) );
    assert( Abc_NtkCiNum(pNtk) == Gia_ManCiNum(pGia) );
    assert( Abc_NtkCoNum(pNtk) == Gia_ManCoNum(pGia) );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        pObj = Abc_ObjFanin0(pObj);
        if ( !Abc_ObjIsNode(pObj) || Abc_ObjFaninNum(pObj) == 0 )
            continue;
        assert( Abc_ObjFaninNum(pObj) == Gia_ManCiNum(pGia) );

        Vec_StrClear( vSop );
        Vec_StrAppend( vSop, (char *)pObj->pData );
        Vec_StrPush( vSop, '\0' );

        pNew = Gia_ManDupCones( pGia, &i, 1, 0 );
        assert( Gia_ManCiNum(pNew) == Gia_ManCiNum(pGia) );
        if ( Abc_ObjExpandCubes( vSop, pNew, Abc_ObjFaninNum(pObj) ) )
            Vec_IntClear( &pObj->vFanins );
        Gia_ManStop( pNew );

        pObj->pData = Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, Vec_StrArray(vSop) );
    }
    Vec_StrFree( vSop );
    Abc_NtkSortSops( pNtk );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

