/**CFile****************************************************************

  FileName    [saigAbsStart.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Counter-example-based abstraction.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigAbsStart.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abs.h"
#include "proof/ssw/ssw.h"
#include "proof/fra/fra.h"
#include "bdd/bbr/bbr.h"
#include "proof/pdr/pdr.h"
#include "sat/bmc/bmc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManAbsSetDefaultParams( Gia_ParAbs_t * p )
{
    memset( p, 0, sizeof(Gia_ParAbs_t) );
    p->Algo        =        0;    // algorithm: CBA
    p->nFramesMax  =       10;    // timeframes for PBA
    p->nConfMax    =    10000;    // conflicts for PBA
    p->fDynamic    =        1;    // dynamic unfolding for PBA
    p->fConstr     =        0;    // use constraints
    p->nFramesBmc  =      250;    // timeframes for BMC
    p->nConfMaxBmc =     5000;    // conflicts for BMC
    p->nStableMax  =  1000000;    // the number of stable frames to quit
    p->nRatio      =       10;    // ratio of flops to quit
    p->nBobPar     =  1000000;    // the number of frames before trying to quit
    p->fUseBdds    =        0;    // use BDDs to refine abstraction
    p->fUseDprove  =        0;    // use 'dprove' to refine abstraction
    p->fUseStart   =        1;    // use starting frame
    p->fVerbose    =        0;    // verbose output
    p->fVeryVerbose=        0;    // printing additional information
    p->Status      =       -1;    // the problem status
    p->nFramesDone =       -1;    // the number of rames covered
}


/**Function*************************************************************

  Synopsis    [Derive a new counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Saig_ManCexRemap( Aig_Man_t * p, Aig_Man_t * pAbs, Abc_Cex_t * pCexAbs )
{
    Abc_Cex_t * pCex;
    Aig_Obj_t * pObj;
    int i, f;
    if ( !Saig_ManVerifyCex( pAbs, pCexAbs ) )
        printf( "Saig_ManCexRemap(): The initial counter-example is invalid.\n" );
//    else
//        printf( "Saig_ManCexRemap(): The initial counter-example is correct.\n" );
    // start the counter-example
    pCex = Abc_CexAlloc( Aig_ManRegNum(p), Saig_ManPiNum(p), pCexAbs->iFrame+1 );
    pCex->iFrame = pCexAbs->iFrame;
    pCex->iPo    = pCexAbs->iPo;
    // copy the bit data
    for ( f = 0; f <= pCexAbs->iFrame; f++ )
    {
        Saig_ManForEachPi( pAbs, pObj, i )
        {
            if ( i == Saig_ManPiNum(p) )
                break;
            if ( Abc_InfoHasBit( pCexAbs->pData, pCexAbs->nRegs + pCexAbs->nPis * f + i ) )
                Abc_InfoSetBit( pCex->pData, pCex->nRegs + pCex->nPis * f + i );
        }
    }
    // verify the counter example
    if ( !Saig_ManVerifyCex( p, pCex ) )
    {
        printf( "Saig_ManCexRemap(): Counter-example is invalid.\n" );
        Abc_CexFree( pCex );
        pCex = NULL;
    }
    else
    {
        Abc_Print( 1, "Counter-example verification is successful.\n" );
        Abc_Print( 1, "Output %d of miter \"%s\" was asserted in frame %d. \n", pCex->iPo, p->pName, pCex->iFrame );
    }
    return pCex;
}

/**Function*************************************************************

  Synopsis    [Find the first PI corresponding to the flop.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManCexFirstFlopPi( Aig_Man_t * p, Aig_Man_t * pAbs )
{ 
    Aig_Obj_t * pObj;
    int i;
    assert( pAbs->vCiNumsOrig != NULL );
    Aig_ManForEachCi( p, pObj, i )
    {
        if ( Vec_IntEntry(pAbs->vCiNumsOrig, i) >= Saig_ManPiNum(p) )
            return i;
    }
    return -1;
}

/**Function*************************************************************

  Synopsis    [Refines abstraction using one step.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManCexRefine( Aig_Man_t * p, Aig_Man_t * pAbs, Vec_Int_t * vFlops, int nFrames, int nConfMaxOne, int fUseBdds, int fUseDprove, int fVerbose, int * pnUseStart, int * piRetValue, int * pnFrames )
{ 
    Vec_Int_t * vFlopsNew;
    int i, Entry, RetValue;
    *piRetValue = -1;
    if ( fUseDprove && Aig_ManRegNum(pAbs) > 0 )
    {
/*
        Fra_Sec_t SecPar, * pSecPar = &SecPar;
        Fra_SecSetDefaultParams( pSecPar );
        pSecPar->fVerbose       = fVerbose;
        RetValue = Fra_FraigSec( pAbs, pSecPar, NULL );
*/
        Aig_Man_t * pAbsOrpos = Saig_ManDupOrpos( pAbs );
        Pdr_Par_t Pars, * pPars = &Pars;
        Pdr_ManSetDefaultParams( pPars );
        pPars->nTimeOut = 10;
        pPars->fVerbose = fVerbose;
        if ( pPars->fVerbose )
            printf( "Running property directed reachability...\n" );
        RetValue = Pdr_ManSolve( pAbsOrpos, pPars );
        if ( pAbsOrpos->pSeqModel )
            pAbsOrpos->pSeqModel->iPo = Saig_ManFindFailedPoCex( pAbs, pAbsOrpos->pSeqModel );
        pAbs->pSeqModel = pAbsOrpos->pSeqModel;
        pAbsOrpos->pSeqModel = NULL;
        Aig_ManStop( pAbsOrpos );
        if ( RetValue )
            *piRetValue = 1;

    }
    else if ( fUseBdds && (Aig_ManRegNum(pAbs) > 0 && Aig_ManRegNum(pAbs) <= 80) )
    {
        Saig_ParBbr_t Pars, * pPars = &Pars;
        Bbr_ManSetDefaultParams( pPars );
        pPars->TimeLimit     = 0;
        pPars->nBddMax       = 1000000;
        pPars->nIterMax      = nFrames;
        pPars->fPartition    = 1;
        pPars->fReorder      = 1;
        pPars->fReorderImage = 1;
        pPars->fVerbose      = fVerbose;
        pPars->fSilent       = 0;
        RetValue = Aig_ManVerifyUsingBdds( pAbs, pPars );
        if ( RetValue )
            *piRetValue = 1;
    }
    else 
    {
        Saig_BmcPerform( pAbs, pnUseStart? *pnUseStart: 0, nFrames, 2000, 0, nConfMaxOne, 0, fVerbose, 0, pnFrames, 0, 0 );
    }
    if ( pAbs->pSeqModel == NULL )
        return NULL;
    if ( pnUseStart )
        *pnUseStart = pAbs->pSeqModel->iFrame;
