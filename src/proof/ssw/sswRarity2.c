/**CFile****************************************************************

  FileName    [sswRarity.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Inductive prover with constraints.]

  Synopsis    [Rarity-driven refinement of equivalence classes.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2008.]

  Revision    [$Id: sswRarity.c,v 1.00 2008/09/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sswInt.h"
#include "aig/gia/giaAig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Ssw_RarMan_t_ Ssw_RarMan_t;
struct Ssw_RarMan_t_
{
    // parameters
    int            nWords;       // the number of words to simulate
    int            nFrames;      // the number of frames to simulate
    int            nBinSize;     // the number of flops in one group
    int            fVerbose;     // the verbosiness flag
    int            nGroups;      // the number of flop groups
    // internal data
    Aig_Man_t *    pAig;         // AIG with equivalence classes
    Ssw_Cla_t *    ppClasses;    // equivalence classes
    Ssw_Sml_t *    pSml;         // simulation manager
    Vec_Ptr_t *    vSimInfo;     // simulation info from pSml manager
    Vec_Int_t *    vInits;       // initial state
    // rarity data
    int *          pRarity;      // occur counts for patterns in groups
    int *          pGroupValues; // occur counts in each group
    double *       pPatCosts;    // pattern costs

};

static inline int  Ssw_RarGetBinPat( Ssw_RarMan_t * p, int iBin, int iPat )
{
    assert( iBin >= 0 && iBin < Aig_ManRegNum(p->pAig) / p->nBinSize );
    assert( iPat >= 0 && iPat < (1 << p->nBinSize) );
    return p->pRarity[iBin * (1 << p->nBinSize) + iPat];
}
static inline void Ssw_RarSetBinPat( Ssw_RarMan_t * p, int iBin, int iPat, int Value )
{
    assert( iBin >= 0 && iBin < Aig_ManRegNum(p->pAig) / p->nBinSize );
    assert( iPat >= 0 && iPat < (1 << p->nBinSize) );
    p->pRarity[iBin * (1 << p->nBinSize) + iPat] = Value;
}
static inline void Ssw_RarAddToBinPat( Ssw_RarMan_t * p, int iBin, int iPat )
{
    assert( iBin >= 0 && iBin < Aig_ManRegNum(p->pAig) / p->nBinSize );
    assert( iPat >= 0 && iPat < (1 << p->nBinSize) );
    p->pRarity[iBin * (1 << p->nBinSize) + iPat]++;
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static Ssw_RarMan_t * Ssw_RarManStart( Aig_Man_t * pAig, int nWords, int nFrames, int nBinSize, int fVerbose )
{
    Ssw_RarMan_t * p;
    if ( Aig_ManRegNum(pAig) < nBinSize || nBinSize <= 0 )
        return NULL;
    p = ABC_CALLOC( Ssw_RarMan_t, 1 );
    p->pAig = pAig;
    p->nWords = nWords;
    p->nFrames = nFrames;
    p->nBinSize = nBinSize;
    p->fVerbose = fVerbose;
    p->nGroups  = Aig_ManRegNum(pAig) / nBinSize;
    p->pRarity  = ABC_CALLOC( int, (1 << nBinSize) * p->nGroups );
    p->pGroupValues = ABC_CALLOC( int, p->nGroups );
    p->pPatCosts = ABC_CALLOC( double, p->nWords * 32 );
    p->pSml = Ssw_SmlStart( pAig, 0, nFrames, nWords );
    p->vSimInfo = Ssw_SmlSimDataPointers( p->pSml );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Ssw_RarManStop( Ssw_RarMan_t * p )
{
    if ( p->pSml ) Ssw_SmlStop( p->pSml );
    if ( p->ppClasses ) Ssw_ClassesStop( p->ppClasses );
    Vec_PtrFreeP( &p->vSimInfo );
    Vec_IntFreeP( &p->vInits );
    ABC_FREE( p->pGroupValues );
    ABC_FREE( p->pPatCosts );
    ABC_FREE( p->pRarity );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Updates rarity counters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Ssw_RarUpdateCounters( Ssw_RarMan_t * p )
{
    Aig_Obj_t * pObj;
    unsigned * pData;
    int i, k;
/*
    Saig_ManForEachLi( p->pAig, pObj, i )
    {
        pData = (unsigned *)Vec_PtrEntry( p->vSimInfo, Aig_ObjId(pObj) ) + p->nWords * (p->nFrames - 1);
        Extra_PrintBinary( stdout, pData, 32 );  Abc_Print( 1, "\n" );
    }
*/
    for ( k = 0; k < p->nWords * 32; k++ )
    {
        for ( i = 0; i < p->nGroups; i++ )
            p->pGroupValues[i] = 0;
        Saig_ManForEachLi( p->pAig, pObj, i )
        {
            pData = (unsigned *)Vec_PtrEntry( p->vSimInfo, Aig_ObjId(pObj) ) + p->nWords * (p->nFrames - 1);
            if ( Abc_InfoHasBit(pData, k) && i / p->nBinSize < p->nGroups )
                p->pGroupValues[i / p->nBinSize] |= (1 << (i % p->nBinSize));
        }
        for ( i = 0; i < p->nGroups; i++ )
            Ssw_RarAddToBinPat( p, i, p->pGroupValues[i] );
    }
