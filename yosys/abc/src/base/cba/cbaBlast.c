/**CFile****************************************************************

  FileName    [cbaBlast.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [Collapsing word-level design.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 21, 2015.]

  Revision    [$Id: cbaBlast.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cba.h"
#include "base/abc/abc.h"
#include "map/mio/mio.h"
#include "bool/dec/dec.h"
#include "base/main/mainInt.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Helper functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cba_NtkPrepareBits( Cba_Ntk_t * p )
{
    int i, nBits = 0;
    Cba_NtkCleanFonCopies( p );
    Cba_NtkForEachFon( p, i )
    {
        Cba_FonSetCopy( p, i, nBits );
        nBits += Cba_FonRangeSize( p, i );
    }
    return nBits;
}
int * Cba_VecCopy( Vec_Int_t * vOut, int * pArray, int nSize )
{
    int i; Vec_IntClear( vOut );
    for( i = 0; i < nSize; i++) 
        Vec_IntPush( vOut, pArray[i] );
    return Vec_IntArray( vOut );
}
int Cba_ReadHexDigit( char HexChar )
{
    if ( HexChar >= '0' && HexChar <= '9' )
        return HexChar - '0';
    if ( HexChar >= 'A' && HexChar <= 'F' )
        return HexChar - 'A' + 10;
    if ( HexChar >= 'a' && HexChar <= 'f' )
        return HexChar - 'a' + 10;
    assert( 0 ); // not a hexadecimal symbol
    return -1; // return value which makes no sense
}
void Cba_BlastConst( Cba_Ntk_t * p, Vec_Int_t * vOut, int iFon, int nTotal, int fSigned )
{
    char * pConst = Cba_NtkConst(p, Cba_FonConst(iFon));
    char * pLimit = pConst + strlen(pConst);
    int i, Number, nBits = atoi( pConst );
    assert( nBits <= nTotal );
    while ( *pConst >= '0' && *pConst <= '9' )
        pConst++;
    assert( *pConst == '\'' );
    pConst++;
    if ( *pConst == 's' ) // assume signedness is already used in setting fSigned
        pConst++;
    Vec_IntClear( vOut );
    if ( *pConst == 'b' )
    {
        while ( --pLimit > pConst )
            Vec_IntPush( vOut, *pLimit == '0' ? 0 : 1 );
    }
    else if ( *pConst == 'h' )
    {
        while ( --pLimit > pConst )
        {
            Number = Cba_ReadHexDigit( *pLimit );
            for ( i = 0; i < 4; i++ )
                Vec_IntPush( vOut, (Number >> i) & 1 );
        }
        if ( Vec_IntSize(vOut) > nTotal )
            Vec_IntShrink( vOut, nTotal );
    }
    else if ( *pConst == 'd' )
    {
        Number = atoi( pConst+1 );
        assert( Number <= 0x7FFFFFFF );
        for ( i = 0; i < 32; i++ )
            Vec_IntPush( vOut, (Number >> i) & 1 );
        if ( Vec_IntSize(vOut) > nTotal )
            Vec_IntShrink( vOut, nTotal );
    }
    else assert( 0 );
    if ( fSigned && Vec_IntSize(vOut) < nTotal )
        Vec_IntFillExtra( vOut, nTotal - Vec_IntSize(vOut), Vec_IntEntryLast(vOut) );
}
int * Cba_VecLoadFanins( Cba_Ntk_t * p, Vec_Int_t * vOut, int iFon, int * pFanins, int nFanins, int nTotal, int fSigned )
{
    assert( nFanins <= nTotal );
    if ( Cba_FonIsReal(iFon) )
    {
        int i, Fill = fSigned ? pFanins[nFanins-1] : 0;
        Vec_IntClear( vOut );
        for( i = 0; i < nTotal; i++) 
            Vec_IntPush( vOut, i < nFanins ? pFanins[i] : Fill );
    }
    else if ( Cba_FonIsConst(iFon) )
        Cba_BlastConst( p, vOut, iFon, nTotal, fSigned );
    else if ( iFon == 0 ) // undriven input
        Vec_IntFill( vOut, nTotal, 0 );
    else assert( 0 );
    assert( Vec_IntSize(vOut) == nTotal );
    return Vec_IntArray( vOut );
}
int Cba_NtkMuxTree_rec( Gia_Man_t * pNew, int * pCtrl, int nCtrl, Vec_Int_t * vData, int Shift )
{
    int iLit0, iLit1;
    if ( nCtrl == 0 )
        return Vec_IntEntry( vData, Shift );
    iLit0 = Cba_NtkMuxTree_rec( pNew, pCtrl, nCtrl-1, vData, Shift );
    iLit1 = Cba_NtkMuxTree_rec( pNew, pCtrl, nCtrl-1, vData, Shift + (1<<(nCtrl-1)) );
    return Gia_ManHashMux( pNew, pCtrl[nCtrl-1], iLit1, iLit0 );
}

/**Function*************************************************************

  Synopsis    [Bit blasting for specific operations.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cba_BlastShiftRight( Gia_Man_t * pNew, int * pNum, int nNum, int * pShift, int nShift, int fSticky, Vec_Int_t * vRes )
{
    int * pRes = Cba_VecCopy( vRes, pNum, nNum );
    int Fill = fSticky ? pNum[nNum-1] : 0;
    int i, j, fShort = 0;
    if ( nShift > 32 )
        nShift = 32;
    assert( nShift <= 32 );
    for( i = 0; i < nShift; i++ ) 
        for( j = 0; j < nNum - fSticky; j++ ) 
        {
            if( fShort || j + (1<<i) >= nNum ) 
            {
                pRes[j] = Gia_ManHashMux( pNew, pShift[i], Fill, pRes[j] );
                if ( (1<<i) > nNum ) 
                    fShort = 1;
            } 
            else 
                pRes[j] = Gia_ManHashMux( pNew, pShift[i], pRes[j+(1<<i)], pRes[j] );
        }
}
void Cba_BlastShiftLeft( Gia_Man_t * pNew, int * pNum, int nNum, int * pShift, int nShift, int fSticky, Vec_Int_t * vRes )
{
    int * pRes = Cba_VecCopy( vRes, pNum, nNum );
    int Fill = fSticky ? pNum[0] : 0;
    int i, j, fShort = 0;
    if ( nShift > 32 )
        nShift = 32;
    assert( nShift <= 32 );
    for( i = 0; i < nShift; i++ ) 
        for( j = nNum-1; j >= fSticky; j-- ) 
        {
            if( fShort || (1<<i) > j ) 
            {
                pRes[j] = Gia_ManHashMux( pNew, pShift[i], Fill, pRes[j] );
                if ( (1<<i) > nNum ) 
                    fShort = 1;
            } 
            else 
                pRes[j] = Gia_ManHashMux( pNew, pShift[i], pRes[j-(1<<i)], pRes[j] );
        }
}
void Cba_BlastRotateRight( Gia_Man_t * pNew, int * pNum, int nNum, int * pShift, int nShift, Vec_Int_t * vRes )
{
    int * pRes = Cba_VecCopy( vRes, pNum, nNum );
    int i, j, * pTemp = ABC_ALLOC( int, nNum );
    assert( nShift <= 32 );
    for( i = 0; i < nShift; i++, pRes = Cba_VecCopy(vRes, pTemp, nNum) ) 
        for( j = 0; j < nNum; j++ ) 
            pTemp[j] = Gia_ManHashMux( pNew, pShift[i], pRes[(j+(1<<i))%nNum], pRes[j] );
    ABC_FREE( pTemp );
}
void Cba_BlastRotateLeft( Gia_Man_t * pNew, int * pNum, int nNum, int * pShift, int nShift, Vec_Int_t * vRes )
{
    int * pRes = Cba_VecCopy( vRes, pNum, nNum );
    int i, j, * pTemp = ABC_ALLOC( int, nNum );
    assert( nShift <= 32 );
    for( i = 0; i < nShift; i++, pRes = Cba_VecCopy(vRes, pTemp, nNum) ) 
        for( j = 0; j < nNum; j++ ) 
        {
            int move = (j >= (1<<i)) ? (j-(1<<i))%nNum : (nNum - (((1<<i)-j)%nNum)) % nNum;
            pTemp[j] = Gia_ManHashMux( pNew, pShift[i], pRes[move], pRes[j] );
//            pTemp[j] = Gia_ManHashMux( pNew, pShift[i], pRes[((unsigned)(nNum-(1<<i)+j))%nNum], pRes[j] );
        }
    ABC_FREE( pTemp );
}
int Cba_BlastReduction( Gia_Man_t * pNew, int * pFans, int nFans, int Type )
{
    if ( Type == CBA_BOX_RAND )
    {
        int k, iLit = 1;
        for ( k = 0; k < nFans; k++ )
            iLit = Gia_ManHashAnd( pNew, iLit, pFans[k] );
        return iLit;
    }
    if ( Type == CBA_BOX_ROR )
    {
        int k, iLit = 0;
        for ( k = 0; k < nFans; k++ )
            iLit = Gia_ManHashOr( pNew, iLit, pFans[k] );
        return iLit;
    }
    if ( Type == CBA_BOX_RXOR )
    {
        int k, iLit = 0;
        for ( k = 0; k < nFans; k++ )
            iLit = Gia_ManHashXor( pNew, iLit, pFans[k] );
        return iLit;
    }
    assert( 0 );
    return -1;
}
int Cba_BlastLess2( Gia_Man_t * pNew, int * pArg0, int * pArg1, int nBits )
{
    int k, iKnown = 0, iRes = 0;
    for ( k = nBits - 1; k >= 0; k-- )
    {
        iRes   = Gia_ManHashMux( pNew, iKnown, iRes, Gia_ManHashAnd(pNew, Abc_LitNot(pArg0[k]), pArg1[k]) );
        iKnown = Gia_ManHashOr( pNew, iKnown, Gia_ManHashXor(pNew, pArg0[k], pArg1[k]) );
        if ( iKnown == 1 )
            break;
    }
    return iRes;
}
void Cba_BlastLess_rec( Gia_Man_t * pNew, int * pArg0, int * pArg1, int nBits, int * pYes, int * pNo )
{
    if ( nBits > 1 )
    {
        int Yes = Gia_ManHashAnd( pNew, Abc_LitNot(pArg0[nBits-1]), pArg1[nBits-1] ), YesR;
        int No  = Gia_ManHashAnd( pNew, Abc_LitNot(pArg1[nBits-1]), pArg0[nBits-1] ), NoR;
        if ( Yes == 1 || No == 1 )
        {
            *pYes = Yes;
            *pNo  = No;
            return;
        }
        Cba_BlastLess_rec( pNew, pArg0, pArg1, nBits-1, &YesR, &NoR );
        *pYes = Gia_ManHashOr( pNew, Yes, Gia_ManHashAnd(pNew, Abc_LitNot(No),  YesR) );
        *pNo  = Gia_ManHashOr( pNew, No,  Gia_ManHashAnd(pNew, Abc_LitNot(Yes), NoR ) );
        return;
    }
    assert( nBits == 1 );
    *pYes = Gia_ManHashAnd( pNew, Abc_LitNot(pArg0[0]), pArg1[0] );
    *pNo  = Gia_ManHashAnd( pNew, Abc_LitNot(pArg1[0]), pArg0[0] );
}
int Cba_BlastLess( Gia_Man_t * pNew, int * pArg0, int * pArg1, int nBits )
{
    int Yes, No;
    if ( nBits == 0 )
        return 0;
    Cba_BlastLess_rec( pNew, pArg0, pArg1, nBits, &Yes, &No );
    return Yes;
}
int Cba_BlastLessSigned( Gia_Man_t * pNew, int * pArg0, int * pArg1, int nBits )
{
    int iDiffSign = Gia_ManHashXor( pNew, pArg0[nBits-1], pArg1[nBits-1] );
    return Gia_ManHashMux( pNew, iDiffSign, pArg0[nBits-1], Cba_BlastLess(pNew, pArg0, pArg1, nBits-1) );
}
void Cba_BlastFullAdder( Gia_Man_t * pNew, int a, int b, int c, int * pc, int * ps )
{
    int fUseXor = 0;
    if ( fUseXor )
    {
        int Xor  = Gia_ManHashXor(pNew, a, b);
        int And1 = Gia_ManHashAnd(pNew, a, b);
        int And2 = Gia_ManHashAnd(pNew, c, Xor);
        *ps      = Gia_ManHashXor(pNew, c, Xor);
        *pc      = Gia_ManHashOr (pNew, And1, And2);
    }
    else
    {
        int And1 = Gia_ManHashAnd(pNew, a, b);
        int And1_= Gia_ManHashAnd(pNew, Abc_LitNot(a), Abc_LitNot(b));
        int Xor  = Abc_LitNot(Gia_ManHashOr(pNew, And1, And1_));
        int And2 = Gia_ManHashAnd(pNew, c, Xor);
        int And2_= Gia_ManHashAnd(pNew, Abc_LitNot(c), Abc_LitNot(Xor));
        *ps      = Abc_LitNot(Gia_ManHashOr(pNew, And2, And2_));
        *pc      = Gia_ManHashOr (pNew, And1, And2);
    }
}
int Cba_BlastAdder( Gia_Man_t * pNew, int Carry, int * pAdd0, int * pAdd1, int nBits ) // result is in pAdd0
{
    int b;
    for ( b = 0; b < nBits; b++ )
        Cba_BlastFullAdder( pNew, pAdd0[b], pAdd1[b], Carry, &Carry, &pAdd0[b] );
    return Carry;
}
void Cba_BlastSubtract( Gia_Man_t * pNew, int * pAdd0, int * pAdd1, int nBits ) // result is in pAdd0
{
    int b, Carry = 1;
    for ( b = 0; b < nBits; b++ )
        Cba_BlastFullAdder( pNew, pAdd0[b], Abc_LitNot(pAdd1[b]), Carry, &Carry, &pAdd0[b] );
}
void Cba_BlastMinus( Gia_Man_t * pNew, int * pNum, int nNum, Vec_Int_t * vRes )
{
    int * pRes  = Cba_VecCopy( vRes, pNum, nNum );
    int i, invert = 0;
    for ( i = 0; i < nNum; i++ )
    {
        pRes[i] = Gia_ManHashMux( pNew, invert, Abc_LitNot(pRes[i]), pRes[i] );
        invert = Gia_ManHashOr( pNew, invert, pNum[i] );    
    }
}
void Cba_BlastMultiplier2( Gia_Man_t * pNew, int * pArg0, int * pArg1, int nBits, Vec_Int_t * vTemp, Vec_Int_t * vRes )
{
    int i, j;
    Vec_IntFill( vRes, nBits, 0 );
    for ( i = 0; i < nBits; i++ )
    {
        Vec_IntFill( vTemp, i, 0 );
        for ( j = 0; Vec_IntSize(vTemp) < nBits; j++ )
            Vec_IntPush( vTemp, Gia_ManHashAnd(pNew, pArg0[j], pArg1[i]) );
        assert( Vec_IntSize(vTemp) == nBits );
        Cba_BlastAdder( pNew, 0, Vec_IntArray(vRes), Vec_IntArray(vTemp), nBits );
    }
}
void Cba_BlastFullAdderCtrl( Gia_Man_t * pNew, int a, int ac, int b, int c, int * pc, int * ps, int fNeg )
{
    int And  = Abc_LitNotCond( Gia_ManHashAnd(pNew, a, ac), fNeg );
    Cba_BlastFullAdder( pNew, And, b, c, pc, ps );
}
void Cba_BlastFullAdderSubtr( Gia_Man_t * pNew, int a, int b, int c, int * pc, int * ps, int fSub )
{
    Cba_BlastFullAdder( pNew, Gia_ManHashXor(pNew, a, fSub), b, c, pc, ps );
}
void Cba_BlastMultiplier( Gia_Man_t * pNew, int * pArgA, int * pArgB, int nArgA, int nArgB, Vec_Int_t * vTemp, Vec_Int_t * vRes, int fSigned )
{
    int * pRes, * pArgC, * pArgS, a, b, Carry = fSigned;
    assert( nArgA > 0 && nArgB > 0 );
    assert( fSigned == 0 || fSigned == 1 );
    // prepare result
    Vec_IntFill( vRes, nArgA + nArgB, 0 );
    pRes = Vec_IntArray( vRes );
    // prepare intermediate storage
    Vec_IntFill( vTemp, 2 * nArgA, 0 );
    pArgC = Vec_IntArray( vTemp );
    pArgS = pArgC + nArgA;
    // create matrix
    for ( b = 0; b < nArgB; b++ )
        for ( a = 0; a < nArgA; a++ )
            Cba_BlastFullAdderCtrl( pNew, pArgA[a], pArgB[b], pArgS[a], pArgC[a], 
                &pArgC[a], a ? &pArgS[a-1] : &pRes[b], fSigned && ((a+1 == nArgA) ^ (b+1 == nArgB)) );
    // final addition
    pArgS[nArgA-1] = fSigned;
    for ( a = 0; a < nArgA; a++ )
        Cba_BlastFullAdderCtrl( pNew, 1, pArgC[a], pArgS[a], Carry, &Carry, &pRes[nArgB+a], 0 );
}
void Cba_BlastDivider( Gia_Man_t * pNew, int * pNum, int nNum, int * pDiv, int nDiv, int fQuo, Vec_Int_t * vRes )
{
    int * pRes  = Cba_VecCopy( vRes, pNum, nNum );
    int * pQuo  = ABC_ALLOC( int, nNum );
    int * pTemp = ABC_ALLOC( int, nNum );
    int i, j, known, borrow, y_bit, top_bit;
    assert( nNum == nDiv );
    for ( j = nNum - 1; j >= 0; j-- ) 
    {
        known = 0;
        for ( i = nNum - 1; i > nNum - 1 - j; i-- ) 
        {
            known = Gia_ManHashOr( pNew, known, pDiv[i] );
            if( known == 1 ) 
                break;
        }
        pQuo[j] = known;
        for ( i = nNum - 1; i >= 0; i-- ) 
        {
            if ( known == 1 ) 
                break;
            y_bit = (i >= j) ? pDiv[i-j] : 0;
            pQuo[j] = Gia_ManHashMux( pNew, known, pQuo[j], Gia_ManHashAnd( pNew, y_bit, Abc_LitNot(pRes[i]) ) );
            known = Gia_ManHashOr( pNew, known, Gia_ManHashXor(pNew, y_bit, pRes[i]));
        }
        pQuo[j] = Abc_LitNot(pQuo[j]);
        if ( pQuo[j] == 0 )
            continue;
        borrow = 0;
        for ( i = 0; i < nNum; i++ ) 
        {
            top_bit  = Gia_ManHashMux( pNew, borrow, Abc_LitNot(pRes[i]), pRes[i] );
            y_bit    = (i >= j) ? pDiv[i-j] : 0;
            borrow   = Gia_ManHashMux( pNew, pRes[i], Gia_ManHashAnd(pNew, borrow, y_bit), Gia_ManHashOr(pNew, borrow, y_bit) );
            pTemp[i] = Gia_ManHashXor( pNew, top_bit, y_bit );
        }
        if ( pQuo[j] == 1 ) 
            Cba_VecCopy( vRes, pTemp, nNum );
        else 
            for( i = 0; i < nNum; i++ ) 
                pRes[i] = Gia_ManHashMux( pNew, pQuo[j], pTemp[i], pRes[i] );
    }
    ABC_FREE( pTemp );
    if ( fQuo )
        Cba_VecCopy( vRes, pQuo, nNum );
    ABC_FREE( pQuo );
}
// non-restoring divider
void Cba_BlastDivider2( Gia_Man_t * pNew, int * pNum, int nNum, int * pDiv, int nDiv, int fQuo, Vec_Int_t * vRes )
{
    int i, * pRes  = Vec_IntArray(vRes);
    int k, * pQuo  = ABC_ALLOC( int, nNum );
    assert( nNum > 0 && nDiv > 0 );
    assert( Vec_IntSize(vRes) < nNum + nDiv );
    for ( i = 0; i < nNum + nDiv; i++ )
        pRes[i] = i < nNum ? pNum[i] : 0;
    for ( i = nNum-1; i >= 0; i-- )
    {
        int Cntrl = i == nNum-1 ? 1 : pQuo[i+1];
        int Carry = Cntrl;
        for ( k = 0; k <= nDiv; k++ )
            Cba_BlastFullAdderSubtr( pNew, k < nDiv ? pDiv[k] : 0, pRes[i+k], Carry, &Carry, &pRes[i+k], Cntrl );
        pQuo[i] = Abc_LitNot(pRes[i+nDiv]);
    }
    if ( fQuo )
        Cba_VecCopy( vRes, pQuo, nNum );
    else
    {
        int Carry = 0, Temp;
        for ( k = 0; k < nDiv; k++ )
        {
            Cba_BlastFullAdder( pNew, pDiv[k], pRes[k], Carry, &Carry, &Temp );
            pRes[k] = Gia_ManHashMux( pNew, pQuo[0], pRes[k], Temp );
        }
        Vec_IntShrink( vRes, nDiv );
    }
    ABC_FREE( pQuo );
}
void Cba_BlastDividerSigned( Gia_Man_t * pNew, int * pNum, int nNum, int * pDiv, int nDiv, int fQuo, Vec_Int_t * vRes )
{
    Vec_Int_t * vNum   = Vec_IntAlloc( nNum );
    Vec_Int_t * vDiv   = Vec_IntAlloc( nDiv );
    Vec_Int_t * vRes00 = Vec_IntAlloc( nNum + nDiv );
    Vec_Int_t * vRes01 = Vec_IntAlloc( nNum + nDiv );
    Vec_Int_t * vRes10 = Vec_IntAlloc( nNum + nDiv );
    Vec_Int_t * vRes11 = Vec_IntAlloc( nNum + nDiv );
    Vec_Int_t * vRes2  = Vec_IntAlloc( nNum );
    int k, iDiffSign   = Gia_ManHashXor( pNew, pNum[nNum-1], pDiv[nDiv-1] );
    Cba_BlastMinus( pNew, pNum, nNum, vNum );
    Cba_BlastMinus( pNew, pDiv, nDiv, vDiv );
    Cba_BlastDivider( pNew,               pNum, nNum,               pDiv, nDiv, fQuo, vRes00 );
    Cba_BlastDivider( pNew,               pNum, nNum, Vec_IntArray(vDiv), nDiv, fQuo, vRes01 );
    Cba_BlastDivider( pNew, Vec_IntArray(vNum), nNum,               pDiv, nDiv, fQuo, vRes10 );
    Cba_BlastDivider( pNew, Vec_IntArray(vNum), nNum, Vec_IntArray(vDiv), nDiv, fQuo, vRes11 );
    Vec_IntClear( vRes );
    for ( k = 0; k < nNum; k++ )
    {
        int Data0 =  Gia_ManHashMux( pNew, pDiv[nDiv-1], Vec_IntEntry(vRes01,k), Vec_IntEntry(vRes00,k) );
        int Data1 =  Gia_ManHashMux( pNew, pDiv[nDiv-1], Vec_IntEntry(vRes11,k), Vec_IntEntry(vRes10,k) );
        Vec_IntPush( vRes, Gia_ManHashMux(pNew, pNum[nNum-1], Data1, Data0) );
    }
    Cba_BlastMinus( pNew, Vec_IntArray(vRes), nNum, vRes2 );
    for ( k = 0; k < nNum; k++ )
        Vec_IntWriteEntry( vRes, k, Gia_ManHashMux(pNew, fQuo ? iDiffSign : pNum[nNum-1], Vec_IntEntry(vRes2,k), Vec_IntEntry(vRes,k)) );
    Vec_IntFree( vNum );
    Vec_IntFree( vDiv );
    Vec_IntFree( vRes00 );
    Vec_IntFree( vRes01 );
    Vec_IntFree( vRes10 );
    Vec_IntFree( vRes11 );
    Vec_IntFree( vRes2 );
    assert( Vec_IntSize(vRes) == nNum );
}
void Cba_BlastZeroCondition( Gia_Man_t * pNew, int * pDiv, int nDiv, Vec_Int_t * vRes )
{
    int i, Entry, iLit = Cba_BlastReduction( pNew, pDiv, nDiv, CBA_BOX_ROR );
    Vec_IntForEachEntry( vRes, Entry, i )
        Vec_IntWriteEntry( vRes, i, Gia_ManHashAnd(pNew, iLit, Entry) );
}
void Cba_BlastTable( Gia_Man_t * pNew, word * pTable, int * pFans, int nFans, int nOuts, Vec_Int_t * vRes )
{
    extern int Kit_TruthToGia( Gia_Man_t * pMan, unsigned * pTruth, int nVars, Vec_Int_t * vMemory, Vec_Int_t * vLeaves, int fHash );
    Vec_Int_t * vMemory = Vec_IntAlloc( 0 );
    Vec_Int_t vLeaves = { nFans, nFans, pFans };
    word * pTruth = ABC_ALLOC( word, Abc_TtWordNum(nFans) );
    int o, i, m, iLit, nMints = (1 << nFans);
    Vec_IntClear( vRes );
    for ( o = 0; o < nOuts; o++ )
    {
        // derive truth table
        memset( pTruth, 0, sizeof(word) * Abc_TtWordNum(nFans) );
        for ( m = 0; m < nMints; m++ )
            for ( i = 0; i < nFans; i++ )
                if ( Abc_TtGetBit( pTable, m * nFans + i ) )
                    Abc_TtSetBit( pTruth, m );
        // implement truth table
        if ( nFans < 6 )
            pTruth[0] = Abc_Tt6Stretch( pTruth[0], nFans );
        iLit = Kit_TruthToGia( pNew, (unsigned *)pTruth, nFans, vMemory, &vLeaves, 1 );
        Vec_IntPush( vRes, iLit );
    }
    Vec_IntFree( vMemory );
    ABC_FREE( pTruth );
}
void Cba_BlastPower( Gia_Man_t * pNew, int * pNum, int nNum, int * pExp, int nExp, Vec_Int_t * vTemp, Vec_Int_t * vRes )
{
    Vec_Int_t * vDegrees = Vec_IntAlloc( 2*nNum );
    Vec_Int_t * vResTemp = Vec_IntAlloc( 2*nNum );
    int i, * pDegrees = NULL, * pRes = Vec_IntArray(vRes);
    int k, * pResTemp = Vec_IntArray(vResTemp);
    Vec_IntFill( vRes, nNum, 0 );
    Vec_IntWriteEntry( vRes, 0, 1 );
    for ( i = 0; i < nExp; i++ )
    {
        if ( i == 0 )
            pDegrees = Cba_VecCopy( vDegrees, pNum, nNum );
        else
        {
            Cba_BlastMultiplier2( pNew, pDegrees, pDegrees, nNum, vTemp, vResTemp );
            pDegrees = Cba_VecCopy( vDegrees, pResTemp, nNum );
        }
        Cba_BlastMultiplier2( pNew, pRes, pDegrees, nNum, vTemp, vResTemp );
        for ( k = 0; k < nNum; k++ )
            pRes[k] = Gia_ManHashMux( pNew, pExp[i], pResTemp[k], pRes[k] );
    }
    Vec_IntFree( vResTemp );
    Vec_IntFree( vDegrees );
}
void Cba_BlastSqrt( Gia_Man_t * pNew, int * pNum, int nNum, Vec_Int_t * vTmp, Vec_Int_t * vRes )
{
    int * pRes, * pSum, * pSumP;
    int i, k, Carry = -1;
    assert( nNum % 2 == 0 );
    Vec_IntFill( vRes, nNum/2, 0 );
    Vec_IntFill( vTmp, 2*nNum, 0 );
    pRes = Vec_IntArray( vRes );
    pSum = Vec_IntArray( vTmp );
    pSumP = pSum + nNum;
    for ( i = 0; i < nNum/2; i++ )
    {
        pSumP[0] = pNum[nNum-2*i-2];
        pSumP[1] = pNum[nNum-2*i-1];
        for ( k = 0; k < i+1; k++ )
            pSumP[k+2] = pSum[k];
        for ( k = 0; k < i + 3; k++ )
        {
            if ( k >= 2 && k < i + 2 ) // middle ones
                Cba_BlastFullAdder( pNew, pSumP[k], Abc_LitNot(pRes[i-k+1]), Carry, &Carry, &pSum[k] );
            else
                Cba_BlastFullAdder( pNew, pSumP[k], Abc_LitNot(k ? Carry:1), 1,     &Carry, &pSum[k] );
            if ( k == 0 || k > i )
                Carry = Abc_LitNot(Carry);
        }
        pRes[i] = Abc_LitNot(Carry);
        for ( k = 0; k < i + 3; k++ )
            pSum[k] = Gia_ManHashMux( pNew, pRes[i], pSum[k], pSumP[k] );
    }
    Vec_IntReverseOrder( vRes );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Cba_NtkBlast( Cba_Ntk_t * p, int fSeq )
{
    int fUseOldMultiplierBlasting = 0;
    Gia_Man_t * pTemp, * pNew;
    Vec_Int_t * vTemp0, * vTemp1, * vTemp2, * vRes;
    Vec_Str_t * vInit = fSeq ? Vec_StrAlloc(100) : NULL;
    Vec_Int_t * vBits = &p->vFonBits;
    int nBits = Cba_NtkPrepareBits( p );
    int * pFans0, * pFans1, * pFans2;
    int nRange, nRange0, nRange1, nRange2;
    int Type, iFon, iFon0, iFon1, iFon2, fSigned01;
    int i, k, b, iFin, iObj, iLit, nAndPrev;
    Vec_IntClear( vBits );
    Vec_IntGrow( vBits, nBits );
    vTemp0 = Vec_IntAlloc( 1000 );
    vTemp1 = Vec_IntAlloc( 1000 );
    vTemp2 = Vec_IntAlloc( 1000 );
    vRes   = Vec_IntAlloc( 1000 );
    // clean AND-gate counters
    memset( p->pDesign->nAnds, 0, sizeof(int) * CBA_BOX_LAST );
    // create AIG manager
    pNew = Gia_ManStart( 5 * Cba_NtkObjNum(p) + 1000 );
    pNew->pName = Abc_UtilStrsav( Cba_ManName(p->pDesign) );
    Gia_ManHashAlloc( pNew );
    // blast in the topological order
    Cba_NtkForEachObj( p, i )
    {
        Type = Cba_ObjType(p, i);
        if ( Type == CBA_OBJ_PO )
            continue;
        assert( Vec_IntSize(vBits) == Cba_FonCopy(p, Cba_ObjFon0(p, i)) );
        nRange = Cba_ObjRangeSize(p, i); assert( nRange > 0 );
        if ( Cba_ObjIsPi(p, i) || Cba_ObjIsSeq(p, i) )
        {
            for ( k = 0; k < nRange; k++ )
                Vec_IntPush( vBits, Gia_ManAppendCi(pNew) );
            assert( Type == CBA_BOX_DFFCPL || Cba_ObjFonNum(p, i) == 1 );
            continue;
        }
        assert( Cba_ObjFinNum(p, i) > 0 );
        iFon0     = Cba_ObjFinNum(p, i) > 0 ? Cba_ObjFinFon(p, i, 0) : -1;
        iFon1     = Cba_ObjFinNum(p, i) > 1 ? Cba_ObjFinFon(p, i, 1) : -1;
        iFon2     = Cba_ObjFinNum(p, i) > 2 ? Cba_ObjFinFon(p, i, 2) : -1;
        nRange0   = Cba_ObjFinNum(p, i) > 0 ? Cba_FonRangeSize(p, iFon0) : -1; 
        nRange1   = Cba_ObjFinNum(p, i) > 1 ? Cba_FonRangeSize(p, iFon1) : -1; 
        nRange2   = Cba_ObjFinNum(p, i) > 2 ? Cba_FonRangeSize(p, iFon2) : -1; 
        pFans0    = (Cba_ObjFinNum(p, i) > 0 && Cba_FonIsReal(iFon0)) ? Vec_IntEntryP( vBits, Cba_FonCopy(p, iFon0) ) : NULL;
        pFans1    = (Cba_ObjFinNum(p, i) > 1 && Cba_FonIsReal(iFon1)) ? Vec_IntEntryP( vBits, Cba_FonCopy(p, iFon1) ) : NULL;
        pFans2    = (Cba_ObjFinNum(p, i) > 2 && Cba_FonIsReal(iFon2)) ? Vec_IntEntryP( vBits, Cba_FonCopy(p, iFon2) ) : NULL;
        fSigned01 = (Cba_ObjFinNum(p, i) > 1 && Cba_FonSigned(p, iFon0) && Cba_FonSigned(p, iFon1));
        nAndPrev  = Gia_ManAndNum(pNew);
        Vec_IntClear( vRes );
        if ( Type == CBA_BOX_SLICE )
        {
            int Left   = Cba_ObjLeft( p, i );
            int Right  = Cba_ObjRight( p, i );
            int Left0  = Cba_FonLeft( p, iFon0 );
            int Right0 = Cba_FonRight( p, iFon0 );
            if ( Left > Right )
            {
                assert( Left0 > Right0 );
                for ( k = Right; k <= Left; k++ )
                    Vec_IntPush( vRes, pFans0[k - Right0] );
            }
            else
            {
                assert( Left < Right && Left0 < Right0 );
                for ( k = Right; k >= Left; k-- )
                    Vec_IntPush( vRes, pFans0[k - Right0] );
            }
        }
        else if ( Type == CBA_BOX_CONCAT )
        {
            int iFinT, iFonT, nTotal = 0;
            Cba_ObjForEachFinFon( p, i, iFinT, iFonT, k )
                nTotal += Cba_FonRangeSize( p, iFonT );
            assert( nRange == nTotal );
            Cba_ObjForEachFinFon( p, i, iFinT, iFonT, k )
            {
                nRange0 = Cba_FonRangeSize( p, iFonT );
                pFans0  = Cba_FonIsReal(iFonT) ? Vec_IntEntryP( vBits, Cba_FonCopy(p, iFonT) ) : NULL;
                pFans0  = Cba_VecLoadFanins( p, vTemp0, iFonT, pFans0, nRange0, nRange0, Cba_FonSigned(p, iFonT) );
                for ( b = 0; b < nRange0; b++ )
                    Vec_IntPush( vRes, pFans0[b] );
            }
        }
        else if ( Type == CBA_BOX_BUF )
        {
            int nRangeMax = Abc_MaxInt( nRange0, nRange );
            int * pArg0 = Cba_VecLoadFanins( p, vTemp0, iFon0, pFans0, nRange0, nRangeMax, Cba_FonSigned(p, iFon0) );
            for ( k = 0; k < nRange; k++ )
                Vec_IntPush( vRes, pArg0[k] );
        }
        else if ( Type >= CBA_BOX_CF && Type <= CBA_BOX_CZ )
        {
            assert( 0 );
            //word * pTruth = (word *)Cba_ObjFanins(p, i);
            //for ( k = 0; k < nRange; k++ )
            //    Vec_IntPush( vRes, Abc_TtGetBit(pTruth, k) );
        }
        else if ( Type == CBA_BOX_MUX || Type == CBA_BOX_NMUX )
        {
            // It is strange and disturbing that Verilog standard treats these statements differently:
            // Statement 1:   
            //     assign o = i ? b : a;
            // Statement 2:
            //     always @( i or a or b )
            //       begin
            //         case ( i )
            //           0 : o = a ;
            //           1 : o = b ;
            //         endcase
            //       end
            // If a is signed and b is unsigned,  Statement 1 does not sign-extend a, while Statement 2 does.
            // The signedness of o does not matter.
            //
            // Below we (somewhat arbitrarily) distinguish these two by assuming that 
            // Statement 1 has three fanins, while Statement 2 has more than three fanins.
            //
            int iFinT, iFonT, fSigned = 1;
            assert( nRange0 >= 1 && Cba_ObjFinNum(p, i) >= 3 );
            assert( 1 + (1 << nRange0) == Cba_ObjFinNum(p, i) );
            Cba_ObjForEachFinFon( p, i, iFinT, iFonT, k )
                if ( k > 0 )
                    fSigned &= Cba_FonSigned(p, iFonT);
            for ( b = 0; b < nRange; b++ )
            {
                Vec_IntClear( vTemp0 );
                Cba_ObjForEachFinFon( p, i, iFinT, iFonT, k )
                    if ( k > 0 )
                    {
                        nRange1 = Cba_FonRangeSize( p, iFonT );
                        pFans1  = Cba_FonIsReal(iFonT) ? Vec_IntEntryP( vBits, Cba_FonCopy(p, iFonT) ) : NULL;
                        if ( Cba_ObjFinNum(p, i) == 3 ) // Statement 1
                            Vec_IntPush( vTemp0, k < nRange1 ? pFans1[k] : (fSigned? pFans1[nRange1-1] : 0) );
                        else // Statement 2
                            Vec_IntPush( vTemp0, k < nRange1 ? pFans1[k] : (Cba_FonSigned(p, iFonT)? pFans1[nRange1-1] : 0) );
                    }
                Vec_IntPush( vRes, Cba_NtkMuxTree_rec(pNew, pFans0, nRange0, vTemp0, 0) );
            }
        }
        else if ( Type == CBA_BOX_SHIR || Type == CBA_BOX_SHIRA ||
                  Type == CBA_BOX_SHIL || Type == CBA_BOX_SHILA )
        {
            int nRangeMax = Abc_MaxInt( nRange, nRange0 );
            int * pArg0 = Cba_VecLoadFanins( p, vTemp0, iFon0, pFans0, nRange0, nRangeMax, Cba_FonSigned(p, iFon0) );
            if ( Type == CBA_BOX_SHIR || Type == CBA_BOX_SHIRA )
                Cba_BlastShiftRight( pNew, pArg0, nRangeMax, pFans1, nRange1, Cba_FonSigned(p, iFon0) && Type == CBA_BOX_SHIRA, vRes );
            else
                Cba_BlastShiftLeft( pNew, pArg0, nRangeMax, pFans1, nRange1, 0, vRes );
            Vec_IntShrink( vRes, nRange );
        }
        else if ( Type == CBA_BOX_ROTR )
        {
            assert( nRange0 == nRange );
            Cba_BlastRotateRight( pNew, pFans0, nRange0, pFans1, nRange1, vRes );
        }
        else if ( Type == CBA_BOX_ROTL )
        {
            assert( nRange0 == nRange );
            Cba_BlastRotateLeft( pNew, pFans0, nRange0, pFans1, nRange1, vRes );
        }
        else if ( Type == CBA_BOX_INV )
        {
            int nRangeMax = Abc_MaxInt( nRange, nRange0 );
            int * pArg0 = Cba_VecLoadFanins( p, vTemp0, iFon0, pFans0, nRange0, nRangeMax, Cba_FonSigned(p, iFon0) );
            for ( k = 0; k < nRange; k++ )
                Vec_IntPush( vRes, Abc_LitNot(pArg0[k]) );
        }
        else if ( Type == CBA_BOX_AND )
        {
            int nRangeMax = Abc_MaxInt( nRange, Abc_MaxInt(nRange0, nRange1) );
            int * pArg0 = Cba_VecLoadFanins( p, vTemp0, iFon0, pFans0, nRange0, nRangeMax, fSigned01 );
            int * pArg1 = Cba_VecLoadFanins( p, vTemp1, iFon1, pFans1, nRange1, nRangeMax, fSigned01 );
            for ( k = 0; k < nRange; k++ )
                Vec_IntPush( vRes, Gia_ManHashAnd(pNew, pArg0[k], pArg1[k]) );
        }
        else if ( Type == CBA_BOX_OR )
        {
            int nRangeMax = Abc_MaxInt( nRange, Abc_MaxInt(nRange0, nRange1) );
            int * pArg0 = Cba_VecLoadFanins( p, vTemp0, iFon0, pFans0, nRange0, nRangeMax, fSigned01 );
            int * pArg1 = Cba_VecLoadFanins( p, vTemp1, iFon1, pFans1, nRange1, nRangeMax, fSigned01 );
            for ( k = 0; k < nRange; k++ )
                Vec_IntPush( vRes, Gia_ManHashOr(pNew, pArg0[k], pArg1[k]) );
        }
        else if ( Type == CBA_BOX_XOR )
        {
            int nRangeMax = Abc_MaxInt( nRange, Abc_MaxInt(nRange0, nRange1) );
            int * pArg0 = Cba_VecLoadFanins( p, vTemp0, iFon0, pFans0, nRange0, nRangeMax, fSigned01 );
            int * pArg1 = Cba_VecLoadFanins( p, vTemp1, iFon1, pFans1, nRange1, nRangeMax, fSigned01 );
            for ( k = 0; k < nRange; k++ )
                Vec_IntPush( vRes, Gia_ManHashXor(pNew, pArg0[k], pArg1[k]) );
        }
        else if ( Type == CBA_BOX_LNOT )
        {
            iLit = Cba_BlastReduction( pNew, pFans0, nRange0, CBA_BOX_ROR );
            Vec_IntFill( vRes, 1, Abc_LitNot(iLit) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
        }
        else if ( Type == CBA_BOX_LAND )
        {
            int iLit0 = Cba_BlastReduction( pNew, pFans0, nRange0, CBA_BOX_ROR );
            int iLit1 = Cba_BlastReduction( pNew, pFans1, nRange1, CBA_BOX_ROR );
            Vec_IntFill( vRes, 1, Gia_ManHashAnd(pNew, iLit0, iLit1) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
        }
        else if ( Type == CBA_BOX_LOR )
        {
            int iLit0 = Cba_BlastReduction( pNew, pFans0, nRange0, CBA_BOX_ROR );
            int iLit1 = Cba_BlastReduction( pNew, pFans1, nRange1, CBA_BOX_ROR );
            Vec_IntFill( vRes, 1, Gia_ManHashOr(pNew, iLit0, iLit1) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
        }
        else if ( Type == CBA_BOX_LXOR )
        {
            int iLit0 = Cba_BlastReduction( pNew, pFans0, nRange0, CBA_BOX_ROR );
            int iLit1 = Cba_BlastReduction( pNew, pFans1, nRange1, CBA_BOX_ROR );
            Vec_IntFill( vRes, 1, Gia_ManHashXor(pNew, iLit0, iLit1) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
        }
        else if ( Type == CBA_BOX_EQU || Type == CBA_BOX_NEQU )
        {
            int iLit = 0, nRangeMax = Abc_MaxInt( nRange0, nRange1 );
            int * pArg0 = Cba_VecLoadFanins( p, vTemp0, iFon0, pFans0, nRange0, nRangeMax, fSigned01 );
            int * pArg1 = Cba_VecLoadFanins( p, vTemp1, iFon1, pFans1, nRange1, nRangeMax, fSigned01 );
            for ( k = 0; k < nRangeMax; k++ )
                iLit = Gia_ManHashOr( pNew, iLit, Gia_ManHashXor(pNew, pArg0[k], pArg1[k]) ); 
            Vec_IntFill( vRes, 1, Abc_LitNotCond(iLit, Type == CBA_BOX_EQU) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
        }
        else if ( Type == CBA_BOX_LTHAN || Type == CBA_BOX_METHAN ||
                  Type == CBA_BOX_MTHAN || Type == CBA_BOX_LETHAN )
        {
            int nRangeMax = Abc_MaxInt( nRange0, nRange1 );
            int * pArg0 = Cba_VecLoadFanins( p, vTemp0, iFon0, pFans0, nRange0, nRangeMax, fSigned01 );
            int * pArg1 = Cba_VecLoadFanins( p, vTemp1, iFon1, pFans1, nRange1, nRangeMax, fSigned01 );
            int fSwap  = (Type == CBA_BOX_MTHAN  || Type == CBA_BOX_LETHAN);
            int fCompl = (Type == CBA_BOX_METHAN || Type == CBA_BOX_LETHAN);
            if ( fSwap ) ABC_SWAP( int *, pArg0, pArg1 );
            if ( fSigned01 )
                iLit = Cba_BlastLessSigned( pNew, pArg0, pArg1, nRangeMax );
            else
                iLit = Cba_BlastLess( pNew, pArg0, pArg1, nRangeMax );
            iLit = Abc_LitNotCond( iLit, fCompl );
            Vec_IntFill( vRes, 1, iLit );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
        }
        else if ( Type == CBA_BOX_RAND || Type == CBA_BOX_ROR || Type == CBA_BOX_RXOR )
        {
            Vec_IntPush( vRes, Cba_BlastReduction( pNew, pFans0, nRange0, Type ) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
        }
        else if ( Type == CBA_BOX_ADD ) 
        {
            int nRangeMax = Abc_MaxInt( nRange, Abc_MaxInt(nRange1, nRange2) );
            int * pArg0 = Cba_VecLoadFanins( p, vTemp0, iFon0, pFans0, 1, 1, 0 );
            int * pArg1 = Cba_VecLoadFanins( p, vRes,   iFon1, pFans1, nRange1, nRangeMax, fSigned01 );
            int * pArg2 = Cba_VecLoadFanins( p, vTemp1, iFon2, pFans2, nRange2, nRangeMax, fSigned01 );
            int Carry   = Cba_BlastAdder( pNew, pArg0[0], pArg1, pArg2, nRange ); // result is in pArg1 (vRes)
            assert( nRange0 == 1 );
            Vec_IntShrink( vRes, nRange );
            Vec_IntPush( vRes, Carry );
        }
        else if ( Type == CBA_BOX_SUB ) 
        {
            int nRangeMax = Abc_MaxInt( nRange, Abc_MaxInt(nRange0, nRange1) );
            int * pArg0 = Cba_VecLoadFanins( p, vRes,   iFon0, pFans0, nRange0, nRangeMax, fSigned01 );
            int * pArg1 = Cba_VecLoadFanins( p, vTemp1, iFon1, pFans1, nRange1, nRangeMax, fSigned01 );
            Cba_BlastSubtract( pNew, pArg0, pArg1, nRange ); // result is in pFan0 (vRes)
            Vec_IntShrink( vRes, nRange );
        }
        else if ( Type == CBA_BOX_MUL )
        {
            if ( fUseOldMultiplierBlasting )
            {
                int nRangeMax = Abc_MaxInt( nRange, Abc_MaxInt(nRange0, nRange1) );
                int * pArg0 = Cba_VecLoadFanins( p, vTemp0, iFon0, pFans0, nRange0, nRangeMax, fSigned01 );
                int * pArg1 = Cba_VecLoadFanins( p, vTemp1, iFon1, pFans1, nRange1, nRangeMax, fSigned01 );
                Cba_BlastMultiplier2( pNew, pArg0, pArg1, nRange, vTemp2, vRes );
                Vec_IntShrink( vRes, nRange );
            }
            else
            {
                int nRangeMax = Abc_MaxInt(nRange0, nRange1);
                int * pArg0 = Cba_VecLoadFanins( p, vTemp0, iFon0, pFans0, nRange0, nRangeMax, fSigned01 );
                int * pArg1 = Cba_VecLoadFanins( p, vTemp1, iFon1, pFans1, nRange1, nRangeMax, fSigned01 );
                Cba_BlastMultiplier( pNew, pArg0, pArg1, nRangeMax, nRangeMax, vTemp2, vRes, fSigned01 );
                if ( nRange > nRangeMax + nRangeMax )
                    Vec_IntFillExtra( vRes, nRange, fSigned01 ? Vec_IntEntryLast(vRes) : 0 );
                else
                    Vec_IntShrink( vRes, nRange );
                assert( Vec_IntSize(vRes) == nRange );
            }
        }
        else if ( Type == CBA_BOX_DIV || Type == CBA_BOX_MOD )
        {
            int nRangeMax = Abc_MaxInt( nRange, Abc_MaxInt(nRange0, nRange1) );
            int * pArg0 = Cba_VecLoadFanins( p, vTemp0, iFon0, pFans0, nRange0, nRangeMax, fSigned01 );
            int * pArg1 = Cba_VecLoadFanins( p, vTemp1, iFon1, pFans1, nRange1, nRangeMax, fSigned01 );
            if ( fSigned01 )
                Cba_BlastDividerSigned( pNew, pArg0, nRangeMax, pArg1, nRangeMax, Type == CBA_BOX_DIV, vRes );
            else
                Cba_BlastDivider( pNew, pArg0, nRangeMax, pArg1, nRangeMax, Type == CBA_BOX_DIV, vRes );
            Vec_IntShrink( vRes, nRange );
            if ( Type == CBA_BOX_DIV )
                Cba_BlastZeroCondition( pNew, pFans1, nRange1, vRes );
        }
        else if ( Type == CBA_BOX_MIN )
        {
            int nRangeMax = Abc_MaxInt( nRange0, nRange );
            int * pArg0 = Cba_VecLoadFanins( p, vTemp0, iFon0, pFans0, nRange0, nRangeMax, Cba_FonSigned(p, iFon0) );
            Cba_BlastMinus( pNew, pArg0, nRangeMax, vRes );
            Vec_IntShrink( vRes, nRange );
        }
        else if ( Type == CBA_BOX_POW )
        {
            int nRangeMax = Abc_MaxInt(nRange0, nRange);
            int * pArg0 = Cba_VecLoadFanins( p, vTemp0, iFon0, pFans0, nRange0, nRangeMax, Cba_FonSigned(p, iFon0) );
            int * pArg1 = Cba_VecLoadFanins( p, vTemp1, iFon1, pFans1, nRange1, nRange1,   Cba_FonSigned(p, iFon1) );
            Cba_BlastPower( pNew, pArg0, nRangeMax, pArg1, nRange1, vTemp2, vRes );
            Vec_IntShrink( vRes, nRange );
        }
        else if ( Type == CBA_BOX_SQRT )
        {
            int * pArg0 = Cba_VecLoadFanins( p, vTemp0, iFon0, pFans0, nRange0, nRange0 + (nRange0 & 1), 0 );
            nRange0 += (nRange0 & 1);
            Cba_BlastSqrt( pNew, pArg0, nRange0, vTemp2, vRes );
            if ( nRange > Vec_IntSize(vRes) )
                Vec_IntFillExtra( vRes, nRange, 0 );
            else
                Vec_IntShrink( vRes, nRange );
        }
        else if ( Type == CBA_BOX_TABLE )
        {
            assert( 0 );
            //Cba_BlastTable( pNew, Cba_ObjTable(p, p, i), pFans0, nRange0, nRange, vRes );
        }
        else assert( 0 );
        Vec_IntAppend( vBits, vRes );
        p->pDesign->nAnds[Type] += Gia_ManAndNum(pNew) - nAndPrev;
    }
    assert( nBits == Vec_IntSize(vBits) );
    p->pDesign->nAnds[0] = Gia_ManAndNum(pNew);
    // create COs
    Cba_NtkForEachPo( p, iObj, i )
    {
        Cba_ObjForEachFinFon( p, iObj, iFin, iFon, k )
        {
            nRange = Cba_FonRangeSize( p, iFon );
            pFans0  = Cba_FonIsReal(iFon) ? Vec_IntEntryP( vBits, Cba_FonCopy(p, iFon) ) : NULL;
            pFans0  = Cba_VecLoadFanins( p, vTemp0, iFon, pFans0, nRange, nRange, Cba_FonSigned(p, iFon) );
            for ( b = 0; b < nRange; b++ )
                Gia_ManAppendCo( pNew, pFans0[b] );
        }
    }
    Cba_NtkForEachBoxSeq( p, iObj, i )
    {
        if ( fSeq )
        {
            assert( Cba_ObjType(p, iObj) == CBA_BOX_DFFCPL );
            iFon0 = Cba_ObjFinFon( p, iObj, 0 );
            iFon1 = Cba_ObjFinFon( p, iObj, 1 );
            nRange0 = Cba_FonRangeSize( p, iFon0 );
            nRange1 = Cba_FonRangeSize( p, iFon1 );
            assert( nRange0 == nRange1 );
            Cba_ObjForEachFinFon( p, iObj, iFin, iFon, k )
            {
                nRange = Cba_FonRangeSize( p, iFon );
                pFans0 = Cba_FonIsReal(iFon) ? Vec_IntEntryP( vBits, Cba_FonCopy(p, iFon) ) : NULL;
                pFans0 = Cba_VecLoadFanins( p, vTemp0, iFon, pFans0, nRange0, nRange0, Cba_FonSigned(p, iFon) );
                if ( k == 0 )
                {
                    for ( b = 0; b < nRange; b++ )
                        Gia_ManAppendCo( pNew, pFans0[b] );
                }
                else if ( k == 1 )
                {
                    for ( b = 0; b < nRange; b++ )
                        if ( pFans0[b] == 0 )
                            Vec_StrPush( vInit, '0' );
                        else if ( pFans0[b] == 1 )
                            Vec_StrPush( vInit, '1' );
                        else
                            Vec_StrPush( vInit, 'x' );
                }
                else break;
            }
        }
        else // combinational
        {
            Cba_ObjForEachFinFon( p, iObj, iFin, iFon, k )
            {
                nRange = Cba_FonRangeSize( p, iFon );
                pFans0  = Cba_FonIsReal(iFon) ? Vec_IntEntryP( vBits, Cba_FonCopy(p, iFon) ) : NULL;
                pFans0  = Cba_VecLoadFanins( p, vTemp0, iFon, pFans0, nRange, nRange, Cba_FonSigned(p, iFon) );
                for ( b = 0; b < nRange; b++ )
                    Gia_ManAppendCo( pNew, pFans0[b] );
            }
        }
    }
    Vec_IntFree( vTemp0 );
    Vec_IntFree( vTemp1 );
    Vec_IntFree( vTemp2 );
    Vec_IntFree( vRes );
    // finalize AIG
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManDupRemapLiterals( vBits, pTemp );
    Gia_ManStop( pTemp );
    // transform AIG with init state
    if ( fSeq )
    {
        Gia_ManSetRegNum( pNew, Vec_StrSize(vInit) );
        Vec_StrPush( vInit, '\0' );
        pNew = Gia_ManDupZeroUndc( pTemp = pNew, Vec_StrArray(vInit), 0, 0, 1 );
        Gia_ManDupRemapLiterals( vBits, pTemp );
        Gia_ManStop( pTemp );
        Vec_StrFreeP( &vInit );
    }
    //Vec_IntErase( vBits );
    //Vec_IntErase( &p->vCopies );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Cba_ManBlast( Cba_Man_t * p, int fBarBufs, int fSeq, int fVerbose )
{
    return Cba_NtkBlast( Cba_ManRoot(p), fSeq );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cba_Man_t * Cba_ManInsertGia( Cba_Man_t * p, Gia_Man_t * pGia )
{
    return NULL;
}
Cba_Man_t * Cba_ManInsertAbc( Cba_Man_t * p, void * pAbc )
{
    Abc_Ntk_t * pNtk = (Abc_Ntk_t *)pAbc;
    return (Cba_Man_t *)pNtk;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

