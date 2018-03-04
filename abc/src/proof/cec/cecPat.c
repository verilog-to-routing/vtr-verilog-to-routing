/**CFile****************************************************************

  FileName    [cecPat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Combinational equivalence checking.]

  Synopsis    [Simulation pattern manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cecPat.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cecInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cec_ManPatStoreNum( Cec_ManPat_t * p, int Num )
{
    unsigned x = (unsigned)Num;
    assert( Num >= 0 );
    while ( x & ~0x7f )
    {
        Vec_StrPush( p->vStorage, (char)((x & 0x7f) | 0x80) );
        x >>= 7;
    }
    Vec_StrPush( p->vStorage, (char)x );
}

/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cec_ManPatRestoreNum( Cec_ManPat_t * p )
{
    int ch, i, x = 0;
    for ( i = 0; (ch = Vec_StrEntry(p->vStorage, p->iStart++)) & 0x80; i++ )
        x |= (ch & 0x7f) << (7 * i);
    return x | (ch << (7 * i));
}

/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cec_ManPatStore( Cec_ManPat_t * p, Vec_Int_t * vPat )
{
    int i, Number, NumberPrev;
    assert( Vec_IntSize(vPat) > 0 );
    Cec_ManPatStoreNum( p, Vec_IntSize(vPat) );
    NumberPrev = Vec_IntEntry( vPat, 0 );
    Cec_ManPatStoreNum( p, NumberPrev );
    Vec_IntForEachEntryStart( vPat, Number, i, 1 )
    {
        assert( NumberPrev < Number );
        Cec_ManPatStoreNum( p, Number - NumberPrev );
        NumberPrev = Number;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cec_ManPatRestore( Cec_ManPat_t * p, Vec_Int_t * vPat )
{
    int i, Size, Number;
    Vec_IntClear( vPat );
    Size = Cec_ManPatRestoreNum( p );
    Number = Cec_ManPatRestoreNum( p );
    Vec_IntPush( vPat, Number );
    for ( i = 1; i < Size; i++ )
    {
        Number += Cec_ManPatRestoreNum( p );
        Vec_IntPush( vPat, Number );
    }
    assert( Vec_IntSize(vPat) == Size );
}


/**Function*************************************************************

  Synopsis    [Derives satisfying assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec_ManPatComputePattern_rec( Cec_ManSat_t * pSat, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    int Counter = 0;
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return 0;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
    {
        pObj->fMark1 = Cec_ObjSatVarValue( pSat, pObj );
        return 1;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Counter += Cec_ManPatComputePattern_rec( pSat, p, Gia_ObjFanin0(pObj) );
    Counter += Cec_ManPatComputePattern_rec( pSat, p, Gia_ObjFanin1(pObj) );
    pObj->fMark1 = (Gia_ObjFanin0(pObj)->fMark1 ^ Gia_ObjFaninC0(pObj)) & 
                   (Gia_ObjFanin1(pObj)->fMark1 ^ Gia_ObjFaninC1(pObj));
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Derives satisfying assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManPatComputePattern1_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vPat )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
    {
        Vec_IntPush( vPat, Abc_Var2Lit( Gia_ObjCioId(pObj), pObj->fMark1==0 ) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    if ( pObj->fMark1 == 1 )
    {
        Cec_ManPatComputePattern1_rec( p, Gia_ObjFanin0(pObj), vPat );
        Cec_ManPatComputePattern1_rec( p, Gia_ObjFanin1(pObj), vPat );
    }
    else
    {
        assert( (Gia_ObjFanin0(pObj)->fMark1 ^ Gia_ObjFaninC0(pObj)) == 0 ||
                (Gia_ObjFanin1(pObj)->fMark1 ^ Gia_ObjFaninC1(pObj)) == 0 );
        if ( (Gia_ObjFanin0(pObj)->fMark1 ^ Gia_ObjFaninC0(pObj)) == 0 )
            Cec_ManPatComputePattern1_rec( p, Gia_ObjFanin0(pObj), vPat );
        else
            Cec_ManPatComputePattern1_rec( p, Gia_ObjFanin1(pObj), vPat );
    }
}

/**Function*************************************************************

  Synopsis    [Derives satisfying assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManPatComputePattern2_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vPat )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
    {
        Vec_IntPush( vPat, Abc_Var2Lit( Gia_ObjCioId(pObj), pObj->fMark1==0 ) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    if ( pObj->fMark1 == 1 )
    {
        Cec_ManPatComputePattern2_rec( p, Gia_ObjFanin0(pObj), vPat );
        Cec_ManPatComputePattern2_rec( p, Gia_ObjFanin1(pObj), vPat );
    }
    else
    {
        assert( (Gia_ObjFanin0(pObj)->fMark1 ^ Gia_ObjFaninC0(pObj)) == 0 ||
                (Gia_ObjFanin1(pObj)->fMark1 ^ Gia_ObjFaninC1(pObj)) == 0 );
        if ( (Gia_ObjFanin1(pObj)->fMark1 ^ Gia_ObjFaninC1(pObj)) == 0 )
            Cec_ManPatComputePattern2_rec( p, Gia_ObjFanin1(pObj), vPat );
        else
            Cec_ManPatComputePattern2_rec( p, Gia_ObjFanin0(pObj), vPat );
    }
}

/**Function*************************************************************

  Synopsis    [Derives satisfying assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec_ManPatComputePattern3_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    int Value0, Value1, Value;
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return (pObj->fMark1 << 1) | pObj->fMark0;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
    {
        pObj->fMark0 = 1;
        pObj->fMark1 = 1;
        return GIA_UND;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Value0 = Cec_ManPatComputePattern3_rec( p, Gia_ObjFanin0(pObj) );
    Value1 = Cec_ManPatComputePattern3_rec( p, Gia_ObjFanin1(pObj) );
    Value = Gia_XsimAndCond( Value0, Gia_ObjFaninC0(pObj), Value1, Gia_ObjFaninC1(pObj) );
    pObj->fMark0 =  (Value & 1);
    pObj->fMark1 = ((Value >> 1) & 1);
    return Value;
}

/**Function*************************************************************

  Synopsis    [Derives satisfying assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManPatVerifyPattern( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vPat )
{
    Gia_Obj_t * pTemp;
    int i, Value;
    Gia_ManIncrementTravId( p );
    Vec_IntForEachEntry( vPat, Value, i )
    {
        pTemp = Gia_ManCi( p, Abc_Lit2Var(Value) );
//        assert( Abc_LitIsCompl(Value) != (int)pTemp->fMark1 );
        if ( pTemp->fMark1 )
        {
            pTemp->fMark0 = 0;
            pTemp->fMark1 = 1;
        }
        else
        {
            pTemp->fMark0 = 1;
            pTemp->fMark1 = 0;
        }
        Gia_ObjSetTravIdCurrent( p, pTemp );
    }
    Value = Cec_ManPatComputePattern3_rec( p, Gia_ObjFanin0(pObj) );
    Value = Gia_XsimNotCond( Value, Gia_ObjFaninC0(pObj) );
    if ( Value != GIA_ONE )
        Abc_Print( 1, "Cec_ManPatVerifyPattern(): Verification failed.\n" );
    assert( Value == GIA_ONE );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManPatComputePattern4_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    pObj->fMark0 = 0;
    if ( Gia_ObjIsCi(pObj) )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Cec_ManPatComputePattern4_rec( p, Gia_ObjFanin0(pObj) );
    Cec_ManPatComputePattern4_rec( p, Gia_ObjFanin1(pObj) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManPatCleanMark0( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    assert( Gia_ObjIsCo(pObj) );
    Gia_ManIncrementTravId( p );
    Cec_ManPatComputePattern4_rec( p, Gia_ObjFanin0(pObj) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManPatSavePattern( Cec_ManPat_t * pMan, Cec_ManSat_t *  p, Gia_Obj_t * pObj )
{
    Vec_Int_t * vPat;
    int nPatLits;
    abctime clkTotal = Abc_Clock();
//    abctime clk;
    assert( Gia_ObjIsCo(pObj) );
    pMan->nPats++;
    pMan->nPatsAll++;
    // compute values in the cone of influence
//clk = Abc_Clock();
    Gia_ManIncrementTravId( p->pAig );
    nPatLits = Cec_ManPatComputePattern_rec( p, p->pAig, Gia_ObjFanin0(pObj) );
    assert( (Gia_ObjFanin0(pObj)->fMark1 ^ Gia_ObjFaninC0(pObj)) == 1 );
    pMan->nPatLits += nPatLits;
    pMan->nPatLitsAll += nPatLits;
//pMan->timeFind += Abc_Clock() - clk;
    // compute sensitizing path
//clk = Abc_Clock();
    Vec_IntClear( pMan->vPattern1 );
    Gia_ManIncrementTravId( p->pAig );
    Cec_ManPatComputePattern1_rec( p->pAig, Gia_ObjFanin0(pObj), pMan->vPattern1 );
    // compute sensitizing path
    Vec_IntClear( pMan->vPattern2 );
    Gia_ManIncrementTravId( p->pAig );
    Cec_ManPatComputePattern2_rec( p->pAig, Gia_ObjFanin0(pObj), pMan->vPattern2 );
    // compare patterns
    vPat = Vec_IntSize(pMan->vPattern1) < Vec_IntSize(pMan->vPattern2) ? pMan->vPattern1 : pMan->vPattern2;
    pMan->nPatLitsMin += Vec_IntSize(vPat);
    pMan->nPatLitsMinAll += Vec_IntSize(vPat);
//pMan->timeShrink += Abc_Clock() - clk;
    // verify pattern using ternary simulation
//clk = Abc_Clock();
//    Cec_ManPatVerifyPattern( p->pAig, pObj, vPat );
//pMan->timeVerify += Abc_Clock() - clk;
    // sort pattern
//clk = Abc_Clock();
    Vec_IntSort( vPat, 0 );
//pMan->timeSort += Abc_Clock() - clk;
    // save pattern
    Cec_ManPatStore( pMan, vPat );
    pMan->timeTotal += Abc_Clock() - clkTotal;
}
void Cec_ManPatSavePatternCSat( Cec_ManPat_t * pMan, Vec_Int_t * vPat )
{
    // sort pattern
    Vec_IntSort( vPat, 0 );
    // save pattern
    Cec_ManPatStore( pMan, vPat );
}

/**Function*************************************************************

  Synopsis    [Packs patterns into array of simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

*************************************`**********************************/
int Cec_ManPatCollectTry( Vec_Ptr_t * vInfo, Vec_Ptr_t * vPres, int iBit, int * pLits, int nLits )
{
    unsigned * pInfo, * pPres;
    int i;
    for ( i = 0; i < nLits; i++ )
    {
        pInfo = (unsigned *)Vec_PtrEntry(vInfo, Abc_Lit2Var(pLits[i]));
        pPres = (unsigned *)Vec_PtrEntry(vPres, Abc_Lit2Var(pLits[i]));
        if ( Abc_InfoHasBit( pPres, iBit ) && 
             Abc_InfoHasBit( pInfo, iBit ) == Abc_LitIsCompl(pLits[i]) )
             return 0;
    }
    for ( i = 0; i < nLits; i++ )
    {
        pInfo = (unsigned *)Vec_PtrEntry(vInfo, Abc_Lit2Var(pLits[i]));
        pPres = (unsigned *)Vec_PtrEntry(vPres, Abc_Lit2Var(pLits[i]));
        Abc_InfoSetBit( pPres, iBit );
        if ( Abc_InfoHasBit( pInfo, iBit ) == Abc_LitIsCompl(pLits[i]) )
            Abc_InfoXorBit( pInfo, iBit );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Packs patterns into array of simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Cec_ManPatCollectPatterns( Cec_ManPat_t *  pMan, int nInputs, int nWordsInit )
{
    Vec_Int_t * vPat = pMan->vPattern1;
    Vec_Ptr_t * vInfo, * vPres;
    int k, kMax = -1, nPatterns = 0;
    int iStartOld = pMan->iStart;
    int nWords = nWordsInit;
    int nBits = 32 * nWords;
    abctime clk = Abc_Clock();
    vInfo = Vec_PtrAllocSimInfo( nInputs, nWords );
    Gia_ManRandomInfo( vInfo, 0, 0, nWords );
    vPres = Vec_PtrAllocSimInfo( nInputs, nWords );
    Vec_PtrCleanSimInfo( vPres, 0, nWords );
    while ( pMan->iStart < Vec_StrSize(pMan->vStorage) )
    {
        nPatterns++;
        Cec_ManPatRestore( pMan, vPat );
        for ( k = 1; k < nBits; k++, k += ((k % (32 * nWordsInit)) == 0) )
            if ( Cec_ManPatCollectTry( vInfo, vPres, k, (int *)Vec_IntArray(vPat), Vec_IntSize(vPat) ) )
                break;
        kMax = Abc_MaxInt( kMax, k );
        if ( k == nBits-1 )
        {
            Vec_PtrReallocSimInfo( vInfo );
            Gia_ManRandomInfo( vInfo, 0, nWords, 2*nWords );
            Vec_PtrReallocSimInfo( vPres );
            Vec_PtrCleanSimInfo( vPres, nWords, 2*nWords );
            nWords *= 2;
            nBits *= 2;
        }
    }
    Vec_PtrFree( vPres );
    pMan->nSeries = Vec_PtrReadWordsSimInfo(vInfo) / nWordsInit;
    pMan->timePack += Abc_Clock() - clk;
    pMan->timeTotal += Abc_Clock() - clk;
    pMan->iStart = iStartOld;
    if ( pMan->fVerbose )
    {
        Abc_Print( 1, "Total = %5d. Max used = %5d. Full = %5d. Series = %d. ", 
            nPatterns, kMax, nWordsInit*32, pMan->nSeries );
        ABC_PRT( "Time", Abc_Clock() - clk );
        Cec_ManPatPrintStats( pMan );
    }
    return vInfo;
}


/**Function*************************************************************

  Synopsis    [Packs patterns into array of simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Cec_ManPatPackPatterns( Vec_Int_t * vCexStore, int nInputs, int nRegs, int nWordsInit )
{
    Vec_Int_t * vPat;
    Vec_Ptr_t * vInfo, * vPres;
    int k, nSize, iStart, kMax = 0, nPatterns = 0;
    int nWords = nWordsInit;
    int nBits = 32 * nWords;
//    int RetValue;
    assert( nRegs <= nInputs );
    vPat  = Vec_IntAlloc( 100 );

    vInfo = Vec_PtrAllocSimInfo( nInputs, nWords );
    Vec_PtrCleanSimInfo( vInfo, 0, nWords );
    Gia_ManRandomInfo( vInfo, nRegs, 0, nWords );

    vPres = Vec_PtrAllocSimInfo( nInputs, nWords );
    Vec_PtrCleanSimInfo( vPres, 0, nWords );
    iStart = 0;
    while ( iStart < Vec_IntSize(vCexStore) )
    {
        nPatterns++;
        // skip the output number
        iStart++;
        // get the number of items
        nSize = Vec_IntEntry( vCexStore, iStart++ );
        if ( nSize <= 0 )
            continue;
        // extract pattern
        Vec_IntClear( vPat );
        for ( k = 0; k < nSize; k++ )
            Vec_IntPush( vPat, Vec_IntEntry( vCexStore, iStart++ ) );
        // add pattern to storage
        for ( k = 1; k < nBits; k++, k += ((k % (32 * nWordsInit)) == 0) )
            if ( Cec_ManPatCollectTry( vInfo, vPres, k, (int *)Vec_IntArray(vPat), Vec_IntSize(vPat) ) )
                break;

//        k = kMax + 1;
//        RetValue = Cec_ManPatCollectTry( vInfo, vPres, k, (int *)Vec_IntArray(vPat), Vec_IntSize(vPat) );
//        assert( RetValue == 1 );

        kMax = Abc_MaxInt( kMax, k );
        if ( k == nBits-1 )
        {
            Vec_PtrReallocSimInfo( vInfo );
            Vec_PtrCleanSimInfo( vInfo, nWords, 2*nWords );
            Gia_ManRandomInfo( vInfo, nRegs, nWords, 2*nWords );

            Vec_PtrReallocSimInfo( vPres );
            Vec_PtrCleanSimInfo( vPres, nWords, 2*nWords );
            nWords *= 2;
            nBits *= 2;
        }
    }
//    Abc_Print( 1, "packed %d patterns into %d vectors (out of %d)\n", nPatterns, kMax, nBits );
    Vec_PtrFree( vPres );
    Vec_IntFree( vPat );
    return vInfo;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

