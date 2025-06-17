/**CFile****************************************************************

  FileName    [sswPairs.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Inductive prover with constraints.]

  Synopsis    [Calls to the SAT solver.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2008.]

  Revision    [$Id: sswPairs.c,v 1.00 2008/09/01 00:00:00 alanmi Exp $]

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

  Synopsis    [Reports the status of the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_MiterStatus( Aig_Man_t * p, int fVerbose )
{
    Aig_Obj_t * pObj, * pChild;
    int i, CountConst0 = 0, CountNonConst0 = 0, CountUndecided = 0;
//    if ( p->pData )
//        return 0;
    Saig_ManForEachPo( p, pObj, i )
    {
        pChild = Aig_ObjChild0(pObj);
        // check if the output is constant 0
        if ( pChild == Aig_ManConst0(p) )
        {
            CountConst0++;
            continue;
        }
        // check if the output is constant 1
        if ( pChild == Aig_ManConst1(p) )
        {
            CountNonConst0++;
            continue;
        }
        // check if the output is a primary input
        if ( p->nRegs == 0 && Aig_ObjIsCi(Aig_Regular(pChild)) )
        {
            CountNonConst0++;
            continue;
        }
        // check if the output can be not constant 0
        if ( Aig_Regular(pChild)->fPhase != (unsigned)Aig_IsComplement(pChild) )
        {
            CountNonConst0++;
            continue;
        }
        CountUndecided++;
    }

    if ( fVerbose )
    {
        Abc_Print( 1, "Miter has %d outputs. ", Saig_ManPoNum(p) );
        Abc_Print( 1, "Const0 = %d.  ", CountConst0 );
        Abc_Print( 1, "NonConst0 = %d.  ", CountNonConst0 );
        Abc_Print( 1, "Undecided = %d.  ", CountUndecided );
        Abc_Print( 1, "\n" );
    }

    if ( CountNonConst0 )
        return 0;
    if ( CountUndecided )
        return -1;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Transfer equivalent pairs to the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Ssw_TransferSignalPairs( Aig_Man_t * pMiter, Aig_Man_t * pAig1, Aig_Man_t * pAig2, Vec_Int_t * vIds1, Vec_Int_t * vIds2 )
{
    Vec_Int_t * vIds;
    Aig_Obj_t * pObj1, * pObj2;
    Aig_Obj_t * pObj1m, * pObj2m;
    int i;
    vIds = Vec_IntAlloc( 2 * Vec_IntSize(vIds1) );
    for ( i = 0; i < Vec_IntSize(vIds1); i++ )
    {
        pObj1 = Aig_ManObj( pAig1, Vec_IntEntry(vIds1, i) );
        pObj2 = Aig_ManObj( pAig2, Vec_IntEntry(vIds2, i) );
        pObj1m = Aig_Regular((Aig_Obj_t *)pObj1->pData);
        pObj2m = Aig_Regular((Aig_Obj_t *)pObj2->pData);
        assert( pObj1m && pObj2m );
        if ( pObj1m == pObj2m )
            continue;
        if ( pObj1m->Id < pObj2m->Id )
        {
            Vec_IntPush( vIds, pObj1m->Id );
            Vec_IntPush( vIds, pObj2m->Id );
        }
        else
        {
            Vec_IntPush( vIds, pObj2m->Id );
            Vec_IntPush( vIds, pObj1m->Id );
        }
    }
    return vIds;
}

/**Function*************************************************************

  Synopsis    [Transform pairs into class representation.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t ** Ssw_TransformPairsIntoTempClasses( Vec_Int_t * vPairs, int nObjNumMax )
{
    Vec_Int_t ** pvClasses;   // vector of classes
    int * pReprs;             // mapping nodes into their representatives
    int Entry, idObj, idRepr, idReprObj, idReprRepr, i;
    // allocate data-structures
    pvClasses = ABC_CALLOC( Vec_Int_t *, nObjNumMax );
    pReprs    = ABC_ALLOC( int, nObjNumMax );
    for ( i = 0; i < nObjNumMax; i++ )
        pReprs[i] = -1;
    // consider pairs
    for ( i = 0; i < Vec_IntSize(vPairs); i += 2 )
    {
        // get both objects
        idRepr = Vec_IntEntry( vPairs, i   );
        idObj  = Vec_IntEntry( vPairs, i+1 );
        assert( idObj > 0 );
        assert( (pReprs[idRepr] == -1) || (pvClasses[pReprs[idRepr]] != NULL) );
        assert( (pReprs[idObj]  == -1) || (pvClasses[pReprs[idObj] ] != NULL) );
        // get representatives of both objects
        idReprRepr = pReprs[idRepr];
        idReprObj  = pReprs[idObj];
        // check different situations
        if ( idReprRepr == -1 && idReprObj == -1 )
        {   // they do not have classes
            // create a class
            pvClasses[idRepr] = Vec_IntAlloc( 4 );
            Vec_IntPush( pvClasses[idRepr], idRepr );
            Vec_IntPush( pvClasses[idRepr], idObj );
            pReprs[ idRepr ] = idRepr;
            pReprs[ idObj  ] = idRepr;
        }
        else if ( idReprRepr >= 0 && idReprObj == -1 )
        {   // representative has a class
            // add iObj to the same class
            Vec_IntPushUniqueOrder( pvClasses[idReprRepr], idObj );
            pReprs[ idObj ] = idReprRepr;
        }
        else if ( idReprRepr == -1 && idReprObj >= 0 )
        {   // object has a class
            assert( idReprObj != idRepr );
            if ( idReprObj < idRepr )
            { // add idRepr to the same class
                Vec_IntPushUniqueOrder( pvClasses[idReprObj], idRepr );
                pReprs[ idRepr ] = idReprObj;
            }
            else // if ( idReprObj > idRepr )
            { // make idRepr new representative
                Vec_IntPushFirst( pvClasses[idReprObj], idRepr );
                pvClasses[idRepr] = pvClasses[idReprObj];
                pvClasses[idReprObj] = NULL;
                // set correct representatives of each node
                Vec_IntForEachEntry( pvClasses[idRepr], Entry, i )
                    pReprs[ Entry ] = idRepr;
            }
        }
        else // if ( idReprRepr >= 0 && idReprObj >= 0 )
        {   // both have classes
            if ( idReprRepr == idReprObj )
            {  // the classes are the same
                // nothing to do
            }
            else
            {  // the classes are different
                // find the repr of the new class
                if ( idReprRepr < idReprObj )
                {
                    Vec_IntForEachEntry( pvClasses[idReprObj], Entry, i )
                    {
                        Vec_IntPushUniqueOrder( pvClasses[idReprRepr], Entry );
                        pReprs[ Entry ] = idReprRepr;
                    }
                    Vec_IntFree( pvClasses[idReprObj] );
                    pvClasses[idReprObj] = NULL;
                }
                else // if ( idReprRepr > idReprObj )
                {
                    Vec_IntForEachEntry( pvClasses[idReprRepr], Entry, i )
                    {
                        Vec_IntPushUniqueOrder( pvClasses[idReprObj], Entry );
                        pReprs[ Entry ] = idReprObj;
                    }
                    Vec_IntFree( pvClasses[idReprRepr] );
                    pvClasses[idReprRepr] = NULL;
                }
            }
        }
    }
    ABC_FREE( pReprs );
    return pvClasses;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_FreeTempClasses( Vec_Int_t ** pvClasses, int nObjNumMax )
{
    int i;
    for ( i = 0; i < nObjNumMax; i++ )
        if ( pvClasses[i] )
            Vec_IntFree( pvClasses[i] );
    ABC_FREE( pvClasses );
}

/**Function*************************************************************

  Synopsis    [Performs signal correspondence for the miter of two AIGs with node pairs defined.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Ssw_SignalCorrespondenceWithPairs( Aig_Man_t * pAig1, Aig_Man_t * pAig2, Vec_Int_t * vIds1, Vec_Int_t * vIds2, Ssw_Pars_t * pPars )
{
    Ssw_Man_t * p;
    Aig_Man_t * pAigNew, * pMiter;
    Ssw_Pars_t Pars;
    Vec_Int_t * vPairs;
    Vec_Int_t ** pvClasses;
    assert( Vec_IntSize(vIds1) == Vec_IntSize(vIds2) );
    // create sequential miter
    pMiter = Saig_ManCreateMiter( pAig1, pAig2, 0 );
    Aig_ManCleanup( pMiter );
    // transfer information to the miter
    vPairs = Ssw_TransferSignalPairs( pMiter, pAig1, pAig2, vIds1, vIds2 );
    // create representation of the classes
    pvClasses = Ssw_TransformPairsIntoTempClasses( vPairs, Aig_ManObjNumMax(pMiter) );
    Vec_IntFree( vPairs );
    // if parameters are not given, create them
    if ( pPars == NULL )
        Ssw_ManSetDefaultParams( pPars = &Pars );
    // start the induction manager
    p = Ssw_ManCreate( pMiter, pPars );
    // create equivalence classes using these IDs
    p->ppClasses = Ssw_ClassesPreparePairs( pMiter, pvClasses );
    p->pSml = Ssw_SmlStart( pMiter, 0, p->nFrames + p->pPars->nFramesAddSim, 1 );
    Ssw_ClassesSetData( p->ppClasses, p->pSml, (unsigned(*)(void *,Aig_Obj_t *))Ssw_SmlObjHashWord, (int(*)(void *,Aig_Obj_t *))Ssw_SmlObjIsConstWord, (int(*)(void *,Aig_Obj_t *,Aig_Obj_t *))Ssw_SmlObjsAreEqualWord );
    // perform refinement of classes
    pAigNew = Ssw_SignalCorrespondenceRefine( p );
    // cleanup
    Ssw_FreeTempClasses( pvClasses, Aig_ManObjNumMax(pMiter) );
    Ssw_ManStop( p );
    Aig_ManStop( pMiter );
    return pAigNew;
}

/**Function*************************************************************

  Synopsis    [Runs inductive SEC for the miter of two AIGs with node pairs defined.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Ssw_SignalCorrespondeceTestPairs( Aig_Man_t * pAig )
{
    Aig_Man_t * pAigNew, * pAigRes;
    Ssw_Pars_t Pars, * pPars = &Pars;
    Vec_Int_t * vIds1, * vIds2;
    Aig_Obj_t * pObj, * pRepr;
    int RetValue, i;
    abctime clk = Abc_Clock();
    Ssw_ManSetDefaultParams( pPars );
    pPars->fVerbose = 1;
    pAigNew = Ssw_SignalCorrespondence( pAig, pPars );
    // record pairs of equivalent nodes
    vIds1 = Vec_IntAlloc( Aig_ManObjNumMax(pAig) );
    vIds2 = Vec_IntAlloc( Aig_ManObjNumMax(pAig) );
    Aig_ManForEachObj( pAig, pObj, i )
    {
        pRepr = Aig_Regular((Aig_Obj_t *)pObj->pData);
        if ( pRepr == NULL )
            continue;
        if ( Aig_ManObj(pAigNew, pRepr->Id) == NULL )
            continue;
/*
        if ( Aig_ObjIsNode(pObj) )
            Abc_Print( 1, "n " );
        else if ( Saig_ObjIsPi(pAig, pObj) )
            Abc_Print( 1, "pi " );
        else if ( Saig_ObjIsLo(pAig, pObj) )
            Abc_Print( 1, "lo " );
*/
        Vec_IntPush( vIds1, Aig_ObjId(pObj) );
        Vec_IntPush( vIds2, Aig_ObjId(pRepr) );
    }
    Abc_Print( 1, "Recorded %d pairs (before: %d  after: %d).\n", Vec_IntSize(vIds1), Aig_ManObjNumMax(pAig), Aig_ManObjNumMax(pAigNew) );
    // try the new AIGs
    pAigRes = Ssw_SignalCorrespondenceWithPairs( pAig, pAigNew, vIds1, vIds2, pPars );
    Vec_IntFree( vIds1 );
    Vec_IntFree( vIds2 );
    // report the results
    RetValue = Ssw_MiterStatus( pAigRes, 1 );
    if ( RetValue == 1 )
        Abc_Print( 1, "Verification successful.  " );
    else if ( RetValue == 0 )
        Abc_Print( 1, "Verification failed with the counter-example.  " );
    else
        Abc_Print( 1, "Verification UNDECIDED. Remaining registers %d (total %d).  ",
            Aig_ManRegNum(pAigRes), Aig_ManRegNum(pAig) + Aig_ManRegNum(pAigNew) );
    ABC_PRT( "Time", Abc_Clock() - clk );
    // cleanup
    Aig_ManStop( pAigNew );
    return pAigRes;
}

