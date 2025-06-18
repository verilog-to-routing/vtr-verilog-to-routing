/**CFile****************************************************************

  FileName    [sswBmc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Inductive prover with constraints.]

  Synopsis    [Bounded model checker using dynamic unrolling.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2008.]

  Revision    [$Id: sswBmc.c,v 1.00 2008/09/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sswInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Incrementally unroll the timeframes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Ssw_BmcUnroll_rec( Ssw_Frm_t * pFrm, Aig_Obj_t * pObj, int f )
{
    Aig_Obj_t * pRes, * pRes0, * pRes1;
    if ( (pRes = Ssw_ObjFrame_(pFrm, pObj, f)) )
        return pRes;
    if ( Aig_ObjIsConst1(pObj) )
        pRes = Aig_ManConst1( pFrm->pFrames );
    else if ( Saig_ObjIsPi(pFrm->pAig, pObj) )
        pRes = Aig_ObjCreateCi( pFrm->pFrames );
    else if ( Aig_ObjIsCo(pObj) )
    {
        Ssw_BmcUnroll_rec( pFrm, Aig_ObjFanin0(pObj), f );
        pRes = Ssw_ObjChild0Fra_( pFrm, pObj, f );
    }
    else if ( Saig_ObjIsLo(pFrm->pAig, pObj) )
    {
        if ( f == 0 )
            pRes = Aig_ManConst0( pFrm->pFrames );
        else
            pRes = Ssw_BmcUnroll_rec( pFrm, Saig_ObjLoToLi(pFrm->pAig, pObj), f-1 );
    }
    else
    {
        assert( Aig_ObjIsNode(pObj) );
        Ssw_BmcUnroll_rec( pFrm, Aig_ObjFanin0(pObj), f );
        Ssw_BmcUnroll_rec( pFrm, Aig_ObjFanin1(pObj), f );
        pRes0 = Ssw_ObjChild0Fra_( pFrm, pObj, f );
        pRes1 = Ssw_ObjChild1Fra_( pFrm, pObj, f );
        pRes = Aig_And( pFrm->pFrames, pRes0, pRes1 );
    }
    Ssw_ObjSetFrame_( pFrm, pObj, f, pRes );
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Derives counter-example.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Ssw_BmcGetCounterExample( Ssw_Frm_t * pFrm, Ssw_Sat_t * pSat, int iPo, int iFrame )
{
    Abc_Cex_t * pCex;
    Aig_Obj_t * pObj, * pObjFrames;
    int f, i, nShift;
    assert( Saig_ManRegNum(pFrm->pAig) > 0 );
    // allocate the counter example
    pCex = Abc_CexAlloc( Saig_ManRegNum(pFrm->pAig), Saig_ManPiNum(pFrm->pAig), iFrame + 1 );
    pCex->iPo    = iPo;
    pCex->iFrame = iFrame;
    // create data-bits
    nShift = Saig_ManRegNum(pFrm->pAig);
    for ( f = 0; f <= iFrame; f++, nShift += Saig_ManPiNum(pFrm->pAig) )
        Saig_ManForEachPi( pFrm->pAig, pObj, i )
        {
            pObjFrames = Ssw_ObjFrame_(pFrm, pObj, f);
            if ( pObjFrames == NULL )
                continue;
            if ( Ssw_CnfGetNodeValue( pSat, pObjFrames ) )
                Abc_InfoSetBit( pCex->pData, nShift + i );
        }
    return pCex;
}


/**Function*************************************************************

  Synopsis    [Performs BMC for the given AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_BmcDynamic( Aig_Man_t * pAig, int nFramesMax, int nConfLimit, int fVerbose, int * piFrame )
{
    Ssw_Frm_t * pFrm;
    Ssw_Sat_t * pSat;
    Aig_Obj_t * pObj, * pObjFrame;
    int status, Lit, i, f, RetValue;
    abctime clkPart;

    // start managers
    assert( Saig_ManRegNum(pAig) > 0 );
    Aig_ManSetCioIds( pAig );
    pSat = Ssw_SatStart( 0 );
    pFrm = Ssw_FrmStart( pAig );
    pFrm->pFrames = Aig_ManStart( Aig_ManObjNumMax(pAig) * 3 );
    // report statistics
    if ( fVerbose )
    {
        Abc_Print( 1, "AIG:  PI/PO/Reg = %d/%d/%d.  Node = %6d. Lev = %5d.\n",
            Saig_ManPiNum(pAig), Saig_ManPoNum(pAig), Saig_ManRegNum(pAig),
            Aig_ManNodeNum(pAig), Aig_ManLevelNum(pAig) );
        fflush( stdout );
    }
    // perform dynamic unrolling
    RetValue = -1;
    for ( f = 0; f < nFramesMax; f++ )
    {
        clkPart = Abc_Clock();
        Saig_ManForEachPo( pAig, pObj, i )
        {
            // unroll the circuit for this output
            Ssw_BmcUnroll_rec( pFrm, pObj, f );
            pObjFrame = Ssw_ObjFrame_( pFrm, pObj, f );
            Ssw_CnfNodeAddToSolver( pSat, Aig_Regular(pObjFrame) );
            status = sat_solver_simplify(pSat->pSat);
            assert( status );
            // solve
            Lit = toLitCond( Ssw_ObjSatNum(pSat,pObjFrame), Aig_IsComplement(pObjFrame) );
            if ( fVerbose )
            {
                Abc_Print( 1, "Solving output %2d of frame %3d ... \r",
                    i % Saig_ManPoNum(pAig), i / Saig_ManPoNum(pAig) );
            }
            status = sat_solver_solve( pSat->pSat, &Lit, &Lit + 1, (ABC_INT64_T)nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
            if ( status == l_False )
            {
/*
                Lit = lit_neg( Lit );
                RetValue = sat_solver_addclause( pSat->pSat, &Lit, &Lit + 1 );
                assert( RetValue );
                if ( pSat->pSat->qtail != pSat->pSat->qhead )
                {
                    RetValue = sat_solver_simplify(pSat->pSat);
                    assert( RetValue );
                }
*/
                RetValue = 1;
                continue;
            }
            else if ( status == l_True )
            {
                pAig->pSeqModel = Ssw_BmcGetCounterExample( pFrm, pSat, i, f );
                if ( piFrame )
                    *piFrame = f;
                RetValue = 0;
                break;
            }
            else
            {
                if ( piFrame )
                    *piFrame = f;
                RetValue = -1;
                break;
            }
        }
        if ( fVerbose )
        {
            Abc_Print( 1, "Solved %2d outputs of frame %3d.  ", Saig_ManPoNum(pAig), f );
            Abc_Print( 1, "Conf =%8.0f. Var =%8d. AIG=%9d. ",
                (double)pSat->pSat->stats.conflicts,
                pSat->nSatVars, Aig_ManNodeNum(pFrm->pFrames) );
            ABC_PRT( "T", Abc_Clock() - clkPart );
            clkPart = Abc_Clock();
            fflush( stdout );
        }
        if ( RetValue != 1 )
            break;
    }

    Ssw_SatStop( pSat );
    Ssw_FrmStop( pFrm );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
