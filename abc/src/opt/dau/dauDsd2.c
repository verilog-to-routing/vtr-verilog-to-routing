/**CFile****************************************************************

  FileName    [dauDsd2.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware unmapping.]

  Synopsis    [Disjoint-support decomposition.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: dauDsd2.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dauInt.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#include DSD_MAX_VAR 12
#include DSD_MAX_WRD ((DSD_MAX_VAR > 6) ? (1 << (DSD_MAX_VAR-6)) : 1)

typedef struct Dua_Obj_t_ Dua_Obj_t;
struct Dua_Obj_t_
{
    int       Type;                // dec type (1=var; 2=and; 3=xor; 4=mux; 5=prime)
    int       nFans;               // fanin count
    char      pFans[DSD_MAX_VAR];  // fanins
};

typedef struct Dua_Dsd_t_ Dua_Dsd_t;
struct Dua_Dsd_t_
{
    int       nSupp;               // original variables
    int       nVars;               // remaining variables
    int       nWords;              // largest non-dec prime
    int       nObjs;               // object count
    int       iRoot;               // the root of the tree
    Dua_Obj_t pObjs[DSD_MAX_VAR];  // objects
    word      pTruth[DSD_MAX_WRD]; // original/current truth table
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Makes the fCof1-th cofactor of iVar the 0-th cofactor.]

  Description [Variable iVar becomes last varaible; others shift back.
  Only the 0-th cofactor is computed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Abc_Tt6HalfUnShuffleVars( word t, int iVar, int fCof1 )
{
    static word Masks[6] = {
        ABC_CONST(0x5555555555555555),
        ABC_CONST(0x3333333333333333),
        ABC_CONST(0x0F0F0F0F0F0F0F0F),
        ABC_CONST(0x00FF00FF00FF00FF),
        ABC_CONST(0x0000FFFF0000FFFF),
        ABC_CONST(0x00000000FFFFFFFF) 
    };
    int v, s = (1 << iVar);
    t = (t >> (fCof1 ? 0 : s)) & Masks[iVar];
    for ( v = iVar, s = (1 << v); v < 5; v++, s <<= 1 )
        t = ((t >> s) | t) & Masks[v+1];
    return t;
}

static inline void Abc_TtHalfUnShuffleVars( word * pTruth, int nVars, int iVar, int jVar, int fCof1 )
{
    int w, nWords = Abc_TtWordNum( nVars );
    if ( iVar == jVar )
        return;
    assert( iVar < jVar );
    if ( iVar < 5 )
    {
        for ( w = 0; w < nWords; w++ )
            pTruth[w] = Abc_Tt6HalfUnShuffleVars( pTruth[w], iVar, fCof1 );
        iVar = 5;
    }
    if ( jVar < 6 )
    {
        for ( w = 0; w < nWords; w++ )
            pTruth[w] = (pTruth[w] << 32) | pTruth[w];
        return;
    }
    if ( iVar == 5 )
    {
        unsigned * pTruthU = (unsigned *)pTruth;
        for ( w = 0; w < nWords; w += 2 )
            pTruthU[w] = pTruthU[w+1];
        iVar = 6;
    }
    {
        word * pLimit = pTruth + nWords;
        int i, iStep = Abc_TtWordNum(iVar);
        int j, jStep = Abc_TtWordNum(jVar);
        for ( ; pTruth < pLimit; pTruth += jStep )
            for ( i = 0; i < jStep; i += iStep )
                for ( j = 0; j < iStep; j++ )
                    pTruth[w++] = pTruth[iStep + i + j];
        assert( w == (nWords >> 1) );
        return;
    }    
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dua_DsdInit( Dua_Dsd_t * pRes, word * pTruth, int nVars )
{
    int i;
    pRes->nSupp  = nVars;
    pRes->nVars  = nVars;
    pRes->nWords = Abc_TtWordNum( nVars );
    pRes->nObjs  = 1;
    pRes->iRoot  = Abc_Var2Lit( 0, 0 );
    pRes->pObjs[0].Type = 5;
    pRes->pObjs[0].nFans = nVars;
    for ( i = 0; i < nVars; i++ )
        pRes->pObjs[0].pFans[i] = (char)Abc_Var2Lit( i, 0 );
    memcpy( pRes->pTruth, pTruth, sizeof(word) * pRes->nWords );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// returns 1 if the truth table was complemented
int Dua_DsdTryConst( word * pTruth, int nVars )
{
    if ( !(pTruth[0] & 1) )
        return 0;
    Abc_TtNot( pTruth, Abc_TtWordNum(nVars) );
    return 1;
}
int Dua_DsdTryVar( word * pTruth, int nWords, int iVar )
{
    int nWordsI = Abc_TtWordNum(iVar);
    word c0 = (iVar < 6) ? Abc_Tt6Cofactor0( pTruth[0], iVar ) : pTruth[0];
    word c1 = (iVar < 6) ? Abc_Tt6Cofactor1( pTruth[0], iVar ) : pTruth[nWords];
    if ( c0 != c1 )
    {
        if ( c1 < c0 && c1 < ~c1 ) // flip
        {
            Abc_TtFlip( pTruth, nWords, iVar );
            return 0;
        }
        if ( ~c1 < c0 && ~c1 < c1 ) // flip and compl
        {
            Abc_TtFlipNot( pTruth, nWords, iVar );
            return 1;
        }
    }
    if ( iVar < 6 )
    {
        word * pLimit = pTruth + nWords;
        for ( pTruth++; pTruth < pLimit; pTruth++ )
        {
            c0 = Abc_Tt6Cofactor0( pTruth[0], iVar );
            c1 = Abc_Tt6Cofactor1( pTruth[0], iVar );
            if ( c0 == c1 )
                continue;
            if ( c0 < c1 )
                return 0;
            for ( ; pTruth < pLimit; pTruth++ )
                pTruth[0] = Abc_Tt6Flip( pTruth[0], iVar );
            return 0;
        }
    }
    else
    {
        for ( ; pTruth < pLimit; pTruth += (nWordsI << 1) )
        for ( w = 0; w < nWordsI; w++ )
        {
            c0 = pTruth[0];
            c1 = pTruth[nWordsI];
            if ( c0 == c1 )
                continue;
            if ( c0 < c1 )
                return 0;
            for ( ; pTruth < pLimit; pTruth += (nWordsI << 1) )
            for ( ; w < nWordsI; w++ )
                ABC_SWAP( word, pTruth[0], pTruth[nWordsI] );
            return 0;
        }
    }
    assert( 0 );
    return -1;
}
int Dua_DsdCheckCof0Const0( word * pTruth, int nWords, int iVar )
{
    if ( nWords == 1 )
        return (pTruth[0] & s_Truths6Neg[iVar]) == 0;
    if ( iVar <= 5 )
    {
        int w;
        for ( w = 0; w < nWords; w++ )
            if ( (pTruth[w] & s_Truths6Neg[iVar]) )
                return 0;
        return 1;
    }
    else // if ( iVar > 5 )
    {
        word * pLimit = pTruth + nWords;
        int i, iStep = Abc_TtWordNum(iVar);
        for ( ; pTruth < pLimit; pTruth += (iStep << 1) )
            for ( i = 0; i < iStep; i++ )
                if ( pTruth[i] )
                    return 0;
        return 1;
    }    
}
int Dua_DsdCheckCofsEqualNot( word * pTruth, int nWords, int iVar )
{
    if ( nWords == 1 )
        return (pTruth[0] & s_Truths6Neg[iVar]) == ((~pTruth[0] & s_Truths6[iVar]) >> (1 << iVar));
    if ( iVar <= 5 )
    {
        int w, shift = (1 << iVar);
        for ( w = 0; w < nWords; w++ )
            if ( (pTruth[w] & s_Truths6Neg[iVar]) != ((~pTruth[w] & s_Truths6[iVar]) >> shift) )
                return 0;
        return 1;
    }
    else // if ( iVar > 5 )
    {
        word * pLimit = pTruth + nWords;
        int i, iStep = Abc_TtWordNum(iVar);
        for ( ; pTruth < pLimit; pTruth += (iStep << 1) )
            for ( i = 0; i < iStep; i++ )
                if ( pTruth[i] != ~pTruth[i + iStep] )
                    return 0;
        return 1;
    }    
}
int Dua_DsdOneVar( Dua_Dsd_t * pRes )
{
    int v, fCompl, fChange = 1;
    fCompl = Dua_DsdTryConst( pRes->pTruth, pRes->nWords );
    while ( fChange && pRes->nVars > 2 )
    {
        fChange = 0;
        for ( v = 0; v < pRes->nVars; v++ )
        {
            fCompl ^= Dua_DsdTryVar( pRes->pTruth, pRes->nWords, v );
            if ( Dua_DsdCheckCof0Const0( pRes->pTruth, pRes->nWords, v ) )
            {
                fChange = 1;
                // record AND(v, F)
            }
            else if ( Dua_DsdCheckCofsEqualNot( pRes->pTruth, pRes->nWords, v ) )
            {
                fChange = 1;
                // record XOR(v, F)
            }
        }
    }
    return fCompl;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dua_DsdTrySwap( word * pTruth, int nWords, int iVar )
{
    static word s_PMasks[5][3] = {
        { ABC_CONST(0x9999999999999999), ABC_CONST(0x2222222222222222), ABC_CONST(0x4444444444444444) },
        { ABC_CONST(0xC3C3C3C3C3C3C3C3), ABC_CONST(0x0C0C0C0C0C0C0C0C), ABC_CONST(0x3030303030303030) },
        { ABC_CONST(0xF00FF00FF00FF00F), ABC_CONST(0x00F000F000F000F0), ABC_CONST(0x0F000F000F000F00) },
        { ABC_CONST(0xFF0000FFFF0000FF), ABC_CONST(0x0000FF000000FF00), ABC_CONST(0x00FF000000FF0000) },
        { ABC_CONST(0xFFFF00000000FFFF), ABC_CONST(0x00000000FFFF0000), ABC_CONST(0x0000FFFF00000000) }
    };
    if ( iVar < 5 )
    {
        int Shift = (1 << iVar);
        word c01, c10, * pLimit = pTruth + nWords;
        for ( ; pTruth < pLimit; pTruth++ )
        {
            c01 = (pTruth[0] & s_PMasks[iVar][1]);
            c10 = (pTruth[0] & s_PMasks[iVar][2]) >> Shift;
            if ( c01 == c10 )
                continue;
            if ( c01 < c10 )
                return 0;
            pTruth[0] = (pTruth[0] & s_PMasks[iVar][0]) | ((pTruth[0] & s_PMasks[iVar][1]) << Shift) | ((pTruth[0] & s_PMasks[iVar][2]) >> Shift);
            return 1;
        }
    }
    else if ( iVar == 5 )
    {
        unsigned * pTruthU = (unsigned *)pTruth;
        unsigned * pLimitU = (unsigned *)(pTruth + nWords);
        for ( ; pTruthU < pLimitU; pTruthU += 4 )
        {
            c01 = pTruthU[1];
            c10 = pTruthU[2];
            if ( c01 == c10 )
                continue;
            if ( c01 < c10 )
                return 0;
            for ( ; pTruthU < pLimitU; pTruthU += 4 )
                ABC_SWAP( unsigned, pTruthU[1], pTruthU[2] );
            return 1;
        }
    }
    else // if ( iVar > 5 )
    {
        word * pLimit = pTruth + nWords;
        int i, iStep = Abc_TtWordNum(iVar);
        for ( ; pTruth < pLimit; pTruth += 4*iStep )
        for ( i = 0; i < iStep; i++ )
        {
            c01 = pTruth[i + iStep];
            c10 = pTruth[i + 2*iStep];
            if ( c01 == c10 )
                continue;
            if ( c01 < c10 )
                return 0;
            for ( ; pTruth < pLimit; pTruth += 4*iStep )
            for ( ; i < iStep; i++ )
                ABC_SWAP( word, pTruth[1], pTruth[2] );
            return 1;
        }
    }
    return 2;
}
int Dua_DsdCheckDecomp( word * pTruth, int nWords, int iVar )
{
    static word s_PMasks[5][4] = {
        { ABC_CONST(0x1111111111111111), ABC_CONST(0x2222222222222222), ABC_CONST(0x4444444444444444), ABC_CONST(0x8888888888888888) },
        { ABC_CONST(0x0303030303030303), ABC_CONST(0x0C0C0C0C0C0C0C0C), ABC_CONST(0x3030303030303030), ABC_CONST(0xC0C0C0C0C0C0C0C0) },
        { ABC_CONST(0x000F000F000F000F), ABC_CONST(0x00F000F000F000F0), ABC_CONST(0x0F000F000F000F00), ABC_CONST(0xF000F000F000F000) },
        { ABC_CONST(0x000000FF000000FF), ABC_CONST(0x0000FF000000FF00), ABC_CONST(0x00FF000000FF0000), ABC_CONST(0xFF000000FF000000) },
        { ABC_CONST(0x000000000000FFFF), ABC_CONST(0x00000000FFFF0000), ABC_CONST(0x0000FFFF00000000), ABC_CONST(0xFFFF000000000000) }
    };
    int fC0eC1 = 1, fC0eC3 = 1;
    if ( iVar < 5 )
    {
        int Shift = (1 << iVar);
        word c01, c10, * pLimit = pTruth + nWords;
        for ( ; pTruth < pLimit; pTruth++ )
        {
            if ( fC0eC1 && (pTruth[0] & s_PMasks[iVar][0]) != ((pTruth[0] & s_PMasks[iVar][1]) >>    Shift) )
                fC0eC1 = 0;
            if ( fC0eC3 && (pTruth[0] & s_PMasks[iVar][0]) != ((pTruth[0] & s_PMasks[iVar][3]) >> (3*Shift)) )
                fC0eC3 = 0;
            if ( !fC0eC1 && !fC0eC3 )
                return 0;
        }
    }
    if ( iVar == 5 )
    {
        unsigned * pTruthU = (unsigned *)pTruth;
        unsigned * pLimitU = (unsigned *)(pTruth + nWords);
        for ( ; pTruthU < pLimitU; pTruthU += 4 )
        {
            if ( fC0eC1 && pTruthU[0] != pTruthU[1] )
                fC0eC1 = 0;
            if ( fC0eC3 && pTruthU[0] != pTruthU[3] )
                fC0eC3 = 0;
            if ( !fC0eC1 && !fC0eC3 )
                return 0;
        }
    }
    else // if ( iVar > 5 )
    {
        word * pLimit = pTruth + nWords;
        int i, iStep = Abc_TtWordNum(iVar);
        for ( ; pTruth < pLimit; pTruth += 4*iStep )
        for ( i = 0; i < iStep; i++ )
        {
            if ( fC0eC1 && pTruth[0] != pTruth[1] )
                fC0eC1 = 0;
            if ( fC0eC3 && pTruth[0] != pTruth[3] )
                fC0eC3 = 0;
            if ( !fC0eC1 && !fC0eC3 )
                return 0;
        }
    }
    assert( fC0eC1 != fC0eC3 );
    return fC0eC1 ? 1 : 2;
}

// returns 1 if decomposition detected
int Dua_DsdTwoVars( Dua_Dsd_t * pRes )
{
    int v, RetValue, fChange = 1;
    while ( fChange && pRes->nVars > 2 )
    {
        fChange = 0;
        for ( v = 0; v < pRes->nVars - 1; v++ )
        {
            RetValue = Dua_DsdTrySwap( pRes->pTruth, pRes->nWords, v );            
            if ( RetValue == 1 )
                fChange = 1;
            if ( RetValue != 2 )
                continue;
            // vars are symmetric, check decomp
            RetValue = Dua_DsdCheckDecomp( pRes->pTruth, pRes->nWords, v );
            if ( RetValue == 0 )
                continue;
            if ( RetValue == 1 )
            {
                fChange = 1;
                // record AND(a, b)
            }
            else
            {
                fChange = 1;
                // record XOR(a, b)
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    [Check DSD for bound-set [iVar; jVar).]

  Description [Return D-func if decomposable.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word Dua_DsdRangeVars( word * pTruth, int nVars, int iVar, int jVar, int fPerform )
{
    int Part, nParts = 1 << (nVars - jVar);
    int Mint, nMints = 1 << (jVar - iVar);
    word MaskOne, MaskAll = 0;
    assert( jVar - iVar > 2 );
    assert( jVar - iVar < 7 );
    if ( iVar < 6 )
    {
        int Shift = 6 - iVar, MaskF = (1 << Shift) - 1, iMint = 0;
        word MaskFF = (((word)1) << (1 << iVar)) - 1;
        word Cof0, Cof1, Value; 
        for ( Part = 0; Part < nParts; Part++ )
        {
            MaskOne = 0;
            Cof0 = Cof1 = ~(word)0;
            for ( Mint = 0; Mint < nMints; Mint++, iMint++ )
            {
                Value = (pTruth[iMint>>Shift] >> ((iMint & MaskF)<<iVar)) & MaskFF;
                if ( !~Cof0 || Cof0 == Value )
                    Cof0 = Value;
                else if ( !~Cof1 || Cof1 == Value )
                {
                    Cof1 = Value;
                    MaskOne |= ((word)1) << Mint;
                }
                else
                    return 0;
            }
            if ( Part == 0 )
                MaskAll = MaskOne;
            else if ( MaskAll != MaskOne )
                return 0;
            if ( fPerform )
            {
                assert( ~Cof0 && ~Cof1 );
                Mint = 2 * Part;
                Value = (pTruth[Mint>>Shift] >> ((Mint & MaskF)<<nVarsF)) & MaskFF;
                pTruth[Mint>>Shift] ^= (Value ^ Cof0) << ((Mint & MaskF)<<nVarsF)
                Mint = 2 * Part + 1;
                Value = (pTruth[Mint>>Shift] >> ((Mint & MaskF)<<nVarsF)) & MaskFF;
                pTruth[Mint>>Shift] ^= (Value ^ Cof1) << ((Mint & MaskF)<<nVarsF)
            }
        }
        // stretch
        if ( nVars - (jVar - iVar) + 1 < 6 )
            pTruth[0] = Abc_Tt6Stretch( pTruth[0], nVars - (jVar - iVar) + 1 );
    }
    else
    {
        int nWordsF = Abc_TtWordNum(iVar); 
        int iWord = 0, nBytes = sizeof(word) * nWordsF;
        word * pCof0, * pCof1;
        for ( Part = 0; Part < nParts; Part++ )
        {
            MaskOne = 0;
            pCof0 = pCof1 = NULL;
            for ( Mint = 0; Mint < nMints; Mint++, iWord += nWordsF )
            {
                if ( !pCof0 || !memcmp(pCof0, pTruth + iWord, nBytes) )
                    pCof0 = pTruth + iWord;
                else if ( !pCof1 || !memcmp(pCof1, pTruth + iWord, nBytes) )
                {
                    pCof1 = pTruth + iWord;
                    MaskOne |= ((word)1) << Mint;
                }
                else
                    return 0;
            }
            if ( Part == 0 )
                MaskAll = MaskOne;
            else if ( MaskAll != MaskOne )
                return 0;
            if ( fPerform )
            {
                assert( pCof0 && pCof1 );
                memcpy( pTruth + (2 * Part + 0) * nWordsF, pCof0, nBytes );
                memcpy( pTruth + (2 * Part + 1) * nWordsF, pCof1, nBytes );
            }
        }
    }
    return MaskAll;
}

/**Function*************************************************************

  Synopsis    [Check DSD for bound-set [0; iVar).]

  Description [Return D-func if decomposable.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dua_DsdRangeVars0( word * pTruth, int nVars, int iVar, int fPerform )
{
    int i, nParts = 1 << (nVars - iVar);
    assert( iVar > 2 && iVar < nVars );
    if ( iVar == 3 )
    {
        unsigned char * pTruthP = (unsigned char *)pTruth, Dfunc = pTruthP[0];
        for ( i = 1; i < nParts; i++ )
            if ( pTruthP[i] != Dfunc && pTruthP[i] != ~Dfunc )
                return 0;
    }
    else if ( iVar == 4 )
    {
        unsigned short * pTruthP = (unsigned short *)pTruth, Dfunc = pTruthP[0];
        for ( i = 1; i < nParts; i++ )
            if ( pTruthP[i] != Dfunc && pTruthP[i] != ~Dfunc )
                return 0;
    }
    else if ( iVar == 5 )
    {
        unsigned int * pTruthP = (unsigned int *)pTruth, Dfunc = pTruthP[0];
        for ( i = 1; i < nParts; i++ )
            if ( pTruthP[i] != Dfunc && pTruthP[i] != ~Dfunc )
                return 0;
    }
    else 
    {
        int nStep = 1 << (6 - iVar);
        assert( iVar >= 6 );
        for ( i = 1; i < nParts; i++ )
            if ( !Abc_TtEqual(pTruth, pTruth + i * nStep, nStep) && !Abc_TtEqualNot(pTruth, pTruth + i * nStep, nStep) )
                return 0;
    }
    return 1;
}
void Dua_DsdRangeVars0Derive( word * pTruth, int nVars, int iVar )
{
    int i, nParts = 1 << (nVars - iVar);
    assert( iVar > 2 && iVar < nVars );
    if ( iVar == 3 )
    {
        unsigned char * pTruthP = (unsigned char *)pTruth, Dfunc = pTruthP[0];
        for ( i = 0; i < nParts; i++ )
            if ( Abc_TtGetBit(pTruth, i) ^ (pTruthP[i] != Dfunc) )
                Abc_TtXorBit(pTruth, i);
    }
    else if ( iVar == 4 )
    {
        unsigned short * pTruthP = (unsigned short *)pTruth, Dfunc = pTruthP[0];
        for ( i = 0; i < nParts; i++ )
            if ( Abc_TtGetBit(pTruth, i) ^ (pTruthP[i] != Dfunc) )
                Abc_TtXorBit(pTruth, i);
    }
    else if ( iVar == 5 )
    {
        unsigned int * pTruthP = (unsigned int *)pTruth, Dfunc = pTruthP[0];
        for ( i = 0; i < nParts; i++ )
            if ( Abc_TtGetBit(pTruth, i) ^ (pTruthP[i] != Dfunc) )
                Abc_TtXorBit(pTruth, i);
    }
    else 
    {
        word Dfunc = pTruth[0];
        assert( iVar == 6 );
        for ( i = 0; i < nParts; i++ )
            if ( Abc_TtGetBit(pTruth, i) ^ (pTruth[i] != Dfunc) )
                Abc_TtXorBit(pTruth, i);
   }
    // stretch
    if ( nVars - iVar + 1 < 6 )
        pTruth[0] = Abc_Tt6Stretch( pTruth[0], nVars - iVar + 1 < 6 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dua_DsdTest( word * pTruth, int nVar )
{
    Dua_Dsd_t Res, * pRes = &Res;
    Dua_DsdInit( pRes, pTruth, nVars );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END


