/**CFile****************************************************************

  FileName    [wlcBlast.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Bit-blasting.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: wlcBlast.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wlc.h"
#include "misc/tim/tim.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Counts constant bits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wlc_NtkCountConstBits( int * pArray, int nSize )
{
    int i, Counter = 0;
    for ( i = 0; i < nSize; i++ )
        Counter += (pArray[i] == 0 || pArray[i] == 1);
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Helper functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wlc_NtkPrepareBits( Wlc_Ntk_t * p )
{
    Wlc_Obj_t * pObj;
    int i, nBits = 0;
    Wlc_NtkCleanCopy( p );
    Wlc_NtkForEachObj( p, pObj, i )
    {
        Wlc_ObjSetCopy( p, i, nBits );
        nBits += Wlc_ObjRange(pObj);
    }
    return nBits;
}
int * Wlc_VecCopy( Vec_Int_t * vOut, int * pArray, int nSize )
{
    int i; Vec_IntClear( vOut );
    for( i = 0; i < nSize; i++) 
        Vec_IntPush( vOut, pArray[i] );
    return Vec_IntArray( vOut );
}
int * Wlc_VecLoadFanins( Vec_Int_t * vOut, int * pFanins, int nFanins, int nTotal, int fSigned )
{
    int Fill = fSigned ? pFanins[nFanins-1] : 0;
    int i; Vec_IntClear( vOut );
    assert( nFanins <= nTotal );
    for( i = 0; i < nTotal; i++) 
        Vec_IntPush( vOut, i < nFanins ? pFanins[i] : Fill );
    return Vec_IntArray( vOut );
}
int Wlc_BlastGetConst( int * pNum, int nNum )
{
    int i, Res = 0;
    for ( i = 0; i < nNum; i++ )
        if ( pNum[i] == 1 )
            Res |= (1 << i);
        else if ( pNum[i] != 0 )
            return -1;
    return Res;
}
int Wlc_NtkMuxTree_rec( Gia_Man_t * pNew, int * pCtrl, int nCtrl, Vec_Int_t * vData, int Shift )
{
    int iLit0, iLit1;
    if ( nCtrl == 0 )
        return Vec_IntEntry( vData, Shift );
    iLit0 = Wlc_NtkMuxTree_rec( pNew, pCtrl, nCtrl-1, vData, Shift );
    iLit1 = Wlc_NtkMuxTree_rec( pNew, pCtrl, nCtrl-1, vData, Shift + (1<<(nCtrl-1)) );
    return Gia_ManHashMux( pNew, pCtrl[nCtrl-1], iLit1, iLit0 );
}
int Wlc_NtkMuxTree2_nb( Gia_Man_t * pNew, int * pCtrl, int nCtrl, Vec_Int_t * vData, Vec_Int_t * vAnds )
{
    int iLitOr = 0, iLitAnd, m;
    assert( Vec_IntSize(vData) == (1 << nCtrl) );
    assert( Vec_IntSize(vAnds) == (1 << nCtrl) );
    for ( m = 0; m < (1 << nCtrl); m++ )
    {
        iLitAnd = Gia_ManHashAnd( pNew, Vec_IntEntry(vAnds, m), Vec_IntEntry(vData, m) );
        iLitOr  = Gia_ManHashOr( pNew, iLitOr, iLitAnd );
    }
    return iLitOr;
}
int Wlc_NtkMuxTree2( Gia_Man_t * pNew, int * pCtrl, int nCtrl, Vec_Int_t * vData, Vec_Int_t * vAnds, Vec_Int_t * vTemp )
{
    int m, iLit;
    assert( !nCtrl || Vec_IntSize(vData) == (1 << nCtrl) );
    assert( !nCtrl || Vec_IntSize(vAnds) == (1 << nCtrl) );
    Vec_IntClear( vTemp );
    Vec_IntForEachEntry( vAnds, iLit, m )
        Vec_IntPush( vTemp, Abc_LitNot( Gia_ManHashAnd(pNew, iLit, Vec_IntEntry(vData, m)) ) );
    return Abc_LitNot( Gia_ManHashAndMulti(pNew, vTemp) );
}
void Wlc_NtkPrintNameArray( Vec_Ptr_t * vNames )
{
    int i; char * pTemp;
    Vec_PtrForEachEntry( char *, vNames, pTemp, i )
        printf( "%2d : %s\n", i, pTemp );
}

/**Function*************************************************************

  Synopsis    [Bit blasting for specific operations.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_BlastShiftRightInt( Gia_Man_t * pNew, int * pNum, int nNum, int * pShift, int nShift, int fSticky, Vec_Int_t * vRes )
{
    int * pRes = Wlc_VecCopy( vRes, pNum, nNum );
    int Fill = fSticky ? pNum[nNum-1] : 0;
    int i, j, fShort = 0;
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
void Wlc_BlastShiftRight( Gia_Man_t * pNew, int * pNum, int nNum, int * pShift, int nShift, int fSticky, Vec_Int_t * vRes )
{
    int nShiftMax = Abc_Base2Log(nNum);
    int * pShiftNew = ABC_ALLOC( int, nShift );
    memcpy( pShiftNew, pShift, sizeof(int)*nShift );
    if ( nShiftMax < nShift && nShift > 30 )
    {
        int i, iRes = pShiftNew[nShiftMax];
        for ( i = nShiftMax + 1; i < nShift; i++ )
            iRes = Gia_ManHashOr( pNew, iRes, pShiftNew[i] );
        pShiftNew[nShiftMax++] = iRes;
    }
    else 
        nShiftMax = nShift;
    Wlc_BlastShiftRightInt( pNew, pNum, nNum, pShiftNew, nShiftMax, fSticky, vRes );
    ABC_FREE( pShiftNew );
}
void Wlc_BlastShiftLeftInt( Gia_Man_t * pNew, int * pNum, int nNum, int * pShift, int nShift, int fSticky, Vec_Int_t * vRes )
{
    int * pRes = Wlc_VecCopy( vRes, pNum, nNum );
    int Fill = fSticky ? pNum[0] : 0;
    int i, j, fShort = 0;
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
void Wlc_BlastShiftLeft( Gia_Man_t * pNew, int * pNum, int nNum, int * pShift, int nShift, int fSticky, Vec_Int_t * vRes )
{
    int nShiftMax = Abc_Base2Log(nNum);
    int * pShiftNew = ABC_ALLOC( int, nShift );
    memcpy( pShiftNew, pShift, sizeof(int)*nShift );
    if ( nShiftMax < nShift )
    {
        int i, iRes = pShiftNew[nShiftMax];
        for ( i = nShiftMax + 1; i < nShift; i++ )
            iRes = Gia_ManHashOr( pNew, iRes, pShiftNew[i] );
        pShiftNew[nShiftMax++] = iRes;
    }
    else 
        nShiftMax = nShift;
    Wlc_BlastShiftLeftInt( pNew, pNum, nNum, pShiftNew, nShiftMax, fSticky, vRes );
    ABC_FREE( pShiftNew );
}
void Wlc_BlastRotateRight( Gia_Man_t * pNew, int * pNum, int nNum, int * pShift, int nShift, Vec_Int_t * vRes )
{
    int * pRes = Wlc_VecCopy( vRes, pNum, nNum );
    int i, j, * pTemp = ABC_ALLOC( int, nNum );
    assert( nShift <= 32 );
    for( i = 0; i < nShift; i++, pRes = Wlc_VecCopy(vRes, pTemp, nNum) ) 
        for( j = 0; j < nNum; j++ ) 
            pTemp[j] = Gia_ManHashMux( pNew, pShift[i], pRes[(j+(1<<i))%nNum], pRes[j] );
    ABC_FREE( pTemp );
}
void Wlc_BlastRotateLeft( Gia_Man_t * pNew, int * pNum, int nNum, int * pShift, int nShift, Vec_Int_t * vRes )
{
    int * pRes = Wlc_VecCopy( vRes, pNum, nNum );
    int i, j, * pTemp = ABC_ALLOC( int, nNum );
    assert( nShift <= 32 );
    for( i = 0; i < nShift; i++, pRes = Wlc_VecCopy(vRes, pTemp, nNum) ) 
        for( j = 0; j < nNum; j++ ) 
        {
            int move = (j >= (1<<i)) ? (j-(1<<i))%nNum : (nNum - (((1<<i)-j)%nNum)) % nNum;
            pTemp[j] = Gia_ManHashMux( pNew, pShift[i], pRes[move], pRes[j] );
//            pTemp[j] = Gia_ManHashMux( pNew, pShift[i], pRes[((unsigned)(nNum-(1<<i)+j))%nNum], pRes[j] );
        }
    ABC_FREE( pTemp );
}
int Wlc_BlastReduction( Gia_Man_t * pNew, int * pFans, int nFans, int Type )
{
    if ( Type == WLC_OBJ_REDUCT_AND || Type == WLC_OBJ_REDUCT_NAND )
    {
        int k, iLit = 1;
        for ( k = 0; k < nFans; k++ )
            iLit = Gia_ManHashAnd( pNew, iLit, pFans[k] );
        return Abc_LitNotCond( iLit, Type == WLC_OBJ_REDUCT_NAND );
    }
    if ( Type == WLC_OBJ_REDUCT_OR || Type == WLC_OBJ_REDUCT_NOR )
    {
        int k, iLit = 0;
        for ( k = 0; k < nFans; k++ )
            iLit = Gia_ManHashOr( pNew, iLit, pFans[k] );
        return Abc_LitNotCond( iLit, Type == WLC_OBJ_REDUCT_NOR );
    }
    if ( Type == WLC_OBJ_REDUCT_XOR || Type == WLC_OBJ_REDUCT_NXOR )
    {
        int k, iLit = 0;
        for ( k = 0; k < nFans; k++ )
            iLit = Gia_ManHashXor( pNew, iLit, pFans[k] );
        return Abc_LitNotCond( iLit, Type == WLC_OBJ_REDUCT_NXOR );
    }
    assert( 0 );
    return -1;
}
int Wlc_BlastLess2( Gia_Man_t * pNew, int * pArg0, int * pArg1, int nBits )
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
void Wlc_BlastLess_rec( Gia_Man_t * pNew, int * pArg0, int * pArg1, int nBits, int * pYes, int * pNo )
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
        Wlc_BlastLess_rec( pNew, pArg0, pArg1, nBits-1, &YesR, &NoR );
        *pYes = Gia_ManHashOr( pNew, Yes, Gia_ManHashAnd(pNew, Abc_LitNot(No),  YesR) );
        *pNo  = Gia_ManHashOr( pNew, No,  Gia_ManHashAnd(pNew, Abc_LitNot(Yes), NoR ) );
        return;
    }
    assert( nBits == 1 );
    *pYes = Gia_ManHashAnd( pNew, Abc_LitNot(pArg0[0]), pArg1[0] );
    *pNo  = Gia_ManHashAnd( pNew, Abc_LitNot(pArg1[0]), pArg0[0] );
}
int Wlc_BlastLess( Gia_Man_t * pNew, int * pArg0, int * pArg1, int nBits )
{
    int Yes, No;
    if ( nBits == 0 )
        return 0;
    Wlc_BlastLess_rec( pNew, pArg0, pArg1, nBits, &Yes, &No );
    return Yes;
}
int Wlc_BlastLessSigned( Gia_Man_t * pNew, int * pArg0, int * pArg1, int nBits )
{
    int iDiffSign = Gia_ManHashXor( pNew, pArg0[nBits-1], pArg1[nBits-1] );
    return Gia_ManHashMux( pNew, iDiffSign, pArg0[nBits-1], Wlc_BlastLess(pNew, pArg0, pArg1, nBits-1) );
}
void Wlc_BlastFullAdder( Gia_Man_t * pNew, int a, int b, int c, int * pc, int * ps )
{
    int fUseXor = 0;
    int fCompl = (a == 1 || b == 1 || c == 1);
    // propagate complement through the FA - helps generate less redundant logic
    if ( fCompl )
        a = Abc_LitNot(a), b = Abc_LitNot(b), c = Abc_LitNot(c); 
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
        int Xor  = Gia_ManHashAnd(pNew, Abc_LitNot(And1), Abc_LitNot(And1_));
        int And2 = Gia_ManHashAnd(pNew, c, Xor);
        int And2_= Gia_ManHashAnd(pNew, Abc_LitNot(c), Abc_LitNot(Xor));
        *ps      = Gia_ManHashAnd(pNew, Abc_LitNot(And2), Abc_LitNot(And2_));
        *pc      = Gia_ManHashOr (pNew, And1, And2);
    }
    if ( fCompl )
        *ps = Abc_LitNot(*ps), *pc = Abc_LitNot(*pc); 
}
int Wlc_BlastAdder( Gia_Man_t * pNew, int * pAdd0, int * pAdd1, int nBits, int Carry ) // result is in pAdd0
{
    int b;
    for ( b = 0; b < nBits; b++ )
        Wlc_BlastFullAdder( pNew, pAdd0[b], pAdd1[b], Carry, &Carry, &pAdd0[b] );
    return Carry;
}
void Wlc_BlastSubtract( Gia_Man_t * pNew, int * pAdd0, int * pAdd1, int nBits, int Carry ) // result is in pAdd0
{
    int b;
    for ( b = 0; b < nBits; b++ )
        Wlc_BlastFullAdder( pNew, pAdd0[b], Abc_LitNot(pAdd1[b]), Carry, &Carry, &pAdd0[b] );
}

void Wlc_BlastAdderCLA_one( Gia_Man_t * pNew, int * pGen, int * pPro, int * pCar, int * pGen1, int * pPro1, int * pCar1 )
{
    int Temp = Gia_ManHashAnd( pNew, pGen[0], pPro[1] );
    *pPro1   = Gia_ManHashAnd( pNew, pPro[0], pPro[1] );
    *pGen1   = Gia_ManHashOr( pNew, Gia_ManHashOr(pNew, pGen[1], Temp), Gia_ManHashAnd(pNew, *pPro1, pCar[0]) );
    *pCar1   = Gia_ManHashOr( pNew, pGen[0], Gia_ManHashAnd(pNew, pPro[0], pCar[0]) );
}
void Wlc_BlastAdderCLA_rec( Gia_Man_t * pNew, int * pGen, int * pPro, int * pCar, int nBits, int * pGen1, int * pPro1 )
{
    if ( nBits == 2 )
        Wlc_BlastAdderCLA_one( pNew, pGen, pPro, pCar, pGen1, pPro1, pCar+1 ); // returns *pGen1, *pPro1, pCar[1]
    else
    {
        int pGen2[2], pPro2[2];
        assert( nBits % 2 == 0 );
        // call recursively
        Wlc_BlastAdderCLA_rec( pNew, pGen,         pPro,         pCar,         nBits/2, pGen2,   pPro2   );
        pCar[nBits/2] = *pGen2;
        Wlc_BlastAdderCLA_rec( pNew, pGen+nBits/2, pPro+nBits/2, pCar+nBits/2, nBits/2, pGen2+1, pPro2+1 );
        // create structure
        Wlc_BlastAdderCLA_one( pNew, pGen2, pPro2, pCar, pGen1, pPro1, pCar+nBits/2 ); // returns *pGen1, *pPro1, pCar[nBits/2]
    }
}
void Wlc_BlastAdderCLA_int( Gia_Man_t * pNew, int * pAdd0, int * pAdd1, int nBits, int CarryIn ) // result is in pAdd0
{
    int * pGen = ABC_CALLOC( int, nBits );
    int * pPro = ABC_CALLOC( int, nBits );
    int * pCar = ABC_CALLOC( int, nBits+1 );
    int b, Gen, Pro;
    if ( nBits == 1 )
    {
        int Carry = CarryIn;
        Wlc_BlastFullAdder( pNew, pAdd0[0], pAdd1[0], Carry, &Carry, &pAdd0[0] );
        return;
    }
    assert( nBits >= 2 );
    pCar[0] = CarryIn;
    for ( b = 0; b < nBits; b++ )
    {
        pGen[b] = Gia_ManHashAnd(pNew, pAdd0[b], pAdd1[b]);
        pPro[b] = Gia_ManHashXor(pNew, pAdd0[b], pAdd1[b]);
    }
    Wlc_BlastAdderCLA_rec( pNew, pGen, pPro, pCar, nBits, &Gen, &Pro );
    for ( b = 0; b < nBits; b++ )
        pAdd0[b] = Gia_ManHashXor(pNew, pPro[b], pCar[b]);
    ABC_FREE(pGen);
    ABC_FREE(pPro);
    ABC_FREE(pCar);
}
void Wlc_BlastAdderCLA( Gia_Man_t * pNew, int * pAdd0, int * pAdd1, int nBits, int fSign, int CarryIn ) // result is in pAdd0
{
    int i, Log2 = Abc_Base2Log(nBits);
    int * pAdd0n = ABC_CALLOC( int, 1<<Log2 );
    int * pAdd1n = ABC_CALLOC( int, 1<<Log2 );
    for ( i = 0; i < nBits; i++ )
    {
        pAdd0n[i] = pAdd0[i];
        pAdd1n[i] = pAdd1[i];
    }
    for ( ; i < (1<<Log2); i++ )
    {
        pAdd0n[i] = fSign ? pAdd0[nBits-1] : 0;
        pAdd1n[i] = fSign ? pAdd1[nBits-1] : 0;
    }
    Wlc_BlastAdderCLA_int( pNew, pAdd0n, pAdd1n, 1<<Log2, CarryIn );
    for ( i = 0; i < nBits; i++ )
        pAdd0[i] = pAdd0n[i];
    ABC_FREE(pAdd0n);
    ABC_FREE(pAdd1n);
}

void Wlc_BlastAdderFast_int( Gia_Man_t * pNew, int * pAdd0, int * pAdd1, int Log2, int CarryIn ) // result is in pAdd0
{
    int i, b, Gen, Pro, nBits = 1 << Log2;
    int * pGen = ABC_CALLOC( int, nBits+1 );
    int * pPro = ABC_CALLOC( int, nBits+1 );
    int * pPro2= ABC_CALLOC( int, nBits+1 );
    if ( nBits == 1 )
    {
        int Carry = CarryIn;
        Wlc_BlastFullAdder( pNew, pAdd0[0], pAdd1[0], Carry, &Carry, &pAdd0[0] );
        ABC_FREE(pGen);
        ABC_FREE(pPro);
        ABC_FREE(pPro2);
        return;
    }
    assert( nBits >= 2 );
    pGen[0] = CarryIn;
    pPro[0] = 0;
    pPro2[0]= 0;
    for ( b = 1; b <= nBits; b++ )
    {
        pGen[b] = Gia_ManHashAnd(pNew, pAdd0[b-1], pAdd1[b-1]);
        pPro[b] = Gia_ManHashXor(pNew, pAdd0[b-1], pAdd1[b-1]);
        pPro2[b]= pPro[b];
    }
    
    // Han-Carlson adder from http://www.aoki.ecei.tohoku.ac.jp/arith/mg/algorithm.html
    for ( b = 1; b <= nBits; b += 2 )
    {
        Gen = Gia_ManHashOr( pNew, pGen[b], Gia_ManHashAnd(pNew, pPro[b], pGen[b-1]) );
        Pro = Gia_ManHashAnd(pNew, pPro[b], pPro[b-1]);
        pPro[b] = Pro;
        pGen[b] = Gen;
    }
    for ( i = 1; i < Log2-1; i++ )
    {
        for ( b = 1 + 2*i; b <= nBits; b += 2 )
        {
            Gen = Gia_ManHashOr( pNew, pGen[b], Gia_ManHashAnd(pNew, pPro[b], pGen[b-i*2]) );
            Pro = Gia_ManHashAnd(pNew, pPro[b], pPro[b-i*2]);
            pPro[b] = Pro;
            pGen[b] = Gen;
        }
    }
    for ( b = nBits/2 + 1; b <= nBits; b += 2 )
    {
        Gen = Gia_ManHashOr( pNew, pGen[b], Gia_ManHashAnd(pNew, pPro[b], pGen[b-nBits/2]) );
        Pro = Gia_ManHashAnd(pNew, pPro[b], pPro[b-nBits/2]);
        pPro[b] = Pro;
        pGen[b] = Gen;
    }
    for ( b = 2; b <= nBits; b += 2 )
    {
        Gen = Gia_ManHashOr( pNew, pGen[b], Gia_ManHashAnd(pNew, pPro[b], pGen[b-1]) );
        Pro = Gia_ManHashAnd(pNew, pPro[b], pPro[b-1]);
        pPro[b] = Pro;
        pGen[b] = Gen;
    }
    
    for ( b = 0; b < nBits; b++ )
        pAdd0[b] = Gia_ManHashXor(pNew, pPro2[b+1], pGen[b]);
    pAdd0[nBits] = pGen[nBits];

    ABC_FREE(pGen);
    ABC_FREE(pPro);
    ABC_FREE(pPro2);
}
void Wlc_BlastAdderFast( Gia_Man_t * pNew, int * pAdd0, int * pAdd1, int nBits, int fSign, int CarryIn ) // result is in pAdd0
{
    int i, Log2 = Abc_Base2Log(nBits);
    int * pAdd0n = ABC_CALLOC( int, (1<<Log2)+1 );
    int * pAdd1n = ABC_CALLOC( int, (1<<Log2)+1 );
    for ( i = 0; i < nBits; i++ )
    {
        pAdd0n[i] = pAdd0[i];
        pAdd1n[i] = pAdd1[i];
    }
    for ( ; i < (1<<Log2); i++ )
    {
        pAdd0n[i] = fSign ? pAdd0[nBits-1] : 0;
        pAdd1n[i] = fSign ? pAdd1[nBits-1] : 0;
    }
    Wlc_BlastAdderFast_int( pNew, pAdd0n, pAdd1n, Log2, CarryIn );
    for ( i = 0; i <= nBits; i++ )
        pAdd0[i] = pAdd0n[i];
    ABC_FREE(pAdd0n);
    ABC_FREE(pAdd1n);
}

void Wlc_BlastMinus( Gia_Man_t * pNew, int * pNum, int nNum, Vec_Int_t * vRes )
{
    int * pRes  = Wlc_VecCopy( vRes, pNum, nNum );
    int i, invert = 0;
    for ( i = 0; i < nNum; i++ )
    {
        pRes[i] = Gia_ManHashMux( pNew, invert, Abc_LitNot(pRes[i]), pRes[i] );
        invert = Gia_ManHashOr( pNew, invert, pNum[i] );    
    }
}
void Wlc_BlastMultiplier2( Gia_Man_t * pNew, int * pArg0, int * pArg1, int nBits, Vec_Int_t * vTemp, Vec_Int_t * vRes )
{
    int i, j;
    Vec_IntFill( vRes, nBits, 0 );
    for ( i = 0; i < nBits; i++ )
    {
        Vec_IntFill( vTemp, i, 0 );
        for ( j = 0; Vec_IntSize(vTemp) < nBits; j++ )
            Vec_IntPush( vTemp, Gia_ManHashAnd(pNew, pArg0[j], pArg1[i]) );
        assert( Vec_IntSize(vTemp) == nBits );
        Wlc_BlastAdder( pNew, Vec_IntArray(vRes), Vec_IntArray(vTemp), nBits, 0 );
    }
}
void Wlc_BlastFullAdderCtrl( Gia_Man_t * pNew, int a, int ac, int b, int c, int * pc, int * ps, int fNeg )
{
    int And  = Abc_LitNotCond( Gia_ManHashAnd(pNew, a, ac), fNeg );
    Wlc_BlastFullAdder( pNew, And, b, c, pc, ps );
}
void Wlc_BlastFullAdderSubtr( Gia_Man_t * pNew, int a, int b, int c, int * pc, int * ps, int fSub )
{
    Wlc_BlastFullAdder( pNew, Gia_ManHashXor(pNew, a, fSub), b, c, pc, ps );
}
void Wlc_BlastMultiplier( Gia_Man_t * pNew, int * pArgA, int * pArgB, int nArgA, int nArgB, Vec_Int_t * vTemp, Vec_Int_t * vRes, int fSigned )
{
    int * pRes, * pArgC, * pArgS, a, b, Carry = fSigned;
    assert( nArgA > 0 && nArgB > 0 );
    assert( fSigned == 0 || fSigned == 1 );
    // prepare result
    Vec_IntFill( vRes, nArgA + nArgB, 0 );
    //Vec_IntFill( vRes, nArgA + nArgB + 1, 0 );
    pRes = Vec_IntArray( vRes );
    // prepare intermediate storage
    Vec_IntFill( vTemp, 2 * nArgA, 0 );
    pArgC = Vec_IntArray( vTemp );
    pArgS = pArgC + nArgA;
    // create matrix
    for ( b = 0; b < nArgB; b++ )
        for ( a = 0; a < nArgA; a++ )
            Wlc_BlastFullAdderCtrl( pNew, pArgA[a], pArgB[b], pArgS[a], pArgC[a], 
                &pArgC[a], a ? &pArgS[a-1] : &pRes[b], fSigned && ((a+1 == nArgA) ^ (b+1 == nArgB)) );
    // final addition
    pArgS[nArgA-1] = fSigned;
    for ( a = 0; a < nArgA; a++ )
        Wlc_BlastFullAdderCtrl( pNew, 1, pArgC[a], pArgS[a], Carry, &Carry, &pRes[nArgB+a], 0 );
    //Vec_IntWriteEntry( vRes, nArgA + nArgB, Carry );
}
void Wlc_BlastMultiplierC( Gia_Man_t * pNew, int * pArgA, int * pArgB, int nArgA, int nArgB, Vec_Int_t * vTemp, Vec_Int_t * vRes, int fSigned )
{
    int * pRes, * pArgC, * pArgS, a, b, Carry = !fSigned; // change
    assert( nArgA > 0 && nArgB > 0 );
    assert( fSigned == 0 || fSigned == 1 );
    Vec_IntFill( vRes, nArgA + nArgB, 0 );
    pRes = Vec_IntArray( vRes );
    Vec_IntFill( vTemp, 2 * nArgA, 1 ); // change
    pArgC = Vec_IntArray( vTemp );
    pArgS = pArgC + nArgA;
    for ( b = 0; b < nArgB; b++ )
        for ( a = 0; a < nArgA; a++ )
            Wlc_BlastFullAdderCtrl( pNew, pArgA[a], pArgB[b], pArgS[a], pArgC[a], 
                &pArgC[a], a ? &pArgS[a-1] : &pRes[b], !(fSigned && ((a+1 == nArgA) ^ (b+1 == nArgB))) ); // change
    pArgS[nArgA-1] = !fSigned; // change
    for ( a = 0; a < nArgA; a++ )
        Wlc_BlastFullAdderCtrl( pNew, 1, pArgC[a], pArgS[a], Carry, &Carry, &pRes[nArgB+a], 0 );
    for ( b = 0; b < nArgA + nArgB; b++ ) // change
        pRes[b] = Abc_LitNot(pRes[b]);
}
void Wlc_BlastDivider( Gia_Man_t * pNew, int * pNum, int nNum, int * pDiv, int nDiv, int fQuo, Vec_Int_t * vRes )
{
    int * pRes  = Wlc_VecCopy( vRes, pNum, nNum );
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
            Wlc_VecCopy( vRes, pTemp, nNum );
        else 
            for( i = 0; i < nNum; i++ ) 
                pRes[i] = Gia_ManHashMux( pNew, pQuo[j], pTemp[i], pRes[i] );
    }
    ABC_FREE( pTemp );
    if ( fQuo )
        Wlc_VecCopy( vRes, pQuo, nNum );
    ABC_FREE( pQuo );
}
// non-restoring divider
void Wlc_BlastDividerNR( Gia_Man_t * pNew, int * pNum, int nNum, int * pDiv, int nDiv, int fQuo, Vec_Int_t * vRes )
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
            Wlc_BlastFullAdderSubtr( pNew, k < nDiv ? pDiv[k] : 0, pRes[i+k], Carry, &Carry, &pRes[i+k], Cntrl );
        pQuo[i] = Abc_LitNot(pRes[i+nDiv]);
    }
    if ( fQuo )
        Wlc_VecCopy( vRes, pQuo, nNum );
    else
    {
        int Carry = 0, Temp;
        for ( k = 0; k < nDiv; k++ )
        {
            Wlc_BlastFullAdder( pNew, pDiv[k], pRes[k], Carry, &Carry, &Temp );
            pRes[k] = Gia_ManHashMux( pNew, pQuo[0], pRes[k], Temp );
        }
        Vec_IntShrink( vRes, nDiv );
    }
    ABC_FREE( pQuo );
}
void Wlc_BlastDividerTop( Gia_Man_t * pNew, int * pNum, int nNum, int * pDiv, int nDiv, int fQuo, Vec_Int_t * vRes, int fNonRest )
{
    if ( fNonRest )
        Wlc_BlastDividerNR( pNew, pNum, nNum, pDiv, nDiv, fQuo, vRes );
    else
        Wlc_BlastDivider( pNew, pNum, nNum, pDiv, nDiv, fQuo, vRes );
}
void Wlc_BlastDividerSigned( Gia_Man_t * pNew, int * pNum, int nNum, int * pDiv, int nDiv, int fQuo, Vec_Int_t * vRes, int fNonRest )
{
    Vec_Int_t * vNum   = Vec_IntAlloc( nNum );
    Vec_Int_t * vDiv   = Vec_IntAlloc( nDiv );
    Vec_Int_t * vRes00 = Vec_IntAlloc( nNum + nDiv );
    Vec_Int_t * vRes01 = Vec_IntAlloc( nNum + nDiv );
    Vec_Int_t * vRes10 = Vec_IntAlloc( nNum + nDiv );
    Vec_Int_t * vRes11 = Vec_IntAlloc( nNum + nDiv );
    Vec_Int_t * vRes2  = Vec_IntAlloc( nNum );
    int k, iDiffSign   = Gia_ManHashXor( pNew, pNum[nNum-1], pDiv[nDiv-1] );
    Wlc_BlastMinus( pNew, pNum, nNum, vNum );
    Wlc_BlastMinus( pNew, pDiv, nDiv, vDiv );
    Wlc_BlastDividerTop( pNew,               pNum, nNum,               pDiv, nDiv, fQuo, vRes00, fNonRest );
    Wlc_BlastDividerTop( pNew,               pNum, nNum, Vec_IntArray(vDiv), nDiv, fQuo, vRes01, fNonRest );
    Wlc_BlastDividerTop( pNew, Vec_IntArray(vNum), nNum,               pDiv, nDiv, fQuo, vRes10, fNonRest );
    Wlc_BlastDividerTop( pNew, Vec_IntArray(vNum), nNum, Vec_IntArray(vDiv), nDiv, fQuo, vRes11, fNonRest );
    Vec_IntClear( vRes );
    for ( k = 0; k < nNum; k++ )
    {
        int Data0 =  Gia_ManHashMux( pNew, pDiv[nDiv-1], Vec_IntEntry(vRes01,k), Vec_IntEntry(vRes00,k) );
        int Data1 =  Gia_ManHashMux( pNew, pDiv[nDiv-1], Vec_IntEntry(vRes11,k), Vec_IntEntry(vRes10,k) );
        Vec_IntPush( vRes, Gia_ManHashMux(pNew, pNum[nNum-1], Data1, Data0) );
    }
    Wlc_BlastMinus( pNew, Vec_IntArray(vRes), nNum, vRes2 );
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
void Wlc_BlastZeroCondition( Gia_Man_t * pNew, int * pDiv, int nDiv, Vec_Int_t * vRes )
{
    int i, Entry, iLit = Wlc_BlastReduction( pNew, pDiv, nDiv, WLC_OBJ_REDUCT_OR );
    Vec_IntForEachEntry( vRes, Entry, i )
        Vec_IntWriteEntry( vRes, i, Gia_ManHashAnd(pNew, iLit, Entry) );
}
void Wlc_BlastTable( Gia_Man_t * pNew, word * pTable, int * pFans, int nFans, int nOuts, Vec_Int_t * vRes )
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
void Wlc_BlastLut( Gia_Man_t * pNew, word Truth, int * pFans, int nFans, int nOuts, Vec_Int_t * vRes )
{
    extern int Kit_TruthToGia( Gia_Man_t * pMan, unsigned * pTruth, int nVars, Vec_Int_t * vMemory, Vec_Int_t * vLeaves, int fHash );
    Vec_Int_t * vMemory = Vec_IntAlloc( 0 );
    Vec_Int_t vLeaves = { nFans, nFans, pFans };
    int iLit;
    Vec_IntClear( vRes );
    assert( nOuts == 1 );
    if ( nFans < 6 )
        Truth = Abc_Tt6Stretch( Truth, nFans );
    iLit = Kit_TruthToGia( pNew, (unsigned *)&Truth, nFans, vMemory, &vLeaves, 1 );
    Vec_IntPush( vRes, iLit );
    Vec_IntFree( vMemory );
}
void Wlc_BlastPower( Gia_Man_t * pNew, int * pNum, int nNum, int * pExp, int nExp, Vec_Int_t * vTemp, Vec_Int_t * vRes )
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
            pDegrees = Wlc_VecCopy( vDegrees, pNum, nNum );
        else
        {
            Wlc_BlastMultiplier2( pNew, pDegrees, pDegrees, nNum, vTemp, vResTemp );
            pDegrees = Wlc_VecCopy( vDegrees, pResTemp, nNum );
        }
        Wlc_BlastMultiplier2( pNew, pRes, pDegrees, nNum, vTemp, vResTemp );
        for ( k = 0; k < nNum; k++ )
            pRes[k] = Gia_ManHashMux( pNew, pExp[i], pResTemp[k], pRes[k] );
    }
    Vec_IntFree( vResTemp );
    Vec_IntFree( vDegrees );
}
void Wlc_BlastSqrt( Gia_Man_t * pNew, int * pNum, int nNum, Vec_Int_t * vTmp, Vec_Int_t * vRes )
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
                Wlc_BlastFullAdder( pNew, pSumP[k], Abc_LitNot(pRes[i-k+1]), Carry, &Carry, &pSum[k] );
            else
                Wlc_BlastFullAdder( pNew, pSumP[k], Abc_LitNot(k ? Carry:1), 1,     &Carry, &pSum[k] );
            if ( k == 0 || k > i )
                Carry = Abc_LitNot(Carry);
        }
        pRes[i] = Abc_LitNot(Carry);
        for ( k = 0; k < i + 3; k++ )
            pSum[k] = Gia_ManHashMux( pNew, pRes[i], pSum[k], pSumP[k] );
    }
    Vec_IntReverseOrder( vRes );
}
void Wlc_BlastSqrtNR( Gia_Man_t * pNew, int * pNum, int nNum, Vec_Int_t * vTmp, Vec_Int_t * vRes )
{
    int * pSqr, * pRem, * pIn1;
    int i, k, Carry = 1;
    assert( nNum % 2 == 0 );
    Vec_IntFill( vRes, nNum/2, 0 );
    Vec_IntFill( vTmp, 2*nNum, 0 );
    pSqr = Vec_IntArray( vRes );
    pRem = Vec_IntArray( vTmp );
    pIn1 = pRem + nNum;
    for ( i = 0; i < nNum/2; i++ )
    {
        int SqrPrev = Carry;
        assert( pIn1[0] == 0 );
        for ( k = 1; k < i; k++ )
            pIn1[k] = 0;
        for ( k = i; k < 2*i; k++ )
            pIn1[k] = pSqr[k-i];
        pIn1[k++] = Abc_LitNot(Carry);
        pIn1[k++] = 1;
        assert( k == 2*i+2 );
        pRem[2*i+0] = pNum[nNum-2*i-1];
        pRem[2*i+1] = pNum[nNum-2*i-2];
        for ( k = 2*i+1; k >= 0; k-- )
            Wlc_BlastFullAdder( pNew, Gia_ManHashXor(pNew, SqrPrev, pIn1[k]), pRem[k], Carry, &Carry, &pRem[k] );
        pSqr[i] = Carry;
    }
    Vec_IntReverseOrder( vRes );
}
void Wlc_IntInsert( Vec_Int_t * vProd, Vec_Int_t * vLevel, int Node, int Level )
{
    int i;
    for ( i = Vec_IntSize(vLevel) - 1; i >= 0; i-- )
        if ( Vec_IntEntry(vLevel, i) >= Level )
            break;
    Vec_IntInsert( vProd,  i + 1, Node  );
    Vec_IntInsert( vLevel, i + 1, Level );
}
void Wlc_BlastPrintMatrix( Gia_Man_t * p, Vec_Wec_t * vProds, int fVerbose )
{
    Vec_Int_t * vSupp = Vec_IntAlloc( 100 );
    Vec_Wrd_t * vTemp = Vec_WrdStart( Gia_ManObjNum(p) );
    Vec_Int_t * vLevel;  word Truth;
    int i, k, iLit; 
    Vec_WecForEachLevel( vProds, vLevel, i )
        Vec_IntForEachEntry( vLevel, iLit, k )
            if ( Gia_ObjIsAnd(Gia_ManObj(p, Abc_Lit2Var(iLit))) )
                Vec_IntPushUnique( vSupp, Abc_Lit2Var(iLit) );
    printf( "Booth partial products: %d pps, %d unique, %d nodes.\n", 
        Vec_WecSizeSize(vProds), Vec_IntSize(vSupp), Gia_ManAndNum(p) );
    Vec_IntPrint( vSupp );

    if ( fVerbose )
    Vec_WecForEachLevel( vProds, vLevel, i )
        Vec_IntForEachEntry( vLevel, iLit, k )
        {
            printf( "Obj = %4d : ", Abc_Lit2Var(iLit) );
            printf( "Compl = %d  ", Abc_LitIsCompl(iLit) );
            printf( "Rank = %2d  ", i );
            Truth = Gia_ObjComputeTruth6Cis( p, iLit, vSupp, vTemp );
            Extra_PrintHex( stdout, (unsigned*)&Truth, Vec_IntSize(vSupp) );
            if ( Vec_IntSize(vSupp) == 4 ) printf( "    " );
            if ( Vec_IntSize(vSupp) == 3 ) printf( "      " );
            if ( Vec_IntSize(vSupp) <= 2 ) printf( "       " );
            printf( "  " );
            Vec_IntPrint( vSupp );
            if ( k == Vec_IntSize(vLevel)-1 )
                printf( "\n" );
        }
    Vec_IntFree( vSupp );
    Vec_WrdFree( vTemp );
}
void Wlc_BlastReduceMatrix( Gia_Man_t * pNew, Vec_Wec_t * vProds, Vec_Wec_t * vLevels, Vec_Int_t * vRes, int fSigned, int fCla )
{
    Vec_Int_t * vLevel, * vProd;
    int i, NodeS, NodeC, LevelS, LevelC, Node1, Node2, Node3, Level1, Level2, Level3;
    int nSize = Vec_WecSize(vProds);
    assert( nSize == Vec_WecSize(vLevels) );
    for ( i = 0; i < nSize; i++ )
    {
        while ( 1 )
        {
            vProd  = Vec_WecEntry( vProds, i );
            if ( Vec_IntSize(vProd) < 3 )
                break;

            Node1  = Vec_IntPop( vProd );
            Node2  = Vec_IntPop( vProd );
            Node3  = Vec_IntPop( vProd );

            vLevel = Vec_WecEntry( vLevels, i );

            Level1 = Vec_IntPop( vLevel );
            Level2 = Vec_IntPop( vLevel );
            Level3 = Vec_IntPop( vLevel );

            Wlc_BlastFullAdder( pNew, Node1, Node2, Node3, &NodeC, &NodeS );
            LevelS = Abc_MaxInt( Abc_MaxInt(Level1, Level2), Level3 ) + 2;
            LevelC = LevelS - 1;

            Wlc_IntInsert( vProd, vLevel, NodeS, LevelS );

            vProd  = Vec_WecEntry( vProds, i+1 );
            vLevel = Vec_WecEntry( vLevels, i+1 );

            Wlc_IntInsert( vProd, vLevel, NodeC, LevelC );
        }
    }

    // make all ranks have two products
    for ( i = 0; i < nSize; i++ )
    {
        vProd  = Vec_WecEntry( vProds, i );
        while ( Vec_IntSize(vProd) < 2 )
            Vec_IntPush( vProd, 0 );
        assert( Vec_IntSize(vProd) == 2 );
    }
//    Vec_WecPrint( vProds, 0 );

    vLevel = Vec_WecEntry( vLevels, 0 );
    Vec_IntClear( vRes );
    Vec_IntClear( vLevel );
    for ( i = 0; i < nSize; i++ )
    {
        vProd  = Vec_WecEntry( vProds, i );
        Vec_IntPush( vRes,   Vec_IntEntry(vProd, 0) );
        Vec_IntPush( vLevel, Vec_IntEntry(vProd, 1) );
    }
    Vec_IntPush( vRes,   0 );
    Vec_IntPush( vLevel, 0 );

    if ( fCla )
        Wlc_BlastAdderCLA( pNew, Vec_IntArray(vRes), Vec_IntArray(vLevel), Vec_IntSize(vRes), fSigned, 0 );
    else
        Wlc_BlastAdder( pNew, Vec_IntArray(vRes), Vec_IntArray(vLevel), Vec_IntSize(vRes), 0 );
}

int Wlc_BlastAddLevel( Gia_Man_t * pNew, int Start )
{
    int i;
    if ( Start == 0 )
        Gia_ManCleanLevels( pNew, 5 * Gia_ManObjNum(pNew) );
    for ( i = Start; i < Gia_ManObjNum(pNew); i++ )
    {
        Gia_Obj_t * pObj = Gia_ManObj( pNew, i );
        if ( Gia_ObjIsAnd(pObj) )
            Gia_ObjSetAndLevel( pNew, pObj );
    }
    return Gia_ManObjNum(pNew);
}
void Wlc_IntInsert2( Gia_Man_t * pNew, Vec_Int_t * vProd, int iLit )
{
    int i, Entry, Level = Gia_ObjLevelId(pNew, Abc_Lit2Var(iLit));
    Vec_IntForEachEntryReverse( vProd, Entry, i )
        if ( Gia_ObjLevelId(pNew, Abc_Lit2Var(Entry)) >= Level )
            break;
    Vec_IntInsert( vProd, i + 1, iLit );
//    Vec_IntForEachEntry( vProd, Entry, i )
//        printf( "Obj=%d(%d) ", Abc_Lit2Var(Entry), Gia_ObjLevelId(pNew, Abc_Lit2Var(Entry)) );
//    printf( "\n" );
}
void Wlc_IntSortCostReverse( Gia_Man_t * pNew, int * pArray, int nSize )
{
    int i, j, best_i;
    for ( i = 0; i < nSize-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nSize; j++ )
            if ( Gia_ObjLevelId(pNew, Abc_Lit2Var(pArray[j])) > Gia_ObjLevelId(pNew, Abc_Lit2Var(pArray[best_i])) )
                best_i = j;
        ABC_SWAP( int, pArray[i], pArray[best_i] );
    }
}
void Wlc_BlastReduceMatrix2( Gia_Man_t * pNew, Vec_Wec_t * vProds, Vec_Int_t * vRes, int fSigned, int fCla )
{
    Vec_Int_t * vProd, * vTemp;
    int i, NodeS, NodeC, Node1, Node2, Node3;
    int Start = Wlc_BlastAddLevel( pNew, 0 );
    int nSize = Vec_WecSize(vProds);
    Vec_WecForEachLevel( vProds, vProd, i )
        Wlc_IntSortCostReverse( pNew, Vec_IntArray(vProd), Vec_IntSize(vProd) );
    for ( i = 0; i < nSize; i++ )
    {
        while ( 1 )
        {
            vProd = Vec_WecEntry( vProds, i );
            if ( Vec_IntSize(vProd) < 3 )
                break;

            Node1  = Vec_IntPop( vProd );
            Node2  = Vec_IntPop( vProd );
            Node3  = Vec_IntPop( vProd );

            assert( Gia_ObjLevelId(pNew, Abc_Lit2Var(Node3)) >= Gia_ObjLevelId(pNew, Abc_Lit2Var(Node2)) );
            assert( Gia_ObjLevelId(pNew, Abc_Lit2Var(Node2)) >= Gia_ObjLevelId(pNew, Abc_Lit2Var(Node1)) );

            Wlc_BlastFullAdder( pNew, Node1, Node2, Node3, &NodeC, &NodeS );
            Start = Wlc_BlastAddLevel( pNew, Start );

            Wlc_IntInsert2( pNew, vProd, NodeS );

            vProd  = Vec_WecEntry( vProds, i+1 );
            Wlc_IntInsert2( pNew, vProd, NodeC );
        }
    }

    // make all ranks have two products
    for ( i = 0; i < nSize; i++ )
    {
        vProd  = Vec_WecEntry( vProds, i );
        while ( Vec_IntSize(vProd) < 2 )
            Vec_IntPush( vProd, 0 );
        assert( Vec_IntSize(vProd) == 2 );
    }
//    Vec_WecPrint( vProds, 0 );

    Vec_IntClear( vRes );
    vTemp = Vec_IntAlloc( 100 );
    for ( i = 0; i < nSize; i++ )
    {
        vProd  = Vec_WecEntry( vProds, i );
        Vec_IntPush( vRes,  Vec_IntEntry(vProd, 0) );
        Vec_IntPush( vTemp, Vec_IntEntry(vProd, 1) );
    }
    Vec_IntPush( vRes,  0 );
    Vec_IntPush( vTemp, 0 );

    if ( fCla )
        Wlc_BlastAdderCLA( pNew, Vec_IntArray(vRes), Vec_IntArray(vTemp), Vec_IntSize(vRes), fSigned, 0 );
    else
        Wlc_BlastAdder( pNew, Vec_IntArray(vRes), Vec_IntArray(vTemp), Vec_IntSize(vRes), 0 );
    Vec_IntFree( vTemp );
}


void Wlc_BlastMultiplier3( Gia_Man_t * pNew, int * pArgA, int * pArgB, int nArgA, int nArgB, Vec_Int_t * vRes, int fSigned, int fCla, Vec_Wec_t ** pvProds )
{
    Vec_Wec_t * vProds  = Vec_WecStart( nArgA + nArgB );
    Vec_Wec_t * vLevels = Vec_WecStart( nArgA + nArgB );
    int i, k;
    for ( i = 0; i < nArgA; i++ )
        for ( k = 0; k < nArgB; k++ )
        {
            int fCompl = fSigned && ((i == nArgA-1) ^ (k == nArgB-1));
            Vec_WecPush( vProds,  i+k, Abc_LitNotCond(Gia_ManHashAnd(pNew, pArgA[i], pArgB[k]), fCompl) );
            Vec_WecPush( vLevels, i+k, 0 );
        }
    if ( fSigned )
    {
        Vec_WecPush( vProds,  nArgA, 1 );
        Vec_WecPush( vLevels, nArgA, 0 );

        Vec_WecPush( vProds,  nArgA+nArgB-1, 1 );
        Vec_WecPush( vLevels, nArgA+nArgB-1, 0 );
    }

    if ( pvProds )
        *pvProds = Vec_WecDup(vProds);
    else
        Wlc_BlastReduceMatrix( pNew, vProds, vLevels, vRes, fSigned, fCla );
//    Wlc_BlastReduceMatrix2( pNew, vProds, vRes, fSigned, fCla );

    Vec_WecFree( vProds );
    Vec_WecFree( vLevels );
}
void Wlc_BlastSquare( Gia_Man_t * pNew, int * pNum, int nNum, Vec_Int_t * vTmp, Vec_Int_t * vRes )
{
    Vec_Wec_t * vProds  = Vec_WecStart( 2*nNum );
    Vec_Wec_t * vLevels = Vec_WecStart( 2*nNum );
    int i, k;
    for ( i = 0; i < nNum; i++ )
        for ( k = 0; k < nNum; k++ )
        {
            if ( i == k )
            {
                Vec_WecPush( vProds,  i+k, pNum[i] );
                Vec_WecPush( vLevels, i+k, 0 );
            }
            else if ( i < k )
            {
                Vec_WecPush( vProds,  i+k+1, Gia_ManHashAnd(pNew, pNum[i], pNum[k]) );
                Vec_WecPush( vLevels, i+k+1, 0 );
            }
        }

    Wlc_BlastReduceMatrix( pNew, vProds, vLevels, vRes, 0, 0 );

    Vec_WecFree( vProds );
    Vec_WecFree( vLevels );
}
void Wlc_BlastDecoder( Gia_Man_t * pNew, int * pNum, int nNum, Vec_Int_t * vTmp, Vec_Int_t * vRes )
{
    int i, k, nMints = 1 << nNum;
    Vec_IntClear( vRes );
    for ( i = 0; i < nMints; i++ )
    {
        int iMint = 1;
        for ( k = 0; k < nNum; k++ )
            iMint = Gia_ManHashAnd( pNew, iMint, Abc_LitNotCond(pNum[k], !((i >> k) & 1)) );
        Vec_IntPush( vRes, iMint );
    }
}
void Wlc_BlastBooth( Gia_Man_t * pNew, int * pArgA, int * pArgB, int nArgA, int nArgB, Vec_Int_t * vRes, int fSigned, int fCla, Vec_Wec_t ** pvProds )
{
    Vec_Wec_t * vProds  = Vec_WecStart( nArgA + nArgB + 3 );
    Vec_Wec_t * vLevels = Vec_WecStart( nArgA + nArgB + 3 );
    int FillA = fSigned ? pArgA[nArgA-1] : 0;
    int FillB = fSigned ? pArgB[nArgB-1] : 0;
    int i, k, Sign;

    // extend argument B
    Vec_Int_t * vArgB = Vec_IntAlloc( nArgB + 2 );
    Vec_IntPush( vArgB, 0 );
    for ( i = 0; i < nArgB; i++ )
        Vec_IntPush( vArgB, pArgB[i] );
    if ( !fSigned )
        Vec_IntPushTwo( vArgB, FillB, FillB );
    if ( Vec_IntSize(vArgB) % 2 == 0 )
        Vec_IntPush( vArgB, FillB );
    assert( Vec_IntSize(vArgB) % 2 == 1 );

    // iterate through bit-pairs of B
    for ( k = 0; k+2 < Vec_IntSize(vArgB); k+=2 )
    {
        int pp    = -1;
        int Q2jM1 = Vec_IntEntry(vArgB, k);   // q(2*j-1)
        int Q2j   = Vec_IntEntry(vArgB, k+1); // q(2*j+0)
        int Q2jP1 = Vec_IntEntry(vArgB, k+2); // q(2*j+1)
        int Neg   = Q2jP1;
        int One   = Gia_ManHashXor( pNew, Q2j, Q2jM1 );
        int Two   = Gia_ManHashMux( pNew, Neg, Gia_ManHashAnd(pNew, Abc_LitNot(Q2j), Abc_LitNot(Q2jM1)), Gia_ManHashAnd(pNew, Q2j, Q2jM1) );
        for ( i = 0; i <= nArgA; i++ )
        {
            int This = i == nArgA ? FillA : pArgA[i];
            int Prev = i ? pArgA[i-1] : 0;
            int Part = Gia_ManHashOr( pNew, Gia_ManHashAnd(pNew, One, This), Gia_ManHashAnd(pNew, Two, Prev) );
            pp = Gia_ManHashXor( pNew, Part, Neg );
            if ( pp == 0 || (fSigned && i == nArgA) )
                continue;
            if ( pp )
            {
                Vec_WecPush( vProds,  k+i, pp );
                Vec_WecPush( vLevels, k+i, 0 );
            }
        }
        if ( fSigned ) i--;
        // perform sign extension
        Sign = fSigned ? pp : Neg;
        if ( k == 0 )
        {
            Vec_WecPush( vProds,  k+i, Sign );
            Vec_WecPush( vLevels, k+i, 0 );

            Vec_WecPush( vProds,  k+i+1, Sign );
            Vec_WecPush( vLevels, k+i+1, 0 );

            if ( Sign != 1 )
            {
            Vec_WecPush( vProds,  k+i+2, Abc_LitNot(Sign) );
            Vec_WecPush( vLevels, k+i+2, 0 );
            }
        }
        else 
        {
            if ( Sign != 1 )
            {
            Vec_WecPush( vProds,  k+i, Abc_LitNot(Sign) );
            Vec_WecPush( vLevels, k+i, 0 );
            }

            Vec_WecPush( vProds,  k+i+1, 1 );
            Vec_WecPush( vLevels, k+i+1, 0 );
        }
        // add neg to the first column
        if ( Neg == 0 )
            continue;
        Vec_WecPush( vProds,  k, Neg );
        Vec_WecPush( vLevels, k, 0 );
    }
    //Vec_WecPrint( vProds, 0 );
    //Wlc_BlastPrintMatrix( pNew, vProds, 1 );
    //printf( "Cutoff ID for partial products = %d.\n", Gia_ManObjNum(pNew) );
    if ( pvProds )
        *pvProds = Vec_WecDup(vProds);
    else
        Wlc_BlastReduceMatrix( pNew, vProds, vLevels, vRes, fSigned, fCla );
    //    Wlc_BlastReduceMatrix2( pNew, vProds, vRes, fSigned, fCla );
    Vec_WecFree( vProds );
    Vec_WecFree( vLevels );
    Vec_IntFree( vArgB );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Wlc_NtkBitBlast( Wlc_Ntk_t * p, Wlc_BstPar_t * pParIn )
{
    int fVerbose = 0;
    int fUseOldMultiplierBlasting = 0;
    int fSkipBitRange = 0;
    Tim_Man_t * pManTime = NULL;
    If_LibBox_t * pBoxLib = NULL;
    Vec_Ptr_t * vTables = NULL;
    Vec_Int_t * vFf2Ci = Vec_IntAlloc( 100 );
    Vec_Int_t * vRegClasses = NULL;
    Gia_Man_t * pTemp, * pNew, * pExtra = NULL;
    Wlc_Obj_t * pObj, * pObj2;
    Vec_Int_t * vBits = &p->vBits, * vTemp0, * vTemp1, * vTemp2, * vRes, * vAddOutputs = NULL, * vAddObjs = NULL;
    int nBits = Wlc_NtkPrepareBits( p );
    int nRange, nRange0, nRange1, nRange2, nRange3;
    int i, k, b, iFanin, iLit, nAndPrev, * pFans0, * pFans1, * pFans2, * pFans3;
    int nFFins = 0, nFFouts = 0, curPi = 0, curPo = 0, nFf2Regs = 0;
    int nBitCis = 0, nBitCos = 0, fAdded = 0;
    Wlc_BstPar_t Par, * pPar = &Par;
    Wlc_BstParDefault( pPar );
    pPar = pParIn ? pParIn : pPar;
    Vec_IntClear( vBits );
    Vec_IntGrow( vBits, nBits );
    vTemp0 = Vec_IntAlloc( 1000 );
    vTemp1 = Vec_IntAlloc( 1000 );
    vTemp2 = Vec_IntAlloc( 1000 );
    vRes   = Vec_IntAlloc( 1000 );
    // clean AND-gate counters
    memset( p->nAnds, 0, sizeof(int) * WLC_OBJ_NUMBER );
    // create AIG manager
    pNew = Gia_ManStart( 5 * Wlc_NtkObjNum(p) + 1000 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->fGiaSimple = pPar->fGiaSimple;
    if ( !pPar->fGiaSimple )
        Gia_ManHashAlloc( pNew );
    if ( pPar->fAddOutputs )
        vAddOutputs = Vec_IntAlloc( 100 );
    if ( pPar->fAddOutputs )
        vAddObjs = Vec_IntAlloc( 100 );
    // prepare for AIG with boxes
    if ( pPar->vBoxIds )
    {
        int nNewCis = 0, nNewCos = 0;
        assert( Vec_IntSize(&p->vFfs2) == 0 );
        Wlc_NtkForEachObj( p, pObj, i )
            pObj->Mark = 0;
        // count bit-width of regular CIs/COs
        Wlc_NtkForEachCi( p, pObj, i )
            nBitCis += Wlc_ObjRange( pObj );
        Wlc_NtkForEachCo( p, pObj, i )
            nBitCos += Wlc_ObjRange( pObj );
        // count bit-width of additional CIs/COs due to selected multipliers
        assert( Vec_IntSize(pPar->vBoxIds) > 0 );
        Wlc_NtkForEachObjVec( pPar->vBoxIds, p, pObj, i )
        {
            // currently works only for multipliers
            assert( pObj->Type == WLC_OBJ_ARI_MULTI || pObj->Type == WLC_OBJ_ARI_ADD );
            nNewCis += Wlc_ObjRange( pObj );
            nNewCos += Wlc_ObjRange( Wlc_ObjFanin0(p, pObj) );
            nNewCos += Wlc_ObjRange( Wlc_ObjFanin1(p, pObj) );
            if ( Wlc_ObjFaninNum(pObj) > 2 )
            nNewCos += Wlc_ObjRange( Wlc_ObjFanin2(p, pObj) );
            pObj->Mark = 1;
        }
        // create hierarchy manager
        pManTime = Tim_ManStart( nBitCis + nNewCis, nBitCos + nNewCos );
        curPi = nBitCis;
        curPo = 0;
        // create AIG manager for logic of the boxes
        pExtra = Gia_ManStart( Wlc_NtkObjNum(p) );
        Gia_ManHashAlloc( pExtra );
        assert( !pPar->fGiaSimple );
        // create box library
        pBoxLib = If_LibBoxStart();
    }
    if ( Vec_IntSize(&p->vFfs2) > 0 )
    {
        Vec_Int_t * vSignature;
        int nNewCis = 0, nNewCos = 0;
        assert( pPar->vBoxIds == 0 );
        // count bit-width of regular CIs/COs
        Wlc_NtkForEachCi( p, pObj, i )
            nBitCis += Wlc_ObjRange( pObj );
        Wlc_NtkForEachCo( p, pObj, i )
            nBitCos += Wlc_ObjRange( pObj );
        // count bit-width of additional CIs/COs due to selected multipliers
        Wlc_NtkForEachFf2( p, pObj, i )
        {
            // currently works only for multipliers
            assert( pObj->Type == WLC_OBJ_FF );
            assert( Wlc_ObjRange(pObj) == Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) );
            nNewCis += Wlc_ObjRange(pObj);
            nNewCos += 2*Wlc_ObjRange(pObj) + 3;
            nFf2Regs+= Wlc_ObjRange(pObj);
        }
        // create hierarchy manager
        pManTime = Tim_ManStart( nBitCis + nNewCis + nFf2Regs, nBitCos + nNewCos + nFf2Regs );
        curPi = nBitCis + nFf2Regs;
        curPo = 0;
        // create AIG manager for logic of the boxes
        pExtra = Gia_ManStart( Wlc_NtkObjNum(p) );
        Gia_ManHashAlloc( pExtra );
        assert( !pPar->fGiaSimple );
        // create register classes
        vRegClasses = Vec_IntAlloc(0);
        vSignature = Vec_IntAlloc( 100 );
        Vec_IntPushTwo( vSignature, -1, -1 );
        Wlc_NtkForEachFf2( p, pObj, i )
        {
            int iClk0,  iClk  = Wlc_ObjFaninId( pObj, 1 );
            int iAsyn0, iAsyn = Wlc_ObjFaninId( pObj, 5 );
            nRange = Wlc_ObjRange(pObj);
            Vec_IntForEachEntryDouble( vSignature, iClk0, iAsyn0, k )
                if ( iClk == iClk0 && iAsyn == iAsyn0 )
                {
                    for ( b = 0; b < nRange; b++ )
                        Vec_IntPush( vRegClasses, k/2 );
                    break;
                }
            if ( k < Vec_IntSize(vSignature) )
                continue;
            for ( b = 0; b < nRange; b++ )
                Vec_IntPush( vRegClasses, k/2 );
            Vec_IntPushTwo( vSignature, iClk, iAsyn );
        }
        assert( Vec_IntSize(vRegClasses) == nFf2Regs );
        Vec_IntFree( vSignature );
        // create box library
        pBoxLib = If_LibBoxStart();
    }
    //printf( "Init state: %s\n", p->pInits );

    // blast in the topological order
    Wlc_NtkForEachObj( p, pObj, i )
    {
        //char * pName1 = Wlc_ObjName(p, i);
        //char * pName2 = Wlc_ObjFaninNum(pObj) ? Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)) : NULL;

        nAndPrev = Gia_ManAndNum(pNew);
        nRange  = Wlc_ObjRange( pObj );
        nRange0 = Wlc_ObjFaninNum(pObj) > 0 ? Wlc_ObjRange( Wlc_ObjFanin0(p, pObj) ) : -1;
        nRange1 = Wlc_ObjFaninNum(pObj) > 1 ? Wlc_ObjRange( Wlc_ObjFanin1(p, pObj) ) : -1;
        nRange2 = Wlc_ObjFaninNum(pObj) > 2 ? Wlc_ObjRange( Wlc_ObjFanin2(p, pObj) ) : -1;
        nRange3 = Wlc_ObjFaninNum(pObj) > 3 ? Wlc_ObjRange( Wlc_ObjFanin(p, pObj, 3) ) : -1;
        pFans0  = pObj->Type != WLC_OBJ_FF && Wlc_ObjFaninNum(pObj) > 0 ? Vec_IntEntryP( vBits, Wlc_ObjCopy(p, Wlc_ObjFaninId0(pObj)) ) : NULL;
        pFans1  = pObj->Type != WLC_OBJ_FF && Wlc_ObjFaninNum(pObj) > 1 ? Vec_IntEntryP( vBits, Wlc_ObjCopy(p, Wlc_ObjFaninId1(pObj)) ) : NULL;
        pFans2  = pObj->Type != WLC_OBJ_FF && Wlc_ObjFaninNum(pObj) > 2 ? Vec_IntEntryP( vBits, Wlc_ObjCopy(p, Wlc_ObjFaninId2(pObj)) ) : NULL;
        pFans3  = pObj->Type != WLC_OBJ_FF && Wlc_ObjFaninNum(pObj) > 3 ? Vec_IntEntryP( vBits, Wlc_ObjCopy(p, Wlc_ObjFaninId(pObj,3)) ) : NULL;
        Vec_IntClear( vRes );
        assert( nRange > 0 );
        if ( pPar->vBoxIds && pObj->Mark )
        {
            If_Box_t * pBox;
            char Buffer[100];
            float * pTable;
            int CarryIn = 0;

            pObj->Mark = 0;
            assert( pObj->Type == WLC_OBJ_ARI_MULTI || pObj->Type == WLC_OBJ_ARI_ADD || pObj->Type == WLC_OBJ_ARI_SUB );

            // account for carry-in
            if ( Wlc_ObjFaninNum(pObj) == 3 )
                assert( nRange2 == 1 );
            else
                nRange2 = 0;

            // create new box
            if ( vTables == NULL )
                Tim_ManSetDelayTables( pManTime, (vTables = Vec_PtrAlloc(100)) );
            Tim_ManCreateBox( pManTime, curPo, nRange0 + nRange1 + nRange2, curPi, nRange, Vec_PtrSize(vTables), 0 );
            curPi += nRange;
            curPo += nRange0 + nRange1 + nRange2;

            // create delay table
            pTable = ABC_ALLOC( float, 3 + nRange * (nRange0 + nRange1 + nRange2) );
            pTable[0] = Vec_PtrSize(vTables);
            pTable[1] = nRange0 + nRange1 + nRange2;
            pTable[2] = nRange;
            for ( k = 0; k < nRange * (nRange0 + nRange1 + nRange2); k++ )
                pTable[3 + k] = 1.0;
            Vec_PtrPush( vTables, pTable );

            // create combinational outputs in the normal manager
            for ( k = 0; k < nRange0; k++ )
                Gia_ManAppendCo( pNew, pFans0[k] );
            for ( k = 0; k < nRange1; k++ )
                Gia_ManAppendCo( pNew, pFans1[k] );
            for ( k = 0; k < nRange2; k++ )
                Gia_ManAppendCo( pNew, pFans2[0] );

            // make sure there is enough primary inputs in the manager
            for ( k = Gia_ManPiNum(pExtra); k < nRange0 + nRange1 + nRange2; k++ )
                Gia_ManAppendCi( pExtra );
            // create combinational inputs
            Vec_IntClear( vTemp0 );
            for ( k = 0; k < nRange0; k++ )
                Vec_IntPush( vTemp0, Gia_Obj2Lit(pExtra, Gia_ManPi(pExtra, k)) );
            Vec_IntClear( vTemp1 );
            for ( k = 0; k < nRange1; k++ )
                Vec_IntPush( vTemp1, Gia_Obj2Lit(pExtra, Gia_ManPi(pExtra, nRange0+k)) );
            if ( nRange2 == 1 )
                CarryIn = Gia_Obj2Lit(pExtra, Gia_ManPi(pExtra, nRange0+nRange1));

            // get new fanin arrays
            pFans0 = Vec_IntArray( vTemp0 );
            pFans1 = Vec_IntArray( vTemp1 );

            // bit-blast in the external manager
            if ( pObj->Type == WLC_OBJ_ARI_ADD || pObj->Type == WLC_OBJ_ARI_SUB ) 
            {
                int nRangeMax = Abc_MaxInt( nRange, Abc_MaxInt(nRange0, nRange1) );
                int * pArg0 = Wlc_VecLoadFanins( vRes,   pFans0, nRange0, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
                int * pArg1 = Wlc_VecLoadFanins( vTemp1, pFans1, nRange1, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
                if ( pObj->Type == WLC_OBJ_ARI_ADD )
                    Wlc_BlastAdder( pExtra, pArg0, pArg1, nRange, CarryIn ); // result is in pFan0 (vRes)
                else 
                    Wlc_BlastSubtract( pExtra, pArg0, pArg1, nRange, 1 ); // result is in pFan0 (vRes)
                Vec_IntShrink( vRes, nRange );
            }
            else if ( fUseOldMultiplierBlasting )
            {                
                int nRangeMax = Abc_MaxInt( nRange, Abc_MaxInt(nRange0, nRange1) );
                int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
                int * pArg1 = Wlc_VecLoadFanins( vTemp1, pFans1, nRange1, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
                Wlc_BlastMultiplier2( pExtra, pArg0, pArg1, nRange, vTemp2, vRes );
                Vec_IntShrink( vRes, nRange );
            }
            else
            {
                int fSigned = Wlc_ObjIsSignedFanin01(p, pObj);
                int nRangeMax = Abc_MaxInt(nRange0, nRange1);
                int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRangeMax, fSigned );
                int * pArg1 = Wlc_VecLoadFanins( vTemp1, pFans1, nRange1, nRangeMax, fSigned );
                Wlc_BlastMultiplier( pExtra, pArg0, pArg1, nRangeMax, nRangeMax, vTemp2, vRes, fSigned );
                if ( nRange > nRangeMax + nRangeMax )
                    Vec_IntFillExtra( vRes, nRange, fSigned ? Vec_IntEntryLast(vRes) : 0 );
                else
                    Vec_IntShrink( vRes, nRange );
                assert( Vec_IntSize(vRes) == nRange );
            }
            // create outputs in the external manager
            for ( k = 0; k < nRange; k++ )
                Gia_ManAppendCo( pExtra, Vec_IntEntry(vRes, k) );

            // create combinational inputs in the normal manager
            Vec_IntClear( vRes );
            for ( k = 0; k < nRange; k++ )
                Vec_IntPush( vRes, Gia_ManAppendCi(pNew) );

            // add box to the library
            sprintf( Buffer, "%s%03d", pObj->Type == WLC_OBJ_ARI_ADD ? "add":"mul", 1+If_LibBoxNum(pBoxLib) );
            pBox = If_BoxStart( Abc_UtilStrsav(Buffer), 1+If_LibBoxNum(pBoxLib), nRange0 + nRange1 + nRange2, nRange, 0, 0, 0 ); 
            If_LibBoxAdd( pBoxLib, pBox );
            for ( k = 0; k < pBox->nPis * pBox->nPos; k++ )
                pBox->pDelays[k] = 1;
        }
        else if ( Wlc_ObjIsCi(pObj) || pObj->Type == WLC_OBJ_FF ) // assuming that FFs are ordered immediately after PIs
        {
            if ( pObj->Type == WLC_OBJ_FF )
                Vec_IntPush( vFf2Ci, Gia_ManCiNum(pNew) );
            if ( Wlc_ObjRangeIsReversed(pObj) )
            {
                for ( k = 0; k < nRange; k++ )
                    Vec_IntPush( vRes, -1 );
                for ( k = 0; k < nRange; k++ )
                    Vec_IntWriteEntry( vRes, Vec_IntSize(vRes)-1-k, Gia_ManAppendCi(pNew) );
            }
            else
            {
                for ( k = 0; k < nRange; k++ )
                    Vec_IntPush( vRes, Gia_ManAppendCi(pNew) );
            }
            if ( pObj->Type == WLC_OBJ_FO )
                nFFouts += Vec_IntSize(vRes);
            if ( pObj->Type == WLC_OBJ_FF )
            {
                // complement flop output whose init state is 1
            }
        }
        else if ( pObj->Type == WLC_OBJ_BUF )
        {
            int nRangeMax = Abc_MaxInt( nRange0, nRange );
            int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRangeMax, Wlc_ObjIsSignedFanin0(p, pObj) );
            for ( k = 0; k < nRange; k++ )
                Vec_IntPush( vRes, pArg0[k] );
        }
        else if ( pObj->Type == WLC_OBJ_CONST )
        {
            word * pTruth = (word *)Wlc_ObjFanins(pObj);
            for ( k = 0; k < nRange; k++ )
                Vec_IntPush( vRes, Abc_TtGetBit(pTruth, k) );
        }
        else if ( pObj->Type == WLC_OBJ_MUX )
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
            int fSigned = 1;
            assert( nRange0 >= 1 && Wlc_ObjFaninNum(pObj) >= 3 );
            assert( 1 + (1 << nRange0) == Wlc_ObjFaninNum(pObj) );
            Wlc_ObjForEachFanin( pObj, iFanin, k )
                if ( k > 0 )
                    fSigned &= Wlc_NtkObj(p, iFanin)->Signed;
            Vec_IntClear( vTemp1 );
            if ( pPar->fDecMuxes )
            {
                for ( k = 0; k < (1 << nRange0); k++ )
                {
                    int iLitAnd = 1;
                    for ( b = 0; b < nRange0; b++ )
                        iLitAnd = Gia_ManHashAnd( pNew, iLitAnd, Abc_LitNotCond(pFans0[b], ((k >> b) & 1) == 0) );
                    Vec_IntPush( vTemp1, iLitAnd );
                }
            }
            for ( b = 0; b < nRange; b++ )
            {
                Vec_IntClear( vTemp0 );
                Wlc_ObjForEachFanin( pObj, iFanin, k )
                    if ( k > 0 )
                    {
                        nRange1 = Wlc_ObjRange( Wlc_NtkObj(p, iFanin) );
                        pFans1  = Vec_IntEntryP( vBits, Wlc_ObjCopy(p, iFanin) );
                        if ( Wlc_ObjFaninNum(pObj) == 3 ) // Statement 1
                            Vec_IntPush( vTemp0, b < nRange1 ? pFans1[b] : (fSigned? pFans1[nRange1-1] : 0) );
                        else // Statement 2
                            Vec_IntPush( vTemp0, b < nRange1 ? pFans1[b] : (Wlc_NtkObj(p, iFanin)->Signed? pFans1[nRange1-1] : 0) );
                    }
                if ( pPar->fDecMuxes )
                    Vec_IntPush( vRes, Wlc_NtkMuxTree2(pNew, pFans0, nRange0, vTemp0, vTemp1, vTemp2) );
                else
                    Vec_IntPush( vRes, Wlc_NtkMuxTree_rec(pNew, pFans0, nRange0, vTemp0, 0) );
            }
        }
        else if ( pObj->Type == WLC_OBJ_SEL )
        {
            assert( nRange0 == Wlc_ObjFaninNum(pObj)-1 );
            Vec_IntClear( vTemp1 );
            for ( k = 0; k < nRange0; k++ )
                Vec_IntPush( vTemp1, pFans0[k] );
            for ( b = 0; b < nRange; b++ )
            {
                Vec_IntClear( vTemp0 );
                Wlc_ObjForEachFanin( pObj, iFanin, k )
                    if ( k > 0 )
                    {
                        Wlc_Obj_t * pFanin = Wlc_NtkObj(p, iFanin);
                        assert( nRange == Wlc_ObjRange(pFanin) );
                        pFans1 = Vec_IntEntryP( vBits, Wlc_ObjCopy(p, iFanin) );
                        Vec_IntPush( vTemp0, pFans1[b] );
                    }
                Vec_IntPush( vRes, Wlc_NtkMuxTree2(pNew, NULL, 0, vTemp0, vTemp1, vTemp2) );
            }
        }
        else if ( pObj->Type == WLC_OBJ_SHIFT_R || pObj->Type == WLC_OBJ_SHIFT_RA ||
                  pObj->Type == WLC_OBJ_SHIFT_L || pObj->Type == WLC_OBJ_SHIFT_LA )
        {
            int nRangeMax = Abc_MaxInt( nRange, nRange0 );
            int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRangeMax, Wlc_ObjIsSignedFanin0(p, pObj) );
            if ( pObj->Type == WLC_OBJ_SHIFT_R || pObj->Type == WLC_OBJ_SHIFT_RA )
                Wlc_BlastShiftRight( pNew, pArg0, nRangeMax, pFans1, nRange1, Wlc_ObjIsSignedFanin0(p, pObj) && pObj->Type == WLC_OBJ_SHIFT_RA, vRes );
            else
                Wlc_BlastShiftLeft( pNew, pArg0, nRangeMax, pFans1, nRange1, 0, vRes );
            Vec_IntShrink( vRes, nRange );
        }
        else if ( pObj->Type == WLC_OBJ_ROTATE_R )
        {
            assert( nRange0 == nRange );
            Wlc_BlastRotateRight( pNew, pFans0, nRange0, pFans1, nRange1, vRes );
        }
        else if ( pObj->Type == WLC_OBJ_ROTATE_L )
        {
            assert( nRange0 == nRange );
            Wlc_BlastRotateLeft( pNew, pFans0, nRange0, pFans1, nRange1, vRes );
        }
        else if ( pObj->Type == WLC_OBJ_BIT_NOT )
        {
            int nRangeMax = Abc_MaxInt( nRange, nRange0 );
            int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRangeMax, Wlc_ObjIsSignedFanin0(p, pObj) );
            for ( k = 0; k < nRange; k++ )
                Vec_IntPush( vRes, Abc_LitNot(pArg0[k]) );
        }
        else if ( pObj->Type == WLC_OBJ_BIT_AND || pObj->Type == WLC_OBJ_BIT_NAND )
        {
            int nRangeMax = Abc_MaxInt( nRange, Abc_MaxInt(nRange0, nRange1) );
            int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
            int * pArg1 = Wlc_VecLoadFanins( vTemp1, pFans1, nRange1, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
            for ( k = 0; k < nRange; k++ )
                Vec_IntPush( vRes, Abc_LitNotCond(Gia_ManHashAnd(pNew, pArg0[k], pArg1[k]), pObj->Type == WLC_OBJ_BIT_NAND) );
        }
        else if ( pObj->Type == WLC_OBJ_BIT_OR || pObj->Type == WLC_OBJ_BIT_NOR )
        {
            int nRangeMax = Abc_MaxInt( nRange, Abc_MaxInt(nRange0, nRange1) );
            int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
            int * pArg1 = Wlc_VecLoadFanins( vTemp1, pFans1, nRange1, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
            for ( k = 0; k < nRange; k++ )
                Vec_IntPush( vRes, Abc_LitNotCond(Gia_ManHashOr(pNew, pArg0[k], pArg1[k]), pObj->Type == WLC_OBJ_BIT_NOR) );
        }
        else if ( pObj->Type == WLC_OBJ_BIT_XOR || pObj->Type == WLC_OBJ_BIT_NXOR )
        {
            int nRangeMax = Abc_MaxInt( nRange, Abc_MaxInt(nRange0, nRange1) );
            int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
            int * pArg1 = Wlc_VecLoadFanins( vTemp1, pFans1, nRange1, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
            for ( k = 0; k < nRange; k++ )
                Vec_IntPush( vRes, Abc_LitNotCond(Gia_ManHashXor(pNew, pArg0[k], pArg1[k]), pObj->Type == WLC_OBJ_BIT_NXOR) );
        }
        else if ( pObj->Type == WLC_OBJ_BIT_SELECT )
        {
            Wlc_Obj_t * pFanin = Wlc_ObjFanin0(p, pObj);
            int End = Wlc_ObjRangeEnd(pObj);
            int Beg = Wlc_ObjRangeBeg(pObj);
            if ( End >= Beg )
            {
                assert( nRange == End - Beg + 1 );
                assert( pFanin->Beg <= Beg && End <= pFanin->End );
                for ( k = Beg; k <= End; k++ )
                    Vec_IntPush( vRes, pFans0[k - pFanin->Beg] );
            }
            else
            {
                assert( nRange == Beg - End + 1 );
                assert( pFanin->End <= End && Beg <= pFanin->Beg );
                for ( k = End; k <= Beg; k++ )
                    Vec_IntPush( vRes, pFans0[k - pFanin->End] );
            }
        }
        else if ( pObj->Type == WLC_OBJ_BIT_CONCAT )
        {
            int iFanin, nTotal = 0;
            Wlc_ObjForEachFanin( pObj, iFanin, k )
                nTotal += Wlc_ObjRange( Wlc_NtkObj(p, iFanin) );
            assert( nRange == nTotal );
            Wlc_ObjForEachFaninReverse( pObj, iFanin, k )
            {
                nRange0 = Wlc_ObjRange( Wlc_NtkObj(p, iFanin) );
                pFans0 = Vec_IntEntryP( vBits, Wlc_ObjCopy(p, iFanin) );
                for ( b = 0; b < nRange0; b++ )
                    Vec_IntPush( vRes, pFans0[b] );
            }
        }
        else if ( pObj->Type == WLC_OBJ_BIT_ZEROPAD || pObj->Type == WLC_OBJ_BIT_SIGNEXT )
        {
            int Pad = pObj->Type == WLC_OBJ_BIT_ZEROPAD ? 0 : pFans0[nRange0-1];
            assert( nRange0 <= nRange );
            for ( k = 0; k < nRange0; k++ )
                Vec_IntPush( vRes, pFans0[k] );
            for (      ; k < nRange; k++ )
                Vec_IntPush( vRes, Pad );
        }
        else if ( pObj->Type == WLC_OBJ_LOGIC_NOT )
        {
            iLit = Wlc_BlastReduction( pNew, pFans0, nRange0, WLC_OBJ_REDUCT_OR );
            Vec_IntFill( vRes, 1, Abc_LitNot(iLit) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
        }
        else if ( pObj->Type == WLC_OBJ_LOGIC_IMPL )
        {
            int iLit0 = Wlc_BlastReduction( pNew, pFans0, nRange0, WLC_OBJ_REDUCT_OR );
            int iLit1 = Wlc_BlastReduction( pNew, pFans1, nRange1, WLC_OBJ_REDUCT_OR );
            Vec_IntFill( vRes, 1, Gia_ManHashOr(pNew, Abc_LitNot(iLit0), iLit1) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
        }
        else if ( pObj->Type == WLC_OBJ_LOGIC_AND )
        {
            int iLit0 = Wlc_BlastReduction( pNew, pFans0, nRange0, WLC_OBJ_REDUCT_OR );
            int iLit1 = Wlc_BlastReduction( pNew, pFans1, nRange1, WLC_OBJ_REDUCT_OR );
            Vec_IntFill( vRes, 1, Gia_ManHashAnd(pNew, iLit0, iLit1) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
        }
        else if ( pObj->Type == WLC_OBJ_LOGIC_OR )
        {
            int iLit0 = Wlc_BlastReduction( pNew, pFans0, nRange0, WLC_OBJ_REDUCT_OR );
            int iLit1 = Wlc_BlastReduction( pNew, pFans1, nRange1, WLC_OBJ_REDUCT_OR );
            Vec_IntFill( vRes, 1, Gia_ManHashOr(pNew, iLit0, iLit1) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
        }
        else if ( pObj->Type == WLC_OBJ_LOGIC_XOR )
        {
            int iLit0 = Wlc_BlastReduction( pNew, pFans0, nRange0, WLC_OBJ_REDUCT_OR );
            int iLit1 = Wlc_BlastReduction( pNew, pFans1, nRange1, WLC_OBJ_REDUCT_OR );
            Vec_IntFill( vRes, 1, Gia_ManHashXor(pNew, iLit0, iLit1) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
        }
        else if ( pObj->Type == WLC_OBJ_COMP_NOTEQU && Wlc_ObjFaninNum(pObj) > 2 )
        {
            // find the max range
            int a, b, iRes = 1, nRangeMax = Abc_MaxInt( nRange0, nRange1 );
            for ( k = 2; k < Wlc_ObjFaninNum(pObj); k++ )
                nRangeMax = Abc_MaxInt( nRangeMax, Wlc_ObjRange( Wlc_NtkObj(p, Wlc_ObjFaninId(pObj, k)) ) );
            // create pairwise distinct
            for ( a = 0; a < Wlc_ObjFaninNum(pObj); a++ )
            for ( b = a+1; b < Wlc_ObjFaninNum(pObj); b++ )
            {
                int nRange0 = Wlc_ObjRange( Wlc_NtkObj(p, Wlc_ObjFaninId(pObj, a)) );
                int nRange1 = Wlc_ObjRange( Wlc_NtkObj(p, Wlc_ObjFaninId(pObj, b)) );
                int * pFans0 = Vec_IntEntryP( vBits, Wlc_ObjCopy(p, Wlc_ObjFaninId(pObj, a)) );
                int * pFans1 = Vec_IntEntryP( vBits, Wlc_ObjCopy(p, Wlc_ObjFaninId(pObj, b)) );
                int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRangeMax, 0 );
                int * pArg1 = Wlc_VecLoadFanins( vTemp1, pFans1, nRange1, nRangeMax, 0 );
                int iLit = 0;
                for ( k = 0; k < nRangeMax; k++ )
                    iLit = Gia_ManHashOr( pNew, iLit, Gia_ManHashXor(pNew, pArg0[k], pArg1[k]) ); 
                iRes = Gia_ManHashAnd( pNew, iRes, iLit ); 
            }
            Vec_IntFill( vRes, 1, iRes );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
        }
        else if ( pObj->Type == WLC_OBJ_COMP_EQU || pObj->Type == WLC_OBJ_COMP_NOTEQU )
        {
            int iLit = 0, nRangeMax = Abc_MaxInt( nRange0, nRange1 );
            int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
            int * pArg1 = Wlc_VecLoadFanins( vTemp1, pFans1, nRange1, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
            for ( k = 0; k < nRangeMax; k++ )
                iLit = Gia_ManHashOr( pNew, iLit, Gia_ManHashXor(pNew, pArg0[k], pArg1[k]) ); 
            Vec_IntFill( vRes, 1, Abc_LitNotCond(iLit, pObj->Type == WLC_OBJ_COMP_EQU) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
        }
        else if ( pObj->Type == WLC_OBJ_COMP_LESS || pObj->Type == WLC_OBJ_COMP_MOREEQU ||
                  pObj->Type == WLC_OBJ_COMP_MORE || pObj->Type == WLC_OBJ_COMP_LESSEQU )
        {
            int nRangeMax = Abc_MaxInt( nRange0, nRange1 );
            int fSigned = Wlc_ObjIsSignedFanin01(p, pObj);
            int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRangeMax, fSigned );
            int * pArg1 = Wlc_VecLoadFanins( vTemp1, pFans1, nRange1, nRangeMax, fSigned );
            int fSwap  = (pObj->Type == WLC_OBJ_COMP_MORE    || pObj->Type == WLC_OBJ_COMP_LESSEQU);
            int fCompl = (pObj->Type == WLC_OBJ_COMP_MOREEQU || pObj->Type == WLC_OBJ_COMP_LESSEQU);
            if ( fSwap ) ABC_SWAP( int *, pArg0, pArg1 );
            if ( fSigned )
                iLit = Wlc_BlastLessSigned( pNew, pArg0, pArg1, nRangeMax );
            else
                iLit = Wlc_BlastLess( pNew, pArg0, pArg1, nRangeMax );
            iLit = Abc_LitNotCond( iLit, fCompl );
            Vec_IntFill( vRes, 1, iLit );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
        }
        else if ( pObj->Type == WLC_OBJ_REDUCT_AND  || pObj->Type == WLC_OBJ_REDUCT_OR  || pObj->Type == WLC_OBJ_REDUCT_XOR ||
                  pObj->Type == WLC_OBJ_REDUCT_NAND || pObj->Type == WLC_OBJ_REDUCT_NOR || pObj->Type == WLC_OBJ_REDUCT_NXOR )
        {
            Vec_IntPush( vRes, Wlc_BlastReduction( pNew, pFans0, nRange0, pObj->Type ) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
        }
        else if ( pObj->Type == WLC_OBJ_ARI_ADD || pObj->Type == WLC_OBJ_ARI_SUB ) 
        {
            int nRangeMax = Abc_MaxInt( nRange, Abc_MaxInt(nRange0, nRange1) );
            int * pArg0 = Wlc_VecLoadFanins( vRes,   pFans0, nRange0, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
            int * pArg1 = Wlc_VecLoadFanins( vTemp1, pFans1, nRange1, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
            int CarryIn = Wlc_ObjFaninNum(pObj) == 3 ? pFans2[0] : 0;
            if ( pObj->Type == WLC_OBJ_ARI_ADD )
            {
                if ( pPar->fCla )
                    Wlc_BlastAdderCLA( pNew, pArg0, pArg1, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj), CarryIn ); // result is in pFan0 (vRes)
                    //Wlc_BlastAdderFast( pNew, pArg0, pArg1, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj), CarryIn ); // result is in pFan0 (vRes)
                else
                    Wlc_BlastAdder( pNew, pArg0, pArg1, nRangeMax, CarryIn ); // result is in pFan0 (vRes)
            }
            else 
                Wlc_BlastSubtract( pNew, pArg0, pArg1, nRange, 1 ); // result is in pFan0 (vRes)
            Vec_IntShrink( vRes, nRange );
        }
        else if ( pObj->Type == WLC_OBJ_ARI_ADDSUB ) 
        {
            int nRangeMax = Abc_MaxInt( nRange, Abc_MaxInt(nRange2, nRange3) );
            int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans2, nRange2, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
            int * pArg1 = Wlc_VecLoadFanins( vTemp1, pFans2, nRange2, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
            int * pArg2 = Wlc_VecLoadFanins( vTemp2, pFans3, nRange3, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
            int ModeIn   = pFans0[0];
            int CarryIn  = pFans1[0]; int j;
            Wlc_BlastAdder   ( pNew, pArg0, pArg2, nRangeMax, CarryIn ); // result is in pArg0 (vTemp0)
            Wlc_BlastSubtract( pNew, pArg1, pArg2, nRangeMax, Abc_LitNot(CarryIn) ); // result is in pArg1 (vTemp1)
            Vec_IntClear( vRes );
            for ( j = 0; j < nRange; j++ )
                Vec_IntPush( vRes, Gia_ManHashMux(pNew, ModeIn, pArg0[j], pArg1[j]) ); 
        }
        else if ( pObj->Type == WLC_OBJ_ARI_MULTI )
        {
            if ( fUseOldMultiplierBlasting )
            {
                int nRangeMax = Abc_MaxInt( nRange, Abc_MaxInt(nRange0, nRange1) );
                int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
                int * pArg1 = Wlc_VecLoadFanins( vTemp1, pFans1, nRange1, nRangeMax, Wlc_ObjIsSignedFanin01(p, pObj) );
                Wlc_BlastMultiplier2( pNew, pArg0, pArg1, nRange, vTemp2, vRes );
                Vec_IntShrink( vRes, nRange );
            }
            else
            {
                int fSigned = Wlc_ObjIsSignedFanin01(p, pObj);
                int nRangeMax = Abc_MaxInt(nRange0, nRange1);
                int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRangeMax, fSigned );
                int * pArg1 = Wlc_VecLoadFanins( vTemp1, pFans1, nRange1, nRangeMax, fSigned );
                if ( Wlc_NtkCountConstBits(pArg0, nRangeMax) < Wlc_NtkCountConstBits(pArg1, nRangeMax) )
                    ABC_SWAP( int *, pArg0, pArg1 );
                if ( pPar->fBooth )
                    Wlc_BlastBooth( pNew, pArg0, pArg1, nRange0, nRange1, vRes, fSigned, pPar->fCla, NULL );
                else if ( pPar->fCla )
                    Wlc_BlastMultiplier3( pNew, pArg0, pArg1, nRange0, nRange1, vRes, Wlc_ObjIsSignedFanin01(p, pObj), pPar->fCla, NULL );
                else
                    Wlc_BlastMultiplier( pNew, pArg0, pArg1, nRangeMax, nRangeMax, vTemp2, vRes, fSigned );
                    //Wlc_BlastMultiplierC( pNew, pArg0, pArg1, nRangeMax, nRangeMax, vTemp2, vRes, fSigned );
                if ( nRange > Vec_IntSize(vRes) )
                    Vec_IntFillExtra( vRes, nRange, fSigned ? Vec_IntEntryLast(vRes) : 0 );
                else
                    Vec_IntShrink( vRes, nRange );
                assert( Vec_IntSize(vRes) == nRange );
            }
        }
        else if ( pObj->Type == WLC_OBJ_ARI_DIVIDE || pObj->Type == WLC_OBJ_ARI_REM || pObj->Type == WLC_OBJ_ARI_MODULUS )
        {
            int nRangeMax = Abc_MaxInt( nRange, Abc_MaxInt(nRange0, nRange1) );
            int fSigned = Wlc_ObjIsSignedFanin01(p, pObj);
            int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRangeMax, fSigned );
            int * pArg1 = Wlc_VecLoadFanins( vTemp1, pFans1, nRange1, nRangeMax, fSigned );
            if ( fSigned )
                Wlc_BlastDividerSigned( pNew, pArg0, nRangeMax, pArg1, nRangeMax, pObj->Type == WLC_OBJ_ARI_DIVIDE, vRes, pPar->fNonRest );
            else
                Wlc_BlastDividerTop( pNew, pArg0, nRangeMax, pArg1, nRangeMax, pObj->Type == WLC_OBJ_ARI_DIVIDE, vRes, pPar->fNonRest );
            Vec_IntShrink( vRes, nRange );
            if ( !pPar->fDivBy0 )
                Wlc_BlastZeroCondition( pNew, pFans1, nRange1, vRes );
        }
        else if ( pObj->Type == WLC_OBJ_ARI_MINUS )
        {
            int nRangeMax = Abc_MaxInt( nRange0, nRange );
            int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRangeMax, Wlc_ObjIsSignedFanin0(p, pObj) );
            Wlc_BlastMinus( pNew, pArg0, nRangeMax, vRes );
            Vec_IntShrink( vRes, nRange );
        }
        else if ( pObj->Type == WLC_OBJ_ARI_POWER )
        {
            int nRangeMax = Abc_MaxInt(nRange0, nRange);
            int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRangeMax, Wlc_ObjIsSignedFanin0(p, pObj) );
            int * pArg1 = Wlc_VecLoadFanins( vTemp1, pFans1, nRange1, nRange1, Wlc_ObjIsSignedFanin1(p, pObj) );
            Wlc_BlastPower( pNew, pArg0, nRangeMax, pArg1, nRange1, vTemp2, vRes );
            Vec_IntShrink( vRes, nRange );
        }
        else if ( pObj->Type == WLC_OBJ_ARI_SQRT )
        {
            int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRange0 + (nRange0 & 1), 0 );
            nRange0 += (nRange0 & 1);
            if ( pPar->fNonRest )
                Wlc_BlastSqrtNR( pNew, pArg0, nRange0, vTemp2, vRes );
            else
                Wlc_BlastSqrt( pNew, pArg0, nRange0, vTemp2, vRes );
            if ( nRange > Vec_IntSize(vRes) )
                Vec_IntFillExtra( vRes, nRange, 0 );
            else
                Vec_IntShrink( vRes, nRange );
        }
        else if ( pObj->Type == WLC_OBJ_ARI_SQUARE )
        {
            int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRange0, 0 );
            Wlc_BlastSquare( pNew, pArg0, nRange0, vTemp2, vRes );
            if ( nRange > Vec_IntSize(vRes) )
                Vec_IntFillExtra( vRes, nRange, 0 );
            else
                Vec_IntShrink( vRes, nRange );
        }
        else if ( pObj->Type == WLC_OBJ_DEC )
        {
            int * pArg0 = Wlc_VecLoadFanins( vTemp0, pFans0, nRange0, nRange0, 0 );
            Wlc_BlastDecoder( pNew, pArg0, nRange0, vTemp2, vRes );
            if ( nRange > Vec_IntSize(vRes) )
                Vec_IntFillExtra( vRes, nRange, 0 );
            else
                Vec_IntShrink( vRes, nRange );
        }
        else if ( pObj->Type == WLC_OBJ_TABLE )
            Wlc_BlastTable( pNew, Wlc_ObjTable(p, pObj), pFans0, nRange0, nRange, vRes );
        else if ( pObj->Type == WLC_OBJ_LUT && p->vLutTruths )
            Wlc_BlastLut( pNew, Vec_WrdEntry(p->vLutTruths, Wlc_ObjId(p, pObj)), pFans0, nRange0, nRange, vRes );
        else assert( 0 );
        assert( Vec_IntSize(vBits) == Wlc_ObjCopy(p, i) );
        Vec_IntAppend( vBits, vRes );
        if ( vAddOutputs && !Wlc_ObjIsCo(pObj) && 
            (
             (pObj->Type >= WLC_OBJ_MUX      && pObj->Type <= WLC_OBJ_ROTATE_L)     || 
             (pObj->Type >= WLC_OBJ_COMP_EQU && pObj->Type <= WLC_OBJ_COMP_MOREEQU) || 
             (pObj->Type >= WLC_OBJ_ARI_ADD  && pObj->Type <= WLC_OBJ_ARI_SQUARE)
            )
           )
        {
            Vec_IntAppend( vAddOutputs, vRes );
            Vec_IntPush( vAddObjs, Wlc_ObjId(p, pObj) );
        }
        p->nAnds[pObj->Type] += Gia_ManAndNum(pNew) - nAndPrev;
    }
    p->nAnds[0] = Gia_ManAndNum(pNew);
    assert( nBits == Vec_IntSize(vBits) );
    Vec_IntFree( vTemp0 );
    Vec_IntFree( vTemp1 );
    Vec_IntFree( vTemp2 );
    Vec_IntFree( vRes );
    // create flop boxes
    Wlc_NtkForEachFf2( p, pObj, i )
    {
        If_Box_t * pBox;
        char Buffer[100];
        float * pTable;
        Vec_Int_t * vTemp0 = Vec_IntAlloc( 100 );
        Vec_Int_t * vTemp1 = Vec_IntAlloc( 100 );
        int iLit, nRange = Wlc_ObjRange(pObj);
        int * pFans0, * pFans1, * pFans2, * pFans3;
        int iReset, iSet, iEnable;
        int nRangeIn = 2*nRange + 3; // D, reset, set, enable, Q
        int iSre = Wlc_ObjFaninId(pObj, 6);

        assert( pObj->Type == WLC_OBJ_FF );

        // create new box
        if ( vTables == NULL )
            Tim_ManSetDelayTables( pManTime, (vTables = Vec_PtrAlloc(100)) );
        Tim_ManCreateBox( pManTime, curPo, nRangeIn, curPi, nRange, Vec_PtrSize(vTables), 0 );
        curPi += nRange;
        curPo += nRangeIn;

        // create delay table
        pTable = ABC_ALLOC( float, 3 + nRange * nRangeIn );
        pTable[0] = Vec_PtrSize(vTables);
        pTable[1] = nRangeIn;
        pTable[2] = nRange;
        for ( k = 0; k < nRange * nRangeIn; k++ )
            pTable[3 + k] = 1.0;
        Vec_PtrPush( vTables, pTable );

        // create combinational outputs in the normal manager
        pFans0  = Wlc_ObjFaninNum(pObj) > 0 ? Vec_IntEntryP( vBits, Wlc_ObjCopy(p, Wlc_ObjFaninId0(pObj)) ) : NULL;
        pFans1  = Wlc_ObjFaninNum(pObj) > 2 ? Vec_IntEntryP( vBits, Wlc_ObjCopy(p, Wlc_ObjFaninId(pObj,2)) ) : NULL; // reset
        pFans2  = Wlc_ObjFaninNum(pObj) > 3 ? Vec_IntEntryP( vBits, Wlc_ObjCopy(p, Wlc_ObjFaninId(pObj,3)) ) : NULL; // set
        pFans3  = Wlc_ObjFaninNum(pObj) > 4 ? Vec_IntEntryP( vBits, Wlc_ObjCopy(p, Wlc_ObjFaninId(pObj,4)) ) : NULL; // enable
        for ( k = 0; k < nRange; k++ )
            Gia_ManAppendCo( pNew, pFans0[k] );
        Gia_ManAppendCo( pNew, pFans1[0] );
        Gia_ManAppendCo( pNew, pFans2[0] );
        Gia_ManAppendCo( pNew, pFans3[0] );
        for ( k = 0; k < nRange; k++ )
            Gia_ManAppendCo( pNew, Gia_Obj2Lit(pNew, Gia_ManCi(pNew, Vec_IntEntry(vFf2Ci, i)+k)) );

        // make sure there is enough primary inputs in the manager
        for ( k = Gia_ManPiNum(pExtra); k < nRangeIn; k++ )
            Gia_ManAppendCi( pExtra );
        // create combinational inputs
        for ( k = 0; k < nRange; k++ )
            Vec_IntPush( vTemp0, Gia_Obj2Lit(pExtra, Gia_ManPi(pExtra, k)) );
        iReset  = Gia_Obj2Lit(pExtra, Gia_ManPi(pExtra, nRange+0));
        iSet    = Gia_Obj2Lit(pExtra, Gia_ManPi(pExtra, nRange+1));
        iEnable = Gia_Obj2Lit(pExtra, Gia_ManPi(pExtra, nRange+2));
        for ( k = 0; k < nRange; k++ )
            Vec_IntPush( vTemp1, Gia_Obj2Lit(pExtra, Gia_ManPi(pExtra, nRangeIn-nRange+k)) );

        // bit-blast in the external manager
        for ( k = 0; k < nRange; k++ )
        {
            // enable
            //iLitFlop = Gia_LitNotCond( Gia_ObjCopy(pObj), Gia_FlopIsOne(pObj) );
            //iLit = Gia_ManHashMux( Gia_ObjGia(pObj), Gia_FlopEnableCopy(pObj), iLit, iLitFlop );
            iLit = Gia_ManHashMux( pExtra, iEnable, Vec_IntEntry(vTemp0, k), Vec_IntEntry(vTemp1, k) );
            if ( iSre )
            {
                // reset
                //iLit = Gia_ManHashAnd( Gia_ObjGia(pObj), iLit, Gia_LitNot(Gia_FlopResetCopy(pObj)) );
                iLit = Gia_ManHashAnd( pExtra, iLit, Abc_LitNot(iReset) );
                // set
                //iLit = Gia_ManHashOr( Gia_ObjGia(pObj), iLit, Gia_FlopSetCopy(pObj) );
                iLit = Gia_ManHashOr( pExtra, iLit, iSet );
            }
            else
            {
                // set
                //iLit = Gia_ManHashOr( Gia_ObjGia(pObj), iLit, Gia_FlopSetCopy(pObj) );
                iLit = Gia_ManHashOr( pExtra, iLit, iSet );
                // reset
                //iLit = Gia_ManHashAnd( Gia_ObjGia(pObj), iLit, Gia_LitNot(Gia_FlopResetCopy(pObj)) );
                iLit = Gia_ManHashAnd( pExtra, iLit, Abc_LitNot(iReset) );
            }
            // create outputs in the external manager
            Gia_ManAppendCo( pExtra, iLit );
        }

        // add box to the library
        sprintf( Buffer, "%s%03d", "ff_comb", 1+If_LibBoxNum(pBoxLib) );
        pBox = If_BoxStart( Abc_UtilStrsav(Buffer), 1+If_LibBoxNum(pBoxLib), nRangeIn, nRange, 0, 0, 0 ); 
        If_LibBoxAdd( pBoxLib, pBox );
        for ( k = 0; k < pBox->nPis * pBox->nPos; k++ )
            pBox->pDelays[k] = 1;
        Vec_IntFree( vTemp0 );
        Vec_IntFree( vTemp1 );
    }
    Vec_IntFree( vFf2Ci );
    // create COs
    if ( pPar->fCreateMiter || pPar->fCreateWordMiter )
    {
        int nPairs = 0, nBits = 0;
        Vec_Int_t * vOuts = Vec_IntAlloc( 100 );
        assert( Wlc_NtkPoNum(p) % 2 == 0 );
        Wlc_NtkForEachCo( p, pObj, i )
        {
            if ( pObj->fIsFi )
            {
                nRange = Wlc_ObjRange( pObj );
                pFans0 = Vec_IntEntryP( vBits, Wlc_ObjCopy(p, Wlc_ObjId(p, pObj)) );
                if ( Wlc_ObjRangeIsReversed(pObj) )
                {
                    for ( k = 0; k < nRange; k++ )
                        Gia_ManAppendCo( pNew, pFans0[nRange-1-k] );
                }
                else
                {
                    for ( k = 0; k < nRange; k++ )
                        Gia_ManAppendCo( pNew, pFans0[k] );
                }
                nFFins += nRange;
                continue;
            }
            pObj2   = Wlc_NtkCo( p, ++i );
            nRange1 = Wlc_ObjRange( pObj );
            nRange2 = Wlc_ObjRange( pObj2 );
            assert( nRange1 == nRange2 );
            pFans1  = Vec_IntEntryP( vBits, Wlc_ObjCopy(p, Wlc_ObjId(p, pObj)) );
            pFans2  = Vec_IntEntryP( vBits, Wlc_ObjCopy(p, Wlc_ObjId(p, pObj2)) );
            if ( pPar->fCreateWordMiter )
            {
                Vec_IntClear( vOuts );
                if ( Wlc_ObjRangeIsReversed(pObj) )
                {
                    for ( k = 0; k < nRange1; k++ )
                        Vec_IntPushTwo( vOuts, pFans1[nRange1-1-k], pFans2[nRange2-1-k] );
                }
                else
                {
                    for ( k = 0; k < nRange1; k++ )
                        Vec_IntPushTwo( vOuts, pFans1[k], pFans2[k] );
                }
                Gia_ManAppendCo( pNew, Gia_ManHashDualMiter(pNew, vOuts) );
            }
            else
            {
                if ( Wlc_ObjRangeIsReversed(pObj) )
                {
                    for ( k = 0; k < nRange1; k++ )
                    {
                        Gia_ManAppendCo( pNew, pFans1[nRange1-1-k] );
                        Gia_ManAppendCo( pNew, pFans2[nRange2-1-k] );
                    }
                }
                else
                {
                    for ( k = 0; k < nRange1; k++ )
                    {
                        Gia_ManAppendCo( pNew, pFans1[k] );
                        Gia_ManAppendCo( pNew, pFans2[k] );
                    }
                }
            }
            nPairs++;
            nBits += nRange1;
        }
        Vec_IntFree( vOuts );
        if ( pPar->fCreateWordMiter )
            printf( "Derived an ordinary miter with %d bit-level outputs, one for each pair of word-level outputs.\n", nPairs );
        else
            printf( "Derived a dual-output miter with %d pairs of bits belonging to %d pairs of word-level outputs.\n", nBits, nPairs );
    }
    else
    {
        Wlc_NtkForEachCo( p, pObj, i )
        {
            // skip all outputs except the given ones
            if ( pPar->iOutput >= 0 && (i < pPar->iOutput || i >= pPar->iOutput + pPar->nOutputRange) )
                continue;
            // create additional PO literals
            if ( vAddOutputs && pObj->fIsFi )
            {
                Vec_IntForEachEntry( vAddOutputs, iLit, k )
                    Gia_ManAppendCo( pNew, iLit );
                printf( "Created %d additional POs for %d interesting internal word-level variables.\n", Vec_IntSize(vAddOutputs), Vec_IntSize(vAddObjs) );
                Vec_IntFreeP( &vAddOutputs );
            }
            nRange = Wlc_ObjRange( pObj );
            pFans0 = Vec_IntEntryP( vBits, Wlc_ObjCopy(p, Wlc_ObjId(p, pObj)) );
            if ( fVerbose )
                printf( "%s(%d) ", Wlc_ObjName(p, Wlc_ObjId(p, pObj)), Gia_ManCoNum(pNew) );
            if ( Wlc_ObjRangeIsReversed(pObj) )
            {
                for ( k = 0; k < nRange; k++ )
                    Gia_ManAppendCo( pNew, pFans0[nRange-1-k] );
            }
            else
            {
                for ( k = 0; k < nRange; k++ )
                    Gia_ManAppendCo( pNew, pFans0[k] );
            }
            if ( pObj->fIsFi )
                nFFins += nRange;
        }
        if ( fVerbose )
            printf( "\n" );
    }
    //Vec_IntErase( vBits );
    //Vec_IntErase( &p->vCopies );
    // set the number of registers
    if ( Vec_IntSize(&p->vFfs2) > 0 )
    {
        assert( nFFins == 0 && nFFouts == 0 );
        // complement flop inputs whose init state is 1
        for ( i = 0; i < nFf2Regs; i++ )
            Gia_ManAppendCo( pNew, Gia_ManAppendCi(pNew) );
        Gia_ManSetRegNum( pNew, nFf2Regs );
    }
    else
    {
        assert( nFFins == nFFouts );
        Gia_ManSetRegNum( pNew, nFFins );
    }
    // finalize AIG
    if ( !pPar->fGiaSimple && !pPar->fNoCleanup )
    {
        pNew = Gia_ManCleanup( pTemp = pNew );
        Gia_ManDupRemapLiterals( vBits, pTemp );
        //printf( "Cutoff ID %d became %d.\n", 75, Abc_Lit2Var(Gia_ManObj(pTemp, 73)->Value) );
        Gia_ManStop( pTemp );
    }
    // transform AIG with init state
    if ( p->pInits )
    {
        if ( (int)strlen(p->pInits) != Gia_ManRegNum(pNew) )
        {
            printf( "The number of init values (%d) does not match the number of flops (%d).\n", (int)strlen(p->pInits), Gia_ManRegNum(pNew) );
            printf( "It is assumed that the AIG has constant 0 initial state.\n" );
        }
        else
        {
            pNew = Gia_ManDupZeroUndc( pTemp = pNew, p->pInits, pPar->fSaveFfNames ? 1+Gia_ManRegNum(pNew) : 0, pPar->fGiaSimple, pPar->fVerbose );
            Gia_ManDupRemapLiterals( vBits, pTemp );
            Gia_ManStop( pTemp );
        }
    }
    // finalize AIG with boxes
    if ( Vec_IntSize(&p->vFfs2) > 0 )
    {
        curPo += nBitCos + nFf2Regs;
        assert( curPi == Tim_ManCiNum(pManTime) );
        assert( curPo == Tim_ManCoNum(pManTime) );
        // finalize the extra AIG
        pExtra = Gia_ManCleanup( pTemp = pExtra );
        Gia_ManStop( pTemp );
        assert( Gia_ManPoNum(pExtra) == Gia_ManCiNum(pNew) - nBitCis - nFf2Regs );
        // attach
        pNew->pAigExtra = pExtra;
        pNew->pManTime = pManTime;
        // normalize AIG
        pNew = Gia_ManDupNormalize( pTemp = pNew, 0 );
        Gia_ManTransferTiming( pNew, pTemp );
        Gia_ManStop( pTemp );
        //Tim_ManPrint( pManTime );
    }
    if ( pPar->vBoxIds )
    {
        curPo += nBitCos;
        assert( curPi == Tim_ManCiNum(pManTime) );
        assert( curPo == Tim_ManCoNum(pManTime) );
        // finalize the extra AIG
        pExtra = Gia_ManCleanup( pTemp = pExtra );
        Gia_ManStop( pTemp );
        assert( Gia_ManPoNum(pExtra) == Gia_ManCiNum(pNew) - nBitCis - nFf2Regs );
        // attach
        pNew->pAigExtra = pExtra;
        pNew->pManTime = pManTime;
        // normalize AIG
        pNew = Gia_ManDupNormalize( pTemp = pNew, 0 );
        Gia_ManTransferTiming( pNew, pTemp );
        Gia_ManStop( pTemp );
        //Tim_ManPrint( pManTime );
    }
    // create input names
    pNew->vNamesIn = Vec_PtrAlloc( Gia_ManCiNum(pNew) );
    Wlc_NtkForEachCi( p, pObj, i )
    if ( Wlc_ObjIsPi(pObj) )
    {
        char * pName = Wlc_ObjName(p, Wlc_ObjId(p, pObj));
        nRange = Wlc_ObjRange( pObj );
        if ( fSkipBitRange && nRange == 1 )
            Vec_PtrPush( pNew->vNamesIn, Abc_UtilStrsav(pName) );
        else
            for ( k = 0; k < nRange; k++ )
            {
                char Buffer[1000];
                sprintf( Buffer, "%s[%d]", pName, pObj->Beg < pObj->End ? pObj->Beg+k : pObj->Beg-k );
                Vec_PtrPush( pNew->vNamesIn, Abc_UtilStrsav(Buffer) );
                //printf( "Writing %s\n", Buffer );
            }
    }
    if ( p->pInits )
    {
        int Length = strlen(p->pInits);
        for ( i = 0; i < Length; i++ )
        if ( p->pInits[i] == 'x' || p->pInits[i] == 'X' )
        {
            char Buffer[100];
            sprintf( Buffer, "_%s_abc_%d_", "init", i );
            Vec_PtrPush( pNew->vNamesIn, Abc_UtilStrsav(Buffer) );
            fAdded = 1;
        }
        if ( pPar->fSaveFfNames )
            for ( i = 0; i < 1+Length; i++ )
                Vec_PtrPush( pNew->vNamesIn, NULL );
    }
    Wlc_NtkForEachCi( p, pObj, i )
    if ( !Wlc_ObjIsPi(pObj) )
    {
        char * pName = Wlc_ObjName(p, Wlc_ObjId(p, pObj));
        nRange = Wlc_ObjRange( pObj );
        if ( fSkipBitRange && nRange == 1 )
            Vec_PtrPush( pNew->vNamesIn, Abc_UtilStrsav(pName) );
        else
            for ( k = 0; k < nRange; k++ )
            {
                char Buffer[1000];
                sprintf( Buffer, "%s[%d]", pName, pObj->Beg < pObj->End ? pObj->Beg+k : pObj->Beg-k );
                Vec_PtrPush( pNew->vNamesIn, Abc_UtilStrsav(Buffer) );
            }
    }
    Wlc_NtkForEachFf2( p, pObj, i )
    {
        char * pName = Wlc_ObjName(p, Wlc_ObjId(p, pObj));
        nRange = Wlc_ObjRange( pObj );
        if ( fSkipBitRange && nRange == 1 )
        {
            char Buffer[1000];
            sprintf( Buffer, "%s_fo", pName );
            Vec_PtrPush( pNew->vNamesIn, Abc_UtilStrsav(Buffer) );
        }
        else
            for ( k = 0; k < nRange; k++ )
            {
                char Buffer[1000];
                sprintf( Buffer, "%s_fo[%d]", pName, pObj->Beg < pObj->End ? pObj->Beg+k : pObj->Beg-k );
                Vec_PtrPush( pNew->vNamesIn, Abc_UtilStrsav(Buffer) );
            }
    }
    Wlc_NtkForEachFf2( p, pObj, i )
    {
        char * pName = Wlc_ObjName(p, Wlc_ObjId(p, pObj));
        nRange = Wlc_ObjRange( pObj );
        if ( fSkipBitRange && nRange == 1 )
            Vec_PtrPush( pNew->vNamesIn, Abc_UtilStrsav(pName) );
        else
            for ( k = 0; k < nRange; k++ )
            {
                char Buffer[1000];
                sprintf( Buffer, "%s[%d]", pName, pObj->Beg < pObj->End ? pObj->Beg+k : pObj->Beg-k );
                Vec_PtrPush( pNew->vNamesIn, Abc_UtilStrsav(Buffer) );
            }
    }
    if ( p->pInits && pPar->fSaveFfNames )
    {
        char * pName;
        int Length    = (int)strlen(p->pInits);
        int NameStart = Vec_PtrSize(pNew->vNamesIn)-Length;
        int NullStart = Vec_PtrSize(pNew->vNamesIn)-2*Length;
        int SepStart  = Vec_PtrSize(pNew->vNamesIn)-2*Length-1;
        assert( Vec_PtrEntry(pNew->vNamesIn, SepStart) == NULL );
        Vec_PtrWriteEntry( pNew->vNamesIn, SepStart, Abc_UtilStrsav("_abc_190121_abc_") );
        for ( i = 0; i < Length; i++ )
        {
            char Buffer[100];
            sprintf( Buffer, "%c%s", p->pInits[i], (char *)Vec_PtrEntry(pNew->vNamesIn, NameStart+i) );
            assert( Vec_PtrEntry(pNew->vNamesIn, NullStart+i) == NULL );
            Vec_PtrWriteEntry( pNew->vNamesIn, NullStart+i, Abc_UtilStrsav(Buffer) );
        }
        Vec_PtrForEachEntry( char *, pNew->vNamesIn, pName, i )
            assert( pName != NULL );
    }
    if ( p->pInits && fAdded )
        Vec_PtrPush( pNew->vNamesIn, Abc_UtilStrsav("abc_reset_flop") );
    if ( pPar->vBoxIds )
    {
        Wlc_NtkForEachObjVec( pPar->vBoxIds, p, pObj, i )
        {
            char * pName = Wlc_ObjName(p, Wlc_ObjId(p, pObj));
            nRange = Wlc_ObjRange( pObj );
            assert( nRange > 1 );
            for ( k = 0; k < nRange; k++ )
            {
                char Buffer[1000];
                sprintf( Buffer, "%s[%d]", pName, pObj->Beg < pObj->End ? pObj->Beg+k : pObj->Beg-k );
                Vec_PtrPush( pNew->vNamesIn, Abc_UtilStrsav(Buffer) );
            }
        }
    }
    assert( Vec_PtrSize(pNew->vNamesIn) == Gia_ManCiNum(pNew) );
    // create output names
    pNew->vNamesOut = Vec_PtrAlloc( Gia_ManCoNum(pNew) );
    Wlc_NtkForEachFf2( p, pObj, i )
    {
        int iFanin;
        Wlc_ObjForEachFanin( pObj, iFanin, b )
        {
            char * pName = Wlc_ObjName(p, iFanin);
            nRange = Wlc_ObjRange( Wlc_NtkObj(p, iFanin) );
            if ( fSkipBitRange && nRange == 1 )
                Vec_PtrPush( pNew->vNamesOut, Abc_UtilStrsav(pName) );
            else
                for ( k = 0; k < nRange; k++ )
                {
                    char Buffer[1000];
                    Wlc_Obj_t * pFanin = Wlc_NtkObj(p, iFanin);
                    sprintf( Buffer, "%s[%d]", pName, pFanin->Beg < pFanin->End ? pFanin->Beg+k : pFanin->Beg-k );
                    Vec_PtrPush( pNew->vNamesOut, Abc_UtilStrsav(Buffer) );
                }
            if ( b == 3 )
                break;
        }
        {
            char * pName = Wlc_ObjName(p, Wlc_ObjId(p, pObj));
            nRange = Wlc_ObjRange( pObj );
            if ( fSkipBitRange && nRange == 1 )
            {
                char Buffer[1000];
                sprintf( Buffer, "%s_in", pName );
                Vec_PtrPush( pNew->vNamesOut, Abc_UtilStrsav(Buffer) );
            }
            else
                for ( k = 0; k < nRange; k++ )
                {
                    char Buffer[1000];
                    sprintf( Buffer, "%s_in[%d]", pName, pObj->Beg < pObj->End ? pObj->Beg+k : pObj->Beg-k );
                    Vec_PtrPush( pNew->vNamesOut, Abc_UtilStrsav(Buffer) );
                }
        }
    }
    if ( pPar->vBoxIds )
    {
        Wlc_NtkForEachObjVec( pPar->vBoxIds, p, pObj, i )
        {
            int iFanin, f;
            Wlc_ObjForEachFanin( pObj, iFanin, f )
            {
                char * pName = Wlc_ObjName(p, iFanin);
                nRange = Wlc_ObjRange( Wlc_NtkObj(p, iFanin) );
                assert( nRange >= 1 );
                for ( k = 0; k < nRange; k++ )
                {
                    char Buffer[1000];
                    Wlc_Obj_t * pFanin = Wlc_NtkObj(p, iFanin);
                    sprintf( Buffer, "%s[%d]", pName, pFanin->Beg < pFanin->End ? pFanin->Beg+k : pFanin->Beg-k );
                    Vec_PtrPush( pNew->vNamesOut, Abc_UtilStrsav(Buffer) );
                }
            }
        }
    }
    // add real primary outputs
    Wlc_NtkForEachCo( p, pObj, i )
    if ( Wlc_ObjIsPo(pObj) )
    {
        char * pName = Wlc_ObjName(p, Wlc_ObjId(p, pObj));
        nRange = Wlc_ObjRange( pObj );
        if ( pPar->fCreateWordMiter )
        {
            Wlc_Obj_t * pObj2 = Wlc_NtkCo( p, ++i );
            char * pName2 = Wlc_ObjName(p, Wlc_ObjId(p, pObj2));
            char Buffer[1000];
            sprintf( Buffer, "%s_xor_%s", pName, pName2 );
            Vec_PtrPush( pNew->vNamesOut, Abc_UtilStrsav(Buffer) );
            //printf( "Adding output %s\n", Buffer );
        }
        else if ( pPar->fCreateMiter && nRange > 1 )
        {
            Wlc_Obj_t * pObj2 = Wlc_NtkCo( p, ++i );
            char * pName2 = Wlc_ObjName(p, Wlc_ObjId(p, pObj2));
            int nRange1 = Wlc_ObjRange( pObj );
            assert( nRange == nRange1 );
            for ( k = 0; k < nRange; k++ )
            {
                char Buffer[1000];
                sprintf( Buffer, "%s[%d]", pName, pObj->Beg < pObj->End ? pObj->Beg+k : pObj->Beg-k );
                Vec_PtrPush( pNew->vNamesOut, Abc_UtilStrsav(Buffer) );
                sprintf( Buffer, "%s[%d]", pName2, pObj->Beg < pObj->End ? pObj->Beg+k : pObj->Beg-k );
                Vec_PtrPush( pNew->vNamesOut, Abc_UtilStrsav(Buffer) );
            }
        }
        else 
        {
            if ( fSkipBitRange && nRange == 1 )
                Vec_PtrPush( pNew->vNamesOut, Abc_UtilStrsav(pName) );
            else
                for ( k = 0; k < nRange; k++ )
                {
                    char Buffer[1000];
                    sprintf( Buffer, "%s[%d]", pName, pObj->Beg < pObj->End ? pObj->Beg+k : pObj->Beg-k );
                    Vec_PtrPush( pNew->vNamesOut, Abc_UtilStrsav(Buffer) );
                }
        }
    }
    if ( vAddObjs )
    {
        // add internal primary outputs
        Wlc_NtkForEachObjVec( vAddObjs, p, pObj, i )
        {
            char * pName = Wlc_ObjName(p, Wlc_ObjId(p, pObj));
            nRange = Wlc_ObjRange( pObj );
            if ( fSkipBitRange && nRange == 1 )
                Vec_PtrPush( pNew->vNamesOut, Abc_UtilStrsav(pName) );
            else
                for ( k = 0; k < nRange; k++ )
                {
                    char Buffer[1000];
                    sprintf( Buffer, "%s[%d]", pName, pObj->Beg < pObj->End ? pObj->Beg+k : pObj->Beg-k );
                    Vec_PtrPush( pNew->vNamesOut, Abc_UtilStrsav(Buffer) );
                }
        }
        Vec_IntFreeP( &vAddObjs );
    }
    // add flop outputs
    if ( fAdded )
        Vec_PtrPush( pNew->vNamesOut, Abc_UtilStrsav("abc_reset_flop_in") );
    Wlc_NtkForEachCo( p, pObj, i )
    if ( !Wlc_ObjIsPo(pObj) )
    {
        char * pName = Wlc_ObjName(p, Wlc_ObjId(p, pObj));
        nRange = Wlc_ObjRange( pObj );
        if ( fSkipBitRange && nRange == 1 )
        {
            char Buffer[1000];
            sprintf( Buffer, "%s_fi", pName );
            Vec_PtrPush( pNew->vNamesOut, Abc_UtilStrsav(Buffer) );
        }
        else
            for ( k = 0; k < nRange; k++ )
            {
                char Buffer[1000];
                sprintf( Buffer, "%s_fi[%d]", pName, pObj->Beg < pObj->End ? pObj->Beg+k : pObj->Beg-k );
                Vec_PtrPush( pNew->vNamesOut, Abc_UtilStrsav(Buffer) );
            }
    }
    Wlc_NtkForEachFf2( p, pObj, i )
    {
        char * pName = Wlc_ObjName(p, Wlc_ObjId(p, pObj));
        nRange = Wlc_ObjRange( pObj );
        if ( fSkipBitRange && nRange == 1 )
        {
            char Buffer[1000];
            sprintf( Buffer, "%s_fi", pName );
            Vec_PtrPush( pNew->vNamesOut, Abc_UtilStrsav(Buffer) );
        }
        else
            for ( k = 0; k < nRange; k++ )
            {
                char Buffer[1000];
                sprintf( Buffer, "%s_fi[%d]", pName, pObj->Beg < pObj->End ? pObj->Beg+k : pObj->Beg-k );
                Vec_PtrPush( pNew->vNamesOut, Abc_UtilStrsav(Buffer) );
            }
    }
    assert( Vec_PtrSize(pNew->vNamesOut) == Gia_ManCoNum(pNew) );

    // replace the current library
    if ( pBoxLib )
    {
        If_LibBoxFree( (If_LibBox_t *)Abc_FrameReadLibBox() );
        Abc_FrameSetLibBox( pBoxLib );
    }

    //pNew->pSpec = Abc_UtilStrsav( p->pSpec ? p->pSpec : p->pName );
    // dump the miter parts
    if ( 0 )
    {
        char pFileName0[1000], pFileName1[1000];
        char * pNameGeneric = Extra_FileNameGeneric( p->pSpec );
        Vec_Int_t * vOrder = Vec_IntStartNatural( Gia_ManPoNum(pNew) );
        Gia_Man_t * pGia0 = Gia_ManDupCones( pNew, Vec_IntArray(vOrder),                         Vec_IntSize(vOrder)/2, 0 );
        Gia_Man_t * pGia1 = Gia_ManDupCones( pNew, Vec_IntArray(vOrder) + Vec_IntSize(vOrder)/2, Vec_IntSize(vOrder)/2, 0 );
        assert( Gia_ManPoNum(pNew) % 2 == 0 );
        sprintf( pFileName0, "%s_lhs_.aig", pNameGeneric );
        sprintf( pFileName1, "%s_rhs_.aig", pNameGeneric );
        Gia_AigerWrite( pGia0, pFileName0, 0, 0, 0 );
        Gia_AigerWrite( pGia1, pFileName1, 0, 0, 0 );
        Gia_ManStop( pGia0 );
        Gia_ManStop( pGia1 );
        Vec_IntFree( vOrder );
        ABC_FREE( pNameGeneric );
        printf( "Dumped two parts of the miter into files \"%s\" and \"%s\".\n", pFileName0, pFileName1 );
    }
    //Wlc_NtkPrintNameArray( pNew->vNamesIn );
    //Wlc_NtkPrintNameArray( pNew->vNamesOut );
    if ( pPar->vBoxIds )
    {
        Vec_PtrFreeFree( pNew->vNamesIn );   pNew->vNamesIn  = NULL;
        Vec_PtrFreeFree( pNew->vNamesOut );  pNew->vNamesOut = NULL;
    }
    pNew->vRegClasses = vRegClasses;
    return pNew;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float * Extra_FileReadFloat( FILE * pFile, int * pnFileSize )
{
    float * pBuffer;
    int RetValue, nFileSize;
    fseek( pFile, 0, SEEK_END );  
    nFileSize = *pnFileSize = ftell( pFile );  
    rewind( pFile ); 
    assert( nFileSize%4 == 0 );
    pBuffer = ABC_CALLOC( float, nFileSize/4 );
    RetValue = fread( pBuffer, nFileSize, 1, pFile );
    return pBuffer;
}
float * Extra_FileReadFloatContents( char * pFileName, int * pnFileSize )
{
    FILE * pFile;
    float * pBuffer;
    pFile = fopen( pFileName, "rb" );
    pBuffer = pFile ? Extra_FileReadFloat( pFile, pnFileSize ) : NULL;
    if ( pFile )  fclose( pFile );
    return pBuffer;
}
static inline int Extra_FixedFound( int Value, int Fixed )
{
    Value  += 1 << (Fixed-1);
    Value >>= Fixed;
    return Value;
}
static inline int Extra_ConvertFloat8( float Value )
{
    return Extra_FixedFound( (int)(Value * (1 << 16)), 8 );
}
Gia_Man_t * Wlc_BlastArray( char * pFileName )
{
    int nFileSize = 0;
    float * pBuffer = Extra_FileReadFloatContents( pFileName, &nFileSize );
    int i, v, Value, nInputs = nFileSize/4 - 1;
    Vec_Int_t * vArg0 = Vec_IntAlloc( 100 );
    Vec_Int_t * vArg1 = Vec_IntAlloc( 100 );
    Vec_Int_t * vTemp = Vec_IntAlloc( 100 );
    Vec_Int_t * vRes  = Vec_IntAlloc( 100 );
    Vec_Int_t * vSum  = Vec_IntAlloc( 100 );
    Gia_Man_t * pTemp, * pNew = Gia_ManStart( 10000 );
    pNew->pName = Abc_UtilStrsav( "blast" );
    Gia_ManHashAlloc( pNew );
    for ( i = 0; i < 8*nInputs; i++ )
        Gia_ManAppendCi(pNew);

    Value = (Extra_ConvertFloat8(pBuffer[0]) << 8) | (1 << 7);
    for ( v = 0; v < 20; v++ )
        Vec_IntPush( vSum, (Value >> v) & 1 );
    
    for ( i = 0; i < nInputs; i++ )
    {
        Value = Extra_ConvertFloat8( pBuffer[1+i] );

        Vec_IntClear( vArg0 );
        for ( v = 0; v < 8; v++ )
            Vec_IntPush( vArg0, Gia_ManCiLit(pNew, 8*i+v) );

        Vec_IntClear( vArg1 );
        for ( v = 0; v < 12; v++ )
            Vec_IntPush( vArg1, (Value >> v) & 1 );

        Wlc_BlastMultiplier( pNew, Vec_IntArray(vArg0), Vec_IntArray(vArg1), 8, 12, vTemp, vRes, 1 );
        Wlc_BlastAdder( pNew, Vec_IntArray(vSum), Vec_IntArray(vRes), 20, 0 );
    }
    ABC_FREE( pBuffer );
    for ( v = 8; v < 16; v++ )
        Gia_ManAppendCo( pNew, Vec_IntEntry(vSum, v) );
    Vec_IntFree( vArg0 );
    Vec_IntFree( vArg1 );
    Vec_IntFree( vTemp );
    Vec_IntFree( vRes );
    Vec_IntFree( vSum );

    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Wlc_ComputePerm( Wlc_Ntk_t * pNtk, int nPis )
{
    Vec_Int_t * vPerm  = Vec_IntAlloc( 100 );
    Vec_Int_t * vSizes = Vec_IntAlloc( 100 );
    Vec_Int_t * vOffs  = Vec_IntAlloc( 100 );
    Wlc_Obj_t * pObj; 
    int i, k, First, Size, nBitCis = 0, fChange = 1;
    Wlc_NtkForEachPi( pNtk, pObj, i )
    {
        Vec_IntPush( vOffs, nBitCis );
        Vec_IntPush( vSizes, Wlc_ObjRange(pObj) );
        nBitCis += Wlc_ObjRange(pObj);
    }
    for ( k = 0; fChange; k++ )
    {
        fChange = 0;
        Vec_IntForEachEntryTwo( vOffs, vSizes, First, Size, i )
            if ( k < Size )
            {
                Vec_IntPush( vPerm, First+k );
                fChange = 1;
            }
    }
    assert( Vec_IntSize(vPerm) == nBitCis );
    Vec_IntFree( vOffs );
    Vec_IntFree( vSizes );
    Vec_IntReverseOrder( vPerm );
    for ( i = Vec_IntSize(vPerm); i < nPis; i++ )
        Vec_IntPush( vPerm, i );
    //Vec_IntPrint( vPerm );
    return vPerm;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

