/**CFile****************************************************************

  FileName    [mfsGia.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [The good old minimization with complete don't-cares.]

  Synopsis    [Experimental code based on the new AIG package.]

  Author      [Alan Mishchenko]

  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 29, 2009.]

  Revision    [$Id: mfsGia.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#include "mfsInt.h"
#include "aig/gia/giaAig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline int Gia_ObjChild0Copy( Aig_Obj_t * pObj )  { return Gia_LitNotCond( Aig_ObjFanin0(pObj)->iData, Aig_ObjFaninC0(pObj) ); }
static inline int Gia_ObjChild1Copy( Aig_Obj_t * pObj )  { return Gia_LitNotCond( Aig_ObjFanin1(pObj)->iData, Aig_ObjFaninC1(pObj) ); }

// r i10_if6.blif;  ps; mfs -v
// r pj1_if6.blif;  ps; mfs -v
// r x/01_if6.blif; ps; mfs -v

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives the resubstitution miter as an GIA.]

  Description []
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
Gia_Man_t * Gia_ManCreateResubMiter( Aig_Man_t * p )
{
    Gia_Man_t * pNew;//, * pTemp;
    Aig_Obj_t * pObj;
    int i, * pOuts0, * pOuts1;
    Aig_ManSetPioNumbers( p );
    // create the new manager
    pNew = Gia_ManStart( Aig_ManObjNum(p) );
    pNew->pName = Gia_UtilStrsav( p->pName );
    pNew->pSpec = Gia_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );
    // create the objects
    pOuts0 = ABC_ALLOC( int, Aig_ManPoNum(p) );
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( Aig_ObjIsAnd(pObj) )
            pObj->iData = Gia_ManHashAnd( pNew, Gia_ObjChild0Copy(pObj), Gia_ObjChild1Copy(pObj) );
        else if ( Aig_ObjIsPi(pObj) )
            pObj->iData = Gia_ManAppendCi( pNew );
        else if ( Aig_ObjIsPo(pObj) )
            pOuts0[ Aig_ObjPioNum(pObj) ] = Gia_ObjChild0Copy(pObj);
        else if ( Aig_ObjIsConst1(pObj) )
            pObj->iData = 1;
        else
            assert( 0 );
    }
    // create the objects
    pOuts1 = ABC_ALLOC( int, Aig_ManPoNum(p) );
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( Aig_ObjIsAnd(pObj) )
            pObj->iData = Gia_ManHashAnd( pNew, Gia_ObjChild0Copy(pObj), Gia_ObjChild1Copy(pObj) );
        else if ( Aig_ObjIsPi(pObj) )
            pObj->iData = Gia_ManAppendCi( pNew );
        else if ( Aig_ObjIsPo(pObj) )
            pOuts1[ Aig_ObjPioNum(pObj) ] = Gia_ObjChild0Copy(pObj);
        else if ( Aig_ObjIsConst1(pObj) )
            pObj->iData = 1;
        else
            assert( 0 );
    }
    // add the outputs
    Gia_ManAppendCo( pNew, pOuts0[0] );
    Gia_ManAppendCo( pNew, pOuts1[0] );
    Gia_ManAppendCo( pNew, pOuts0[1] );
    Gia_ManAppendCo( pNew, Gia_LitNot(pOuts1[1]) );
    for ( i = 2; i < Aig_ManPoNum(p); i++ )
        Gia_ManAppendCo( pNew, Gia_LitNot( Gia_ManHashXor(pNew, pOuts0[i], pOuts1[i]) ) );
    Gia_ManHashStop( pNew );
    ABC_FREE( pOuts0 );
    ABC_FREE( pOuts1 );
//    pNew = Gia_ManCleanup( pTemp = pNew );
//    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMfsConstructGia( Mfs_Man_t * p )
{
    int nBTLimit = 500;
    // prepare AIG
    assert( p->pGia == NULL );
    p->pGia = Gia_ManCreateResubMiter( p->pAigWin );
    // prepare AIG
    Gia_ManCreateRefs( p->pGia );
    Gia_ManCleanMark0( p->pGia );
    Gia_ManCleanMark1( p->pGia );
    Gia_ManFillValue ( p->pGia ); // maps nodes into trail ids
    Gia_ManCleanPhase( p->pGia ); 
    // prepare solver
    p->pTas = Tas_ManAlloc( p->pGia, nBTLimit );
    p->vCex = Tas_ReadModel( p->pTas );
    p->vGiaLits = Vec_PtrAlloc( 100 );
}


/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMfsDeconstructGia( Mfs_Man_t * p )
{
    assert( p->pGia != NULL );
    Gia_ManStop( p->pGia ); p->pGia = NULL;
    Tas_ManStop( p->pTas ); p->pTas = NULL;
    Vec_PtrFree( p->vGiaLits );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMfsResimulate( Gia_Man_t * p, Vec_Int_t * vCex )
{
    Gia_Obj_t * pObj;
    int i, Entry;
//    Gia_ManCleanMark1( p );
    Gia_ManConst0(p)->fMark1 = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->fMark1 = 0;
//        pObj->fMark1 = Gia_ManRandom(0);
    Vec_IntForEachEntry( vCex, Entry, i )
    {
        pObj = Gia_ManCi( p, Gia_Lit2Var(Entry) );
        pObj->fMark1 = !Gia_LitIsCompl(Entry);
    }
    Gia_ManForEachAnd( p, pObj, i )
        pObj->fMark1 = (Gia_ObjFanin0(pObj)->fMark1 ^ Gia_ObjFaninC0(pObj)) & 
                       (Gia_ObjFanin1(pObj)->fMark1 ^ Gia_ObjFaninC1(pObj));
    Gia_ManForEachCo( p, pObj, i )
        pObj->fMark1 =  Gia_ObjFanin0(pObj)->fMark1 ^ Gia_ObjFaninC0(pObj);
}




/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMfsTryResubOnceGia( Mfs_Man_t * p, int * pCands, int nCands )
{
    int fVeryVerbose = 0;
    int fUseGia = 1;
    unsigned * pData;
    int i, iVar, status, iOut;
    clock_t clk = clock();
    p->nSatCalls++;
//    return -1;
    assert( p->pGia != NULL );
    assert( p->pTas != NULL );
    // convert to literals
    Vec_PtrClear( p->vGiaLits );
    // create the first four literals
    Vec_PtrPush( p->vGiaLits, Gia_ObjChild0(Gia_ManPo(p->pGia, 0)) );
    Vec_PtrPush( p->vGiaLits, Gia_ObjChild0(Gia_ManPo(p->pGia, 1)) );
    Vec_PtrPush( p->vGiaLits, Gia_ObjChild0(Gia_ManPo(p->pGia, 2)) );
    Vec_PtrPush( p->vGiaLits, Gia_ObjChild0(Gia_ManPo(p->pGia, 3)) );
    for ( i = 0; i < nCands; i++ )
    {
        // get the output number
        iOut = Gia_Lit2Var(pCands[i]) - 2 * p->pCnf->nVars;
        // write the literal
        Vec_PtrPush( p->vGiaLits, Gia_ObjChild0(Gia_ManPo(p->pGia, 4 + iOut)) );
    }
    // perform SAT solving
    status = Tas_ManSolveArray( p->pTas, p->vGiaLits );
    if ( status == -1 )
    {
        p->nTimeOuts++;
        if ( fVeryVerbose )
        printf( "t" );
//        p->nSatUndec++;
//        p->nConfUndec += p->Pars.nBTThis;
//        Cec_ManSatAddToStore( vCexStore, NULL, i ); // timeout
//        p->timeSatUndec += clock() - clk;
    }
    else if ( status == 1 )
    {
        if ( fVeryVerbose )
        printf( "u" );
//        p->nSatUnsat++;
//        p->nConfUnsat += p->Pars.nBTThis;
//        p->timeSatUnsat += clock() - clk;
    }
    else
    {
        p->nSatCexes++;
        if ( fVeryVerbose )
        printf( "s" );
//        p->nSatSat++;
//        p->nConfSat += p->Pars.nBTThis;
//        Gia_SatVerifyPattern( pAig, pRoot, vCex, vVisit );
//        Cec_ManSatAddToStore( vCexStore, vCex, i );
//        p->timeSatSat += clock() - clk;

        // resimulate the counter-example
        Abc_NtkMfsResimulate( p->pGia, Tas_ReadModel(p->pTas) );
   
        if ( fUseGia )
        {
/*
            int Val0 = Gia_ManPo(p->pGia, 0)->fMark1;
            int Val1 = Gia_ManPo(p->pGia, 1)->fMark1;
            int Val2 = Gia_ManPo(p->pGia, 2)->fMark1;
            int Val3 = Gia_ManPo(p->pGia, 3)->fMark1;
            assert( Val0 == 1 );
            assert( Val1 == 1 );
            assert( Val2 == 1 );
            assert( Val3 == 1 );
*/
            // store the counter-example
            Vec_IntForEachEntry( p->vProjVarsSat, iVar, i )
            {
                pData = (unsigned *)Vec_PtrEntry( p->vDivCexes, i );
                iOut = iVar - 2 * p->pCnf->nVars;
//                if ( !Gia_ManPo( p->pGia, 4 + iOut )->fMark1 ) // remove 0s!!!
                if ( Gia_ManPo( p->pGia, 4 + iOut )->fMark1 ) // remove 0s!!!  - rememeber complemented attribute
                {
                    assert( Aig_InfoHasBit(pData, p->nCexes) );
                    Aig_InfoXorBit( pData, p->nCexes );
                }
            }
            p->nCexes++;
        }

    }
    return status;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

