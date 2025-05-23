/**CFile****************************************************************

  FileName    [ifCount.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifCount.h,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__map__if__if_Count_h
#define ABC__map__if__if_Count_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////
 
////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int If_LogCreateAnd( Vec_Int_t * vAig, int iLit0, int iLit1, int nSuppAll )
{
    int iObjId = Vec_IntSize(vAig)/2 + nSuppAll;
    assert( Abc_Lit2Var(iLit0) != Abc_Lit2Var(iLit1) );
    Vec_IntPush( vAig, iLit0 );
    Vec_IntPush( vAig, iLit1 );
    return Abc_Var2Lit( iObjId, 0 );
}
static inline int If_LogCreateMux( Vec_Int_t * vAig, int iLitC, int iLit1, int iLit0, int nSuppAll )
{
    int iFanLit0 = If_LogCreateAnd( vAig, iLitC, iLit1, nSuppAll );
    int iFanLit1 = If_LogCreateAnd( vAig, Abc_LitNot(iLitC), iLit0, nSuppAll );
    int iResLit  = If_LogCreateAnd( vAig, Abc_LitNot(iFanLit0), Abc_LitNot(iFanLit1), nSuppAll );
    return Abc_LitNot(iResLit);
}
static inline int If_LogCreateXor( Vec_Int_t * vAig, int iLit0, int iLit1, int nSuppAll )
{
    return If_LogCreateMux( vAig, iLit0, Abc_LitNot(iLit1), iLit1, nSuppAll );
}
static inline int If_LogCreateAndXor( Vec_Int_t * vAig, int iLit0, int iLit1, int nSuppAll, int fXor )
{
    return fXor ? If_LogCreateXor(vAig, iLit0, iLit1, nSuppAll) : If_LogCreateAnd(vAig, iLit0, iLit1, nSuppAll);
}
static inline int If_LogCreateAndXorMulti( Vec_Int_t * vAig, int * pFaninLits, int nFanins, int nSuppAll, int fXor )
{
    int i;
    assert( nFanins > 0 );
    for ( i = nFanins - 1; i > 0; i-- )
        pFaninLits[i-1] = If_LogCreateAndXor( vAig, pFaninLits[i], pFaninLits[i-1], nSuppAll, fXor );
    return pFaninLits[0];
}
static inline int If_LogCounterAddAig( int * pTimes, int * pnTimes, int * pFaninLits, int Num, int iLit, Vec_Int_t * vAig, int nSuppAll, int fXor, int fXorFunc )
{
    int nTimes = *pnTimes;
    if ( vAig )
        pFaninLits[nTimes] = iLit;
    pTimes[nTimes++] = Num;
    if ( nTimes > 1 )
    {
        int i, k;
        for ( k = nTimes-1; k > 0; k-- )
        {
            if ( pTimes[k] < pTimes[k-1] )
                break;
            if ( pTimes[k] > pTimes[k-1] )
            { 
                ABC_SWAP( int, pTimes[k], pTimes[k-1] ); 
                if ( vAig )
                    ABC_SWAP( int, pFaninLits[k], pFaninLits[k-1] ); 
                continue; 
            }
            pTimes[k-1] += 1 + fXor;
            if ( vAig )
                pFaninLits[k-1] = If_LogCreateAndXor( vAig, pFaninLits[k], pFaninLits[k-1], nSuppAll, fXorFunc );
            for ( nTimes--, i = k; i < nTimes; i++ )
            {
                pTimes[i] = pTimes[i+1];
                if ( vAig )
                    pFaninLits[i] = pFaninLits[i+1];
            }
        }
    }
    assert( nTimes > 0 );
    *pnTimes = nTimes;
    return pTimes[0] + (nTimes > 1 ? 1 + fXor : 0);
}
static inline int If_LogCounterDelayXor( int * pTimes, int nTimes )
{
    int i;
    assert( nTimes > 0 );
    for ( i = nTimes - 1; i > 0; i-- )
        pTimes[i-1] = 2 + Abc_MaxInt( pTimes[i], pTimes[i-1] );
    return pTimes[0];
}


/**Function*************************************************************

  Synopsis    [Compute delay/area profile of the structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int  If_CutPinDelayGet( word D, int v )           { assert(v >= 0 && v < 16); return (int)((D >> (v << 2)) & 0xF);                    }
static inline void If_CutPinDelaySet( word * pD, int v, int d ) { assert(v >= 0 && v < 16); assert(d >= 0 && d < 16); *pD |= ((word)d << (v << 2)); }
static inline word If_CutPinDelayInit( int v )                  { assert(v >= 0 && v < 16); return (word)1 << (v << 2);                             }
static inline word If_CutPinDelayMax( word D1, word D2, int nVars, int AddOn )
{
    int v, Max;
    word D = 0;
    for ( v = 0; v < nVars; v++ )
        if ( (Max = Abc_MaxInt(If_CutPinDelayGet(D1, v), If_CutPinDelayGet(D2, v))) )
            If_CutPinDelaySet( &D, v, Abc_MinInt(Max + AddOn, 15) );
    return D;
}
static inline word If_CutPinDelayDecrement( word D1, int nVars )
{
    int v;
    word D = 0;
    for ( v = 0; v < nVars; v++ )
        if ( If_CutPinDelayGet(D1, v) )
            If_CutPinDelaySet( &D, v, If_CutPinDelayGet(D1, v) - 1 );
    return D;
}
static inline int If_CutPinDelayEqual( word D1, word D2, int nVars ) // returns 1 if D1 has the same delays than D2
{
    int v;
    for ( v = 0; v < nVars; v++ )
        if ( If_CutPinDelayGet(D1, v) != If_CutPinDelayGet(D2, v) )
            return 0;
    return 1;
}
static inline int If_CutPinDelayDom( word D1, word D2, int nVars ) // returns 1 if D1 has the same or smaller delays than D2
{
    int v;
    for ( v = 0; v < nVars; v++ )
        if ( If_CutPinDelayGet(D1, v) > If_CutPinDelayGet(D2, v) )
            return 0;
    return 1;
}
static inline void If_CutPinDelayTranslate( word D, int nVars, char * pPerm ) 
{
    int v, Delay;
    for ( v = 0; v < nVars; v++ )
    {
        Delay = If_CutPinDelayGet(D, v);
        assert( Delay > 1 );
        pPerm[v] = Delay - 1;
    }
}
static inline void If_CutPinDelayPrint( word D, int nVars )
{
    int v;
    printf( "Delay profile = {" );
    for ( v = 0; v < nVars; v++ )
        printf( " %d", If_CutPinDelayGet(D, v) );
    printf( " }\n" );
}
static inline int If_LogCounterPinDelays( int * pTimes, int * pnTimes, word * pPinDels, int Num, word PinDel, int nSuppAll, int fXor )
{
    int nTimes = *pnTimes;
    pPinDels[nTimes] = PinDel;
    pTimes[nTimes++] = Num;
    if ( nTimes > 1 )
    {
        int i, k;
        for ( k = nTimes-1; k > 0; k-- )
        {
            if ( pTimes[k] < pTimes[k-1] )
                break;
            if ( pTimes[k] > pTimes[k-1] )
            { 
                ABC_SWAP( int, pTimes[k], pTimes[k-1] ); 
                ABC_SWAP( word, pPinDels[k], pPinDels[k-1] ); 
                continue; 
            }
            pTimes[k-1] += 1 + fXor;
            pPinDels[k-1] = If_CutPinDelayMax( pPinDels[k], pPinDels[k-1], nSuppAll, 1 + fXor );
            for ( nTimes--, i = k; i < nTimes; i++ )
            {
                pTimes[i] = pTimes[i+1];
                pPinDels[i] = pPinDels[i+1];
            }
        }
    }
    assert( nTimes > 0 );
    *pnTimes = nTimes;
    return pTimes[0] + (nTimes > 1 ? 1 + fXor : 0);
}
static inline word If_LogPinDelaysMulti( word * pPinDels, int nFanins, int nSuppAll, int fXor )
{
    int i;
    assert( nFanins > 0 );
    for ( i = nFanins - 1; i > 0; i-- )
        pPinDels[i-1] = If_CutPinDelayMax( pPinDels[i], pPinDels[i-1], nSuppAll, 1 + fXor );
    return pPinDels[0];
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word If_AigVerifyArray( Vec_Int_t * vAig, int nLeaves )
{
    assert( Vec_IntSize(vAig) > 0 );
    assert( Vec_IntEntryLast(vAig) < 2 );
    if ( Vec_IntSize(vAig) == 1 ) // const
        return Vec_IntEntry(vAig, 0) ? ~((word)0) : 0;
    if ( Vec_IntSize(vAig) == 2 ) // variable
    {
        assert( Vec_IntEntry(vAig, 0) == 0 );
        return Vec_IntEntry(vAig, 1) ? ~s_Truths6[0] : s_Truths6[0];
    }
    else
    {
        word Truth0 = 0, Truth1 = 0, TruthR;
        int i, iVar0, iVar1, iLit0, iLit1;
        assert( Vec_IntSize(vAig) & 1 );
        Vec_IntForEachEntryDouble( vAig, iLit0, iLit1, i )
        {
            iVar0 = Abc_Lit2Var( iLit0 );
            iVar1 = Abc_Lit2Var( iLit1 );
            Truth0 = iVar0 < nLeaves ? s_Truths6[iVar0] : Vec_WrdEntry( (Vec_Wrd_t *)vAig, iVar0 - nLeaves );
            Truth1 = iVar1 < nLeaves ? s_Truths6[iVar1] : Vec_WrdEntry( (Vec_Wrd_t *)vAig, iVar1 - nLeaves );
            if ( Abc_LitIsCompl(iLit0) )
                Truth0 = ~Truth0;
            if ( Abc_LitIsCompl(iLit1) )
                Truth1 = ~Truth1;
            assert( (i & 1) == 0 );
            Vec_WrdWriteEntry( (Vec_Wrd_t *)vAig, Abc_Lit2Var(i), Truth0 & Truth1 );  // overwriting entries
        }
        assert( i == Vec_IntSize(vAig) - 1 );
        TruthR = Truth0 & Truth1;
        if ( Vec_IntEntry(vAig, i) )
            TruthR = ~TruthR;
        Vec_IntClear( vAig ); // useless
        return TruthR;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void If_AigPrintArray( Vec_Int_t * vAig, int nLeaves )
{
    assert( Vec_IntSize(vAig) > 0 );
    assert( Vec_IntEntryLast(vAig) < 2 );
    if ( Vec_IntSize(vAig) == 1 ) // const
        printf( "Const %d\n", Vec_IntEntry(vAig, 0) );
    else if ( Vec_IntSize(vAig) == 2 ) // variable
        printf( "Variable %s\n", Vec_IntEntry(vAig, 1) ? "Compl" : "" );
    else
    {
        int i, iLit0, iLit1;
        assert( Vec_IntSize(vAig) & 1 );
        Vec_IntForEachEntryDouble( vAig, iLit0, iLit1, i )
            printf( "%d %d\n", iLit0, iLit1 );
        assert( i == Vec_IntSize(vAig) - 1 );
        printf( "%s\n", Vec_IntEntry(vAig, i) ? "Compl" : "" );
    }
}


/**Function*************************************************************

  Synopsis    [Naive implementation of log-counter.]

  Description [Incrementally computes [log2(SUMi(2^di)).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int If_LogCounter64Eval( word Count )
{
    int n = ((Count & (Count - 1)) > 0) ? -1 : 0;
    assert( Count > 0 );
    if ( (Count & ABC_CONST(0xFFFFFFFF00000000)) == 0 ) { n += 32; Count <<= 32; }
    if ( (Count & ABC_CONST(0xFFFF000000000000)) == 0 ) { n += 16; Count <<= 16; }
    if ( (Count & ABC_CONST(0xFF00000000000000)) == 0 ) { n +=  8; Count <<=  8; }
    if ( (Count & ABC_CONST(0xF000000000000000)) == 0 ) { n +=  4; Count <<=  4; }
    if ( (Count & ABC_CONST(0xC000000000000000)) == 0 ) { n +=  2; Count <<=  2; }
    if ( (Count & ABC_CONST(0x8000000000000000)) == 0 ) { n++; }
    return 63 - n;
}
static word If_LogCounter64Add( word Count, int Num )
{
    assert( Num < 48 );
    return Count + (((word)1) << Num);
}

/**Function*************************************************************

  Synopsis    [Implementation of log-counter.]

  Description [Incrementally computes [log2(SUMi(2^di)).
  Supposed to work correctly up to 16 entries.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int If_LogCounter32Eval( unsigned Count, int Start )
{
    int n = (Abc_LitIsCompl(Start) || (Count & (Count - 1)) > 0) ? -1 : 0;
    assert( Count > 0 );
    if ( (Count & 0xFFFF0000) == 0 ) { n += 16; Count <<= 16; }
    if ( (Count & 0xFF000000) == 0 ) { n +=  8; Count <<=  8; }
    if ( (Count & 0xF0000000) == 0 ) { n +=  4; Count <<=  4; }
    if ( (Count & 0xC0000000) == 0 ) { n +=  2; Count <<=  2; }
    if ( (Count & 0x80000000) == 0 ) { n++; }
    return Abc_Lit2Var(Start) + 31 - n;
}
static unsigned If_LogCounter32Add( unsigned Count, int * pStart, int Num )
{
    int Start = Abc_Lit2Var(*pStart);
    if ( Num < Start )
    {
        *pStart |= 1;
        return Count;
    }
    if ( Num > Start + 16 )
    {
        int Shift = Num - (Start + 16);
        if ( !Abc_LitIsCompl(*pStart) && (Shift >= 32 ? Count : Count & ~(~0 << Shift)) > 0 )
            *pStart |= 1;
        Count >>= Shift;
        Start += Shift;
        *pStart = Abc_Var2Lit( Start, Abc_LitIsCompl(*pStart) );
        assert( Num <= Start + 16 );
    }
    return Count + (1 << (Num-Start));
}

/**Function*************************************************************

  Synopsis    [Testing of the counter]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
void If_LogCounterTest2()
{
    word Count64 = 0;

    unsigned Count = 0; 
    int Start = 0;

    int Result, Result64;

    Count = If_LogCounter32Add( Count, &Start, 39 );
    Count = If_LogCounter32Add( Count, &Start, 35 );
    Count = If_LogCounter32Add( Count, &Start, 35 );
    Count = If_LogCounter32Add( Count, &Start, 36 );
    Count = If_LogCounter32Add( Count, &Start, 37 );
    Count = If_LogCounter32Add( Count, &Start, 38 );
    Count = If_LogCounter32Add( Count, &Start, 40 );
    Count = If_LogCounter32Add( Count, &Start, 1 );
    Count = If_LogCounter32Add( Count, &Start, 41 );
    Count = If_LogCounter32Add( Count, &Start, 42 );

    Count64 = If_LogCounter64Add( Count64, 1 );
    Count64 = If_LogCounter64Add( Count64, 35 );
    Count64 = If_LogCounter64Add( Count64, 35 );
    Count64 = If_LogCounter64Add( Count64, 36 );
    Count64 = If_LogCounter64Add( Count64, 37 );
    Count64 = If_LogCounter64Add( Count64, 38 );
    Count64 = If_LogCounter64Add( Count64, 39 );
    Count64 = If_LogCounter64Add( Count64, 40 );
    Count64 = If_LogCounter64Add( Count64, 41 );
    Count64 = If_LogCounter64Add( Count64, 42 );

    Result = If_LogCounter32Eval( Count, Start );
    Result64 = If_LogCounter64Eval( Count64 );

    printf( "%d  %d\n", Result, Result64 );
}
*/

