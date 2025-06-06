/**CFile****************************************************************

  FileName    [giaDecs.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Calling various decomposition engines.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaDecs.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig/gia/gia.h"
#include "misc/util/utilTruth.h"
#include "misc/extra/extra.h"
#include "bool/bdc/bdc.h"
#include "bool/kit/kit.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern void Extra_BitMatrixTransposeP( Vec_Wrd_t * vSimsIn, int nWordsIn, Vec_Wrd_t * vSimsOut, int nWordsOut );
extern Vec_Int_t * Gia_ManResubOne( Vec_Ptr_t * vDivs, int nWords, int nLimit, int nDivsMax, int iChoice, int fUseXor, int fDebug, int fVerbose, word * pFunc, int Depth );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ResubVarNum( Vec_Int_t * vResub )
{
    if ( Vec_IntSize(vResub) == 1 )
        return Vec_IntEntryLast(vResub) >= 2;
    return Vec_IntEntryLast(vResub)/2 - Vec_IntSize(vResub)/2 - 1;
}
word Gia_ResubToTruth6_rec( Vec_Int_t * vResub, int iNode, int nVars )
{
    assert( iNode >= 0 && nVars <= 6 );
    if ( iNode < nVars )
        return s_Truths6[iNode];
    else
    {
        int iLit0 = Vec_IntEntry( vResub, Abc_Var2Lit(iNode-nVars, 0) );
        int iLit1 = Vec_IntEntry( vResub, Abc_Var2Lit(iNode-nVars, 1) );
        word Res0 = Gia_ResubToTruth6_rec( vResub, Abc_Lit2Var(iLit0)-2, nVars );
        word Res1 = Gia_ResubToTruth6_rec( vResub, Abc_Lit2Var(iLit1)-2, nVars );
        Res0 = Abc_LitIsCompl(iLit0) ? ~Res0 : Res0;
        Res1 = Abc_LitIsCompl(iLit1) ? ~Res1 : Res1;
        return iLit0 > iLit1 ? Res0 ^ Res1 : Res0 & Res1;
    }
}
word Gia_ResubToTruth6( Vec_Int_t * vResub )
{
    word Res;
    int iRoot = Vec_IntEntryLast(vResub);
    if ( iRoot < 2 )
        return iRoot ? ~(word)0 : 0;
    assert( iRoot != 2 && iRoot != 3 );
    Res = Gia_ResubToTruth6_rec( vResub, Abc_Lit2Var(iRoot)-2, Gia_ResubVarNum(vResub) );
    return Abc_LitIsCompl(iRoot) ? ~Res : Res;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wrd_t * Gia_ManDeriveTruths( Gia_Man_t * p, Vec_Wrd_t * vSims, Vec_Wrd_t * vIsfs, Vec_Int_t * vCands, Vec_Int_t * vSet, int nWords )
{
    int nTtWords = Abc_Truth6WordNum(Vec_IntSize(vSet));
    int nFuncs = Vec_WrdSize(vIsfs) / 2 / nWords;
    Vec_Wrd_t * vRes = Vec_WrdStart( 2 * nFuncs * nTtWords );
    Vec_Wrd_t * vIn = Vec_WrdStart( 64*nWords ), * vOut;
    int i, f, m, iObj; word Func;
    assert( Vec_IntSize(vSet) <= 64 );
    Vec_IntForEachEntry( vSet, iObj, i )
        Abc_TtCopy( Vec_WrdEntryP(vIn, i*nWords), Vec_WrdEntryP(vSims, Vec_IntEntry(vCands, iObj)*nWords), nWords, 0 );
    vOut = Vec_WrdStart( Vec_WrdSize(vIn) );
    Extra_BitMatrixTransposeP( vIn, nWords, vOut, 1 );
    for ( f = 0; f < nFuncs; f++ )
    {
        word * pIsf[2]   = { Vec_WrdEntryP(vIsfs, (2*f+0)*nWords), 
                             Vec_WrdEntryP(vIsfs, (2*f+1)*nWords) };
        word * pTruth[2] = { Vec_WrdEntryP(vRes, (2*f+0)*nTtWords), 
                             Vec_WrdEntryP(vRes, (2*f+1)*nTtWords) };
        for ( m = 0; m < 64*nWords; m++ )
        {
            int iMint = (int)Vec_WrdEntry(vOut, m);
            int Value0 = Abc_TtGetBit( pIsf[0], m );
            int Value1 = Abc_TtGetBit( pIsf[1], m );
            if ( !Value0 && !Value1 )
                continue;
            if ( Value0 && Value1 )
                printf( "Internal error: Onset and Offset overlap.\n" );
            assert( !Value0 || !Value1 );
            Abc_TtSetBit( pTruth[Value1], iMint );
        }
        if ( Abc_TtCountOnesVecMask(pTruth[0], pTruth[1], nTtWords, 0) )
            printf( "Verification for function %d failed for %d minterm pairs.\n", f,
                Abc_TtCountOnesVecMask(pTruth[0], pTruth[1], nTtWords, 0) );
    }
    if ( Vec_IntSize(vSet) < 6 )
        Vec_WrdForEachEntry( vRes, Func, i )
            Vec_WrdWriteEntry( vRes, i, Abc_Tt6Stretch(Func, Vec_IntSize(vSet)) );
    Vec_WrdFree( vIn );
    Vec_WrdFree( vOut );
    return vRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCountResub( Vec_Wrd_t * vTruths, int nVars, int fVerbose )
{
    Vec_Int_t * vResub; int nNodes;
    int nTtWords = Abc_Truth6WordNum(nVars);
    int v, nFuncs = Vec_WrdSize(vTruths) / 2 / nTtWords;
    Vec_Wrd_t * vElems = Vec_WrdStartTruthTables( nVars );
    Vec_Ptr_t * vDivs = Vec_PtrAlloc( 2 + nVars );
    assert( Vec_WrdSize(vElems) == nTtWords * nVars );
    assert( nFuncs == 1 );
    Vec_PtrPush( vDivs, Vec_WrdEntryP(vTruths, (2*0+0)*nTtWords) );
    Vec_PtrPush( vDivs, Vec_WrdEntryP(vTruths, (2*0+1)*nTtWords) );
    for ( v = 0; v < nVars; v++ )
        Vec_PtrPush( vDivs, Vec_WrdEntryP(vElems, v*nTtWords) );
    vResub = Gia_ManResubOne( vDivs, nTtWords, 30, 100, 0, 0, 0, fVerbose, NULL, 0 );
    Vec_PtrFree( vDivs );
    Vec_WrdFree( vElems );
    nNodes = Vec_IntSize(vResub) ? Vec_IntSize(vResub)/2 : 999;
    Vec_IntFree( vResub );
    return nNodes;
}
Vec_Int_t * Gia_ManDeriveResub( Vec_Wrd_t * vTruths, int nVars )
{
    Vec_Int_t * vResub;  
    int nTtWords = Abc_Truth6WordNum(nVars);
    int v, nFuncs = Vec_WrdSize(vTruths) / 2 / nTtWords;
    Vec_Wrd_t * vElems = Vec_WrdStartTruthTables( nVars );
    Vec_Ptr_t * vDivs = Vec_PtrAlloc( 2 + nVars );
    assert( Vec_WrdSize(vElems) == nTtWords * nVars );
    assert( nFuncs == 1 );
    Vec_PtrPush( vDivs, Vec_WrdEntryP(vTruths, (2*0+0)*nTtWords) );
    Vec_PtrPush( vDivs, Vec_WrdEntryP(vTruths, (2*0+1)*nTtWords) );
    for ( v = 0; v < nVars; v++ )
        Vec_PtrPush( vDivs, Vec_WrdEntryP(vElems, v*nTtWords) );
    vResub = Gia_ManResubOne( vDivs, nTtWords, 30, 100, 0, 0, 0, 0, NULL, 0 );
    Vec_PtrFree( vDivs );
    Vec_WrdFree( vElems );
    return vResub;
}

int Gia_ManCountBidec( Vec_Wrd_t * vTruths, int nVars, int fVerbose )
{
    int nNodes, nTtWords = Abc_Truth6WordNum(nVars);
    word * pTruth[2] = { Vec_WrdEntryP(vTruths, 0*nTtWords), 
                         Vec_WrdEntryP(vTruths, 1*nTtWords) };
    Abc_TtOr( pTruth[0], pTruth[0], pTruth[1], nTtWords );
    nNodes = Bdc_ManBidecNodeNum( pTruth[1], pTruth[0], nVars, fVerbose );
    Abc_TtSharp( pTruth[0], pTruth[0], pTruth[1], nTtWords );
    return nNodes;
}
Vec_Int_t * Gia_ManDeriveBidec( Vec_Wrd_t * vTruths, int nVars )
{
    Vec_Int_t * vRes = NULL;
    int nTtWords = Abc_Truth6WordNum(nVars);
    word * pTruth[2] = { Vec_WrdEntryP(vTruths, 0*nTtWords), 
                         Vec_WrdEntryP(vTruths, 1*nTtWords) };
    Abc_TtOr( pTruth[0], pTruth[0], pTruth[1], nTtWords );
    vRes = Bdc_ManBidecResub( pTruth[1], pTruth[0], nVars );
    Abc_TtSharp( pTruth[0], pTruth[0], pTruth[1], nTtWords );
    return vRes;
}

int Gia_ManCountIsop( Vec_Wrd_t * vTruths, int nVars, int fVerbose )
{
    int nTtWords = Abc_Truth6WordNum(nVars);
    word * pTruth[2] = { Vec_WrdEntryP(vTruths, 0*nTtWords), 
                         Vec_WrdEntryP(vTruths, 1*nTtWords) };
    int nNodes = Kit_IsopNodeNum( (unsigned *)pTruth[0], (unsigned *)pTruth[1], nVars, NULL );
    return nNodes;
}
Vec_Int_t * Gia_ManDeriveIsop( Vec_Wrd_t * vTruths, int nVars )
{
    Vec_Int_t * vRes = NULL;
    int nTtWords = Abc_Truth6WordNum(nVars);
    word * pTruth[2] = { Vec_WrdEntryP(vTruths, 0*nTtWords), 
                         Vec_WrdEntryP(vTruths, 1*nTtWords) };
    vRes = Kit_IsopResub( (unsigned *)pTruth[0], (unsigned *)pTruth[1], nVars, NULL );
    return vRes;
}

int Gia_ManCountBdd( Vec_Wrd_t * vTruths, int nVars, int fVerbose )
{
    extern Gia_Man_t * Gia_TryPermOptNew( word * pTruths, int nIns, int nOuts, int nWords, int nRounds, int fVerbose );
    int nTtWords = Abc_Truth6WordNum(nVars);
    word * pTruth[2] = { Vec_WrdEntryP(vTruths, 0*nTtWords), 
                         Vec_WrdEntryP(vTruths, 1*nTtWords) };
    Gia_Man_t * pGia; int nNodes;

    Abc_TtOr( pTruth[1], pTruth[1], pTruth[0], nTtWords );
    Abc_TtNot( pTruth[0], nTtWords );
    pGia = Gia_TryPermOptNew( pTruth[0], nVars, 1, nTtWords, 50, 0 );
    Abc_TtNot( pTruth[0], nTtWords );
    Abc_TtSharp( pTruth[1], pTruth[1], pTruth[0], nTtWords );

    nNodes = Gia_ManAndNum(pGia);
    Gia_ManStop( pGia );
    return nNodes;
}
Vec_Int_t * Gia_ManDeriveBdd( Vec_Wrd_t * vTruths, int nVars )
{
    extern Vec_Int_t * Gia_ManToGates( Gia_Man_t * p );
    Vec_Int_t * vRes = NULL;
    extern Gia_Man_t * Gia_TryPermOptNew( word * pTruths, int nIns, int nOuts, int nWords, int nRounds, int fVerbose );
    int nTtWords = Abc_Truth6WordNum(nVars);
    word * pTruth[2] = { Vec_WrdEntryP(vTruths, 0*nTtWords), 
                         Vec_WrdEntryP(vTruths, 1*nTtWords) };
    Gia_Man_t * pGia;

    Abc_TtOr( pTruth[1], pTruth[1], pTruth[0], nTtWords );
    Abc_TtNot( pTruth[0], nTtWords );
    pGia = Gia_TryPermOptNew( pTruth[0], nVars, 1, nTtWords, 50, 0 );
    Abc_TtNot( pTruth[0], nTtWords );
    Abc_TtSharp( pTruth[1], pTruth[1], pTruth[0], nTtWords );

    vRes = Gia_ManToGates( pGia );
    Gia_ManStop( pGia );
    return vRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManEvalSolutionOne( Gia_Man_t * p, Vec_Wrd_t * vSims, Vec_Wrd_t * vIsfs, Vec_Int_t * vCands, Vec_Int_t * vSet, int nWords, int fVerbose )
{
    Vec_Wrd_t * vTruths = Gia_ManDeriveTruths( p, vSims, vIsfs, vCands, vSet, nWords );
    int nTtWords = Vec_WrdSize(vTruths)/2, nVars = Vec_IntSize(vSet);
    word * pTruth[2] = { Vec_WrdEntryP(vTruths, 0*nTtWords), 
                         Vec_WrdEntryP(vTruths, 1*nTtWords) };
    int nNodesResub = Gia_ManCountResub( vTruths, nVars, 0 );
    int nNodesBidec = nVars > 2 ? Gia_ManCountBidec( vTruths, nVars, 0 ) : 999;
    int nNodesIsop  = nVars > 2 ? Gia_ManCountIsop( vTruths, nVars, 0 ) : 999;
    int nNodesBdd   = nVars > 2 ? Gia_ManCountBdd( vTruths, nVars, 0 ) : 999;
    int nNodesMin   = Abc_MinInt( Abc_MinInt(nNodesResub, nNodesBidec), Abc_MinInt(nNodesIsop, nNodesBdd) );
    if ( fVerbose )
    {
        printf( "Size = %2d  ", nVars );
        printf( "Resub =%3d  ", nNodesResub );
        printf( "Bidec =%3d  ", nNodesBidec );
        printf( "Isop =%3d  ",  nNodesIsop );
        printf( "Bdd =%3d  ",   nNodesBdd );
        Abc_TtIsfPrint( pTruth[0], pTruth[1], nTtWords ); 
        if ( nVars <= 6 )
        {
            printf( "  " );
            Extra_PrintHex( stdout, (unsigned*)pTruth[0], nVars );
            printf( "  " );
            Extra_PrintHex( stdout, (unsigned*)pTruth[1], nVars );
        }
        printf( "\n" );
    }
    Vec_WrdFree( vTruths );
    if ( nNodesMin > 500 )
        return -1;
    if ( nNodesMin == nNodesResub )
        return (nNodesMin << 2) | 0;
    if ( nNodesMin == nNodesBidec )
        return (nNodesMin << 2) | 1;
    if ( nNodesMin == nNodesIsop )
        return (nNodesMin << 2) | 2;
    if ( nNodesMin == nNodesBdd )
        return (nNodesMin << 2) | 3;
    return -1;
}
Vec_Int_t * Gia_ManDeriveSolutionOne( Gia_Man_t * p, Vec_Wrd_t * vSims, Vec_Wrd_t * vIsfs, Vec_Int_t * vCands, Vec_Int_t * vSet, int nWords, int Type )
{
    Vec_Int_t * vRes = NULL;
    Vec_Wrd_t * vTruths = Gia_ManDeriveTruths( p, vSims, vIsfs, vCands, vSet, nWords );
    int nTtWords = Vec_WrdSize(vTruths)/2, nVars = Vec_IntSize(vSet);
    word * pTruth[2] = { Vec_WrdEntryP(vTruths, 0*nTtWords), 
                         Vec_WrdEntryP(vTruths, 1*nTtWords) };
    if ( Type == 0 )
        vRes = Gia_ManDeriveResub( vTruths, nVars );
    else if ( Type == 1 )
        vRes = Gia_ManDeriveBidec( vTruths, nVars );
    else if ( Type == 2 )
        vRes = Gia_ManDeriveIsop( vTruths, nVars );
    else if ( Type == 3 )
        vRes = Gia_ManDeriveBdd( vTruths, nVars );
    if ( vRes && Gia_ResubVarNum(vRes) <= 6 )
    {
        word Func = Gia_ResubToTruth6( vRes );
        assert( !(Func &  pTruth[0][0]) );
        assert( !(pTruth[1][0] & ~Func) );   
    }
    Vec_WrdFree( vTruths );
    return vRes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

