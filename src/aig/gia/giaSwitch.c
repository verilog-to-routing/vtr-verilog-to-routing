/**CFile****************************************************************

  FileName    [giaSwitch.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Computing switching activity.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaSwitch.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "giaAig.h"
#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// switching estimation parameters
typedef struct Gia_ParSwi_t_ Gia_ParSwi_t;
struct Gia_ParSwi_t_
{
    // user-controlled parameters
    int            nWords;       // the number of machine words
    int            nIters;       // the number of timeframes
    int            nPref;        // the number of first timeframes to skip
    int            nRandPiFactor;   // PI trans prob (-1=3/8; 0=1/2; 1=1/4; 2=1/8, etc)
    int            fProbOne;     // collect probability of one
    int            fProbTrans;   // collect probatility of Swiing
    int            fVerbose;     // enables verbose output
};

typedef struct Gia_ManSwi_t_ Gia_ManSwi_t;
struct Gia_ManSwi_t_
{
    Gia_Man_t *    pAig;
    Gia_ParSwi_t * pPars; 
    int            nWords;
    // simulation information
    unsigned *     pDataSim;     // simulation data
    unsigned *     pDataSimCis;  // simulation data for CIs
    unsigned *     pDataSimCos;  // simulation data for COs
    int *          pData1;       // switching data
};

static inline unsigned * Gia_SwiData( Gia_ManSwi_t * p, int i )    { return p->pDataSim + i * p->nWords;    }
static inline unsigned * Gia_SwiDataCi( Gia_ManSwi_t * p, int i )  { return p->pDataSimCis + i * p->nWords; }
static inline unsigned * Gia_SwiDataCo( Gia_ManSwi_t * p, int i )  { return p->pDataSimCos + i * p->nWords; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSetDefaultParamsSwi( Gia_ParSwi_t * p )
{
    memset( p, 0, sizeof(Gia_ParSwi_t) );
    p->nWords        =  10;  // the number of machine words of simulatation data 
    p->nIters        =  48;  // the number of all timeframes to simulate
    p->nPref         =  16;  // the number of first timeframes to skip when computing switching
    p->nRandPiFactor =   0;  // primary input transition probability (-1=3/8; 0=1/2; 1=1/4; 2=1/8, etc)
    p->fProbOne      =   0;  // compute probability of signal being one (if 0, compute probability of switching)
    p->fProbTrans    =   1;  // compute signal transition probability (if 0, compute transition probability using probability of being one)
    p->fVerbose      =   0;  // enables verbose output
}

/**Function*************************************************************

  Synopsis    [Creates fast simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_ManSwi_t * Gia_ManSwiCreate( Gia_Man_t * pAig, Gia_ParSwi_t * pPars )
{
    Gia_ManSwi_t * p;
    p = ABC_ALLOC( Gia_ManSwi_t, 1 );
    memset( p, 0, sizeof(Gia_ManSwi_t) );
    p->pAig   = Gia_ManFront( pAig );
    p->pPars  = pPars;
    p->nWords = pPars->nWords;
    p->pDataSim = ABC_ALLOC( unsigned, p->nWords * p->pAig->nFront );
    p->pDataSimCis = ABC_ALLOC( unsigned, p->nWords * Gia_ManCiNum(p->pAig) );
    p->pDataSimCos = ABC_ALLOC( unsigned, p->nWords * Gia_ManCoNum(p->pAig) );
    p->pData1 = ABC_CALLOC( int, Gia_ManObjNum(pAig) );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSwiDelete( Gia_ManSwi_t * p )
{
    Gia_ManStop( p->pAig );
    ABC_FREE( p->pData1 );
    ABC_FREE( p->pDataSim );
    ABC_FREE( p->pDataSimCis );
    ABC_FREE( p->pDataSimCos );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSwiSimInfoRandom( Gia_ManSwi_t * p, unsigned * pInfo, int nProbNum )
{
    unsigned Mask = 0;
    int w, i;
    if ( nProbNum == -1 )
    { // 3/8 = 1/4 + 1/8
        Mask = (Gia_ManRandom( 0 ) & Gia_ManRandom( 0 )) | 
               (Gia_ManRandom( 0 ) & Gia_ManRandom( 0 ) & Gia_ManRandom( 0 ));
        for ( w = p->nWords-1; w >= 0; w-- )
            pInfo[w] ^= Mask;
    }
    else if ( nProbNum > 0 )
    {
        Mask = Gia_ManRandom( 0 );
        for ( i = 0; i < nProbNum; i++ )
            Mask &= Gia_ManRandom( 0 );
        for ( w = p->nWords-1; w >= 0; w-- )
            pInfo[w] ^= Mask;
    }
    else if ( nProbNum == 0 )
    {
        for ( w = p->nWords-1; w >= 0; w-- )
            pInfo[w] = Gia_ManRandom( 0 );
    }
    else
        assert( 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSwiSimInfoRandomShift( Gia_ManSwi_t * p, unsigned * pInfo, int nProbNum )
{
    unsigned Mask = 0;
    int w, i;
    if ( nProbNum == -1 )
    { // 3/8 = 1/4 + 1/8
        Mask = (Gia_ManRandom( 0 ) & Gia_ManRandom( 0 )) | 
               (Gia_ManRandom( 0 ) & Gia_ManRandom( 0 ) & Gia_ManRandom( 0 ));
    }
    else if ( nProbNum >= 0 )
    {
        Mask = Gia_ManRandom( 0 );
        for ( i = 0; i < nProbNum; i++ )
            Mask &= Gia_ManRandom( 0 );
    }
    else
        assert( 0 );
    for ( w = p->nWords-1; w >= 0; w-- )
        pInfo[w] = (pInfo[w] << 16) | ((pInfo[w] ^ Mask) & 0xffff);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSwiSimInfoZero( Gia_ManSwi_t * p, unsigned * pInfo )
{
    int w;
    for ( w = p->nWords-1; w >= 0; w-- )
        pInfo[w] = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSwiSimInfoOne( Gia_ManSwi_t * p, unsigned * pInfo )
{
    int w;
    for ( w = p->nWords-1; w >= 0; w-- )
        pInfo[w] = ~0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSwiSimInfoCopy( Gia_ManSwi_t * p, unsigned * pInfo, unsigned * pInfo0 )
{
    int w;
    for ( w = p->nWords-1; w >= 0; w-- )
        pInfo[w] = pInfo0[w];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSwiSimInfoCopyShift( Gia_ManSwi_t * p, unsigned * pInfo, unsigned * pInfo0 )
{
    int w;
    for ( w = p->nWords-1; w >= 0; w-- )
        pInfo[w] = (pInfo[w] << 16) | (pInfo0[w] & 0xffff);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSwiSimulateCi( Gia_ManSwi_t * p, Gia_Obj_t * pObj, int iCi )
{
    unsigned * pInfo  = Gia_SwiData( p, Gia_ObjValue(pObj) );
    unsigned * pInfo0 = Gia_SwiDataCi( p, iCi );
    int w;
    for ( w = p->nWords-1; w >= 0; w-- )
        pInfo[w] = pInfo0[w];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSwiSimulateCo( Gia_ManSwi_t * p, int iCo, Gia_Obj_t * pObj )
{
    unsigned * pInfo  = Gia_SwiDataCo( p, iCo );
    unsigned * pInfo0 = Gia_SwiData( p, Gia_ObjDiff0(pObj) );
    int w;
    if ( Gia_ObjFaninC0(pObj) )
        for ( w = p->nWords-1; w >= 0; w-- )
            pInfo[w] = ~pInfo0[w];
    else 
        for ( w = p->nWords-1; w >= 0; w-- )
            pInfo[w] = pInfo0[w];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSwiSimulateNode( Gia_ManSwi_t * p, Gia_Obj_t * pObj )
{
    unsigned * pInfo  = Gia_SwiData( p, Gia_ObjValue(pObj) );
    unsigned * pInfo0 = Gia_SwiData( p, Gia_ObjDiff0(pObj) );
    unsigned * pInfo1 = Gia_SwiData( p, Gia_ObjDiff1(pObj) );
    int w;
    if ( Gia_ObjFaninC0(pObj) )
    {
        if (  Gia_ObjFaninC1(pObj) )
            for ( w = p->nWords-1; w >= 0; w-- )
                pInfo[w] = ~(pInfo0[w] | pInfo1[w]);
        else 
            for ( w = p->nWords-1; w >= 0; w-- )
                pInfo[w] = ~pInfo0[w] & pInfo1[w];
    }
    else 
    {
        if (  Gia_ObjFaninC1(pObj) )
            for ( w = p->nWords-1; w >= 0; w-- )
                pInfo[w] = pInfo0[w] & ~pInfo1[w];
        else 
            for ( w = p->nWords-1; w >= 0; w-- )
                pInfo[w] = pInfo0[w] & pInfo1[w];
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSwiSimInfoInit( Gia_ManSwi_t * p )
{
    int i = 0;
    for ( ; i < Gia_ManPiNum(p->pAig); i++ )
        Gia_ManSwiSimInfoRandom( p, Gia_SwiDataCi(p, i), 0 );
    for ( ; i < Gia_ManCiNum(p->pAig); i++ )
        Gia_ManSwiSimInfoZero( p, Gia_SwiDataCi(p, i) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSwiSimInfoTransfer( Gia_ManSwi_t * p, int nProbNum )
{
    int i = 0, nShift = Gia_ManPoNum(p->pAig)-Gia_ManPiNum(p->pAig);
    for ( ; i < Gia_ManPiNum(p->pAig); i++ )
        Gia_ManSwiSimInfoRandom( p, Gia_SwiDataCi(p, i), nProbNum );
    for ( ; i < Gia_ManCiNum(p->pAig); i++ )
        Gia_ManSwiSimInfoCopy( p, Gia_SwiDataCi(p, i), Gia_SwiDataCo(p, nShift+i) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSwiSimInfoTransferShift( Gia_ManSwi_t * p, int nProbNum )
{
    int i = 0, nShift = Gia_ManPoNum(p->pAig)-Gia_ManPiNum(p->pAig);
    for ( ; i < Gia_ManPiNum(p->pAig); i++ )
        Gia_ManSwiSimInfoRandomShift( p, Gia_SwiDataCi(p, i), nProbNum );
    for ( ; i < Gia_ManCiNum(p->pAig); i++ )
        Gia_ManSwiSimInfoCopyShift( p, Gia_SwiDataCi(p, i), Gia_SwiDataCo(p, nShift+i) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManSwiSimInfoCountOnes( Gia_ManSwi_t * p, int iPlace )
{
    unsigned * pInfo;
    int w, Counter = 0;
    pInfo = Gia_SwiData( p, iPlace );
    for ( w = p->nWords-1; w >= 0; w-- )
        Counter += Gia_WordCountOnes( pInfo[w] );
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManSwiSimInfoCountTrans( Gia_ManSwi_t * p, int iPlace )
{
    unsigned * pInfo;
    int w, Counter = 0;
    pInfo = Gia_SwiData( p, iPlace );
    for ( w = p->nWords-1; w >= 0; w-- )
        Counter += 2*Gia_WordCountOnes( (pInfo[w] ^ (pInfo[w] >> 16)) & 0xffff );
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSwiSimulateRound( Gia_ManSwi_t * p, int fCount )
{
    Gia_Obj_t * pObj;
    int i;//, iCis = 0, iCos = 0;
    assert( p->pAig->nFront > 0 );
    assert( Gia_ManConst0(p->pAig)->Value == 0 );
    Gia_ManSwiSimInfoZero( p, Gia_SwiData(p, 0) );
    Gia_ManForEachObj1( p->pAig, pObj, i )
    {
        if ( Gia_ObjIsAndOrConst0(pObj) )
        {
            assert( Gia_ObjValue(pObj) < p->pAig->nFront );
            Gia_ManSwiSimulateNode( p, pObj );
        }
        else if ( Gia_ObjIsCo(pObj) )
        {
            assert( Gia_ObjValue(pObj) == GIA_NONE );
//            Gia_ManSwiSimulateCo( p, iCos++, pObj );
            Gia_ManSwiSimulateCo( p, Gia_ObjCioId(pObj), pObj );
        }
        else // if ( Gia_ObjIsCi(pObj) )
        {
            assert( Gia_ObjValue(pObj) < p->pAig->nFront );
//            Gia_ManSwiSimulateCi( p, pObj, iCis++ );
            Gia_ManSwiSimulateCi( p, pObj, Gia_ObjCioId(pObj) );
        }
        if ( fCount && !Gia_ObjIsCo(pObj) )
        {
            if ( p->pPars->fProbTrans )
                p->pData1[i] += Gia_ManSwiSimInfoCountTrans( p, Gia_ObjValue(pObj) );
            else 
                p->pData1[i] += Gia_ManSwiSimInfoCountOnes( p, Gia_ObjValue(pObj) );
        }
    }
//    assert( Gia_ManCiNum(p->pAig) == iCis );
//    assert( Gia_ManCoNum(p->pAig) == iCos );
}

/**Function*************************************************************

  Synopsis    [Computes switching activity of one node.]

  Description [Uses the formula: Switching = 2 * nOnes * nZeros / (nTotal ^ 2) ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Gia_ManSwiComputeSwitching( int nOnes, int nSimWords )
{
    int nTotal = 32 * nSimWords;
    return (float)2.0 * nOnes / nTotal * (nTotal - nOnes) / nTotal;
}

/**Function*************************************************************

  Synopsis    [Computes switching activity of one node.]

  Description [Uses the formula: Switching = 2 * nOnes * nZeros / (nTotal ^ 2) ]
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
float Gia_ManSwiComputeProbOne( int nOnes, int nSimWords )
{
    int nTotal = 32 * nSimWords;
    return (float)nOnes / nTotal;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManSwiSimulate( Gia_Man_t * pAig, Gia_ParSwi_t * pPars )
{
    Gia_ManSwi_t * p;
    Gia_Obj_t * pObj;
    Vec_Int_t * vSwitching;
    float * pSwitching;
    int i;
    abctime clk, clkTotal = Abc_Clock();
    if ( pPars->fProbOne && pPars->fProbTrans )
        printf( "Conflict of options: Can either compute probability of 1, or probability of switching by observing transitions.\n" );
    // create manager
    clk = Abc_Clock();
    p = Gia_ManSwiCreate( pAig, pPars );
    if ( pPars->fVerbose )
    {
        printf( "Obj = %8d (%8d). F = %6d. ", 
            pAig->nObjs, Gia_ManCiNum(pAig) + Gia_ManAndNum(pAig), p->pAig->nFront );
        printf( "AIG = %7.2f MB. F-mem = %7.2f MB. Other = %7.2f MB.  ", 
            12.0*Gia_ManObjNum(p->pAig)/(1<<20), 
            4.0*p->nWords*p->pAig->nFront/(1<<20), 
            4.0*p->nWords*(Gia_ManCiNum(p->pAig) + Gia_ManCoNum(p->pAig))/(1<<20) );
        ABC_PRT( "Time", Abc_Clock() - clk );
    }
    // perform simulation
    Gia_ManRandom( 1 );
    Gia_ManSwiSimInfoInit( p );
    for ( i = 0; i < pPars->nIters; i++ )
    {
        Gia_ManSwiSimulateRound( p, i >= pPars->nPref );
        if ( i == pPars->nIters - 1 )
            break;
        if ( pPars->fProbTrans )
            Gia_ManSwiSimInfoTransferShift( p, pPars->nRandPiFactor );
        else
            Gia_ManSwiSimInfoTransfer( p, pPars->nRandPiFactor );
    }
    if ( pPars->fVerbose )
    {
        printf( "Simulated %d frames with %d words. ", pPars->nIters, pPars->nWords );
        ABC_PRT( "Simulation time", Abc_Clock() - clkTotal );
    }
    // derive the result
    vSwitching = Vec_IntStart( Gia_ManObjNum(pAig) );
    pSwitching = (float *)vSwitching->pArray;
    if ( pPars->fProbOne )
    {
        Gia_ManForEachObj( pAig, pObj, i )
            pSwitching[i] = Gia_ManSwiComputeProbOne( p->pData1[i], pPars->nWords*(pPars->nIters-pPars->nPref) );
        Gia_ManForEachCo( pAig, pObj, i )
        {
            if ( Gia_ObjFaninC0(pObj) )
                pSwitching[Gia_ObjId(pAig,pObj)] = (float)1.0-pSwitching[Gia_ObjId(pAig,Gia_ObjFanin0(pObj))];
            else
                pSwitching[Gia_ObjId(pAig,pObj)] = pSwitching[Gia_ObjId(pAig,Gia_ObjFanin0(pObj))];
        }
    }
    else if ( pPars->fProbTrans )
    {
        Gia_ManForEachObj( pAig, pObj, i )
            pSwitching[i] = Gia_ManSwiComputeProbOne( p->pData1[i], pPars->nWords*(pPars->nIters-pPars->nPref) );
    }
    else
    {
        Gia_ManForEachObj( pAig, pObj, i )
            pSwitching[i] = Gia_ManSwiComputeSwitching( p->pData1[i], pPars->nWords*(pPars->nIters-pPars->nPref) );
    }
/*
    printf( "PI: " );
    Gia_ManForEachPi( pAig, pObj, i )
        printf( "%d=%d (%f)  ", i, p->pData1[Gia_ObjId(pAig,pObj)], pSwitching[Gia_ObjId(pAig,pObj)] );
    printf( "\n" );

    printf( "LO: " );
    Gia_ManForEachRo( pAig, pObj, i )
        printf( "%d=%d (%f)  ", i, p->pData1[Gia_ObjId(pAig,pObj)], pSwitching[Gia_ObjId(pAig,pObj)] );
    printf( "\n" );

    printf( "PO: " );
    Gia_ManForEachPo( pAig, pObj, i )
        printf( "%d=%d (%f)  ", i, p->pData1[Gia_ObjId(pAig,pObj)], pSwitching[Gia_ObjId(pAig,pObj)] );
    printf( "\n" );

    printf( "LI: " );
    Gia_ManForEachRi( pAig, pObj, i )
        printf( "%d=%d (%f)  ", i, p->pData1[Gia_ObjId(pAig,pObj)], pSwitching[Gia_ObjId(pAig,pObj)] );
    printf( "\n" );
*/
    Gia_ManSwiDelete( p );
    return vSwitching;

}
/**Function*************************************************************

  Synopsis    [Computes probability of switching (or of being 1).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManComputeSwitchProbs( Gia_Man_t * pGia, int nFrames, int nPref, int fProbOne )
{
    Gia_ParSwi_t Pars, * pPars = &Pars;
    // set the default parameters
    Gia_ManSetDefaultParamsSwi( pPars ); 
    // override some of the defaults
    pPars->nIters   = nFrames;  // set number of total timeframes
    if ( Abc_FrameReadFlag("seqsimframes") )
        pPars->nIters = atoi( Abc_FrameReadFlag("seqsimframes") );
    pPars->nPref    = nPref;    // set number of first timeframes to skip  
    // decide what should be computed
    if ( fProbOne )
    {
        // if the user asked to compute propability of 1, we do not need transition information
        pPars->fProbOne   = 1;  // enable computing probabiblity of being one
        pPars->fProbTrans = 0;  // disable computing transition probability 
    }
    else
    {
        // if the user asked for transition propabability, we do not need to compute probability of 1
        pPars->fProbOne   = 0;  // disable computing probabiblity of being one
        pPars->fProbTrans = 1;  // enable computing transition probability 
    }
    // perform the computation of switching activity
    return Gia_ManSwiSimulate( pGia, pPars );
}
Vec_Int_t * Saig_ManComputeSwitchProbs( Aig_Man_t * pAig, int nFrames, int nPref, int fProbOne )
{
    Vec_Int_t * vSwitching, * vResult;
    Gia_Man_t * p;
    Aig_Obj_t * pObj;
    int i;
    // translate AIG into the intermediate form (takes care of choices if present!)
    p = Gia_ManFromAigSwitch( pAig );
    // perform the computation of switching activity
    vSwitching = Gia_ManComputeSwitchProbs( p, nFrames, nPref, fProbOne );
    // transfer the computed result to the original AIG
    vResult = Vec_IntStart( Aig_ManObjNumMax(pAig) );
    Aig_ManForEachObj( pAig, pObj, i )
    {
//        if ( Aig_ObjIsCo(pObj) )
//            printf( "%d=%f\n", i, Abc_Int2Float( Vec_IntEntry(vSwitching, Abc_Lit2Var(pObj->iData)) ) );
        Vec_IntWriteEntry( vResult, i, Vec_IntEntry(vSwitching, Abc_Lit2Var(pObj->iData)) );
    }
    // delete intermediate results
    Vec_IntFree( vSwitching );
    Gia_ManStop( p );
    return vResult;
}

/**Function*************************************************************

  Synopsis    [Computes probability of switching (or of being 1).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Gia_ManEvaluateSwitching( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    float SwitchTotal = 0.0;
    int i;
    assert( p->pSwitching );
    ABC_FREE( p->pRefs );
    Gia_ManCreateRefs( p );
    Gia_ManForEachObj( p, pObj, i )
        SwitchTotal += (float)Gia_ObjRefNum(p, pObj) * p->pSwitching[i] / 255;
    return SwitchTotal;
}

/**Function*************************************************************

  Synopsis    [Computes probability of switching (or of being 1).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
float Gia_ManComputeSwitching( Gia_Man_t * p, int nFrames, int nPref, int fProbOne )
{
    Gia_Man_t * pDfs;
    Gia_Obj_t * pObj, * pObjDfs;
    Vec_Int_t * vSwitching;
    float * pSwitching, Switch, SwitchTotal = 0.0;
    int i;
    // derives the DFS ordered AIG
    if ( Gia_ManHasMapping(p) )
        Gia_ManSetRefsMapped(p);
    else
        Gia_ManCreateRefs( p );
//    pDfs = Gia_ManDupOrderDfs( p );
    pDfs = Gia_ManDup( p );
    assert( Gia_ManObjNum(pDfs) == Gia_ManObjNum(p) );
    // perform the computation of switching activity
    vSwitching = Gia_ManComputeSwitchProbs( pDfs, nFrames, nPref, fProbOne );
    // transfer the computed result to the original AIG
    ABC_FREE( p->pSwitching );
    p->pSwitching = ABC_CALLOC( unsigned char, Gia_ManObjNum(p) );
    pSwitching = (float *)vSwitching->pArray;
    Gia_ManForEachObj( p, pObj, i )
    {
        pObjDfs = Gia_ObjFromLit( pDfs, pObj->Value );
        Switch = pSwitching[ Gia_ObjId(pDfs, pObjDfs) ];
        p->pSwitching[i] = (char)((Switch >= 1.0) ? 255 : (int)((0.002 + Switch) * 255)); // 0.00196 = (1/255)/2
        if ( Gia_ObjIsCi(pObj) || (Gia_ObjIsAnd(pObj) && (!Gia_ManHasMapping(p) || Gia_ObjIsLut(p, i))) )
        {
            SwitchTotal += (float)Gia_ObjRefNum(p, pObj) * p->pSwitching[i] / 255;
//            printf( "%d = %.2f\n", i, (float)Gia_ObjRefNum(p, pObj) * p->pSwitching[i] / 255 );            
        }
    }
    Vec_IntFree( vSwitching );
    Gia_ManStop( pDfs );
    return SwitchTotal;
}
*/
float Gia_ManComputeSwitching( Gia_Man_t * p, int nFrames, int nPref, int fProbOne )
{
    Vec_Int_t * vSwitching = Gia_ManComputeSwitchProbs( p, nFrames, nPref, fProbOne );
    float * pSwi = (float *)Vec_IntArray(vSwitching), SwiTotal = 0;
    Gia_Obj_t * pObj;
    int i, k, iFan;
    if ( Gia_ManHasMapping(p) )
    {
        Gia_ManForEachLut( p, i )
            Gia_LutForEachFanin( p, i, iFan, k )
                SwiTotal += pSwi[iFan];
    }
    else
    {
        Gia_ManForEachAnd( p, pObj, i )
            SwiTotal += pSwi[Gia_ObjFaninId0(pObj, i)] + pSwi[Gia_ObjFaninId1(pObj, i)];
    }
    Vec_IntFree( vSwitching );
    return SwiTotal;
}

/**Function*************************************************************

  Synopsis    [Determine probability of being 1 at the outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Flt_t * Gia_ManPrintOutputProb( Gia_Man_t * p )
{
    Vec_Flt_t * vSimData;
    Gia_Man_t * pDfs = Gia_ManDup( p );
    assert( Gia_ManObjNum(pDfs) == Gia_ManObjNum(p) );
    vSimData = (Vec_Flt_t *)Gia_ManComputeSwitchProbs( pDfs, (Gia_ManRegNum(p) ? 16 : 1), 0, 1 );
    Gia_ManStop( pDfs );
    return vSimData;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