/*
    for ( i = 0; i < p->nGroups; i++ )
    {
        for ( k = 0; k < (1 << p->nBinSize); k++ )
            Abc_Print( 1, "%d ", Ssw_RarGetBinPat(p, i, k) );
        Abc_Print( 1, "\n" );
    }
*/
}

/**Function*************************************************************

  Synopsis    [Select best patterns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Ssw_RarTransferPatterns( Ssw_RarMan_t * p, Vec_Int_t * vInits )
{
    Aig_Obj_t * pObj;
    unsigned * pData;
    int i, k, Value;

    // for each pattern
    for ( k = 0; k < p->nWords * 32; k++ )
    {
        for ( i = 0; i < p->nGroups; i++ )
            p->pGroupValues[i] = 0;
        // compute its group values
        Saig_ManForEachLi( p->pAig, pObj, i )
        {
            pData = (unsigned *)Vec_PtrEntry( p->vSimInfo, Aig_ObjId(pObj) ) + p->nWords * (p->nFrames - 1);
            if ( Abc_InfoHasBit(pData, k) && i / p->nBinSize < p->nGroups )
                p->pGroupValues[i / p->nBinSize] |= (1 << (i % p->nBinSize));
        }
        // find the cost of its values
        p->pPatCosts[k] = 0.0;
        for ( i = 0; i < p->nGroups; i++ )
        {
            Value = Ssw_RarGetBinPat( p, i, p->pGroupValues[i] );
            assert( Value > 0 );
            p->pPatCosts[k] += 1.0/(Value*Value);
        }
        // print the result
//        Abc_Print( 1, "%3d : %9.6f\n", k, p->pPatCosts[k] );
    }

    // choose as many as there are words
    Vec_IntClear( vInits );
    for ( i = 0; i < p->nWords; i++ )
    {
        // select the best
        int iPatBest = -1;
        double iCostBest = -ABC_INFINITY;
        for ( k = 0; k < p->nWords * 32; k++ )
            if ( iCostBest < p->pPatCosts[k] )
            {
                iCostBest = p->pPatCosts[k];
                iPatBest  = k;
            }
        // remove from costs
        assert( iPatBest >= 0 );
        p->pPatCosts[iPatBest] = -ABC_INFINITY;
        // set the flops
        Saig_ManForEachLi( p->pAig, pObj, k )
        {
            pData = (unsigned *)Vec_PtrEntry( p->vSimInfo, Aig_ObjId(pObj) ) + p->nWords * (p->nFrames - 1);
            Vec_IntPush( vInits, Abc_InfoHasBit(pData, iPatBest) );
        }
//Abc_Print( 1, "Best pattern %5d\n", iPatBest );
    }
    assert( Vec_IntSize(vInits) == Aig_ManRegNum(p->pAig) * p->nWords );
}


/**Function*************************************************************

  Synopsis    [Performs fraiging for one node.]

  Description [Returns the fraiged node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Vec_Int_t * Ssw_RarFindStartingState( Aig_Man_t * pAig, Abc_Cex_t * pCex )
{
    Vec_Int_t * vInit;
    Aig_Obj_t * pObj, * pObjLi;
    int f, i, iBit;
    // assign register outputs
    Saig_ManForEachLi( pAig, pObj, i )
        pObj->fMarkB = Abc_InfoHasBit( pCex->pData, i );
    // simulate the timeframes
    iBit = pCex->nRegs;
    for ( f = 0; f <= pCex->iFrame; f++ )
    {
        // set the PI simulation information
        Aig_ManConst1(pAig)->fMarkB = 1;
        Saig_ManForEachPi( pAig, pObj, i )
            pObj->fMarkB = Abc_InfoHasBit( pCex->pData, iBit++ );
        Saig_ManForEachLiLo( pAig, pObjLi, pObj, i )
            pObj->fMarkB = pObjLi->fMarkB;
        // simulate internal nodes
        Aig_ManForEachNode( pAig, pObj, i )
            pObj->fMarkB = ( Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj) )
                         & ( Aig_ObjFanin1(pObj)->fMarkB ^ Aig_ObjFaninC1(pObj) );
        // assign the COs
        Aig_ManForEachCo( pAig, pObj, i )
            pObj->fMarkB = ( Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj) );
    }
    assert( iBit == pCex->nBits );
    // check that the output failed as expected -- cannot check because it is not an SRM!
//    pObj = Aig_ManCo( pAig, pCex->iPo );
//    if ( pObj->fMarkB != 1 )
//        Abc_Print( 1, "The counter-example does not refine the output.\n" );
    // record the new pattern
    vInit = Vec_IntAlloc( Saig_ManRegNum(pAig) );
    Saig_ManForEachLo( pAig, pObj, i )
        Vec_IntPush( vInit, pObj->fMarkB );
    return vInit;
}


/**Function*************************************************************

  Synopsis    [Perform sequential simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_RarSimulate2( Aig_Man_t * pAig, int nFrames, int nWords, int nBinSize, int nRounds, int TimeOut, int fVerbose )
{
    int fMiter = 1;
    Ssw_RarMan_t * p;
    int r;
    abctime clk, clkTotal = Abc_Clock();
    abctime nTimeToStop = TimeOut ? TimeOut * CLOCKS_PER_SEC + Abc_Clock(): 0;
    int RetValue = -1;
    assert( Aig_ManRegNum(pAig) > 0 );
    assert( Aig_ManConstrNum(pAig) == 0 );
    // consider the case of empty AIG
    if ( Aig_ManNodeNum(pAig) == 0 )
        return -1;
    if ( fVerbose )
        Abc_Print( 1, "Simulating %d words through %d frames with %d binsize, %d rounds, and %d sec timeout.\n",
            nWords, nFrames, nBinSize, nRounds, TimeOut );
    // reset random numbers
    Aig_ManRandom( 1 );

    // create manager
    p = Ssw_RarManStart( pAig, nWords, nFrames, nBinSize, fVerbose );
    p->vInits = Vec_IntStart( Aig_ManRegNum(pAig) * nWords );
    Ssw_SmlInitializeSpecial( p->pSml, p->vInits );

    // perform simulation rounds
    for ( r = 0; r < nRounds; r++ )
    {
        clk = Abc_Clock();
        // simulate
        Ssw_SmlSimulateOne( p->pSml );
        if ( fMiter && Ssw_SmlCheckNonConstOutputs(p->pSml) )
        {
            if ( fVerbose ) Abc_Print( 1, "\n" );
            Abc_Print( 1, "Simulation asserted a PO in frame f: %d <= f < %d.\n", r * nFrames, (r+1) * nFrames );
            RetValue = 0;
            break;
        }
        // get initialization patterns
        Ssw_RarUpdateCounters( p );
        Ssw_RarTransferPatterns( p, p->vInits );
        Ssw_SmlInitializeSpecial( p->pSml, p->vInits );
        // printout
        if ( fVerbose )
        {
//            Abc_Print( 1, "Round %3d:  ", r );
//            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
            Abc_Print( 1, "." );
        }
        // check timeout
        if ( TimeOut && Abc_Clock() > nTimeToStop )
        {
            if ( fVerbose ) Abc_Print( 1, "\n" );
            Abc_Print( 1, "Reached timeout (%d seconds).\n",  TimeOut );
            break;
        }
    }
    if ( r == nRounds )
    {
        if ( fVerbose ) Abc_Print( 1, "\n" );
        Abc_Print( 1, "Simulation did not assert POs in the first %d frames.  ", nRounds * nFrames );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
    }
    // cleanup
    Ssw_RarManStop( p );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Filter equivalence classes of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_RarSignalFilter2( Aig_Man_t * pAig, int nFrames, int nWords, int nBinSize, int nRounds, int TimeOut, Abc_Cex_t * pCex, int fLatchOnly, int fVerbose )
{
    int fMiter = 0;
    Ssw_RarMan_t * p;
    int r, i, k;
    abctime clkTotal = Abc_Clock();
    abctime nTimeToStop = TimeOut ? TimeOut * CLOCKS_PER_SEC + Abc_Clock(): 0;
    int RetValue = -1;
    assert( Aig_ManRegNum(pAig) > 0 );
    assert( Aig_ManConstrNum(pAig) == 0 );
    // consider the case of empty AIG
    if ( Aig_ManNodeNum(pAig) == 0 )
        return -1;
    if ( fVerbose )
        Abc_Print( 1, "Filtering equivs with %d words through %d frames with %d binsize, %d rounds, and %d sec timeout.\n",
            nWords, nFrames, nBinSize, nRounds, TimeOut );
    // reset random numbers
    Aig_ManRandom( 1 );

    // create manager
    p = Ssw_RarManStart( pAig, nWords, nFrames, nBinSize, fVerbose );
    // compute starting state if needed
    assert( p->vInits == NULL );
    if ( pCex )
        p->vInits = Ssw_RarFindStartingState( pAig, pCex );
    else
        p->vInits = Vec_IntStart( Aig_ManRegNum(pAig) );
    // duplicate the array
    for ( i = 1; i < nWords; i++ )
        for ( k = 0; k < Aig_ManRegNum(pAig); k++ )
            Vec_IntPush( p->vInits, Vec_IntEntry(p->vInits, k) );
    assert( Vec_IntSize(p->vInits) == Aig_ManRegNum(pAig) * nWords );
    // initialize simulation manager
    Ssw_SmlInitializeSpecial( p->pSml, p->vInits );

    // create trivial equivalence classes with all nodes being candidates for constant 1
    if ( pAig->pReprs == NULL )
        p->ppClasses = Ssw_ClassesPrepareSimple( pAig, fLatchOnly, 0 );
    else
        p->ppClasses = Ssw_ClassesPrepareFromReprs( pAig );
    Ssw_ClassesSetData( p->ppClasses, p->pSml, NULL, (int(*)(void *,Aig_Obj_t *))Ssw_SmlObjIsConstWord, (int(*)(void *,Aig_Obj_t *,Aig_Obj_t *))Ssw_SmlObjsAreEqualWord );
    // print the stats
    if ( fVerbose )
    {
        Abc_Print( 1, "Initial  :  " );
        Ssw_ClassesPrint( p->ppClasses, 0 );
    }
    // refine classes using BMC
    for ( r = 0; r < nRounds; r++ )
    {
        // start filtering equivalence classes
        if ( Ssw_ClassesCand1Num(p->ppClasses) == 0 && Ssw_ClassesClassNum(p->ppClasses) == 0 )
        {
            Abc_Print( 1, "All equivalences are refined away.\n" );
            break;
        }
        // simulate
        Ssw_SmlSimulateOne( p->pSml );
        if ( fMiter && Ssw_SmlCheckNonConstOutputs(p->pSml) )
        {
            if ( fVerbose ) Abc_Print( 1, "\n" );
            Abc_Print( 1, "Simulation asserted a PO in frame f: %d <= f < %d.\n", r * nFrames, (r+1) * nFrames );
            RetValue = 0;
            break;
        }
        // check equivalence classes
        Ssw_ClassesRefineConst1( p->ppClasses, 1 );
        Ssw_ClassesRefine( p->ppClasses, 1 );
        // printout
        if ( fVerbose )
        {
            Abc_Print( 1, "Round %3d:  ", r );
            Ssw_ClassesPrint( p->ppClasses, 0 );
        }
        // get initialization patterns
        Ssw_RarUpdateCounters( p );
        Ssw_RarTransferPatterns( p, p->vInits );
        Ssw_SmlInitializeSpecial( p->pSml, p->vInits );
        // check timeout
        if ( TimeOut && Abc_Clock() > nTimeToStop )
        {
            if ( fVerbose ) Abc_Print( 1, "\n" );
            Abc_Print( 1, "Reached timeout (%d seconds).\n",  TimeOut );
            break;
        }
    }
    if ( r == nRounds )
    {
        Abc_Print( 1, "Simulation did not assert POs in the first %d frames.  ", nRounds * nFrames );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
    }
    // cleanup
    Ssw_RarManStop( p );
    return -1;
}

/**Function*************************************************************

  Synopsis    [Filter equivalence classes of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_RarSignalFilterGia2( Gia_Man_t * p, int nFrames, int nWords, int nBinSize, int nRounds, int TimeOut, Abc_Cex_t * pCex, int fLatchOnly, int fVerbose )
{
    Aig_Man_t * pAig;
    int RetValue;
    pAig = Gia_ManToAigSimple( p );
    if ( p->pReprs != NULL )
    {
        Gia_ManReprToAigRepr2( pAig, p );
        ABC_FREE( p->pReprs );
        ABC_FREE( p->pNexts );
    }
    RetValue = Ssw_RarSignalFilter2( pAig, nFrames, nWords, nBinSize, nRounds, TimeOut, pCex, fLatchOnly, fVerbose );
    Gia_ManReprFromAigRepr( pAig, p );
    Aig_ManStop( pAig );
    return RetValue;
}




////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