//    vFlopsNew = Saig_ManExtendCounterExampleTest( pAbs, Saig_ManCexFirstFlopPi(p, pAbs), pAbs->pSeqModel, 1, fVerbose );
    vFlopsNew = Saig_ManExtendCounterExampleTest3( pAbs, Saig_ManCexFirstFlopPi(p, pAbs), pAbs->pSeqModel, fVerbose );
    if ( vFlopsNew == NULL )
        return NULL;
    if ( Vec_IntSize(vFlopsNew) == 0 )
    {
        printf( "Discovered a true counter-example!\n" );
        p->pSeqModel = Saig_ManCexRemap( p, pAbs, pAbs->pSeqModel );
        Vec_IntFree( vFlopsNew );
        *piRetValue = 0;
        return NULL;
    }
    // vFlopsNew contains PI numbers that should be kept in pAbs
    if ( fVerbose )
        printf( "Adding %d registers to the abstraction (total = %d).\n\n", Vec_IntSize(vFlopsNew), Aig_ManRegNum(pAbs)+Vec_IntSize(vFlopsNew) );
    // add to the abstraction
    Vec_IntForEachEntry( vFlopsNew, Entry, i )
    {
        Entry = Vec_IntEntry(pAbs->vCiNumsOrig, Entry);
        assert( Entry >= Saig_ManPiNum(p) );
        assert( Entry < Aig_ManCiNum(p) );
        Vec_IntPush( vFlops, Entry-Saig_ManPiNum(p) );
    }
    Vec_IntFree( vFlopsNew );

    Vec_IntSort( vFlops, 0 );
    Vec_IntForEachEntryStart( vFlops, Entry, i, 1 )
        assert( Vec_IntEntry(vFlops, i-1) != Entry );

    return Saig_ManDupAbstraction( p, vFlops );
}
 