/**Function*************************************************************

  Synopsis    [Adds the number to the numbers stored.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int If_LogCounterAdd( int * pTimes, int * pnTimes, int Num, int fXor )
{
    int nTimes = *pnTimes;
    pTimes[nTimes++] = Num;
    if ( nTimes > 1 )
    {
        int i, k;
        for ( k = nTimes-1; k > 0; k-- )
        {
            if ( pTimes[k] < pTimes[k-1] )
                break;
            if ( pTimes[k] > pTimes[k-1] )
            { 
                ABC_SWAP( int, pTimes[k], pTimes[k-1] ); 
                continue; 
            }
            pTimes[k-1] += 1 + fXor;
            for ( nTimes--, i = k; i < nTimes; i++ )
                pTimes[i] = pTimes[i+1];
        }
    }
    assert( nTimes > 0 );
    *pnTimes = nTimes;
    return pTimes[0] + (nTimes > 1 ? 1 + fXor : 0);
}

/**Function*************************************************************

  Synopsis    [Testing of the counter]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
void If_LogCounterTest()
{
    int pArray[10] = { 1, 2, 4, 5, 6, 3, 1 };
    int i, nSize = 4;

    word Count64 = 0;
    int Result, Result64;

    int pTimes[100];
    int nTimes = 0;

    for ( i = 0; i < nSize; i++ )
        Count64 = If_LogCounter64Add( Count64, pArray[i] );
    Result64 = If_LogCounter64Eval( Count64 );

    for ( i = 0; i < nSize; i++ )
        Result = If_LogCounterAdd( pTimes, &nTimes, pArray[i], 0 );

    printf( "%d  %d\n", Result64, Result );
}
*/

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

