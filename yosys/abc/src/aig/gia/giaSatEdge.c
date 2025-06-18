/**CFile****************************************************************

  FileName    [giaSatEdge.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaSatEdge.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/tim/tim.h"
#include "sat/bsat/satStore.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Seg_Man_t_ Seg_Man_t;
struct Seg_Man_t_
{
    sat_solver *   pSat;         // SAT solver
    //Vec_Int_t *    vCardVars;    // candinality variables
    int            nVars;        // max vars (edge num)
    int            LogN;         // base-2 log of max vars
    int            Power2;       // power-2 of LogN
    int            FirstVar;     // first variable to be used
    // parameters
    int            nBTLimit;     // conflicts
    int            DelayMax;     // external delay
    int            nEdges;       // the number of edges
    int            fDelay;       // delay mode
    int            fReverse;     // reverse windowing
    int            fVerbose;     // verbose
    // window
    Gia_Man_t *    pGia;
    Vec_Int_t *    vPolars;      // polarity
    Vec_Int_t *    vToSkip;      // edges to skip
    Vec_Int_t *    vEdges;       // edges as fanin/fanout pairs 
    Vec_Int_t *    vFirsts;      // first SAT variable
    Vec_Int_t *    vNvars;       // number of SAT variables
    Vec_Int_t *    vLits;        // literals
    int *          pLevels;      // levels

    // statistics
    abctime        timeStart;
};

extern sat_solver * Sbm_AddCardinSolver( int LogN, Vec_Int_t ** pvVars );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Count the number of edges between internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Seg_ManCountIntEdges( Gia_Man_t * p, Vec_Int_t * vPolars, Vec_Int_t * vToSkip, int nFanouts )
{
    int i, iLut, iFanin;
    Vec_Int_t * vEdges = Vec_IntAlloc( 1000 );
    assert( Gia_ManHasMapping(p) );
    Vec_IntClear( vPolars );
    Vec_IntClear( vToSkip );
    if ( nFanouts )
        Gia_ManSetLutRefs( p );
    Gia_ManForEachLut( p, iLut )
        Gia_LutForEachFanin( p, iLut, iFanin, i )
            if ( Gia_ObjIsAnd(Gia_ManObj(p, iFanin)) )
            {
                if ( p->vEdge1 && Gia_ObjCheckEdge(p, iFanin, iLut) )
                    Vec_IntPush( vPolars, Vec_IntSize(vEdges)/2 );
                if ( nFanouts && Gia_ObjLutRefNumId(p, iFanin) >= nFanouts )
                    Vec_IntPush( vToSkip, Vec_IntSize(vEdges)/2 );
                Vec_IntPushTwo( vEdges, iFanin, iLut );
            }
    if ( nFanouts )
        ABC_FREE( p->pLutRefs );
    return vEdges;
}
Vec_Wec_t * Seg_ManCollectObjEdges( Vec_Int_t * vEdges, int nObjs )
{
    int iFanin, iObj, i;
    Vec_Wec_t * vRes = Vec_WecStart( nObjs );
    Vec_IntForEachEntryDouble( vEdges, iFanin, iObj, i )
    {
        Vec_WecPush( vRes, iFanin, i/2 );
        Vec_WecPush( vRes, iObj, i/2 );
    }
    return vRes;
}
int Seg_ManCountIntLevels( Seg_Man_t * p, int iStartVar )
{
    Gia_Obj_t * pObj;
    int iLut, nVars;
    Vec_IntFill( p->vFirsts, Gia_ManObjNum(p->pGia), -1 );
    Vec_IntFill( p->vNvars,  Gia_ManObjNum(p->pGia), 0 );
    ABC_FREE( p->pLevels );
    if ( p->pGia->pManTime )
    {
        p->DelayMax = Gia_ManLutLevelWithBoxes( p->pGia );
        p->pLevels = Vec_IntReleaseArray( p->pGia->vLevels );
        Vec_IntFreeP( &p->pGia->vLevels );
    }
    else
        p->DelayMax = Gia_ManLutLevel( p->pGia, &p->pLevels );
    Gia_ManForEachObj1( p->pGia, pObj, iLut )
    {
        if ( Gia_ObjIsCo(pObj) )
            continue;
        if ( Gia_ObjIsAnd(pObj) && !Gia_ObjIsLut(p->pGia, iLut) )
            continue;
        assert( Gia_ObjIsCi(pObj) || Gia_ObjIsLut(p->pGia, iLut) );
        Vec_IntWriteEntry( p->vFirsts, iLut, iStartVar );
        //assert( p->pLevels[iLut] > 0 );
        nVars = p->pLevels[iLut] < 2 ? 0 : p->pLevels[iLut];
        Vec_IntWriteEntry( p->vNvars,  iLut, nVars );
        iStartVar += nVars;
    }
    return iStartVar;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Seg_Man_t * Seg_ManAlloc( Gia_Man_t * pGia, int nFanouts )
{
    int nVarsAll;
    Seg_Man_t * p = ABC_CALLOC( Seg_Man_t, 1 );
    p->vPolars    = Vec_IntAlloc( 1000 ); 
    p->vToSkip    = Vec_IntAlloc( 1000 ); 
    p->vEdges     = Seg_ManCountIntEdges( pGia, p->vPolars, p->vToSkip, nFanouts );
    p->nVars      = Vec_IntSize(p->vEdges)/2;
    p->LogN       = Abc_Base2Log(p->nVars);
    p->Power2     = 1 << p->LogN;
    //p->pSat       = Sbm_AddCardinSolver( p->LogN, &p->vCardVars );
    p->pSat       = sat_solver_new();
    sat_solver_setnvars( p->pSat, p->nVars );
    p->FirstVar   = sat_solver_nvars( p->pSat );
    sat_solver_bookmark( p->pSat );
    p->pGia       = pGia;
    // internal
    p->vFirsts    = Vec_IntAlloc( 0 ); 
    p->vNvars     = Vec_IntAlloc( 0 ); 
    p->vLits      = Vec_IntAlloc( 0 ); 
    nVarsAll      = Seg_ManCountIntLevels( p, p->FirstVar );
    sat_solver_setnvars( p->pSat, nVarsAll );
    // other
    Gia_ManFillValue( pGia );
    return p;
}
void Seg_ManClean( Seg_Man_t * p )
{
    p->timeStart = Abc_Clock();
    sat_solver_rollback( p->pSat );
    sat_solver_bookmark( p->pSat );
    // internal
    Vec_IntClear( p->vEdges );
    Vec_IntClear( p->vFirsts );
    Vec_IntClear( p->vNvars );
    Vec_IntClear( p->vLits );
    Vec_IntClear( p->vPolars );
    Vec_IntClear( p->vToSkip );
    // other
    Gia_ManFillValue( p->pGia );
}
void Seg_ManStop( Seg_Man_t * p )
{
    sat_solver_delete( p->pSat );
    //Vec_IntFree( p->vCardVars );
    // internal
    Vec_IntFree( p->vEdges );
    Vec_IntFree( p->vFirsts );
    Vec_IntFree( p->vNvars );
    Vec_IntFree( p->vLits );
    Vec_IntFree( p->vPolars );
    Vec_IntFree( p->vToSkip );
    ABC_FREE( p->pLevels );
    // other
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seg_ManCreateCnf( Seg_Man_t * p, int fTwo, int fVerbose )
{
    Tim_Man_t * pTim = (Tim_Man_t *)p->pGia->pManTime;
    Gia_Obj_t * pObj;
    Vec_Wec_t * vObjEdges;
    Vec_Int_t * vLevel;
    int iLut, iFanin, iFirst;
    int pLits[3], Count = 0;
    int i, k, nVars, Edge, value;
    abctime clk = Abc_Clock();
    // edge delay constraints
    int nConstr = sat_solver_nclauses(p->pSat);
    Gia_ManForEachObj( p->pGia, pObj, iLut )
    {
        int iFirstLut = Vec_IntEntry( p->vFirsts, iLut );
        int nVarsLut  = Vec_IntEntry( p->vNvars, iLut );
        if ( pTim && Gia_ObjIsCi(pObj) )
        {
            int iBox = Tim_ManBoxForCi( pTim, Gia_ObjCioId(pObj) );
            if ( nVarsLut > 0 && iBox >= 0 )
            {
                int iCiId = Tim_ManBoxOutputFirst( pTim, iBox );
                if ( iCiId == Gia_ObjCioId(pObj) ) // first input
                {
                    int nIns = Tim_ManBoxInputNum( pTim, iBox );
                    int iIn0 = Tim_ManBoxInputFirst( pTim, iBox );
                    for ( i = 0; i < nIns-1; i++ ) // skip carry-in pin
                    {
                        Gia_Obj_t * pOut = Gia_ManCo( p->pGia, iIn0+i );
                        int iDriverId = Gia_ObjFaninId0p( p->pGia, pOut );
                        int AddOn;

                        iFirst = Vec_IntEntry( p->vFirsts, iDriverId );
                        nVars  = Vec_IntEntry( p->vNvars, iDriverId );
                        assert( nVars < nVarsLut );
                        AddOn = (int)(nVars < nVarsLut);
                        for ( k = 0; k < nVars; k++ )
                        {
                            pLits[0] = Abc_Var2Lit( iFirst+k, 1 );
                            pLits[1] = Abc_Var2Lit( iFirstLut+k+AddOn, 0 );
                            value = sat_solver_addclause( p->pSat, pLits, pLits+2 );
                            assert( value );
                        }
                    }
                }
                else // intermediate input
                {
                    Gia_Obj_t * pIn = Gia_ManCi( p->pGia, iCiId );
                    int iObjId = Gia_ObjId( p->pGia, pIn );

                    iFirst = Vec_IntEntry( p->vFirsts, iObjId );
                    nVars  = Vec_IntEntry( p->vNvars, iObjId );
                    if ( nVars > 0 )
                    {
                        for ( k = 0; k < nVars; k++ )
                        {
                            pLits[0] = Abc_Var2Lit( iFirst+k, 1 );
                            pLits[1] = Abc_Var2Lit( iFirstLut+k, 0 );
                            value = sat_solver_addclause( p->pSat, pLits, pLits+2 );
                            assert( value );
                        }
                    }
                }
            }
            continue;
        }
        if ( !Gia_ObjIsLut(p->pGia, iLut) )
            continue;
        Gia_LutForEachFanin( p->pGia, iLut, iFanin, i )
            if ( pTim && Gia_ObjIsCi(Gia_ManObj(p->pGia, iFanin)) )
            {
                iFirst = Vec_IntEntry( p->vFirsts, iFanin );
                nVars  = Vec_IntEntry( p->vNvars, iFanin );
                assert( nVars <= nVarsLut );
                if ( nVars > 0 )
                {
                    for ( k = 0; k < nVars; k++ )
                    {
                        pLits[0] = Abc_Var2Lit( iFirst+k, 1 );
                        pLits[1] = Abc_Var2Lit( iFirstLut+k, 0 );
                        value = sat_solver_addclause( p->pSat, pLits, pLits+2 );
                        assert( value );
                    }
                }
            }
            else if ( Gia_ObjIsAnd(Gia_ManObj(p->pGia, iFanin)) )
            {
                iFirst = Vec_IntEntry( p->vFirsts, iFanin );
                nVars  = Vec_IntEntry( p->vNvars, iFanin );
                assert( nVars != 1 && nVars < nVarsLut );
                // add initial
                if ( nVars == 0 )
                {
                    pLits[0] = Abc_Var2Lit( Count, 1 );
                    pLits[1] = Abc_Var2Lit( iFirstLut, 0 );
                    value = sat_solver_addclause( p->pSat, pLits, pLits+2 );
                    assert( value );

                    pLits[0] = Abc_Var2Lit( Count, 0 );
                    pLits[1] = Abc_Var2Lit( iFirstLut+1, 0 );
                    value = sat_solver_addclause( p->pSat, pLits, pLits+2 );
                    assert( value );
                }
                // add others
                for ( k = 0; k < nVars; k++ )
                {
                    pLits[0] = Abc_Var2Lit( iFirst+k, 1 );
                    pLits[1] = Abc_Var2Lit( Count, 1 );
                    pLits[2] = Abc_Var2Lit( iFirstLut+k, 0 );
                    value = sat_solver_addclause( p->pSat, pLits, pLits+3 );
                    assert( value );

                    pLits[0] = Abc_Var2Lit( iFirst+k, 1 );
                    pLits[1] = Abc_Var2Lit( Count, 0 );
                    pLits[2] = Abc_Var2Lit( iFirstLut+k+1, 0 );
                    value = sat_solver_addclause( p->pSat, pLits, pLits+3 );
                    assert( value );
                }
                Count++;
            }
    }
    assert( Count == p->nVars );
    if ( fVerbose )
    printf( "Delay constraints = %d. ", sat_solver_nclauses(p->pSat) - nConstr );
    nConstr = sat_solver_nclauses(p->pSat);
/*
    // delay relationship constraints
    Vec_IntForEachEntryTwo( p->vFirsts, p->vNvars, iFirst, nVars, iLut )
    {
        if ( nVars < 2 )
            continue;
        for ( i = 1; i < nVars; i++ )
        {
            pLits[0] = Abc_Var2Lit( iFirst + i - 1, 1 );
            pLits[1] = Abc_Var2Lit( iFirst + i,     0 );
            value = sat_solver_addclause( p->pSat, pLits, pLits+2 );
            assert( value );
        }
    }
*/
    // edge compatibility constraint
    vObjEdges = Seg_ManCollectObjEdges( p->vEdges, Gia_ManObjNum(p->pGia) );
    Vec_WecForEachLevel( vObjEdges, vLevel, i )
    {
        int v1, v2, v3, Var1, Var2, Var3;
        if ( (!fTwo && Vec_IntSize(vLevel) >= 2) || (fTwo && Vec_IntSize(vLevel) > 10) )
        {
            Vec_IntForEachEntry( vLevel, Var1, v1 )
            Vec_IntForEachEntryStart( vLevel, Var2, v2, v1 + 1 )
            {
                pLits[0] = Abc_Var2Lit( Var1, 1 );
                pLits[1] = Abc_Var2Lit( Var2, 1 );
                value = sat_solver_addclause( p->pSat, pLits, pLits+2 );
                assert( value );
            }
        }
        else if ( fTwo && Vec_IntSize(vLevel) >= 3 )
        {
            Vec_IntForEachEntry( vLevel, Var1, v1 )
            Vec_IntForEachEntryStart( vLevel, Var2, v2, v1 + 1 )
            Vec_IntForEachEntryStart( vLevel, Var3, v3, v2 + 1 )
            {
                pLits[0] = Abc_Var2Lit( Var1, 1 );
                pLits[1] = Abc_Var2Lit( Var2, 1 );
                pLits[2] = Abc_Var2Lit( Var3, 1 );
                value = sat_solver_addclause( p->pSat, pLits, pLits+3 );
                assert( value );
            }
        }
    }
    Vec_WecFree( vObjEdges );
    // block forbidden edges
    Vec_IntForEachEntry( p->vToSkip, Edge, i )
    {
        pLits[0] = Abc_Var2Lit( Edge, 1 );
        value = sat_solver_addclause( p->pSat, pLits, pLits+1 );
        assert( value );
    }
    if ( fVerbose )
    printf( "Edge constraints = %d. ", sat_solver_nclauses(p->pSat) - nConstr );
    if ( fVerbose )
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Seg_ManConvertResult( Seg_Man_t * p )
{
    int iFanin, iObj, i;
    Vec_Int_t * vEdges2 = Vec_IntAlloc( 1000 );
    Vec_IntForEachEntryDouble( p->vEdges, iFanin, iObj, i )
        if ( sat_solver_var_value(p->pSat, i/2) )
            Vec_IntPushTwo( vEdges2, iFanin, iObj );
    return vEdges2;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seg_ManComputeDelay( Gia_Man_t * pGia, int DelayInit, int nFanouts, int fTwo, int fVerbose )
{
    int nBTLimit = 0;
    int nTimeOut = 0;
    int fVeryVerbose = 0;

    Gia_Obj_t * pObj;
    abctime clk = Abc_Clock();
    Vec_Int_t * vEdges2 = NULL;
    int i, iLut, iFirst, nVars, status, Delay, nConfs;
    Seg_Man_t * p = Seg_ManAlloc( pGia, nFanouts );
    int DelayStart = DelayInit ? DelayInit : p->DelayMax;

    if ( fVerbose )
    printf( "Running SatEdge with starting delay %d and edge %d (edge vars %d, total vars %d)\n", 
        DelayStart, fTwo+1, p->FirstVar, sat_solver_nvars(p->pSat) );
    Seg_ManCreateCnf( p, fTwo, fVerbose );
    //Sat_SolverWriteDimacs( p->pSat, "test_edge.cnf", NULL, NULL, 0 );
    // set resource limits
    sat_solver_set_resource_limits( p->pSat, nBTLimit, 0, 0, 0 );
    sat_solver_set_runtime_limit( p->pSat, nTimeOut ? nTimeOut * CLOCKS_PER_SEC + Abc_Clock(): 0 );
    sat_solver_set_random( p->pSat, 1 );
    sat_solver_set_polarity( p->pSat, Vec_IntArray(p->vPolars), Vec_IntSize(p->vPolars) );
    //sat_solver_set_var_activity( p->pSat, NULL, p->nVars );
    // increment delay gradually
    for ( Delay = p->DelayMax; Delay >= 0; Delay-- )
    {
        // we constrain COs, although it would be fine to constrain only POs
        Gia_ManForEachCoDriver( p->pGia, pObj, i )
        {
            iLut   = Gia_ObjId( p->pGia, pObj );
            iFirst = Vec_IntEntry( p->vFirsts, iLut );
            nVars  = Vec_IntEntry( p->vNvars, iLut );
            if ( Delay < nVars && !sat_solver_push(p->pSat, Abc_Var2Lit(iFirst + Delay, 1)) )
                break;
        }
        if ( i < Gia_ManCoNum(p->pGia) )
        {
            printf( "Proved UNSAT for delay %d.  ", Delay );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
            break;
        }
        if ( Delay > DelayStart )
            continue;
        // solve with assumptions
        //clk = Abc_Clock();
        nConfs = sat_solver_nconflicts( p->pSat );
        status = sat_solver_solve_internal( p->pSat );
        nConfs = sat_solver_nconflicts( p->pSat ) - nConfs;
        if ( status == l_True )
        {
            if ( fVerbose )
            {
                int Count = 0;
                for ( i = 0; i < p->nVars; i++ )
                    Count += sat_solver_var_value(p->pSat, i);
                printf( "Solution with delay %2d and %5d edges exists. Conf = %8d.  ", Delay, Count, nConfs );
                Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
            }
            // save the result
            Vec_IntFreeP( &vEdges2 );
            vEdges2 = Seg_ManConvertResult( p );
            if ( fVeryVerbose )
            {
                for ( i = 0; i < p->nVars; i++ )
                    printf( "%d=%d ", i, sat_solver_var_value(p->pSat, i) );
                printf( "   " );

                for ( i = p->nVars; i < sat_solver_nvars(p->pSat); i++ )
                    printf( "%d=%d ", i, sat_solver_var_value(p->pSat, i) );
                printf( "\n" );
            }
        }
        else
        {
            if ( fVerbose )
            {
                if ( status == l_False )
                    printf( "Proved UNSAT for delay %d.  ", Delay );
                else
                    printf( "Resource limit reached for delay %d.  ", Delay );
                Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
            }
            break;
        }
    }
    Gia_ManEdgeFromArray( p->pGia, vEdges2 );
    Vec_IntFreeP( &vEdges2 );
    Seg_ManStop( p );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

