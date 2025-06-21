/**CFile****************************************************************

  FileName    [saigTempor.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Temporal decomposition.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigTempor.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"
#include "sat/bmc/bmc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates initialized timeframes for temporal decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManTemporFrames( Aig_Man_t * pAig, int nFrames )
{
    Aig_Man_t * pFrames;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i, f;
    // start the frames package
    Aig_ManCleanData( pAig );
    pFrames = Aig_ManStart( Aig_ManObjNumMax(pAig) * nFrames );
    pFrames->pName = Abc_UtilStrsav( pAig->pName );
    // initiliaze the flops
    Saig_ManForEachLo( pAig, pObj, i )
        pObj->pData = Aig_ManConst0(pFrames);
    // for each timeframe
    for ( f = 0; f < nFrames; f++ )
    {
        Aig_ManConst1(pAig)->pData = Aig_ManConst1(pFrames);
        Saig_ManForEachPi( pAig, pObj, i )
            pObj->pData = Aig_ObjCreateCi(pFrames);
        Aig_ManForEachNode( pAig, pObj, i )
            pObj->pData = Aig_And( pFrames, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
        Aig_ManForEachCo( pAig, pObj, i )
            pObj->pData = Aig_ObjChild0Copy(pObj);
        Saig_ManForEachLiLo( pAig, pObjLi, pObjLo, i )
            pObjLo->pData = pObjLi->pData;
    }
    // create POs for the flop inputs
    Saig_ManForEachLi( pAig, pObj, i )
        Aig_ObjCreateCo( pFrames, (Aig_Obj_t *)pObj->pData );
    Aig_ManCleanup( pFrames );
    return pFrames;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManTemporDecompose( Aig_Man_t * pAig, int nFrames )
{
    Aig_Man_t * pAigNew, * pFrames;
    Aig_Obj_t * pObj, * pReset;
    int i;
    if ( pAig->nConstrs > 0 )
    {
        printf( "The AIG manager should have no constraints.\n" );
        return NULL;
    }
    // create initialized timeframes
    pFrames = Saig_ManTemporFrames( pAig, nFrames );
    assert( Aig_ManCoNum(pFrames) == Aig_ManRegNum(pAig) );

    // start the new manager
    Aig_ManCleanData( pAig );
    pAigNew = Aig_ManStart( Aig_ManNodeNum(pAig) );
    pAigNew->pName = Abc_UtilStrsav( pAig->pName );
    // map the constant node and primary inputs
    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pAigNew );
    Saig_ManForEachPi( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pAigNew );

    // insert initialization logic
    Aig_ManConst1(pFrames)->pData = Aig_ManConst1( pAigNew );
    Aig_ManForEachCi( pFrames, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pAigNew );
    Aig_ManForEachNode( pFrames, pObj, i )
        pObj->pData = Aig_And( pAigNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    Aig_ManForEachCo( pFrames, pObj, i )
        pObj->pData = Aig_ObjChild0Copy(pObj);

    // create reset latch (the first one among the latches)
    pReset = Aig_ObjCreateCi( pAigNew );

    // create flop output values
    Saig_ManForEachLo( pAig, pObj, i )
        pObj->pData = Aig_Mux( pAigNew, pReset, Aig_ObjCreateCi(pAigNew), (Aig_Obj_t *)Aig_ManCo(pFrames, i)->pData );
    Aig_ManStop( pFrames );

    // add internal nodes of this frame
    Aig_ManForEachNode( pAig, pObj, i )
        pObj->pData = Aig_And( pAigNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // create primary outputs
    Saig_ManForEachPo( pAig, pObj, i )
        Aig_ObjCreateCo( pAigNew, Aig_ObjChild0Copy(pObj) );

    // create reset latch (the first one among the latches)
    Aig_ObjCreateCo( pAigNew, Aig_ManConst1(pAigNew) );
    // create latch inputs
    Saig_ManForEachLi( pAig, pObj, i )
        Aig_ObjCreateCo( pAigNew, Aig_ObjChild0Copy(pObj) );

    // finalize
    Aig_ManCleanup( pAigNew );
    Aig_ManSetRegNum( pAigNew, Aig_ManRegNum(pAig)+1 ); // + reset latch (011111...)
    return pAigNew;
}

/**Function*************************************************************

  Synopsis    [Find index of first non-zero entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Vec_IntLastNonZeroBeforeLimit( Vec_Int_t * vTemp, int Limit )
{
    int Entry, i;
    if ( vTemp == NULL )
        return -1;
    Vec_IntForEachEntryReverse( vTemp, Entry, i )
    {
        if ( i >= Limit )
            continue;
        if ( Entry )
            return i;
    }
    return -1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManTempor( Aig_Man_t * pAig, int nFrames, int TimeOut, int nConfLimit, int fUseBmc, int fUseTransSigs, int fVerbose, int fVeryVerbose )
{ 
    extern int Saig_ManPhasePrefixLength( Aig_Man_t * p, int fVerbose, int fVeryVerbose, Vec_Int_t ** pvTrans );

    Vec_Int_t * vTransSigs = NULL;
    int RetValue, nFramesFinished = -1;
    assert( nFrames >= 0 ); 
    if ( nFrames == 0 )
    {
        nFrames = Saig_ManPhasePrefixLength( pAig, fVerbose, fVeryVerbose, &vTransSigs );
        if ( nFrames == 0 )
        {
            Vec_IntFreeP( &vTransSigs );
            printf( "The leading sequence has length 0. Temporal decomposition is not performed.\n" );
            return NULL;
        }
        if ( nFrames == 1 )
        {
            Vec_IntFreeP( &vTransSigs );
            printf( "The leading sequence has length 1. Temporal decomposition is not performed.\n" );
            return NULL;
        }
        if ( fUseTransSigs )
        {
            int Entry, i, iLast = -1;
            Vec_IntForEachEntry( vTransSigs, Entry, i )
                iLast = Entry ? i :iLast;
            if ( iLast > 0 && iLast < nFrames )
            {
                Abc_Print( 1, "Reducing frame count from %d to %d to fit the last transient.\n", nFrames, iLast );
                nFrames = iLast;
            }
        }
        Abc_Print( 1, "Using computed frame number (%d).\n", nFrames );
    }
    else
        Abc_Print( 1, "Using user-given frame number (%d).\n", nFrames );
    // run BMC2
    if ( fUseBmc )
    {
        RetValue = Saig_BmcPerform( pAig, 0, nFrames, 2000, TimeOut, nConfLimit, 0, fVerbose, 0, &nFramesFinished, 0, 0 );
        if ( RetValue == 0 )
        {
            Vec_IntFreeP( &vTransSigs );
            printf( "A cex found in the first %d frames.\n", nFrames );
            return NULL;
        }
        if ( nFramesFinished + 1 < nFrames )
        {
            int iLastBefore = Vec_IntLastNonZeroBeforeLimit( vTransSigs, nFramesFinished );
            if ( iLastBefore < 1 || !fUseTransSigs )
            {
                Vec_IntFreeP( &vTransSigs );
                printf( "BMC for %d frames could not be completed. A cex may exist!\n", nFrames );
                return NULL;
            }
            assert( iLastBefore < nFramesFinished );
            printf( "BMC succeeded to frame %d. Adjusting frame count to be (%d) based on the last transient signal.\n", nFramesFinished, iLastBefore );
            nFrames = iLastBefore;
        }
    }
    Vec_IntFreeP( &vTransSigs );
    return Saig_ManTemporDecompose( pAig, nFrames );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