/**Function*************************************************************

  Synopsis    [Runs inductive SEC for the miter of two AIGs with node pairs defined.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SecWithPairs( Aig_Man_t * pAig1, Aig_Man_t * pAig2, Vec_Int_t * vIds1, Vec_Int_t * vIds2, Ssw_Pars_t * pPars )
{
    Aig_Man_t * pAigRes;
    int RetValue;
    abctime clk = Abc_Clock();
    assert( vIds1 != NULL && vIds2 != NULL );
    // try the new AIGs
    Abc_Print( 1, "Performing specialized verification with node pairs.\n" );
    pAigRes = Ssw_SignalCorrespondenceWithPairs( pAig1, pAig2, vIds1, vIds2, pPars );
    // report the results
    RetValue = Ssw_MiterStatus( pAigRes, 1 );
    if ( RetValue == 1 )
        Abc_Print( 1, "Verification successful.  " );
    else if ( RetValue == 0 )
        Abc_Print( 1, "Verification failed with a counter-example.  " );
    else
        Abc_Print( 1, "Verification UNDECIDED. The number of remaining regs = %d (total = %d).  ",
            Aig_ManRegNum(pAigRes), Aig_ManRegNum(pAig1) + Aig_ManRegNum(pAig2) );
    ABC_PRT( "Time", Abc_Clock() - clk );
    // cleanup
    Aig_ManStop( pAigRes );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Runs inductive SEC for the miter of two AIGs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SecGeneral( Aig_Man_t * pAig1, Aig_Man_t * pAig2, Ssw_Pars_t * pPars )
{
    Aig_Man_t * pAigRes, * pMiter;
    int RetValue;
    abctime clk = Abc_Clock();
    // try the new AIGs
    Abc_Print( 1, "Performing general verification without node pairs.\n" );
    pMiter = Saig_ManCreateMiter( pAig1, pAig2, 0 );
    Aig_ManCleanup( pMiter );
    pAigRes = Ssw_SignalCorrespondence( pMiter, pPars );
    Aig_ManStop( pMiter );
    // report the results
    RetValue = Ssw_MiterStatus( pAigRes, 1 );
    if ( RetValue == 1 )
        Abc_Print( 1, "Verification successful.  " );
    else if ( RetValue == 0 )
        Abc_Print( 1, "Verification failed with a counter-example.  " );
    else
        Abc_Print( 1, "Verification UNDECIDED. The number of remaining regs = %d (total = %d).  ",
            Aig_ManRegNum(pAigRes), Aig_ManRegNum(pAig1) + Aig_ManRegNum(pAig2) );
    ABC_PRT( "Time", Abc_Clock() - clk );
    // cleanup
    Aig_ManStop( pAigRes );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Runs inductive SEC for the miter of two AIGs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SecGeneralMiter( Aig_Man_t * pMiter, Ssw_Pars_t * pPars )
{
    Aig_Man_t * pAigRes;
    int RetValue;
    abctime clk = Abc_Clock();
    // try the new AIGs
//    Abc_Print( 1, "Performing general verification without node pairs.\n" );
    pAigRes = Ssw_SignalCorrespondence( pMiter, pPars );
    // report the results
    RetValue = Ssw_MiterStatus( pAigRes, 1 );
    if ( RetValue == 1 )
        Abc_Print( 1, "Verification successful.  " );
    else if ( RetValue == 0 )
        Abc_Print( 1, "Verification failed with a counter-example.  " );
    else
        Abc_Print( 1, "Verification UNDECIDED. The number of remaining regs = %d (total = %d).  ",
            Aig_ManRegNum(pAigRes), Aig_ManRegNum(pMiter) );
    ABC_PRT( "Time", Abc_Clock() - clk );
    // cleanup
    Aig_ManStop( pAigRes );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
