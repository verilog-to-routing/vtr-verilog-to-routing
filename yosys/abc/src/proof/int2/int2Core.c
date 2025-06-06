/**CFile****************************************************************

  FileName    [int2Core.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Interpolation engine.]

  Synopsis    [Core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Dec 1, 2013.]

  Revision    [$Id: int2Core.c,v 1.00 2013/12/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "int2Int.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure sets default values of interpolation parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Int2_ManSetDefaultParams( Int2_ManPars_t * p )
{ 
    memset( p, 0, sizeof(Int2_ManPars_t) );
    p->nBTLimit      =  0;     // limit on the number of conflicts
    p->nFramesS      =  1;     // the starting number timeframes
    p->nFramesMax    =  0;     // the max number timeframes to unroll
    p->nSecLimit     =  0;     // time limit in seconds
    p->nFramesK      =  1;     // the number of timeframes to use in induction
    p->fRewrite      =  0;     // use additional rewriting to simplify timeframes
    p->fTransLoop    =  0;     // add transition into the init state under new PI var
    p->fDropInvar    =  0;     // dump inductive invariant into file
    p->fVerbose      =  0;     // print verbose statistics
    p->iFrameMax     = -1;
}
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Int2_ManUnroll( Gia_Man_t * p, int nFrames )
{
    Gia_Man_t * pFrames, * pTemp;
    Gia_Obj_t * pObj;
    int i, f;
    assert( Gia_ManRegNum(pAig) > 0 );
    pFrames = Gia_ManStart( Gia_ManObjNum(pAig) );
    pFrames->pName = Abc_UtilStrsav( pAig->pName );
    pFrames->pSpec = Abc_UtilStrsav( pAig->pSpec );
    Gia_ManHashAlloc( pFrames );
    Gia_ManConst0(pAig)->Value = 0;
    for ( f = 0; f < nFrames; f++ )
    {
        Gia_ManForEachRo( pAig, pObj, i )
            pObj->Value = f ? Gia_ObjRoToRi( pAig, pObj )->Value : 0;
        Gia_ManForEachPi( pAig, pObj, i )
            pObj->Value = Gia_ManAppendCi( pFrames );
        Gia_ManForEachAnd( pAig, pObj, i )
            pObj->Value = Gia_ManHashAnd( pFrames, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        Gia_ManForEachRi( pAig, pObj, i )
            pObj->Value = Gia_ObjFanin0Copy(pObj);
    }
    Gia_ManForEachRi( pAig, pObj, i )
        Gia_ManAppendCo( pFrames, pObj->Value );
    Gia_ManHashStop( pFrames );
    pFrames = Gia_ManCleanup( pTemp = pFrames );
    Gia_ManStop( pTemp );
    return pFrames;
}
sat_solver * Int2_ManPreparePrefix( Gia_Man_t * p, int f, Vec_Int_t ** pvCiMap )
{
    Gia_Man_t * pPref, * pNew;
    sat_solver * pSat;
    // create subset of the timeframe
    pPref = Int2_ManUnroll( p, f );
    // create SAT solver
    pNew = Jf_ManDeriveCnf( pPref, 0 );  
    pCnf = (Cnf_Dat_t *)pPref->pData; pPref->pData = NULL;
    Gia_ManStop( pPref );
    // derive the SAT solver
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    // collect indexes of CO variables
    *pvCiMap = Vec_IntAlloc( 100 );
    Gia_ManForEachPo( pNew, pObj, i )     
        Vec_IntPush( *pvCiMap, pCnf->pVarNums[ Gia_ObjId(pNew, pObj) ] );
    // cleanup
    Cnf_DataFree( pCnf );
    Gia_ManStop( pNew );
    return pSat;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
sat_solver * Int2_ManPrepareSuffix( Gia_Man_t * p, Vec_Int_t * vImageOne, Vec_Int_t * vImagesAll, Vec_Int_t ** pvCoMap, Gia_Man_t ** ppSuff )
{
    Gia_Man_t * pSuff, * pNew;
    Gia_Obj_t * pObj;
    Cnf_Dat_t * pCnf;
    sat_solver * pSat;
    Vec_Int_t * vLits;
    int i, Lit, Limit;
    // create subset of the timeframe
    pSuff = Int2_ManProbToGia( p, vImageOne );
    assert( Gia_ManPiNum(pSuff) == Gia_ManCiNum(p) );
    // create SAT solver
    pNew = Jf_ManDeriveCnf( pSuff, 0 );  
    pCnf = (Cnf_Dat_t *)pSuff->pData; pSuff->pData = NULL;
    Gia_ManStop( pSuff );
    // derive the SAT solver
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    // create new constraints
    vLits = Vec_IntAlloc( 1000 );
    Vec_IntForEachEntryStart( vImagesAll, Limit, i, 1 )
    {
        Vec_IntClear( vLits );
        for ( k = 0; k < Limit; k++ )
        {
            i++;
            Lit = Vec_IntEntry( vSop, i + k );
            Vec_IntPush( vLits, Abc_LitNot(Lit) );
        }
        if ( !sat_solver_addclause( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits) )  )  // UNSAT
        {
            Vec_IntFree( vLits );
            Cnf_DataFree( pCnf );
            Gia_ManStop( pNew );
            *pvCoMap = NULL;
            return NULL;
        }
    }
    Vec_IntFree( vLits );
    // collect indexes of CO variables
    *pvCoMap = Vec_IntAlloc( 100 );
    Gia_ManForEachRo( p, pObj, i )     
    {
        pObj = Gia_ManPi( pNew, i + Gia_ManPiNum(p) );
        Vec_IntPush( *pvCoMap, pCnf->pVarNums[ Gia_ObjId(pNew, pObj) ] );
    }
    // cleanup
    Cnf_DataFree( pCnf );
    if ( ppSuff )
        *ppSuff = pNew;
    else
        Gia_ManStop( pNew );
    return pSat;
}

/**Function*************************************************************

  Synopsis    [Returns the cube cover and status.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Int2_ManComputePreimage( Gia_Man_t * pSuff, sat_solver * pSatPref, sat_solver * pSatSuff, Vec_Int_t * vCiMap, Vec_Int_t * vCoMap, Vec_Int_t * vPrio )
{
    int i, iVar, status;
    Vec_IntClear( p->vImage );
    while ( 1 )
    {
        // run suffix solver
        status = sat_solver_solve( p->pSatSuff, NULL, NULL, 0, 0, 0, 0 );
        if ( status == l_Undef )
            return NULL; // timeout
        if ( status == l_False )
            return INT2_COMPUTED;
        assert( status == l_True );
        // collect assignment
        Vec_IntClear( p->vAssign );
        Vec_IntForEachEntry( p->vCiMap, iVar, i )
            Vec_IntPush( p->vAssign, sat_solver_var_value(p->pSatSuff, iVar) );
        // derive initial cube
        vCube = Int2_ManRefineCube( p->pSuff, p->vAssign, p->vPrio );
        // expend the cube using prefix
        status = sat_solver_solve( p->pSatPref, Vec_IntArray(vCube), Vec_IntArray(vCube) + Vec_IntSize(vCube), 0, 0, 0, 0 );
        if ( status == l_False )
        {
            int k, nCoreLits, * pCoreLits;
            nCoreLits = sat_solver_final( p->pSatPref, &pCoreLits );
            // create cube
            Vec_IntClear( vCube );
            Vec_IntPush( vImage, nCoreLits );
            for ( k = 0; k < nCoreLits; k++ )
            {
                Vec_IntPush( vCube, pCoreLits[k] );
                Vec_IntPush( vImage, pCoreLits[k] );
            }
            // add cube to the solver
            if ( !sat_solver_addclause( p->pSatSuff, Vec_IntArray(vCube), Vec_IntArray(vCube) + Vec_IntSize(vCube) ) )
            {
                Vec_IntFree( vCube );
                return INT2_COMPUTED;
            }
        }
        Vec_IntFree( vCube );
        if ( status == l_Undef )
            return INT2_TIME_OUT;
        if ( status == l_True )
            return INT2_FALSE_NEG;
        assert( status == l_False );
        continue;
    }
    return p->vImage;
}

/**Function*************************************************************

  Synopsis    [Interpolates while the number of conflicts is not exceeded.]

  Description [Returns 1 if proven. 0 if failed. -1 if undecided.]
               
  SideEffects [Does not check the property in 0-th frame.]

  SeeAlso     []

***********************************************************************/
int Int2_ManPerformInterpolation( Gia_Man_t * pInit, Int2_ManPars_t * pPars )
{
    Int2_Man_t * p;
    int f, i, RetValue = -1;
    abctime clk, clkTotal = Abc_Clock(), timeTemp = 0;
    abctime nTimeToStop = pPars->nSecLimit ? pPars->nSecLimit * CLOCKS_PER_SEC + Abc_Clock() : 0;

    // sanity checks
    assert( Gia_ManPiNum(pInit) > 0 );
    assert( Gia_ManPoNum(pInit) > 0 );
    assert( Gia_ManRegNum(pInit) > 0 );

    // create manager
    p = Int2_ManCreate( pInit, pPars );

    // create SAT solver
    p->pSatPref = sat_solver_new();
    sat_solver_setnvars( p->pSatPref, 1000 );
    sat_solver_set_runtime_limit( p->pSatPref, nTimeToStop );

    // check outputs in the first frame
    for ( i = 0; i < Gia_ManPoNum(pInit); i++ )
        Vec_IntPush( p->vPrefCos, i );
    Int2_ManCreateFrames( p, 0, p->vPrefCos );
    RetValue = Int2_ManCheckBmc( p, NULL );
    if ( RetValue != 1 )
        return RetValue;

    // create original image
    for ( f = pPars->nFramesS; f < p->nFramesMax; f++ )
    {
        for ( i = 0; i < p->nFramesMax; i++ )
        {
            p->pSatSuff = Int2_ManPrepareSuffix( p, vImageOne. vImagesAll, &vCoMap, &pGiaSuff );
            sat_solver_set_runtime_limit( p->pSatSuff, nTimeToStop );
            Vec_IntFreeP( &vImageOne );
            vImageOne = Int2_ManComputePreimage( pGiaSuff, p->pSatPref, p->pSatSuff, vCiMap, vCoMap );
            Vec_IntFree( vCoMap );
            Gia_ManStop( pGiaSuff );
            if ( nTimeToStop && Abc_Clock() > nTimeToStop )
                return -1;
            if ( vImageOne == NULL )
            {
                if ( i == 0 )
                {
                    printf( "Satisfiable in frame %d.\n", f );
                    Vec_IntFree( vCiMap );
                    sat_solver_delete( p->pSatPref ); p->pSatPref = NULL;
                    return 0;
                }
                f += i;
                break;
            }
            Vec_IntAppend( vImagesAll, vImageOne );
            sat_solver_delete( p->pSatSuff ); p->pSatSuff = NULL;
        }
        Vec_IntFree( vCiMap );
        sat_solver_delete( p->pSatPref ); p->pSatPref = NULL;
    }
    Abc_PrintTime( "Time", Abc_Clock() - clk );


p->timeSatPref += Abc_Clock() - clk;

    Int2_ManStop( p );
    return RetValue;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

