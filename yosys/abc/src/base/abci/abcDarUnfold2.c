/**Function*************************************************************

  Synopsis    [Performs BDD-based reachability analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarFold2( Abc_Ntk_t * pNtk, int fCompl, int fVerbose , int);
Abc_Ntk_t * Abc_NtkDarUnfold2( Abc_Ntk_t * pNtk, int nFrames, int nConfs, int nProps, int fStruct, int fOldAlgo, int fVerbose )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp = NULL;
    int typeII_cnt = 0;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
    if ( fStruct ){
      assert(0);//pMan = Saig_ManDupUnfoldConstrs( pTemp = pMan );
    }else
      pMan = Saig_ManDupUnfoldConstrsFunc2( pTemp = pMan, nFrames, nConfs, nProps, fOldAlgo, fVerbose , &typeII_cnt);
    Aig_ManStop( pTemp );
    if ( pMan == NULL )
        return NULL;
    //    typeII_cnt = pMan->nConstrsTypeII;
    pNtkAig = Abc_NtkFromAigPhase( pMan );
    pNtkAig->pName = Extra_UtilStrsav(pMan->pName);
    pNtkAig->pSpec = Extra_UtilStrsav(pMan->pSpec);
    Aig_ManStop( pMan );

    return pNtkAig;//Abc_NtkDarFold2(pNtkAig, 0, fVerbose, typeII_cnt);
    
    //return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Performs BDD-based reachability analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarFold2( Abc_Ntk_t * pNtk, int fCompl, int fVerbose
                             , int typeII_cnt
                             )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
    pMan = Saig_ManDupFoldConstrsFunc2( pTemp = pMan, fCompl, fVerbose, typeII_cnt );
    Aig_ManStop( pTemp );
    pNtkAig = Abc_NtkFromAigPhase( pMan );
    pNtkAig->pName = Extra_UtilStrsav(pMan->pName);
    pNtkAig->pSpec = Extra_UtilStrsav(pMan->pSpec);
    Aig_ManStop( pMan );
    return pNtkAig;
}

