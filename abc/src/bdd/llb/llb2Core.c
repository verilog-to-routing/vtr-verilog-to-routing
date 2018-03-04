/**CFile****************************************************************

  FileName    [llb2Core.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD based reachability.]

  Synopsis    [Core procedure.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: llb2Core.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "llbInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Llb_Img_t_ Llb_Img_t;
struct Llb_Img_t_
{
    Aig_Man_t *     pInit;          // AIG manager
    Aig_Man_t *     pAig;           // AIG manager
    Gia_ParLlb_t *  pPars;          // parameters

    DdManager *     dd;             // BDD manager
    DdManager *     ddG;            // BDD manager
    DdManager *     ddR;            // BDD manager
    Vec_Ptr_t *     vDdMans;        // BDD managers for each partition
    Vec_Ptr_t *     vRings;         // onion rings in ddR

    Vec_Int_t *     vDriRefs;       // driver references
    Vec_Int_t *     vVarsCs;        // cur state variables
    Vec_Int_t *     vVarsNs;        // next state variables

    Vec_Int_t *     vCs2Glo;        // cur state variables into global variables
    Vec_Int_t *     vNs2Glo;        // next state variables into global variables
    Vec_Int_t *     vGlo2Cs;        // global variables into cur state variables
    Vec_Int_t *     vGlo2Ns;        // global variables into next state variables
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes cube composed of given variables with given values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_CoreComputeCube( DdManager * dd, Vec_Int_t * vVars, int fUseVarIndex, char * pValues )
{
    DdNode * bRes, * bVar, * bTemp;
    int i, iVar, Index;
    abctime TimeStop;
    TimeStop = dd->TimeStop; dd->TimeStop = 0;
    bRes = Cudd_ReadOne( dd );   Cudd_Ref( bRes );
    Vec_IntForEachEntry( vVars, Index, i )
    {
        iVar  = fUseVarIndex ? Index : i;
        bVar  = Cudd_NotCond( Cudd_bddIthVar(dd, iVar), (int)(pValues == NULL || pValues[i] != 1) );
        bRes  = Cudd_bddAnd( dd, bTemp = bRes, bVar );  Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bRes );
    dd->TimeStop = TimeStop;
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Derives counter-example by backward reachability.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Llb_CoreDeriveCex( Llb_Img_t * p )
{
    Abc_Cex_t * pCex;
    Aig_Obj_t * pObj;
    Vec_Ptr_t * vSupps, * vQuant0, * vQuant1;
    DdNode * bState = NULL, * bImage, * bOneCube, * bTemp, * bRing;
    int i, v, RetValue, nPiOffset;
    char * pValues = ABC_ALLOC( char, Cudd_ReadSize(p->ddR) );
    assert( Vec_PtrSize(p->vRings) > 0 );

    p->dd->TimeStop  = 0;
    p->ddR->TimeStop = 0;

    // get supports and quantified variables
    Vec_PtrReverseOrder( p->vDdMans );
    vSupps = Llb_ImgSupports( p->pAig, p->vDdMans, p->vVarsNs, p->vVarsCs, 1, 0 );
    Llb_ImgSchedule( vSupps, &vQuant0, &vQuant1, 0 );
    Vec_VecFree( (Vec_Vec_t *)vSupps );
    Llb_ImgQuantifyReset( p->vDdMans );
//    Llb_ImgQuantifyFirst( p->pAig, p->vDdMans, vQuant0 );

    // allocate room for the counter-example
    pCex = Abc_CexAlloc( Saig_ManRegNum(p->pAig), Saig_ManPiNum(p->pAig), Vec_PtrSize(p->vRings) );
    pCex->iFrame = Vec_PtrSize(p->vRings) - 1;
    pCex->iPo = -1;

    // get the last cube
    bOneCube = Cudd_bddIntersect( p->ddR, (DdNode *)Vec_PtrEntryLast(p->vRings), p->ddR->bFunc );  Cudd_Ref( bOneCube );
    RetValue = Cudd_bddPickOneCube( p->ddR, bOneCube, pValues );
    Cudd_RecursiveDeref( p->ddR, bOneCube );
    assert( RetValue );

    // write PIs of counter-example
    nPiOffset = Saig_ManRegNum(p->pAig) + Saig_ManPiNum(p->pAig) * (Vec_PtrSize(p->vRings) - 1);
    Saig_ManForEachPi( p->pAig, pObj, i )
        if ( pValues[Saig_ManRegNum(p->pAig)+i] == 1 )
            Abc_InfoSetBit( pCex->pData, nPiOffset + i );

    // write state in terms of NS variables
    if ( Vec_PtrSize(p->vRings) > 1 )
    {
        bState = Llb_CoreComputeCube( p->dd, p->vVarsNs, 1, pValues );   Cudd_Ref( bState );
    }
    // perform backward analysis
    Vec_PtrForEachEntryReverse( DdNode *, p->vRings, bRing, v )
    { 
        if ( v == Vec_PtrSize(p->vRings) - 1 )
            continue;
        // compute the next states
        bImage = Llb_ImgComputeImage( p->pAig, p->vDdMans, p->dd, bState, 
            vQuant0, vQuant1, p->vDriRefs, p->pPars->TimeTarget, 1, 0, 0 );
        assert( bImage != NULL );
        Cudd_Ref( bImage );
        Cudd_RecursiveDeref( p->dd, bState );
//Extra_bddPrintSupport( p->dd, bImage ); printf( "\n" );

        // move reached states into ring manager
        bImage = Extra_TransferPermute( p->dd, p->ddR, bTemp = bImage, Vec_IntArray(p->vCs2Glo) );    Cudd_Ref( bImage );
        Cudd_RecursiveDeref( p->dd, bTemp );

        // intersect with the previous set
        bOneCube = Cudd_bddIntersect( p->ddR, bImage, bRing );                Cudd_Ref( bOneCube );
        Cudd_RecursiveDeref( p->ddR, bImage );

        // find any assignment of the BDD
        RetValue = Cudd_bddPickOneCube( p->ddR, bOneCube, pValues );
        Cudd_RecursiveDeref( p->ddR, bOneCube );
        assert( RetValue );

        // write PIs of counter-example
        nPiOffset -= Saig_ManPiNum(p->pAig);
        Saig_ManForEachPi( p->pAig, pObj, i )
            if ( pValues[Saig_ManRegNum(p->pAig)+i] == 1 )
                Abc_InfoSetBit( pCex->pData, nPiOffset + i );

        // check that we get the init state
        if ( v == 0 )
        {
            Saig_ManForEachLo( p->pAig, pObj, i )
                assert( pValues[i] == 0 );
            break;
        }

        // write state in terms of NS variables
        bState = Llb_CoreComputeCube( p->dd, p->vVarsNs, 1, pValues );   Cudd_Ref( bState );
    }
    assert( nPiOffset == Saig_ManRegNum(p->pAig) );
    // update the output number
    RetValue = Saig_ManFindFailedPoCex( p->pInit, pCex );
    assert( RetValue >= 0 && RetValue < Saig_ManPoNum(p->pInit) ); // invalid CEX!!!
    pCex->iPo = RetValue;
    // cleanup
    ABC_FREE( pValues );
    Vec_VecFree( (Vec_Vec_t *)vQuant0 );
    Vec_VecFree( (Vec_Vec_t *)vQuant1 );
    return pCex;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_CoreReachability_int( Llb_Img_t * p, Vec_Ptr_t * vQuant0, Vec_Ptr_t * vQuant1 )
{
    int * pLoc2Glo  = p->pPars->fBackward? Vec_IntArray( p->vCs2Glo ) : Vec_IntArray( p->vNs2Glo );
    int * pLoc2GloR = p->pPars->fBackward? Vec_IntArray( p->vNs2Glo ) : Vec_IntArray( p->vCs2Glo );
    int * pGlo2Loc  = p->pPars->fBackward? Vec_IntArray( p->vGlo2Ns ) : Vec_IntArray( p->vGlo2Cs );
    DdNode * bCurrent, * bReached, * bNext, * bTemp;
    abctime clk2, clk = Abc_Clock();
    int nIters, nBddSize;//, iOutFail = -1;
/*
    // compute time to stop
    if ( p->pPars->TimeLimit )
        p->pPars->TimeTarget = Abc_Clock() + p->pPars->TimeLimit * CLOCKS_PER_SEC;
    else
        p->pPars->TimeTarget = 0;
*/

    if ( Abc_Clock() > p->pPars->TimeTarget )
    {
        if ( !p->pPars->fSilent )
            printf( "Reached timeout (%d seconds) before image computation.\n", p->pPars->TimeLimit );
        p->pPars->iFrame = -1;
        return -1;
    }

    // set the stop time parameter
    p->dd->TimeStop  = p->pPars->TimeTarget;
    p->ddG->TimeStop = p->pPars->TimeTarget;
    p->ddR->TimeStop = p->pPars->TimeTarget;

    // compute initial states
    if ( p->pPars->fBackward )
    {
        // create init state in the global manager
        bTemp = Llb_BddComputeBad( p->pInit, p->ddR, p->pPars->TimeTarget );   
        if ( bTemp == NULL )
        {
            if ( !p->pPars->fSilent )
                printf( "Reached timeout (%d seconds) while computing bad states.\n", p->pPars->TimeLimit );
            p->pPars->iFrame = -1;
            return -1;
        }
        Cudd_Ref( bTemp );
        // create bad state in the ring manager
        p->ddR->bFunc = Llb_CoreComputeCube( p->ddR, p->vVarsCs, 0, NULL );      Cudd_Ref( p->ddR->bFunc );
        bCurrent = Llb_BddQuantifyPis( p->pInit, p->ddR, bTemp );                Cudd_Ref( bCurrent );
        Cudd_RecursiveDeref( p->ddR, bTemp );
        bReached = Cudd_bddTransfer( p->ddR, p->ddG, bCurrent );                 Cudd_Ref( bReached );
        Cudd_RecursiveDeref( p->ddR, bCurrent );
        // move init state to the working manager
        bCurrent = Extra_TransferPermute( p->ddG, p->dd, bReached, pGlo2Loc );   
        if ( bCurrent == NULL )
        {
            Cudd_RecursiveDeref( p->ddG, bReached );
            if ( !p->pPars->fSilent )
                printf( "Reached timeout (%d seconds) during transfer 0.\n", p->pPars->TimeLimit );
            p->pPars->iFrame = -1;
            return -1;
        }
        Cudd_Ref( bCurrent );
    }
    else
    {
        // create bad state in the ring manager
        p->ddR->bFunc = Llb_BddComputeBad( p->pInit, p->ddR, p->pPars->TimeTarget );  
        if ( p->ddR->bFunc == NULL )
        {
            if ( !p->pPars->fSilent )
                printf( "Reached timeout (%d seconds) while computing bad states.\n", p->pPars->TimeLimit );
            p->pPars->iFrame = -1;
            return -1;
        }
        Cudd_Ref( p->ddR->bFunc );
        // create init state in the working and global manager
        bCurrent = Llb_CoreComputeCube( p->dd,  p->vVarsCs, 1, NULL );           Cudd_Ref( bCurrent );
        bReached = Llb_CoreComputeCube( p->ddG, p->vVarsCs, 0, NULL );           Cudd_Ref( bReached );
//Extra_bddPrint( p->dd, bCurrent );  printf( "\n" );
//Extra_bddPrint( p->ddG, bReached );  printf( "\n" );
    }

    // compute onion rings
    for ( nIters = 0; nIters < p->pPars->nIterMax; nIters++ )
    { 
        clk2 = Abc_Clock();
        // check the runtime limit
        if ( p->pPars->TimeLimit && Abc_Clock() > p->pPars->TimeTarget )
        {
            if ( !p->pPars->fSilent )
                printf( "Reached timeout (%d seconds) during image computation.\n",  p->pPars->TimeLimit );
            p->pPars->iFrame = nIters - 1;
            Cudd_RecursiveDeref( p->dd,  bCurrent );  bCurrent = NULL;
            Cudd_RecursiveDeref( p->ddG, bReached );  bReached = NULL;
            return -1;
        }

        // save the onion ring
        bTemp = Extra_TransferPermute( p->dd, p->ddR, bCurrent, pLoc2GloR );  
        if ( bTemp == NULL )
        {
            if ( !p->pPars->fSilent )
                printf( "Reached timeout (%d seconds) during image computation.\n",  p->pPars->TimeLimit );
            p->pPars->iFrame = nIters - 1;
            Cudd_RecursiveDeref( p->dd,  bCurrent );  bCurrent = NULL;
            Cudd_RecursiveDeref( p->ddG, bReached );  bReached = NULL;
            return -1;
        }
        Cudd_Ref( bTemp );
        Vec_PtrPush( p->vRings, bTemp );

        // check it for bad states
        if ( !p->pPars->fSkipOutCheck && !Cudd_bddLeq( p->ddR, bTemp, Cudd_Not(p->ddR->bFunc) ) ) 
        {
            assert( p->pInit->pSeqModel == NULL );
            if ( !p->pPars->fBackward )
                p->pInit->pSeqModel = Llb_CoreDeriveCex( p ); 
            Cudd_RecursiveDeref( p->dd,  bCurrent );  bCurrent = NULL;
            Cudd_RecursiveDeref( p->ddG, bReached );  bReached = NULL;
            if ( !p->pPars->fSilent )
            {
                if ( !p->pPars->fBackward )
                    Abc_Print( 1, "Output %d of miter \"%s\" was asserted in frame %d.  ", p->pInit->pSeqModel->iPo, p->pInit->pName, nIters );
                else
                    Abc_Print( 1, "Output ??? was asserted in frame %d (counter-example is not produced).  ", nIters );
                Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
            }
            p->pPars->iFrame = nIters - 1;
            return 0;
        }

        // compute the next states
        bNext = Llb_ImgComputeImage( p->pAig, p->vDdMans, p->dd, bCurrent, 
            vQuant0, vQuant1, p->vDriRefs, p->pPars->TimeTarget, 
            p->pPars->fBackward, p->pPars->fReorder, p->pPars->fVeryVerbose );
        if ( bNext == NULL )
        {
            if ( !p->pPars->fSilent )
                printf( "Reached timeout (%d seconds) during image computation.\n",  p->pPars->TimeLimit );
            p->pPars->iFrame = nIters - 1;
            Cudd_RecursiveDeref( p->dd,  bCurrent );   bCurrent = NULL;
            Cudd_RecursiveDeref( p->ddG, bReached );   bReached = NULL;
            return -1;
        }
        Cudd_Ref( bNext );
        Cudd_RecursiveDeref( p->dd, bCurrent );        bCurrent = NULL;
//Extra_bddPrintSupport( p->dd, bNext ); printf( "\n" );

        // remap these states into the global manager
//        bNext = Extra_TransferPermute( p->dd, p->ddG, bTemp = bNext, pLoc2Glo );    Cudd_Ref( bNext );
//        Cudd_RecursiveDeref( p->dd, bTemp );

//        bNext = Extra_TransferPermuteTime( p->dd, p->ddG, bTemp = bNext, pLoc2Glo, p->pPars->TimeTarget );    
        bNext = Extra_TransferPermute( p->dd, p->ddG, bTemp = bNext, pLoc2Glo );    
        if ( bNext == NULL )
        {
            if ( !p->pPars->fSilent )
                printf( "Reached timeout (%d seconds) during image computation in transfer 1.\n",  p->pPars->TimeLimit );
            p->pPars->iFrame = nIters - 1;
            Cudd_RecursiveDeref( p->dd,  bTemp );  
            Cudd_RecursiveDeref( p->ddG, bReached );   bReached = NULL;
            return -1;
        }
        Cudd_Ref( bNext );
        Cudd_RecursiveDeref( p->dd, bTemp );  

        nBddSize = Cudd_DagSize(bNext);
        // check if there are any new states
        if ( Cudd_bddLeq( p->ddG, bNext, bReached ) ) // implication = no new states
        {
            Cudd_RecursiveDeref( p->ddG,  bNext );     bNext = NULL;
            break;
        }

        // get the new states
        bCurrent = Cudd_bddAnd( p->ddG, bNext, Cudd_Not(bReached) );                    
        if ( bCurrent == NULL )
        {
            if ( !p->pPars->fSilent )
                printf( "Reached timeout (%d seconds) during image computation in transfer 2.\n",  p->pPars->TimeLimit );
            p->pPars->iFrame = nIters - 1;
            Cudd_RecursiveDeref( p->ddG, bNext );  
            Cudd_RecursiveDeref( p->ddG, bReached );   bReached = NULL;
            return -1;
        }
        Cudd_Ref( bCurrent );

        // remap these states into the current state vars
//        bCurrent = Extra_TransferPermute( p->ddG, p->dd, bTemp = bCurrent, pGlo2Loc );   Cudd_Ref( bCurrent );
//        Cudd_RecursiveDeref( p->ddG, bTemp );

//        bCurrent = Extra_TransferPermuteTime( p->ddG, p->dd, bTemp = bCurrent, pGlo2Loc, p->pPars->TimeTarget );    
        bCurrent = Extra_TransferPermute( p->ddG, p->dd, bTemp = bCurrent, pGlo2Loc );    
        if ( bCurrent == NULL )
        {
            if ( !p->pPars->fSilent )
                printf( "Reached timeout (%d seconds) during image computation in transfer 2.\n",  p->pPars->TimeLimit );
            p->pPars->iFrame = nIters - 1;
            Cudd_RecursiveDeref( p->ddG, bTemp );  
            Cudd_RecursiveDeref( p->ddG, bReached );   bReached = NULL;
            return -1;
        }
        Cudd_Ref( bCurrent );
        Cudd_RecursiveDeref( p->ddG, bTemp );

        // add to the reached states
        bReached = Cudd_bddOr( p->ddG, bTemp = bReached, bNext );                       Cudd_Ref( bReached );
        Cudd_RecursiveDeref( p->ddG, bTemp );
        Cudd_RecursiveDeref( p->ddG, bNext );
        bNext = NULL;

        if ( p->pPars->fVeryVerbose )
        {
            double nMints = Cudd_CountMinterm(p->ddG, bReached, Saig_ManRegNum(p->pAig) );
//            Extra_bddPrint( p->ddG, bReached );printf( "\n" );
            fprintf( stdout, "        Reachable states = %.0f. (Ratio = %.4f %%)\n", nMints, 100.0*nMints/pow(2.0, Saig_ManRegNum(p->pAig)) );
            fflush( stdout ); 
        }
        if ( p->pPars->fVerbose )
        {
            fprintf( stdout, "F =%3d : ",    nIters );
            fprintf( stdout, "Image =%6d  ", nBddSize );
            fprintf( stdout, "(%4d%4d)  ", 
                Cudd_ReadReorderings(p->dd),  Cudd_ReadGarbageCollections(p->dd) );
            fprintf( stdout, "Reach =%6d  ", Cudd_DagSize(bReached) );
            fprintf( stdout, "(%4d%4d)  ", 
                Cudd_ReadReorderings(p->ddG), Cudd_ReadGarbageCollections(p->ddG) );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk2 );
        }

        // check timeframe limit
        if ( nIters == p->pPars->nIterMax - 1 )
        {
            if ( !p->pPars->fSilent )
                printf( "Reached limit on the number of timeframes (%d).\n",  p->pPars->nIterMax );
            p->pPars->iFrame = nIters;
            Cudd_RecursiveDeref( p->dd,  bCurrent );  bCurrent = NULL;
            Cudd_RecursiveDeref( p->ddG, bReached );  bReached = NULL;
            return -1;
        }
    }
    if ( bReached == NULL )
    {
        p->pPars->iFrame = nIters - 1;
        return 0; // reachable
    }
    if ( bCurrent )
        Cudd_RecursiveDeref( p->dd, bCurrent );
    // report the stats
    if ( p->pPars->fVerbose )
    {
        double nMints = Cudd_CountMinterm(p->ddG, bReached, Saig_ManRegNum(p->pAig) );
        if ( nIters >= p->pPars->nIterMax )
            fprintf( stdout, "Reachability analysis is stopped after %d frames.\n", nIters );
        else
            fprintf( stdout, "Reachability analysis completed after %d frames.\n", nIters );
        fprintf( stdout, "Reachable states = %.0f. (Ratio = %.4f %%)\n", nMints, 100.0*nMints/pow(2.0, Saig_ManRegNum(p->pAig)) );
        fflush( stdout ); 
    }
    if ( p->pPars->fDumpReached )
    {
        Llb_ManDumpReached( p->ddG, bReached, p->pAig->pName, "reached.blif" );
        printf( "Reached states with %d BDD nodes are dumpted into file \"reached.blif\".\n", Cudd_DagSize(bReached) );
    }
    Cudd_RecursiveDeref( p->ddG, bReached );
    if ( nIters >= p->pPars->nIterMax )
    {
        if ( !p->pPars->fSilent )
        {
            printf( "Verified only for states reachable in %d frames.  ", nIters );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        }
        p->pPars->iFrame = p->pPars->nIterMax;
        return -1; // undecided
    }
    if ( !p->pPars->fSilent )
    {
        printf( "The miter is proved unreachable after %d iterations.  ", nIters );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    p->pPars->iFrame = nIters - 1;
    return 1; // unreachable
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_CoreReachability( Llb_Img_t * p )
{
    Vec_Ptr_t * vSupps, * vQuant0, * vQuant1;
    int RetValue;
    // get supports and quantified variables
    if ( p->pPars->fBackward )
    {
        Vec_PtrReverseOrder( p->vDdMans );
        vSupps = Llb_ImgSupports( p->pAig, p->vDdMans, p->vVarsNs, p->vVarsCs, 0, p->pPars->fVeryVerbose );
    }
    else
        vSupps = Llb_ImgSupports( p->pAig, p->vDdMans, p->vVarsCs, p->vVarsNs, 0, p->pPars->fVeryVerbose );
    Llb_ImgSchedule( vSupps, &vQuant0, &vQuant1, p->pPars->fVeryVerbose );
    Vec_VecFree( (Vec_Vec_t *)vSupps );
    // remove variables
    Llb_ImgQuantifyFirst( p->pAig, p->vDdMans, vQuant0, p->pPars->fVeryVerbose );
    // perform reachability
    RetValue = Llb_CoreReachability_int( p, vQuant0, vQuant1 );
    Vec_VecFree( (Vec_Vec_t *)vQuant0 );
    Vec_VecFree( (Vec_Vec_t *)vQuant1 );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_CoreConstructAll( Aig_Man_t * p, Vec_Ptr_t * vResult, Vec_Int_t * vVarsNs, abctime TimeTarget )
{
    DdManager * dd;
    Vec_Ptr_t * vDdMans;
    Vec_Ptr_t * vLower, * vUpper = NULL;
    int i;
    vDdMans = Vec_PtrStart( Vec_PtrSize(vResult) );
    Vec_PtrForEachEntryReverse( Vec_Ptr_t *, vResult, vLower, i )
    {
        if ( i < Vec_PtrSize(vResult) - 1 )
            dd = Llb_ImgPartition( p, vLower, vUpper, TimeTarget );
        else
            dd = Llb_DriverLastPartition( p, vVarsNs, TimeTarget );
        if ( dd == NULL )
        {
            Vec_PtrForEachEntry( DdManager *, vDdMans, dd, i )
            {
                if ( dd == NULL )
                    continue;
                if ( dd->bFunc )
                    Cudd_RecursiveDeref( dd, dd->bFunc );
                Extra_StopManager( dd );
            }
            Vec_PtrFree( vDdMans );
            return NULL;
        }
        Vec_PtrWriteEntry( vDdMans, i, dd );
        vUpper = vLower;
    }
    return vDdMans;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_CoreSetVarMaps( Llb_Img_t * p )
{
    Aig_Obj_t * pObj;
    int i, iVarCs, iVarNs;
    assert( p->vVarsCs != NULL );
    assert( p->vVarsNs != NULL );
    assert( p->vCs2Glo == NULL );
    assert( p->vNs2Glo == NULL );
    assert( p->vGlo2Cs == NULL );
    assert( p->vGlo2Ns == NULL );
    p->vCs2Glo = Vec_IntStartFull( Aig_ManObjNumMax(p->pAig) );
    p->vNs2Glo = Vec_IntStartFull( Aig_ManObjNumMax(p->pAig) );
    p->vGlo2Cs = Vec_IntStartFull( Aig_ManRegNum(p->pAig) );
    p->vGlo2Ns = Vec_IntStartFull( Aig_ManRegNum(p->pAig) );
    for ( i = 0; i < Aig_ManRegNum(p->pAig); i++ )
    {
        iVarCs = Vec_IntEntry( p->vVarsCs, i );
        iVarNs = Vec_IntEntry( p->vVarsNs, i );
        assert( iVarCs >= 0 && iVarCs < Aig_ManObjNumMax(p->pAig) );
        assert( iVarNs >= 0 && iVarNs < Aig_ManObjNumMax(p->pAig) );
        Vec_IntWriteEntry( p->vCs2Glo, iVarCs, i );
        Vec_IntWriteEntry( p->vNs2Glo, iVarNs, i );
        Vec_IntWriteEntry( p->vGlo2Cs, i, iVarCs );
        Vec_IntWriteEntry( p->vGlo2Ns, i, iVarNs );
    }
    // add mapping of the PIs
    Saig_ManForEachPi( p->pAig, pObj, i )
        Vec_IntWriteEntry( p->vCs2Glo, Aig_ObjId(pObj), Aig_ManRegNum(p->pAig)+i );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Llb_Img_t * Llb_CoreStart( Aig_Man_t * pInit, Aig_Man_t * pAig, Gia_ParLlb_t *  pPars )
{
    Llb_Img_t * p;
    p = ABC_CALLOC( Llb_Img_t, 1 );
    p->pInit = pInit;
    p->pAig  = pAig;
    p->pPars = pPars;
    p->dd    = Cudd_Init( Aig_ManObjNumMax(pAig), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    p->ddG   = Cudd_Init( Aig_ManRegNum(pAig),    0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    p->ddR   = Cudd_Init( Aig_ManCiNum(pAig),     0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_AutodynEnable( p->dd,  CUDD_REORDER_SYMM_SIFT );
    Cudd_AutodynEnable( p->ddG, CUDD_REORDER_SYMM_SIFT );
    Cudd_AutodynEnable( p->ddR, CUDD_REORDER_SYMM_SIFT );
    p->vRings = Vec_PtrAlloc( 100 );
    p->vDriRefs = Llb_DriverCountRefs( pAig );
    p->vVarsCs  = Llb_DriverCollectCs( pAig );
    p->vVarsNs  = Llb_DriverCollectNs( pAig, p->vDriRefs );
    Llb_CoreSetVarMaps( p );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_CoreStop( Llb_Img_t * p )
{
    DdManager * dd;
    DdNode * bTemp;
    int i;
    if ( p->vDdMans )
    Vec_PtrForEachEntry( DdManager *, p->vDdMans, dd, i )
    {
        if ( dd->bFunc )
            Cudd_RecursiveDeref( dd, dd->bFunc );
        if ( dd->bFunc2 )
            Cudd_RecursiveDeref( dd, dd->bFunc2 );
        Extra_StopManager( dd );
    }
    Vec_PtrFreeP( &p->vDdMans );
    if ( p->ddR->bFunc )
        Cudd_RecursiveDeref( p->ddR, p->ddR->bFunc );
    Vec_PtrForEachEntry( DdNode *, p->vRings, bTemp, i )
        Cudd_RecursiveDeref( p->ddR, bTemp );
    Vec_PtrFree( p->vRings );
    Extra_StopManager( p->dd );
    Extra_StopManager( p->ddG );
    Extra_StopManager( p->ddR );
    Vec_IntFreeP( &p->vDriRefs );
    Vec_IntFreeP( &p->vVarsCs );
    Vec_IntFreeP( &p->vVarsNs );
    Vec_IntFreeP( &p->vCs2Glo );
    Vec_IntFreeP( &p->vNs2Glo );
    Vec_IntFreeP( &p->vGlo2Cs );
    Vec_IntFreeP( &p->vGlo2Ns );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_CoreExperiment( Aig_Man_t * pInit, Aig_Man_t * pAig, Gia_ParLlb_t * pPars, Vec_Ptr_t * vResult, abctime TimeTarget )
{
    int RetValue;
    Llb_Img_t * p;
//    printf( "\n" );
//    pPars->fVerbose = 1;
    p = Llb_CoreStart( pInit, pAig, pPars );
    p->vDdMans = Llb_CoreConstructAll( pAig, vResult, p->vVarsNs, TimeTarget );
    if ( p->vDdMans == NULL )
    {
        if ( !pPars->fSilent )
            printf( "Reached timeout (%d seconds) while deriving the partitions.\n", pPars->TimeLimit );
        Llb_CoreStop( p );
        return -1;
    }
    RetValue = Llb_CoreReachability( p );
    Llb_CoreStop( p );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Finds balanced cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManReachMinCut( Aig_Man_t * pAig, Gia_ParLlb_t * pPars )
{
    extern Vec_Ptr_t * Llb_ManComputeCuts( Aig_Man_t * p, int Num, int fVerbose, int fVeryVerbose );
    Vec_Ptr_t * vResult;
    Aig_Man_t * p;
    int RetValue = -1;
    abctime clk = Abc_Clock();

    // compute time to stop
    pPars->TimeTarget = pPars->TimeLimit ? pPars->TimeLimit * CLOCKS_PER_SEC + Abc_Clock(): 0;

    p = Aig_ManDupFlopsOnly( pAig );
//Aig_ManShow( p, 0, NULL );
    if ( pPars->fVerbose )
    Aig_ManPrintStats( pAig );
    if ( pPars->fVerbose )
    Aig_ManPrintStats( p );
    Aig_ManFanoutStart( p );

    vResult = Llb_ManComputeCuts( p, pPars->nPartValue, pPars->fVerbose, pPars->fVeryVerbose );

    if ( pPars->TimeLimit && Abc_Clock() > pPars->TimeTarget )
    {
        if ( !pPars->fSilent )
            printf( "Reached timeout (%d seconds) after partitioning.\n", pPars->TimeLimit );

        Vec_VecFree( (Vec_Vec_t *)vResult );
        Aig_ManFanoutStop( p );
        Aig_ManCleanMarkAB( p );
        Aig_ManStop( p );
        return RetValue;
    }

    if ( !pPars->fSkipReach )
        RetValue = Llb_CoreExperiment( pAig, p, pPars, vResult, pPars->TimeTarget );

    Vec_VecFree( (Vec_Vec_t *)vResult );
    Aig_ManFanoutStop( p );
    Aig_ManCleanMarkAB( p );
    Aig_ManStop( p );

    if ( RetValue == -1 )
        Abc_PrintTime( 1, "Total runtime of the min-cut-based reachability engine", Abc_Clock() - clk );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

