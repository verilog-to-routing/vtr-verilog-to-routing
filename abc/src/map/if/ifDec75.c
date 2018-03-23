/**CFile****************************************************************

  FileName    [ifDec75.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Performs additional check.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifDec75.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"
#include "misc/extra/extra.h"
#include "bool/kit/kit.h"
#include "opt/dau/dau.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Finds all boundsets for which decomposition exists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dau_DsdCheckDecExist_rec( char * pStr, char ** p, int * pMatches, int * pnSupp )
{
    if ( **p == '!' )
        (*p)++;
    while ( (**p >= 'A' && **p <= 'F') || (**p >= '0' && **p <= '9') )
        (*p)++;
    if ( **p >= 'a' && **p <= 'z' ) // var
    {
        (*pnSupp)++;
        return 0;
    }
    if ( **p == '(' || **p == '[' ) // and/xor
    {
        unsigned Mask = 0;
        int m, pSupps[8] = {0}, nParts = 0, nMints;
        char * q = pStr + pMatches[ *p - pStr ];
        assert( *q == **p + 1 + (**p != '(') );
        for ( (*p)++; *p < q; (*p)++ )
        {
            Mask |= Dau_DsdCheckDecExist_rec( pStr, p, pMatches, &pSupps[nParts] );
            *pnSupp += pSupps[nParts++];
        }
        assert( *p == q );
        assert( nParts > 1 );
        nMints = (1 << nParts);
        for ( m = 1; m < nMints; m++ )
        {
            int i, Sum = 0;
            for ( i = 0; i < nParts; i++ )
                if ( (m >> i) & 1 )
                    Sum += pSupps[i];
            assert( Sum > 0 && Sum <= 8 );
            if ( Sum >= 2 )
                Mask |= (1 << Sum);
        }
        return Mask;
    }
    if ( **p == '<' || **p == '{' ) // mux
    {
        int uSupp;
        unsigned Mask = 0;
        char * q = pStr + pMatches[ *p - pStr ];
        assert( *q == **p + 1 + (**p != '(') );
        for ( (*p)++; *p < q; (*p)++ )
        {
            uSupp = 0;
            Mask |= Dau_DsdCheckDecExist_rec( pStr, p, pMatches, &uSupp );
            *pnSupp += uSupp;
        }
        assert( *p == q );
        Mask |= (1 << *pnSupp);
        return Mask;
    }
    assert( 0 );
    return 0;
}
int Dau_DsdCheckDecExist( char * pDsd )
{
    int nSupp = 0;
    if ( pDsd[1] == 0 )
        return 0;
    return Dau_DsdCheckDecExist_rec( pDsd, &pDsd, Dau_DsdComputeMatches(pDsd), &nSupp );
}

/**Function*************************************************************

  Synopsis    [Finds all boundsets for which AND-decomposition exists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dau_DsdCheckDecAndExist_rec( char * pStr, char ** p, int * pMatches, int * pnSupp )
{
    if ( **p == '!' )
        (*p)++;
    while ( (**p >= 'A' && **p <= 'F') || (**p >= '0' && **p <= '9') )
        (*p)++;
    if ( **p >= 'a' && **p <= 'z' ) // var
    {
        (*pnSupp)++;
        return 0;
    }
    if ( **p == '(' ) // and
    {
        unsigned Mask = 0;
        int m, i, pSupps[8] = {0}, nParts = 0, nSimple = 0, nMints;
        char * q = pStr + pMatches[ *p - pStr ];
        assert( *q == **p + 1 + (**p != '(') );
        for ( (*p)++; *p < q; (*p)++ )
        {
            Mask |= Dau_DsdCheckDecAndExist_rec( pStr, p, pMatches, &pSupps[nParts] );
            nSimple += (pSupps[nParts] == 1);
            *pnSupp += pSupps[nParts++];
        }
        assert( *p == q );
        assert( nParts > 1 );
        if ( nSimple > 0 )
        {
            nMints = (1 << nParts);
            for ( m = 1; m < nMints; m++ )
            {
                int Sum = 0;
                for ( i = 0; i < nParts; i++ )
                    if ( pSupps[i] > 1 && ((m >> i) & 1) )
                        Sum += pSupps[i];
                assert( Sum <= 8 );
                if ( Sum >= 2 )
                    for ( i = 0; i < nSimple; i++ )
                        Mask |= (1 << (Sum + i));
            }
            for ( i = 2; i < nSimple; i++ )
                Mask |= (1 << i);
        }
        return Mask;
    }
    if ( **p == '<' || **p == '{' || **p == '[' ) // mux/xor/nondec
    {
        int uSupp;
        unsigned Mask = 0;
        char * q = pStr + pMatches[ *p - pStr ];
        assert( *q == **p + 1 + (**p != '(') );
        for ( (*p)++; *p < q; (*p)++ )
        {
            uSupp = 0;
            Mask |= Dau_DsdCheckDecAndExist_rec( pStr, p, pMatches, &uSupp );
            *pnSupp += uSupp;
        }
        assert( *p == q );
        return Mask;
    }
    assert( 0 );
    return 0;
}
int Dau_DsdCheckDecAndExist( char * pDsd )
{
    int nSupp = 0;
    if ( pDsd[1] == 0 )
        return 1;
    return Dau_DsdCheckDecAndExist_rec( pDsd, &pDsd, Dau_DsdComputeMatches(pDsd), &nSupp );
}

/**Function*************************************************************

  Synopsis    [Performs additional check.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutPerformCheck75__( If_Man_t * p, unsigned * pTruth, int nVars, int nLeaves, char * pStr )
{
    char pDsdStr[1000];
    int nSizeNonDec, nDecExists, nDecAndExists;
    static int Counter = 0;
    Counter++;
    if ( nLeaves < 6 )
        return 1;
    assert( nLeaves <= 8 );
    if ( nLeaves < 8 && If_CutPerformCheck16( p, pTruth, nVars, nLeaves, "44" ) )
        return 1;
    // check decomposability
    nSizeNonDec = Dau_DsdDecompose( (word *)pTruth, nLeaves, 0, 0, pDsdStr );
//    printf( "Vars = %d  %s", nLeaves, pDsdStr );    printf( "\n" );
//    Extra_PrintBinary( stdout, &nDecExists, 8 );    printf( "\n" );
//    Extra_PrintBinary( stdout, &nDecAndExists, 8 ); printf( "\n" );
    if ( nLeaves == 8 )
    {
        if ( nSizeNonDec >= 5 )
            return 0;
        nDecAndExists = Dau_DsdCheckDecAndExist( pDsdStr );
        if ( nDecAndExists & 0x10 ) // bit 4
            return 1;
        else
            return 0;
    }
    if ( nLeaves == 7 )
    {
        extern void If_Dec7MinimumBase( word uTruth[2], int * pSupp, int nVarsAll, int * pnVars );
        word * pT = (word *)pTruth;
        word pCof0[2], pCof1[2];
        int v, nVarsMin;
        if ( nSizeNonDec < 5 )
        {
            nDecExists = Dau_DsdCheckDecExist( pDsdStr );
            if ( nDecExists & 0x10 ) // bit 4
                return 1;
            nDecAndExists = Dau_DsdCheckDecAndExist( pDsdStr );
            if ( nDecAndExists & 0x18 ) // bit 4, 3
                return 1;
        }
        // check cofactors
        for ( v = 0; v < 7; v++ )
        {
            pCof0[0] = pCof1[0] = pT[0];
            pCof0[1] = pCof1[1] = pT[1];
            Abc_TtCofactor0( pCof0, 2, v );
            Abc_TtCofactor1( pCof1, 2, v );
            if ( Abc_TtSupportSize(pCof0, 7) < 4 )
            {
                If_Dec7MinimumBase( pCof1, NULL, 7, &nVarsMin );
                nSizeNonDec = Dau_DsdDecompose( pCof1, nVarsMin, 0, 0, pDsdStr );
                if ( nSizeNonDec >= 5 )
                    continue;
                nDecExists = Dau_DsdCheckDecExist( pDsdStr );
                if ( nDecExists & 0x18 ) // bit 4, 3
                    return 1;
            }
            else if ( Abc_TtSupportSize(pCof1, 7) < 4 )
            {
                If_Dec7MinimumBase( pCof0, NULL, 7, &nVarsMin );
                nSizeNonDec = Dau_DsdDecompose( pCof0, nVarsMin, 0, 0, pDsdStr );
                if ( nSizeNonDec >= 5 )
                    continue;
                nDecExists = Dau_DsdCheckDecExist( pDsdStr );
                if ( nDecExists & 0x18 ) // bit 4, 3
                    return 1;
            }
        }
        return 0;
    }
    if ( nLeaves == 6 )
    {
        if ( nSizeNonDec < 5 )
        {
            nDecExists = Dau_DsdCheckDecExist( pDsdStr );
            if ( nDecExists & 0x18 ) // bit 4, 3
                return 1;
            nDecAndExists = Dau_DsdCheckDecAndExist( pDsdStr );
            if ( nDecAndExists & 0x1C ) // bit 4, 3, 2
                return 1;
        }
        return If_CutPerformCheck07( p, pTruth, nVars, nLeaves, pStr );
    }
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Performs additional check.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutPerformCheck75( If_Man_t * p, unsigned * pTruth0, int nVars, int nLeaves, char * pStr )
{
    word * pTruthW = (word *)pTruth0;
    word pTruth[4] = { pTruthW[0], pTruthW[1], pTruthW[2], pTruthW[3] };
    assert( nLeaves <= 8 );
    if ( !p->pPars->fCutMin )
        Abc_TtMinimumBase( pTruth, NULL, nLeaves, &nLeaves );
    if ( nLeaves < 6 )
        return 1;
//    if ( nLeaves < 8 && If_CutPerformCheck07( p, (unsigned *)pTruth, nVars, nLeaves, "44" ) )
    if ( nLeaves < 8 && If_CutPerformCheck16( p, (unsigned *)pTruth, nVars, nLeaves, "44" ) )
        return 1;
    // this is faster but not compatible with -z
    if ( !p->pPars->fDeriveLuts && p->pPars->fEnableCheck75 && nLeaves == 8 )
    {
//        char pDsdStr[1000] = "(!(abd)!(c!([fe][gh])))";
        char pDsdStr[1000];
        int nSizeNonDec = Dau_DsdDecompose( (word *)pTruth, nLeaves, 0, 0, pDsdStr );
        if ( nSizeNonDec >= 5 )
            return 0;
        if ( Dau_DsdCheckDecAndExist(pDsdStr) & 0x10 ) // bit 4
            return 1;
        return 0;
    }
    if ( If_CutPerformCheck45( p, (unsigned *)pTruth, nVars, nLeaves, pStr ) )
        return 1;
    if ( If_CutPerformCheck54( p, (unsigned *)pTruth, nVars, nLeaves, pStr ) )
        return 1;
    return 0;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