/**Function*************************************************************

  Synopsis    [Refines abstraction using one step.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManCexRefineStep( Aig_Man_t * p, Vec_Int_t * vFlops, Vec_Int_t * vFlopClasses, Abc_Cex_t * pCex, int nFfToAddMax, int fTryFour, int fSensePath, int fVerbose )
{
    Aig_Man_t * pAbs;
    Vec_Int_t * vFlopsNew;
    int i, Entry;
    abctime clk = Abc_Clock();
    pAbs = Saig_ManDupAbstraction( p, vFlops );
    if ( fSensePath )
        vFlopsNew = Saig_ManExtendCounterExampleTest2( pAbs, Saig_ManCexFirstFlopPi(p, pAbs), pCex, fVerbose );
    else
//        vFlopsNew = Saig_ManExtendCounterExampleTest( pAbs, Saig_ManCexFirstFlopPi(p, pAbs), pCex, fTryFour, fVerbose );
        vFlopsNew = Saig_ManExtendCounterExampleTest3( pAbs, Saig_ManCexFirstFlopPi(p, pAbs), pCex, fVerbose );
    if ( vFlopsNew == NULL )
    {
        Aig_ManStop( pAbs );
        return 0;
    }
    if ( Vec_IntSize(vFlopsNew) == 0 )
    {
        printf( "Refinement did not happen. Discovered a true counter-example.\n" );
        printf( "Remapping counter-example from %d to %d primary inputs.\n", Aig_ManCiNum(pAbs), Aig_ManCiNum(p) );
        p->pSeqModel = Saig_ManCexRemap( p, pAbs, pCex );
        Vec_IntFree( vFlopsNew );
        Aig_ManStop( pAbs );
        return 0;
    }
    if ( fVerbose )
    {
        printf( "Adding %d registers to the abstraction (total = %d).  ", Vec_IntSize(vFlopsNew), Aig_ManRegNum(p)+Vec_IntSize(vFlopsNew) );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    // vFlopsNew contains PI numbers that should be kept in pAbs
    // select the most useful flops among those to be added
    if ( nFfToAddMax > 0 && Vec_IntSize(vFlopsNew) > nFfToAddMax )
    {
        Vec_Int_t * vFlopsNewBest;
        // shift the indices
        Vec_IntForEachEntry( vFlopsNew, Entry, i )
            Vec_IntAddToEntry( vFlopsNew, i, -Saig_ManPiNum(p) );
        // create new flops
        vFlopsNewBest = Saig_ManCbaFilterFlops( p, pCex, vFlopClasses, vFlopsNew, nFfToAddMax );
        assert( Vec_IntSize(vFlopsNewBest) == nFfToAddMax );
        printf( "Filtering flops based on cost (%d -> %d).\n", Vec_IntSize(vFlopsNew), Vec_IntSize(vFlopsNewBest) );
        // update
        Vec_IntFree( vFlopsNew );
        vFlopsNew = vFlopsNewBest;
        // shift the indices
        Vec_IntForEachEntry( vFlopsNew, Entry, i )
            Vec_IntAddToEntry( vFlopsNew, i, Saig_ManPiNum(p) );
    }
    // add to the abstraction
    Vec_IntForEachEntry( vFlopsNew, Entry, i )
    {
        Entry = Vec_IntEntry(pAbs->vCiNumsOrig, Entry);
        assert( Entry >= Saig_ManPiNum(p) );
        assert( Entry < Aig_ManCiNum(p) );
        Vec_IntPush( vFlops, Entry-Saig_ManPiNum(p) );
    }
    Vec_IntFree( vFlopsNew );
    Aig_ManStop( pAbs );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Transform flop map into flop list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManClasses2Flops( Vec_Int_t * vFlopClasses )
{
    Vec_Int_t * vFlops;
    int i, Entry;
    vFlops = Vec_IntAlloc( 100 );
    Vec_IntForEachEntry( vFlopClasses, Entry, i )
        if ( Entry )
            Vec_IntPush( vFlops, i );
    return vFlops;
}

/**Function*************************************************************

  Synopsis    [Transform flop list into flop map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManFlops2Classes( Gia_Man_t * pGia, Vec_Int_t * vFlops )
{
    Vec_Int_t * vFlopClasses;
    int i, Entry;
    vFlopClasses = Vec_IntStart( Gia_ManRegNum(pGia) );
    Vec_IntForEachEntry( vFlops, Entry, i )
        Vec_IntWriteEntry( vFlopClasses, Entry, 1 );
    return vFlopClasses;
}

/**Function*************************************************************

  Synopsis    [Refines abstraction using the latch map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCexAbstractionRefine( Gia_Man_t * pGia, Abc_Cex_t * pCex, int nFfToAddMax, int fTryFour, int fSensePath, int fVerbose )
{
    Aig_Man_t * pNew;
    Vec_Int_t * vFlops;
    if ( pGia->vFlopClasses == NULL )
    {
        printf( "Gia_ManCexAbstractionRefine(): Abstraction latch map is missing.\n" );
        return -1;
    }
    pNew = Gia_ManToAig( pGia, 0 );
    vFlops = Gia_ManClasses2Flops( pGia->vFlopClasses );
    if ( !Saig_ManCexRefineStep( pNew, vFlops, pGia->vFlopClasses, pCex, nFfToAddMax, fTryFour, fSensePath, fVerbose ) )
    {
        pGia->pCexSeq = pNew->pSeqModel; pNew->pSeqModel = NULL;
        Vec_IntFree( vFlops );
        Aig_ManStop( pNew );
        return 0;
    }
    Vec_IntFree( pGia->vFlopClasses );
    pGia->vFlopClasses = Gia_ManFlops2Classes( pGia, vFlops );
    Vec_IntFree( vFlops );
    Aig_ManStop( pNew );
    return -1;
}


/**Function*************************************************************

  Synopsis    [Computes the flops to remain after abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
Vec_Int_t * Saig_ManCexAbstractionFlops( Aig_Man_t * p, Gia_ParAbs_t * pPars )
{
    int nUseStart = 0;
    Aig_Man_t * pAbs, * pTemp;
    Vec_Int_t * vFlops;
    int Iter;//, clk = Abc_Clock(), clk2 = Abc_Clock();//, iFlop;
    assert( Aig_ManRegNum(p) > 0 );
    if ( pPars->fVerbose )
        printf( "Performing counter-example-based refinement.\n" );
    Aig_ManSetCioIds( p );
    vFlops = Vec_IntStartNatural( 1 );
/*
    iFlop = Saig_ManFindFirstFlop( p );
    assert( iFlop >= 0 );
    vFlops = Vec_IntAlloc( 1 );
    Vec_IntPush( vFlops, iFlop );
*/
    // create the resulting AIG
    pAbs = Saig_ManDupAbstraction( p, vFlops );
    if ( !pPars->fVerbose )
    {
        printf( "Init : " );
        Aig_ManPrintStats( pAbs );
    }
    printf( "Refining abstraction...\n" );
    for ( Iter = 0; ; Iter++ )
    {
        pTemp = Saig_ManCexRefine( p, pAbs, vFlops, pPars->nFramesBmc, pPars->nConfMaxBmc, pPars->fUseBdds, pPars->fUseDprove, pPars->fVerbose, pPars->fUseStart?&nUseStart:NULL, &pPars->Status, &pPars->nFramesDone );
        if ( pTemp == NULL )
        {
            ABC_FREE( p->pSeqModel );
            p->pSeqModel = pAbs->pSeqModel;
            pAbs->pSeqModel = NULL;
            Aig_ManStop( pAbs );
            break;
        }
        Aig_ManStop( pAbs );
        pAbs = pTemp;
        printf( "ITER %4d : ", Iter );
        if ( !pPars->fVerbose )
            Aig_ManPrintStats( pAbs );
        // output the intermediate result of abstraction
        Ioa_WriteAiger( pAbs, "gabs.aig", 0, 0 );
//            printf( "Intermediate abstracted model was written into file \"%s\".\n", "gabs.aig" );
        // check if the ratio is reached
        if ( 100.0*(Aig_ManRegNum(p)-Aig_ManRegNum(pAbs))/Aig_ManRegNum(p) < 1.0*pPars->nRatio )
        {
            printf( "Refinements is stopped because flop reduction is less than %d%%\n", pPars->nRatio );
            Aig_ManStop( pAbs );
            pAbs = NULL;
            Vec_IntFree( vFlops );
            vFlops = NULL;
            break;
        }
    }
    return vFlops;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

