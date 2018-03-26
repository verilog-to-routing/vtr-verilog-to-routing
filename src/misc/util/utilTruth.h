/**CFile****************************************************************

  FileName    [utilTruth.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Truth table manipulation.]

  Synopsis    [Truth table manipulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - October 28, 2012.]

  Revision    [$Id: utilTruth.h,v 1.00 2012/10/28 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__misc__util__utilTruth_h
#define ABC__misc__util__utilTruth_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

static word s_Truths6[6] = {
    ABC_CONST(0xAAAAAAAAAAAAAAAA),
    ABC_CONST(0xCCCCCCCCCCCCCCCC),
    ABC_CONST(0xF0F0F0F0F0F0F0F0),
    ABC_CONST(0xFF00FF00FF00FF00),
    ABC_CONST(0xFFFF0000FFFF0000),
    ABC_CONST(0xFFFFFFFF00000000)
};

static word s_Truths6Neg[6] = {
    ABC_CONST(0x5555555555555555),
    ABC_CONST(0x3333333333333333),
    ABC_CONST(0x0F0F0F0F0F0F0F0F),
    ABC_CONST(0x00FF00FF00FF00FF),
    ABC_CONST(0x0000FFFF0000FFFF),
    ABC_CONST(0x00000000FFFFFFFF)
};

static word s_TruthXors[6] = {
    ABC_CONST(0x0000000000000000),
    ABC_CONST(0x6666666666666666),
    ABC_CONST(0x6969696969696969),
    ABC_CONST(0x6996699669966996),
    ABC_CONST(0x6996966969969669),
    ABC_CONST(0x6996966996696996)
};

static word s_PMasks[5][3] = {
    { ABC_CONST(0x9999999999999999), ABC_CONST(0x2222222222222222), ABC_CONST(0x4444444444444444) },
    { ABC_CONST(0xC3C3C3C3C3C3C3C3), ABC_CONST(0x0C0C0C0C0C0C0C0C), ABC_CONST(0x3030303030303030) },
    { ABC_CONST(0xF00FF00FF00FF00F), ABC_CONST(0x00F000F000F000F0), ABC_CONST(0x0F000F000F000F00) },
    { ABC_CONST(0xFF0000FFFF0000FF), ABC_CONST(0x0000FF000000FF00), ABC_CONST(0x00FF000000FF0000) },
    { ABC_CONST(0xFFFF00000000FFFF), ABC_CONST(0x00000000FFFF0000), ABC_CONST(0x0000FFFF00000000) }
};

static word s_PPMasks[5][6][3] = {
    { 
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000) }, // 0 0  
        { ABC_CONST(0x9999999999999999), ABC_CONST(0x2222222222222222), ABC_CONST(0x4444444444444444) }, // 0 1  
        { ABC_CONST(0xA5A5A5A5A5A5A5A5), ABC_CONST(0x0A0A0A0A0A0A0A0A), ABC_CONST(0x5050505050505050) }, // 0 2 
        { ABC_CONST(0xAA55AA55AA55AA55), ABC_CONST(0x00AA00AA00AA00AA), ABC_CONST(0x5500550055005500) }, // 0 3 
        { ABC_CONST(0xAAAA5555AAAA5555), ABC_CONST(0x0000AAAA0000AAAA), ABC_CONST(0x5555000055550000) }, // 0 4 
        { ABC_CONST(0xAAAAAAAA55555555), ABC_CONST(0x00000000AAAAAAAA), ABC_CONST(0x5555555500000000) }  // 0 5 
    },
    { 
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000) }, // 1 0  
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000) }, // 1 1  
        { ABC_CONST(0xC3C3C3C3C3C3C3C3), ABC_CONST(0x0C0C0C0C0C0C0C0C), ABC_CONST(0x3030303030303030) }, // 1 2 
        { ABC_CONST(0xCC33CC33CC33CC33), ABC_CONST(0x00CC00CC00CC00CC), ABC_CONST(0x3300330033003300) }, // 1 3 
        { ABC_CONST(0xCCCC3333CCCC3333), ABC_CONST(0x0000CCCC0000CCCC), ABC_CONST(0x3333000033330000) }, // 1 4 
        { ABC_CONST(0xCCCCCCCC33333333), ABC_CONST(0x00000000CCCCCCCC), ABC_CONST(0x3333333300000000) }  // 1 5 
    },
    { 
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000) }, // 2 0  
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000) }, // 2 1  
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000) }, // 2 2 
        { ABC_CONST(0xF00FF00FF00FF00F), ABC_CONST(0x00F000F000F000F0), ABC_CONST(0x0F000F000F000F00) }, // 2 3 
        { ABC_CONST(0xF0F00F0FF0F00F0F), ABC_CONST(0x0000F0F00000F0F0), ABC_CONST(0x0F0F00000F0F0000) }, // 2 4 
        { ABC_CONST(0xF0F0F0F00F0F0F0F), ABC_CONST(0x00000000F0F0F0F0), ABC_CONST(0x0F0F0F0F00000000) }  // 2 5 
    },
    { 
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000) }, // 3 0  
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000) }, // 3 1  
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000) }, // 3 2 
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000) }, // 3 3 
        { ABC_CONST(0xFF0000FFFF0000FF), ABC_CONST(0x0000FF000000FF00), ABC_CONST(0x00FF000000FF0000) }, // 3 4 
        { ABC_CONST(0xFF00FF0000FF00FF), ABC_CONST(0x00000000FF00FF00), ABC_CONST(0x00FF00FF00000000) }  // 3 5 
    },
    { 
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000) }, // 4 0  
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000) }, // 4 1  
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000) }, // 4 2 
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000) }, // 4 3 
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000) }, // 4 4 
        { ABC_CONST(0xFFFF00000000FFFF), ABC_CONST(0x00000000FFFF0000), ABC_CONST(0x0000FFFF00000000) }  // 4 5 
    }
};

// the bit count for the first 256 integer numbers
static int Abc_TtBitCount8[256] = {
    0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};
static inline int Abc_TtBitCount16( int i ) { return Abc_TtBitCount8[i & 0xFF] + Abc_TtBitCount8[i >> 8]; }

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
// read/write/flip i-th bit of a bit string table:
static inline int     Abc_TtGetBit( word * p, int i )         { return (int)(p[i>>6] >> (i & 63)) & 1;        }
static inline void    Abc_TtSetBit( word * p, int i )         { p[i>>6] |= (((word)1)<<(i & 63));             }
static inline void    Abc_TtXorBit( word * p, int i )         { p[i>>6] ^= (((word)1)<<(i & 63));             }

// read/write k-th digit d of a quaternary number:
static inline int     Abc_TtGetQua( word * p, int k )         { return (int)(p[k>>5] >> ((k<<1) & 63)) & 3;   }
static inline void    Abc_TtSetQua( word * p, int k, int d )  { p[k>>5] |= (((word)d)<<((k<<1) & 63));        }
static inline void    Abc_TtXorQua( word * p, int k, int d )  { p[k>>5] ^= (((word)d)<<((k<<1) & 63));        }

// read/write k-th digit d of a hexadecimal number:
static inline int     Abc_TtGetHex( word * p, int k )         { return (int)(p[k>>4] >> ((k<<2) & 63)) & 15;  }
static inline void    Abc_TtSetHex( word * p, int k, int d )  { p[k>>4] |= (((word)d)<<((k<<2) & 63));        }
static inline void    Abc_TtXorHex( word * p, int k, int d )  { p[k>>4] ^= (((word)d)<<((k<<2) & 63));        }

// read/write k-th digit d of a 256-base number:
static inline int     Abc_TtGet256( word * p, int k )         { return (int)(p[k>>3] >> ((k<<3) & 63)) & 255; }
static inline void    Abc_TtSet256( word * p, int k, int d )  { p[k>>3] |= (((word)d)<<((k<<3) & 63));        }
static inline void    Abc_TtXor256( word * p, int k, int d )  { p[k>>3] ^= (((word)d)<<((k<<3) & 63));        }

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int  Abc_TtWordNum( int nVars )     { return nVars <= 6 ? 1 : 1 << (nVars-6); }
static inline int  Abc_TtByteNum( int nVars )     { return nVars <= 3 ? 1 : 1 << (nVars-3); }
static inline int  Abc_TtHexDigitNum( int nVars ) { return nVars <= 2 ? 1 : 1 << (nVars-2); }

/**Function*************************************************************

  Synopsis    [Bit mask.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Abc_Tt6Mask( int nBits )       { assert( nBits >= 0 && nBits <= 64 ); return (~(word)0) >> (64-nBits);        }
static inline void Abc_TtMask( word * pTruth, int nWords, int nBits )
{ 
    int w;
    assert( nBits >= 0 && nBits <= nWords * 64 );
    for ( w = 0; w < nWords; w++ )
        if ( nBits >= (w + 1) * 64 )
            pTruth[w] = ~(word)0;
        else if ( nBits > w * 64 )
            pTruth[w] = Abc_Tt6Mask( nBits - w * 64 );
        else
            pTruth[w] = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_TtConst( word * pOut, int nWords, int fConst1 )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        pOut[w] = fConst1 ? ~(word)0 : 0;
}
static inline void Abc_TtClear( word * pOut, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        pOut[w] = 0;
}
static inline void Abc_TtFill( word * pOut, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        pOut[w] = ~(word)0;
}
static inline void Abc_TtUnit( word * pOut, int nWords, int fCompl )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        pOut[w] = fCompl ? ~s_Truths6[0] : s_Truths6[0];
}
static inline void Abc_TtNot( word * pOut, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        pOut[w] = ~pOut[w];
}
static inline void Abc_TtCopy( word * pOut, word * pIn, int nWords, int fCompl )
{
    int w;
    if ( fCompl )
        for ( w = 0; w < nWords; w++ )
            pOut[w] = ~pIn[w];
    else
        for ( w = 0; w < nWords; w++ )
            pOut[w] = pIn[w];
}
static inline void Abc_TtAnd( word * pOut, word * pIn1, word * pIn2, int nWords, int fCompl )
{
    int w;
    if ( fCompl )
        for ( w = 0; w < nWords; w++ )
            pOut[w] = ~(pIn1[w] & pIn2[w]);
    else
        for ( w = 0; w < nWords; w++ )
            pOut[w] = pIn1[w] & pIn2[w];
}
static inline void Abc_TtAndCompl( word * pOut, word * pIn1, int fCompl1, word * pIn2, int fCompl2, int nWords )
{
    int w;
    if ( fCompl1 )
    {
        if ( fCompl2 )
            for ( w = 0; w < nWords; w++ )
                pOut[w] = ~pIn1[w] & ~pIn2[w];
        else
            for ( w = 0; w < nWords; w++ )
                pOut[w] = ~pIn1[w] & pIn2[w];
    }
    else
    {
        if ( fCompl2 )
            for ( w = 0; w < nWords; w++ )
                pOut[w] = pIn1[w] & ~pIn2[w];
        else
            for ( w = 0; w < nWords; w++ )
                pOut[w] = pIn1[w] & pIn2[w];
    }
}
static inline void Abc_TtAndSharp( word * pOut, word * pIn1, word * pIn2, int nWords, int fCompl )
{
    int w;
    if ( fCompl )
        for ( w = 0; w < nWords; w++ )
            pOut[w] = pIn1[w] & ~pIn2[w];
    else
        for ( w = 0; w < nWords; w++ )
            pOut[w] = pIn1[w] & pIn2[w];
}
static inline void Abc_TtSharp( word * pOut, word * pIn1, word * pIn2, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        pOut[w] = pIn1[w] & ~pIn2[w];
}
static inline void Abc_TtOr( word * pOut, word * pIn1, word * pIn2, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        pOut[w] = pIn1[w] | pIn2[w];
}
static inline void Abc_TtOrXor( word * pOut, word * pIn1, word * pIn2, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        pOut[w] |= pIn1[w] ^ pIn2[w];
}
static inline void Abc_TtXor( word * pOut, word * pIn1, word * pIn2, int nWords, int fCompl )
{
    int w;
    if ( fCompl )
        for ( w = 0; w < nWords; w++ )
            pOut[w] = pIn1[w] ^ ~pIn2[w];
    else
        for ( w = 0; w < nWords; w++ )
            pOut[w] = pIn1[w] ^ pIn2[w];
}
static inline void Abc_TtMux( word * pOut, word * pCtrl, word * pIn1, word * pIn0, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        pOut[w] = (pCtrl[w] & pIn1[w]) | (~pCtrl[w] & pIn0[w]);
}
static inline void Abc_TtMaj( word * pOut, word * pIn0, word * pIn1, word * pIn2, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        pOut[w] = (pIn0[w] & pIn1[w]) | (pIn0[w] & pIn2[w]) | (pIn1[w] & pIn2[w]);
}
static inline int Abc_TtIntersect( word * pIn1, word * pIn2, int nWords, int fCompl )
{
    int w;
    if ( fCompl )
    {
        for ( w = 0; w < nWords; w++ )
            if ( ~pIn1[w] & pIn2[w] )
                return 1;
    }
    else
    {
        for ( w = 0; w < nWords; w++ )
            if ( pIn1[w] & pIn2[w] )
                return 1;
    }
    return 0;
}
static inline int Abc_TtEqual( word * pIn1, word * pIn2, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        if ( pIn1[w] != pIn2[w] )
            return 0;
    return 1;
}
static inline int Abc_TtOpposite( word * pIn1, word * pIn2, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        if ( pIn1[w] != ~pIn2[w] )
            return 0;
    return 1;
}
static inline int Abc_TtImply( word * pIn1, word * pIn2, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        if ( (pIn1[w] & pIn2[w]) != pIn1[w] )
            return 0;
    return 1;
}
static inline int Abc_TtCompare( word * pIn1, word * pIn2, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        if ( pIn1[w] != pIn2[w] )
            return (pIn1[w] < pIn2[w]) ? -1 : 1;
    return 0;
}
static inline int Abc_TtCompareRev( word * pIn1, word * pIn2, int nWords )
{
    int w;
    for ( w = nWords - 1; w >= 0; w-- )
        if ( pIn1[w] != pIn2[w] )
            return (pIn1[w] < pIn2[w]) ? -1 : 1;
    return 0;
}
static inline int Abc_TtIsConst0( word * pIn1, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        if ( pIn1[w] )
            return 0;
    return 1;
}
static inline int Abc_TtIsConst1( word * pIn1, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        if ( ~pIn1[w] )
            return 0;
    return 1;
}
static inline void Abc_TtConst0( word * pIn1, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        pIn1[w] = 0;
}
static inline void Abc_TtConst1( word * pIn1, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        pIn1[w] = ~(word)0;
}
static inline void Abc_TtIthVar( word * pOut, int iVar, int nVars )
{
    int k, nWords = Abc_TtWordNum( nVars );
    if ( iVar < 6 )
    {
        for ( k = 0; k < nWords; k++ )
            pOut[k] = s_Truths6[iVar];
    }
    else
    {
        for ( k = 0; k < nWords; k++ )
            if ( k & (1 << (iVar-6)) )
                pOut[k] = ~(word)0;
            else
                pOut[k] = 0;
    }
}

/**Function*************************************************************

  Synopsis    [Compares Cof0 and Cof1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_TtCompare1VarCofs( word * pTruth, int nWords, int iVar )
{
    if ( nWords == 1 )
    {
        word Cof0 = pTruth[0] & s_Truths6Neg[iVar];
        word Cof1 = (pTruth[0] >> (1 << iVar)) & s_Truths6Neg[iVar];
        if ( Cof0 != Cof1 )
            return Cof0 < Cof1 ? -1 : 1;
        return 0;
    }
    if ( iVar <= 5 )
    {
        word Cof0, Cof1;
        int w, shift = (1 << iVar);
        for ( w = 0; w < nWords; w++ )
        {
            Cof0 = pTruth[w] & s_Truths6Neg[iVar];
            Cof1 = (pTruth[w] >> shift) & s_Truths6Neg[iVar];
            if ( Cof0 != Cof1 )
                return Cof0 < Cof1 ? -1 : 1;
        }
        return 0;
    }
    // if ( iVar > 5 )
    {
        word * pLimit = pTruth + nWords;
        int i, iStep = Abc_TtWordNum(iVar);
        assert( nWords >= 2 );
        for ( ; pTruth < pLimit; pTruth += 2*iStep )
            for ( i = 0; i < iStep; i++ )
                if ( pTruth[i] != pTruth[i + iStep] )
                    return pTruth[i] < pTruth[i + iStep] ? -1 : 1;
        return 0;
    }    
}
static inline int Abc_TtCompare1VarCofsRev( word * pTruth, int nWords, int iVar )
{
    if ( nWords == 1 )
    {
        word Cof0 = pTruth[0] & s_Truths6Neg[iVar];
        word Cof1 = (pTruth[0] >> (1 << iVar)) & s_Truths6Neg[iVar];
        if ( Cof0 != Cof1 )
            return Cof0 < Cof1 ? -1 : 1;
        return 0;
    }
    if ( iVar <= 5 )
    {
        word Cof0, Cof1;
        int w, shift = (1 << iVar);
        for ( w = nWords - 1; w >= 0; w-- )
        {
            Cof0 = pTruth[w] & s_Truths6Neg[iVar];
            Cof1 = (pTruth[w] >> shift) & s_Truths6Neg[iVar];
            if ( Cof0 != Cof1 )
                return Cof0 < Cof1 ? -1 : 1;
        }
        return 0;
    }
    // if ( iVar > 5 )
    {
        word * pLimit = pTruth + nWords;
        int i, iStep = Abc_TtWordNum(iVar);
        assert( nWords >= 2 );
        for ( pLimit -= 2*iStep; pLimit >= pTruth; pLimit -= 2*iStep )
            for ( i = iStep - 1; i >= 0; i-- )
                if ( pLimit[i] != pLimit[i + iStep] )
                    return pLimit[i] < pLimit[i + iStep] ? -1 : 1;
        return 0;
    }    
}

/**Function*************************************************************

  Synopsis    [Compute elementary truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_TtElemInit( word ** pTtElems, int nVars )
{
    int i, k, nWords = Abc_TtWordNum( nVars );
    for ( i = 0; i < nVars; i++ )
        if ( i < 6 )
            for ( k = 0; k < nWords; k++ )
                pTtElems[i][k] = s_Truths6[i];
        else
            for ( k = 0; k < nWords; k++ )
                pTtElems[i][k] = (k & (1 << (i-6))) ? ~(word)0 : 0;
}
static inline void Abc_TtElemInit2( word * pTtElems, int nVars )
{
    int i, k, nWords = Abc_TtWordNum( nVars );
    for ( i = 0; i < nVars; i++ )
    {
        word * pTruth = pTtElems + i * nWords;
        if ( i < 6 )
            for ( k = 0; k < nWords; k++ )
                pTruth[k] = s_Truths6[i];
        else
            for ( k = 0; k < nWords; k++ )
                pTruth[k] = (k & (1 << (i-6))) ? ~(word)0 : 0;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Abc_Tt6Cofactor0( word t, int iVar )
{
    assert( iVar >= 0 && iVar < 6 );
    return (t &s_Truths6Neg[iVar]) | ((t &s_Truths6Neg[iVar]) << (1<<iVar));
}
static inline word Abc_Tt6Cofactor1( word t, int iVar )
{
    assert( iVar >= 0 && iVar < 6 );
    return (t & s_Truths6[iVar]) | ((t & s_Truths6[iVar]) >> (1<<iVar));
}

static inline void Abc_TtCofactor0p( word * pOut, word * pIn, int nWords, int iVar )
{
    if ( nWords == 1 )
        pOut[0] = ((pIn[0] & s_Truths6Neg[iVar]) << (1 << iVar)) | (pIn[0] & s_Truths6Neg[iVar]);
    else if ( iVar <= 5 )
    {
        int w, shift = (1 << iVar);
        for ( w = 0; w < nWords; w++ )
            pOut[w] = ((pIn[w] & s_Truths6Neg[iVar]) << shift) | (pIn[w] & s_Truths6Neg[iVar]);
    }
    else // if ( iVar > 5 )
    {
        word * pLimit = pIn + nWords;
        int i, iStep = Abc_TtWordNum(iVar);
        for ( ; pIn < pLimit; pIn += 2*iStep, pOut += 2*iStep )
            for ( i = 0; i < iStep; i++ )
            {
                pOut[i]         = pIn[i];
                pOut[i + iStep] = pIn[i];
            }
    }    
}
static inline void Abc_TtCofactor1p( word * pOut, word * pIn, int nWords, int iVar )
{
    if ( nWords == 1 )
        pOut[0] = (pIn[0] & s_Truths6[iVar]) | ((pIn[0] & s_Truths6[iVar]) >> (1 << iVar));
    else if ( iVar <= 5 )
    {
        int w, shift = (1 << iVar);
        for ( w = 0; w < nWords; w++ )
            pOut[w] = (pIn[w] & s_Truths6[iVar]) | ((pIn[w] & s_Truths6[iVar]) >> shift);
    }
    else // if ( iVar > 5 )
    {
        word * pLimit = pIn + nWords;
        int i, iStep = Abc_TtWordNum(iVar);
        for ( ; pIn < pLimit; pIn += 2*iStep, pOut += 2*iStep )
            for ( i = 0; i < iStep; i++ )
            {
                pOut[i]         = pIn[i + iStep];
                pOut[i + iStep] = pIn[i + iStep];
            }
    }    
}
static inline void Abc_TtCofactor0( word * pTruth, int nWords, int iVar )
{
    if ( nWords == 1 )
        pTruth[0] = ((pTruth[0] & s_Truths6Neg[iVar]) << (1 << iVar)) | (pTruth[0] & s_Truths6Neg[iVar]);
    else if ( iVar <= 5 )
    {
        int w, shift = (1 << iVar);
        for ( w = 0; w < nWords; w++ )
            pTruth[w] = ((pTruth[w] & s_Truths6Neg[iVar]) << shift) | (pTruth[w] & s_Truths6Neg[iVar]);
    }
    else // if ( iVar > 5 )
    {
        word * pLimit = pTruth + nWords;
        int i, iStep = Abc_TtWordNum(iVar);
        for ( ; pTruth < pLimit; pTruth += 2*iStep )
            for ( i = 0; i < iStep; i++ )
                pTruth[i + iStep] = pTruth[i];
    }
}
static inline void Abc_TtCofactor1( word * pTruth, int nWords, int iVar )
{
    if ( nWords == 1 )
        pTruth[0] = (pTruth[0] & s_Truths6[iVar]) | ((pTruth[0] & s_Truths6[iVar]) >> (1 << iVar));
    else if ( iVar <= 5 )
    {
        int w, shift = (1 << iVar);
        for ( w = 0; w < nWords; w++ )
            pTruth[w] = (pTruth[w] & s_Truths6[iVar]) | ((pTruth[w] & s_Truths6[iVar]) >> shift);
    }
    else // if ( iVar > 5 )
    {
        word * pLimit = pTruth + nWords;
        int i, iStep = Abc_TtWordNum(iVar);
        for ( ; pTruth < pLimit; pTruth += 2*iStep )
            for ( i = 0; i < iStep; i++ )
                pTruth[i] = pTruth[i + iStep];
    }
}

/**Function*************************************************************

  Synopsis    [Checks pairs of cofactors w.r.t. two variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_TtCheckEqualCofs( word * pTruth, int nWords, int iVar, int jVar, int Num1, int Num2 )
{
    assert( Num1 < Num2 && Num2 < 4 );
    assert( iVar < jVar );
    if ( nWords == 1 )
    {
        word Mask = s_Truths6Neg[jVar] & s_Truths6Neg[iVar];
        int shift1 = (Num1 >> 1) * (1 << jVar) + (Num1 & 1) * (1 << iVar);
        int shift2 = (Num2 >> 1) * (1 << jVar) + (Num2 & 1) * (1 << iVar);
        return ((pTruth[0] >> shift1) & Mask) == ((pTruth[0] >> shift2) & Mask);
    }
    if ( jVar <= 5 )
    {
        word Mask = s_Truths6Neg[jVar] & s_Truths6Neg[iVar];
        int shift1 = (Num1 >> 1) * (1 << jVar) + (Num1 & 1) * (1 << iVar);
        int shift2 = (Num2 >> 1) * (1 << jVar) + (Num2 & 1) * (1 << iVar);
        int w;
        for ( w = 0; w < nWords; w++ )
            if ( ((pTruth[w] >> shift1) & Mask) != ((pTruth[w] >> shift2) & Mask) )
                return 0;
        return 1;
    }
    if ( iVar <= 5 && jVar > 5 )
    {
        word * pLimit = pTruth + nWords;
        int j, jStep = Abc_TtWordNum(jVar);
        int shift1 = (Num1 & 1) * (1 << iVar);
        int shift2 = (Num2 & 1) * (1 << iVar);
        int Offset1 = (Num1 >> 1) * jStep;
        int Offset2 = (Num2 >> 1) * jStep;
        for ( ; pTruth < pLimit; pTruth += 2*jStep )
            for ( j = 0; j < jStep; j++ )
                if ( ((pTruth[j + Offset1] >> shift1) & s_Truths6Neg[iVar]) != ((pTruth[j + Offset2] >> shift2) & s_Truths6Neg[iVar]) )
                    return 0;
        return 1;
    }
    {
        word * pLimit = pTruth + nWords;
        int j, jStep = Abc_TtWordNum(jVar);
        int i, iStep = Abc_TtWordNum(iVar);
        int Offset1 = (Num1 >> 1) * jStep + (Num1 & 1) * iStep;
        int Offset2 = (Num2 >> 1) * jStep + (Num2 & 1) * iStep;
        for ( ; pTruth < pLimit; pTruth += 2*jStep )
            for ( i = 0; i < jStep; i += 2*iStep )
                for ( j = 0; j < iStep; j++ )
                    if ( pTruth[Offset1 + i + j] != pTruth[Offset2 + i + j] )
                        return 0;
        return 1;
    }    
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_Tt6Cof0IsConst0( word t, int iVar ) { return (t & s_Truths6Neg[iVar]) == 0;                                          }
static inline int Abc_Tt6Cof0IsConst1( word t, int iVar ) { return (t & s_Truths6Neg[iVar]) == s_Truths6Neg[iVar];                         }
static inline int Abc_Tt6Cof1IsConst0( word t, int iVar ) { return (t & s_Truths6[iVar]) == 0;                                             }
static inline int Abc_Tt6Cof1IsConst1( word t, int iVar ) { return (t & s_Truths6[iVar]) == s_Truths6[iVar];                               }
static inline int Abc_Tt6CofsOpposite( word t, int iVar ) { return (~t & s_Truths6Neg[iVar]) == ((t >> (1 << iVar)) & s_Truths6Neg[iVar]); } 
static inline int Abc_Tt6Cof0EqualCof1( word t1, word t2, int iVar ) { return (t1 & s_Truths6Neg[iVar]) == ((t2 >> (1 << iVar)) & s_Truths6Neg[iVar]); } 
static inline int Abc_Tt6Cof0EqualCof0( word t1, word t2, int iVar ) { return (t1 & s_Truths6Neg[iVar]) == (t2 & s_Truths6Neg[iVar]); } 
static inline int Abc_Tt6Cof1EqualCof1( word t1, word t2, int iVar ) { return (t1 & s_Truths6[iVar])    == (t2 & s_Truths6[iVar]); } 

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_TtTruthIsConst0( word * p, int nWords ) { int w; for ( w = 0; w < nWords; w++ ) if ( p[w] != 0        ) return 0; return 1; }
static inline int Abc_TtTruthIsConst1( word * p, int nWords ) { int w; for ( w = 0; w < nWords; w++ ) if ( p[w] != ~(word)0 ) return 0; return 1; }

static inline int Abc_TtCof0IsConst0( word * t, int nWords, int iVar ) 
{ 
    if ( iVar < 6 )
    {
        int i;
        for ( i = 0; i < nWords; i++ )
            if ( t[i] & s_Truths6Neg[iVar] )
                return 0;
        return 1;
    }
    else
    {
        int i, Step = (1 << (iVar - 6));
        word * tLimit = t + nWords;
        for ( ; t < tLimit; t += 2*Step )
            for ( i = 0; i < Step; i++ )
                if ( t[i] )
                    return 0;
        return 1;
    }
}
static inline int Abc_TtCof0IsConst1( word * t, int nWords, int iVar ) 
{ 
    if ( iVar < 6 )
    {
        int i;
        for ( i = 0; i < nWords; i++ )
            if ( (t[i] & s_Truths6Neg[iVar]) != s_Truths6Neg[iVar] )
                return 0;
        return 1;
    }
    else
    {
        int i, Step = (1 << (iVar - 6));
        word * tLimit = t + nWords;
        for ( ; t < tLimit; t += 2*Step )
            for ( i = 0; i < Step; i++ )
                if ( ~t[i] )
                    return 0;
        return 1;
    }
}
static inline int Abc_TtCof1IsConst0( word * t, int nWords, int iVar ) 
{ 
    if ( iVar < 6 )
    {
        int i;
        for ( i = 0; i < nWords; i++ )
            if ( t[i] & s_Truths6[iVar] )
                return 0;
        return 1;
    }
    else
    {
        int i, Step = (1 << (iVar - 6));
        word * tLimit = t + nWords;
        for ( ; t < tLimit; t += 2*Step )
            for ( i = 0; i < Step; i++ )
                if ( t[i+Step] )
                    return 0;
        return 1;
    }
}
static inline int Abc_TtCof1IsConst1( word * t, int nWords, int iVar ) 
{ 
    if ( iVar < 6 )
    {
        int i;
        for ( i = 0; i < nWords; i++ )
            if ( (t[i] & s_Truths6[iVar]) != s_Truths6[iVar] )
                return 0;
        return 1;
    }
    else
    {
        int i, Step = (1 << (iVar - 6));
        word * tLimit = t + nWords;
        for ( ; t < tLimit; t += 2*Step )
            for ( i = 0; i < Step; i++ )
                if ( ~t[i+Step] )
                    return 0;
        return 1;
    }
}
static inline int Abc_TtCofsOpposite( word * t, int nWords, int iVar ) 
{ 
    if ( iVar < 6 )
    {
        int i, Shift = (1 << iVar);
        for ( i = 0; i < nWords; i++ )
            if ( ((t[i] << Shift) & s_Truths6[iVar]) != (~t[i] & s_Truths6[iVar]) )
                return 0;
        return 1;
    }
    else
    {
        int i, Step = (1 << (iVar - 6));
        word * tLimit = t + nWords;
        for ( ; t < tLimit; t += 2*Step )
            for ( i = 0; i < Step; i++ )
                if ( t[i] != ~t[i+Step] )
                    return 0;
        return 1;
    }
}

/**Function*************************************************************

  Synopsis    [Stretch truthtable to have more input variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_TtStretch5( unsigned * pInOut, int nVarS, int nVarB )
{
    int w, i, step, nWords;
    if ( nVarS == nVarB )
        return;
    assert( nVarS < nVarB );
    step = Abc_TruthWordNum(nVarS);
    nWords = Abc_TruthWordNum(nVarB);
    if ( step == nWords )
        return;
    assert( step < nWords );
    for ( w = 0; w < nWords; w += step )
        for ( i = 0; i < step; i++ )
            pInOut[w + i] = pInOut[i];              
}
static inline void Abc_TtStretch6( word * pInOut, int nVarS, int nVarB )
{
    int w, i, step, nWords;
    if ( nVarS == nVarB )
        return;
    assert( nVarS < nVarB );
    step = Abc_Truth6WordNum(nVarS);
    nWords = Abc_Truth6WordNum(nVarB);
    if ( step == nWords )
        return;
    assert( step < nWords );
    for ( w = 0; w < nWords; w += step )
        for ( i = 0; i < step; i++ )
            pInOut[w + i] = pInOut[i];              
}
static inline word Abc_Tt6Stretch( word t, int nVars )
{
    assert( nVars >= 0 );
    if ( nVars == 0 )
        nVars++, t = (t & 0x1) | ((t & 0x1) << 1);
    if ( nVars == 1 )
        nVars++, t = (t & 0x3) | ((t & 0x3) << 2);
    if ( nVars == 2 )
        nVars++, t = (t & 0xF) | ((t & 0xF) << 4);
    if ( nVars == 3 )
        nVars++, t = (t & 0xFF) | ((t & 0xFF) << 8);
    if ( nVars == 4 )
        nVars++, t = (t & 0xFFFF) | ((t & 0xFFFF) << 16);
    if ( nVars == 5 )
        nVars++, t = (t & 0xFFFFFFFF) | ((t & 0xFFFFFFFF) << 32);
    assert( nVars == 6 );
    return t;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_TtIsHexDigit( char HexChar )
{
    return (HexChar >= '0' && HexChar <= '9') || (HexChar >= 'A' && HexChar <= 'F') || (HexChar >= 'a' && HexChar <= 'f');
}
static inline char Abc_TtPrintDigit( int Digit )
{
    assert( Digit >= 0 && Digit < 16 );
    if ( Digit < 10 )
        return '0' + Digit;
    return 'A' + Digit-10;
}
static inline char Abc_TtPrintDigitLower( int Digit )
{
    assert( Digit >= 0 && Digit < 16 );
    if ( Digit < 10 )
        return '0' + Digit;
    return 'a' + Digit-10;
}
static inline int Abc_TtReadHexDigit( char HexChar )
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

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_TtPrintHex( word * pTruth, int nVars )
{
    word * pThis, * pLimit = pTruth + Abc_TtWordNum(nVars);
    int k;
    assert( nVars >= 2 );
    for ( pThis = pTruth; pThis < pLimit; pThis++ )
        for ( k = 0; k < 16; k++ )
            printf( "%c", Abc_TtPrintDigit((int)(pThis[0] >> (k << 2)) & 15) );
    printf( "\n" );
}
static inline void Abc_TtPrintHexRev( FILE * pFile, word * pTruth, int nVars )
{
    word * pThis;
    int k, StartK = nVars >= 6 ? 16 : (1 << (nVars - 2));
    assert( nVars >= 2 );
    for ( pThis = pTruth + Abc_TtWordNum(nVars) - 1; pThis >= pTruth; pThis-- )
        for ( k = StartK - 1; k >= 0; k-- )
            fprintf( pFile, "%c", Abc_TtPrintDigit((int)(pThis[0] >> (k << 2)) & 15) );
//    printf( "\n" );
}
static inline void Abc_TtPrintHexSpecial( word * pTruth, int nVars )
{
    word * pThis;
    int k;
    assert( nVars >= 2 );
    for ( pThis = pTruth + Abc_TtWordNum(nVars) - 1; pThis >= pTruth; pThis-- )
        for ( k = 0; k < 16; k++ )
            printf( "%c", Abc_TtPrintDigit((int)(pThis[0] >> (k << 2)) & 15) );
    printf( "\n" );
}
static inline int Abc_TtWriteHexRev( char * pStr, word * pTruth, int nVars )
{
    word * pThis;
    char * pStrInit = pStr;
    int k, StartK = nVars >= 6 ? 16 : (1 << (nVars - 2));
    assert( nVars >= 2 );
    for ( pThis = pTruth + Abc_TtWordNum(nVars) - 1; pThis >= pTruth; pThis-- )
        for ( k = StartK - 1; k >= 0; k-- )
            *pStr++ = Abc_TtPrintDigit( (int)(pThis[0] >> (k << 2)) & 15 );
    return pStr - pStrInit;
}
static inline void Abc_TtPrintHexArrayRev( FILE * pFile, word * pTruth, int nDigits )
{
    int k;
    for ( k = nDigits - 1; k >= 0; k-- )
        fprintf( pFile, "%c", Abc_TtPrintDigitLower( Abc_TtGetHex(pTruth, k) ) );
}

/**Function*************************************************************

  Synopsis    [Reads hex truth table from a string.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_TtReadHex( word * pTruth, char * pString )
{
    int k, nVars, Digit, nDigits;
    // skip the first 2 symbols if they are "0x"
    if ( pString[0] == '0' && pString[1] == 'x' )
        pString += 2;
    // count the number of hex digits
    nDigits = 0;
    for ( k = 0; Abc_TtIsHexDigit(pString[k]); k++ )
        nDigits++;
    if ( nDigits == 1 )
    {
        if ( pString[0] == '0' || pString[0] == 'F' )
        {
            pTruth[0] = (pString[0] == '0') ? 0 : ~(word)0;
            return 0;
        }
        if ( pString[0] == '5' || pString[0] == 'A' )
        {
            pTruth[0] = (pString[0] == '5') ? s_Truths6Neg[0] : s_Truths6[0];
            return 1;
        }
    }
    // determine the number of variables
    nVars = 2 + (nDigits == 1 ? 0 : Abc_Base2Log(nDigits));
    // clean storage
    for ( k = Abc_TtWordNum(nVars) - 1; k >= 0; k-- )
        pTruth[k] = 0;
    // read hexadecimal digits in the reverse order
    // (the last symbol in the string is the least significant digit)
    for ( k = 0; k < nDigits; k++ )
    {
        Digit = Abc_TtReadHexDigit( pString[nDigits - 1 - k] );
        assert( Digit >= 0 && Digit < 16 );
        Abc_TtSetHex( pTruth, k, Digit );
    }
    if ( nVars < 6 )
        pTruth[0] = Abc_Tt6Stretch( pTruth[0], nVars );
    return nVars;
}
static inline int Abc_TtReadHexNumber( word * pTruth, char * pString )
{
    // count the number of hex digits
    int k, Digit, nDigits = 0;
    for ( k = 0; Abc_TtIsHexDigit(pString[k]); k++ )
        nDigits++;
    // read hexadecimal digits in the reverse order
    // (the last symbol in the string is the least significant digit)
    for ( k = 0; k < nDigits; k++ )
    {
        Digit = Abc_TtReadHexDigit( pString[nDigits - 1 - k] );
        assert( Digit >= 0 && Digit < 16 );
        Abc_TtSetHex( pTruth, k, Digit );
    }
    return nDigits;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_TtPrintBinary( word * pTruth, int nVars )
{
    word * pThis, * pLimit = pTruth + Abc_TtWordNum(nVars);
    int k, Limit = Abc_MinInt( 64, (1 << nVars) );
    assert( nVars >= 2 );
    for ( pThis = pTruth; pThis < pLimit; pThis++ )
        for ( k = 0; k < Limit; k++ )
            printf( "%d", Abc_InfoHasBit( (unsigned *)pThis, k ) );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_TtSuppFindFirst( int Supp )
{
    int i;
    assert( Supp > 0 );
    for ( i = 0; i < 32; i++ )
        if ( Supp & (1 << i) )
            return i;
    return -1;
}
static inline int Abc_TtSuppOnlyOne( int Supp )
{
    if ( Supp == 0 )
        return 0;
    return (Supp & (Supp-1)) == 0;
}
static inline int Abc_TtSuppIsMinBase( int Supp )
{
    assert( Supp > 0 );
    return (Supp & (Supp+1)) == 0;
}
static inline int Abc_Tt6HasVar( word t, int iVar )
{
    return ((t >> (1<<iVar)) & s_Truths6Neg[iVar]) != (t & s_Truths6Neg[iVar]);
}
static inline int Abc_TtHasVar( word * t, int nVars, int iVar )
{
    assert( iVar < nVars );
    if ( nVars <= 6 )
        return Abc_Tt6HasVar( t[0], iVar );
    if ( iVar < 6 )
    {
        int i, Shift = (1 << iVar);
        int nWords = Abc_TtWordNum( nVars );
        for ( i = 0; i < nWords; i++ )
            if ( ((t[i] >> Shift) & s_Truths6Neg[iVar]) != (t[i] & s_Truths6Neg[iVar]) )
                return 1;
        return 0;
    }
    else
    {
        int i, Step = (1 << (iVar - 6));
        word * tLimit = t + Abc_TtWordNum( nVars );
        for ( ; t < tLimit; t += 2*Step )
            for ( i = 0; i < Step; i++ )
                if ( t[i] != t[Step+i] )
                    return 1;
        return 0;
    }
}
static inline int Abc_TtSupport( word * t, int nVars )
{
    int v, Supp = 0;
    for ( v = 0; v < nVars; v++ )
        if ( Abc_TtHasVar( t, nVars, v ) )
            Supp |= (1 << v);
    return Supp;
}
static inline int Abc_TtSupportSize( word * t, int nVars )
{
    int v, SuppSize = 0;
    for ( v = 0; v < nVars; v++ )
        if ( Abc_TtHasVar( t, nVars, v ) )
            SuppSize++;
    return SuppSize;
}
static inline int Abc_TtSupportAndSize( word * t, int nVars, int * pSuppSize )
{
    int v, Supp = 0;
    *pSuppSize = 0;
    for ( v = 0; v < nVars; v++ )
        if ( Abc_TtHasVar( t, nVars, v ) )
            Supp |= (1 << v), (*pSuppSize)++;
    return Supp;
}
static inline int Abc_Tt6SupportAndSize( word t, int nVars, int * pSuppSize )
{
    int v, Supp = 0;
    *pSuppSize = 0;
    assert( nVars <= 6 );
    for ( v = 0; v < nVars; v++ )
        if ( Abc_Tt6HasVar( t, v ) )
            Supp |= (1 << v), (*pSuppSize)++;
    return Supp;
}

/**Function*************************************************************

  Synopsis    [Checks if there is a var whose both cofs have supp <= nSuppLim.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_TtCheckCondDep2( word * pTruth, int nVars, int nSuppLim )
{
    int v, d, nWords = Abc_TtWordNum(nVars);
    if ( nVars <= nSuppLim + 1 )
        return 0;
    for ( v = 0; v < nVars; v++ )
    {
        int nDep0 = 0, nDep1 = 0;
        for ( d = 0; d < nVars; d++ )
        {
            if ( v == d )
                continue;
            if ( v < d )
            {
                nDep0 += !Abc_TtCheckEqualCofs( pTruth, nWords, v, d, 0, 2 );
                nDep1 += !Abc_TtCheckEqualCofs( pTruth, nWords, v, d, 1, 3 );
            }
            else // if ( v > d )
            {
                nDep0 += !Abc_TtCheckEqualCofs( pTruth, nWords, d, v, 0, 1 );
                nDep1 += !Abc_TtCheckEqualCofs( pTruth, nWords, d, v, 2, 3 );
            }
            if ( nDep0 > nSuppLim || nDep1 > nSuppLim )
                break;
        }
        if ( d == nVars )
            return v;
    }
    return nVars;
}
static inline int Abc_TtCheckCondDep( word * pTruth, int nVars, int nSuppLim )
{
    int nVarsMax = 13;
    word Cof0[128], Cof1[128]; // pow( 2, nVarsMax-6 )
    int v, d, nWords = Abc_TtWordNum(nVars);
    assert( nVars <= nVarsMax );
    if ( nVars <= nSuppLim + 1 )
        return 0;
    for ( v = 0; v < nVars; v++ )
    {
        int nDep0 = 0, nDep1 = 0;
        Abc_TtCofactor0p( Cof0, pTruth, nWords, v );
        Abc_TtCofactor1p( Cof1, pTruth, nWords, v );
        for ( d = 0; d < nVars; d++ )
        {
            if ( v == d )
                continue;
            nDep0 += Abc_TtHasVar( Cof0, nVars, d );
            nDep1 += Abc_TtHasVar( Cof1, nVars, d );
            if ( nDep0 > nSuppLim || nDep1 > nSuppLim )
                break;
        }
        if ( d == nVars )
            return v;
    }
    return nVars;
}


/**Function*************************************************************

  Synopsis    [Detecting elementary functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_TtOnlyOneOne( word t )
{
    if ( t == 0 )
        return 0;
    return (t & (t-1)) == 0;
}
static inline int Abc_Tt6IsAndType( word t, int nVars )
{
    return Abc_TtOnlyOneOne( t & Abc_Tt6Mask(1 << nVars) );
}
static inline int Abc_Tt6IsOrType( word t, int nVars )
{
    return Abc_TtOnlyOneOne( ~t & Abc_Tt6Mask(1 << nVars) );
}
static inline int Abc_Tt6IsXorType( word t, int nVars )
{
    return ((((t & 1) ? ~t : t) ^ s_TruthXors[nVars]) & Abc_Tt6Mask(1 << nVars)) == 0;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Abc_Tt6Flip( word Truth, int iVar )
{
    return Truth = ((Truth << (1 << iVar)) & s_Truths6[iVar]) | ((Truth & s_Truths6[iVar]) >> (1 << iVar));
}
static inline void Abc_TtFlip( word * pTruth, int nWords, int iVar )
{
    if ( nWords == 1 )
        pTruth[0] = ((pTruth[0] << (1 << iVar)) & s_Truths6[iVar]) | ((pTruth[0] & s_Truths6[iVar]) >> (1 << iVar));
    else if ( iVar <= 5 )
    {
        int w, shift = (1 << iVar);
        for ( w = 0; w < nWords; w++ )
            pTruth[w] = ((pTruth[w] << shift) & s_Truths6[iVar]) | ((pTruth[w] & s_Truths6[iVar]) >> shift);
    }
    else // if ( iVar > 5 )
    {
        word * pLimit = pTruth + nWords;
        int i, iStep = Abc_TtWordNum(iVar);
        for ( ; pTruth < pLimit; pTruth += 2*iStep )
            for ( i = 0; i < iStep; i++ )
                ABC_SWAP( word, pTruth[i], pTruth[i + iStep] );
    }    
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Abc_Tt6Permute_rec( word t, int * pPerm, int nVars )
{
    word uRes0, uRes1; int Var;
    if (  t == 0 ) return 0;
    if ( ~t == 0 ) return ~(word)0;
    for ( Var = nVars-1; Var >= 0; Var-- )
        if ( Abc_Tt6HasVar( t, Var ) )
             break;
    assert( Var >= 0 );
    uRes0 = Abc_Tt6Permute_rec( Abc_Tt6Cofactor0(t, Var), pPerm, Var );
    uRes1 = Abc_Tt6Permute_rec( Abc_Tt6Cofactor1(t, Var), pPerm, Var );
    return (uRes0 & s_Truths6Neg[pPerm[Var]]) | (uRes1 & s_Truths6[pPerm[Var]]);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Abc_Tt6SwapAdjacent( word Truth, int iVar )
{
    return (Truth & s_PMasks[iVar][0]) | ((Truth & s_PMasks[iVar][1]) << (1 << iVar)) | ((Truth & s_PMasks[iVar][2]) >> (1 << iVar));
}
static inline void Abc_TtSwapAdjacent( word * pTruth, int nWords, int iVar )
{
    if ( iVar < 5 )
    {
        int i, Shift = (1 << iVar);
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & s_PMasks[iVar][0]) | ((pTruth[i] & s_PMasks[iVar][1]) << Shift) | ((pTruth[i] & s_PMasks[iVar][2]) >> Shift);
    }
    else if ( iVar == 5 )
    {
        unsigned * pTruthU = (unsigned *)pTruth;
        unsigned * pLimitU = (unsigned *)(pTruth + nWords);
        for ( ; pTruthU < pLimitU; pTruthU += 4 )
            ABC_SWAP( unsigned, pTruthU[1], pTruthU[2] );
    }
    else // if ( iVar > 5 )
    {
        word * pLimit = pTruth + nWords;
        int i, iStep = Abc_TtWordNum(iVar);
        for ( ; pTruth < pLimit; pTruth += 4*iStep )
            for ( i = 0; i < iStep; i++ )
                ABC_SWAP( word, pTruth[i + iStep], pTruth[i + 2*iStep] );
    }
}
static inline word Abc_Tt6SwapVars( word t, int iVar, int jVar )
{
    word * s_PMasks = s_PPMasks[iVar][jVar];
    int shift = (1 << jVar) - (1 << iVar);
    assert( iVar < jVar );
    return (t & s_PMasks[0]) | ((t & s_PMasks[1]) << shift) | ((t & s_PMasks[2]) >> shift);
}
static inline void Abc_TtSwapVars( word * pTruth, int nVars, int iVar, int jVar )
{
    if ( iVar == jVar )
        return;
    if ( jVar < iVar )
        ABC_SWAP( int, iVar, jVar );
    assert( iVar < jVar && jVar < nVars );
    if ( nVars <= 6 )
    {
        pTruth[0] = Abc_Tt6SwapVars( pTruth[0], iVar, jVar );
        return;
    }
    if ( jVar <= 5 )
    {
        word * s_PMasks = s_PPMasks[iVar][jVar];
        int nWords = Abc_TtWordNum(nVars);
        int w, shift = (1 << jVar) - (1 << iVar);
        for ( w = 0; w < nWords; w++ )
            pTruth[w] = (pTruth[w] & s_PMasks[0]) | ((pTruth[w] & s_PMasks[1]) << shift) | ((pTruth[w] & s_PMasks[2]) >> shift);
        return;
    }
    if ( iVar <= 5 && jVar > 5 )
    {
        word low2High, high2Low;
        word * pLimit = pTruth + Abc_TtWordNum(nVars);
        int j, jStep = Abc_TtWordNum(jVar);
        int shift = 1 << iVar;
        for ( ; pTruth < pLimit; pTruth += 2*jStep )
            for ( j = 0; j < jStep; j++ )
            {
                low2High = (pTruth[j] & s_Truths6[iVar]) >> shift;
                high2Low = (pTruth[j+jStep] << shift) & s_Truths6[iVar];
                pTruth[j] = (pTruth[j] & ~s_Truths6[iVar]) | high2Low;
                pTruth[j+jStep] = (pTruth[j+jStep] & s_Truths6[iVar]) | low2High;
            }
        return;
    }
    {
        word * pLimit = pTruth + Abc_TtWordNum(nVars);
        int i, iStep = Abc_TtWordNum(iVar);
        int j, jStep = Abc_TtWordNum(jVar);
        for ( ; pTruth < pLimit; pTruth += 2*jStep )
            for ( i = 0; i < jStep; i += 2*iStep )
                for ( j = 0; j < iStep; j++ )
                    ABC_SWAP( word, pTruth[iStep + i + j], pTruth[jStep + i + j] );
        return;
    }    
}
// moves one var (v) to the given position (p)
static inline void Abc_TtMoveVar( word * pF, int nVars, int * V2P, int * P2V, int v, int p )
{
    int iVar = V2P[v], jVar = p;
    if ( iVar == jVar )
        return;
    Abc_TtSwapVars( pF, nVars, iVar, jVar );
    V2P[P2V[iVar]] = jVar;
    V2P[P2V[jVar]] = iVar;
    P2V[iVar] ^= P2V[jVar];
    P2V[jVar] ^= P2V[iVar];
    P2V[iVar] ^= P2V[jVar];
}
static inline word Abc_Tt6RemoveVar( word t, int iVar )
{
    assert( !Abc_Tt6HasVar(t, iVar) );
    while ( iVar < 5 )
        t = Abc_Tt6SwapAdjacent( t, iVar++ );
    return t;
}

/**Function*************************************************************

  Synopsis    [Support minimization.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_TtShrink( word * pF, int nVars, int nVarsAll, unsigned Phase )
{
    int i, k, Var = 0;
    assert( nVarsAll <= 16 );
    for ( i = 0; i < nVarsAll; i++ )
        if ( Phase & (1 << i) )
        {
            for ( k = i-1; k >= Var; k-- )
                Abc_TtSwapAdjacent( pF, Abc_TtWordNum(nVarsAll), k );
            Var++;
        }
    assert( Var == nVars );
}
static inline int Abc_TtMinimumBase( word * t, int * pSupp, int nVarsAll, int * pnVars )
{
    int v, iVar = 0, uSupp = 0;
    assert( nVarsAll <= 16 );
    for ( v = 0; v < nVarsAll; v++ )
        if ( Abc_TtHasVar( t, nVarsAll, v ) )
        {
            uSupp |= (1 << v);
            if ( pSupp )
                pSupp[iVar] = pSupp[v];
            iVar++;
        }
    if ( pnVars )
        *pnVars = iVar;
    if ( uSupp == 0 || Abc_TtSuppIsMinBase( uSupp ) )
        return 0;
    Abc_TtShrink( t, iVar, nVarsAll, uSupp );
    return 1;
}
static inline int Abc_TtSimplify( word * t, int * pLits, int nVarsAll, int * pnVars )
{
    int v, u;
    for ( v = 0; v < nVarsAll; v++ )
    {
        if ( pLits[v] == 0 )
            Abc_TtCofactor0( t, Abc_TtWordNum(nVarsAll), v );
        else if ( pLits[v] == 1 )
            Abc_TtCofactor1( t, Abc_TtWordNum(nVarsAll), v );
    }
    for ( v = 0;   v < nVarsAll; v++ )
    for ( u = v+1; u < nVarsAll; u++ )
        if ( Abc_Lit2Var(pLits[v]) == Abc_Lit2Var(pLits[u]) )
        {
            assert( nVarsAll <= 6 );
            if ( pLits[v] == pLits[u] )
            {
                word t0 = Abc_Tt6Cofactor0(Abc_Tt6Cofactor0(*t, v), u);
                word t1 = Abc_Tt6Cofactor1(Abc_Tt6Cofactor1(*t, v), u);
                *t = (t0 & s_Truths6Neg[v]) | (t1 & s_Truths6[v]);
            }
            else // if ( pLits[v] == Abc_LitNot(pLits[u]) )
            {
                word t0 = Abc_Tt6Cofactor1(Abc_Tt6Cofactor0(*t, v), u);
                word t1 = Abc_Tt6Cofactor0(Abc_Tt6Cofactor1(*t, v), u);
                *t = (t0 & s_Truths6Neg[v]) | (t1 & s_Truths6[v]);
            }
        }
    return Abc_TtMinimumBase( t, pLits, nVarsAll, pnVars );
}

/**Function*************************************************************

  Synopsis    [Cut minimization.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Abc_Tt6Expand( word t, int * pCut0, int nCutSize0, int * pCut, int nCutSize )
{
    int i, k;
    for ( i = nCutSize - 1, k = nCutSize0 - 1; i >= 0 && k >= 0; i-- )
    {
        if ( pCut[i] > pCut0[k] )
            continue;
        assert( pCut[i] == pCut0[k] );
        if ( k < i )
            t = Abc_Tt6SwapVars( t, k, i );
        k--;
    }
    assert( k == -1 );
    return t;
}
static inline void Abc_TtExpand( word * pTruth0, int nVars, int * pCut0, int nCutSize0, int * pCut, int nCutSize )
{
    int i, k;
    for ( i = nCutSize - 1, k = nCutSize0 - 1; i >= 0 && k >= 0; i-- )
    {
        if ( pCut[i] > pCut0[k] )
            continue;
        assert( pCut[i] == pCut0[k] );
        if ( k < i )
            Abc_TtSwapVars( pTruth0, nVars, k, i );
        k--;
    }
    assert( k == -1 );
}
static inline int Abc_Tt6MinBase( word * pTruth, int * pVars, int nVars ) 
{
    word t = *pTruth;
    int i, k;
    for ( i = k = 0; i < nVars; i++ )
    {
        if ( !Abc_Tt6HasVar( t, i ) )
            continue;
        if ( k < i )
        {
            if ( pVars ) pVars[k] = pVars[i];
            t = Abc_Tt6SwapVars( t, k, i );
        }
        k++;
    }
    if ( k == nVars )
        return k;
    assert( k < nVars );
    *pTruth = t;
    return k;
}
static inline int Abc_TtMinBase( word * pTruth, int * pVars, int nVars, int nVarsAll ) 
{
    int i, k;
    assert( nVars <= nVarsAll );
    for ( i = k = 0; i < nVars; i++ )
    {
        if ( !Abc_TtHasVar( pTruth, nVarsAll, i ) )
            continue;
        if ( k < i )
        {
            if ( pVars ) pVars[k] = pVars[i];
            Abc_TtSwapVars( pTruth, nVarsAll, k, i );
        }
        k++;
    }
    if ( k == nVars )
        return k;
    assert( k < nVars );
//    assert( k == Abc_TtSupportSize(pTruth, nVars) );
    return k;
}

/**Function*************************************************************

  Synopsis    [Implemeting given NPN config.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_TtImplementNpnConfig( word * pTruth, int nVars, char * pCanonPerm, unsigned uCanonPhase )
{
    int i, k, nWords = Abc_TtWordNum( nVars );
    if ( (uCanonPhase >> nVars) & 1 )
        Abc_TtNot( pTruth, nWords );
    for ( i = 0; i < nVars; i++ )
        if ( (uCanonPhase >> i) & 1 )
            Abc_TtFlip( pTruth, nWords, i );
    if ( pCanonPerm )
    for ( i = 0; i < nVars; i++ )
    {
        for ( k = i; k < nVars; k++ )
            if ( pCanonPerm[k] == i )
                break;
        assert( k < nVars );
        if ( i == k )
            continue;
        Abc_TtSwapVars( pTruth, nVars, i, k );
        ABC_SWAP( int, pCanonPerm[i], pCanonPerm[k] );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_TtCountOnesSlow( word t )
{
    t =    (t & ABC_CONST(0x5555555555555555)) + ((t>> 1) & ABC_CONST(0x5555555555555555));
    t =    (t & ABC_CONST(0x3333333333333333)) + ((t>> 2) & ABC_CONST(0x3333333333333333));
    t =    (t & ABC_CONST(0x0F0F0F0F0F0F0F0F)) + ((t>> 4) & ABC_CONST(0x0F0F0F0F0F0F0F0F));
    t =    (t & ABC_CONST(0x00FF00FF00FF00FF)) + ((t>> 8) & ABC_CONST(0x00FF00FF00FF00FF));
    t =    (t & ABC_CONST(0x0000FFFF0000FFFF)) + ((t>>16) & ABC_CONST(0x0000FFFF0000FFFF));
    return (t & ABC_CONST(0x00000000FFFFFFFF)) +  (t>>32);
}
static inline int Abc_TtCountOnes( word x )
{
    x = x - ((x >> 1) & ABC_CONST(0x5555555555555555));   
    x = (x & ABC_CONST(0x3333333333333333)) + ((x >> 2) & ABC_CONST(0x3333333333333333));    
    x = (x + (x >> 4)) & ABC_CONST(0x0F0F0F0F0F0F0F0F);    
    x = x + (x >> 8);
    x = x + (x >> 16);
    x = x + (x >> 32); 
    return (int)(x & 0xFF);
}
static inline int Abc_TtCountOnesVec( word * x, int nWords )
{
    int w, Count = 0;
    for ( w = 0; w < nWords; w++ )
        Count += Abc_TtCountOnes( x[w] );
    return Count;
}
static inline int Abc_TtCountOnesVecMask( word * x, word * pMask, int nWords, int fCompl )
{
    int w, Count = 0;
    if ( fCompl )
        for ( w = 0; w < nWords; w++ )
            Count += Abc_TtCountOnes( pMask[w] & ~x[w] );
    else
        for ( w = 0; w < nWords; w++ )
            Count += Abc_TtCountOnes( pMask[w] & x[w] );
    return Count;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_Tt6FirstBit( word t )
{
    int n = 0;
    if ( t == 0 ) return -1;
    if ( (t & ABC_CONST(0x00000000FFFFFFFF)) == 0 ) { n += 32; t >>= 32; }
    if ( (t & ABC_CONST(0x000000000000FFFF)) == 0 ) { n += 16; t >>= 16; }
    if ( (t & ABC_CONST(0x00000000000000FF)) == 0 ) { n +=  8; t >>=  8; }
    if ( (t & ABC_CONST(0x000000000000000F)) == 0 ) { n +=  4; t >>=  4; }
    if ( (t & ABC_CONST(0x0000000000000003)) == 0 ) { n +=  2; t >>=  2; }
    if ( (t & ABC_CONST(0x0000000000000001)) == 0 ) { n++; }
    return n;
}
static inline int Abc_Tt6LastBit( word t )
{
    int n = 0;
    if ( t == 0 ) return -1;
    if ( (t & ABC_CONST(0xFFFFFFFF00000000)) == 0 ) { n += 32; t <<= 32; }
    if ( (t & ABC_CONST(0xFFFF000000000000)) == 0 ) { n += 16; t <<= 16; }
    if ( (t & ABC_CONST(0xFF00000000000000)) == 0 ) { n +=  8; t <<=  8; }
    if ( (t & ABC_CONST(0xF000000000000000)) == 0 ) { n +=  4; t <<=  4; }
    if ( (t & ABC_CONST(0xC000000000000000)) == 0 ) { n +=  2; t <<=  2; }
    if ( (t & ABC_CONST(0x8000000000000000)) == 0 ) { n++; }
    return 63-n;
}
static inline int Abc_TtFindFirstBit( word * pIn, int nVars )
{
    int w, nWords = Abc_TtWordNum(nVars);
    for ( w = 0; w < nWords; w++ )
        if ( pIn[w] )
            return 64*w + Abc_Tt6FirstBit(pIn[w]);
    return -1;
}
static inline int Abc_TtFindFirstBit2( word * pIn, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        if ( pIn[w] )
            return 64*w + Abc_Tt6FirstBit(pIn[w]);
    return -1;
}
static inline int Abc_TtFindLastBit( word * pIn, int nVars )
{
    int w, nWords = Abc_TtWordNum(nVars);
    for ( w = nWords - 1; w >= 0; w-- )
        if ( pIn[w] )
            return 64*w + Abc_Tt6LastBit(pIn[w]);
    return -1;
}
static inline int Abc_TtFindLastBit2( word * pIn, int nWords )
{
    int w;
    for ( w = nWords - 1; w >= 0; w-- )
        if ( pIn[w] )
            return 64*w + Abc_Tt6LastBit(pIn[w]);
    return -1;
}
static inline int Abc_TtFindFirstDiffBit( word * pIn1, word * pIn2, int nVars )
{
    int w, nWords = Abc_TtWordNum(nVars);
    for ( w = 0; w < nWords; w++ )
        if ( pIn1[w] ^ pIn2[w] )
            return 64*w + Abc_Tt6FirstBit(pIn1[w] ^ pIn2[w]);
    return -1;
}
static inline int Abc_TtFindFirstDiffBit2( word * pIn1, word * pIn2, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        if ( pIn1[w] ^ pIn2[w] )
            return 64*w + Abc_Tt6FirstBit(pIn1[w] ^ pIn2[w]);
    return -1;
}
static inline int Abc_TtFindLastDiffBit( word * pIn1, word * pIn2, int nVars )
{
    int w, nWords = Abc_TtWordNum(nVars);
    for ( w = nWords - 1; w >= 0; w-- )
        if ( pIn1[w] ^ pIn2[w] )
            return 64*w + Abc_Tt6LastBit(pIn1[w] ^ pIn2[w]);
    return -1;
}
static inline int Abc_TtFindLastDiffBit2( word * pIn1, word * pIn2, int nWords )
{
    int w;
    for ( w = nWords - 1; w >= 0; w-- )
        if ( pIn1[w] ^ pIn2[w] )
            return 64*w + Abc_Tt6LastBit(pIn1[w] ^ pIn2[w]);
    return -1;
}
static inline int Abc_TtFindFirstZero( word * pIn, int nVars )
{
    int w, nWords = Abc_TtWordNum(nVars);
    for ( w = 0; w < nWords; w++ )
        if ( ~pIn[w] )
            return 64*w + Abc_Tt6FirstBit(~pIn[w]);
    return -1;
}
static inline int Abc_TtFindLastZero( word * pIn, int nVars )
{
    int w, nWords = Abc_TtWordNum(nVars);
    for ( w = nWords - 1; w >= 0; w-- )
        if ( ~pIn[w] )
            return 64*w + Abc_Tt6LastBit(~pIn[w]);
    return -1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_TtReverseVars( word * pTruth, int nVars )
{
    int k;
    for ( k = 0; k < nVars/2 ; k++ )
        Abc_TtSwapVars( pTruth, nVars, k, nVars - 1 - k );
}
static inline void Abc_TtReverseBits( word * pTruth, int nVars )
{
    static unsigned char pMirror[256] = {
          0, 128,  64, 192,  32, 160,  96, 224,  16, 144,  80, 208,  48, 176, 112, 240,
          8, 136,  72, 200,  40, 168, 104, 232,  24, 152,  88, 216,  56, 184, 120, 248,
          4, 132,  68, 196,  36, 164, 100, 228,  20, 148,  84, 212,  52, 180, 116, 244,
         12, 140,  76, 204,  44, 172, 108, 236,  28, 156,  92, 220,  60, 188, 124, 252,
          2, 130,  66, 194,  34, 162,  98, 226,  18, 146,  82, 210,  50, 178, 114, 242,
         10, 138,  74, 202,  42, 170, 106, 234,  26, 154,  90, 218,  58, 186, 122, 250,
          6, 134,  70, 198,  38, 166, 102, 230,  22, 150,  86, 214,  54, 182, 118, 246,
         14, 142,  78, 206,  46, 174, 110, 238,  30, 158,  94, 222,  62, 190, 126, 254,
          1, 129,  65, 193,  33, 161,  97, 225,  17, 145,  81, 209,  49, 177, 113, 241,
          9, 137,  73, 201,  41, 169, 105, 233,  25, 153,  89, 217,  57, 185, 121, 249,
          5, 133,  69, 197,  37, 165, 101, 229,  21, 149,  85, 213,  53, 181, 117, 245,
         13, 141,  77, 205,  45, 173, 109, 237,  29, 157,  93, 221,  61, 189, 125, 253,
          3, 131,  67, 195,  35, 163,  99, 227,  19, 147,  83, 211,  51, 179, 115, 243,
         11, 139,  75, 203,  43, 171, 107, 235,  27, 155,  91, 219,  59, 187, 123, 251,
          7, 135,  71, 199,  39, 167, 103, 231,  23, 151,  87, 215,  55, 183, 119, 247,
         15, 143,  79, 207,  47, 175, 111, 239,  31, 159,  95, 223,  63, 191, 127, 255
    };
    unsigned char Temp, * pTruthC = (unsigned char *)pTruth;
    int i, nBytes = (nVars > 6) ? (1 << (nVars - 3)) : 8;
    for ( i = 0; i < nBytes/2; i++ )
    {
        Temp = pMirror[pTruthC[i]];
        pTruthC[i] = pMirror[pTruthC[nBytes-1-i]];
        pTruthC[nBytes-1-i] = Temp;
    }
}


/**Function*************************************************************

  Synopsis    [Checks unateness of a function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_Tt6PosVar( word t, int iVar )
{
    return ((t >> (1<<iVar)) & t & s_Truths6Neg[iVar]) == (t & s_Truths6Neg[iVar]);
}
static inline int Abc_Tt6NegVar( word t, int iVar )
{
    return ((t << (1<<iVar)) & t & s_Truths6[iVar]) == (t & s_Truths6[iVar]);
}
static inline int Abc_TtPosVar( word * t, int nVars, int iVar )
{
    assert( iVar < nVars );
    if ( nVars <= 6 )
        return Abc_Tt6PosVar( t[0], iVar );
    if ( iVar < 6 )
    {
        int i, Shift = (1 << iVar);
        int nWords = Abc_TtWordNum( nVars );
        for ( i = 0; i < nWords; i++ )
            if ( ((t[i] >> Shift) & t[i] & s_Truths6Neg[iVar]) != (t[i] & s_Truths6Neg[iVar]) )
                return 0;
        return 1;
    }
    else
    {
        int i, Step = (1 << (iVar - 6));
        word * tLimit = t + Abc_TtWordNum( nVars );
        for ( ; t < tLimit; t += 2*Step )
            for ( i = 0; i < Step; i++ )
                if ( t[i] != (t[i] & t[Step+i]) )
                    return 0;
        return 1;
    }
}
static inline int Abc_TtNegVar( word * t, int nVars, int iVar )
{
    assert( iVar < nVars );
    if ( nVars <= 6 )
        return Abc_Tt6NegVar( t[0], iVar );
    if ( iVar < 6 )
    {
        int i, Shift = (1 << iVar);
        int nWords = Abc_TtWordNum( nVars );
        for ( i = 0; i < nWords; i++ )
            if ( ((t[i] << Shift) & t[i] & s_Truths6[iVar]) != (t[i] & s_Truths6[iVar]) )
                return 0;
        return 1;
    }
    else
    {
        int i, Step = (1 << (iVar - 6));
        word * tLimit = t + Abc_TtWordNum( nVars );
        for ( ; t < tLimit; t += 2*Step )
            for ( i = 0; i < Step; i++ )
                if ( (t[i] & t[Step+i]) != t[Step+i] )
                    return 0;
        return 1;
    }
}
static inline int Abc_TtIsUnate( word * t, int nVars )
{
    int i;
    for ( i = 0; i < nVars; i++ )
        if ( !Abc_TtNegVar(t, nVars, i) && !Abc_TtPosVar(t, nVars, i) )
            return 0;
    return 1;
}
static inline int Abc_TtIsPosUnate( word * t, int nVars )
{
    int i;
    for ( i = 0; i < nVars; i++ )
        if ( !Abc_TtPosVar(t, nVars, i) )
            return 0;
    return 1;
}
static inline void Abc_TtMakePosUnate( word * t, int nVars )
{
    int i, nWords = Abc_TtWordNum(nVars);
    for ( i = 0; i < nVars; i++ )
        if ( Abc_TtNegVar(t, nVars, i) )
            Abc_TtFlip( t, nWords, i );
        else assert( Abc_TtPosVar(t, nVars, i) );
}


/**Function*************************************************************

  Synopsis    [Computes ISOP for 6 variables or less.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Abc_Tt6Isop( word uOn, word uOnDc, int nVars, int * pnCubes )
{
    word uOn0, uOn1, uOnDc0, uOnDc1, uRes0, uRes1, uRes2;
    int Var;
    assert( nVars <= 6 );
    assert( (uOn & ~uOnDc) == 0 );
    if ( uOn == 0 )
        return 0;
    if ( uOnDc == ~(word)0 )
    {
        (*pnCubes)++;
        return ~(word)0;
    }
    assert( nVars > 0 );
    // find the topmost var
    for ( Var = nVars-1; Var >= 0; Var-- )
        if ( Abc_Tt6HasVar( uOn, Var ) || Abc_Tt6HasVar( uOnDc, Var ) )
             break;
    assert( Var >= 0 );
    // cofactor
    uOn0   = Abc_Tt6Cofactor0( uOn,   Var );
    uOn1   = Abc_Tt6Cofactor1( uOn  , Var );
    uOnDc0 = Abc_Tt6Cofactor0( uOnDc, Var );
    uOnDc1 = Abc_Tt6Cofactor1( uOnDc, Var );
    // solve for cofactors
    uRes0 = Abc_Tt6Isop( uOn0 & ~uOnDc1, uOnDc0, Var, pnCubes );
    uRes1 = Abc_Tt6Isop( uOn1 & ~uOnDc0, uOnDc1, Var, pnCubes );
    uRes2 = Abc_Tt6Isop( (uOn0 & ~uRes0) | (uOn1 & ~uRes1), uOnDc0 & uOnDc1, Var, pnCubes );
    // derive the final truth table
    uRes2 |= (uRes0 & s_Truths6Neg[Var]) | (uRes1 & s_Truths6[Var]);
    assert( (uOn & ~uRes2) == 0 );
    assert( (uRes2 & ~uOnDc) == 0 );
    return uRes2;
}
static inline int Abc_Tt7Isop( word uOn[2], word uOnDc[2], int nVars, word uRes[2] )
{
    int nCubes = 0;
    if ( nVars <= 6 || (uOn[0] == uOn[1] && uOnDc[0] == uOnDc[1]) )
        uRes[0] = uRes[1] = Abc_Tt6Isop( uOn[0], uOnDc[0], Abc_MinInt(nVars, 6), &nCubes );
    else
    {
        word uRes0, uRes1, uRes2;
        assert( nVars == 7 );
        // solve for cofactors
        uRes0 = Abc_Tt6Isop( uOn[0] & ~uOnDc[1], uOnDc[0], 6, &nCubes );
        uRes1 = Abc_Tt6Isop( uOn[1] & ~uOnDc[0], uOnDc[1], 6, &nCubes );
        uRes2 = Abc_Tt6Isop( (uOn[0] & ~uRes0) | (uOn[1] & ~uRes1), uOnDc[0] & uOnDc[1], 6, &nCubes );
        // derive the final truth table
        uRes[0] = uRes2 | uRes0;
        uRes[1] = uRes2 | uRes1;
        assert( (uOn[0] & ~uRes[0]) == 0 && (uOn[1] & ~uRes[1]) == 0 );
        assert( (uRes[0] & ~uOnDc[0])==0 && (uRes[1] & ~uOnDc[1])==0 );
    }
    return nCubes;
}
static inline int Abc_Tt8Isop( word uOn[4], word uOnDc[4], int nVars, word uRes[4] )
{
    int nCubes = 0;
    if ( nVars <= 6 )
        uRes[0] = uRes[1] = uRes[2] = uRes[3] = Abc_Tt6Isop( uOn[0], uOnDc[0], nVars, &nCubes );
    else if ( nVars == 7 || (uOn[0] == uOn[2] && uOn[1] == uOn[3] && uOnDc[0] == uOnDc[2] && uOnDc[1] == uOnDc[3]) )
    {
        nCubes = Abc_Tt7Isop( uOn, uOnDc, 7, uRes );
        uRes[2] = uRes[0];
        uRes[3] = uRes[1];
    }
    else 
    {
        word uOn0[2], uOn1[2], uOn2[2], uOnDc2[2], uRes0[2], uRes1[2], uRes2[2];
        assert( nVars == 8 );
        // cofactor
        uOn0[0] = uOn[0] & ~uOnDc[2];
        uOn0[1] = uOn[1] & ~uOnDc[3];
        uOn1[0] = uOn[2] & ~uOnDc[0];
        uOn1[1] = uOn[3] & ~uOnDc[1];
        uOnDc2[0] = uOnDc[0] & uOnDc[2];
        uOnDc2[1] = uOnDc[1] & uOnDc[3];
        // solve for cofactors
        nCubes += Abc_Tt7Isop( uOn0, uOnDc+0, 7, uRes0 );
        nCubes += Abc_Tt7Isop( uOn1, uOnDc+2, 7, uRes1 );
        uOn2[0] = (uOn[0] & ~uRes0[0]) | (uOn[2] & ~uRes1[0]);
        uOn2[1] = (uOn[1] & ~uRes0[1]) | (uOn[3] & ~uRes1[1]);
        nCubes += Abc_Tt7Isop( uOn2, uOnDc2, 7, uRes2 );
        // derive the final truth table
        uRes[0] = uRes2[0] | uRes0[0];
        uRes[1] = uRes2[1] | uRes0[1];
        uRes[2] = uRes2[0] | uRes1[0];
        uRes[3] = uRes2[1] | uRes1[1];
        assert( (uOn[0] & ~uRes[0]) == 0 && (uOn[1] & ~uRes[1]) == 0 && (uOn[2] & ~uRes[2]) == 0 && (uOn[3] & ~uRes[3]) == 0 );
        assert( (uRes[0] & ~uOnDc[0])==0 && (uRes[1] & ~uOnDc[1])==0 && (uRes[2] & ~uOnDc[2])==0 && (uRes[3] & ~uOnDc[3])==0 );
    }
    return nCubes;
}

/**Function*************************************************************

  Synopsis    [Computes CNF size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_Tt6CnfSize( word t, int nVars )
{
    int nCubes = 0;
    Abc_Tt6Isop(  t,  t, nVars, &nCubes );
    Abc_Tt6Isop( ~t, ~t, nVars, &nCubes );
    assert( nCubes <= 64 );
    return nCubes;
}
static inline int Abc_Tt8CnfSize( word t[4], int nVars )
{
    word uRes[4], tc[4] = {~t[0], ~t[1], ~t[2], ~t[3]};
    int nCubes = 0;
    nCubes += Abc_Tt8Isop( t,  t,  nVars, uRes );
    nCubes += Abc_Tt8Isop( tc, tc, nVars, uRes );
    assert( nCubes <= 256 );
    return nCubes;
}

/**Function*************************************************************

  Synopsis    [Derives ISOP cover for the function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Abc_Tt6IsopCover( word uOn, word uOnDc, int nVars, int * pCover, int * pnCubes )
{
    word uOn0, uOn1, uOnDc0, uOnDc1, uRes0, uRes1, uRes2;
    int c, Var, nBeg0, nEnd0, nEnd1;
    assert( nVars <= 6 );
    assert( (uOn & ~uOnDc) == 0 );
    if ( uOn == 0 )
        return 0;
    if ( uOnDc == ~(word)0 )
    {
        pCover[(*pnCubes)++] = 0;
        return ~(word)0;
    }
    assert( nVars > 0 );
    // find the topmost var
    for ( Var = nVars-1; Var >= 0; Var-- )
        if ( Abc_Tt6HasVar( uOn, Var ) || Abc_Tt6HasVar( uOnDc, Var ) )
             break;
    assert( Var >= 0 );
    // cofactor
    uOn0   = Abc_Tt6Cofactor0( uOn,   Var );
    uOn1   = Abc_Tt6Cofactor1( uOn  , Var );
    uOnDc0 = Abc_Tt6Cofactor0( uOnDc, Var );
    uOnDc1 = Abc_Tt6Cofactor1( uOnDc, Var );
    // solve for cofactors
    nBeg0 = *pnCubes; 
    uRes0 = Abc_Tt6IsopCover( uOn0 & ~uOnDc1, uOnDc0, Var, pCover, pnCubes );
    nEnd0 = *pnCubes;
    uRes1 = Abc_Tt6IsopCover( uOn1 & ~uOnDc0, uOnDc1, Var, pCover, pnCubes );
    nEnd1 = *pnCubes;
    uRes2 = Abc_Tt6IsopCover( (uOn0 & ~uRes0) | (uOn1 & ~uRes1), uOnDc0 & uOnDc1, Var, pCover, pnCubes );
    // derive the final truth table
    uRes2 |= (uRes0 & s_Truths6Neg[Var]) | (uRes1 & s_Truths6[Var]);
    for ( c = nBeg0; c < nEnd0; c++ )
        pCover[c] |= (1 << (2*Var+0));
    for ( c = nEnd0; c < nEnd1; c++ )
        pCover[c] |= (1 << (2*Var+1));
    assert( (uOn & ~uRes2) == 0 );
    assert( (uRes2 & ~uOnDc) == 0 );
    return uRes2;
}
static inline void Abc_Tt7IsopCover( word uOn[2], word uOnDc[2], int nVars, word uRes[2], int * pCover, int * pnCubes )
{
    if ( nVars <= 6 || (uOn[0] == uOn[1] && uOnDc[0] == uOnDc[1]) )
        uRes[0] = uRes[1] = Abc_Tt6IsopCover( uOn[0], uOnDc[0], Abc_MinInt(nVars, 6), pCover, pnCubes );
    else
    {
        word uRes0, uRes1, uRes2;
        int c, nBeg0, nEnd0, nEnd1;
        assert( nVars == 7 );
        // solve for cofactors
        nBeg0 = *pnCubes; 
        uRes0 = Abc_Tt6IsopCover( uOn[0] & ~uOnDc[1], uOnDc[0], 6, pCover, pnCubes );   
        nEnd0 = *pnCubes;
        uRes1 = Abc_Tt6IsopCover( uOn[1] & ~uOnDc[0], uOnDc[1], 6, pCover, pnCubes );   
        nEnd1 = *pnCubes;
        uRes2 = Abc_Tt6IsopCover( (uOn[0] & ~uRes0) | (uOn[1] & ~uRes1), uOnDc[0] & uOnDc[1], 6, pCover, pnCubes );
        // derive the final truth table
        uRes[0] = uRes2 | uRes0;
        uRes[1] = uRes2 | uRes1;
        for ( c = nBeg0; c < nEnd0; c++ )
            pCover[c] |= (1 << (2*6+0));
        for ( c = nEnd0; c < nEnd1; c++ )
            pCover[c] |= (1 << (2*6+1));
        assert( (uOn[0] & ~uRes[0]) == 0 && (uOn[1] & ~uRes[1]) == 0 );
        assert( (uRes[0] & ~uOnDc[0])==0 && (uRes[1] & ~uOnDc[1])==0 );
    }
}
static inline void Abc_Tt8IsopCover( word uOn[4], word uOnDc[4], int nVars, word uRes[4], int * pCover, int * pnCubes )
{
    if ( nVars <= 6 )
        uRes[0] = uRes[1] = uRes[2] = uRes[3] = Abc_Tt6IsopCover( uOn[0], uOnDc[0], nVars, pCover, pnCubes );
    else if ( nVars == 7 || (uOn[0] == uOn[2] && uOn[1] == uOn[3] && uOnDc[0] == uOnDc[2] && uOnDc[1] == uOnDc[3]) )
    {
        Abc_Tt7IsopCover( uOn, uOnDc, 7, uRes, pCover, pnCubes );
        uRes[2] = uRes[0];
        uRes[3] = uRes[1];
    }
    else 
    {
        word uOn0[2], uOn1[2], uOn2[2], uOnDc2[2], uRes0[2], uRes1[2], uRes2[2];
        int c, nBeg0, nEnd0, nEnd1;
        assert( nVars == 8 );
        // cofactor
        uOn0[0] = uOn[0] & ~uOnDc[2];
        uOn0[1] = uOn[1] & ~uOnDc[3];
        uOn1[0] = uOn[2] & ~uOnDc[0];
        uOn1[1] = uOn[3] & ~uOnDc[1];
        uOnDc2[0] = uOnDc[0] & uOnDc[2];
        uOnDc2[1] = uOnDc[1] & uOnDc[3];
        // solve for cofactors
        nBeg0 = *pnCubes; 
        Abc_Tt7IsopCover( uOn0, uOnDc+0, 7, uRes0, pCover, pnCubes );
        nEnd0 = *pnCubes;
        Abc_Tt7IsopCover( uOn1, uOnDc+2, 7, uRes1, pCover, pnCubes );
        nEnd1 = *pnCubes;
        uOn2[0] = (uOn[0] & ~uRes0[0]) | (uOn[2] & ~uRes1[0]);
        uOn2[1] = (uOn[1] & ~uRes0[1]) | (uOn[3] & ~uRes1[1]);
        Abc_Tt7IsopCover( uOn2, uOnDc2, 7, uRes2, pCover, pnCubes );
        // derive the final truth table
        uRes[0] = uRes2[0] | uRes0[0];
        uRes[1] = uRes2[1] | uRes0[1];
        uRes[2] = uRes2[0] | uRes1[0];
        uRes[3] = uRes2[1] | uRes1[1];
        for ( c = nBeg0; c < nEnd0; c++ )
            pCover[c] |= (1 << (2*7+0));
        for ( c = nEnd0; c < nEnd1; c++ )
            pCover[c] |= (1 << (2*7+1));
        assert( (uOn[0] & ~uRes[0]) == 0 && (uOn[1] & ~uRes[1]) == 0 && (uOn[2] & ~uRes[2]) == 0 && (uOn[3] & ~uRes[3]) == 0 );
        assert( (uRes[0] & ~uOnDc[0])==0 && (uRes[1] & ~uOnDc[1])==0 && (uRes[2] & ~uOnDc[2])==0 && (uRes[3] & ~uOnDc[3])==0 );
    }
}

/**Function*************************************************************

  Synopsis    [Computes CNF for the function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_Tt6Cnf( word t, int nVars, int * pCover )
{
    int c, nCubes = 0;
    Abc_Tt6IsopCover( t, t, nVars, pCover, &nCubes );
    for ( c = 0; c < nCubes; c++ )
        pCover[c] |= (1 << (2*nVars+0));
    Abc_Tt6IsopCover( ~t, ~t, nVars, pCover, &nCubes );
    for ( ; c < nCubes; c++ )
        pCover[c] |= (1 << (2*nVars+1));
    assert( nCubes <= 64 );
    return nCubes;
}
static inline int Abc_Tt8Cnf( word t[4], int nVars, int * pCover )
{
    word uRes[4], tc[4] = {~t[0], ~t[1], ~t[2], ~t[3]};
    int c, nCubes = 0;
    Abc_Tt8IsopCover( t,  t,  nVars, uRes, pCover, &nCubes );
    for ( c = 0; c < nCubes; c++ )
        pCover[c] |= (1 << (2*nVars+0));
    Abc_Tt8IsopCover( tc, tc, nVars, uRes, pCover, &nCubes );
    for ( ; c < nCubes; c++ )
        pCover[c] |= (1 << (2*nVars+1));
    assert( nCubes <= 256 );
    return nCubes;
}


/**Function*************************************************************

  Synopsis    [Computes ISOP for 6 variables or less.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_Tt6Esop( word t, int nVars, int * pCover )
{
    word c0, c1;
    int Var, r0, r1, r2, Max, i;
    assert( nVars <= 6 );
    if ( t == 0 )
        return 0;
    if ( t == ~(word)0 )
    {
        if ( pCover ) *pCover = 0;
        return 1;
    }
    assert( nVars > 0 );
    // find the topmost var
    for ( Var = nVars-1; Var >= 0; Var-- )
        if ( Abc_Tt6HasVar( t, Var ) )
             break;
    assert( Var >= 0 );
    // cofactor
    c0 = Abc_Tt6Cofactor0( t, Var );
    c1 = Abc_Tt6Cofactor1( t, Var );
    // call recursively
    r0 = Abc_Tt6Esop( c0,      Var, pCover ? pCover : NULL );
    r1 = Abc_Tt6Esop( c1,      Var, pCover ? pCover + r0 : NULL );
    r2 = Abc_Tt6Esop( c0 ^ c1, Var, pCover ? pCover + r0 + r1 : NULL );
    Max = Abc_MaxInt( r0, Abc_MaxInt(r1, r2) );
    // add literals
    if ( pCover )
    {
        if ( Max == r0 )
        {
            for ( i = 0; i < r1; i++ )
                pCover[i] = pCover[r0+i];
            for ( i = 0; i < r2; i++ )
                pCover[r1+i] = pCover[r0+r1+i] | (1 << (2*Var+0));
        }
        else if ( Max == r1 )
        {
            for ( i = 0; i < r2; i++ )
                pCover[r0+i] = pCover[r0+r1+i] | (1 << (2*Var+1));
        }
        else
        {
            for ( i = 0; i < r0; i++ )
                pCover[i] |= (1 << (2*Var+0));
            for ( i = 0; i < r1; i++ )
                pCover[r0+i] |= (1 << (2*Var+1));
        }
    }
    return r0 + r1 + r2 - Max;
}
static inline word Abc_Tt6EsopBuild( int nVars, int * pCover, int nCubes )
{
    word p, t = 0; int c, v;
    for ( c = 0; c < nCubes; c++ )
    {
        p = ~(word)0;
        for ( v = 0; v < nVars; v++ )
            if ( ((pCover[c] >> (v << 1)) & 3) == 1 )
                p &= ~s_Truths6[v];
            else if ( ((pCover[c] >> (v << 1)) & 3) == 2 )
                p &= s_Truths6[v];
        t ^= p;
    }
    return t;
}
static inline int Abc_Tt6EsopVerify( word t, int nVars )
{
    int pCover[64];
    int nCubes = Abc_Tt6Esop( t, nVars, pCover );
    word t2 = Abc_Tt6EsopBuild( nVars, pCover, nCubes );
    if ( t != t2 )
        printf( "Verification failed.\n" );
    return nCubes;
}

/**Function*************************************************************

  Synopsis    [Check if the function is output-decomposable with the given var.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_Tt6CheckOutDec( word t, int i, word * pOut )
{
    word c0 = Abc_Tt6Cofactor0( t, i );
    word c1 = Abc_Tt6Cofactor1( t, i );
    assert( c0 != c1 );
    if ( c0 == 0 ) //  F = i * G
    {
        if ( pOut ) *pOut = c1;
        return 0;
    }
    if ( c1 == 0 ) //  F = ~i * G
    {
        if ( pOut ) *pOut = c0;
        return 1;
    }
    if ( ~c0 == 0 ) //  F = ~i + G
    {
        if ( pOut ) *pOut = c1;
        return 2;
    }
    if ( ~c1 == 0 ) //  F = i + G
    {
        if ( pOut ) *pOut = c0;
        return 3;
    }
    if ( c0 == ~c1 ) //  F = i # G
    {
        if ( pOut ) *pOut = c0;
        return 4;
    }
    return -1;
}
static inline int Abc_TtCheckOutDec( word * pTruth, int nVars, int v, word * pOut )
{
    word Cof0[4], Cof1[4];
    int nWords = Abc_TtWordNum(nVars);
    assert( nVars <= 8 );
    Abc_TtCofactor0p( Cof0, pTruth, nWords, v );
    Abc_TtCofactor1p( Cof1, pTruth, nWords, v );
    assert( !Abc_TtEqual(Cof0, Cof1, nWords) );
    if ( Abc_TtIsConst0(Cof0, nWords) ) //if ( c0 == 0 ) //  F = i * G
    {
        if ( pOut ) Abc_TtCopy( pOut, Cof1, nWords, 0 ); //*pOut = c1;
        return 0;
    }
    if ( Abc_TtIsConst0(Cof1, nWords) ) //if ( c1 == 0 ) //  F = ~i * G
    {
        if ( pOut ) Abc_TtCopy( pOut, Cof0, nWords, 0 ); //*pOut = c0;
        return 1;
    }
    if ( Abc_TtIsConst1(Cof0, nWords) ) //if ( ~c0 == 0 ) //  F = ~i + G
    {
        if ( pOut ) Abc_TtCopy( pOut, Cof1, nWords, 0 ); //*pOut = c1;
        return 2;
    }
    if ( Abc_TtIsConst1(Cof1, nWords) ) //if ( ~c1 == 0 ) //  F = i + G
    {
        if ( pOut ) Abc_TtCopy( pOut, Cof0, nWords, 0 ); //*pOut = c0;
        return 3;
    }
    if ( Abc_TtOpposite(Cof0, Cof1, nWords) ) //if ( c0 == ~c1 )  //  F = i # G
    {
        if ( pOut ) Abc_TtCopy( pOut, Cof0, nWords, 0 ); //*pOut = c0;
        return 4;
    }
    return -1;
}
static inline word Abc_TtCheckDecOutOne7( word * t, int * piVar, int * pType )
{
    int v, Type, Type2; word Out[2];
    for ( v = 6; v >= 0; v-- )
        if ( (Type = Abc_TtCheckOutDec(t, 7, v, NULL)) != -1 )
        {
            Abc_TtSwapVars( t, 7, 6, v );
            Type2 = Abc_TtCheckOutDec( t, 7, 6, Out );
            assert( Type == Type2 );
            *piVar = v;
            *pType = Type;
            return Out[0];
        }
    return 0;
}
static inline word Abc_TtCheckDecOutOne8( word * t, int * piVar1, int * piVar2, int * pType1, int * pType2 )
{
    int v, Type1, Type12, Type2, Type22; word Out[4], Out2[2];
    for ( v = 7; v >= 0; v-- )
        if ( (Type1 = Abc_TtCheckOutDec(t, 8, v, NULL)) != -1 )
        {
            Abc_TtSwapVars( t, 8, 7, v );
            Type12 = Abc_TtCheckOutDec( t, 8, 7, Out );
            assert( Type1 == Type12 );
            *piVar1 = v;
            *pType1 = Type1;
            break;
        }
    if ( v == -1 )
        return 0;
    for ( v = 6; v >= 0; v-- )
        if ( (Type2 = Abc_TtCheckOutDec(Out, 7, v, NULL)) != -1 && Abc_Lit2Var(Type2) == Abc_Lit2Var(Type1) )
        {
            Abc_TtSwapVars( Out, 7, 6, v );
            Type22 = Abc_TtCheckOutDec(Out, 7, 6, Out2);
            assert( Type2 == Type22 );
            *piVar2 = v;
            *pType2 = Type2;
            assert( *piVar2 < *piVar1 );
            return Out2[0];
        }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Check if the function is input-decomposable with the given pair.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_TtCheckDsdAnd( word t, int i, int j, word * pOut )
{
    word c0 = Abc_Tt6Cofactor0( t, i );
    word c1 = Abc_Tt6Cofactor1( t, i );
    word c00 = Abc_Tt6Cofactor0( c0, j );
    word c01 = Abc_Tt6Cofactor1( c0, j );
    word c10 = Abc_Tt6Cofactor0( c1, j );
    word c11 = Abc_Tt6Cofactor1( c1, j );
    if ( c00 == c01 && c00 == c10 ) //  i *  j
    {
        if ( pOut ) *pOut = (~s_Truths6[i] & c00) | (s_Truths6[i] & c11);
        return 0;
    }
    if ( c11 == c00 && c11 == c10 ) //  i * !j
    {
        if ( pOut ) *pOut = (~s_Truths6[i] & c11) | (s_Truths6[i] & c01);
        return 1;
    }
    if ( c11 == c00 && c11 == c01 ) // !i *  j
    {
        if ( pOut ) *pOut = (~s_Truths6[i] & c11) | (s_Truths6[i] & c10);
        return 2;
    }
    if ( c11 == c01 && c11 == c10 ) // !i * !j
    {
        if ( pOut ) *pOut = (~s_Truths6[i] & c11) | (s_Truths6[i] & c00);
        return 3;
    }
    if ( c00 == c11 && c01 == c10 )
    {
        if ( pOut ) *pOut = (~s_Truths6[i] & c11) | (s_Truths6[i] & c10);
        return 4;
    }
    return -1;
}
static inline int Abc_TtCheckDsdMux( word t, int i, word * pOut )
{
    word c0 = Abc_Tt6Cofactor0( t, i );
    word c1 = Abc_Tt6Cofactor1( t, i );
    word c00, c01, c10, c11;
    int k, fPres0, fPres1, iVar0 = -1, iVar1 = -1;
    for ( k = 0; k < 6; k++ )
    {
        if ( k == i ) continue;
        fPres0 = Abc_Tt6HasVar( c0, k );
        fPres1 = Abc_Tt6HasVar( c1, k );
        if ( fPres0 && !fPres1 )
        {
            if ( iVar0 >= 0 )
                return -1;
            iVar0 = k;
        }
        if ( !fPres0 && fPres1 )
        {
            if ( iVar1 >= 0 )
                return -1;
            iVar1 = k;
        }
    }
    if ( iVar0 == -1 || iVar1 == -1 )
        return -1;
    c00 = Abc_Tt6Cofactor0( c0, iVar0 );
    c01 = Abc_Tt6Cofactor1( c0, iVar0 );
    c10 = Abc_Tt6Cofactor0( c1, iVar1 );
    c11 = Abc_Tt6Cofactor1( c1, iVar1 );
    if ( c00 ==  c10 && c01 ==  c11 ) //  ITE(i,  iVar1,  iVar0)
    {
        if ( pOut ) *pOut = (~s_Truths6[i] & c10) | (s_Truths6[i] & c11);
        return (Abc_Var2Lit(iVar1, 0) << 16) | Abc_Var2Lit(iVar0, 0);
    }
    if ( c00 == ~c10 && c01 == ~c11 ) //  ITE(i,  iVar1, !iVar0)
    {
        if ( pOut ) *pOut = (~s_Truths6[i] & c10) | (s_Truths6[i] & c11);
        return (Abc_Var2Lit(iVar1, 0) << 16) | Abc_Var2Lit(iVar0, 1);
    }
    return -1;
}
static inline void Unm_ManCheckTest2()
{
    word t, t1, Out, Var0, Var1, Var0_, Var1_;
    int iVar0, iVar1, i, Res;
    for ( iVar0 = 0; iVar0 < 6; iVar0++ )
    for ( iVar1 = 0; iVar1 < 6; iVar1++ )
    {
        if ( iVar0 == iVar1 )
            continue;
        Var0 = s_Truths6[iVar0];
        Var1 = s_Truths6[iVar1];
        for ( i = 0; i < 5; i++ )
        {
            Var0_ = ((i >> 0) & 1) ? ~Var0 : Var0;
            Var1_ = ((i >> 1) & 1) ? ~Var1 : Var1;

            t = Var0_ & Var1_;
            if ( i == 4 )
                t = ~(Var0_ ^ Var1_);

//            Kit_DsdPrintFromTruth( (unsigned *)&t, 6 ), printf( "\n" );

            Res = Abc_TtCheckDsdAnd( t, iVar0, iVar1, &Out );
            if ( Res == -1 )
            {
                printf( "No decomposition\n" );
                continue;
            }

            Var0_ = s_Truths6[iVar0];
            Var0_ = ((Res >> 0) & 1) ? ~Var0_ : Var0_;

            Var1_ = s_Truths6[iVar1];
            Var1_ = ((Res >> 1) & 1) ? ~Var1_ : Var1_;

            t1 = Var0_ & Var1_;
            if ( Res == 4 )
                t1 = Var0_ ^ Var1_;

            t1 = (~t1 & Abc_Tt6Cofactor0(Out, iVar0)) | (t1 & Abc_Tt6Cofactor1(Out, iVar0));

//            Kit_DsdPrintFromTruth( (unsigned *)&t1, 6 ), printf( "\n" );

            if ( t1 != t )
                printf( "Verification failed.\n" );
            else
                printf( "Verification succeeded.\n" );
        }
    }
}
static inline void Unm_ManCheckTest()
{
    word t, t1, Out, Ctrl, Var0, Var1, Ctrl_, Var0_, Var1_;
    int iVar0, iVar1, iCtrl, i, Res;
    for ( iCtrl = 0; iCtrl < 6; iCtrl++ )
    for ( iVar0 = 0; iVar0 < 6; iVar0++ )
    for ( iVar1 = 0; iVar1 < 6; iVar1++ )
    {
        if ( iCtrl == iVar0 || iCtrl == iVar1 || iVar0 == iVar1 )
            continue;
        Ctrl = s_Truths6[iCtrl];
        Var0 = s_Truths6[iVar0];
        Var1 = s_Truths6[iVar1];
        for ( i = 0; i < 8; i++ )
        {
            Ctrl_ = ((i >> 0) & 1) ? ~Ctrl : Ctrl;
            Var0_ = ((i >> 1) & 1) ? ~Var0 : Var0;
            Var1_ = ((i >> 2) & 1) ? ~Var1 : Var1;

            t = (~Ctrl_ & Var0_) | (Ctrl_ & Var1_);

//            Kit_DsdPrintFromTruth( (unsigned *)&t, 6 ), printf( "\n" );

            Res = Abc_TtCheckDsdMux( t, iCtrl, &Out );
            if ( Res == -1 )
            {
                printf( "No decomposition\n" );
                continue;
            }

//            Kit_DsdPrintFromTruth( (unsigned *)&Out, 6 ), printf( "\n" );

            Ctrl_ = s_Truths6[iCtrl];
            Var0_ = s_Truths6[Abc_Lit2Var(Res & 0xFFFF)];
            Var0_ = Abc_LitIsCompl(Res & 0xFFFF) ? ~Var0_ : Var0_;

            Res >>= 16;
            Var1_ = s_Truths6[Abc_Lit2Var(Res & 0xFFFF)];
            Var1_ = Abc_LitIsCompl(Res & 0xFFFF) ? ~Var1_ : Var1_;

            t1 = (~Ctrl_ & Var0_) | (Ctrl_ & Var1_);

//            Kit_DsdPrintFromTruth( (unsigned *)&t1, 6 ), printf( "\n" );
//            Kit_DsdPrintFromTruth( (unsigned *)&Out, 6 ), printf( "\n" );

            t1 = (~t1 & Abc_Tt6Cofactor0(Out, iCtrl)) | (t1 & Abc_Tt6Cofactor1(Out, iCtrl));

//            Kit_DsdPrintFromTruth( (unsigned *)&t1, 6 ), printf( "\n" );

            if ( t1 != t )
                printf( "Verification failed.\n" );
            else
                printf( "Verification succeeded.\n" );
        }
    }
}


/**Function*************************************************************

  Synopsis    [Truth table evaluation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Abc_TtEvalLut6( word Ins[6], word Lut, int nVars )
{
    word Cube, Res = 0; int k, i;
    for ( k = 0; k < (1<<nVars); k++ )
    {
        if ( ((Lut >> k) & 1) == 0 )
            continue;
        Cube = ~(word)0;
        for ( i = 0; i < nVars; i++ )
            Cube &= ((k >> i) & 1) ? Ins[i] : ~Ins[i];
        Res |= Cube;
    }
    return Res;
}
static inline unsigned Abc_TtEvalLut5( unsigned Ins[5], int Lut, int nVars )
{
    unsigned Cube, Res = 0; int k, i;
    for ( k = 0; k < (1<<nVars); k++ )
    {
        if ( ((Lut >> k) & 1) == 0 )
            continue;
        Cube = ~(unsigned)0;
        for ( i = 0; i < nVars; i++ )
            Cube &= ((k >> i) & 1) ? Ins[i] : ~Ins[i];
        Res |= Cube;
    }
    return Res;
}
static inline int Abc_TtEvalLut4( int Ins[4], int Lut, int nVars )
{
    int Cube, Res = 0; int k, i;
    for ( k = 0; k < (1<<nVars); k++ )
    {
        if ( ((Lut >> k) & 1) == 0 )
            continue;
        Cube = ~(int)0;
        for ( i = 0; i < nVars; i++ )
            Cube &= ((k >> i) & 1) ? Ins[i] : ~Ins[i];
        Res |= Cube;
    }
    return Res & ~(~0 << (1<<nVars));
}


/**Function*************************************************************

  Synopsis    [Checks existence of bi-decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_TtComputeGraph( word * pTruth, int v, int nVars, int * pGraph )
{
    word Cof0[64], Cof1[64]; // pow( 2, nVarsMax-6 )
    word Cof00[64], Cof01[64], Cof10[64], Cof11[64];
    word CofXor, CofAndTest; 
    int i, w, nWords = Abc_TtWordNum(nVars);
    pGraph[v] |= (1 << v);
    if ( v == nVars - 1 )
        return;
    assert( v < nVars - 1 );
    Abc_TtCofactor0p( Cof0, pTruth, nWords, v );
    Abc_TtCofactor1p( Cof1, pTruth, nWords, v );
    for ( i = v + 1; i < nVars; i++ )
    {
        Abc_TtCofactor0p( Cof00, Cof0, nWords, i );
        Abc_TtCofactor1p( Cof01, Cof0, nWords, i );
        Abc_TtCofactor0p( Cof10, Cof1, nWords, i );
        Abc_TtCofactor1p( Cof11, Cof1, nWords, i );
        for ( w = 0; w < nWords; w++ )
        {
            CofXor     =  Cof00[w] ^ Cof01[w]  ^  Cof10[w] ^ Cof11[w];
            CofAndTest = (Cof00[w] & Cof01[w]) | (Cof10[w] & Cof11[w]);
            if ( CofXor & CofAndTest )
            {
                pGraph[v] |= (1 << i);
                pGraph[i] |= (1 << v);
            }
            else if ( CofXor & ~CofAndTest )
            {
                pGraph[v] |= (1 << (16+i));
                pGraph[i] |= (1 << (16+v));
            }
        }
    }
}
static inline void Abc_TtPrintVarSet( int Mask, int nVars )
{
    int i;
    for ( i = 0; i < nVars; i++ )
        if ( (Mask >> i) & 1 )
            printf( "1" );
        else
            printf( "." );
}
static inline void Abc_TtPrintBiDec( word * pTruth, int nVars )
{
    int v, pGraph[12] = {0};
    assert( nVars <= 12 );
    for ( v = 0; v < nVars; v++ )
    {
        Abc_TtComputeGraph( pTruth, v, nVars, pGraph );
        Abc_TtPrintVarSet( pGraph[v], nVars );
        printf( "    " );
        Abc_TtPrintVarSet( pGraph[v] >> 16, nVars );
        printf( "\n" );
    }
}
static inline int Abc_TtVerifyBiDec( word * pTruth, int nVars, int This, int That, int nSuppLim, word wThis, word wThat )
{
    int pVarsThis[12], pVarsThat[12], pVarsAll[12];
    int nThis = Abc_TtBitCount16(This);
    int nThat = Abc_TtBitCount16(That);
    int i, k, nWords = Abc_TtWordNum(nVars);
    word pThis[64] = {wThis}, pThat[64] = {wThat};
    assert( nVars <= 12 );
    for ( i = 0; i < nVars; i++ )
        pVarsAll[i] = i;
    for ( i = k = 0; i < nVars; i++ )
        if ( (This >> i) & 1 )
            pVarsThis[k++] = i;
    assert( k == nThis );
    for ( i = k = 0; i < nVars; i++ )
        if ( (That >> i) & 1 )
            pVarsThat[k++] = i;
    assert( k == nThat );
    Abc_TtStretch6( pThis, nThis, nVars );
    Abc_TtStretch6( pThat, nThat, nVars );
    Abc_TtExpand( pThis, nVars, pVarsThis, nThis, pVarsAll, nVars );
    Abc_TtExpand( pThat, nVars, pVarsThat, nThat, pVarsAll, nVars );
    for ( k = 0; k < nWords; k++ )
        if ( pTruth[k] != (pThis[k] & pThat[k]) )
            return 0;
    return 1;
}
static inline void Abc_TtExist( word * pTruth, int iVar, int nWords )
{
    word Cof0[64], Cof1[64]; 
    Abc_TtCofactor0p( Cof0, pTruth, nWords, iVar );
    Abc_TtCofactor1p( Cof1, pTruth, nWords, iVar );
    Abc_TtOr( pTruth, Cof0, Cof1, nWords );
}
static inline int Abc_TtCheckBiDec( word * pTruth, int nVars, int This, int That )
{
    int VarMask[2] = {This & ~That, That & ~This};
    int v, c, nWords = Abc_TtWordNum(nVars);
    word pTempR[2][64]; 
    for ( c = 0; c < 2; c++ )
    {
        Abc_TtCopy( pTempR[c], pTruth, nWords, 0 );
        for ( v = 0; v < nVars; v++ )
            if ( ((VarMask[c] >> v) & 1) )
                Abc_TtExist( pTempR[c], v, nWords );
    }
    for ( v = 0; v < nWords; v++ )
        if ( ~pTruth[v] & pTempR[0][v] & pTempR[1][v] )
            return 0;
    return 1;
}
static inline word Abc_TtDeriveBiDecOne( word * pTruth, int nVars, int This )
{
    word pTemp[64]; 
    int nThis = Abc_TtBitCount16(This);
    int v, nWords = Abc_TtWordNum(nVars);
    Abc_TtCopy( pTemp, pTruth, nWords, 0 );
    for ( v = 0; v < nVars; v++ )
        if ( !((This >> v) & 1) )
            Abc_TtExist( pTemp, v, nWords );
    Abc_TtShrink( pTemp, nThis, nVars, This );
    return Abc_Tt6Stretch( pTemp[0], nThis );
}
static inline void Abc_TtDeriveBiDec( word * pTruth, int nVars, int This, int That, int nSuppLim, word * pThis, word * pThat )
{
    assert( Abc_TtBitCount16(This) <= nSuppLim );
    assert( Abc_TtBitCount16(That) <= nSuppLim );
    pThis[0] = Abc_TtDeriveBiDecOne( pTruth, nVars, This );
    pThat[0] = Abc_TtDeriveBiDecOne( pTruth, nVars, That );
    if ( !Abc_TtVerifyBiDec(pTruth, nVars, This, That, nSuppLim, pThis[0], pThat[0] ) )
        printf( "Bi-decomposition verification failed.\n" );
}
// detect simple case of decomposition with topmost AND gate
static inline int Abc_TtCheckBiDecSimple( word * pTruth, int nVars, int nSuppLim )
{
    word Cof0[64], Cof1[64]; 
    int v, Res = 0, nDecVars = 0, nWords = Abc_TtWordNum(nVars);
    for ( v = 0; v < nVars; v++ )
    {
        Abc_TtCofactor0p( Cof0, pTruth, nWords, v );
        Abc_TtCofactor1p( Cof1, pTruth, nWords, v );
        if ( !Abc_TtIsConst0(Cof0, nWords) && !Abc_TtIsConst0(Cof1, nWords) )
            continue;
        nDecVars++;
        Res |= 1 << v;
        if ( nDecVars >= nVars - nSuppLim )
            return ((Res ^ (int)Abc_Tt6Mask(nVars)) << 16) | Res;
    }
    return 0;
}
static inline int Abc_TtProcessBiDecInt( word * pTruth, int nVars, int nSuppLim )
{
    int i, v, Res, nSupp, CountShared = 0, pGraph[12] = {0};
    assert( nSuppLim < nVars && nVars <= 2 * nSuppLim && nVars <= 12 );
    assert( 2 <= nSuppLim && nSuppLim <= 6 );
    Res = Abc_TtCheckBiDecSimple( pTruth, nVars, nSuppLim );
    if ( Res )
        return Res;
    for ( v = 0; v < nVars; v++ )
    {
        Abc_TtComputeGraph( pTruth, v, nVars, pGraph );
        nSupp = Abc_TtBitCount16(pGraph[v] & 0xFFFF);
        if ( nSupp > nSuppLim ) 
        {
            // this variable is shared - check if the limit is exceeded
            if ( ++CountShared > 2*nSuppLim - nVars )
                return 0;
        }
        else if ( nVars - nSupp <= nSuppLim )
        {
            int This = pGraph[v] & 0xFFFF;
            int That = This ^ (int)Abc_Tt6Mask(nVars);
            // find the other component
            int Graph = That;
            for ( i = 0; i < nVars; i++ )
                if ( (That >> i) & 1 )
                    Graph |= pGraph[i] & 0xFFFF;
            // check if this can be done
            if ( Abc_TtBitCount16(Graph) > nSuppLim )
                continue;
            // try decomposition
            if ( Abc_TtCheckBiDec(pTruth, nVars, This, Graph) )
                return (Graph << 16) | This;
        }
    }
    return 0;
}
static inline int Abc_TtProcessBiDec( word * pTruth, int nVars, int nSuppLim )
{
    word pFunc[64];
    int Res, nWords = Abc_TtWordNum(nVars);
    Abc_TtCopy( pFunc, pTruth, nWords, 0 );
    Res = Abc_TtProcessBiDecInt( pFunc, nVars, nSuppLim );
    if ( Res )
        return Res;
    Abc_TtCopy( pFunc, pTruth, nWords, 1 );
    Res = Abc_TtProcessBiDecInt( pFunc, nVars, nSuppLim );
    if ( Res )
        return Res | (1 << 30);
    return 0;
}

/**Function*************************************************************

  Synopsis    [Tests decomposition procedures.]

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
static inline void Abc_TtProcessBiDecTest( word * pTruth, int nVars, int nSuppLim )
{
    word This, That, pTemp[64];
    int Res, resThis, resThat;//, nThis, nThat;
    int nWords = Abc_TtWordNum(nVars);
    Abc_TtCopy( pTemp, pTruth, nWords, 0 );
    Res = Abc_TtProcessBiDec( pTemp, nVars, nSuppLim );
    if ( Res == 0 )
    {
        //Dau_DsdPrintFromTruth( pTemp, nVars );
        //printf( "Non_dec\n\n" );
        return;
    }

    resThis = Res & 0xFFFF;
    resThat = Res >> 16;

    Abc_TtDeriveBiDec( pTemp, nVars, resThis, resThat, nSuppLim, &This, &That );
    return;

    //if ( !(resThis & resThat) )
    //    return;

//    Dau_DsdPrintFromTruth( pTemp, nVars );

    //nThis = Abc_TtBitCount16(resThis);
    //nThat = Abc_TtBitCount16(resThat);

    printf( "Variable sets:    " );
    Abc_TtPrintVarSet( resThis, nVars );
    printf( "    " );
    Abc_TtPrintVarSet( resThat, nVars );
    printf( "\n" );
    Abc_TtDeriveBiDec( pTemp, nVars, resThis, resThat, nSuppLim, &This, &That );
//    Dau_DsdPrintFromTruth( &This, nThis );
//    Dau_DsdPrintFromTruth( &That, nThat );
    printf( "\n" );
}
static inline void Abc_TtProcessBiDecExperiment()
{
    int nVars = 3;
    int nSuppLim = 2;
    int Res, resThis, resThat;
    word This, That;
//    word t = ABC_CONST(0x8000000000000000);
//    word t = (s_Truths6[0] | s_Truths6[1]) & (s_Truths6[2] | s_Truths6[3] | s_Truths6[4] | s_Truths6[5]);
//    word t = ((s_Truths6[0] & s_Truths6[1]) | (~s_Truths6[1] & s_Truths6[2]));
    word t = ((s_Truths6[0] | s_Truths6[1]) & (s_Truths6[1] | s_Truths6[2]));
    Abc_TtPrintBiDec( &t, nVars );
    Res = Abc_TtProcessBiDec( &t, nVars, nSuppLim );
    resThis = Res & 0xFFFF;
    resThat = Res >> 16;
    Abc_TtDeriveBiDec( &t, nVars, resThis, resThat, nSuppLim, &This, &That );
//    Dau_DsdPrintFromTruth( &This, Abc_TtBitCount16(resThis) );
//    Dau_DsdPrintFromTruth( &That, Abc_TtBitCount16(resThat) );
    nVars = nSuppLim;
}

/**Function*************************************************************

  Synopsis    [Truth table checking procedure.]

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
static inline int Abc_Tt4Equal3( int c0, int c1, int c2, int c3 )
{
    if ( c0 == c1 && c0 == c2 ) return 3;
    if ( c0 == c1 && c0 == c3 ) return 2;
    if ( c0 == c3 && c0 == c2 ) return 1;
    if ( c3 == c1 && c3 == c2 ) return 0;
    return -1;
}
static inline int Abc_Tt4Check2( int t, int i, int j, int * f, int * r )
{
    int c0  =  t & r[j];
    int c1  = (t & f[j]) >> (1 << j);
    int c00 =  c0 & r[i];
    int c01 = (c0 & f[i]) >> (1 << i);
    int c10 =  c1 & r[i];
    int c11 = (c1 & f[i]) >> (1 << i);
    return Abc_Tt4Equal3( c00, c01, c10, c11 );
}
static inline int Abc_Tt4CheckTwoLevel( int t )
{
    int pair1, pair2;
    int f[4] = { 0xAAAA, 0xCCCC, 0xF0F0, 0xFF00 };
    int r[4] = { 0x5555, 0x3333, 0x0F0F, 0x00FF };
    if ( (pair1 = Abc_Tt4Check2(t, 0, 1, f, r)) >= 0 && (pair2 = Abc_Tt4Check2(t, 2, 3, f, r)) >= 0 ) return (1 << 4) | (pair2 << 2) | pair1;
    if ( (pair1 = Abc_Tt4Check2(t, 0, 2, f, r)) >= 0 && (pair2 = Abc_Tt4Check2(t, 1, 3, f, r)) >= 0 ) return (2 << 4) | (pair2 << 2) | pair1;
    if ( (pair1 = Abc_Tt4Check2(t, 0, 3, f, r)) >= 0 && (pair2 = Abc_Tt4Check2(t, 1, 2, f, r)) >= 0 ) return (3 << 4) | (pair2 << 2) | pair1;
    return -1;
}
static inline int Abc_Tt4CountOnes( int t )
{
    t = (t & (0x5555)) + ((t >> 1) & (0x5555));
    t = (t & (0x3333)) + ((t >> 2) & (0x3333));
    t = (t & (0x0f0f)) + ((t >> 4) & (0x0f0f));
    t = (t & (0x00ff)) + ((t >> 8) & (0x00ff));
    return t;
}
static inline int Abc_Tt4FirstBit( int t )
{
    int n = 0;
    if ( t == 0 ) return -1;
    if ( (t & 0x00FF) == 0 ) { n +=  8; t >>=  8; }
    if ( (t & 0x000F) == 0 ) { n +=  4; t >>=  4; }
    if ( (t & 0x0003) == 0 ) { n +=  2; t >>=  2; }
    if ( (t & 0x0001) == 0 ) { n++; }
    return n;
}
static inline int Abc_Tt4Check( int t )
{
    int Count, tn = 0xFFFF & ~t;
    if ( t == 0x6996 || tn == 0x6996 ) return 1;
    if ( (t & (t-1)) == 0 )            return 1;
    if ( (tn & (tn-1)) == 0 )          return 1;
    Count = Abc_Tt4CountOnes( t );
    if ( Count == 7 && Abc_Tt4CheckTwoLevel(t)  > 0 ) return 1;
    if ( Count == 9 && Abc_Tt4CheckTwoLevel(tn) > 0 ) return 1;
    return 0;
}

/*=== utilTruth.c ===========================================================*/


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
